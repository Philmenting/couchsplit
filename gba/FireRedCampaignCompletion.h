// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedCampaignRuntime.h"

namespace coop::firered {

// Append these commands to an already preflighted LOCKALL/map-refresh program
// before its RELEASEALL/END. The planner's completeNetwork obligation is an
// explicitly reviewed shared Sapphire installation; arbitrary flag844 edits are
// not authorization. All prerequisites below describe the proposed own state.
inline std::vector<uint8_t> nativeNetworkCompletionCommands(mCore* core, const Progress& own) {
    requirePokemon(campaignFieldQuiescent(core), "Network completion requires an idle native field");
    requirePokemon(own.flag(FlagPokemonObtained) && own.flag(FlagPokedexObtained) &&
                   own.flag(0x82C) && own.flag(0x840) && own.var(0x404E) == 0x6258 &&
                   own.var(0x4031) <= 2 && own.flag(0x846) && own.inventory.quantity(368) == 1 &&
                   own.var(0x4076) >= 5 && own.var(0x4076) < 6 && !own.flag(0x844),
                   "Network completion requires own Hall of Fame, National Dex and Rainbow Pass");
    const auto memory = readSaveMemory(core);
    requirePokemon(core->busRead8(core,memory.block2+0x1B) == 0xB9,
                   "Network completion requires the native National Dex magic");
    // Original DE Sapphire scene uses SPECIAL410 (SetPostgameFlags), then
    // SPECIAL297 (InitRoamer). Validate the original call site before changes.
    constexpr std::array<uint8_t,20> original{{0x25,0x9A,1,0x29,0x44,8,0x29,0x5C,0,
        0x29,0x8C,0,0x16,0x76,0x40,6,0,0x25,0x29,1}};
    for(unsigned i=0;i<original.size();++i)
        requirePokemon(core->busRead8(core,abiAddress(core, 0x08170F94)+i)==original[i],
                       "Native Sapphire completion preimage mismatch");
    // Flag844 is the final durable receipt, AFTER native own-starter roamer
    // creation and postgame flags. Sampling, saving and restart cannot replay
    // this initialization and erase a later caught/fled roamer.
    return {0x25,0x9A,1, 0x25,0x29,1, 0x45,0x76,1,1,0,
            0x29,0x5C,0, 0x29,0x8C,0, 0x29,0xDC,2,
            0x16,0x76,0x40,6,0, 0x29,0x44,8};
}

// The caller must fingerprint and bind an exact supported original AND authorize this
// operation from a committed, WON shared Champion battle. Persistent game-clear
// flags are deliberately insufficient authorization: the game's first own Hall
// of Fame entry must create its own record sectors, ribbons and debut time.
inline bool canStartNativeHallOfFameCompletion(mCore* core) {
    if (!campaignFieldQuiescent(core)) return false;
    const auto progress = readProgress(core);
    if (progress.mapGroup != 1 || progress.mapNum != 79) return false;
    for (unsigned badge = 0x820; badge <= 0x827; ++badge)
        if (!progress.flag(badge)) return false;
    // Preimages for the original native Champion->Hall-of-Fame warp and the
    // Hall-of-Fame entry scene. Check both before touching either script VM.
    constexpr std::array<uint8_t, 11> warp{{0x39,1,80,0xFF,5,0,12,0,0x27,0x6B,2}};
    std::array<uint8_t, 13> entry{{0x69,0xC7,0,0x4F,0xFF,0,0x33,0x2D,0x16,8,0x51,0,0}};
    monPut32(entry.data()+6,abiAddress(core,0x08162D33));
    for (unsigned i = 0; i < warp.size(); ++i)
        if (core->busRead8(core, abiAddress(core, 0x08162BC3) + i) != warp[i]) return false;
    for (unsigned i = 0; i < entry.size(); ++i)
        if (core->busRead8(core, abiAddress(core, 0x08162CD6) + i) != entry[i]) return false;
    const auto table = inventoryRead32(core, GlobalScriptContextAddress + 0x5C);
    const auto end = inventoryRead32(core, GlobalScriptContextAddress + 0x60);
    return table >= 0x08000000 && end > table && end <= 0x09000000 && end - table >= 0xA3 * 4;
}

inline bool startNativeHallOfFameCompletion(mCore* core) {
    if (!canStartNativeHallOfFameCompletion(core)) return false;
    // Native LOCKALL, then GOTO the original Champion warp command. Once the
    // jump has run, the original ROM script can use this context's return stack
    // normally. Native map entry owns positions, Oak, healing, ribbons, the Hall
    // of Fame save sectors, credits and the continue warp. No commercial script
    // or graphics are copied, and no game-clear flag/save bytes are fabricated.
    std::array<uint8_t, 6> program{{0x69,0x05,0xC3,0x2B,0x16,0x08}};
    monPut32(program.data()+2,abiAddress(core,0x08162BC3));
    constexpr auto context = GlobalScriptContextAddress;
    for (unsigned i = 0; i < 80; ++i)
        core->busWrite8(core, context + 12 + i, i < program.size() ? program[i] : 0);
    for (unsigned i = 0; i < 12; ++i) core->busWrite8(core, context + i, 0);
    for (unsigned i = 0; i < 4; ++i)
        core->busWrite8(core, context + 8 + i, uint8_t((context + 12) >> (i * 8)));
    core->busWrite8(core, context + 1, 1);
    core->busWrite8(core, FieldControlsLockAddress, 1);
    core->busWrite8(core, GlobalScriptStatusAddress, 0);
    core->setKeys(core, 0);
    return true;
}
} // namespace coop::firered
