// SPDX-License-Identifier: GPL-3.0-or-later
// Authored bus/machine/Flash fixtures. No commercial game bytes.
#include "../gba/FireRedCampaignSync.h"
#include <cstring>
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
struct Bus {
    mCore core{};
    std::vector<uint8_t> ew=std::vector<uint8_t>(0x40000),iw=std::vector<uint8_t>(0x8000),rom=std::vector<uint8_t>(0x180000),flash=std::vector<uint8_t>(131072,0xFF);
    unsigned snapshots=0,restores=0,flashRestores=0;
    bool inject=false;
    uint8_t& at(uint32_t a) {
        if(a>=0x02000000 && a<0x02040000)return ew[a-0x02000000];
        if(a>=0x03000000 && a<0x03008000)return iw[a-0x03000000];
        if(a>=0x08000000 && a<0x08000000+rom.size())return rom[a-0x08000000];
        throw std::runtime_error("Campaign bus escaped fixture");
    }
    void put(uint32_t a,uint32_t value,unsigned n=4){for(unsigned i=0;i<n;++i)at(a+i)=uint8_t(value>>(8*i));}
    Bus() {
        core.board=this;
        core.busRead8=[](mCore*c,uint32_t a)->uint32_t{return static_cast<Bus*>(c->board)->at(a);};
        core.busRead16=[](mCore*c,uint32_t a)->uint32_t{return inventoryRead16(c,a);};
        core.busRead32=[](mCore*c,uint32_t a)->uint32_t{return inventoryRead32(c,a);};
        core.busWrite8=[](mCore*c,uint32_t a,uint8_t v){auto*b=static_cast<Bus*>(c->board);b->at(a)=v;
            if(b->inject && a==0x02000FE4){b->inject=false;b->at(CampaignMapHeaderAddress)^=1;}};
        core.stateSize=[](mCore*)->size_t{return 0x48000;};
        core.saveState=[](mCore*c,void*out){auto*b=static_cast<Bus*>(c->board);++b->snapshots;
            std::memcpy(out,b->ew.data(),b->ew.size());std::memcpy(static_cast<uint8_t*>(out)+b->ew.size(),b->iw.data(),b->iw.size());return true;};
        core.loadState=[](mCore*c,const void*in){auto*b=static_cast<Bus*>(c->board);++b->restores;
            std::memcpy(b->ew.data(),in,b->ew.size());std::memcpy(b->iw.data(),static_cast<const uint8_t*>(in)+b->ew.size(),b->iw.size());return true;};
        core.savedataClone=[](mCore*c,void**out)->size_t{const auto&v=static_cast<Bus*>(c->board)->flash;*out=std::malloc(v.size());std::memcpy(*out,v.data(),v.size());return v.size();};
        core.savedataRestore=[](mCore*c,const void*in,size_t size,bool){auto*b=static_cast<Bus*>(c->board);++b->flashRestores;
            if(size!=b->flash.size())return false;
            std::memcpy(b->flash.data(),in,size);return true;};
        put(SaveBlock1PointerAddress,0x02000000);put(SaveBlock2PointerAddress,0x02010000);
        put(0x03003044,0x080565D5);put(GlobalScriptStatusAddress,2,1);
        put(GlobalScriptContextAddress+0x5C,0x08008000);put(GlobalScriptContextAddress+0x60,0x08008400);
        put(CampaignObjectsAddress,0x10001);put(CampaignObjectsAddress+8,0xFF,1);
        for(unsigned offset:{0x10u,0x12u,0x14u,0x16u})put(CampaignObjectsAddress+offset,12,2);
        put(CampaignMapHeaderAddress,0x08001000);put(CampaignMapHeaderAddress+4,0x08002000);
        put(0x08001000,20);put(0x08001004,20);put(0x0800100C,0x08003000);
        auto p=readProgress(&core);p.flag(FlagPokemonObtained,true);p.flag(FlagPokedexObtained,true);p.var(VarLabScene,6);
        p.inventory.setQuantity(13,7);writeProgressDelta(&core,readProgress(&core),p);
    }
    Progress state(){return readProgress(&core);}
    void state(const Progress&p){writeProgressDelta(&core,state(),p);}
};
void ordinary() {
    Bus a,b;auto p=a.state();p.flag(0x821,true);p.flag(0x4B1,true);a.state(p);
    CampaignProgress sync({{&a.core,&b.core}});
    a.at(QuestLogStateAddress)=2;sync.sample();check(!b.state().flag(0x821),"Campaign read a historical quest-log source");
    a.at(QuestLogStateAddress)=1;b.at(QuestLogStateAddress)=3;sync.sample();check(!b.state().flag(0x821),"Campaign wrote into a final quest-log transition");
    b.at(QuestLogStateAddress)=0;
    b.at(FieldControlsLockAddress)=1;sync.sample();check(!b.state().flag(0x821),"Campaign bypassed an owned menu");
    b.at(FieldControlsLockAddress)=0;sync.sample();
    check(b.state().flag(0x821) && b.state().flag(0x297) && b.state().inventory.quantity(291)==1,"Campaign failed own Misty receipt");
    check(b.state().inventory.quantity(13)==7,"Campaign overwrote own healing items");
    check(a.snapshots==1 && b.snapshots==1,"Campaign did not snapshot both machines");
    CampaignProgress restarted({{&a.core,&b.core}});restarted.sample();restarted.sample();
    check(b.state().inventory.quantity(291)==1,"Campaign restart duplicated the own TM");
}
void rollback() {
    Bus a,b;auto p=a.state();p.flag(0x821,true);p.flag(0x4B1,true);a.state(p);
    auto q=b.state();q.flag(0x823,true);q.flag(0x4B3,true);b.state(q);
    const auto ae=a.ew,ai=a.iw,be=b.ew,bi=b.iw,af=a.flash,bf=b.flash;
    b.inject=true;CampaignProgress sync({{&a.core,&b.core}});bool rejected=false;
    try{sync.sample();}catch(const std::runtime_error&){rejected=true;}
    check(rejected,"Campaign accepted a changed map preimage during paired commit");
    check(a.ew==ae && a.iw==ai && b.ew==be && b.iw==bi,"Campaign rollback did not restore both complete machines");
    check(a.flash==af && b.flash==bf && a.flashRestores==1 && b.flashRestores==1,"Campaign rollback did not restore and verify both Flash buffers");
    check(a.restores==1 && b.restores==1,"Campaign rollback omitted one player");
}
void league() {
    Bus a,b;auto own=b.state();own.mapGroup=1;own.mapNum=75;own.var(0x4068,1);
    b.put(0x02000004,1,1);b.put(0x02000005,75,1);own=b.state();own.var(0x4068,1);b.state(own);
    auto source=a.state();source.flag(0x4B8,true);source.var(0x4068,2);source.mapGroup=1;source.mapNum=76;
    a.put(0x02000004,1,1);a.put(0x02000005,76,1);source.mapGroup=1;source.mapNum=76;a.state(source);
    CampaignProgress sync({{&a.core,&b.core}});sync.sample();
    check(b.state().flag(0x4B8) && b.state().var(0x4068)==1,"League catchup copied the other room stage");
    auto reset=b.state();reset.flag(0x4B8,false);reset.var(0x4068,0);b.state(reset);sync.sample();
    check(!b.state().flag(0x4B8),"Old League win survived a native expedition reset");
}
void networkPreview(bool largePureUpdate) {
    Bus a,b;
    for(auto* owner:{&a,&b}) {
        auto p=owner->state();p.flag(0x82C,true);p.flag(0x840,true);p.var(0x404E,0x6258);
        p.flag(0x845,true);p.flag(0x846,true);p.var(0x4076,5);
        p.inventory.setQuantity(368,1);p.inventory.setQuantity(374,1);owner->state(p);
        owner->put(0x0201001B,0xB9,1);
        constexpr std::array<uint8_t,20> native{{0x25,0x9A,1,0x29,0x44,8,0x29,0x5C,0,
            0x29,0x8C,0,0x16,0x76,0x40,6,0,0x25,0x29,1}};
        for(unsigned i=0;i<native.size();++i)owner->put(0x08170F94+i,native[i],1);
    }
    auto source=a.state();source.flag(0x844,true);source.var(0x4076,6);source.flag(0x2DD,true);
    source.inventory.setQuantity(374,0);a.state(source);
    // Authored native map: one conditional 3-tile machine. An optional 8-tile
    // pure flag update consumes the first refresh's entire 80-byte workspace.
    std::vector<uint8_t> setup;
    auto byte=[&](unsigned n){setup.push_back(uint8_t(n));};
    auto half=[&](unsigned n){byte(n);byte(n>>8);};
    auto pointer=[&](unsigned n){half(n);half(n>>16);};
    auto conditional=[&](unsigned flag,unsigned count,unsigned xStart) {
        byte(0x2B);half(flag);byte(0x06);byte(0);pointer(0x08006100+setup.size()+4+count*9);
        for(unsigned x=0;x<count;++x){byte(0xA2);half(xStart+x);half(3);half(0x200+x);half(0);}
    };
    if(largePureUpdate)conditional(0x2DD,8,0);
    conditional(0x844,3,10);byte(2);
    b.put(CampaignMapHeaderAddress+8,0x08006000);b.put(0x08006000,1,1);b.put(0x08006001,0x08006100);
    for(unsigned i=0;i<setup.size();++i)b.put(0x08006100+i,setup[i],1);
    const auto prefix=nativeNetworkCompletionCommands(&b.core,b.state());
    CampaignProgress sync({{&a.core,&b.core}});sync.sample();
    if(largePureUpdate) {
        check(b.state().flag(0x2DD) && !b.state().flag(0x844),"Large pure map update lost the durable pending native completion");
        check(b.at(GlobalScriptContextAddress+13)==0xA2,"Capacity fallback ran native metadata before the independent pure refresh");
        // Fake only the completion of that literal native resource program;
        // the real emulator path is checked by private-ROM acceptance.
        b.at(GlobalScriptStatusAddress)=2;b.at(GlobalScriptContextAddress+1)=0;b.at(FieldControlsLockAddress)=0;
        a.at(GlobalScriptStatusAddress)=2;a.at(GlobalScriptContextAddress+1)=0;a.at(FieldControlsLockAddress)=0;
        sync.sample();
    }
    check(!b.state().flag(0x844) && b.state().var(0x4076)==5 && b.state().inventory.quantity(374)==1,
          "Runtime preview fabricated native completion flags/stage or consumed Sapphire before the special");
    for(unsigned i=0;i<prefix.size();++i)
        check(b.at(GlobalScriptContextAddress+13+i)==prefix[i],"Native completion did not precede its own loaded map refresh");
    check(b.at(GlobalScriptContextAddress+13+prefix.size())==0xA2,"Native final-state preview omitted the loaded network machine tiles");
}
}
int main(){ordinary();rollback();league();networkPreview(false);networkPreview(true);std::cout<<checks<<" campaign broker checks passed\n";}
