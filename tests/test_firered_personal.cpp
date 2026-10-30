// SPDX-License-Identifier: GPL-3.0-or-later
// Authored RAM/Pokemon fixtures only; no ROM or save files.
#include "../gba/FireRedPersonal.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F fn){bool failed=false;try{fn();}catch(const std::runtime_error&){failed=true;}check(failed,"Invalid personal state accepted");}
struct Bus {
    std::array<uint8_t,0x40000> ram{};
    std::array<uint8_t,0x8000> iram{};
    mCore c{};
    uint32_t b1=0x02028000,b2=0x02025000;
    unsigned writes=0;
    uint8_t& at(uint32_t a){if(a>=0x02000000&&a<0x02040000)return ram[a-0x02000000];check(a>=0x03000000&&a<0x03008000,"RAM escape");return iram[a-0x03000000];}
    void put32(uint32_t a,uint32_t v){for(unsigned i=0;i<4;++i)at(a+i)=uint8_t(v>>(i*8));}
    Bus(){
        c.board=this;c.busRead8=[](mCore* c,uint32_t a)->uint32_t{return static_cast<Bus*>(c->board)->at(a);};
        c.busWrite8=[](mCore* c,uint32_t a,uint8_t v){auto& b=*static_cast<Bus*>(c->board);b.at(a)=v;++b.writes;};
        put32(SaveBlock1PointerAddress,b1);put32(SaveBlock2PointerAddress,b2);put32(b2+InventoryKeyOffset,0x12345678);
        put32(b1+CaptureCountOffset,0x12345678);
    }
};
PartyMon pokemon(){
    PartyMon m;monPut32(m.bytes.data(),24);monPut32(m.bytes.data()+4,123);m.bytes[18]=5;m.bytes[19]=2;m.bytes[84]=10;
    monPut16(m.bytes.data()+86,20);monPut16(m.bytes.data()+88,30);
    PartyMon::Secure s{};monPut16(s.data(),19);monPut32(s.data()+4,1000);monPut16(s.data()+12,33);monPut16(s.data()+14,45);s[20]=7;s[21]=11;m.setSecure(s);return m;
}
void plans(){
    PersonalCaptureState before;
    for(unsigned old=0;old<BoxCount;++old)for(unsigned current=0;current<BoxCount;++current)for(unsigned dest=0;dest<BoxCount;++dest)for(unsigned shown=0;shown<2;++shown){
        before.pcBox=uint16_t(old);before.shownBoxFull=shown;before.captures=11;
        const auto party=planPersonalCapture(before,std::nullopt,current);
        check(party.pcBox==old&&party.shownBoxFull==bool(shown)&&party.captures==12,"Party capture changed box metadata");
        const auto pc=planPersonalCapture(before,dest,current);
        const bool expected=(old==dest&&shown)||current!=dest;
        check(pc.pcBox==dest&&pc.shownBoxFull==expected&&pc.captures==12,"PC metadata differs from native rules");
        check(pc.owned==before.owned&&pc.seen==before.seen,"PC plan altered dex flags");
    }
    before.captures=MaximumGameStat;check(planPersonalCapture(before,std::nullopt,0).captures==MaximumGameStat,"Capture stat overflow");
    rejects([&]{planPersonalCapture(before,14,0);});rejects([&]{planPersonalCapture(before,0,14);});before.pcBox=14;rejects([&]{before.validate();});
}
void memory(){
    Bus b;auto state=readPersonalCapture(&b.c);state.pcBox=8;state.shownBoxFull=true;state.captures=998;state.unown=0x1234;state.spinda=0x5678;
    state.owned[3]=8;state.seen[0][1]=4;state.seen[1][5]=2;state.seen[2][7]=1;
    b.at(b.b1+0xEE0+ShownBoxFullFlag/8)=0xA5;
    const auto before=b.ram;writePersonalCapture(&b.c,state);check(readPersonalCapture(&b.c)==state,"Personal state roundtrip");
    check(b.at(b.b1+0xEE0+ShownBoxFullFlag/8)==0xAD,"Neighboring personal flags changed");
    for(unsigned i=0;i<b.ram.size();++i){
        const uint32_t a=0x02000000+i;
        const auto within=[&](uint32_t base,unsigned n){return a>=base&&a<base+n;};
        const bool allowed=within(b.b2+DexOwnedOffset,52)||within(b.b2+DexSeenOffset,52)||within(b.b1+DexSeen1Offset,52)||within(b.b1+DexSeen2Offset,52)||
            within(b.b2+DexUnownPersonalityOffset,4)||within(b.b2+DexSpindaPersonalityOffset,4)||within(b.b1+CaptureCountOffset,4)||
            within(b.b1+CaptureBoxVariableOffset,2)||a==b.b1+0xEE0+ShownBoxFullFlag/8;
        check(allowed||b.ram[i]==before[i],"Personal writer changed an unrelated save byte");
    }
    const auto writes=b.writes;state.captures=MaximumGameStat+1;rejects([&]{writePersonalCapture(&b.c,state);});check(b.writes==writes,"Invalid state wrote RAM");
    state.captures=1;state.shownBoxFull=false;writePersonalCapture(&b.c,state);check(b.at(b.b1+0xEE0+ShownBoxFullFlag/8)==0xA5,"Flag clear damaged neighbors");
}
void pp(){
    const auto before=pokemon();auto healed=before;auto s=healed.secure();s[20]=35;s[21]=40;healed.setSecure(s);
    check(preservePartyCapturePP(healed,before)==before,"Temporary PC path healed actual party capture PP");
    auto other=before;monPut32(other.bytes.data(),48);rejects([&]{preservePartyCapturePP(healed,other);});
    other=before;s=other.secure();monPut16(s.data()+12,45);other.setSecure(s);rejects([&]{preservePartyCapturePP(healed,other);});
}
}
int main(){try{plans();memory();pp();std::cout<<"FireRed personal capture: "<<checks<<" checks passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
