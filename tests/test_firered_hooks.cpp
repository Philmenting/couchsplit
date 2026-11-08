// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic CPU/memory fixtures for callback boundaries; no game files required.
#include "../gba/FireRedBattleHooks.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>

namespace {
using coop::firered::BattleHooks;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
unsigned forwarded = 0;
void previousBreakpoint(ARMCore*, int) { ++forwarded; }
struct Fixture;
std::unordered_map<mCore*,Fixture*> fixtures;
struct Fixture {
    mCore core{};
    ARMCore cpu{};
    std::unordered_map<uint32_t,uint8_t> memory;
    bool rejectOtherPatch = false;
    void put(uint32_t address, uint32_t value, unsigned size=4) {
        for(unsigned i=0;i<size;++i)memory[address+i]=uint8_t(value>>(i*8));
    }
    static uint32_t read8(mCore* c,uint32_t a) { auto& m=fixtures.at(c)->memory;auto i=m.find(a);return i==m.end()?0:i->second; }
    static uint32_t read16(mCore* c,uint32_t a) { return read8(c,a)|(read8(c,a+1)<<8); }
    static uint32_t read32(mCore* c,uint32_t a) { return read16(c,a)|(read16(c,a+2)<<16); }
    static uint32_t rawRead16(mCore* c,uint32_t a,int) { return read16(c,a); }
    static void rawWrite16(mCore* c,uint32_t a,int,uint16_t value) {
        auto* f=fixtures.at(c);if(f->rejectOtherPatch&&value==0xBEF2)return;f->put(a,value,2);
    }
    Fixture() {
        fixtures.emplace(&core,this);core.cpu=&cpu;core.busRead8=read8;core.busRead16=read16;core.busRead32=read32;
        core.busWrite8=[](mCore* c,uint32_t a,uint8_t value){fixtures.at(c)->put(a,value,1);};
        core.rawRead16=rawRead16;core.rawWrite16=rawWrite16;
        cpu.executionMode=MODE_THUMB;cpu.cpsr.t=1;cpu.irqh.bkpt16=previousBreakpoint;
        put(0x02022B4C,5);
        const std::array<std::pair<uint32_t,std::array<uint16_t,4>>,38> prologues{{
            {BattleHooks::WishCastAddress,{{0x6838,0x3006,0xE042,0x3F20}}},
            {BattleHooks::WishTextAddress,{{0x1C21,0xF000,0xFCD5,0xF000}}},
            {BattleHooks::ForcedCountAddress,{{0x2F01,0xD805,0x480F,0x6800}}},
            {BattleHooks::ForcedChoiceAddress,{{0x4813,0x7801,0x0048,0x1980}}},
            {BattleHooks::PartyMenuSelectionAddress,{{0x490E,0x7008,0x490E,0x2001}}},
            {BattleHooks::EvolutionDexAddress,{{0xB500,0x480A,0x6800,0x7EC0}}},
            {BattleHooks::ExperienceCommandAddress,{{0xB5F0,0x464F,0x4646,0xB4C0}}},
            {BattleHooks::EvolutionGateAddress,{{0x4806,0x7800,0x2800,0xD003}}},
            {BattleHooks::EvolutionQueueAddress,{{0xB530,0x4814,0x7801,0x1C03}}},
            {BattleHooks::EvolutionQueueEndAddress,{{0x4904,0x4804,0x6008,0xBC30}}},
            {BattleHooks::OpponentResetAddress,{{0xB5F0,0x4647,0xB480,0x0600}}},
            {BattleHooks::HasNoMonsAddress,{{0xB5F0,0x4657,0x464E,0x4645}}},
            {BattleHooks::IsOtherTrainerAddress,{{0xB530,0x1C0D,0x490F,0x680B}}},
            {BattleHooks::FlagGetAddress,{{0xB510,0x0400,0x0C04,0x1C20}}},
            {BattleHooks::CaptureFullAddress,{{0xB510,0x2400,0x2064,0x4360}}},
            {BattleHooks::BallThrowAddress,{{0x4A0B,0x490C,0x8828,0x0080}}},
            {BattleHooks::CheckTeamsLostAddress,{{0xB5F0,0x4647,0xB480,0x2600}}},
            {BattleHooks::GiveMonAddress,{{0xB570,0x1C06,0x4C09,0x6822}}},
            {BattleHooks::EvolutionAddress,{{0xB5F0,0x4657,0x464E,0x4645}}},
            {BattleHooks::IncrementGameStatAddress,{{0xB510,0x0600,0x0E04,0x2C33}}},
            {BattleHooks::TrainerMoneyTextAddress,{{0x490F,0x20FD,0x7008,0x2001}}},
            {BattleHooks::PayDayTextAddress,{{0x490F,0x20FD,0x7008,0x2001}}},
            {BattleHooks::PartyMenuOpenAddress,{{0xB570,0xB081,0x482A,0x21FF}}},
            {BattleHooks::PartyMenuReturnAddress,{{0xB500,0x4802,0xF7CF,0xFDB8}}},
            {BattleHooks::PartyMenuReturn2Address,{{0xB500,0x4802,0xF7CF,0xFDB0}}},
            {BattleHooks::PartyMenuReplyAddress,{{0xB530,0x1C14,0x0600,0x0E05}}},
            {BattleHooks::WildEscapeDoubleCheckAddress,{{0x2001,0x4003,0x2B00,0xD120}}},
            {BattleHooks::PlayerNameTextAddress,{{0x6804,0xE0CF,0x0000,0x4F5C}}},
            {BattleHooks::WildIntroSpriteAddress,{{0x2000,0xF7FA,0xFEDB,0x4649}}},
            {BattleHooks::WildIntroTextAddress,{{0x2001,0x4008,0x2800,0xD003}}},
            {BattleHooks::WildBallTargetAddress,{{0x7030,0x480C,0x6801,0x2080}}},
            {BattleHooks::TrainerIntroBattlerAddress,{{0x4C20,0x7820,0xF061,0xFFFD}}},
            {BattleHooks::TrainerIntroTextAddress,{{0x2001,0x4008,0x2800,0xD015}}},
            {BattleHooks::BattlerSpritePresentAddress,{{0xB530,0x0600,0x0E04,0x1C25}}},
            {BattleHooks::IsDoubleBattleAddress,{{0x4802,0x6800,0x2101,0x4008}}},
            {BattleHooks::ReshowLoadSpriteAddress,{{0xB510,0x0600,0x0E04,0x4810}}},
            {BattleHooks::ReshowCreateSpriteAddress,{{0xB5F0,0x4657,0x464E,0x4645}}},
            {BattleHooks::ReshowCreateHealthboxAddress,{{0xB570,0x0600,0x0E05,0x4808}}}}};
        for(const auto& p:prologues)for(unsigned i=0;i<4;++i)put(p.first+i*2,p.second[i],2);
    }
    ~Fixture(){fixtures.erase(&core);}
    void invoke(uint32_t address,unsigned immediate) {
        cpu.gprs[ARM_PC]=int32_t(address+4);cpu.prefetch[0]=0x1234;cpu.prefetch[1]=0x5678;
        cpu.irqh.bkpt16(&cpu,int(immediate));
    }
    void returned(bool result) {
        check(cpu.gprs[0]==int(result),"predicate return value");check(cpu.prefetch[0]==0x4770,"BX LR queued");
    }
};

BattleHooks reserveHooks(Fixture& f,BattleHooks::WishCast cast={},BattleHooks::WishName name={},
                         BattleHooks::ForcedSwitch forced={}) {
    return BattleHooks(&f.core,[](unsigned,unsigned,unsigned){return true;},
        {}, {}, {}, {}, {}, {}, {}, // OT through evolution callbacks.
        {}, {}, {}, {}, {}, {}, {}, // Statistics through single-wild callback.
        {}, {}, {}, {}, {}, {}, {}, // Single-trainer through own-menu selection.
        std::move(cast),std::move(name),std::move(forced));
}

void wishContext(Fixture& f,unsigned battler=2,unsigned slot=5) {
    f.put(0x02023D6B,battler,1);
    f.put(0x02023F40+battler,2,1);f.put(0x02023F44+battler,slot,1);
    f.put(0x02023BCE + battler*2,slot,2);
    f.cpu.gprs[9]=0x083FBE79;f.cpu.gprs[0]=0x02022AB8;f.cpu.gprs[4]=0x02021CD0;
    f.cpu.gprs[ARM_SP]=0x03007F00;
    for(const auto& [offset,value]:{std::pair<unsigned,unsigned>{0,0xFD},{1,4},{2,battler},{3,slot},{4,0xFF}})
        f.put(0x02022AB8+offset,value,1);
}

void reserveHookBoundaries() {
    using Name=coop::firered::WishJournal::Name;
    Name name;name.fill(0xFF);name[0]=0xCA;name[1]=0xCD;name[2]=0xC8; // Authored nickname "PSN".
    { Fixture f;unsigned casts=0,expectedBattler=0,expectedSlot=0;
      auto hook=reserveHooks(f,[&](unsigned battler,unsigned slot){
          check(battler==expectedBattler&&slot==expectedSlot,"Wish capture lost native caster identity");++casts;});
      for(unsigned battler:{0u,2u}) {
          expectedBattler=battler;expectedSlot=4+battler/2;wishContext(f,battler,expectedSlot);
          const auto before=f.memory;f.invoke(BattleHooks::WishCastAddress,0xD8);
          check(f.cpu.prefetch[0]==0x6838&&f.memory==before&&f.cpu.gprs[ARM_SP]==0x03007F00,
                "Wish success callback changed native state or skipped script advancement");
      }
      for(unsigned battler:{1u,3u}) {wishContext(f,battler);f.invoke(BattleHooks::WishCastAddress,0xD8);}
      check(casts==2&&f.cpu.prefetch[0]==0x6838,"Opponent Wish was captured as a human's Wish");
      for(unsigned flags:{0u,0x43u}) {f.put(0x02022B4C,flags);wishContext(f);f.invoke(BattleHooks::WishCastAddress,0xD8);}
      check(casts==2&&f.cpu.prefetch[0]==0x6838,"Unshared Wish invoked ownership callback");
      hook.check();hook.detach();check(Fixture::read16(&f.core,BattleHooks::WishCastAddress)==0x6838,"Wish capture patch not restored"); }
    for(bool mismatch:{false,true}) {
        Fixture f;unsigned casts=0;auto hook=reserveHooks(f,[&](unsigned,unsigned){++casts;});wishContext(f);
        if(mismatch)f.put(0x02023BD2,4,2);else f.put(0x02023F42,1,1);
        const auto before=f.memory;f.invoke(BattleHooks::WishCastAddress,0xD8);bool failed=false;
        try{hook.check();}catch(const std::runtime_error&){failed=true;}
        check(failed&&casts==0&&f.memory==before&&f.cpu.prefetch[0]==0x6838,"Invalid Wish success context was not contained");
    }
    { Fixture f;unsigned names=0;bool known=true;
      auto hook=reserveHooks(f,{},[&](unsigned battler,unsigned slot)->std::optional<Name>{
          ++names;check(battler==2&&slot==5,"Wish text used another source");return known?std::optional(name):std::nullopt;});
      wishContext(f);auto expected=f.memory;
      for(unsigned i=0;i<name.size();++i)expected[0x02021CD0+i]=name[i];
      f.invoke(BattleHooks::WishTextAddress,0xD7);
      check(names==1&&f.memory==expected&&f.cpu.prefetch[0]==0xE001&&f.cpu.gprs[ARM_SP]==0x03007F00,
            "Status-looking Wish nickname was not copied exactly through native gStringVar1 path");
      f.cpu.gprs[9]=0x083FBE78;f.put(0x02022AB8,0,1);const auto before=f.memory;
      f.invoke(BattleHooks::WishTextAddress,0xD7);
      check(names==1&&f.memory==before&&f.cpu.prefetch[0]==0x1C21,"Unrelated Buff1 text was intercepted");
      for(unsigned battler:{1u,3u}) {wishContext(f,battler);const auto enemy=f.memory;f.invoke(BattleHooks::WishTextAddress,0xD7);
          check(names==1&&f.memory==enemy&&f.cpu.prefetch[0]==0x1C21,"Enemy Wish text was rewritten");}
      wishContext(f);known=false;const auto missing=f.memory;f.invoke(BattleHooks::WishTextAddress,0xD7);
      check(names==2&&f.memory==missing&&f.cpu.prefetch[0]==0x1C21,"Unjournaled Wish did not retain native text expansion");
      f.put(0x02022B4C,0x43);f.invoke(BattleHooks::WishTextAddress,0xD7);
      check(names==2&&f.cpu.prefetch[0]==0x1C21,"Link Wish text was intercepted");
      hook.check();hook.detach();check(Fixture::read16(&f.core,BattleHooks::WishTextAddress)==0x1C21,"Wish text patch not restored"); }
    for(unsigned broken=0;broken<5;++broken) {
        Fixture f;unsigned names=0;auto hook=reserveHooks(f,{},[&](unsigned,unsigned)->std::optional<Name>{++names;return name;});
        wishContext(f);
        if(broken==0)f.cpu.gprs[0]=0x02022AC8;
        else if(broken==1)f.cpu.gprs[4]=0x02021CE0;
        else f.put(0x02022AB8+(broken==2?0:broken==3?1:4),0,1);
        const auto before=f.memory;f.invoke(BattleHooks::WishTextAddress,0xD7);bool failed=false;
        try{hook.check();}catch(const std::runtime_error&){failed=true;}
        check(failed&&names==0&&f.memory==before&&f.cpu.prefetch[0]==0x1C21,"Malformed Wish text context wrote nickname data");
    }
    { Fixture f;unsigned calls=0,expectedBattler=2;bool available=true;std::optional<unsigned> choice=4;
      auto hook=reserveHooks(f,{}, {},[&](unsigned battler,unsigned original)->std::optional<unsigned>{
          ++calls;check(battler==expectedBattler,"Forced switch used another target owner");
          if(original==6)return available?std::optional<unsigned>(0):std::nullopt;
          check(original==5,"Forced switch did not receive native modulo-six draw");return choice;});
      f.put(0x02022B4C,9);f.put(0x02023D6C,2,1);f.cpu.gprs[7]=2;f.cpu.gprs[5]=5;f.cpu.gprs[0]=0xFEDC;
      f.cpu.gprs[ARM_SP]=0x03007F00;const auto before=f.memory;
      f.invoke(BattleHooks::ForcedCountAddress,0xD6);
      check(calls==1&&f.cpu.gprs[7]==3&&f.cpu.prefetch[0]==0x2F01,"Owned reserve count did not enter native level check");
      available=false;f.invoke(BattleHooks::ForcedCountAddress,0xD6);
      check(calls==2&&f.cpu.gprs[7]==0&&f.cpu.prefetch[0]==0x2F01,"No own reserve did not take native failure gate");
      for(unsigned battler:{0u,2u}) {
          expectedBattler=battler;f.put(0x02023D6C,battler,1);f.cpu.gprs[5]=5;
          f.invoke(BattleHooks::ForcedChoiceAddress,0xD5);
          check(f.cpu.gprs[5]==4&&f.cpu.prefetch[0]==0xE04F,"Forced choice did not enter native queue/order commit");
      }
      choice.reset();f.cpu.gprs[5]=5;f.invoke(BattleHooks::ForcedChoiceAddress,0xD5);
      check(calls==5&&f.cpu.gprs[5]==5&&f.cpu.prefetch[0]==0xE7F5&&f.cpu.gprs[0]==0xFEDC&&
            f.cpu.gprs[ARM_SP]==0x03007F00&&f.memory==before,"Rejected forced draw changed RNG input, native memory or stack");
      for(const auto& [flags,battler]:{std::pair<unsigned,unsigned>{5,2},{9,1},{9,3},{0,2},{0x43,2}}) {
          f.put(0x02022B4C,flags);f.put(0x02023D6C,battler,1);f.cpu.gprs[7]=2;f.cpu.gprs[5]=5;
          f.invoke(BattleHooks::ForcedCountAddress,0xD6);
          check(calls==5&&f.cpu.gprs[7]==2&&f.cpu.prefetch[0]==0x2F01,"Wild, enemy or unshared force-out count changed");
          f.invoke(BattleHooks::ForcedChoiceAddress,0xD5);
          check(calls==5&&f.cpu.gprs[5]==5&&f.cpu.prefetch[0]==0x4813,"Wild, enemy or unshared force-out choice changed");
      }
      hook.check();hook.detach();check(Fixture::read16(&f.core,BattleHooks::ForcedCountAddress)==0x2F01&&
          Fixture::read16(&f.core,BattleHooks::ForcedChoiceAddress)==0x4813,"Forced switch patches not restored"); }
    for(unsigned invalid:{6u,7u,255u}) {
        Fixture f;auto hook=reserveHooks(f,{}, {},[&](unsigned,unsigned)->std::optional<unsigned>{return invalid;});
        f.put(0x02022B4C,9);f.put(0x02023D6C,2,1);f.cpu.gprs[5]=5;const auto before=f.memory;
        f.invoke(BattleHooks::ForcedChoiceAddress,0xD5);bool failed=false;
        try{hook.check();}catch(const std::runtime_error&){failed=true;}
        check(failed&&f.cpu.gprs[5]==5&&f.cpu.prefetch[0]==0x4813&&f.memory==before,"Invalid forced native slot escaped into queue commit");
    }
    for(uint32_t address:{BattleHooks::WishCastAddress,BattleHooks::WishTextAddress,
                          BattleHooks::ForcedCountAddress,BattleHooks::ForcedChoiceAddress}) {
        Fixture f;f.put(address+2,0,2);const auto before=f.memory;bool failed=false;
        try{auto hook=reserveHooks(f,[](unsigned,unsigned){},[](unsigned,unsigned)->std::optional<Name>{return std::nullopt;},
            [](unsigned,unsigned)->std::optional<unsigned>{return std::nullopt;});}
        catch(const std::runtime_error&){failed=true;}
        check(failed&&f.memory==before&&f.cpu.irqh.bkpt16==previousBreakpoint,"Unexpected reserve preimage installed a partial hook");
    }
}
}

