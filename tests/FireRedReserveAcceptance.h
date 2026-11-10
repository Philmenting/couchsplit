// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Private-ROM acceptance instrumentation only. Never compiled into a worker.
#include "../gba/FireRedBattle.h"

namespace coop::firered::acceptance {
class ReserveProbe {
    using Callback=void(*)(ARMCore*,int);
    coop::firered::TeamBattle& pair;
    mCore* core;
    ARMCore* cpu;
    Callback delegate,original;
    bool pinned;
    bool attached=true;
    std::exception_ptr error;
    coop::firered::Party beforeAward;
    std::array<unsigned,2> queue{},wish{};
    inline static ReserveProbe* current=nullptr;
    static void breakpoint(ARMCore* cpu,int immediate) noexcept {
        auto& p=*current;
        const auto address=coop::firered::canonicalCodeAddress(p.core,uint32_t(cpu->gprs[ARM_PC])-4);
        try {p.before(address);}catch(...){p.error=std::current_exception();}
        p.delegate(cpu,immediate);
        try {p.after(address);}catch(...){p.error=std::current_exception();}
    }
    void before(uint32_t address) {
        using namespace coop::firered;
        if(pinned && !saturated && address==BattleHooks::ExperienceCommandAddress && core->busRead8(core,0x02023FE0)==1) {
            const auto& tx=pair.partyTransaction();beforeAward=readParty(core);
            const auto battle=core->busRead32(core,0x02023FE8);
            for(unsigned owner=0;owner<2;++owner) {
                std::vector<unsigned> bench;
                const auto active=core->busRead16(core,0x02023BCE + owner*4);
                for(unsigned slot=0;slot<6;++slot)if(tx.owners[slot]==int(owner)&&slot!=active)bench.push_back(slot);
                requirePokemon(bench.size()==2,"Saturation fixture lacks two own bench slots");
                queue[owner]=bench[0];wish[owner]=bench[1];
                core->busWrite8(core,battle+0x5C+owner*2,uint8_t(queue[owner]));
                core->busWrite8(core,0x02023F40+owner*2,1);
                core->busWrite8(core,0x02023F44+owner*2,uint8_t(wish[owner]));
            }
            saturated=true;
        }
    }
    void after(uint32_t address) {
        using namespace coop::firered;
        if(address==BattleHooks::ForcedChoiceAddress && cpu->prefetch[0]==0xE04F) {
            const unsigned owner=core->busRead8(core,0x02023D6C)/2;
            const unsigned slot=unsigned(cpu->gprs[5]);
            const auto& tx=pair.partyTransaction();
            requirePokemon(owner<2 && slot<6 && tx.owners[slot]==int(owner),"Forced switch selected partner Pokemon");
            ++forced[owner];unloadedForced|=tx.originalSlots[slot]>=3;
        }
        if(address==BattleHooks::WishCastAddress) {
            const unsigned battler=core->busRead8(core,0x02023D6B);
            if(battler==0 || battler==2) {
                const auto slot=core->busRead8(core,0x02023F44+battler);
                sources[battler/2]=readParty(core).mons[slot];sourceSlots[battler/2]=slot;++casts[battler/2];
            }
        }
        if(address==BattleHooks::WishTextAddress && cpu->prefetch[0]==0xE001) {
            const unsigned owner=core->busRead8(core,0x02022ABA)/2;
            requirePokemon(owner<2 && sources[owner].has_value(),"Wish text lacks original native cast");
            const auto& mon=*sources[owner];
            for(unsigned i=0;i<10 && mon.bytes[8+i]!=0xFF;++i)
                requirePokemon(core->busRead8(core,0x02021CD0+i)==mon.bytes[8+i],"Wish announced a replacement's nickname");
            sourceEvicted[owner]|=!sameOwnedMon(readParty(core).mons[sourceSlots[owner]],mon);
            ++announcements[owner];
        }
        if(address==BattleHooks::ReshowLoadSpriteAddress && saturated && !restored)++awardReshows;
        if(address==BattleHooks::ExperienceCommandAddress && saturated && !restored && core->busRead8(core,0x02023FE0)==6) {
            const auto actual=readParty(core);const auto battle=core->busRead32(core,0x02023FE8);
            for(unsigned slot=0;slot<6;++slot)requirePokemon(sameOwnedMon(actual.mons[slot],beforeAward.mons[slot]),"EXP did not restore original working records");
            for(unsigned owner=0;owner<2;++owner) {
                requirePokemon(core->busRead8(core,battle+0x5C+owner*2)==queue[owner] &&
                    core->busRead8(core,0x02023F44+owner*2)==wish[owner] && core->busRead8(core,0x02023F40+owner*2)==1,
                    "EXP altered dormant native references");
            }
            restored=true;
        }
    }
public:
    bool saturated=false,restored=false,unloadedForced=false;
    unsigned awardReshows=0;
    std::array<unsigned,2> forced{},casts{},announcements{},sourceSlots{};
    std::array<bool,2> sourceEvicted{};
    std::array<std::optional<coop::firered::PartyMon>,2> sources;
    ReserveProbe(coop::firered::TeamBattle& battle,mCore* target,Callback native,bool injectPins,bool enabled)
        :pair(battle),core(target),cpu(static_cast<ARMCore*>(target->cpu)),delegate(cpu->irqh.bkpt16),original(native),pinned(injectPins) {
        if(!enabled){attached=false;return;}
        coop::firered::requirePokemon(!current && delegate,"Acceptance probe is already attached");current=this;cpu->irqh.bkpt16=breakpoint;
    }
    ~ReserveProbe(){detach();}
    void check() const {if(error)std::rethrow_exception(error);}
    void detach(){if(attached){cpu->irqh.bkpt16=pair.active()?delegate:original;current=nullptr;attached=false;}}
};
}
