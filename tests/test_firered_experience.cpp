// SPDX-License-Identifier: GPL-3.0-or-later
#include "../gba/FireRedExperience.h"
#include "../gba/FireRedPartyMenu.h"
#include <cassert>
#include <map>
#include <set>
using namespace coop::firered;
namespace {
Party roster(unsigned owner) {
    Party p;p.count=6;
    for(unsigned slot=0;slot<6;++slot) {
        auto& m=p.mons[slot];monPut32(m.bytes.data(),24*(owner*6+slot+1));monPut32(m.bytes.data()+4,owner+1);
        m.bytes[8]=0xBB;m.bytes[9]=0xFF;m.bytes[18]=5;m.bytes[19]=2;m.bytes[20]=0xBB;m.bytes[21]=0xFF;m.bytes[84]=10;
        monPut16(m.bytes.data()+86,30);monPut16(m.bytes.data()+88,30);
        for(unsigned i=90;i<100;i+=2)monPut16(m.bytes.data()+i,20);
        PartyMon::Secure s{};monPut16(s.data(),1);monPut16(s.data()+2,ExpShareItem);monPut32(s.data()+4,1000);s[9]=70;
        monPut16(s.data()+12,33);s[20]=35;monPut32(s.data()+40,0x3FFFFFFF);m.setSecure(s);
    }
    p.validate();return p;
}
struct Bus {
    mCore c{};std::map<uint32_t,uint8_t> memory;
    std::vector<Party> partyWrites;
    static Bus& bus(mCore*c){return *static_cast<Bus*>(c->board);}
    Bus() {
        c.board=this;
        c.busRead8=[](mCore*c,uint32_t a)->uint32_t{return bus(c).memory[a];};
        c.busRead16=[](mCore*c,uint32_t a)->uint32_t{return bus(c).memory[a]|(unsigned(bus(c).memory[a+1])<<8);};
        c.busRead32=[](mCore*c,uint32_t a)->uint32_t{uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(bus(c).memory[a+i])<<(8*i);return v;};
        c.busWrite8=[](mCore*c,uint32_t a,uint8_t v){
            auto& b=bus(c);b.memory[a]=v;
            // writeParty publishes the count after its complete records. This
            // exposes each restored layout even when the next award immediately
            // borrows the same slot again before the coordinator returns.
            if(a==PlayerPartyCountAddress)b.partyWrites.push_back(readParty(c));
        };
        put(0x02023FE8,0x02038000);put(0x02023BCE,0,2);put(0x02023BD2,1,2);
        memory[0x0203805C]=6;memory[0x0203805E]=6;
        memory[0x02023F4E]=3;memory[0x02023F4F]=3;
        memory[0x02023D6D]=1;put(0x02023BE4+88,19,2);memory[0x02023BE4+88+0x2A]=10;
        memory[0x02023E8A]=1;
        memory[0x082546A8+19*28+9]=57;
    }
    void put(uint32_t a,uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)memory[a+i]=uint8_t(v>>(i*8));}
};
template<class F> void rejects(F f){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}assert(failed);}
void allRecipientsAndEvolutions(bool pinned=false,unsigned active0=0,unsigned active1=1) {
    Bus b;auto t=combinePagedParties(roster(0),roster(1));const auto original=t;
    b.put(0x02023BCE,active0,2);b.put(0x02023BD2,active1,2);
    b.memory[0x02023F4E]=b.memory[0x02023F4F]=uint8_t((1u<<active0)|(1u<<active1));
    if(pinned)for(unsigned owner=0;owner<2;++owner) {
        bool queued=false;
        for(unsigned slot=0;slot<6;++slot) {
            if(t.owners[slot]!=int(owner) || slot==(owner?active1:active0))continue;
            if(!queued) {b.memory[0x0203805C+owner*2]=uint8_t(slot);queued=true;}
            else {b.memory[0x02023F40+owner*2]=2;b.memory[0x02023F44+owner*2]=uint8_t(slot);}
        }
    }
    const auto references=readBenchReferences(&b.c,6,6);
    if(pinned)assert(references.exclusions().size()==6);
    auto expected=original.originals;
    auto expectedWorking=[&] {
        auto party=original.combined;
        for(unsigned slot=0;slot<party.count;++slot)
            party.mons[slot]=expected[unsigned(original.owners[slot])].mons[unsigned(original.originalSlots[slot])];
        return party;
    };
    auto unchangedReferences=[&] {
        const auto current=readBenchReferences(&b.c,6,6);
        assert(current.active==references.active && current.queued==references.queued &&
               current.wishCounters==references.wishCounters && current.wishSlots==references.wishSlots &&
               current.participation==references.participation && current.knockedOff==references.knockedOff);
    };
    writeParty(&b.c,t.combined);BattleExperience xp(&b.c,t);std::vector<unsigned> owners;
    xp.ownerChanged=[&](unsigned owner){owners.push_back(owner);};
    b.memory[0x02023FE0]=1;assert(xp.experienceCommand());
    assert(inventoryRead16(&b.c,0x02038050)==20 && inventoryRead16(&b.c,0x02023F52)==3);
    std::set<unsigned> visited;
    for(unsigned n=0;n<12;++n) {
        unchangedReferences();
        const unsigned slot=b.memory[0x02038010];auto current=readParty(&b.c);
        const unsigned id=current.mons[slot].identity().personality/24-1;
        assert(visited.insert(id).second && xp.inputOwner()==id/6);
        // Native summary reshow and move-learning helpers must always see
        // the actual two active Pokemon, including after either has switched.
        for(unsigned active:{active0,active1})assert(current.mons[active]==expectedWorking().mons[active]);
        // Model a completed native award, then verify the coordinator's merge.
        auto secure=current.mons[slot].secure();monPut32(secure.data()+4,1001);current.mons[slot].setSecure(secure);
        expected[id/6].mons[id%6]=current.mons[slot];
        writeParty(&b.c,current);b.memory[0x03004F24]=uint8_t(1u<<slot);b.memory[0x02023FE0]=5;
        const auto writesBefore=b.partyWrites.size();
        assert(xp.experienceCommand());
        assert(b.partyWrites.size()>writesBefore && b.partyWrites[writesBefore]==expectedWorking());
        assert(t.originals==expected);
        unchangedReferences();
        if(n<11)assert(b.memory[0x02023FE0]==2);
    }
    assert(visited.size()==12 && !xp.temporaryParty() && b.memory[0x02023FE0]==6);
    assert(t.owners==original.owners && t.originalSlots==original.originalSlots);
    const auto result=restoreParties(t,readParty(&b.c));
    for(const auto& p:result)for(const auto&m:p.mons)assert(m.experience()==1001);
    xp.evolutionGate();assert(b.memory[0x03004F24]);xp.evolutionBoundary(false);
    for(unsigned owner=0;owner<2;++owner) {
        assert(xp.evolutionOwner()==owner && readParty(&b.c)==result[owner] && b.memory[0x03004F24]==63);
        auto current=readParty(&b.c);auto s=current.mons[5].secure();monPut16(s.data(),2);current.mons[5].setSecure(s);writeParty(&b.c,current);
        b.memory[0x03004F24]=0;assert(xp.evolutionBoundary(true)==(owner==0));
    }
    assert(!xp.temporaryParty());
    const auto after=restoreParties(t,readParty(&b.c));assert(after[0].mons[5].species()==2 && after[1].mons[5].species()==2);
}
void invalidAwardReferences() {
    // Missing, sentinel, out-of-range or foreign allied active indexes cannot
    // be hidden by the temporary EXP working-set exception.
    for(const auto active:std::array<std::array<unsigned,2>,5>{{{{6,1}},{{255,1}},{{7,1}},{{1,0}},{{0,0}}}}) {
        Bus b;auto t=combinePagedParties(roster(0),roster(1));writeParty(&b.c,t.combined);
        b.put(0x02023BCE,active[0],2);b.put(0x02023BD2,active[1],2);
        BattleExperience xp(&b.c,t);b.memory[0x02023FE0]=1;const auto writes=b.partyWrites.size();
        rejects([&]{xp.experienceCommand();});assert(b.partyWrites.size()==writes && readParty(&b.c)==t.combined);
    }
    const std::array<std::pair<uint32_t,uint32_t>,9> changes{{
        {0x02023BCE,2}, // still-valid same-owner active slot changed
        {0x02023BD2,3}, // the other active slot must remain pinned too
        {0x0203805C,2}, // queued switch changed
        {0x0203805E,7}, // malformed queued switch
        {0x02023F40,1}, // Wish countdown changed
        {0x02023F44,4}, // Wish source changed, even while its countdown is zero
        {0x02023F4E,0}, // participation changed
        {0x02023F49,4}, // Knock Off reference changed
        {0x02038010,2}  // native recipient changed
    }};
    for(unsigned state:{2u,5u})for(const auto& change:changes) {
        Bus b;auto t=combinePagedParties(roster(0),roster(1));writeParty(&b.c,t.combined);
        BattleExperience xp(&b.c,t);b.memory[0x02023FE0]=1;assert(xp.experienceCommand());
        b.memory[0x02023FE0]=uint8_t(state);b.memory[change.first]=uint8_t(change.second);
        const auto before=readParty(&b.c);const auto originals=t.originals;const auto writes=b.partyWrites.size();
        rejects([&]{xp.experienceCommand();});
        assert(b.partyWrites.size()==writes && readParty(&b.c)==before && t.originals==originals);
    }
    // State 5 is not a completion boundary while another level's EXP remains
    // or a controller still owns the award's task/text result.
    Bus b;auto t=combinePagedParties(roster(0),roster(1));writeParty(&b.c,t.combined);
    BattleExperience xp(&b.c,t);b.memory[0x02023FE0]=1;assert(xp.experienceCommand());
    b.memory[0x02023FE0]=5;b.put(0x02023D50,1);const auto writes=b.partyWrites.size();
    assert(!xp.experienceCommand() && b.partyWrites.size()==writes && xp.temporaryParty());
    b.put(0x02023D50,0);b.put(0x02023BC8,1);
    rejects([&]{xp.experienceCommand();});assert(b.partyWrites.size()==writes);
}
void retiredReferences() {
    Bus b;auto t=combinePagedParties(roster(0),roster(1));writeParty(&b.c,t.combined);BattleExperience xp(&b.c,t);
    b.memory[0x02023F4E]=0x3F;b.memory[0x02023F4F]=0x3F;b.memory[0x02023F49]=4;b.memory[0x03004F24]=4;
    xp.observeBattle();assert(b.memory[0x03004F24]==0);
    auto refs=readBenchReferences(&b.c,6,6);
    auto menu=planPartyMenu(t,readParty(&b.c),0,refs);
    assert(!planPartyMenuSelection(menu,menu.menu,5).accepted);
    auto selected=planPartyMenuSelection(menu,menu.menu,5,true);assert(selected.accepted && selected.imported);
    t=selected.transaction;writeParty(&b.c,selected.party);xp.projectReferences();
    const unsigned reused=selected.nativeSlot;assert(!(b.memory[0x02023F49]&(1u<<reused)));
    assert(!(b.memory[0x02023F4E]&(1u<<reused)));
    refs=readBenchReferences(&b.c,6,6);menu=planPartyMenu(t,readParty(&b.c),0,refs);
    selected=planPartyMenuSelection(menu,menu.menu,1,true);assert(selected.accepted);
    t=selected.transaction;writeParty(&b.c,selected.party);xp.projectReferences();
    assert(b.memory[0x02023F49]&(1u<<selected.nativeSlot));assert(b.memory[0x02023F4E]&(1u<<selected.nativeSlot));
    xp.opponentReset(1);b.memory[0x02023F4E]=3;xp.projectReferences();assert(b.memory[0x02023F4E]==0 && b.memory[0x02023F4F]);
    // Active/queued/Wish protection remains even when EXP is journaled.
    refs=readBenchReferences(&b.c,6,6);refs.queued[0]=2;refs.wishCounters[0]=1;refs.wishSlots[0]=4;
    menu=planPartyMenu(t,readParty(&b.c),0,refs);assert(!planPartyMenuSelection(menu,menu.menu,5,true).accepted);
    rejects([&]{xp.opponentReset(4);});
}
void shedinjaEvolutionJournal() {
    Bus b;auto left=roster(0),right=roster(1);
    for(auto* p:{&left,&right}) {
        p->count=5;p->mons[5]={};auto x=p->mons[0].secure();monPut16(x.data(),301);monPut32(x.data()+44,0x80000000);p->mons[0].setSecure(x);
    }
    auto t=combinePagedParties(left,right);writeParty(&b.c,t.combined);BattleExperience xp(&b.c,t);
    b.memory[0x03004F24]=3;xp.evolutionGate();xp.evolutionBoundary(false);
    for(unsigned owner=0;owner<2;++owner) {
        auto party=readParty(&b.c);auto plain=party.mons[0].secure();monPut16(plain.data(),302);party.mons[0].setSecure(plain);
        party.mons[5]=party.mons[0];auto& shell=party.mons[5];monPut16(plain.data(),303);monPut16(plain.data()+2,0);shell.setSecure(plain);
        shell.bytes[85]=255;monPut16(shell.bytes.data()+86,1);monPut16(shell.bytes.data()+88,1);party.count=6;
        writeParty(&b.c,party);b.memory[0x03004F24]=0;
        assert(xp.evolutionBoundary(true)==(owner==0));
    }
    const auto results=restoreParties(t,readParty(&b.c));
    for(const auto& p:results)assert(p.count==6 && nativeShedinjaSiblings(p.mons[0],p.mons[5]));
    // A subsequent shared battle can page the shell, and both records retain
    // their own progress through menu permutations and save party validation.
    auto next=combinePagedParties(results[0],results[1]);writeParty(&b.c,next.combined);
    const auto view=planPartyMenu(next,next.combined,0,readBenchReferences(&b.c,6,6));
    const auto picked=planPartyMenuSelection(view,view.menu,5,true);
    assert(picked.accepted && picked.imported && picked.party.mons[picked.nativeSlot].species()==303);
    auto current=picked.party;current.mons[picked.nativeSlot].setPp(0,1);std::swap(current.mons[0],current.mons[picked.nativeSlot]);
    const auto restored=restoreParties(picked.transaction,current);
    assert(restored[0].mons[5].pp(0)==1 && restored[0].mons[0].pp(0)==35 && restored[1]==results[1]);
    // A second shell with the same origin is still an ambiguous duplicate.
    auto invalid=results[0];invalid.mons[4]=invalid.mons[5];rejects([&]{invalid.validate();});
    auto altered=results[0];auto x=altered.mons[5].secure();x[40]^=1;altered.mons[5].setSecure(x);rejects([&]{altered.validate();});
    Party a{},c{};a.count=c.count=1;a.mons[0]=results[0].mons[0];c.mons[0]=results[0].mons[5];
    const auto split=combinePagedParties(a,c);assert((restoreParties(split,split.combined)==std::array<Party,2>{{a,c}}));
}

}
int main(){
    allRecipientsAndEvolutions();
    for(unsigned active0:{0u,2u,4u})for(unsigned active1:{1u,3u,5u})
        allRecipientsAndEvolutions(true,active0,active1);
    invalidAwardReferences();retiredReferences();shedinjaEvolutionJournal();
}
