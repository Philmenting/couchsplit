// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// ROM-dependent acceptance helper, never installed or called by the launcher.
// The caller verifies the exact supported German original and pauses its core.
// Save requests below are native button presses. Only private in-memory Flash
// clones are opened by the new core; no battery path or user save is accessed.
#include "../gba/FireRedStorage.h"
#include "../gba/FireRedProgress.h"
#include "../gba/FireRedPersonal.h"
#include "../gba/FireRedStatistics.h"
#include <mgba/core/config.h>
#include <mgba-util/vfs.h>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace coop::firered::acceptance {
struct NativeSaveProof {
    std::vector<uint8_t> battery;
    uint32_t generation = 0;
    unsigned saveFrames = 0, rebootFrames = 0;
};
namespace save_detail {
inline void demand(bool ok, const std::string& why) {
    if (!ok) throw std::runtime_error("Native FireRed save acceptance: " + why);
}
inline std::vector<uint8_t> bytes(mCore* c, uint32_t at, unsigned n) {
    std::vector<uint8_t> r(n);
    for (unsigned i=0;i<n;++i) r[i]=uint8_t(c->busRead8(c,at+i));
    return r;
}
struct Snapshot {
    Party party;
    Inventory bag;
    StorageSnapshot pc;
    uint32_t wallet;
    std::vector<uint8_t> dex, pcItems, trainer;
    std::array<bool, 9> questFlags{};
    std::array<uint16_t, 3> questStages{};
    uint32_t dexUnlocks = 0;
    std::array<uint32_t, 2> questFame{};
    PersonalCaptureState personal;
    BattleStatistics statistics;
    std::array<uint32_t,3> actionStatistics{};
    explicit Snapshot(mCore* c):party(readParty(c)),bag(readInventory(c)),pc(readStorage(c)),wallet(readWallet(c)),
        personal(readPersonalCapture(c)),statistics(readBattleStatistics(c)) {
        actionStatistics={{readGameStatistic(c,14),readGameStatistic(c,26),readGameStatistic(c,27)}};
        const auto m=readSaveMemory(c);
        dex=bytes(c,m.block2+0x18,0x78);
        for (const auto off:{0x5F8u,0x3A18u}) {
            const auto seen=bytes(c,m.block1+off,0x34);dex.insert(dex.end(),seen.begin(),seen.end());
        }
        pcItems=bytes(c,m.block1+0x298,0x78);
        trainer=bytes(c,m.block2,9); // Name + gender; byte 9 is a warp flag.
        const auto id=bytes(c,m.block2+0xA,4);trainer.insert(trainer.end(),id.begin(),id.end());
        const auto progress=readProgress(c);
        unsigned i=0;
        for(const auto flag:{0x258u,0x828u,0x829u,0x820u,0x4B0u,0x254u,0x02Du,0x03Au,0x02Eu})
            questFlags[i++]=progress.flag(flag);
        i=0;
        for(const auto var:{0x4055u,0x4057u,0x406Cu})questStages[i++]=progress.var(var);
        dexUnlocks=progress.gcnLinkFlags & 0x31u;
        questFame={{inventoryRead32(c,m.block1+0x3A54),inventoryRead32(c,m.block1+0x3A5C)}};
        checkSaveMemory(c,m);
    }
    void compare(const Snapshot& r) const {
        demand(party==r.party,"party did not survive native save/reboot");
        demand(wallet==r.wallet,"wallet did not survive native save/reboot");
        if (!(bag==r.bag)) {
            for (unsigned pocket=0;pocket<PocketCount;++pocket)
                for (unsigned slot=0;slot<PocketCapacities[pocket];++slot) {
                    const auto& a=bag.pockets[pocket][slot];const auto& b=r.bag.pockets[pocket][slot];
                    if (!(a==b)) throw std::runtime_error("Native FireRed save acceptance: bag changed at pocket " +
                        std::to_string(pocket) + " slot " + std::to_string(slot) + " from " + std::to_string(a.item) + "/" +
                        std::to_string(a.quantity) + " to " + std::to_string(b.item) + "/" + std::to_string(b.quantity));
                }
            demand(false,"bag did not survive native save/reboot");
        }
        demand(pc==r.pc,"PC Pokemon or box metadata did not survive native save/reboot");
        demand(dex==r.dex,"Pokedex or duplicate seen flags did not survive native save/reboot");
        demand(pcItems==r.pcItems,"PC items did not survive native save/reboot");
        demand(trainer==r.trainer,"trainer identity did not survive native save/reboot");
        demand(questFlags==r.questFlags && questStages==r.questStages && dexUnlocks==r.dexUnlocks,
               "reviewed quest milestones or Pokedex unlocks did not survive native save/reboot");
        demand(actionStatistics==r.actionStatistics,"personal evolution/action counters did not survive native save/reboot");
        demand(personal==r.personal && statistics==r.statistics,
               "personal capture metadata or battle counters did not survive native save/reboot");
        demand(questFame==r.questFame,"reviewed Fame Checker entries did not survive native save/reboot");
    }
};
inline bool fieldReady(mCore* c) {
    return progressFieldReady(c);
}
inline std::vector<uint8_t> flash(mCore* c) {
    void* raw=nullptr;
    const auto size=c->savedataClone(c,&raw);
    std::unique_ptr<void,decltype(&std::free)> memory(raw,&std::free);
    demand(raw && size==0x20000,"core has no complete 128 KiB Flash clone");
    return {static_cast<uint8_t*>(raw),static_cast<uint8_t*>(raw)+size};
}
inline std::optional<uint32_t> generation(const std::vector<uint8_t>& b) {
    if(b.size()!=0x20000)return std::nullopt;
    std::optional<uint32_t> latest;
    for(unsigned slot=0;slot<2;++slot) {
        unsigned mask=0;uint32_t counter=0;bool valid=true;
        for(unsigned sector=0;sector<14;++sector) {
            const auto* p=b.data()+(slot*14+sector)*0x1000;
            const unsigned id=monU16(p+0xFF4);
            if(id>=14 || (mask&(1u<<id)) || monU32(p+0xFF8)!=0x08012025){valid=false;break;}
            const uint32_t total=id==0?SaveBlock2Size:id<5?SaveBlock1Size:PokemonStorageSize;
            const unsigned chunk=id==0?0:id<5?id-1:id-5;
            const unsigned size=std::min(0xF80u,total-chunk*0xF80u);
            uint32_t sum=0;for(unsigned at=0;at<size;at+=4)sum+=monU32(p+at);
            const auto n=monU32(p+0xFFC);
            if(monU16(p+0xFF6)!=uint16_t((sum>>16)+sum) || (sector && n!=counter)){valid=false;break;}
            counter=n;mask|=1u<<id;
        }
        if(valid && mask==0x3FFF && (!latest || int32_t(counter-*latest)>0))latest=counter;
    }
    return latest;
}
struct FreshCore {
    mCore* c=nullptr;bool initialized=false,configured=false;
    std::array<color_t,240*160> video{};
    FreshCore()=default;
    FreshCore(const FreshCore&)=delete;
    FreshCore& operator=(const FreshCore&)=delete;
    ~FreshCore(){if(c){if(configured)mCoreConfigDeinit(&c->config);if(initialized)c->deinit(c);else std::free(c);}}
    void open(const std::string& rom,const std::vector<uint8_t>& battery) {
        c=mCoreFind(rom.c_str());demand(c && c->init(c),"fresh core initialization failed");initialized=true;
        mCoreInitConfig(c,nullptr);configured=true;
        mCoreOptions options{};options.useBios=false;options.skipBios=true;
        mCoreConfigLoadDefaults(&c->config,&options);mCoreLoadConfig(c);
        demand(mCoreLoadFile(c,rom.c_str()),"fresh core cannot open verified ROM");
        c->setVideoBuffer(c,video.data(),240);
        auto* file=VFileMemChunk(battery.data(),battery.size());demand(file,"cannot create isolated Flash clone");
        if(!c->loadSave(c,file)){file->close(file);demand(false,"fresh core rejected Flash clone");}
        c->reset(c);
    }
};
}
inline NativeSaveProof verifyNativeSaveReload(mCore* live,const std::string& verifiedRomPath) {
    using namespace save_detail;
    demand(live && live->setKeys && live->runFrame && live->savedataClone,"invalid live core");
    demand(fieldReady(live),"live core is not in an unlocked normal overworld");
    const Snapshot expected(live);
    const auto previous=generation(flash(live));
    NativeSaveProof proof;
    struct Release {mCore* c;~Release(){c->setKeys(c,0);}}release{live};
    auto frame=[&](unsigned keys){live->setKeys(live,keys);live->runFrame(live);++proof.saveFrames;};
    auto pulse=[&](unsigned keys){for(unsigned n=0;n<3;++n)frame(keys);for(unsigned n=0;n<5;++n)frame(0);};
    pulse(8); // START
    for(unsigned n=0;n<240 && inventoryRead32(live,0x020370F0)!=0x0806F1E5;++n)frame(0);
    demand(inventoryRead32(live,0x020370F0)==0x0806F1E5 && live->busRead8(live,0x03000F9C)!=0,"START menu did not become ready");
    const auto count=live->busRead8(live,0x020370F5);
    demand(count>0 && count<=9,"invalid native START menu length");
    unsigned save=count;for(unsigned n=0;n<count;++n)if(live->busRead8(live,0x020370F6+n)==4)save=n;
    demand(save<count,"native START menu does not offer SAVE here");
    for(unsigned n=0;n<count && live->busRead8(live,0x020370F4)!=save;++n)pulse(0x80); // DOWN
    demand(live->busRead8(live,0x020370F4)==save,"cannot move native cursor to SAVE");
    pulse(1); // A: native Save callback
    demand(inventoryRead32(live,0x020370F0)==0x0806F509 || inventoryRead32(live,0x020370F0)==0x0806F52D,"SAVE callback was not selected");
    // Dialog uses nonwrapping Yes/No. UP ensures YES even for the native
    // different-trainer warning, whose default is NO. Stop on the first field
    // frame so no subsequent A press can trigger a field interaction.
    bool saveComplete = false;
    for(unsigned n=0;n<6000 && !fieldReady(live);++n) {
        const auto phase=n%40;
        // Once Flash contains the completed new generation, B dismisses its
        // final message without immediately talking to an adjacent field NPC.
        if(n%40==0) {
            const auto completed=generation(flash(live));
            saveComplete=saveComplete || (completed && (!previous || uint32_t(*completed-*previous)==1));
        }
        if(saveComplete)
            frame(phase>=8 && phase<11?2:0);
        else frame(phase<3?0x40:phase>=8 && phase<11?1:0);
    }
    demand(fieldReady(live),"native save dialog never returned to the overworld");
    try { expected.compare(Snapshot(live)); }
    catch(const std::exception& e) { throw std::runtime_error(std::string("after native save: ") + e.what()); }
    proof.battery=flash(live);
    const auto next=generation(proof.battery);
    demand(bool(next),"Flash has no checksum-valid complete 14-sector save generation");
    demand(!previous || uint32_t(*next-*previous)==1,"native save did not create the next complete Flash generation");
    proof.generation=*next;
    FreshCore fresh;fresh.open(verifiedRomPath,proof.battery);
    // Skip the intro, confirm title and CONTINUE. During quest-log playback
    // the field is locked: B requests native skip, never a save-RAM shortcut.
    bool enteredField=false;
    for(unsigned n=0;n<6000;++n) {
        unsigned keys=0;
        const auto cb=inventoryRead32(fresh.c,0x03003044);
        if(n<300)keys=(n%120)<3?8:0;
        else if(cb==0x080565D5)keys=(n%60)<3?2:0;
        else keys=(n%60)<3?1:0;
        fresh.c->setKeys(fresh.c,keys);fresh.c->runFrame(fresh.c);++proof.rebootFrames;
        if(n>300 && fieldReady(fresh.c)) {enteredField=true;break;}
    }
    fresh.c->setKeys(fresh.c,0);
    demand(enteredField,"fresh core did not CONTINUE into an unlocked overworld");
    try { expected.compare(Snapshot(fresh.c)); }
    catch(const std::exception& e) { throw std::runtime_error(std::string("after fresh CONTINUE: ") + e.what()); }
    return proof;
}
}
