// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedErrors.h"
#include <cstdint>
#include <stdexcept>

namespace coop::firered {
// Paired-GBA extension of ABI 2. DS and independent GBA keep their protocol.
// bridgeActive: low byte = state, high bytes = unique pause generation.
// touch: launcher echoes the exact pause value to explicitly request native
// individual battles. GBA has no touchscreen. A worker drains this command
// every frame; a stale/held acknowledgement cannot accept a later pause.
enum class PairState : uint32_t { Field=0, Battle=1, Waiting=2, ExpShare=3, Enigma=4, Individual=5 };
inline PairState pairState(uint32_t value) { return PairState(value & 0xFFu); }
inline bool battlePaused(uint32_t value) {
    return (value >> 8) && (pairState(value)==PairState::ExpShare || pairState(value)==PairState::Enigma);
}
class BattleRecovery {
    uint32_t current=0, generation=0;
    unsigned initiator=0;
    void pause(PairState reason,unsigned owner) {
        // Never reuse an acknowledgement during one worker lifetime.
        if(generation==0xFFFFFFu) throw std::runtime_error("FireRed recovery generation exhausted");
        current=(++generation << 8) | uint32_t(reason); initiator=owner;
    }
public:
    uint32_t status() const { return current; }
    unsigned owner() const { return initiator; }
    bool paused() const { return battlePaused(current); }
    bool individual() const { return pairState(current)==PairState::Individual; }
    template<class Battle> bool tryBegin(Battle& battle,unsigned owner) {
        if(current) return false;
        try { return battle.tryBegin(owner); }
        catch(const ExpShareCapacityError&) {
            if(battle.active()) throw; // Only a read-only preflight may recover.
            pause(PairState::ExpShare,owner);
        } catch(const EnigmaBerryError&) {
            if(battle.active()) throw;
            pause(PairState::Enigma,owner);
        }
        return false;
    }
    bool accept(uint32_t command) {
        if(!paused() || command!=current) return false;
        current=uint32_t(PairState::Individual);return true;
    }
    bool returnToCoop(bool bothFieldsReady) {
        if(!individual() || !bothFieldsReady) return false;
        current=0;return true;
    }
};
}
