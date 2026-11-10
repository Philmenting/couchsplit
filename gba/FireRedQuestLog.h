// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedProfile.h"
#include <cstdint>

namespace coop::firered {
// Exact profiled FireRed originals only, after the caller verifies the hash.
// The native SPECIAL391 GetQuestLogState reads this byte. pret/pokefirered
// include/constants/quest_log.h defines 0 = idle, 1 = recording, 2 = replay,
// 3 = final replay transition. Playback temporarily loads older save data;
// its field must not be mistaken for the player's actual current world.
inline constexpr uint32_t QuestLogStateAddress = 0x0203ADFA;
inline bool questLogWorldAvailable(mCore* core) {
    return core && core->busRead8 && core->busRead8(core, QuestLogStateAddress) <= 1;
}
} // namespace coop::firered
