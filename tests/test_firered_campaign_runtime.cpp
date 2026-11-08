// SPDX-License-Identifier: GPL-3.0-or-later
// Authored engine-memory/script fixtures. No commercial ROM or save data.
#include "../gba/FireRedCampaignRuntime.h"
#include <iostream>
#include <unordered_map>

using namespace coop::firered;
namespace {
unsigned checks = 0;
void check(bool good, const char* text) { ++checks; if (!good) throw std::runtime_error(text); }
constexpr uint32_t Save = 0x02000000, Layout = 0x08001000, Events = 0x08002000,
                   Templates = 0x08002100, Scripts = 0x08003000, Load = 0x08003100,
                   Child = 0x08003200, Transition = 0x08003300, Tiles = 0x08004000;
struct Bus {
    mCore core{};
    std::unordered_map<uint32_t,uint8_t> data;
    unsigned writes = 0;
    Bus() {
        core.board = this;
        core.busRead8 = [](mCore* c,uint32_t a)->uint32_t { return static_cast<Bus*>(c->board)->get(a); };
        core.busRead16 = [](mCore* c,uint32_t a)->uint32_t { return inventoryRead16(c,a); };
        core.busRead32 = [](mCore* c,uint32_t a)->uint32_t { return inventoryRead32(c,a); };
        core.busWrite8 = [](mCore* c,uint32_t a,uint8_t v) { auto* b = static_cast<Bus*>(c->board); ++b->writes; b->data[a] = v; };
        put(SaveBlock1PointerAddress,Save); put(SaveBlock2PointerAddress,0x02010000);
        put(0x03003044,0x080565D5); put(GlobalScriptStatusAddress,2,1);
        put(GlobalScriptContextAddress + 0x5C,0x08008000); put(GlobalScriptContextAddress + 0x60,0x08008400);
        put(CampaignObjectsAddress,0x10001); put(CampaignObjectsAddress + 8,0xFF,1);
        put(CampaignObjectsAddress + 9,3,1); put(CampaignObjectsAddress + 10,4,1);
        put(CampaignObjectsAddress + 0x10,12,2); put(CampaignObjectsAddress + 0x12,12,2);
        put(CampaignObjectsAddress + 0x14,12,2); put(CampaignObjectsAddress + 0x16,12,2);
        put(Save + 4,4,1); put(Save + 5,3,1);
        put(CampaignMapHeaderAddress,Layout); put(CampaignMapHeaderAddress + 4,Events);
        put(CampaignMapHeaderAddress + 8,Scripts);
        put(Layout,20); put(Layout + 4,20); put(Layout + 12,Tiles);
        put(Events,2,1); put(Events + 4,Templates);
        object(0,1,0x100,4,3); object(1,2,0x101,7,3);
        for (unsigned i = 0; i < 400; ++i) put(Tiles + i*2,0x154,2);
    }
    uint8_t get(uint32_t a) const { const auto i = data.find(a); return i == data.end() ? 0 : i->second; }
    void put(uint32_t a,uint32_t v,unsigned size = 4) { for (unsigned i = 0; i < size; ++i) data[a+i] = uint8_t(v >> (8*i)); }
    void code(uint32_t a,std::initializer_list<uint8_t> bytes) { for (auto b : bytes) data[a++] = b; }
    void object(unsigned index,unsigned id,unsigned flag,int x,int y) {
        for (unsigned base : {Templates,Save + 0x8E0}) {
            const auto a = base + index * 0x18;
            put(a,id,1); put(a + 4,unsigned(x),2); put(a + 6,unsigned(y),2); put(a + 9,1,1); put(a + 0x14,flag,2);
        }
    }
    void active(unsigned index,unsigned id) {
        const auto a = CampaignObjectsAddress + index * 0x24;
        put(a,1); put(a + 8,id,1); put(a + 9,3,1); put(a + 10,4,1);
    }
    Progress state() { return readProgress(&core); }
    void state(const Progress& p) { writeProgressDelta(&core,state(),p); }
    void loadScript() { put(Scripts,1,1); put(Scripts + 1,Load); put(Scripts + 5,0,1); }
    void map(unsigned group,unsigned num) {
        put(Save+4,group,1);put(Save+5,num,1);
        put(CampaignObjectsAddress+9,num,1);put(CampaignObjectsAddress+10,group,1);
    }
    void conditionalDoor() {
        loadScript();
        // CHECKFLAG 0x102; CALL_IF equal(false check result == 0 uses <);
        // child closes (2,3), then RETURN. All bytes authored for this test.
        code(Load,{0x2B,2,1,0x07,0,0,0x32,0,8,0x02});
        code(Child,{0xA2,2,0,3,0,0x66,1,1,0,0x03});
    }
};

void objects() {
    Bus b; b.active(1,1);
    const auto before = b.state(); auto after = before; after.flag(0x100,true);
    const auto writes = b.writes;
    const auto plan = planCampaignObjectRefresh(&b.core,before,after);
    check(plan.ready && plan.removedObjects == 1 && plan.addedObjects == 0,"Hidden object was not removed");
    check(b.writes == writes && b.state() == before,"Refresh preflight mutated game state");
    check(!startCampaignObjectRefresh(&b.core,plan),"Refresh started before campaign delta");
    b.state(after);
    const auto dataBefore = b.data;
    check(startCampaignObjectRefresh(&b.core,plan),"Native object refresh did not start after its delta");
    check(b.get(GlobalScriptStatusAddress) == 0 && b.get(FieldControlsLockAddress) == 1,"Native script did not own field controls");
    check(inventoryRead32(&b.core,GlobalScriptContextAddress + 8) == GlobalScriptContextAddress + 12,"Native script pointer did not use stopped context stack");
    for (const auto& item : b.data) {
        if (item.first >= GlobalScriptContextAddress && item.first < GlobalScriptContextAddress + 92) continue;
        if (item.first == GlobalScriptStatusAddress || item.first == FieldControlsLockAddress) continue;
        const auto found = dataBefore.find(item.first);
        check(item.second == (found == dataBefore.end() ? 0 : found->second),"Starting native refresh wrote outside its owned context");
    }
    Bus reveal; auto old = reveal.state(); old.flag(0x100,true); reveal.state(old); auto now = old; now.flag(0x100,false);
    const auto add = planCampaignObjectRefresh(&reveal.core,old,now);
    check(add.ready && add.addedObjects == 1,"Unhidden visible object was not added");
    reveal.object(0,1,0x100,100,100);
    const auto distantPlan = planCampaignObjectRefresh(&reveal.core,old,now);
    check(distantPlan.ready && distantPlan.code.empty(),"Far object consumed a native visible object slot");
    reveal.object(0,1,0x100,4,3); reveal.active(1,1);
    const auto already = planCampaignObjectRefresh(&reveal.core,old,now);
    check(already.ready && already.addedObjects == 0,"Existing object was added twice");
    reveal.object(0,1,0x100,5,5); reveal.put(CampaignObjectsAddress + 0x24,0);
    check(!planCampaignObjectRefresh(&reveal.core,old,now).ready,"An appearing NPC trapped the player on their own tile");
}

void metatiles() {
    Bus b; b.conditionalDoor(); const auto before = b.state(); auto after = before; after.flag(0x102,true);
    const auto open = planCampaignObjectRefresh(&b.core,before,after);
    check(open.ready && open.changedTiles == 1,"Conditional native door did not open");
    const std::vector<uint8_t> expected{0x69,0xA2,2,0,3,0,0x54,1,0,0,0x25,0x8E,0,0x6B,2};
    check(open.code == expected,"Opening a formerly closed door did not restore the original live ROM tile");
    b.state(after);
    const auto close = planCampaignObjectRefresh(&b.core,after,before);
    check(close.ready && close.changedTiles == 1 && close.code[8] == 1,"Closing a door lost its original collision operand");
    // Never close a gate under the local avatar.
    b.put(CampaignObjectsAddress + 0x10,9,2); b.put(CampaignObjectsAddress + 0x14,9,2);
    b.put(CampaignObjectsAddress + 0x12,10,2); b.put(CampaignObjectsAddress + 0x16,10,2);
    const auto underfoot = planCampaignObjectRefresh(&b.core,after,before);
    check(!underfoot.ready && underfoot.reason.find("closing gate") != std::string::npos,"A closing gate trapped the player");
    // Unsupported commands in unexecuted branches do not block safe updates.
    b.put(CampaignObjectsAddress + 0x10,12,2); b.put(CampaignObjectsAddress + 0x14,12,2);
    b.code(Child,{0x23,0,0,0,8,0x03});
    check(planCampaignObjectRefresh(&b.core,after,after).ready,"An unexecuted native callback blocked a safe plan");
    const auto unsafe = planCampaignObjectRefresh(&b.core,after,before);
    check(!unsafe.ready && unsafe.code.empty() && unsafe.reason.find("unsupported") != std::string::npos,
          "Unsafe native callback was replayed or ignored in an executed map-load path");
}

void placements() {
    Bus b;
    b.put(Scripts,3,1); b.put(Scripts + 1,Transition);
    // A conditional permanent NPC move on transition; reset to live original
    // position when its condition disappears, preserving the player object.
    b.code(Transition,{0x2B,2,1,0x06,1,0,0x32,0,8,2});
    b.code(Child,{0x63,1,0,8,0,3,0,0x65,1,0,7,2});
    b.active(1,1);
    const auto before = b.state(); auto after = before; after.flag(0x102,true);
    const auto move = planCampaignObjectRefresh(&b.core,before,after);
    check(move.ready && move.movedObjects == 1 && move.removedObjects == 1 && move.addedObjects == 1,
          "Conditional permanent NPC movement did not use native remove/place/add");
    b.state(after); const auto reset = planCampaignObjectRefresh(&b.core,after,before);
    check(reset.ready && reset.movedObjects == 1,"Disappeared native permanent move did not restore original coordinates");
}

void derivedSetup() {
    Bus b; b.active(1,1);
    for(unsigned a : {Templates,Save + 0x8E0}) b.put(a + 1,0xF0,1);
    b.put(Scripts,3,1); b.put(Scripts + 1,Transition); b.put(Scripts + 5,1,1); b.put(Scripts + 6,Load);
    b.code(Transition,{0x2B,2,1,6,1,0,0x32,0,8,0x16,0x10,0x40,3,0,2});
    b.code(Child,{0x16,0x10,0x40,4,0,2});
    b.code(Load,{0xA2,2,0,3,0,0x10,0x40,0,0,2});
    const auto before = b.state(); auto after = before; after.flag(0x102,true);
    const auto plan = planCampaignObjectRefresh(&b.core,before,after);
    check(plan.ready && plan.changedTiles == 1 && plan.movedObjects == 1 && plan.addedObjects == 1,
          "Derived own-map graphics variable did not update dynamic NPC and variable-based tile");
    check(plan.code[4] == 0x16 && plan.code[5] == 0x10 && plan.code[6] == 0x40 && plan.code[7] == 4,
          "Derived native graphics variable was not assigned before respawning its object");
    // Common personal entry metadata is consumed without overwriting a local
    // heal location, current background music, party, inventory or coordinates.
    Bus center; center.loadScript(); center.code(Load,{0x9F,5,0,0x33,0x44,1,0,0x34,0x44,1,2});
    const auto a = center.state(); auto z = a; z.flag(0x102,true);
    const auto noReplay = planCampaignObjectRefresh(&center.core,a,z);
    check(noReplay.ready && noReplay.code.empty(),"Personal healing/music metadata was replayed during campaign sync");

    Bus dex; dex.loadScript();
    dex.code(Load,{0x26,0x0C,0x80,0x93,1,0x21,0x0C,0x80,1,0,0x06,1,0,0x32,0,8,2});
    dex.code(Child,{0x63,1,0,7,0,7,0,2});
    auto p = dex.state(); auto q = p; q.flag(0x840,true); q.var(0x404E,0x6258);
    check(planCampaignObjectRefresh(&dex.core,p,q).code.empty(),"National Dex getter ignored required native magic");
    const auto nativePreview = planCampaignObjectRefresh(&dex.core,p,q,true);
    check(nativePreview.ready && nativePreview.movedObjects == 1 && dex.get(0x0201001B) == 0,
          "Known native National Dex completion did not preview the loaded guard without fabricating magic");
    dex.put(0x0201001B,0xB9,1);
    const auto national = planCampaignObjectRefresh(&dex.core,p,q);
    check(national.ready && national.movedObjects == 1,"Native three-field National Dex predicate was not observed");
}

void originalFlatLeaves() {
    Bus b; b.loadScript();
    b.code(Load,{0x2B,2,1,0x07,1,0,0x32,0,8,2});
    for(unsigned x=0;x<10;++x) {
        const auto a = Child + x * 9;
        b.code(a,{0xA2,uint8_t(x),0,3,0,0x55,2,0,0});
    }
    b.code(Child + 90,{3});
    const auto before = b.state(); auto after = before; after.flag(0x102,true);
    const auto plan = planCampaignObjectRefresh(&b.core,before,after);
    check(plan.ready && plan.changedTiles == 10 && plan.programOffset == 32 && plan.capacity() == 60,
          "Large native tile update did not reserve stack return slots before its wrapper");
    check(plan.originalTileLeaves == std::vector<uint32_t>{Child} && plan.code.size() == 11 && plan.code[1] == 4,
          "Large tile update did not use exactly its validated original flat leaf");
    b.state(after);
    check(startCampaignObjectRefresh(&b.core,plan) && inventoryRead32(&b.core,GlobalScriptContextAddress + 8) == GlobalScriptContextAddress + 32,
          "Original-leaf wrapper overlaps native CALL return storage");
    for(unsigned i=0;i<20;++i)check(b.get(GlobalScriptContextAddress + 12 + i) == 0,"Native return slots contained executable wrapper bytes");

    Bus noLeaf; noLeaf.loadScript(); noLeaf.code(Load,{0x2B,2,1,0x07,1,0,0x32,0,8,2});
    for(unsigned x=0;x<10;++x) noLeaf.code(Child + x * 9,{0xA2,uint8_t(x),0,3,0,0x55,2,0,0});
    noLeaf.code(Child + 90,{0,3}); // NOP is outside the explicitly allowed leaf language.
    const auto old = noLeaf.state(); auto next = old; next.flag(0x102,true);
    check(!planCampaignObjectRefresh(&noLeaf.core,old,next).ready,"An unvalidated original native helper bypassed the workspace bound");
    noLeaf.code(Child + 90,{3});
    // A native variable operand can change after planning. Such a helper can
    // be evaluated into a literal plan, but must never be called directly.
    noLeaf.code(Child + 5,{0x10,0x40});
    check(!planCampaignObjectRefresh(&noLeaf.core,old,next).ready,"A variable-bearing original helper was called as a literal leaf");

    Bus surge; surge.loadScript();
    surge.code(Load,{0x25,0x5B,1,2}); // SPECIAL347 is accepted only on its exact loaded native map.
    const auto unrelated = surge.state(); auto campaign = unrelated; campaign.flag(0x102,true);
    check(!planCampaignObjectRefresh(&surge.core,unrelated,campaign).ready,"Vermilion randomization exception escaped its exact map scope");
}

void retainedPersonalMaps() {
    Bus ice;ice.map(1,111);ice.loadScript();ice.code(Load,{0x25,0x35,1,2});
    auto local=ice.state();local.flag(1,true);ice.state(local);auto shared=local;shared.flag(0x821,true);
    const auto unchangedIce=planCampaignObjectRefresh(&ice.core,local,shared);
    check(unchangedIce.ready && unchangedIce.code.empty(),"Own cracked ice blocked an unrelated campaign receipt");
    shared.flag(2,true);
    check(!planCampaignObjectRefresh(&ice.core,local,shared).ready,"Campaign refresh rewrote the own temporary ice puzzle");
    ice.map(1,110);local=ice.state();shared=local;shared.flag(0x821,true);
    check(!planCampaignObjectRefresh(&ice.core,local,shared).ready,"Icefall native-helper exception escaped its exact map");

    Bus house;house.map(35,4);house.loadScript();house.code(Load,{0x25,0xB9,1,2});
    house.put(Save+0x1228,25);house.object(0,1,0x8C,4,3);house.active(1,1);
    // active() creates the default fixture map; use this house's actual map.
    house.put(CampaignObjectsAddress+0x24+9,4,1);house.put(CampaignObjectsAddress+0x24+10,35,1);
    local=house.state();local.flag(0xA6,true);house.state(local);shared=local;shared.flag(0x8C,true);
    const auto dolls=planCampaignObjectRefresh(&house.core,local,shared);
    check(dolls.ready && dolls.removedObjects==1 && house.state()==local,
          "Lorelei quest refresh changed personal dolls or failed to remove her native object");
    house.put(Save+0x1228,0x1000000);
    check(!planCampaignObjectRefresh(&house.core,local,shared).ready,"Invalid own Hall-of-Fame counter was accepted");

    Bus cave;cave.map(1,86);cave.loadScript();cave.code(Load,{0xA7,0x41,1,2});cave.put(Save+0x32,321,2);
    local=cave.state();shared=local;shared.flag(0x821,true);
    check(planCampaignObjectRefresh(&cave.core,local,shared).ready,"Existing own Seafoam variant blocked unrelated shared progress");
    cave.put(Save+0x32,320,2);
    check(!planCampaignObjectRefresh(&cave.core,local,shared).ready,"Seafoam changed layout without a native grid transaction");
    cave.put(Save+0x32,321,2);cave.map(1,85);local=cave.state();shared=local;shared.flag(0x821,true);
    check(!planCampaignObjectRefresh(&cave.core,local,shared).ready,"Seafoam layout exception escaped its exact map scope");
}

void chamberLighting() {
    Bus b;b.map(2,27);b.loadScript();b.put(Save+0x2E,11,1);
    b.code(Load,{0x2B,0x49,8,0x06,1,0,0x32,0,8,0xA4,11,0,0xA5,2});b.code(Child,{2});
    const auto before=b.state();auto after=before;after.flag(0x849,true);
    const auto clear=planCampaignObjectRefresh(&b.core,before,after);
    check(clear.ready && clear.changedWeather && clear.code==std::vector<uint8_t>({0x69,0xA3,0xA5,0x6B,2}),
          "Unlocked Tanoby chamber did not use native clear-weather reset");
    b.state(after);
    check(startCampaignObjectRefresh(&b.core,clear),"Chamber lighting did not start through native field ownership");
    Bus other;other.map(2,26);other.loadScript();other.code(Load,{0xA4,11,0,0xA5,2});
    auto p=other.state(),q=p;q.flag(0x821,true);
    check(!planCampaignObjectRefresh(&other.core,p,q).ready,"Tanoby weather exception escaped its exact chamber scope");
    other.map(2,27);other.code(Load,{0xA4,3,0,0xA5,2});p=other.state();q=p;q.flag(0x821,true);
    check(!planCampaignObjectRefresh(&other.core,p,q).ready,"Rain/stat-changing weather escaped reviewed NONE/SHADE lighting");
    other.code(Load,{0xA4,11,0,2});
    check(!planCampaignObjectRefresh(&other.core,p,q).ready,"Incomplete chamber weather setup was silently replayed");
}

void pendingSeafoamLayout() {
    constexpr uint32_t OtherLayout=0x08001100,OtherTiles=0x08005000,Border=0x08006000,Grid=0x02020000;
    auto prepare=[&](Bus& b) {
        b.map(1,86);b.loadScript();b.code(Load,{0xA7,0x16,1,2});
        b.put(Save+0x32,159,2);b.put(0x0834EAB0+(159-1)*4,Layout);
        b.put(0x0834EAB0+(278-1)*4,OtherLayout);
        b.put(Layout+8,Border);b.put(Layout+24,0x202);
        b.put(OtherLayout,20);b.put(OtherLayout+4,20);b.put(OtherLayout+8,Border);
        b.put(OtherLayout+12,OtherTiles);b.put(OtherLayout+24,0x202);
        b.put(0x03006000,35);b.put(0x03006004,34);b.put(0x03006008,Grid);
        for(unsigned y=0;y<20;++y)for(unsigned x=0;x<20;++x) {
            b.put(OtherTiles+(y*20+x)*2,0x154,2);b.put(Grid+((y+7)*35+x+7)*2,0x154,2);
        }
        b.put(OtherTiles+(3*20+2)*2,0x4354,2); // Include native elevation bits.
    };
    Bus b;prepare(b);b.put(Grid+((3+7)*35+3+7)*2,0x755,2);
    auto before=b.state(),after=before;after.flag(0x821,true);
    const auto baseline=b.data;const auto plan=planCampaignObjectRefresh(&b.core,before,after);
    check(plan.ready && plan.changedLayout && plan.layoutEntries.size()==1 && plan.code.size()==9,
          "Pending own Seafoam layout did not plan exact grid plus native header/redraw");
    check(b.data==baseline,"Seafoam preflight mutated the live grid");
    b.state(after);check(startCampaignObjectRefresh(&b.core,plan),"Seafoam layout transaction did not start");
    check(inventoryRead16(&b.core,Grid+((3+7)*35+2+7)*2)==0x4354,"Native layout lost its elevation bits");
    check(inventoryRead16(&b.core,Grid+((3+7)*35+3+7)*2)==0x755,"Layout replaced an unchanged personal tile");

    Bus stale;prepare(stale);before=stale.state();after=before;after.flag(0x821,true);
    auto pending=planCampaignObjectRefresh(&stale.core,before,after);stale.state(after);
    stale.put(Grid+((3+7)*35+2+7)*2,0x222,2);const auto staleData=stale.data;
    check(!startCampaignObjectRefresh(&stale.core,pending)&&stale.data==staleData,"Stale Seafoam grid partially committed");
    const auto bad=planCampaignObjectRefresh(&stale.core,after,before);
    check(!bad.ready&&bad.layoutEntries.empty(),"Conflicting own Seafoam tile was overwritten");

    Bus underfoot;prepare(underfoot);underfoot.put(CampaignObjectsAddress+0x10,9,2);
    underfoot.put(CampaignObjectsAddress+0x14,9,2);underfoot.put(CampaignObjectsAddress+0x12,10,2);
    underfoot.put(CampaignObjectsAddress+0x16,10,2);before=underfoot.state();after=before;after.flag(0x821,true);
    const auto standing=planCampaignObjectRefresh(&underfoot.core,before,after);
    check(!standing.ready&&standing.reason.find("player leaves")!=std::string::npos&&standing.layoutEntries.empty(),
          "Seafoam elevation/current changed beneath the player");
    Bus puzzle;prepare(puzzle);before=puzzle.state();after=before;after.flag(0x2D2,true);
    check(!planCampaignObjectRefresh(&puzzle.core,before,after).ready,"Another player's Seafoam solution was copied");
    Bus tileset;prepare(tileset);tileset.put(OtherLayout+16,0x08009000);before=tileset.state();after=before;after.flag(0x821,true);
    check(!planCampaignObjectRefresh(&tileset.core,before,after).ready,"Different Seafoam graphics were applied without loading ownership");
    Bus occupied;prepare(occupied);occupied.active(1,1);occupied.put(CampaignObjectsAddress+0x24+9,86,1);
    occupied.put(CampaignObjectsAddress+0x24+10,1,1);occupied.put(CampaignObjectsAddress+0x24+0x10,9,2);
    occupied.put(CampaignObjectsAddress+0x24+0x12,10,2);before=occupied.state();after=before;after.flag(0x821,true);
    check(!planCampaignObjectRefresh(&occupied.core,before,after).ready,"Seafoam layout changed underneath an active object");
    Bus border;prepare(border);border.put(OtherLayout+8,Border+0x10);border.put(Border+0x10,1,1);
    before=border.state();after=before;after.flag(0x821,true);
    check(!planCampaignObjectRefresh(&border.core,before,after).ready,"Seafoam refresh changed original border geometry");
}

void retainedCableClub() {
    for(bool hidden:{false,true}) {
        Bus b;b.map(5,5);b.loadScript();
        b.code(Load,{0x26,0x0D,0x80,0x80,1,0x21,0x0D,0x80,0,0,0x06,1,0,0x32,0,8,0x2A,0x70,0,2});
        b.code(Child,{0x29,0x70,0,2});auto before=b.state();before.flag(0x70,hidden);b.state(before);
        auto after=before;after.flag(0x821,true);const auto data=b.data;
        const auto plan=planCampaignObjectRefresh(&b.core,before,after);
        check(plan.ready&&plan.code.empty()&&b.data==data,"Own Wonder Card visibility blocked or changed shared campaign receipt");
        after.flag(0x70,!hidden);
        check(!planCampaignObjectRefresh(&b.core,before,after).ready,"Other player's deliveryman receipt replaced a personal Wonder Card");
        after=before;after.var(0x406F,1);
        check(!planCampaignObjectRefresh(&b.core,before,after).ready,"Own loaded Cable Club state changed during campaign receipt");
        b.map(5,4);before=b.state();after=before;after.flag(0x821,true);
        check(!planCampaignObjectRefresh(&b.core,before,after).ready,"Wonder Card exception escaped reviewed Pokemon Center floors");
    }
}

void guards() {
    Bus b; const auto before = b.state(); auto after = before; after.flag(0x100,true);
    auto plan = planCampaignObjectRefresh(&b.core,before,after);
    check(plan.ready,"Idle fixture was not ready");
    b.put(FieldControlsLockAddress,1,1);
    check(!planCampaignObjectRefresh(&b.core,before,after).ready,"Menu owned field was mutated");
    b.put(FieldControlsLockAddress,0,1); b.put(GlobalScriptStatusAddress,1,1);
    check(!planCampaignObjectRefresh(&b.core,before,after).ready,"Waiting cutscene was mutated");
    b.put(GlobalScriptStatusAddress,2,1); b.put(CampaignAvatarAddress + 2,2,1);
    check(!planCampaignObjectRefresh(&b.core,before,after).ready,"Walking avatar was mutated");
    b.put(CampaignAvatarAddress + 2,0,1); b.put(CampaignObjectsAddress,0x10003);
    check(!planCampaignObjectRefresh(&b.core,before,after).ready,"Native single movement was mutated");
    b.put(CampaignObjectsAddress,0x10041);
    check(!campaignFieldQuiescent(&b.core),"Unfinished native held movement was treated as stationary");
    b.put(CampaignObjectsAddress,0x100C1);
    check(campaignFieldQuiescent(&b.core),"Finished native held movement kept the field permanently blocked");
    b.put(CampaignObjectsAddress,0x10001); b.state(after); b.put(CampaignMapHeaderAddress,Layout + 0x80);
    check(!startCampaignObjectRefresh(&b.core,plan),"A stale plan started on a changed map header");

    Bus loop; loop.loadScript(); loop.code(Load,{5,0,0x31,0,8});
    const auto initial = loop.state(); auto changed = initial; changed.flag(0x120,true);
    check(planCampaignObjectRefresh(&loop.core,initial,initial).ready,"No-change delta unnecessarily evaluated unrelated native map script");
    check(!planCampaignObjectRefresh(&loop.core,initial,changed).ready,"Infinite native map script was accepted");
    loop.code(Load,{6,1,0,0x32,0,8,2});
    check(!planCampaignObjectRefresh(&loop.core,initial,changed).ready,"Uninitialized comparison was accepted");
    loop.code(Load,{0x21,0,0x80,1,0,2});
    check(!planCampaignObjectRefresh(&loop.core,initial,changed).ready,"Uninitialized special variable was accepted");
    loop.code(Load,{0x16,0x60,0x50,2,0,2});
    check(!planCampaignObjectRefresh(&loop.core,initial,changed).ready,"Unsupported script variable side effect was accepted");
    loop.code(Load,{0x16,0,0x80,3,0,0x21,0,0x80,3,0,2});
    check(planCampaignObjectRefresh(&loop.core,initial,changed).ready,"Private evaluator temporary assignment failed");
    loop.put(Events,65,1);
    check(!planCampaignObjectRefresh(&loop.core,initial,changed).ready,"Oversized native template table was accepted");
    loop.put(Events,2,1); loop.put(Templates + 0x18,1,1);
    check(!planCampaignObjectRefresh(&loop.core,initial,changed).ready,"Duplicate native object IDs were accepted");
}
}
int main() {
    try { objects(); metatiles(); placements(); derivedSetup(); originalFlatLeaves(); retainedPersonalMaps(); chamberLighting();
        pendingSeafoamLayout(); retainedCableClub(); guards();
        std::cout << "FireRed campaign runtime: " << checks << " checks passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
