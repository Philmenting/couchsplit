// SPDX-License-Identifier: GPL-3.0-or-later
// Real first-rival trigger through native movement and field scripts.
// P2 identity is an explicit authored fixture; no battle/encounter RAM is written.
// Requires a private starter checkpoint and exact original, never distributed.
#include "../gba/FireRedBattle.h"
#include "FireRedSaveAcceptance.h"
#include "FireRedProgressAcceptance.h"
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>

using namespace coop::firered;
namespace {
constexpr auto RomHash = "eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
QByteArray hashFile(const QString& path) {
    QFile f(path); requirePokemon(f.open(QIODevice::ReadOnly), "Cannot read input");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    requirePokemon(hash.addData(&f), "Cannot hash input"); return hash.result().toHex();
}
struct Core {
    mCore* c = nullptr; bool initialized = false, configured = false;
    std::array<color_t, 240 * 160> video{};
    ~Core() { if (c) { if (configured) mCoreConfigDeinit(&c->config); if (initialized) c->deinit(c); else free(c); } }
    void open(const QString& rom, const QByteArray& checkpoint) {
        c = mCoreFind(rom.toUtf8().constData()); requirePokemon(c && c->init(c), "Cannot initialize mGBA"); initialized = true;
        mCoreInitConfig(c, nullptr); configured = true;
        mCoreOptions options{}; options.useBios = false; options.skipBios = true;
        mCoreConfigLoadDefaults(&c->config, &options); mCoreLoadConfig(c);
        requirePokemon(mCoreLoadFile(c, rom.toUtf8().constData()), "Cannot load ROM");
        c->setVideoBuffer(c, video.data(), 240); c->reset(c);
        requirePokemon(checkpoint.size() == qint64(c->stateSize(c)) && c->loadState(c, checkpoint.constData()), "Cannot restore private checkpoint");
        c->setKeys(c, 0); c->runFrame(c);
    }
    void capture(const QString& path, const WorldActor* local = nullptr, const WorldActor* remote = nullptr) {
        QImage image(240,160,QImage::Format_ARGB32);
        for (int y=0;y<160;++y) for(int x=0;x<240;++x) {
            const auto p=video[y*240+x]; image.setPixel(x,y,0xFF000000u|((p&255)<<16)|(p&0xFF00)|((p>>16)&255));
        }
        if (local && remote) compositePartner(reinterpret_cast<uint32_t*>(image.bits()), 240, *local, *remote);
        requirePokemon(image.save(path), "Cannot save acceptance capture");
    }
};

}
int main(int argc,char** argv) {
 QCoreApplication app(argc,argv);
 if(argc!=4 && argc!=5){fprintf(stderr,"verify-firered-story ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_OUTPUT_DIRECTORY [defeat|dex|parcel|brock|brock-full]\n");return 2;}
 try {
  QString rom=argv[1],state=argv[2],out=argv[3];
  const auto scenario=argc==5?QString::fromLocal8Bit(argv[4]):QStringLiteral("rival");
  const bool defeat=scenario=="defeat";
  requirePokemon(scenario=="rival" || defeat || scenario=="dex" || scenario=="parcel" || scenario=="brock" || scenario=="brock-full","Unknown natural-story scenario");
  requirePokemon(hashFile(rom)==RomHash,"ROM identity mismatch");
  QFile f(state);requirePokemon(f.open(QIODevice::ReadOnly),"checkpoint missing");auto bytes=f.readAll();
  const auto checkpointHash=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();
  requirePokemon(QDir().mkdir(out),"output must be new");
  mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
  Core a,b;a.open(rom,bytes);b.open(rom,bytes);
  const std::array<BattleStatistics,2> beforeStatistics{{readBattleStatistics(a.c),readBattleStatistics(b.c)}};
  auto secondIdentity=readTrainerIdentity(b.c);secondIdentity.id=0xCAFEBABE;secondIdentity.name[0]=0xBC;secondIdentity.name[1]=0xFF;
  auto sb=readSaveMemory(b.c);for(unsigned i=0;i<4;++i)b.c->busWrite8(b.c,sb.block2+0xA+i,uint8_t(secondIdentity.id>>(i*8)));
  for(unsigned i=0;i<secondIdentity.name.size();++i)b.c->busWrite8(b.c,sb.block2+i,secondIdentity.name[i]);
  auto p=readParty(b.c);for(unsigned i=0;i<p.count;++i)p.mons[i]=assignCaptureTrainer(p.mons[i],secondIdentity);writeParty(b.c,p);
  if(scenario=="brock" || scenario=="brock-full") {
   TeamBattle pair({{a.c,b.c}});
   const auto proof=acceptance::verifyNativeBrockScene(a.c,b.c,pair,rom.toStdString(),scenario=="brock-full");
   requirePokemon(hashFile(rom)==RomHash && hashFile(state)==checkpointHash,"Story input changed");
   a.capture(out+"/source-complete.png");b.capture(out+"/recipient-complete.png");
   QJsonObject report{{"date","2026-10-29"},{"scenario",scenario},{"rom_sha256",RomHash},
    {"checkpoint_sha256",QString::fromLatin1(checkpointHash)},{"two_real_mgba_cores",true},
    {"authored_quest_prerequisites_and_teams",true},{"original_native_scene_executed",true},
    {"recipient_deliveries",int(proof.deliveries)},{"frames",int(proof.frames)},
    {"original_trainer_prize",int(proof.originalPrize)},
    {"source_wallet",int(proof.sourceMoney)},{"recipient_wallet",int(proof.recipientMoney)},
    {"full_bag_reward_deferred",proof.fullBagDeferred},{"later_native_tm_claim",proof.laterNativeClaim},
    {"native_save_and_fresh_battery_reload_both_players",true},
    {"reviewed_quest_flags_survive_reload",true},{"battery_files_opened_or_written",false},
    {"production_launcher",false},{"complete_world_coop",false}};
   QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot create story report");
   const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Cannot save story report");
   printf("%s",json.constData());return 0;
  }
  if(scenario=="dex" || scenario=="parcel") {
   unsigned frames=0,deliveries=0;
   if(scenario=="dex") {
    const auto proof=acceptance::verifyNativePokedexScene(a.c,b.c,rom.toStdString());
    frames=proof.frames;deliveries=proof.pokedexDeliveries;
   } else {
    const auto proof=acceptance::verifyNativeParcelScene(a.c,b.c,rom.toStdString());
    frames=proof.frames;deliveries=proof.deliveries;
   }
   requirePokemon(hashFile(rom)==RomHash && hashFile(state)==checkpointHash,"Story input changed");
   a.capture(out+"/source-complete.png");b.capture(out+"/recipient-complete.png");
   QJsonObject report{{"date","2026-10-29"},{"scenario",scenario},{"rom_sha256",RomHash},
    {"checkpoint_sha256",QString::fromLatin1(checkpointHash)},{"two_real_mgba_cores",true},
    {"authored_quest_prerequisites",true},{"original_native_scene_executed",true},
    {"recipient_deliveries",int(deliveries)},{"frames",int(frames)},
    {"native_save_and_fresh_battery_reload_both_players",true},
    {"reviewed_quest_flags_survive_reload",true},{"battery_files_opened_or_written",false},
    {"production_launcher",false},{"complete_world_coop",false}};
   QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot create story report");
   const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Cannot save story report");
   printf("%s",json.constData());return 0;
  }
  if(defeat)for(auto* c:{a.c,b.c}) {
   auto party=readParty(c);
   for(unsigned slot=0;slot<party.count;++slot) {
    party.mons[slot].setHp(1);
    auto secure=party.mons[slot].secure();monPut16(secure.data()+12,45);secure[20]=40;
    party.mons[slot].setSecure(secure);
   }
   writeParty(c,party);
  }
  StoryProgress progress({{a.c,b.c}});
  progress.event=[](StoryEvent type,unsigned owner,bool deferred){fprintf(stdout,"native story replicated event=%u player=%u deferred=%u\n",unsigned(type),owner,deferred);};
  TeamBattle pair({{a.c,b.c}});pair.event=[](const char* event,int detail){fprintf(stdout,"%s %d\n",event,detail);fflush(stdout);};
  unsigned frames=0,firstOwned=0,secondOwned=0;bool joined=false;
  while(frames<18000) {
   pair.sampleWorld();
   if(!pair.active()) {
    progress.sample();
    if(a.c->busRead32(a.c,MainCallbackAddress)==InitBattleCallback) {
     fprintf(stdout,"natural encounter flags=%08X return=%08X trainer=%u\n",a.c->busRead32(a.c,BattleFlagsAddress),a.c->busRead32(a.c,SavedCallbackAddress),a.c->busRead16(a.c,TrainerAddress));fflush(stdout);
     requirePokemon(pair.tryBegin(0),"Native rival encounter not joinable");joined=true;
    }
   }
   if(pair.active()) {
    const unsigned pulse=frames%30<4?1u:0u;
    pair.runBattleFrame({{pulse,pulse}});firstOwned+=pair.inputOwner()==0;secondOwned+=pair.inputOwner()==1;
    if(!pair.active())break;
   } else {
    a.c->setKeys(a.c,frames<50?0x20:(frames<150?0x80:(frames%30<4?1:0)));a.c->runFrame(a.c);
    b.c->setKeys(b.c,0);b.c->runFrame(b.c);progress.sample();
   }
   if(frames%500==0)a.capture(out+"/frame"+QString::number(frames)+".png");
   ++frames;
  }
  a.capture(out+"/last.png");
  requirePokemon(joined && !pair.active() && firstOwned && secondOwned,"Natural encounter did not complete with both owners");
  unsigned settling=0;
  while(settling++<10000) {
   progress.sample();
   bool ready=true;
   for(auto* c:{a.c,b.c}) {
    const bool field=progressFieldReady(c);
    ready &= field && storyEventComplete(readProgress(c),StoryEvent::LabRival);
    if(!field){c->setKeys(c,settling%30<4?2u:0u);c->runFrame(c);}
   }
   if(ready)break;
  }
  requirePokemon(storyEventComplete(readProgress(a.c),StoryEvent::LabRival) && storyEventComplete(readProgress(b.c),StoryEvent::LabRival),"First rival progression did not reach both native saves");
  requirePokemon(readParty(a.c).mons[0].hp()==readParty(a.c).mons[0].maxHp() && readParty(b.c).mons[0].hp()==readParty(b.c).mons[0].maxHp(),"First rival did not heal both owners");
  requirePokemon(readBattleStatistics(a.c)==recordSharedBattle(beforeStatistics[0],true) &&
                 readBattleStatistics(b.c)==recordSharedBattle(beforeStatistics[1],true),"First rival did not count once for both trainers");
  for(auto* c:{a.c,b.c}) {
   requirePokemon(acceptance::save_detail::fieldReady(c),"Native rival story did not return to field");
   acceptance::verifyNativeSaveReload(c,rom.toStdString());
  }
  requirePokemon(hashFile(rom)==RomHash,"ROM changed");
  requirePokemon(hashFile(state)==checkpointHash,"Checkpoint changed");
  QJsonObject report{{"date","2026-10-29"},{"scenario",scenario},{"rom_sha256",RomHash},
   {"checkpoint_sha256",QString::fromLatin1(checkpointHash)},
   {"native_movement_and_script_encounter",true},{"authored_encounter",false},
   {"second_trainer_identity_fixture",true},{"authored_hp_and_move_for_loss",defeat},
   {"two_real_mgba_cores",true},{"both_input_owners",true},
   {"first_rival_progress_both_players",true},{"native_healing_both_players",true},
   {"battle_statistics_counted_once_both_players",true},
   {"native_save_and_fresh_battery_reload_both_players",true},
   {"battery_files_opened_or_written",false},{"complete_world_coop",false},
   {"production_launcher",false},{"frames",int(frames)}};
  QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot create story report");
  const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Cannot save story report");
  printf("natural rival complete frames=%u inputowned=%u/%u two native saves/reloads preserved; fixture P2 trainer identity rekeyed, no authored encounter\n",frames,firstOwned,secondOwned);return 0;
 }catch(const std::exception& error){fprintf(stderr,"Natural pair failed: %s\n",error.what());return 1;}
}
