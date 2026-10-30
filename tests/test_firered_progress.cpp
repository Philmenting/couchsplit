// SPDX-License-Identifier: GPL-3.0-or-later
// Authored progress and emulator memory; no commercial ROM data.
#include "../gba/FireRedBattle.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks = 0;
void check(bool value, const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F f, const char* message) {
    bool rejected = false; try { f(); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, message);
}
Progress started() {
    Progress p; p.flag(FlagPokemonObtained, true); p.flag(0x92, true); p.var(VarLabScene, 4);
    p.gcnLinkFlags = 0x9000; p.inventory.setQuantity(13, 7); return p;
}
void pure() {
    auto p = started();
    auto parcel = applyStoryEvent(p, StoryEvent::Parcel);
    check(parcel.applied && parcel.progress.var(VarLabScene)==5 && parcel.progress.var(VarMartScene)==1, "Parcel stage failed");
    check(parcel.progress.inventory.quantity(OaksParcelItem)==1 && parcel.progress.inventory.quantity(13)==7, "Parcel ownership failed");
    check(!applyStoryEvent(parcel.progress, StoryEvent::Parcel).applied, "Parcel was duplicated");
    check(!applyStoryEvent(p, StoryEvent::Pokedex).applied, "Dex skipped parcel prerequisite");
    auto dex = applyStoryEvent(parcel.progress, StoryEvent::Pokedex);
    check(dex.applied && dex.progress.flag(FlagPokedexObtained) && dex.progress.inventory.quantity(PokeBallItem)==5, "Dex reward failed");
    check(dex.progress.inventory.quantity(OaksParcelItem)==0 && dex.progress.gcnLinkFlags==0x9031, "Parcel removal or GCN bits failed");
    check(!applyStoryEvent(dex.progress, StoryEvent::Pokedex).applied, "Dex reward duplicated");
    auto full = parcel.progress; full.inventory.setQuantity(PokeBallItem, 999);
    const auto noRoom = applyStoryEvent(full, StoryEvent::Pokedex);
    check(!noRoom.applied && noRoom.progress==full, "Dex lost reward or partially committed on full stack");
    auto brock = applyStoryEvent(dex.progress, StoryEvent::Brock);
    check(brock.applied && brock.progress.flag(FlagBoulderBadge) && brock.progress.flag(FlagBrockTM), "Brock receipt failed");
    check(brock.progress.inventory.quantity(RockTombItem)==1, "Guest did not receive their own TM");
    check(brock.progress.inventory.quantity(TMCaseItem)==1, "First TM did not create the recipient TM Case");
    auto hadCase=dex.progress;hadCase.inventory.setQuantity(TMCaseItem,1);
    const auto withCase=applyStoryEvent(hadCase,StoryEvent::Brock);
    check(withCase.progress.inventory.quantity(TMCaseItem)==1 && withCase.progress.inventory.quantity(RockTombItem)==1,
          "Existing recipient TM Case was duplicated");
    auto keyFull=dex.progress;
    unsigned keys=0;
    for(unsigned item=1;item<ItemCount && keys<PocketCapacities[KeyItems];++item)
        if(itemPocket(item)==KeyItems && item!=TMCaseItem){keyFull.inventory.setQuantity(item,1);++keys;}
    check(keys==PocketCapacities[KeyItems],"Full-key-pocket fixture is incomplete");
    const auto blockedCase=applyStoryEvent(keyFull,StoryEvent::Brock);
    check(blockedCase.applied && blockedCase.deferredItem && !blockedCase.progress.flag(FlagBrockTM) &&
          blockedCase.progress.inventory==keyFull.inventory,"No-case/full-key-pocket partially delivered or marked TM received");
    auto caseFull=keyFull;
    for(auto& slot:caseFull.inventory.pockets[KeyItems])if(slot.item){slot={uint16_t(TMCaseItem),1};break;}
    const auto fits=applyStoryEvent(caseFull,StoryEvent::Brock);
    check(fits.applied && !fits.deferredItem && fits.progress.flag(FlagBrockTM) &&
          fits.progress.inventory.quantity(TMCaseItem)==1,"Existing case unnecessarily required a free key slot");
    check(brock.progress.flag(0x500+414) && brock.progress.flag(0x500+142), "Gym trainer wins missing");
    check(!brock.progress.flag(0x92) && brock.progress.flag(0x2E), "NPC unlock flags failed");
    check(!applyStoryEvent(brock.progress, StoryEvent::Brock).applied, "Brock duplicated TM");
    full=dex.progress;full.inventory.setQuantity(RockTombItem,999);
    const auto deferred=applyStoryEvent(full,StoryEvent::Brock);
    check(deferred.applied && deferred.deferredItem && deferred.progress.flag(FlagBoulderBadge), "Full TM stack blocked badge");
    check(!deferred.progress.flag(FlagBrockTM) && deferred.progress.inventory.quantity(RockTombItem)==999, "Undelivered TM receipt set");
    // Native Brock can still deliver the TM later: its own receipt remains unset.
    auto rival = started(); rival.var(VarLabScene,3);
    const auto rivalResult=applyStoryEvent(rival,StoryEvent::LabRival);
    check(rivalResult.applied && rivalResult.progress.var(VarLabScene)==4 && rivalResult.progress.flag(0x258), "Initial rival completion failed");
    check(!applyStoryEvent(rivalResult.progress,StoryEvent::LabRival).applied, "Initial rival replayed");
    auto novice=started();novice.flag(FlagPokemonObtained,false);
    check(!applyStoryEvent(novice,StoryEvent::Parcel).applied, "Starter was skipped");
    auto advanced = parcel.progress;advanced.var(0x4054,5);advanced.var(0x407C,3);
    const auto newer=applyStoryEvent(advanced,StoryEvent::Pokedex);
    check(newer.progress.var(0x4054)==5 && newer.progress.var(0x407C)==3, "Recipient later progress regressed");
    // Every non-allowlisted flag and variable survives the transaction exactly.
    for(unsigned bit=0;bit<ProgressFlagBytes*8;++bit) {
        if(bit==FlagPokemonObtained || bit==FlagPokedexObtained || bit==0x3A || bit==0x2D)continue;
        auto own=parcel.progress;own.flag(bit,true);const auto result=applyStoryEvent(own,StoryEvent::Pokedex);
        check(result.progress.flag(bit),"Dex changed an unrelated flag");
    }
    for(unsigned id=0x4000;id<0x4100;++id) {
        if(id==VarLabScene||id==VarMartScene||id==0x407C||id==0x4051||id==0x4058||id==0x4054)continue;
        auto own=parcel.progress;own.var(id,0xBEEF);const auto result=applyStoryEvent(own,StoryEvent::Pokedex);
        check(result.progress.var(id)==0xBEEF,"Dex changed an unrelated variable");
    }
    auto before=p;before.mapGroup=5;before.mapNum=3;auto after=parcel.progress;
    check(completedStoryEvents(before,after)==std::vector<StoryEvent>{StoryEvent::Parcel},"Native parcel transition missed");
    before.mapNum=4;check(completedStoryEvents(before,after).empty(),"Wrong-map parcel transition accepted");
    before=parcel.progress;before.mapGroup=4;before.mapNum=3;after=dex.progress;
    check(completedStoryEvents(before,after)==std::vector<StoryEvent>{StoryEvent::Pokedex},"Native dex transition missed");
    after.inventory.setQuantity(PokeBallItem,0);check(completedStoryEvents(before,after).empty(),"Incomplete dex reward shared");
    before=dex.progress;before.mapGroup=6;before.mapNum=2;after=brock.progress;
    check(completedStoryEvents(before,after)==std::vector<StoryEvent>{StoryEvent::Brock},"Native Brock transition missed");
    after.flag(FlagBoulderBadge,false);check(completedStoryEvents(before,after).empty(),"Partial badge scene shared");
    rejects([&]{p.flag(0x900);},"Out-of-range flag accepted");
    rejects([&]{p.var(0x8000);},"Special variable treated as persistent");
}
struct Bus {
    std::array<uint8_t,0x40000> ew{};std::array<uint8_t,0x8000> iw{};std::array<uint8_t,0x1000> rom{};mCore core{};
    Bus() {
        core.board=this;
        core.busRead8=[](mCore*c,uint32_t a)->uint32_t{return static_cast<Bus*>(c->board)->at(a);};
        core.busRead16=[](mCore*c,uint32_t a)->uint32_t{return inventoryRead16(c,a);};
        core.busRead32=[](mCore*c,uint32_t a)->uint32_t{return inventoryRead32(c,a);};
        core.busWrite8=[](mCore*c,uint32_t a,uint8_t v){static_cast<Bus*>(c->board)->at(a)=v;};
        put32(SaveBlock1PointerAddress,0x02000000);put32(SaveBlock2PointerAddress,0x02010000);
        put32(0x03003044,0x080565D5);at(GlobalScriptStatusAddress)=2;
        put32(GlobalScriptContextAddress+0x5C,0x08100000);put32(GlobalScriptContextAddress+0x60,0x08100400);
        // Empty pocket amounts are encrypted, too.
        for(unsigned pocket=0;pocket<PocketCount;++pocket)
            for(unsigned slot=0;slot<PocketCapacities[pocket];++slot)inventoryWrite16(&core,0x02000000+PocketOffsets[pocket]+slot*4+2,0);
    }
    uint8_t& at(uint32_t a) {
        if(a>=0x08000000&&a<0x08001000)return rom[a-0x08000000];
        if(a>=0x02000000&&a<0x02040000)return ew[a-0x02000000];
        check(a>=0x03000000&&a<0x03008000,"Progress bus boundary violated");return iw[a-0x03000000];
    }
    void put32(uint32_t a,uint32_t v){for(unsigned i=0;i<4;++i)at(a+i)=uint8_t(v>>(8*i));}
    void set(const Progress&p){const auto before=readProgress(&core);writeProgressDelta(&core,before,p);}
};
void broker() {
    Bus a,b;
    auto guest=started(); guest.inventory.setQuantity(PokeBallItem,7); guest.inventory.setQuantity(1,3);
    a.set(guest);b.set(guest);
    StoryProgress progress({{&a.core,&b.core}});progress.sample();
    auto source=applyStoryEvent(guest,StoryEvent::Parcel).progress;
    source=applyStoryEvent(source,StoryEvent::Pokedex).progress;
    a.set(source);
    {
        Bus replaySource,replayGuest;replaySource.set(source);replayGuest.set(guest);
        StoryProgress replay({{&replaySource.core,&replayGuest.core}});
        replaySource.at(QuestLogStateAddress)=2;replay.sample();
        check(readProgress(&replayGuest.core)==guest,"Story used a historical quest-log baseline");
        replaySource.at(QuestLogStateAddress)=0;replay.sample();
        check(readProgress(&replayGuest.core).flag(FlagPokedexObtained),"Story did not resume after quest-log playback");
    }
    b.at(FieldControlsLockAddress)=1;progress.sample();
    check(readProgress(&b.core)==guest && progress.pending(1)!=0,"Pending quest mutated menu-owned memory");
    // Simulate graceful save/restart with source complete and receiver waiting.
    StoryProgress restarted({{&a.core,&b.core}});restarted.sample();
    check(readProgress(&b.core)==guest,"Startup reconciliation bypassed menu ownership");
    b.at(FieldControlsLockAddress)=0;restarted.sample();
    const auto received=readProgress(&b.core);
    check(received.flag(FlagPokedexObtained)&&received.inventory.quantity(PokeBallItem)==12,"Restart lost pending quest rewards");
    check(received.inventory.quantity(1)==3 && received.inventory.quantity(13)==7,"Quest overwrote personal bag");
    check(restarted.pending(1)==0,"Delivered event remained pending");
    StoryProgress again({{&a.core,&b.core}});again.sample();again.sample();
    check(readProgress(&b.core)==received,"Startup duplicated quest rewards");
    // An already advanced guest is never brought back to the source stage.
    auto advanced=received;advanced.var(VarLabScene,9);advanced.var(0x4054,5);b.set(advanced);
    StoryProgress later({{&a.core,&b.core}});later.sample();
    check(readProgress(&b.core)==advanced,"Reconcile regressed later personal progress");
    // Startup stage 3 is healed through the native VM before queued parcel/dex.
    Bus c,d;auto early=started();early.var(VarLabScene,3);c.set(source);d.set(early);
    auto completed=readProgress(&c.core);completed.flag(0x258,true);c.set(completed);
    d.put32(GlobalScriptContextAddress+0x5C,0x08100000);d.put32(GlobalScriptContextAddress+0x60,0x08100400);
    StoryProgress ordered({{&c.core,&d.core}});ordered.sample();
    check(readProgress(&d.core).var(VarLabScene)==4 && scriptInProgress(&d.core),"Startup skipped native rival heal boundary");
    check(d.at(GlobalScriptContextAddress+12)==0x69&&d.at(GlobalScriptContextAddress+13)==0x25,"Native rival heal bytecode missing");
    check(readProgress(&d.core).inventory.quantity(OaksParcelItem)==0,"Next quest committed while native VM owned receiver");
    // Engine completion fixture; actual native execution has separate ROM proof.
    d.at(GlobalScriptStatusAddress)=2;d.at(GlobalScriptContextAddress+1)=0;d.at(FieldControlsLockAddress)=0;
    ordered.sample();check(readProgress(&d.core).flag(FlagPokedexObtained),"Contiguous startup milestone chain did not resume");
}
void martBytecode() {
    Bus b;
    b.at(0x02000004)=5;b.at(0x02000005)=3;
    b.put32(0x02036DFC,0x08000100);
    b.put32(0x08000100,11);b.put32(0x08000104,8);b.put32(0x0800010C,0x08000200);
    inventoryWrite16(&b.core,0x08000200+(3*11+1)*2,0x32A9);
    inventoryWrite16(&b.core,0x08000200+(4*11+1)*2,0x36B1);
    b.put32(GlobalScriptContextAddress+0x5C,0x08000800);
    b.put32(GlobalScriptContextAddress+0x60,0x08000B54);
    check(startStoryObjectRefresh(&b.core,StoryEvent::Pokedex,5,3),"Mart refresh was not scheduled");
    const auto code=GlobalScriptContextAddress+12;
    check(b.at(code)==0x69 && b.at(code+14)==0xA2 && b.at(code+23)==0xA2,"Mart did not use native lock/setmetatile commands");
    check(inventoryRead16(&b.core,code+19)==0x2A9 && inventoryRead16(&b.core,code+28)==0x6B1,
          "Mart did not derive original metatiles/collision from ROM");
    check(b.at(code+32)==0x25 && inventoryRead16(&b.core,code+33)==142 && b.at(code+35)==0x6B && b.at(code+36)==2,
          "Mart redraw/release/end bytecode is invalid");
    check(scriptInProgress(&b.core) && b.at(FieldControlsLockAddress)==1,"Native Mart VM did not own the field");
    Bus malformed;
    malformed.at(0x02000004)=5;malformed.at(0x02000005)=3;
    malformed.put32(0x02036DFC,0x02010000);
    const auto ram=malformed.ew;const auto iw=malformed.iw;
    rejects([&]{startStoryObjectRefresh(&malformed.core,StoryEvent::Pokedex,5,3);},"Mart accepted a non-ROM original map layout");
    check(malformed.ew==ram && malformed.iw==iw,"Rejected Mart layout partially started a script");
}
void memory() {
    Bus b;auto p=started();p.mapGroup=p.mapNum=0;b.set(p);
    check(readProgress(&b.core)==p,"Progress RAM round trip failed");
    const auto ew=b.ew;const auto after=applyStoryEvent(p,StoryEvent::Parcel).progress;
    writeProgressDelta(&b.core,p,after);check(readProgress(&b.core)==after,"Parcel RAM commit failed");
    for(unsigned i=0;i<b.ew.size();++i) {
        const bool owned=(i>=0xEE0&&i<0x1000)||(i>=0x1000&&i<0x1200)||(i>=0x310&&i<0x5F8)||(i>=0x100A8&&i<0x100AC);
        if(!owned)check(b.ew[i]==ew[i],"Story commit modified unrelated RAM");
    }
    rejects([&]{writeProgressDelta(&b.core,p,after);},"Stale story snapshot accepted");
    check(progressFieldReady(&b.core),"Idle field rejected");
    b.at(QuestLogStateAddress)=1;check(progressFieldReady(&b.core),"Normal quest-log recording blocked the field");
    b.at(QuestLogStateAddress)=2;check(!progressFieldReady(&b.core),"Historical quest-log playback mistaken for the live field");
    b.at(QuestLogStateAddress)=3;check(!progressFieldReady(&b.core),"Final quest-log transition mistaken for the live field");
    b.at(QuestLogStateAddress)=0;
    b.at(FieldControlsLockAddress)=1;check(!progressFieldReady(&b.core)&&!scriptInProgress(&b.core),"Menu mistaken for a running script");b.at(FieldControlsLockAddress)=0;
    b.at(GlobalScriptStatusAddress)=1;check(scriptInProgress(&b.core),"WAITING native task lost ownership");b.at(GlobalScriptStatusAddress)=2;
    b.at(ImmediateScriptContextAddress+1)=1;check(!progressFieldReady(&b.core),"Immediate map script ignored");b.at(ImmediateScriptContextAddress+1)=0;
    b.put32(GlobalScriptContextAddress+0x5C,0);
    rejects([&]{startStoryObjectRefresh(&b.core,StoryEvent::LabRival,0,0);},"Uninitialized native VM accepted");
}
void battleDuringReplay() {
    Bus first,second;first.put32(MainCallbackAddress,InitBattleCallback);
    TeamBattle pair({{&first.core,&second.core}});
    second.at(QuestLogStateAddress)=2;
    check(!pair.tryBegin(0)&&pair.waitingForPartner(0),"Native encounter did not wait for the partner's actual save");
    second.at(QuestLogStateAddress)=0;first.at(QuestLogStateAddress)=2;
    check(!pair.tryBegin(0)&&!pair.waitingForPartner(0),"Historical encounter was mistaken for a live battle");
}
}
int main(){try{pure();memory();broker();martBytecode();battleDuringReplay();std::cout<<"FireRed progress: "<<checks<<" checks passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
