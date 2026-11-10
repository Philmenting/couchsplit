// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedInventory.h"
#include <optional>
#include <utility>

namespace coop::firered {

inline constexpr uint32_t PokemonStoragePointerAddress = 0x03004F60;
inline constexpr unsigned BoxMonSize = 80;
inline constexpr unsigned BoxCount = 14;
inline constexpr unsigned BoxCapacity = 30;
inline constexpr unsigned StorageMonCount = BoxCount * BoxCapacity;
// BoxPokemon has four-byte alignment: the first box starts at 4, despite the
// historical decomp header's misleading 0x0001 field comment. The documented
// following field 0x8344 independently confirms 4 + 14 * 30 * 80.
inline constexpr uint32_t StorageBoxesOffset = 4;
inline constexpr uint32_t StorageNamesOffset = 0x8344;
inline constexpr uint32_t PokemonStorageSize = 0x83D0;

struct BoxMon {
    std::array<uint8_t, BoxMonSize> bytes{};
    bool empty() const { return emptyNativeBoxRecord(bytes.data()); }
    MonIdentity identity() const { return {monU32(bytes.data()), monU32(bytes.data() + 4)}; }
    PartyMon asPartyRecord() const {
        PartyMon result;
        std::copy(bytes.begin(), bytes.end(), result.bytes.begin());
        if (!empty()) {
            // A box record has no level/HP/stats. These local placeholders only
            // allow reuse of box-format checks; never commit them as a party.
            result.bytes[84] = 1;
            monPut16(result.bytes.data() + 86, 1); monPut16(result.bytes.data() + 88, 1);
        }
        return result;
    }
    void validate() const { asPartyRecord().validate(); }
    static BoxMon fromParty(const PartyMon& party) {
        party.validate();
        BoxMon result;
        std::copy_n(party.bytes.begin(), BoxMonSize, result.bytes.begin());
        return result;
    }
    bool operator==(const BoxMon& other) const { return bytes == other.bytes; }
};
static_assert(sizeof(BoxMon) == BoxMonSize, "Box Pokemon must preserve all 80 bytes");

inline constexpr uint32_t NationalSpeciesTableAddress = 0x08251F12;
inline unsigned nativeNationalSpecies(mCore* core, unsigned species) {
    requirePokemon(core && core->busRead8 && species >= 1 && species < 412 &&
                   !(species >= 252 && species <= 276), "Invalid FireRed captured species");
    // Read the exact supported ROM's native mapping. Hoenn internal IDs are
    // reordered; subtracting 25 gives wrong dex flags for many species.
    const auto national = inventoryRead16(core, abiAddress(core, NationalSpeciesTableAddress) + (species - 1) * 2);
    requirePokemon(national >= 1 && national <= 386, "Invalid FireRed native national dex mapping");
    return national;
}

struct TrainerIdentity {
    uint32_t id = 0;
    std::array<uint8_t, 7> name{};
    uint8_t gender = 0;
    void validate() const { requirePokemon(gender <= 1, "Invalid FireRed trainer gender"); }
};
inline TrainerIdentity readTrainerIdentity(mCore* core) {
    const auto memory = readSaveMemory(core);
    TrainerIdentity trainer;
    trainer.id = inventoryRead32(core, memory.block2 + 0xA);
    for (unsigned i = 0; i < trainer.name.size(); ++i)
        trainer.name[i] = uint8_t(core->busRead8(core, memory.block2 + i));
    trainer.gender = uint8_t(core->busRead8(core, memory.block2 + 8));
    checkSaveMemory(core, memory); trainer.validate();
    return trainer;
}
inline PartyMon assignCaptureTrainer(const PartyMon& capture, const TrainerIdentity& trainer) {
    capture.validate(); trainer.validate();
    requirePokemon(!capture.empty() && !capture.egg(), "Invalid newly captured FireRed Pokemon");
    auto result = capture;
    auto secure = capture.secure(); // Decrypt under the original OT key FIRST.
    monPut32(result.bytes.data() + 4, trainer.id);
    std::copy(trainer.name.begin(), trainer.name.end(), result.bytes.begin() + 20);
    const auto origins = uint16_t((monU16(secure.data() + 38) & 0x7FFFu) | (uint16_t(trainer.gender) << 15));
    monPut16(secure.data() + 38, origins);
    result.setSecure(secure); // Recompute checksum and encrypt under actual OT.
    result.validate();
    return result;
}
inline BoxMon assignCaptureTrainer(const BoxMon& capture, const TrainerIdentity& trainer) {
    capture.validate();
    return BoxMon::fromParty(assignCaptureTrainer(capture.asPartyRecord(), trainer));
}

struct StorageSnapshot {
    std::array<uint8_t, PokemonStorageSize> bytes{};
    unsigned currentBox() const { return bytes[0]; }
    static uint32_t slotOffset(unsigned box, unsigned slot) {
        requirePokemon(box < BoxCount && slot < BoxCapacity, "FireRed PC slot is out of range");
        return StorageBoxesOffset + (box * BoxCapacity + slot) * BoxMonSize;
    }
    BoxMon mon(unsigned box, unsigned slot) const {
        BoxMon result;
        std::copy_n(bytes.begin() + slotOffset(box, slot), BoxMonSize, result.bytes.begin());
        return result;
    }
    void setMon(unsigned box, unsigned slot, const BoxMon& value) {
        value.validate();
        std::copy(value.bytes.begin(), value.bytes.end(), bytes.begin() + slotOffset(box, slot));
    }
    void validate() const {
        requirePokemon(currentBox() < BoxCount, "Invalid FireRed current PC box");
        for (unsigned box = 0; box < BoxCount; ++box) {
            requirePokemon(bytes[0x83C2 + box] < 16, "Invalid FireRed PC wallpaper");
            for (unsigned slot = 0; slot < BoxCapacity; ++slot) mon(box, slot).validate();
        }
    }
    bool operator==(const StorageSnapshot& other) const { return bytes == other.bytes; }
};
struct StorageMemory {
    SaveMemory save{};
    uint32_t address = 0;
};
inline StorageMemory readStorageMemory(mCore* core) {
    StorageMemory result;
    result.save = readSaveMemory(core);
    result.address = inventoryRead32(core, abiAddress(core, PokemonStoragePointerAddress));
    requirePokemon(validSaveBlock(result.address, PokemonStorageSize), "FireRed PC storage is uninitialized or outside EWRAM");
    requirePokemon((result.address + PokemonStorageSize <= result.save.block1 || result.save.block1 + SaveBlock1Size <= result.address) &&
                   (result.address + PokemonStorageSize <= result.save.block2 || result.save.block2 + SaveBlock2Size <= result.address),
                   "FireRed PC storage overlaps trainer save blocks");
    return result;
}
inline void checkStorageMemory(mCore* core, const StorageMemory& expected) {
    const auto current = readStorageMemory(core);
    requirePokemon(current.save == expected.save && current.address == expected.address,
                   "FireRed PC storage moved during transaction");
}
inline StorageSnapshot readStorage(mCore* core) {
    const auto memory = readStorageMemory(core);
    StorageSnapshot result;
    for (unsigned offset = 0; offset < result.bytes.size(); ++offset)
        result.bytes[offset] = uint8_t(core->busRead8(core, memory.address + offset));
    checkStorageMemory(core, memory);
    result.validate();
    return result;
}
inline void writeStorage(mCore* core, const StorageSnapshot& storage) {
    storage.validate();
    requirePokemon(core && core->busRead8 && core->busWrite8, "Cannot write FireRed PC without an emulator core");
    const auto memory = readStorageMemory(core);
    checkStorageMemory(core, memory);
    // Capture ownership changes only box contents. Never overwrite another
    // trainer's current-box selection, box names, wallpapers or struct padding.
    for (unsigned offset = StorageBoxesOffset; offset < StorageNamesOffset; ++offset)
        core->busWrite8(core, memory.address + offset, storage.bytes[offset]);
}

struct CaptureAddition {
    unsigned box = 0, slot = 0;
    BoxMon pokemon{};
};
inline std::optional<CaptureAddition> captureAddition(const StorageSnapshot& before, const StorageSnapshot& after) {
    before.validate(); after.validate();
    // The native capture may select another current box. Other PC metadata is
    // unrelated to a battle and must not have changed in this transaction.
    requirePokemon(std::equal(before.bytes.begin() + 1, before.bytes.begin() + StorageBoxesOffset, after.bytes.begin() + 1) &&
                   std::equal(before.bytes.begin() + StorageNamesOffset, before.bytes.end(), after.bytes.begin() + StorageNamesOffset),
                   "FireRed capture unexpectedly changed PC names, wallpaper or padding");
    std::optional<CaptureAddition> addition;
    for (unsigned box = 0; box < BoxCount; ++box)
        for (unsigned slot = 0; slot < BoxCapacity; ++slot) {
            const auto oldMon = before.mon(box, slot), newMon = after.mon(box, slot);
            if (oldMon == newMon) continue;
            requirePokemon(oldMon.empty() && !newMon.empty(), "FireRed capture changed or removed an existing PC Pokemon");
            requirePokemon(!addition, "FireRed capture added more than one PC Pokemon");
            addition = CaptureAddition{box, slot, newMon};
        }
    if (addition)
        for (unsigned box = 0; box < BoxCount; ++box)
            for (unsigned slot = 0; slot < BoxCapacity; ++slot) {
                const auto original = before.mon(box, slot);
                requirePokemon(original.empty() || original.identity() != addition->pokemon.identity(),
                               "Captured FireRed Pokemon has an ambiguous existing PC identity");
            }
    return addition;
}
inline CaptureAddition detectCaptureAddition(const StorageSnapshot& before, const StorageSnapshot& after) {
    const auto result = captureAddition(before, after);
    requirePokemon(bool(result), "Expected exactly one native FireRed PC capture addition");
    return *result;
}
inline StorageSnapshot removeCaptureAddition(const StorageSnapshot& after, const CaptureAddition& capture) {
    after.validate(); capture.pokemon.validate();
    requirePokemon(!capture.pokemon.empty() && after.mon(capture.box, capture.slot) == capture.pokemon,
                   "FireRed PC capture removal does not match its snapshot");
    auto result = after;
    result.setMon(capture.box, capture.slot, BoxMon{});
    return result;
}
inline std::pair<StorageSnapshot, CaptureAddition> addCapture(const StorageSnapshot& owner, const BoxMon& pokemon) {
    owner.validate(); pokemon.validate();
    requirePokemon(!pokemon.empty() && !pokemon.asPartyRecord().egg(), "Invalid captured FireRed PC Pokemon");
    for (unsigned box = 0; box < BoxCount; ++box)
        for (unsigned slot = 0; slot < BoxCapacity; ++slot) {
            const auto existing = owner.mon(box, slot);
            requirePokemon(existing.empty() || existing.identity() != pokemon.identity(),
                           "Captured FireRed Pokemon already exists in owner's PC");
        }
    for (unsigned step = 0; step < BoxCount; ++step) {
        const auto box = (owner.currentBox() + step) % BoxCount;
        for (unsigned slot = 0; slot < BoxCapacity; ++slot)
            if (owner.mon(box, slot).empty()) {
                auto result = owner;
                result.setMon(box, slot, pokemon);
                return {result, CaptureAddition{box, slot, pokemon}};
            }
    }
    throw std::runtime_error("FireRed capture owner's PC is full; original storage preserved");
}

} // namespace coop::firered
