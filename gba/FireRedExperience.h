// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedBench.h"
#include "FireRedInventory.h"
#include <functional>

namespace coop::firered {

// The native six records are a working set. Award/participation/evolution
// ownership belongs to the twelve original identities, never to a reused slot.
// This coordinator runs ONLY at verified native command boundaries. The game
// still performs every award, EV gain, level-up, move choice and evolution.
class BattleExperience {
    mCore* core;
    BattleParty& transaction;
    std::array<uint16_t,2> participated{};
    uint16_t leveled=0, knockedOff=0;
    struct Award {
        BattleParty working;
        BenchReferences references;
        uint32_t battle=0;
        std::vector<unsigned> recipients;
        unsigned next=0, slot=6, recipient=12;
        unsigned flank=0;
    };
    std::optional<Award> award;
    std::optional<unsigned> evolving;
    std::optional<BattleParty> evolutionWorking;
    std::array<Party,2> evolutionParties{};
    static constexpr uint32_t State=0x02023FE0, Levels=0x03004F24;
    unsigned virtualSlot(unsigned slot) const {
        requirePokemon(slot<transaction.combined.count && transaction.owners[slot]>=0 &&
                       transaction.owners[slot]<2 && transaction.originalSlots[slot]>=0 &&
                       transaction.originalSlots[slot]<6,"Invalid FireRed experience ownership");
        return unsigned(transaction.owners[slot])*6+unsigned(transaction.originalSlots[slot]);
    }
    void rebase() {
        const auto current=readParty(core);
        const auto journal=restoreParties(transaction,current);
        for(unsigned slot=0;slot<current.count;++slot)
            requirePokemon(sameOwnedMon(current.mons[slot],transaction.combined.mons[slot]),
                           "FireRed experience working order changed unexpectedly");
        transaction.originals=journal;transaction.combined=current;
    }
    void absorbLevels() {
        const auto mask=core->busRead8(core,Levels);
        requirePokemon(!(mask&~0x3Fu),"Invalid FireRed pending evolution mask");
        for(unsigned slot=0;slot<transaction.combined.count;++slot)
            if(mask&(1u<<slot))leveled|=uint16_t(1u<<virtualSlot(slot));
        core->busWrite8(core,Levels,0);
    }
    void restoreWorking(const BattleParty& working) {
        transaction.owners=working.owners;transaction.originalSlots=working.originalSlots;
        transaction.combined=working.combined;
        for(unsigned slot=0;slot<transaction.combined.count;++slot)
            transaction.combined.mons[slot]=transaction.originals[unsigned(transaction.owners[slot])].mons[unsigned(transaction.originalSlots[slot])];
        restoreParties(transaction,transaction.combined);
        writeParty(core,transaction.combined);
    }
    void checkAwardReferences(uint32_t battle) const {
        requirePokemon(award && battle==award->battle,"FireRed experience structure changed during an award");
        const auto current=readBenchReferences(core,6,6);
        const auto& saved=award->references;
        // Among these references only the level mask may change while getexp,
        // its controller tasks and nested move-learning script own the award.
        requirePokemon(current.active==saved.active && current.queued==saved.queued &&
                       current.wishCounters==saved.wishCounters && current.wishSlots==saved.wishSlots &&
                       current.participation==saved.participation && current.knockedOff==saved.knockedOff,
                       "FireRed native battle references changed during an experience award");
        requirePokemon(core->busRead8(core,battle+0x10)==award->slot,
                       "FireRed experience recipient changed during an award");
    }
    void nextRecipient(uint32_t battle) {
        requirePokemon(award && award->next<award->recipients.size(),"Missing FireRed experience recipient");
        const auto recipient=award->recipients[award->next++];
        const unsigned owner=recipient/6, original=recipient%6;
        unsigned slot=6;
        for(unsigned s=0;s<transaction.combined.count;++s)
            if(virtualSlot(s)==recipient)slot=s;
        if(slot==6) {
            // getexp owns the native script until all EXP, level and move
            // dialogs have returned. Queued switches and Wish cannot consume
            // their party IDs until restoreWorking has restored this layout.
            // BOTH active records must remain resident: the EXP controller,
            // move-learning/Choice Band helpers and summary reshow read them.
            const auto& active=award->references.active;
            for(unsigned s=0;s<transaction.combined.count;++s)
                if(transaction.owners[s]==int(owner) &&
                   s!=active[0] && s!=active[1]) {slot=s;break;}
            requirePokemon(slot<6,"FireRed experience recipient has no inactive owned working slot");
            transaction.originalSlots[slot]=int(original);
            transaction.combined.mons[slot]=transaction.originals[owner].mons[original];
            restoreParties(transaction,transaction.combined);
            writeParty(core,transaction.combined);
        }
        award->slot=slot;award->recipient=recipient;
        core->busWrite8(core,battle+0x10,uint8_t(slot));
        core->busWrite8(core,battle+0x53,(participated[award->flank]&(1u<<recipient))?1:0);
        core->busWrite8(core,State,2);
        if(ownerChanged)ownerChanged(owner);
    }
    bool loadEvolutionOwner() {
        for(unsigned owner=0;owner<2;++owner) {
            const unsigned mask=(leveled>>(owner*6))&63u;
            if(!mask)continue;
            evolving=owner;
            writeParty(core,evolutionParties[owner]);
            core->busWrite8(core,Levels,uint8_t(mask));
            if(ownerChanged)ownerChanged(owner);
            if(evolutionBag) evolutionBag(owner);
            return true;
        }
        return false;
    }
public:
    std::function<void(unsigned)> ownerChanged, evolutionBag;
    BattleExperience(mCore* target,BattleParty& party):core(target),transaction(party) {}
    std::optional<unsigned> inputOwner() const {
        if(evolving)return evolving;
        if(award && award->recipient<12)return award->recipient/6;
        return std::nullopt;
    }
    std::optional<unsigned> evolutionOwner() const {return evolving;}
    bool temporaryParty() const {return bool(award)||bool(evolving)||bool(evolutionWorking);}
    void observeBattle() {
        requirePokemon(!temporaryParty(),"FireRed working set changed during a native award");
        const auto refs=readBenchReferences(core,6,6);
        for(unsigned slot=0;slot<transaction.combined.count;++slot) {
            const auto bit=uint16_t(1u<<virtualSlot(slot));
            for(unsigned flank=0;flank<2;++flank)if(refs.participation[flank]&(1u<<slot))participated[flank]|=bit;
            if(refs.knockedOff&(1u<<slot))knockedOff|=bit;
        }
        absorbLevels();
    }
    void projectReferences() {
        requirePokemon(!temporaryParty(),"Cannot project FireRed references during a native award");
        std::array<uint8_t,2> masks{};uint8_t knocked=0;
        for(unsigned slot=0;slot<transaction.combined.count;++slot) {
            const auto bit=uint16_t(1u<<virtualSlot(slot));
            for(unsigned flank=0;flank<2;++flank)if(participated[flank]&bit)masks[flank]|=uint8_t(1u<<slot);
            if(knockedOff&bit)knocked|=uint8_t(1u<<slot);
        }
        for(unsigned flank=0;flank<2;++flank)core->busWrite8(core,0x02023F4E + flank,masks[flank]);
        core->busWrite8(core,0x02023F49,knocked);core->busWrite8(core,Levels,0);
    }
    void opponentReset(unsigned battler) {
        requirePokemon(battler<4 && !temporaryParty(),"Unexpected FireRed opponent reset during experience");
        if(battler&1u)participated[(battler&2u)/2]=0;
    }
    // true skips only the native six-record count/iteration. All per-Pokemon
    // arithmetic, effects and dialogs continue through native states 2..5.
    bool experienceCommand() {
        const auto state=core->busRead8(core,State);
        const auto battle=inventoryRead32(core,0x02023FE8);
        requirePokemon(validSaveBlock(battle,0xE0),"Invalid FireRed experience structure");
        if(award) {
            requirePokemon(state>=2 && state<=5,"Unexpected FireRed native experience state");
            checkAwardReferences(battle);
        }
        if(state==1) {
            requirePokemon(!award && !evolving && !inventoryRead32(core,0x02023BC8),"Overlapping FireRed experience awards");
            observeBattle();rebase();
            const unsigned fainted=core->busRead8(core,0x02023D6D);
            requirePokemon(fainted==1 || fainted==3,"FireRed experience has no defeated opponent");
            Award next;next.working=transaction;next.flank=(fainted&2u)/2;
            next.battle=battle;next.references=readBenchReferences(core,6,6);
            for(unsigned owner=0;owner<2;++owner)
                requirePokemon(next.references.active[owner]<transaction.combined.count &&
                               transaction.owners[next.references.active[owner]]==int(owner),
                               "Invalid FireRed active party ownership during experience");
            unsigned participants=0,holders=0;
            for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<transaction.originals[owner].count;++slot) {
                const auto& mon=transaction.originals[owner].mons[slot];const auto id=owner*6+slot;
                if(!mon.viable())continue;
                const bool sent=(participated[next.flank]&(1u<<id))!=0,share=mon.heldItem()==ExpShareItem;
                participants+=sent;holders+=share;
                if((sent||share) && mon.level()<100)next.recipients.push_back(id);
            }
            const auto enemy=0x02023BE4+fainted*88;
            const unsigned species=inventoryRead16(core,enemy),level=core->busRead8(core,enemy+0x2A);
            requirePokemon(species>0 && species<=411 && level>0 && level<=100,"Invalid FireRed experience opponent");
            const auto base=core->busRead8(core,0x082546A8+species*28+9)*level/7;
            const unsigned shared=holders?base/2:base;
            inventoryWrite16(core,battle+0x50,uint16_t(std::max(1u,participants?shared/participants:0u)));
            inventoryWrite16(core,0x02023F52,uint16_t(holders?std::max(1u,base/2/holders):0u));
            if(next.recipients.empty())core->busWrite8(core,State,6);
            else {award=std::move(next);nextRecipient(battle);}
            return true;
        }
        if(state==5 && award && inventoryRead32(core,0x02023D50)==0) {
            requirePokemon(!inventoryRead32(core,0x02023BC8),"FireRed recipient still owns a native controller");
            absorbLevels();rebase();restoreWorking(award->working);
            if(award->next<award->recipients.size())nextRecipient(battle);
            else {award.reset();projectReferences();core->busWrite8(core,State,6);}
            return true;
        }
        return false;
    }
    void evolutionGate() {
        requirePokemon(!award && !evolving,"FireRed battle ended during a native experience award");
        // Capture can have appended a new party record. Its separate native
        // ownership transaction is finalized by TeamBattle::finish. As in the
        // original game, only a win runs the post-battle evolution queue.
        if(core->busRead8(core,0x02023E8A)!=1) {core->busWrite8(core,Levels,0);return;}
        // FreeResetData has reached its fade-complete branch. The structure is
        // still live here, but is freed before TryEvolvePokemon is called.
        observeBattle();rebase();
        core->busWrite8(core,Levels,leveled?1:0);
    }
    // Called at TryEvolvePokemon entry and its final (zero-mask) branch.
    // Own complete parties give native summaries/evolution logic correct slots.
    // true at the final branch repeats its loop with the next owner's mask.
    bool evolutionBoundary(bool ending) {
        if(!ending) {
            if(!evolutionWorking) {
                requirePokemon(leveled && !award,"Missing FireRed virtual evolution queue");
                evolutionWorking=transaction;evolutionParties=transaction.originals;
                requirePokemon(loadEvolutionOwner(),"Cannot load FireRed evolution owner");
            }
            return false;
        }
        requirePokemon(evolutionWorking && evolving && !core->busRead8(core,Levels),"Invalid FireRed evolution completion");
        const auto owner=*evolving;const auto result=readParty(core);
        const auto& before=evolutionParties[owner];
        requirePokemon(result.count>=before.count,"FireRed evolution removed an owned Pokemon");
        for(unsigned slot=0;slot<before.count;++slot) {
            const auto& a=before.mons[slot];const auto& b=result.mons[slot];
            requirePokemon(sameOwnedMon(a,b) && b.experience()>=a.experience() && b.level()>=a.level(),
                           "FireRed evolution changed owned identity or progress");
            const auto x=a.secure(),y=b.secure();
            requirePokemon(std::equal(x.begin()+36,x.begin()+44,y.begin()+36),"FireRed evolution changed origin or IV identity");
            for(unsigned stat=24;stat<30;++stat)requirePokemon(y[stat]>=x[stat],"FireRed evolution decreased EVs");
        }
        // Only the native Nincada -> Ninjask scene can append a party member.
        // Each completed scene copies its evolved record to one free own slot;
        // FireRed does not consume a Poke Ball for this (unlike later games).
        unsigned additions=0;
        for(unsigned source=0;source<before.count;++source) {
            const auto& parent=result.mons[source];
            if(before.mons[source].species()!=301 || parent.species()!=302)continue;
            if(before.count+additions==PartyCapacity)continue;
            requirePokemon(before.count+additions<result.count,"Missing native Shedinja addition");
            const auto& shell=result.mons[before.count+additions++];
            const auto x=parent.secure(),y=shell.secure();
            requirePokemon(nativeShedinjaSiblings(parent,shell) &&
                std::equal(x.begin()+4,x.begin()+44,y.begin()+4) &&
                std::equal(parent.bytes.begin()+18,parent.bytes.begin()+27,shell.bytes.begin()+18) &&
                shell.heldItem()==0 && shell.status()==0 && shell.hp()==1 && shell.maxHp()==1 &&
                shell.level()==parent.level() && shell.bytes[27]==0 && shell.bytes[85]==255 &&
                monU16(shell.bytes.data()+30)==0 && monU32(y.data()+44)==(monU32(x.data()+44)&0x80000000u),
                "Invalid native Shedinja addition or owner");
        }
        requirePokemon(result.count==before.count+additions,"Unexpected FireRed evolution party addition");
        evolutionParties[owner]=result;leveled&=uint16_t(~(63u<<(owner*6)));evolving.reset();
        if(loadEvolutionOwner())return true;
        transaction.originals=evolutionParties;restoreWorking(*evolutionWorking);evolutionWorking.reset();
        return false;
    }
};

inline BattleParty combinePagedParties(const Party& first,const Party& second) {
    std::array<std::vector<unsigned>,2> selected;
    const std::array<const Party*,2> parties{{&first,&second}};
    for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<parties[owner]->count && selected[owner].size()<3;++slot)
        if(parties[owner]->mons[slot].viable())selected[owner].push_back(slot);
    return combineParties(first,second,selected);
}
}
