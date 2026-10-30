// SPDX-License-Identifier: GPL-3.0-or-later
#include "../gba/FireRedBench.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks = 0;
void check(bool value, const char* text) { ++checks; if (!value) throw std::runtime_error(text); }
template<class F> void rejects(F f, const char* text) {bool rejected = false;try {f();} catch(const std::runtime_error&) {rejected = true;}check(rejected,text);}
Party party(unsigned owner) {
    Party p;p.count=6;
    for(unsigned slot=0;slot<6;++slot) {
        auto& m=p.mons[slot];monPut32(m.bytes.data(),24*(owner*6+slot+1));monPut32(m.bytes.data()+4,owner+1);
        m.bytes[8]=0xBB;m.bytes[9]=0xFF;m.bytes[18]=5;m.bytes[19]=2;m.bytes[20]=0xBB;m.bytes[21]=0xFF;m.bytes[84]=10;
        monPut16(m.bytes.data()+86,30);monPut16(m.bytes.data()+88,30);
        for(unsigned i=90;i<100;i+=2) monPut16(m.bytes.data()+i,20);
        PartyMon::Secure s{};monPut16(s.data(),1);monPut32(s.data()+4,1000);s[9]=70;
        monPut16(s.data()+12,33);s[20]=35;monPut32(s.data()+40,0x3FFFFFFF);m.setSecure(s);
    }
    p.validate();return p;
}
void safeRotation() {
    auto a=party(0),b=party(1);auto transaction=combineParties(a,b);auto current=transaction.combined;
    BenchReferences refs;refs.active={{0,1}};refs.participation={{0x3F,0x3F}};refs.knockedOff=0x3F;
    for(unsigned owner=0;owner<2;++owner) for(unsigned i=0;i<PartyCapacity;++i)
        if(transaction.owners[i]==int(owner))current.mons[i].setHp(0);
    // Loaded living bench members always take precedence over a refill.
    auto living=current;living.mons[2].setHp(1);check(!planBenchRefill(transaction,living,0,refs),"Replaced an available living bench member");
    refs.wishCounters[0]=2;refs.wishSlots[0]=2;
    auto r=planBenchRefill(transaction,current,0,refs);check(bool(r),"No safe bench refill");
    check(r->retiredNativeSlot==4 && r->importedOriginalSlot==3,"Wish-referenced slot was reused");
    check(r->references.participation[0]==0x2F && r->references.participation[1]==0x2F && r->references.knockedOff==0x2F,"Retired Pokemon's slot bits were inherited");
    check(r->references.wishSlots==refs.wishSlots && r->references.wishCounters==refs.wishCounters,"Wish data changed");
    const auto own=restoreParties(r->transaction,r->party);
    for(unsigned i=0;i<3;++i)check(own[0].mons[i].hp()==0,"Retired fainted Pokemon journal was lost");
    for(unsigned i=3;i<6;++i)check(own[0].mons[i]==a.mons[i],"Unselected Pokemon changed");
    check(own[1].mons[3]==b.mons[3],"Partner bench changed");
    refs.leveled=1<<4;check(!planBenchRefill(transaction,current,0,refs),"Retired a pending evolution");
    check(wouldAbandonLivingReserve(transaction,current,0),"Protected slots hid a living original reserve");
    check(!wouldAbandonLivingReserve(transaction,living,0),"A living working member was considered abandoned");
    rejects([&]{wouldAbandonLivingReserve(transaction,current,2);},"Invalid reserve owner accepted");
    refs.leveled=0;refs.queued[0]=4;check(!planBenchRefill(transaction,current,0,refs),"Retired a queued switch");
    refs.queued[0]=6;refs.callerExcluded[0]=4;check(!planBenchRefill(transaction,current,0,refs),"Retired caller-excluded slot");
    refs.callerExcluded[0]=6;refs.wishCounters[0]=0;
    transaction=r->transaction;current=r->party;
    // Exhaust and import the remaining three originals, with per-owner records.
    for(unsigned source=4;source<6;++source) {
        for(unsigned i=0;i<PartyCapacity;++i)if(transaction.owners[i]==0)current.mons[i].setHp(0);
        auto next=planBenchRefill(transaction,current,0,refs);check(bool(next),"Could not rotate all six originals");
        check(next->importedOriginalSlot==source,"Wrong original bench order");transaction=next->transaction;current=next->party;
    }
    for(unsigned i=0;i<PartyCapacity;++i)if(transaction.owners[i]==0)current.mons[i].setHp(0);
    check(!planBenchRefill(transaction,current,0,refs),"Imported an already retired Pokemon");
    const auto result=restoreParties(transaction,current);for(const auto&m:result[0].mons)check(m.hp()==0,"Exhausted owner's original was revived");
    check(!wouldAbandonLivingReserve(transaction,current,0),"A genuinely exhausted roster was considered usable");
}
struct Bus {
    mCore core{};
    std::array<uint8_t,0x40000> ewram{};
    std::array<uint8_t,0x8000> iwram{};
    unsigned writes=0;
    uint8_t& byte(uint32_t address) {
        if(address>=0x02000000&&address<0x02040000)return ewram[address-0x02000000];
        if(address>=0x03000000&&address<0x03008000)return iwram[address-0x03000000];
        throw std::runtime_error("Native bench accessed outside RAM");
    }
    Bus() {
        core.board=this;
        core.busRead8=[](mCore*c,uint32_t a)->uint32_t{return static_cast<Bus*>(c->board)->byte(a);};
        core.busRead16=[](mCore*c,uint32_t a)->uint32_t{auto&b=*static_cast<Bus*>(c->board);return unsigned(b.byte(a))|(unsigned(b.byte(a+1))<<8);};
        core.busRead32=[](mCore*c,uint32_t a)->uint32_t{auto&b=*static_cast<Bus*>(c->board);uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(b.byte(a+i))<<(8*i);return v;};
        core.busWrite8=[](mCore*c,uint32_t a,uint8_t v){auto&b=*static_cast<Bus*>(c->board);
            check(a==PlayerPartyCountAddress||(a>=PlayerPartyAddress&&a<PlayerPartyAddress+600)||a==0x02023F4E||a==0x02023F4F||a==0x02023F49,"Bench changed unowned native RAM");b.byte(a)=v;++b.writes;};
    }
};
void nativeBoundary() {
    Bus bus;auto t=combineParties(party(0),party(1));auto current=t.combined;
    for(unsigned i=0;i<6;++i)current.mons[i].setHp(0);
    writeParty(&bus.core,current);
    const uint32_t bs=0x02038000;for(unsigned i=0;i<4;++i)bus.byte(0x02023FE8+i)=uint8_t(bs>>(i*8));
    bus.byte(0x02023BCE)=0;bus.byte(0x02023BD2)=1;bus.byte(bs+0x5C)=6;bus.byte(bs+0x5E)=6;
    bus.byte(0x02023F4E)=0x3F;bus.byte(0x02023F4F)=0x3F;bus.byte(0x02023F49)=0x3F;
    auto refs=readBenchReferences(&bus.core,6,6);check(refs.active[1]==1&&refs.participation[1]==0x3F,"Native bench layout was misread");
    auto p=planBenchRefill(t,current,0,refs);check(bool(p),"Native plan missing");
    auto referenced=*p;referenced.references.queued[0]=referenced.retiredNativeSlot;
    const auto beforeWrites=bus.writes;
    rejects([&]{applyBenchRefill(&bus.core,referenced);},"Referenced native slot was retired");check(bus.writes==beforeWrites,"Rejected reference wrote RAM");
    applyBenchRefill(&bus.core,*p);check(readParty(&bus.core)==p->party,"Native plan did not install party");
    const auto afterWrites=bus.writes;
    rejects([&]{applyBenchRefill(&bus.core,*p);},"Stale refill applied twice");check(bus.writes==afterWrites,"Stale refill wrote RAM");
    check(bus.byte(0x02023F4E)==0x3B&&bus.byte(0x02023F4F)==0x3B&&bus.byte(0x02023F49)==0x3B,"Native slot mask writes differ");
    auto broken=*p;broken.party.mons[0].bytes[33]^=1;const auto writes=bus.writes;
    rejects([&]{applyBenchRefill(&bus.core,broken);},"Corrupt native refill applied");check(bus.writes==writes,"Rejected refill wrote native RAM");
    bus.byte(0x02023FE8)=1;rejects([&]{readBenchReferences(&bus.core,6,6);},"Misaligned native battle pointer accepted");
}
void referencesAndRejection() {
    auto t=combineParties(party(0),party(1));auto c=t.combined;for(unsigned i=0;i<c.count;++i)c.mons[i].setHp(0);
    BenchReferences refs;refs.active={{0,1}};
    // Enemy Wish IDs index the enemy party, not either human's roster.
    refs.wishCounters[1]=2;refs.wishSlots[1]=2;
    auto p=planBenchRefill(t,c,0,refs);check(p && p->retiredNativeSlot==2,"Enemy Wish blocked player's slot");
    for(unsigned v:{7u,254u,256u}){auto r=refs;r.queued[0]=v;rejects([&]{planBenchRefill(t,c,0,r);},"Invalid party reference accepted");}
    auto bad=refs;bad.leveled=0x80;rejects([&]{planBenchRefill(t,c,0,bad);},"Invalid level mask accepted");
    bad=refs;bad.participation[0]=0x40;rejects([&]{planBenchRefill(t,c,0,bad);},"Invalid participation mask accepted");
    bad=refs;bad.knockedOff=0x80;rejects([&]{planBenchRefill(t,c,0,bad);},"Invalid Knock Off mask accepted");
    rejects([&]{planBenchRefill(t,c,2,refs);},"Foreign owner accepted");
    auto corrupt=c;corrupt.mons[2].bytes[33]^=1;rejects([&]{planBenchRefill(t,corrupt,0,refs);},"Corrupt party accepted");
    check(t.combined==combineParties(party(0),party(1)).combined,"Rejected plan mutated transaction");
}
}
int main(){try{safeRotation();referencesAndRejection();nativeBoundary();std::cout<<"FireRed bench tests: "<<checks<<" checks passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
