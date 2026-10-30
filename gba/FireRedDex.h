// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedStorage.h"

namespace coop::firered {

inline constexpr uint32_t DexOwnedOffset = 0x28;   // SaveBlock2.
inline constexpr uint32_t DexSeenOffset = 0x5C;    // SaveBlock2.
inline constexpr uint32_t DexSeen1Offset = 0x5F8;  // SaveBlock1.
inline constexpr uint32_t DexSeen2Offset = 0x3A18; // SaveBlock1.
inline constexpr uint32_t DexUnownPersonalityOffset = 0x1C; // SaveBlock2.
inline constexpr uint32_t DexSpindaPersonalityOffset = 0x20; // SaveBlock2.
inline constexpr unsigned NationalUnown = 201;
inline constexpr unsigned NationalSpinda = 327;

struct DexEntry {
    // Complete containing bytes preserve all unrelated species bits.
    std::array<uint8_t, 3> seen{};
    uint8_t owned = 0;
    uint8_t mask = 0;
    uint32_t personality = 0;
    bool formSpecies = false;
    bool isSeen() const {
        return (seen[0] & mask) && (seen[1] & mask) && (seen[2] & mask);
    }
    bool isCaught() const { return (owned & mask) && isSeen(); }
};
struct DexUpdate {
    DexEntry entry{};
    bool writePersonality = false;
};
inline DexUpdate planDexUpdate(const DexEntry& before, uint32_t personality, bool caught) {
    requirePokemon(before.mask && (before.mask & (before.mask - 1)) == 0, "Invalid FireRed dex entry bit");
    DexUpdate result{before, false};
    const bool previouslyMarked = caught ? before.isCaught() : before.isSeen();
    for (auto& seen : result.entry.seen) seen |= before.mask;
    if (caught) result.entry.owned |= before.mask;
    // Native HandleSetPokedexFlag records special forms on the first matching
    // event. A first catch can replace a previously seen form; subsequent seen
    // and caught events preserve it. Do not test personality==0 as a sentinel:
    // zero is a legitimate Pokemon personality.
    if (before.formSpecies && !previouslyMarked) {
        result.entry.personality = personality;
        result.writePersonality = true;
    }
    return result;
}
inline std::array<uint32_t, 4> dexEntryAddresses(const SaveMemory& save, unsigned national) {
    requirePokemon(national >= 1 && national <= 386, "Invalid FireRed national dex entry");
    const auto index = (national - 1) / 8;
    return {{save.block2 + DexSeenOffset + index, save.block1 + DexSeen1Offset + index,
             save.block1 + DexSeen2Offset + index, save.block2 + DexOwnedOffset + index}};
}
inline DexEntry readDexEntry(mCore* core, const SaveMemory& save, unsigned national) {
    requirePokemon(core && core->busRead8, "Cannot read FireRed dex without an emulator core");
    checkSaveMemory(core, save);
    const auto addresses = dexEntryAddresses(save, national);
    DexEntry result;
    result.mask = uint8_t(1u << ((national - 1) % 8));
    for (unsigned i = 0; i < 3; ++i) result.seen[i] = uint8_t(core->busRead8(core, addresses[i]));
    result.owned = uint8_t(core->busRead8(core, addresses[3]));
    result.formSpecies = national == NationalUnown || national == NationalSpinda;
    if (result.formSpecies)
        result.personality = inventoryRead32(core, save.block2 +
                            (national == NationalUnown ? DexUnownPersonalityOffset : DexSpindaPersonalityOffset));
    checkSaveMemory(core, save);
    return result;
}
inline DexEntry readDexEntry(mCore* core, unsigned species) {
    const auto national = nativeNationalSpecies(core, species);
    const auto save = readSaveMemory(core);
    return readDexEntry(core, save, national);
}
inline void markDex(mCore* core, unsigned species, uint32_t personality, bool caught) {
    requirePokemon(core && core->busRead8 && core->busWrite8, "Cannot update FireRed dex without an emulator core");
    const auto national = nativeNationalSpecies(core, species);
    const auto save = readSaveMemory(core);
    const auto addresses = dexEntryAddresses(save, national);
    const auto before = readDexEntry(core, save, national);
    const auto update = planDexUpdate(before, personality, caught);
    // Validate the complete plan/location before the first commit. RAM stays
    // paused across this helper, like party/inventory/storage transactions.
    checkSaveMemory(core, save);
    if (update.writePersonality) {
        const auto address = save.block2 + (national == NationalUnown ? DexUnownPersonalityOffset : DexSpindaPersonalityOffset);
        for (unsigned i = 0; i < 4; ++i)
            core->busWrite8(core, address + i, uint8_t(update.entry.personality >> (i * 8)));
    }
    for (unsigned i = 0; i < 3; ++i)
        if (update.entry.seen[i] != before.seen[i]) core->busWrite8(core, addresses[i], update.entry.seen[i]);
    // Commit owned last, after all three anti-cheat seen copies agree.
    if (update.entry.owned != before.owned) core->busWrite8(core, addresses[3], update.entry.owned);
}
inline void markSeen(mCore* core, unsigned species, uint32_t personality) { markDex(core, species, personality, false); }
inline void markCaught(mCore* core, unsigned species, uint32_t personality) { markDex(core, species, personality, true); }

} // namespace coop::firered
