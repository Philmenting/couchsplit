// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedPartyMenu.h"

namespace coop::firered {

// Wish heals the current battler using its current maximum HP. Its saved party
// ID is used only for the original caster's nickname. Capture that name on
// native success, and retain it past counter==0 until message expansion.
class WishJournal {
public:
    using Name = std::array<uint8_t,11>;
private:
    struct Source {unsigned slot; Name name;};
    std::array<std::optional<Source>,2> sources;
public:
    void capture(unsigned battler,unsigned slot,const BattleParty& transaction,const Party& current) {
        requirePokemon((battler==0 || battler==2) && slot<current.count,"Invalid FireRed Wish caster");
        const auto own=restoreParties(transaction,current);
        const auto& mon=current.mons[slot];
        bool found=false;
        for(unsigned i=0;i<own[battler/2].count;++i)found|=sameOwnedMon(mon,own[battler/2].mons[i]);
        requirePokemon(found && !(mon.bytes[19]&5u),"Foreign or invalid FireRed Wish caster");
        // GetMonData(NICKNAME) followed by StringGet_Nickname: at most ten
        // encoded bytes, including the native Japanese language controls.
        Name name;name.fill(0xFF);
        if(mon.bytes[18]==1) {
            name[0]=0xFC;name[1]=0x15;unsigned end=2;
            for(unsigned i=0;i<6 && mon.bytes[8+i]!=0xFF;++i)name[end++]=mon.bytes[8+i];
            name[end++]=0xFC;name[end]=0x16;
        } else std::copy_n(mon.bytes.begin()+8,10,name.begin());
        sources[battler/2]=Source{slot,name};
    }
    BenchReferences protect(BenchReferences references) const {
        references.exclusions();
        references.journaledWish=0;
        for(unsigned owner=0;owner<2;++owner)
            if(sources[owner] && references.wishCounters[owner*2] &&
               references.wishSlots[owner*2]==sources[owner]->slot)
                references.journaledWish|=uint8_t(1u<<(owner*2));
        return references;
    }
    std::optional<Name> name(unsigned battler,unsigned slot) const {
        requirePokemon(battler==0 || battler==2,"Invalid FireRed Wish message owner");
        if(!sources[battler/2])return std::nullopt;
        requirePokemon(sources[battler/2]->slot==slot,"FireRed Wish message source changed");
        return sources[battler/2]->name;
    }
};

// Read-only preflight follows native ability/accuracy gates. A selected plan
// is applied only after native level success. The 0..5 RNG draw is an OWN slot.
// Invalid draws retry the native RNG. A valid plan is committed immediately
// before native monToSwitchIntoId[target] is replaced, so only that old queue
// may be omitted from the planner. Both active records and the partner's queue
// stay resident, including the outgoing record used by Natural Cure.
inline PartyMenuSelection planForcedSwitch(const BattleParty& transaction,const Party& current,
                                           unsigned owner,BenchReferences references,unsigned original) {
    requirePokemon(owner<2 && original<6,"Invalid FireRed forced switch selection");
    references.exclusions();
    references.queued[owner]=6;
    const auto session=planPartyMenu(transaction,current,owner,references);
    if(original>=session.menu.count)return {};
    return planPartyMenuSelection(session,session.menu,original,true);
}
} // namespace coop::firered
