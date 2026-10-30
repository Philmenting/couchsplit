// SPDX-License-Identifier: GPL-3.0-or-later
// Private-ROM native acceptance. Never installed or called by the launcher.
// Quest prerequisites and map entry are authored fixtures; game scripts,
// natural ice cracking, native SAVE and fresh battery CONTINUE run normally.
#include "../gba/FireRedCampaignRuntime.h"
#include "FireRedSaveAcceptance.h"
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <cstdio>

using namespace coop::firered;
namespace {
constexpr auto RomHash="eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
QByteArray fileHash(const QString& path) {
    QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly),"Cannot open verification input");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    requirePokemon(hash.addData(&f),"Cannot hash verification input");return hash.result().toHex();
}
struct Core {
    mCore* c=nullptr;bool initialized=false,configured=false;
    std::array<color_t,240*160> video{};
    std::vector<uint8_t> initialFlash;
    ~Core() {
        if(!c)return;
        if(configured)mCoreConfigDeinit(&c->config);
        if(initialized)c->deinit(c);else free(c);
    }
    void open(const QString& rom,const QByteArray& state) {
        c=mCoreFind(rom.toUtf8().constData());requirePokemon(c&&c->init(c),"Cannot initialize native core");initialized=true;
        mCoreInitConfig(c,nullptr);configured=true;mCoreOptions options{};options.useBios=false;options.skipBios=true;
        mCoreConfigLoadDefaults(&c->config,&options);mCoreLoadConfig(c);
        requirePokemon(mCoreLoadFile(c,rom.toUtf8().constData()),"Cannot load verified original ROM");
        c->setVideoBuffer(c,video.data(),240);c->reset(c);
        requirePokemon(size_t(state.size())==c->stateSize(c)&&c->loadState(c,state.data()),"Private starter checkpoint rejected");
        c->setKeys(c,0);c->runFrame(c);initialFlash=acceptance::save_detail::flash(c);
        requirePokemon(initialFlash.size()==131072,"Checkpoint does not have native Flash128");
    }
    void restore(const QByteArray& state) {
        requirePokemon(c->loadState(c,state.data()),"Cannot restore initial machine fixture");
        requirePokemon(c->savedataRestore(c,initialFlash.data(),initialFlash.size(),true),"Cannot restore matching initial Flash fixture");
        requirePokemon(acceptance::save_detail::flash(c)==initialFlash,"Initial Flash restore did not preserve bytes");
    }
};
struct Grid {
    uint32_t address=0;unsigned width=0,height=0;
    unsigned byteSize() const {return (width+15)*(height+14)*2;}
    unsigned offset(unsigned x,unsigned y) const {return ((y+7)*(width+15)+x+7)*2;}
};
Grid nativeGrid(mCore* c) {
    const auto layout=inventoryRead32(c,CampaignMapHeaderAddress);
    Grid g;g.width=inventoryRead32(c,layout);g.height=inventoryRead32(c,layout+4);
    requirePokemon(g.width&&g.height&&g.width<=2048&&g.height<=2048&&g.byteSize()<=0x40000,
                   "Native map grid dimensions are invalid");
    for(uint32_t at=0x03000000;at<0x03007FF4;at+=4)
        if(inventoryRead32(c,at)==g.width+15&&inventoryRead32(c,at+4)==g.height+14) {
            const auto pointer=inventoryRead32(c,at+8);
            if(validSaveBlock(pointer,g.byteSize())) {
                requirePokemon(!g.address,"Native map grid is ambiguous");g.address=pointer;
            }
        }
    requirePokemon(g.address,"Native map grid is unavailable");return g;
}
enum Kind {Seafoam3,Seafoam4,Icefall,Dolls,Chamber};
struct Fixture {unsigned group,map,x,y;Kind kind;const char* name;};
constexpr std::array<Fixture,11> Fixtures{{
    {1,86,5,6,Seafoam3,"seafoam-b3f"},{1,87,5,6,Seafoam4,"seafoam-b4f"},
    {1,111,8,4,Icefall,"icefall-owned-ice"},{35,4,4,7,Dolls,"lorelei-owned-dolls"},
    {2,27,3,6,Chamber,"tanoby-monean"},{2,28,3,6,Chamber,"tanoby-liptoo"},
    {2,29,3,6,Chamber,"tanoby-weepth"},{2,30,3,6,Chamber,"tanoby-dilford"},
    {2,31,3,6,Chamber,"tanoby-scufib"},{2,32,3,6,Chamber,"tanoby-rixy"},
    {2,33,3,6,Chamber,"tanoby-viapois"},
}};
unsigned prepare(mCore* c,const Fixture& fixture) {
    auto before=readProgress(c),p=before;
    p.flag(FlagPokedexObtained,true);p.var(VarLabScene,6);
    for(unsigned v=0x4000;v<0x4010;++v)p.var(v,0);
    if(fixture.kind==Seafoam3||fixture.kind==Seafoam4) {
        const bool fourth=fixture.kind==Seafoam4;
        p.flag(fourth?0x2D3:0x2D2,true);p.flag(fourth?0x4C:0x46,false);p.flag(fourth?0x4D:0x47,false);
    }
    if(fixture.kind==Dolls) {
        p.flag(0x8C,false);for(unsigned flag=0xA5;flag<=0xAC;++flag)p.flag(flag,true);
    }
    if(fixture.kind==Chamber)p.flag(0x849,false);
    writeProgressDelta(c,before,p);
    const auto m=readSaveMemory(c);
    if(fixture.kind==Dolls) {
        const auto encrypted=25u^m.key;
        for(unsigned i=0;i<4;++i)c->busWrite8(c,m.block1+0x1228+i,uint8_t(encrypted>>(8*i)));
    }
    inventoryWrite16(c,m.block1,fixture.x);inventoryWrite16(c,m.block1+2,fixture.y);
    c->busWrite8(c,m.block1+4,fixture.group);c->busWrite8(c,m.block1+5,fixture.map);
    c->busWrite8(c,0x03003478,0);c->busWrite32(c,0x03003044,0x0805673D); // Authored native map-entry fixture.
    unsigned frames=0;
    for(;frames<600;++frames) {c->setKeys(c,0);c->runFrame(c);if(campaignFieldQuiescent(c))break;}
    requirePokemon(frames<600,"Native fixture map did not become idle");
    if(fixture.kind==Icefall) {
        const auto object=CampaignObjectsAddress+c->busRead8(c,CampaignAvatarAddress+5)*0x24;
        for(unsigned n=0;n<60&&inventoryRead16(c,object+0x12)!=10;++n) {c->setKeys(c,0x40);c->runFrame(c);}
        c->setKeys(c,0);for(unsigned n=0;n<40;++n)c->runFrame(c);
        requirePokemon(campaignFieldQuiescent(c)&&readProgress(c).flag(1),"Actual native step did not crack first ice tile");
    }
    return frames;
}
unsigned continueFresh(mCore* c) {
    for(unsigned n=0;n<10000;++n) {
        const auto cb=inventoryRead32(c,0x03003044);
        c->setKeys(c,n<300?(n%120<3?8u:0u):(n%40<3?(cb==0x080565D5?2u:1u):0u));c->runFrame(c);
        if(n>300&&campaignFieldQuiescent(c)) {c->setKeys(c,0);return n+1;}
    }
    throw std::runtime_error("Native CONTINUE did not reach idle field");
}
QJsonObject verify(mCore* c,const Fixture& fixture,const QString& rom,unsigned entryFrames) {
    const auto m=readSaveMemory(c);
    const auto x=inventoryRead16(c,m.block1),y=inventoryRead16(c,m.block1+2);
    auto before=readProgress(c),after=before;
    if(fixture.kind==Dolls)after.flag(0x8C,true);
    else if(fixture.kind==Chamber)after.flag(0x849,true);
    else after.flag(0x821,true); // Authored unrelated shared campaign receipt.
    const auto layout=inventoryRead32(c,CampaignMapHeaderAddress);
    const auto layoutId=inventoryRead16(c,m.block1+0x32);
    const auto grid=nativeGrid(c);
    const auto gridBytes=acceptance::save_detail::bytes(c,grid.address,grid.byteSize());
    const auto ownParty=readParty(c);const auto ownBag=readInventory(c);
    const auto ownCount=inventoryRead32(c,m.block1+0x1228)^m.key;
    const auto plan=planCampaignObjectRefresh(c,before,after);requirePokemon(plan.ready,plan.reason.c_str());
    if(fixture.kind==Dolls)requirePokemon(plan.removedObjects==1,"Lorelei live object did not refresh");
    if(fixture.kind==Chamber)
        requirePokemon(plan.changedWeather&&c->busRead8(c,m.block1+0x2E)==11,"Chamber did not start with native shade");
    writeProgressDelta(c,before,after);requirePokemon(startCampaignObjectRefresh(c,plan),"Native map refresh did not start");
    for(unsigned n=0;n<90;++n) {c->setKeys(c,0);c->runFrame(c);}
    requirePokemon(campaignFieldQuiescent(c),"Native map refresh did not release field ownership");
    requirePokemon(readParty(c)==ownParty&&readInventory(c)==ownBag,"Map refresh changed own party or bag");
    requirePokemon(acceptance::save_detail::bytes(c,grid.address,gridBytes.size())==gridBytes,"Map refresh changed own native puzzle grid");
    requirePokemon(inventoryRead32(c,CampaignMapHeaderAddress)==layout&&inventoryRead16(c,m.block1+0x32)==layoutId,
                   "Map refresh replaced own native layout");
    requirePokemon((inventoryRead32(c,m.block1+0x1228)^m.key)==ownCount,"Map refresh changed personal Hall-of-Fame count");
    requirePokemon(inventoryRead16(c,m.block1)==x&&inventoryRead16(c,m.block1+2)==y,"Map refresh moved player");
    if(fixture.kind==Icefall)requirePokemon(readProgress(c).flag(1),"Map refresh reset naturally cracked ice");
    if(fixture.kind==Dolls)requirePokemon(!readProgress(c).flag(0xA5)&&readProgress(c).flag(0xA6),"Personal doll thresholds changed");
    if(fixture.kind==Chamber)requirePokemon(c->busRead8(c,m.block1+0x2E)==0,"Unlocked chamber retained native shade");
    const auto save=acceptance::verifyNativeSaveReload(c,rom.toStdString());
    acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),save.battery);
    const auto freshFrames=continueFresh(fresh.c);
    const auto fm=readSaveMemory(fresh.c);
    const auto p=readProgress(fresh.c);
    requirePokemon(p.mapGroup==fixture.group&&p.mapNum==fixture.map,"Native CONTINUE lost own map");
    if(fixture.kind==Seafoam3||fixture.kind==Seafoam4)
        requirePokemon(inventoryRead16(fresh.c,fm.block1+0x32)==layoutId&&p.flag(fixture.kind==Seafoam4?0x2D3:0x2D2),
                       "Native CONTINUE lost own solved current/layout");
    if(fixture.kind==Icefall) {
        const auto continuedGrid=nativeGrid(fresh.c);
        requirePokemon((inventoryRead16(fresh.c,continuedGrid.address+continuedGrid.offset(8,3))&0xFFF)==
                       (monU16(gridBytes.data()+grid.offset(8,3))&0xFFF),"Native CONTINUE lost naturally cracked ice tile");
    }
    if(fixture.kind==Dolls)
        requirePokemon((inventoryRead32(fresh.c,fm.block1+0x1228)^fm.key)==25&&p.flag(0x8C)&&!p.flag(0xA5)&&p.flag(0xA6),
                       "Native CONTINUE lost own dolls or removed Lorelei");
    if(fixture.kind==Chamber)
        requirePokemon(p.flag(0x849)&&fresh.c->busRead8(fresh.c,fm.block1+0x2E)==0,"Native CONTINUE lost chamber lighting");
    QJsonObject report{{"scenario",fixture.name},{"authored_map_entry_and_quest_prerequisites",true},
        {"native_map_entry_frames",int(entryFrames)},{"own_native_layout_id",int(layoutId)},
        {"refresh_program_bytes",int(plan.code.size())},{"production_runtime",true},
        {"own_grid_party_bag_position_and_personal_counter_preserved",true},
        {"native_save_generation",int(save.generation)},{"native_save_frames",int(save.saveFrames)},
        {"native_reload_frames",int(save.rebootFrames)},{"additional_native_continue_frames",int(freshFrames)},
        {"native_save_and_fresh_continue",true}};
    if(fixture.kind==Icefall)report.insert("original_native_step_cracked_ice_and_survived_continue",true);
    if(fixture.kind==Dolls)report.insert("authored_25_own_hall_of_fame_clears_and_personal_dolls_retained",true);
    if(fixture.kind==Seafoam3||fixture.kind==Seafoam4)report.insert("authored_solved_current_native_layout_preserved",true);
    if(fixture.kind==Chamber)report.insert("native_shade_to_clear_lighting_persisted",true);
    return report;
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    if(argc!=4) {fprintf(stderr,"verify-firered-map-setup ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_OUTPUT_DIRECTORY\n");return 2;}
    try {
        const QString rom=QString::fromLocal8Bit(argv[1]),checkpoint=QString::fromLocal8Bit(argv[2]),out=QString::fromLocal8Bit(argv[3]);
        requirePokemon(fileHash(rom)==RomHash,"Map setup verification requires the exact German original ROM");
        const auto checkpointHash=fileHash(checkpoint);
        requirePokemon(!QFileInfo::exists(out),"Verification output directory already exists");
        QFile f(checkpoint);requirePokemon(f.open(QIODevice::ReadOnly)&&f.size()<2*1024*1024,"Cannot read bounded private checkpoint");
        const auto state=f.readAll();requirePokemon(QDir().mkpath(out),"Cannot create new verification output directory");
        mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;
        mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
        Core core;core.open(rom,state);QJsonArray scenarios;
        for(const auto& fixture:Fixtures) {
            core.restore(state);const auto entry=prepare(core.c,fixture);scenarios.append(verify(core.c,fixture,rom,entry));
            fprintf(stdout,"Native map setup: %s PASS\n",fixture.name);fflush(stdout);
        }
        requirePokemon(fileHash(rom)==RomHash&&fileHash(checkpoint)==checkpointHash,"Verification changed protected inputs");
        const QJsonObject result{{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(checkpointHash)},
            {"rom_and_checkpoint_unchanged",true},{"source_revision","ce6406d8c2eaf46d543c8a2606de0c3cb1951982"},
            {"native_scenarios",scenarios},{"complete_campaign_playthrough",false},{"production_launcher",false}};
        QSaveFile report(out+"/result.json");requirePokemon(report.open(QIODevice::WriteOnly),"Cannot create result report");
        const auto json=QJsonDocument(result).toJson(QJsonDocument::Indented);
        requirePokemon(report.write(json)==json.size()&&report.commit(),"Cannot commit result report");
        return 0;
    } catch(const std::exception& e) {fprintf(stderr,"Native map setup acceptance failed: %s\n",e.what());return 1;}
}
