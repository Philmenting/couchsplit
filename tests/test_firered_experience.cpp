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
    static Bus& bus(mCore*c){return *static_cast<Bus*>(c->board);}
    Bus() {
        c.board=this;
        c.busRead8=[](mCore*c,uint32_t a)->uint32_t{return bus(c).memory[a];};
        c.busRead16=[](mCore*c,uint32_t a)->uint32_t{return bus(c).memory[a]|(unsigned(bus(c).memory[a+1])<<8);};
        c.busRead32=[](mCore*c,uint32_t a)->uint32_t{uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(bus(c).memory[a+i])<<(8*i);return v;};
        c.busWrite8=[](mCore*c,uint32_t a,uint8_t v){bus(c).memory[a]=v;};
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
void allRecipientsAndEvolutions() {
    Bus b;auto t=combinePagedParties(roster(0),roster(1));const auto original=t;
    writeParty(&b.c,t.combined);BattleExperience xp(&b.c,t);std::vector<unsigned> owners;
    xp.ownerChanged=[&](unsigned owner){owners.push_back(owner);};
    b.memory[0x02023FE0]=1;assert(xp.experienceCommand());
    assert(inventoryRead16(&b.c,0x02038050)==20 && inventoryRead16(&b.c,0x02023F52)==3);
    std::set<unsigned> visited;
    for(unsigned n=0;n<12;++n) {
        const unsigned slot=b.memory[0x02038010];auto current=readParty(&b.c);
        const unsigned id=current.mons[slot].identity().personality/24-1;
        assert(visited.insert(id).second && xp.inputOwner()==id/6);
        // Model a completed native award, then verify the coordinator's merge.
        auto secure=current.mons[slot].secure();monPut32(secure.data()+4,1001);current.mons[slot].setSecure(secure);
        writeParty(&b.c,current);b.memory[0x03004F24]=uint8_t(1u<<slot);b.memory[0x02023FE0]=5;
        assert(xp.experienceCommand());
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
}
int main(){allRecipientsAndEvolutions();retiredReferences();}
