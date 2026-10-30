// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <initializer_list>
#include <array>

namespace coop::firered {
// Reviewed native flags in pret/pokefirered battle_setup.c (ce6406d8).
// Keep Safari, link, tutorial and facility controllers out of human doubles.
inline bool sharedEncounterFlags(uint32_t flags) {
    if (!(flags & ~0xDu)) return true;
    const auto wild = flags & ~5u; // Native master/double bits do not change kind.
    return wild == 0xA000u || wild == 0x20000u || wild == 0x42000u;
}
inline bool sameStoryTrainer(unsigned a, unsigned b) {
    if (a == b) return true;
    // Each group is one story fight with a team selected by the OWN starter.
    // A different chapter or rematch must never consume the other's encounter.
    for (const unsigned first : {326u,329u,332u,426u,429u,432u,435u,438u,739u})
        if (a >= first && a < first + 3 && b >= first && b < first + 3) return true;
    return false;
}
struct WaitingEncounter {
    uint32_t flags = 0, callback = 0;
    uint16_t trainer = 0, mode = 0;
    uint8_t mapGroup = 0, mapNum = 0;
    uint16_t species = 0;
    uint32_t scriptContinuation = 0;
    uint8_t scriptDepth = 0;
    std::array<uint32_t,20> scriptReturns{};
};
inline bool matchingWaitingEncounters(const WaitingEncounter& a, const WaitingEncounter& b) {
    if (a.mapGroup != b.mapGroup || a.mapNum != b.mapNum || a.callback != b.callback ||
        (a.flags & ~5u) != (b.flags & ~5u)) return false;
    if (a.flags & 8u) return a.mode == b.mode && sameStoryTrainer(a.trainer, b.trainer);
    // Concurrent random grass encounters become ONE shared encounter, chosen
    // by the coordinator's deterministic leader order. Both own ordinary wild
    // return callbacks are safe; no scripted object/quest result is consumed.
    if (!(a.flags & ~5u)) return true;
    // Identical species/map are insufficient: two Electrode objects can call
    // one common battle subroutine. Their original return stacks identify the
    // different object receipts; defer the second encounter instead of merging.
    if (!a.scriptContinuation || a.scriptContinuation != b.scriptContinuation ||
        a.scriptDepth != b.scriptDepth || a.scriptDepth > a.scriptReturns.size()) return false;
    for(unsigned i=0;i<a.scriptDepth;++i) if(a.scriptReturns[i]!=b.scriptReturns[i]) return false;
    return a.species && a.species == b.species;
}
} // namespace coop::firered
