// SPDX-License-Identifier: GPL-3.0-or-later
#include "../gba/FireRedRecovery.h"
#include <cassert>
#include <iostream>
using namespace coop::firered;
struct Preflight {
    unsigned kind=0,calls=0;bool converted=false;
    bool active() const {return converted;}
    bool tryBegin(unsigned) {
        ++calls;
        if(kind==1)throw ExpShareCapacityError();
        if(kind==2)throw EnigmaBerryError();
        if(kind==3)throw std::runtime_error("unexpected damaged party");
        converted=true;return true;
    }
};
int main() {
    for(unsigned reason:{1u,2u})for(unsigned owner:{0u,1u}) {
        BattleRecovery recovery;Preflight battle;battle.kind=reason;
        assert(!recovery.accept(0) && !recovery.accept(259));
        assert(!recovery.tryBegin(battle,owner) && recovery.paused());
        const auto first=recovery.status();
        assert(recovery.owner()==owner && pairState(first)==(reason==1?PairState::ExpShare:PairState::Enigma));
        for(int frame=0;frame<120;++frame)assert(!recovery.tryBegin(battle,1-owner));
        assert(battle.calls==1 && !battle.converted); // No retry/advance during pause.
        assert(!recovery.accept(first+256) && !recovery.returnToCoop(true));
        assert(recovery.accept(first) && recovery.individual());
        assert(!recovery.accept(first) && !recovery.returnToCoop(false));
        assert(!recovery.tryBegin(battle,owner) && battle.calls==1);
        assert(recovery.returnToCoop(true));
        assert(!recovery.tryBegin(battle,owner) && recovery.status()!=first);
        assert(!recovery.accept(first)); // A stale request cannot release a new encounter.
        assert(recovery.accept(recovery.status()) && recovery.returnToCoop(true));
        battle.kind=0;assert(recovery.tryBegin(battle,owner) && battle.converted);
    }
    for(unsigned kind:{1u,2u,3u}) {
        BattleRecovery recovery;Preflight battle;battle.kind=kind;battle.converted=kind!=3;
        bool threw=false;try{recovery.tryBegin(battle,0);}catch(const std::runtime_error&){threw=true;}
        assert(threw && recovery.status()==0); // Never recover unknown errors or mutated transactions.
    }
    assert(!battlePaused(3) && !battlePaused(4) && !battlePaused(261));
    std::cout<<"PASS: expected preflight pauses, explicit single-use approval, stale command rejection, both-field rendezvous and fatal-error separation\n";
}
