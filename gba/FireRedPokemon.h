// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <mgba/core/core.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <optional>
#include <vector>
#include "FireRedErrors.h"

namespace coop::firered {

// German FireRed original only. The caller must independently verify the ROM
// fingerprint and stop emulation while reading or committing a party snapshot.
inline constexpr uint32_t PlayerPartyAddress = 0x02024284;
inline constexpr uint32_t PlayerPartyCountAddress = 0x02024029;
inline constexpr std::size_t PartyCapacity = 6;
inline constexpr std::size_t PartyMonSize = 100;
inline constexpr unsigned ExpShareItem = 182;
inline constexpr unsigned EnigmaBerryItem = 175;

inline void requirePokemon(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
inline uint16_t monU16(const uint8_t* p) {
    return uint16_t(p[0]) | uint16_t(uint16_t(p[1]) << 8);
}
inline uint32_t monU32(const uint8_t* p) {
    return uint32_t(monU16(p)) | (uint32_t(monU16(p + 2)) << 16);
}
inline void monPut16(uint8_t* p, uint16_t value) {
    p[0] = uint8_t(value); p[1] = uint8_t(value >> 8);
}
inline void monPut32(uint8_t* p, uint32_t value) {
    monPut16(p, uint16_t(value)); monPut16(p + 2, uint16_t(value >> 16));
}
inline bool emptyNativeBoxRecord(const uint8_t* bytes) {
    // The native game leaves name terminators and unused party bytes in empty
    // slots. PID/OT, sanity bits, checksum and the encrypted payload establish
    // emptiness; requiring an all-zero100-byte record rejects ordinary saves.
    return monU32(bytes) == 0 && monU32(bytes + 4) == 0 && (bytes[19] & 7u) == 0 &&
        monU16(bytes + 28) == 0 && std::all_of(bytes + 32, bytes + 80, [](uint8_t b) { return b == 0; });
}

struct MonIdentity {
    uint32_t personality = 0;
    uint32_t trainer = 0;
    bool operator==(const MonIdentity& other) const {
        return personality == other.personality && trainer == other.trainer;
    }
    bool operator!=(const MonIdentity& other) const { return !(*this == other); }
};

struct PartyMon {
    std::array<uint8_t, PartyMonSize> bytes{};
    using Secure = std::array<uint8_t, 48>; // Logical Growth/Attacks/EVs/Misc order.

    bool empty() const {
        return emptyNativeBoxRecord(bytes.data());
    }
    MonIdentity identity() const { return {monU32(bytes.data()), monU32(bytes.data() + 4)}; }

    // The physical block order is the lexicographic permutation of G,A,E,M
    // selected by personality % 24. Compute it rather than reusing game code.
    std::array<unsigned, 4> order() const {
        std::array<unsigned, 4> result{{0, 1, 2, 3}};
        const auto permutation = identity().personality % 24;
        for (unsigned i = 0; i < permutation; ++i)
            std::next_permutation(result.begin(), result.end());
        return result;
    }
    Secure secure() const {
        Secure plain{};
        const auto blocks = order();
        const auto id = identity();
        for (unsigned physical = 0; physical < 4; ++physical)
            for (unsigned word = 0; word < 3; ++word) {
                const auto encrypted = monU32(bytes.data() + 32 + physical * 12 + word * 4);
                monPut32(plain.data() + blocks[physical] * 12 + word * 4,
                         encrypted ^ id.personality ^ id.trainer);
            }
        return plain;
    }
    static uint16_t checksum(const Secure& plain) {
        uint32_t total = 0;
        for (unsigned offset = 0; offset < plain.size(); offset += 2)
            total += monU16(plain.data() + offset);
        return uint16_t(total);
    }
    // Also useful for authored test fixtures; runtime callers should preserve
    // the original bytes unless a native game result is being committed.
    void setSecure(const Secure& plain) {
        const auto blocks = order();
        const auto id = identity();
        monPut16(bytes.data() + 28, checksum(plain));
        for (unsigned physical = 0; physical < 4; ++physical)
            for (unsigned word = 0; word < 3; ++word)
                monPut32(bytes.data() + 32 + physical * 12 + word * 4,
                         monU32(plain.data() + blocks[physical] * 12 + word * 4)
                         ^ id.personality ^ id.trainer);
    }
    uint16_t species() const { const auto p = secure(); return monU16(p.data()); }
    uint16_t heldItem() const { const auto p = secure(); return monU16(p.data() + 2); }
    uint32_t experience() const { const auto p = secure(); return monU32(p.data() + 4); }
    bool egg() const { const auto p = secure(); return (monU32(p.data() + 40) & 0x40000000u) != 0; }
    uint32_t status() const { return monU32(bytes.data() + 80); }
    uint8_t level() const { return bytes[84]; }
    uint16_t hp() const { return monU16(bytes.data() + 86); }
    uint16_t maxHp() const { return monU16(bytes.data() + 88); }
    uint16_t move(unsigned slot) const {
        requirePokemon(slot < 4, "FireRed move index is out of range");
        const auto p = secure(); return monU16(p.data() + 12 + slot * 2);
    }
    uint8_t pp(unsigned slot) const {
        requirePokemon(slot < 4, "FireRed PP index is out of range");
        return secure()[20 + slot];
    }
    bool viable() const { return !empty() && !egg() && hp() != 0; }
    void setHp(uint16_t value) {
        requirePokemon(value <= maxHp(), "FireRed HP exceeds maximum HP");
        monPut16(bytes.data() + 86, value);
    }
    void setStatus(uint32_t value) {
        requirePokemon((value & ~0xFFFu) == 0, "Invalid FireRed persistent status");
        monPut32(bytes.data() + 80, value);
    }
    void setPp(unsigned slot, uint8_t value) {
        requirePokemon(slot < 4 && value <= 64, "Invalid FireRed move PP");
        auto p = secure();
        requirePokemon(monU16(p.data() + 12 + slot * 2) != 0 || value == 0,
                       "Empty FireRed move cannot have PP");
        p[20 + slot] = value; setSecure(p);
    }
    void validate() const {
        if (empty()) return;
        const auto p = secure();
        requirePokemon(checksum(p) == monU16(bytes.data() + 28), "FireRed Pokemon checksum mismatch");
        requirePokemon((bytes[19] & 1) == 0, "FireRed Bad Egg is not a valid party Pokemon");
        const auto s = monU16(p.data());
        requirePokemon(s > 0 && s < 412 && !(s >= 252 && s <= 276), "Invalid FireRed Pokemon species");
        requirePokemon((bytes[19] & 2) != 0, "FireRed Pokemon is missing its species flag");
        requirePokemon(bool(bytes[19] & 4) == egg(), "Inconsistent FireRed Egg flags");
        requirePokemon(bytes[18] == 1 || (bytes[18] >= 2 && bytes[18] <= 5) || bytes[18] == 7,
                       "Invalid FireRed Pokemon language");
        requirePokemon(level() >= 1 && level() <= 100, "Invalid FireRed Pokemon level");
        requirePokemon(experience() <= 1640000, "Invalid FireRed Pokemon experience");
        requirePokemon(maxHp() > 0 && hp() <= maxHp(), "Invalid FireRed Pokemon HP");
        requirePokemon((status() & ~0xFFFu) == 0, "Invalid FireRed persistent status");
        requirePokemon(heldItem() < 375, "Invalid FireRed held item");
        for (unsigned slot = 0; slot < 4; ++slot) {
            const auto m = monU16(p.data() + 12 + slot * 2);
            requirePokemon(m < 355 && p[20 + slot] <= 64 && (m != 0 || p[20 + slot] == 0),
                           "Invalid FireRed move or PP");
        }
        unsigned evs = 0;
        for (unsigned i = 24; i < 30; ++i) evs += p[i];
        requirePokemon(evs <= 510, "Invalid FireRed total EVs");
    }
    bool operator==(const PartyMon& other) const { return bytes == other.bytes; }
};
static_assert(sizeof(PartyMon) == PartyMonSize, "Party Pokemon must preserve all 100 bytes");

struct Party {
    std::array<PartyMon, PartyCapacity> mons{};
    uint8_t count = 0;
    void validate() const {
        requirePokemon(count <= PartyCapacity, "Invalid FireRed party count");
        for (unsigned slot = 0; slot < PartyCapacity; ++slot) {
            mons[slot].validate();
            requirePokemon(mons[slot].empty() == (slot >= count), "FireRed party has gaps or a stale count");
            if (slot < count)
                for (unsigned prior = 0; prior < slot; ++prior)
                    requirePokemon(mons[prior].identity() != mons[slot].identity(),
                                   "Ambiguous duplicate FireRed Pokemon identity");
        }
    }
    bool operator==(const Party& other) const { return count == other.count && mons == other.mons; }
};

inline Party readParty(mCore* core) {
    requirePokemon(core && core->busRead8, "Cannot read FireRed party without an emulator core");
    Party result;
    result.count = uint8_t(core->busRead8(core, PlayerPartyCountAddress));
    for (unsigned slot = 0; slot < PartyCapacity; ++slot)
        for (unsigned offset = 0; offset < PartyMonSize; ++offset)
            result.mons[slot].bytes[offset] = uint8_t(core->busRead8(core, PlayerPartyAddress + slot * PartyMonSize + offset));
    result.validate();
    return result;
}
inline void writeParty(mCore* core, const Party& party) {
    // Perform every validation before the first bus write. This routine never
    // touches flash/battery files. The caller owns the paused-core transaction.
    party.validate();
    requirePokemon(core && core->busWrite8, "Cannot write FireRed party without an emulator core");
    for (unsigned slot = 0; slot < PartyCapacity; ++slot)
        for (unsigned offset = 0; offset < PartyMonSize; ++offset)
            core->busWrite8(core, PlayerPartyAddress + slot * PartyMonSize + offset, party.mons[slot].bytes[offset]);
    core->busWrite8(core, PlayerPartyCountAddress, party.count);
}

struct BattleParty {
    std::array<Party, 2> originals{};
    Party combined{};
    // The first two native slots are the two human leads. Later slots are
    // round-robin until one side is exhausted, then appended contiguously.
    // Never derive switch ownership from parity: use this explicit mapping.
    std::array<int, PartyCapacity> owners{{-1, -1, -1, -1, -1, -1}};
    std::array<int, PartyCapacity> originalSlots{{-1, -1, -1, -1, -1, -1}};
};

inline BattleParty combineParties(const Party& first, const Party& second,
                                  const std::array<std::vector<unsigned>, 2>& selected) {
    first.validate(); second.validate();
    // Enigma Berry effects come from each trainer's save rather than the item
    // table. A shared battle runs on one trainer's core, so accepting either
    // owner's berry can silently apply the wrong effect or EXP split. Reject
    // before any machine mutation until per-owner effects are implemented.
    for (const auto* party : {&first, &second})
        for (unsigned slot = 0; slot < party->count; ++slot)
            if (party->mons[slot].heldItem() == EnigmaBerryItem) throw EnigmaBerryError();
    requirePokemon(!selected[0].empty() && !selected[1].empty() &&
                   selected[0].size() <= 5 && selected[1].size() <= 5 &&
                   selected[0].size() + selected[1].size() <= PartyCapacity,
                   "FireRed team battle exceeds its six native Pokemon slots");
    // Cloned PID/OT pairs cannot be assigned safely after party-menu reordering.
    for (unsigned a = 0; a < first.count; ++a)
        for (unsigned b = 0; b < second.count; ++b)
            requirePokemon(first.mons[a].identity() != second.mons[b].identity(),
                           "Players have ambiguous duplicate FireRed Pokemon identities");
    BattleParty result;
    result.originals = {{first, second}};
    unsigned destination = 0;
    for (unsigned index = 0; index < PartyCapacity; ++index) {
        for (unsigned owner = 0; owner < 2; ++owner) {
            if (index >= selected[owner].size()) continue;
            const auto source = selected[owner][index];
            requirePokemon(source < result.originals[owner].count, "Selected FireRed party slot is out of range");
            requirePokemon(result.originals[owner].mons[source].viable(), "Selected FireRed Pokemon cannot battle");
            for (unsigned earlier = 0; earlier < index; ++earlier)
                requirePokemon(selected[owner][earlier] != source, "Duplicate selected FireRed party slot");
            const auto slot = destination++;
            result.owners[slot] = int(owner);
            result.combined.mons[slot] = result.originals[owner].mons[source];
            result.originalSlots[slot] = int(source);
        }
    }
    result.combined.count = uint8_t(destination);
    result.combined.validate();
    return result;
}
inline BattleParty combineParties(const Party& first, const Party& second) {
    std::array<std::vector<unsigned>, 2> selected;
    const std::array<const Party*, 2> parties{{&first, &second}};
    first.validate(); second.validate();
    for (unsigned owner = 0; owner < 2; ++owner) {
        const auto& party = *parties[owner];
        // Keep the normal first usable lead. Every living EXP Share holder
        // must also be resident when the native game calculates the split;
        // otherwise both its award and everyone else's share are incorrect.
        unsigned lead = 0;
        while (lead < party.count && !party.mons[lead].viable()) ++lead;
        requirePokemon(lead < party.count, "FireRed team has no usable lead");
        selected[owner].push_back(lead);
        for (unsigned slot = 0; slot < party.count; ++slot) {
            if (slot == lead || !party.mons[slot].viable() || party.mons[slot].heldItem() != ExpShareItem) continue;
            selected[owner].push_back(slot);
        }
    }
    // More than six mandatory records cannot be represented by the native
    // battle engine. Stop before converting either save instead of silently
    // miscalculating every EXP Share award.
    if (selected[0].size() + selected[1].size() > PartyCapacity)
        throw ExpShareCapacityError();
    // Use the remaining slots for normal reserves, preferring the player with
    // fewer loaded members. Preserve the established three-per-player working
    // set when there are no exceptional EXP Share requirements.
    while (selected[0].size() + selected[1].size() < PartyCapacity) {
        bool added = false;
        const unsigned firstOwner = selected[0].size() <= selected[1].size() ? 0 : 1;
        for (const unsigned owner : {firstOwner, 1u - firstOwner}) {
            if (selected[owner].size() >= 3) continue;
            const auto& party = *parties[owner];
            for (unsigned slot = 0; slot < party.count; ++slot) {
                if (!party.mons[slot].viable() ||
                    std::find(selected[owner].begin(), selected[owner].end(), slot) != selected[owner].end()) continue;
                selected[owner].push_back(slot); added = true; break;
            }
            if (added) break;
        }
        if (!added) break;
    }
    return combineParties(first, second, selected);
}

inline std::array<Party, 2> restoreParties(const BattleParty& transaction, const Party& battleResult) {
    transaction.originals[0].validate(); transaction.originals[1].validate();
    transaction.combined.validate(); battleResult.validate();
    requirePokemon(transaction.combined.count >= 2,
                   "Invalid FireRed battle transaction size");
    requirePokemon(battleResult.count == transaction.combined.count,
                   "FireRed battle result changed party size; capture requires a separate ownership transaction");
    auto result = transaction.originals;
    std::array<bool, PartyCapacity> found{};
    for (unsigned slot = 0; slot < transaction.combined.count; ++slot) {
        requirePokemon(transaction.owners[slot] == 0 || transaction.owners[slot] == 1,
                       "Invalid FireRed battle Pokemon owner");
        const auto owner = unsigned(transaction.owners[slot]);
        const auto original = transaction.originalSlots[slot];
        requirePokemon(original >= 0 && original < transaction.originals[owner].count,
                       "Invalid FireRed battle ownership mapping");
        const auto& before = transaction.originals[owner].mons[unsigned(original)];
        requirePokemon(transaction.combined.mons[slot] == before,
                       "FireRed battle transaction snapshot was changed");
        for (unsigned earlier = 0; earlier < slot; ++earlier)
            requirePokemon(transaction.owners[earlier] != int(owner) || transaction.originalSlots[earlier] != original,
                           "Duplicate FireRed battle ownership mapping");
        int match = -1;
        for (unsigned current = 0; current < battleResult.count; ++current)
            if (battleResult.mons[current].identity() == before.identity()) match = int(current);
        requirePokemon(match >= 0 && !found[unsigned(match)], "Missing or foreign FireRed Pokemon in battle result");
        found[unsigned(match)] = true;
        // PID/OT establishes ownership; a battle may change species, names,
        // moves, experience, EVs, friendship, held item, status, HP and stats.
        // Origin/IVs/egg/ability bits cannot change as an ordinary battle result.
        // Pokerus and ribbons can change at battle completion and are retained.
        const auto oldSecure = before.secure();
        const auto newSecure = battleResult.mons[unsigned(match)].secure();
        requirePokemon(std::equal(oldSecure.begin() + 37, oldSecure.begin() + 44, newSecure.begin() + 37),
                       "FireRed battle result changed Pokemon origin or IV identity");
        const auto& after = battleResult.mons[unsigned(match)];
        requirePokemon(after.experience() >= before.experience() && after.level() >= before.level(),
                       "FireRed battle result decreased experience or level");
        for (unsigned stat = 24; stat < 30; ++stat)
            requirePokemon(newSecure[stat] >= oldSecure[stat], "FireRed battle result decreased EVs");
        result[owner].mons[unsigned(original)] = after;
    }
    requirePokemon(transaction.owners[0] == 0 && transaction.owners[1] == 1, "Invalid FireRed team lead ownership");
    for (unsigned owner = 0; owner < 2; ++owner) {
        unsigned ownedCount = 0;
        for (unsigned slot = 0; slot < transaction.combined.count; ++slot)
            ownedCount += transaction.owners[slot] == int(owner);
        requirePokemon(ownedCount > 0 && ownedCount <= 5, "Invalid FireRed team ownership count");
    }
    for (unsigned slot = transaction.combined.count; slot < PartyCapacity; ++slot)
        requirePokemon(transaction.originalSlots[slot] == -1 && transaction.owners[slot] == -1,
                       "Unexpected unused FireRed ownership mapping");
    result[0].validate(); result[1].validate();
    return result;
}


// Rotate a previously unselected original Pokemon into an exhausted owner's
// fainted native BENCH slot. The six native slots are a working set; originals
// is also the journal for all twelve Pokemon's latest validated battle data.
// The coordinator must exclude both active battlers, every queued switch and
// every native index still referenced by an in-flight choice. It must also
// clear the retired slot's native experience-participation bookkeeping before
// the new Pokemon is introduced; this pure helper cannot alter battle RAM.
inline std::optional<Party> refillBattleBench(BattleParty& transaction, const Party& current,
                                             unsigned owner, const std::vector<unsigned>& excludedNativeSlots) {
    requirePokemon(owner < 2, "Invalid FireRed bench refill owner");
    // Validate all incoming Pokemon and results before planning any mutation.
    const auto journal = restoreParties(transaction, current);
    std::array<bool, PartyCapacity> excluded{};
    for (const auto slot : excludedNativeSlots) {
        requirePokemon(slot <= PartyCapacity || slot == 0xFF, "Invalid FireRed excluded native party index");
        if (slot < PartyCapacity) excluded[slot] = true; //6/0xFF are native sentinels.
    }
    BattleParty planned = transaction;
    planned.originals = journal;
    planned.combined = current;
    // Native menus can reorder records. Rebase ownership by identity, not by
    // an old physical position or parity, before selecting the replacement.
    for (unsigned slot = 0; slot < current.count; ++slot) {
        int previous = -1;
        for (unsigned original = 0; original < transaction.combined.count; ++original)
            if (current.mons[slot].identity() == transaction.combined.mons[original].identity()) previous = int(original);
        requirePokemon(previous >= 0, "Foreign FireRed Pokemon appeared during bench refill");
        planned.owners[slot] = transaction.owners[unsigned(previous)];
        planned.originalSlots[slot] = transaction.originalSlots[unsigned(previous)];
    }
    // Use an already-loaded living bench member before importing another one.
    for (unsigned slot = 0; slot < current.count; ++slot)
        if (planned.owners[slot] == int(owner) && !excluded[slot] && current.mons[slot].viable()) return std::nullopt;
    int destination = -1;
    for (unsigned slot = 0; slot < current.count; ++slot)
        if (planned.owners[slot] == int(owner) && !excluded[slot] && current.mons[slot].hp() == 0) {
            destination = int(slot); break;
        }
    if (destination < 0) return std::nullopt;
    int source = -1;
    for (unsigned original = 0; original < journal[owner].count; ++original) {
        if (!journal[owner].mons[original].viable()) continue;
        bool loaded = false;
        for (unsigned slot = 0; slot < current.count; ++slot)
            loaded |= planned.owners[slot] == int(owner) && planned.originalSlots[slot] == int(original);
        if (!loaded) { source = int(original); break; }
    }
    if (source < 0) return std::nullopt;
    planned.originalSlots[unsigned(destination)] = source;
    planned.combined.mons[unsigned(destination)] = journal[owner].mons[unsigned(source)];
    // Check the rebased journal and new working set together before publishing
    // either value. The retired fainted Pokemon remains in its original slot.
    requirePokemon(restoreParties(planned, planned.combined) == journal,
                   "FireRed bench refill changed original party ownership");
    transaction = planned;
    return planned.combined;
}
template<std::size_t N>
inline std::optional<Party> refillBattleBench(BattleParty& transaction, const Party& current,
                                             unsigned owner, const std::array<unsigned, N>& excludedNativeSlots) {
    return refillBattleBench(transaction, current, owner,
                             std::vector<unsigned>(excludedNativeSlots.begin(), excludedNativeSlots.end()));
}

} // namespace coop::firered