int main() {
    try {
        reserveHookBoundaries();
        auto empty=[](unsigned,unsigned,unsigned){return true;};
        { Fixture f;unsigned calls=0;bool allow=false;
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
            [&](unsigned slot){check(slot==5,"selection guard lost own original index");++calls;return allow;});
          f.put(0x02022B4C,0); // Own-party menus temporarily use single layout.
          f.cpu.gprs[0]=5;f.cpu.gprs[ARM_SP]=0x03007F00;
          f.invoke(BattleHooks::PartyMenuSelectionAddress,0xD9);
          check(calls==1&&f.cpu.prefetch[0]==0xE048&&f.cpu.gprs[ARM_SP]==0x03007F00,"rejection did not use existing native epilogue");
          allow=true;f.invoke(BattleHooks::PartyMenuSelectionAddress,0xD9);
          check(calls==2&&f.cpu.prefetch[0]==0x490E,"accepted selection did not execute original store path");
          hook.check();hook.detach();check(Fixture::read16(&f.core,BattleHooks::PartyMenuSelectionAddress)==0x490E,"selection instruction not restored"); }
        { Fixture f;unsigned calls=0;
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
            [&](unsigned slot)->std::optional<bool>{check(slot==5,"evolution dex checked a wrong party slot");++calls;return true;});
          f.cpu.gprs[ARM_LR]=0x080CEAA9;f.cpu.gprs[9]=0x02024284+500;
          f.invoke(BattleHooks::EvolutionDexAddress,0xDA);f.returned(true);check(calls==1,"owned evolution dex not checked");
          f.cpu.gprs[ARM_LR]=0x080CF70D;f.invoke(BattleHooks::EvolutionDexAddress,0xDA);
          check(calls==1&&f.cpu.prefetch[0]==0xB500,"trade evolution changed");hook.check(); }
        { Fixture f;unsigned awards=0,gates=0,queues=0,resets=0;
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
            [&]{++awards;return true;},[&]{++gates;},[&](bool ending){++queues;return ending;},
            [&](unsigned battler){check(battler==3,"reset received wrong native battler");++resets;});
          f.invoke(BattleHooks::ExperienceCommandAddress,0xDF);check(awards==1&&f.cpu.prefetch[0]==0x4770,"award loop did not return at native entry");
          f.invoke(BattleHooks::EvolutionGateAddress,0xDE);check(gates==1&&f.cpu.prefetch[0]==0x4806,"evolution gate skipped native cleanup");
          f.invoke(BattleHooks::EvolutionQueueAddress,0xDD);check(queues==1&&f.cpu.prefetch[0]==0xB530,"evolution queue skipped native stack setup");
          f.cpu.gprs[3]=0x03004F24;f.invoke(BattleHooks::EvolutionQueueEndAddress,0xDC);
          check(queues==2&&f.cpu.prefetch[0]==0xE7C9,"next owner did not repeat the existing native loop");
          f.cpu.gprs[0]=3;f.invoke(BattleHooks::OpponentResetAddress,0xDB);check(resets==1&&f.cpu.prefetch[0]==0xB5F0,"opponent reset did not continue native code");
          f.put(0x02022B4C,0x43);f.invoke(BattleHooks::ExperienceCommandAddress,0xDF);
          check(awards==1&&f.cpu.prefetch[0]==0xB5F0,"link battle was virtualized");hook.check();
          f.put(0x02022B4C,5);f.cpu.gprs[3]=0;f.invoke(BattleHooks::EvolutionQueueEndAddress,0xDC);
          bool failed=false;try{hook.check();}catch(const std::runtime_error&){failed=true;}
          check(failed&&f.cpu.prefetch[0]==0x4904,"invalid evolution stack context was not contained"); }
        { Fixture f;bool single=true;
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, [&]{return single;});
          constexpr uint32_t ghost=0x02023BE4+3*88;
          f.put(0x02023BC4,3,1);f.put(0x02023BD0,0,2);f.put(0x02023BD4,0,2);
          f.put(ghost+0x20,42,1);f.put(ghost+0x28,100,2);f.put(ghost+0x2E,99,2);
          f.invoke(BattleHooks::WildIntroSpriteAddress,0xE9);
          check(f.cpu.prefetch[0]==0xE005&&Fixture::read8(&f.core,0x02023D70)==8 &&
                Fixture::read16(&f.core,ghost+0x28)==0&&Fixture::read8(&f.core,ghost+0x20)==0&&
                Fixture::read16(&f.core,ghost+0x2E)==0,"phantom wild battler was not absent and inert");
          f.cpu.gprs[1]=5;f.invoke(BattleHooks::WildIntroTextAddress,0xE8);
          check(f.cpu.gprs[1]==4&&f.cpu.prefetch[0]==0x2001&&Fixture::read32(&f.core,0x02022B4C)==5,
                "single-wild text changed global battle flags");
          f.put(0x02023D6B,2,1);f.cpu.gprs[0]=3;f.cpu.gprs[6]=0x02023D6C;
          f.invoke(BattleHooks::WildBallTargetAddress,0xE7);
          check(f.cpu.gprs[0]==1&&f.cpu.prefetch[0]==0x7030,"guest ball did not target the real wild opponent");
          for(const auto& [address,immediate,result] : {
                std::tuple<uint32_t,unsigned,unsigned>{BattleHooks::ReshowLoadSpriteAddress,0xE2,1},
                {BattleHooks::ReshowCreateSpriteAddress,0xE1,0},
                {BattleHooks::ReshowCreateHealthboxAddress,0xE0,0},
                {BattleHooks::BattlerSpritePresentAddress,0xE4,0}}) {
              f.cpu.gprs[0]=3;f.invoke(address,immediate);
              check(f.cpu.gprs[0]==int(result)&&f.cpu.prefetch[0]==0x4770,
                    "reshow recreated the absent wild opponent");
              f.cpu.gprs[0]=1;f.invoke(address,immediate);
              check(f.cpu.prefetch[0]!=0x4770,"reshow skipped the real opponent");
          }
          f.cpu.gprs[ARM_LR]=0x0807794B;f.invoke(BattleHooks::IsDoubleBattleAddress,0xE3);
          check(f.cpu.gprs[0]==0&&f.cpu.prefetch[0]==0x4770,"reshow included an absent enemy shadow");
          single=false;f.cpu.gprs[0]=3;f.invoke(BattleHooks::WildBallTargetAddress,0xE7);
          check(f.cpu.gprs[0]==3&&f.cpu.prefetch[0]==0x7030,"genuine double wild target was changed");
          single=true;f.put(0x02022B4C,9);f.cpu.gprs[0]=3;f.invoke(BattleHooks::WildBallTargetAddress,0xE7);
          check(f.cpu.gprs[0]==3&&f.cpu.prefetch[0]==0x7030,"trainer target was changed");
          hook.check();hook.detach();
          check(Fixture::read16(&f.core,BattleHooks::WildIntroSpriteAddress)==0x2000&&
                Fixture::read16(&f.core,BattleHooks::WildIntroTextAddress)==0x2001&&
                Fixture::read16(&f.core,BattleHooks::WildBallTargetAddress)==0x7030,"single-wild patches were not restored"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, []{return true;});
          f.put(0x02023D70,8,1);f.put(0x02023D6B,2,1);f.cpu.gprs[0]=3;f.cpu.gprs[6]=0;
          f.invoke(BattleHooks::WildBallTargetAddress,0xE7);bool failed=false;
          try{hook.check();}catch(const std::runtime_error&){failed=true;}
          check(failed&&f.cpu.prefetch[0]==0x7030,"invalid single-wild target did not fail closed"); }
        { Fixture f;f.put(0x02022B4C,9);f.put(0x02023BC4,3,1);
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, []{return true;});
          f.put(0x02023BD0,0,2);f.put(0x02023BD4,0,2);
          f.invoke(BattleHooks::TrainerIntroBattlerAddress,0xE6);
          check(Fixture::read8(&f.core,0x02023D70)==8&&f.cpu.prefetch[0]==0x4C20,"single trainer phantom remained active");
          f.cpu.gprs[1]=9;f.invoke(BattleHooks::TrainerIntroTextAddress,0xE5);
          check(f.cpu.gprs[1]==8&&f.cpu.prefetch[0]==0x2001,"single trainer used two-opponent intro text");
          f.put(0x02023BC4,1,1);
          for(uint32_t lr : {0x08038CEDu,0x08035B39u,0x08035B43u,0x08035D9Bu,0x0807794Bu}) {
              f.cpu.gprs[ARM_LR]=int32_t(lr);f.cpu.gprs[0]=1;f.invoke(BattleHooks::IsDoubleBattleAddress,0xE3);
              check(f.cpu.gprs[0]==0&&f.cpu.prefetch[0]==0x4770,"single trainer intro spawned two opponents");
          }
          f.cpu.gprs[ARM_LR]=0x08001001;f.cpu.gprs[0]=1;f.invoke(BattleHooks::IsDoubleBattleAddress,0xE3);
          check(f.cpu.prefetch[0]==0x4802,"unrelated double-battle query was changed");
          hook.check(); }
        { Fixture f;unsigned names=0;
          const std::array<uint8_t,7> guest{{0xBB,0xBC,0xFF,0xFF,0xFF,0xFF,0xFF}};
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
            [&](unsigned battler)->std::optional<std::array<uint8_t,7>> {
              ++names;return battler==2?std::optional(guest):std::nullopt;
            });
          f.put(0x02023D68,1,2);f.put(0x02023D6B,2,1);
          f.cpu.gprs[9]=0x083FD4D9;f.cpu.gprs[0]=0x03004F5C;f.cpu.gprs[ARM_SP]=0x03007000;
          f.invoke(BattleHooks::PlayerNameTextAddress,0xEA);
          check(names==1&&f.cpu.gprs[0]==0x03007000&&f.cpu.prefetch[0]==0x4604,
                "guest ball text did not use local scratch name");
          for(unsigned i=0;i<guest.size();++i)check(Fixture::read8(&f.core,0x03007000+i)==guest[i],"guest text name differs");
          check(Fixture::read8(&f.core,0x03007007)==0xFF,"guest text is not terminated");
          for(const auto& [source,battler,item] : {std::tuple<uint32_t,unsigned,unsigned>{0x083FD4D8,2,1},
                                                   {0x083FD4D9,1,1}, {0x083FD4D9,2,13},
                                                   {0x083FD4D9,0,1}}) {
            f.cpu.gprs[9]=int32_t(source);f.cpu.gprs[0]=0x03004F5C;
            f.put(0x02023D6B,battler,1);f.put(0x02023D68,item,2);
            f.invoke(BattleHooks::PlayerNameTextAddress,0xEA);
            check(f.cpu.prefetch[0]==0x6804&&f.cpu.gprs[0]==0x03004F5C,
                  "unrelated text, opponent, non-ball item or leader text changed");
          }
          hook.check();hook.detach();
          check(Fixture::read16(&f.core,BattleHooks::PlayerNameTextAddress)==0x6804,"text hook was not restored"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
            [](unsigned)->std::optional<std::array<uint8_t,7>>{return std::array<uint8_t,7>{};});
          f.put(0x02023D68,1,2);f.put(0x02023D6B,2,1);
          f.cpu.gprs[9]=0x083FD4D9;f.cpu.gprs[0]=0x03004F5C;f.cpu.gprs[ARM_SP]=0;
          f.invoke(BattleHooks::PlayerNameTextAddress,0xEA);
          bool failed=false;try{hook.check();}catch(const std::runtime_error&){failed=true;}
          check(failed&&f.cpu.prefetch[0]==0x6804,"invalid text scratch context did not roll back"); }
        { Fixture f;unsigned opens=0,returns=0,replies=0;
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {},
            [&](unsigned battler){++opens;check(battler==2,"native party-menu battler context");},
            [&]{++returns;},[&](unsigned selected)->std::optional<unsigned>{++replies;check(selected==5,"virtual party-menu selected original");return 4;});
          f.put(0x02023BC4,2,1);f.invoke(BattleHooks::PartyMenuOpenAddress,0xEC);
          check(opens==1&&f.cpu.prefetch[0]==0xB570,"party-menu entry hook changed native prologue");
          f.put(0x02022B4C,0); // The virtual own menu temporarily uses single layout.
          f.invoke(BattleHooks::PartyMenuReturnAddress,0xED);f.invoke(BattleHooks::PartyMenuReturn2Address,0xEE);
          check(returns==2&&f.cpu.prefetch[0]==0xB500,"party-menu restore was skipped without double-battle flags");
          f.cpu.gprs[1]=5;f.cpu.gprs[2]=0;f.invoke(BattleHooks::PartyMenuReplyAddress,0xEF);
          check(replies==1&&f.cpu.gprs[1]==4&&f.cpu.gprs[2]==int32_t(coop::firered::NativePartyMenuOrderAddress)&&f.cpu.prefetch[0]==0xB530,"native reply slot/order normalization failed");
          hook.check();hook.detach();
          check(Fixture::read16(&f.core,BattleHooks::PartyMenuOpenAddress)==0xB570&&Fixture::read16(&f.core,BattleHooks::PartyMenuReturnAddress)==0xB500&&Fixture::read16(&f.core,BattleHooks::PartyMenuReplyAddress)==0xB530,"menu patches were not detached"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, {}, {},
            [](unsigned){throw std::runtime_error("menu fixture failure");},[]{},[](unsigned)->std::optional<unsigned>{return std::nullopt;});
          f.invoke(BattleHooks::PartyMenuOpenAddress,0xEC);check(f.cpu.prefetch[0]==0xB570,"party-menu exception crossed native C stack");
          bool failed=false;try{hook.check();}catch(const std::runtime_error&){failed=true;}check(failed,"party-menu failure not propagated to coordinator"); }

        bool threw=false;try{BattleHooks bad(nullptr,empty);}catch(const std::runtime_error&){threw=true;}
        check(threw,"null core rejected");
        { Fixture f;BattleHooks hook(&f.core,empty);
          for(unsigned battler : {0u,2u}) {
              f.put(0x02022B4C,5);f.cpu.gprs[3]=5;f.cpu.gprs[4]=int32_t(battler);
              f.invoke(BattleHooks::WildEscapeDoubleCheckAddress,0xEB);
              check(f.cpu.gprs[3]==4&&f.cpu.prefetch[0]==0x2001&&Fixture::read32(&f.core,0x02022B4C)==5,
                    "wild escape did not preserve native flags and resume the speed roll");
          }
          for(const auto [flags,battler] : {std::pair<unsigned,unsigned>{9,0}, {5,1}, {0x43,0}, {0,0}}) {
              f.put(0x02022B4C,flags);f.cpu.gprs[3]=int32_t(flags);f.cpu.gprs[4]=int32_t(battler);
              f.invoke(BattleHooks::WildEscapeDoubleCheckAddress,0xEB);
              check(f.cpu.gprs[3]==int32_t(flags)&&f.cpu.prefetch[0]==0x2001,
                    "trainer, opposing, link or non-double escape was modified");
          }
          hook.check();hook.detach();
          check(Fixture::read16(&f.core,BattleHooks::WildEscapeDoubleCheckAddress)==0x2001,
                "wild escape hook was not detached"); }
        { Fixture f;BattleHooks hook(&f.core,empty);f.cpu.gprs[3]=7;f.cpu.gprs[4]=0;
          f.invoke(BattleHooks::WildEscapeDoubleCheckAddress,0xEB);threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}
          check(threw&&f.cpu.prefetch[0]==0x2001,"invalid escape context was not contained"); }
        { Fixture f;f.put(BattleHooks::HasNoMonsAddress,0,2);threw=false;
          try{BattleHooks bad(&f.core,empty);}catch(const std::runtime_error&){threw=true;}
          check(threw&&f.cpu.irqh.bkpt16==previousBreakpoint,"preimage failure preserves IRQ"); }
        for(unsigned slot=0;slot<2;++slot) {
            Fixture f;f.cpu.gprs[ARM_PC]=int32_t(BattleHooks::HasNoMonsAddress+2-slot*2);f.cpu.prefetch[slot]=0xB5F0;
            { BattleHooks hook(&f.core,empty);check(hook.installed()&&f.cpu.prefetch[slot]==0xBEF1,"prefetched hook installed");
              threw=false;try{BattleHooks duplicate(&f.core,empty);}catch(const std::runtime_error&){threw=true;}
              check(threw&&hook.installed(),"duplicate hook rejected");hook.detach();hook.detach();
              check(!hook.installed()&&f.cpu.prefetch[slot]==0xB5F0,"prefetched instruction restored"); }
            check(f.cpu.irqh.bkpt16==previousBreakpoint,"IRQ restored");
        }
        { Fixture f;unsigned calls=0;BattleHooks hook(&f.core,[&](unsigned b,unsigned a,unsigned c){++calls;check(b==2&&a==6&&c==4,"native switch arguments");return false;});
          f.cpu.gprs[0]=2;f.cpu.gprs[1]=6;f.cpu.gprs[2]=4;f.invoke(BattleHooks::HasNoMonsAddress,0xF1);f.returned(false);
          f.cpu.gprs[0]=1;f.invoke(BattleHooks::HasNoMonsAddress,0xF1);check(f.cpu.prefetch[0]==0xB5F0&&calls==1,"enemy uses native prologue");
          f.put(0x02022B4C,0x43);f.cpu.gprs[0]=2;f.invoke(BattleHooks::HasNoMonsAddress,0xF1);check(f.cpu.prefetch[0]==0xB5F0&&calls==1,"link context stays native");
          unsigned before=forwarded;f.invoke(0x08001000,0x12);check(forwarded==before+1,"unrelated breakpoint forwarded"); }
        { Fixture f;unsigned calls=0;BattleHooks hook(&f.core,empty,{}, {}, {}, {}, [&]{++calls;});
          f.invoke(BattleHooks::CheckTeamsLostAddress,0xF6);
          check(calls==1&&f.cpu.prefetch[0]==0xB5F0,"team health observer continues native prologue");
          f.put(0x02023BC8,1);f.invoke(BattleHooks::CheckTeamsLostAddress,0xF6);
          check(calls==1&&f.cpu.prefetch[0]==0xB5F0,"pending controller blocks team health observer");
          f.put(0x02023BC8,0);f.put(0x02022B4C,0x43);f.invoke(BattleHooks::CheckTeamsLostAddress,0xF6);
          check(calls==1,"link battle cannot run team health observer"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {}, {}, {}, []{throw std::runtime_error("health observer failure");});
          f.invoke(BattleHooks::CheckTeamsLostAddress,0xF6);threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}
          check(threw&&f.cpu.prefetch[0]==0xB5F0,"team health observer failure is contained"); }
        { Fixture f;unsigned calls=0;
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, [&](unsigned slot){++calls;check(slot==2,"evolution party slot");});
          f.put(0x02023E8A,1,1);f.cpu.gprs[ARM_LR]=0x08015A77;f.cpu.gprs[0]=0x02024284+200;f.cpu.gprs[2]=0x81;f.cpu.gprs[3]=2;
          f.invoke(BattleHooks::EvolutionAddress,0xF8);check(calls==1&&f.cpu.prefetch[0]==0xB5F0,"owned evolution continues original prologue");
          f.cpu.gprs[ARM_LR]=0x08000001;f.invoke(BattleHooks::EvolutionAddress,0xF8);check(calls==1,"field evolution stays native");
          f.cpu.gprs[ARM_LR]=0x08015A77;f.put(0x02023E8A,2,1);f.invoke(BattleHooks::EvolutionAddress,0xF8);check(calls==1,"lost battle cannot own an evolution");
          f.put(0x02023E8A,1,1);f.put(0x02022B4C,0x43);f.invoke(BattleHooks::EvolutionAddress,0xF8);check(calls==1,"link evolution stays native"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, [](unsigned){});
          f.put(0x02023E8A,1,1);f.cpu.gprs[ARM_LR]=0x08015A77;f.cpu.gprs[0]=0x02024284;f.cpu.gprs[2]=0x81;f.cpu.gprs[3]=2;
          f.invoke(BattleHooks::EvolutionAddress,0xF8);threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}
          check(threw&&f.cpu.prefetch[0]==0xB5F0,"invalid evolution pointer contained"); }
        { Fixture f;unsigned calls=0;bool handled=true;
          BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, [&](unsigned statistic){++calls;check(statistic==14||statistic==26||statistic==27,"unsupported action statistic");return handled;});
          f.cpu.gprs[ARM_LR]=0x080CEF45;f.cpu.gprs[0]=14;f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);
          check(calls==1&&f.cpu.prefetch[0]==0x4770&&f.cpu.gprs[0]==14,"handled statistic did not bypass native increment");
          handled=false;f.cpu.gprs[ARM_LR]=0x08025B13;f.cpu.gprs[0]=26;f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);
          check(calls==2&&f.cpu.prefetch[0]==0xB510,"unhandled action statistic did not use native prologue");
          handled=true;f.cpu.gprs[0]=27;f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);check(calls==3&&f.cpu.prefetch[0]==0x4770,"Struggle counter was not intercepted");
          f.cpu.gprs[0]=11;f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);check(calls==3&&f.cpu.prefetch[0]==0xB510,"capture statistic must remain native");
          f.cpu.gprs[0]=14;f.cpu.gprs[ARM_LR]=0x080CFABB;f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);check(calls==3&&f.cpu.prefetch[0]==0xB510,"trade evolution must remain native");
          f.cpu.gprs[0]=27;f.cpu.gprs[ARM_LR]=0x08000001;f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);check(calls==3,"unknown action context invoked personal handler");
          f.cpu.gprs[ARM_LR]=0x08025B13;f.put(0x02022B4C,0x43);f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);check(calls==3,"link action invoked personal handler"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {}, {}, {}, {}, {}, {}, [](unsigned)->bool{throw std::runtime_error("stat handler failure");});
          f.cpu.gprs[ARM_LR]=0x080CEF45;f.cpu.gprs[0]=14;f.invoke(BattleHooks::IncrementGameStatAddress,0xF9);threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}
          check(threw&&f.cpu.prefetch[0]==0x4770,"failed statistic handler was not contained"); }
        { Fixture a,b;BattleHooks first(&a.core,empty),second(&b.core,[](unsigned,unsigned,unsigned){return false;});
          a.cpu.gprs[0]=0;b.cpu.gprs[0]=0;a.invoke(BattleHooks::HasNoMonsAddress,0xF1);b.invoke(BattleHooks::HasNoMonsAddress,0xF1);
          a.returned(true);b.returned(false); }
        { Fixture f;BattleHooks hook(&f.core,[](unsigned,unsigned,unsigned)->bool{throw std::runtime_error("authored test failure");});
          f.cpu.gprs[0]=0;f.invoke(BattleHooks::HasNoMonsAddress,0xF1);f.returned(true);threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}check(threw,"predicate exception contained and reported"); }
        { Fixture f;unsigned otCalls=0,badgeCalls=0;
          BattleHooks hook(&f.core,empty,[&](unsigned slot,uint32_t ot,uint32_t name){++otCalls;check(name==0x03007F00,"native OT name pointer");return ot!=(slot==2?0x1234u:0x5678u);},
              [&](unsigned b,unsigned flag){++badgeCalls;check((b==0||b==2)&&flag>=0x820&&flag<=0x827,"badge callback arguments");return b==2;});
          check(Fixture::read16(&f.core,BattleHooks::IsOtherTrainerAddress)==0xBEF2,"OT hook installed");
          f.cpu.gprs[ARM_LR]=0x0804417B;f.cpu.gprs[4]=0x02024284+200;f.cpu.gprs[0]=0x1234;f.cpu.gprs[1]=0x03007F00;
          f.invoke(BattleHooks::IsOtherTrainerAddress,0xF2);f.returned(false);
          f.cpu.gprs[0]=0x7777;f.invoke(BattleHooks::IsOtherTrainerAddress,0xF2);f.returned(true); // genuinely foreign OT stays foreign
          f.put(0x02023D6B,2,1);f.put(0x02023BD2,3,2);f.cpu.gprs[ARM_LR]=0x0801D405;f.cpu.gprs[0]=0x5678;
          f.invoke(BattleHooks::IsOtherTrainerAddress,0xF2);f.returned(false);
          f.cpu.gprs[ARM_LR]=0x08001001;f.invoke(BattleHooks::IsOtherTrainerAddress,0xF2);
          check(f.cpu.prefetch[0]==0xB530&&otCalls==3,"unknown OT callers stay native");
          const std::array<std::pair<uint32_t,unsigned>,10> badgeSites{{
              {0x0801D411,0x827},{0x0801D41F,0x821},{0x0801D42D,0x823},{0x0801D43B,0x825},
              {0x0803ED25,0x820},{0x0803ED8D,0x826},{0x0803ED57,0x824},{0x0803EDC5,0x826},
              {0x08014D9F,0x822},{0x08014E87,0x822}}};
          f.cpu.gprs[ARM_SP]=0x03007E00;f.put(0x03007E10,2);f.cpu.gprs[4]=2;f.cpu.gprs[9]=2;f.cpu.gprs[10]=2;
          for(auto site:badgeSites){f.cpu.gprs[ARM_LR]=int32_t(site.first);f.cpu.gprs[0]=int32_t(site.second);f.invoke(BattleHooks::FlagGetAddress,0xF3);f.returned(true);}
          check(badgeCalls==10,"all native badge contexts routed");
          f.cpu.gprs[4]=1;f.cpu.gprs[ARM_LR]=0x0803ED57;f.cpu.gprs[0]=0x824;f.invoke(BattleHooks::FlagGetAddress,0xF3);
          check(f.cpu.prefetch[0]==0xB510&&badgeCalls==10,"opponent badge query stays native");
          f.cpu.gprs[ARM_LR]=0x08001001;f.invoke(BattleHooks::FlagGetAddress,0xF3);check(f.cpu.prefetch[0]==0xB510,"nonbattle badge query stays native");
          hook.check();hook.detach();check(Fixture::read16(&f.core,BattleHooks::IsOtherTrainerAddress)==0xB530&&Fixture::read16(&f.core,BattleHooks::FlagGetAddress)==0xB510,"all optional instructions restored"); }
        { Fixture f;f.rejectOtherPatch=true;threw=false;
          try{BattleHooks bad(&f.core,empty,[](unsigned,uint32_t,uint32_t){return false;});}catch(const std::runtime_error&){threw=true;}
          check(threw&&Fixture::read16(&f.core,BattleHooks::HasNoMonsAddress)==0xB5F0&&f.cpu.irqh.bkpt16==previousBreakpoint,"partial installation rolled back");
          f.rejectOtherPatch=false;BattleHooks retry(&f.core,empty);check(retry.installed(),"failed installation removed registry entry"); }
        { Fixture f;unsigned capacityCalls=0,throwCalls=0;
          BattleHooks hook(&f.core,empty,{}, {},[&]{++capacityCalls;return true;},
              [&](unsigned battler,unsigned item){++throwCalls;check(battler==2&&item==1,"executed ball arguments");});
          f.cpu.gprs[ARM_LR]=0x080A1F13;f.invoke(BattleHooks::CaptureFullAddress,0xF4);f.returned(true);
          f.cpu.gprs[ARM_LR]=0x080145A9;f.invoke(BattleHooks::CaptureFullAddress,0xF4);
          check(f.cpu.prefetch[0]==0xB510&&capacityCalls==1,"Safari capacity remains native");
          f.put(0x02023D6B,2,1);f.put(0x02023D68,1,2);f.invoke(BattleHooks::BallThrowAddress,0xF5);
          check(throwCalls==1&&f.cpu.prefetch[0]==0x4A0B,"ball observer preserves native midfunction instruction");
          hook.check();hook.detach();check(Fixture::read16(&f.core,BattleHooks::BallThrowAddress)==0x4A0B,"ball patch restored"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {},{},[](unsigned,unsigned){throw std::runtime_error("ball fixture failure");});
          f.put(0x02023D6B,0,1);f.put(0x02023D68,1,2);f.invoke(BattleHooks::BallThrowAddress,0xF5);
          check(f.cpu.prefetch[0]==0x4A0B,"throw failure preserves native stack");threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}check(threw,"ball observer failure contained"); }
        for(unsigned flags:{0xA001u,0x20001u,0x42001u}) {
          Fixture f;unsigned throws=0;f.put(0x02022B4C,flags);
          BattleHooks hook(&f.core,empty,{}, {},{},[&](unsigned owner,unsigned item){++throws;check(owner==2&&item==1,"scripted throw ownership");});
          f.put(0x02023D6B,2,1);f.put(0x02023D68,1,2);f.invoke(BattleHooks::BallThrowAddress,0xF5);hook.check();
          check(throws==1&&f.cpu.prefetch[0]==0x4A0B,"scripted wild flag bypassed ownership hooks");
        }
        { Fixture f;unsigned gifts=0;
          BattleHooks hook(&f.core,empty,{}, {},{}, {},{},[&](uint32_t pointer){++gifts;check(pointer==0x0202402C,"native gift pointer");});
          f.cpu.gprs[ARM_LR]=0x0802D7C1;f.cpu.gprs[0]=0x0202402C;f.invoke(BattleHooks::GiveMonAddress,0xF7);
          check(gifts==1&&f.cpu.prefetch[0]==0xB570,"gift observer preserves native prologue");
          f.cpu.gprs[ARM_LR]=0x08001001;f.invoke(BattleHooks::GiveMonAddress,0xF7);
          check(gifts==1&&f.cpu.prefetch[0]==0xB570,"field gifts stay native");hook.check();
          hook.detach();check(Fixture::read16(&f.core,BattleHooks::GiveMonAddress)==0xB570,"gift hook restored"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{}, {},{}, {},{},[](uint32_t){throw std::runtime_error("gift fixture failure");});
          f.cpu.gprs[ARM_LR]=0x0802D7C1;f.invoke(BattleHooks::GiveMonAddress,0xF7);
          check(f.cpu.prefetch[0]==0xB570,"gift failure preserves native prologue");threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}check(threw,"gift failure contained"); }
        { Fixture f;unsigned calls=0;
          BattleHooks hook(&f.core,empty,{},{},{},{},{},{},{},{},[&](bool day,uint32_t amount){++calls;check(amount==1400,"money operand");return day?350u:700u;});
          f.cpu.gprs[4]=1400;f.invoke(BattleHooks::TrainerMoneyTextAddress,0xFA);
          check(f.cpu.gprs[4]==700 && f.cpu.prefetch[0]==0x490F,"trainer amount uses native text encoder");
          f.cpu.gprs[4]=1400;f.invoke(BattleHooks::PayDayTextAddress,0xFB);
          check(calls==2 && f.cpu.gprs[4]==350 && Fixture::read32(&f.core,0x02022B4C)==5,"payday changed battle flags");
          hook.check();hook.detach();check(Fixture::read16(&f.core,BattleHooks::TrainerMoneyTextAddress)==0x490F,"money hook restored"); }
        { Fixture f;BattleHooks hook(&f.core,empty,{},{},{},{},{},{},{},{},[](bool,uint32_t)->uint32_t{throw std::runtime_error("money fixture");});
          f.cpu.gprs[4]=1400;f.invoke(BattleHooks::TrainerMoneyTextAddress,0xFA);threw=false;
          try{hook.check();}catch(const std::runtime_error&){threw=true;}
          check(threw && f.cpu.gprs[4]==1400 && f.cpu.prefetch[0]==0x490F,"money exception escaped or corrupted native operand"); }
        std::cout<<"FireRed battle hook synthetic fixtures passed\n";
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
