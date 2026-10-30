// SPDX-License-Identifier: GPL-3.0-or-later
// Private-ROM native continued-trainer progression acceptance. Map/quest/party
// prerequisites are authored; original NPC scripts, encounters and saves run.
#include "../gba/FireRedCampaignCompletion.h"
#include "../gba/FireRedCampaign.h"
#include "../gba/FireRedCampaignRuntime.h"
#include "../gba/FireRedCampaignSync.h"
#include "../gba/FireRedBattle.h"
#include "FireRedProgressAcceptance.h"
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>

using namespace coop::firered;
namespace {
constexpr auto RomHash="eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
QByteArray fileHash(const QString& path) {
 QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly),"Input is unreadable");
 QCryptographicHash h(QCryptographicHash::Sha256);requirePokemon(h.addData(&f),"Cannot hash input");return h.result().toHex();
}
struct Core {
 mCore* c=nullptr;bool configured=false,initialized=false;std::array<color_t,240*160> video{};
 ~Core(){if(c){if(configured)mCoreConfigDeinit(&c->config);if(initialized)c->deinit(c);else free(c);}}
 void open(const QString& rom,const QByteArray& state) {
  c=mCoreFind(rom.toUtf8().constData());requirePokemon(c&&c->init(c),"Cannot create core");initialized=true;
  mCoreInitConfig(c,nullptr);configured=true;mCoreOptions o{};o.useBios=false;o.skipBios=true;
  mCoreConfigLoadDefaults(&c->config,&o);mCoreLoadConfig(c);requirePokemon(mCoreLoadFile(c,rom.toUtf8().constData()),"Cannot load ROM");
  c->setVideoBuffer(c,video.data(),240);c->reset(c);requirePokemon(state.size()==qint64(c->stateSize(c))&&c->loadState(c,state.data()),"Checkpoint rejected");
  c->setKeys(c,0);c->runFrame(c);
 }
 void capture(const QString& path) {
  QImage im(240,160,QImage::Format_ARGB32);for(unsigned i=0;i<video.size();++i){auto p=video[i];im.setPixel(i%240,i/240,0xFF000000u|((p&255)<<16)|(p&0xFF00)|((p>>16)&255));}
  requirePokemon(im.save(path),"Cannot capture native screen");
 }
};
uint32_t mapGrid(mCore* core) {
 const auto layout=inventoryRead32(core,CampaignMapHeaderAddress),width=inventoryRead32(core,layout),height=inventoryRead32(core,layout+4);
 uint32_t grid=0;
 for(uint32_t at=0x03000000;at<0x03007FF4;at+=4)
  if(inventoryRead32(core,at)==width+15&&inventoryRead32(core,at+4)==height+14){const auto p=inventoryRead32(core,at+8);
   if(validSaveBlock(p,(width+15)*(height+14)*2)){requirePokemon(!grid,"Ambiguous native map grid");grid=p;}}
 requirePokemon(grid,"Native map grid missing");return grid;
}
uint16_t tile(mCore* core,unsigned x,unsigned y) {
 const auto width=inventoryRead32(core,inventoryRead32(core,CampaignMapHeaderAddress));
 return inventoryRead16(core,mapGrid(core)+((y+7)*(width+15)+x+7)*2)&0xFFF;
}
bool activeObject(mCore* core,unsigned localId) {
 for(unsigned i=0;i<16;++i){const auto at=CampaignObjectsAddress+i*0x24;
  if((core->busRead8(core,at)&1)&&core->busRead8(core,at+8)==localId)return true;}
 return false;
}
void prepare(Core& owner,unsigned index,bool tower) {
 auto* core=owner.c;auto before=readProgress(core),p=before;
 p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
 p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);
 if(tower){p.var(0x4059,1);p.inventory.setQuantity(359,1);}
 else p.flag(0x500+366,true); // Authored prerequisite: first of two barrier guards already defeated.
 writeProgressDelta(core,before,p);
 Party party;party.count=1;party.mons[0]=acceptance::brockFixtureMon(24+index*24,readTrainerIdentity(core).id);writeParty(core,party);
 const auto memory=readSaveMemory(core);
 inventoryWrite16(core,memory.block1,tower?(index?4:10):(index?18:19));
 inventoryWrite16(core,memory.block1+2,tower?(index?14:11):15);
 core->busWrite8(core,memory.block1+4,1);core->busWrite8(core,memory.block1+5,tower?94:45);
 core->busWrite8(core,0x03003478,0);core->busWrite32(core,MainCallbackAddress,0x0805673D);
 for(unsigned f=0;f<240;++f){core->setKeys(core,0);core->runFrame(core);}
 requirePokemon(campaignFieldQuiescent(core),"Native target map did not enter idle field");
}
unsigned exercise(Core& source,Core& guest,const QString& rom,bool tower) {
 prepare(source,0,tower);prepare(guest,1,tower);
 const unsigned trainer=tower?369:367,hidden=tower?0x05E:0;
 const auto beforeBag=readInventory(guest.c);
 if(tower)requirePokemon(activeObject(guest.c,2),"Recipient Tower grunt was not initially active");
 else for(unsigned x:{17u,18u})for(unsigned y:{12u,13u})requirePokemon(tile(source.c,x,y)&0xC00,"Source Giovanni barrier was not initially closed");
 TeamBattle pair({{source.c,guest.c}});CampaignProgress campaign({{source.c,guest.c}});
 campaign.deferred=[](unsigned owner,const std::string& reason){fprintf(stderr,"progression deferred player=%u: %s\n",owner,reason.c_str());};
 unsigned completed=0,frames=0,input1=0,input2=0;bool joined=false,finished=false;
 pair.completed=[&](unsigned leader,unsigned nativeTrainer,unsigned outcome,bool peerNative){requirePokemon(leader==0&&nativeTrainer==trainer&&outcome==1&&!peerNative,"Unexpected continued-trainer shared battle completion");++completed;};
 for(;frames<16000;++frames) {
  if(!pair.active()) {
   campaign.sample();pair.sampleWorld();
   if(!joined&&inventoryRead32(source.c,MainCallbackAddress)==InitBattleCallback){
    fprintf(stdout,"native continued encounter trainer=%u mode=%u flags=%08X\n",inventoryRead16(source.c,TrainerAddress),inventoryRead16(source.c,0x020386AC),inventoryRead32(source.c,BattleFlagsAddress));fflush(stdout);
    requirePokemon(inventoryRead16(source.c,TrainerAddress)==trainer&&inventoryRead16(source.c,0x020386AC)==2&&pair.tryBegin(0),"Native continued trainer did not join");joined=true;
   }
  }
  if(pair.active()){input1+=pair.inputOwner()==0;input2+=pair.inputOwner()==1;const auto pulse=frames%30<3?1u:0u;pair.runBattleFrame({{pulse,pulse}});}
  else {source.c->setKeys(source.c,frames<12?0x40u:(frames%30<3?1u:0u));source.c->runFrame(source.c);guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);campaign.sample();}
  if(joined&&!pair.active()&&campaignFieldQuiescent(source.c)&&campaignFieldQuiescent(guest.c)) {
   const auto a=readProgress(source.c),b=readProgress(guest.c);
   if(a.flag(0x500+trainer)&&b.flag(0x500+trainer)&&(!tower||(a.flag(hidden)&&b.flag(hidden)))){finished=true;break;}
  }
 }
 requirePokemon(finished&&completed==1&&input1&&input2,"Original continued-trainer scene did not reach both owners");
 for(auto* core:{source.c,guest.c}) {
  core->setKeys(core,0);
  if(tower)requirePokemon(!activeObject(core,2),"Completed native Tower grunt remained active");
  else for(unsigned x:{17u,18u})for(unsigned y:{12u,13u})requirePokemon(!(tile(core,x,y)&0xC00),"Completed native Giovanni barrier remained impassable");
 }
 requirePokemon(readInventory(guest.c)==beforeBag,"Continued gate win invented a bag reward");
 for(auto* core:{source.c,guest.c}) {
  const auto proof=acceptance::verifyNativeSaveReload(core,rom.toStdString());
  acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned f=0;f<10000;++f){auto* c=fresh.c;const auto cb=inventoryRead32(c,MainCallbackAddress);c->setKeys(c,f<300?(f%120<3?8u:0u):(f%40<3?(cb==OverworldCallback?2u:1u):0u));c->runFrame(c);if(f>300&&campaignFieldQuiescent(c)){entered=true;break;}}
  requirePokemon(entered&&readProgress(fresh.c).flag(0x500+trainer),"Native trainer gate receipt lost after save and fresh Continue");
  if(tower)requirePokemon(readProgress(fresh.c).flag(hidden)&&!activeObject(fresh.c,2),"Native Tower grunt returned after fresh Continue");
  else for(unsigned x:{17u,18u})for(unsigned y:{12u,13u})requirePokemon(!(tile(fresh.c,x,y)&0xC00),"Giovanni barrier reclosed after fresh Continue");
 }
 return frames;
}
unsigned lostelleFullBerry(Core& source,Core& guest,const QString& rom) {
 for(unsigned owner=0;owner<2;++owner) {
  auto* c=owner?guest.c:source.c;auto before=readProgress(c),p=before;
  p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);p.flag(0x845,true);p.var(0x4076,1);p.var(0x4079,1);
  p.inventory.setQuantity(367,1);p.inventory.setQuantity(280,1);p.inventory.setQuantity(365,1);p.flag(0x847,true);p.inventory.setQuantity(147,999);
  writeProgressDelta(c,before,p);Party party;party.count=1;party.mons[0]=acceptance::brockFixtureMon(24+owner*24,readTrainerIdentity(c).id);writeParty(c,party);
  const auto m=readSaveMemory(c);inventoryWrite16(c,m.block1,owner?5:4);inventoryWrite16(c,m.block1+2,9);
  c->busWrite8(c,m.block1+4,1);c->busWrite8(c,m.block1+5,109);c->busWrite8(c,0x03003478,0);c->busWrite32(c,MainCallbackAddress,0x0805673D);
  for(unsigned f=0;f<240;++f){c->setKeys(c,0);c->runFrame(c);}
  requirePokemon(campaignFieldQuiescent(c)&&activeObject(c,1),"Native Lostelle scene fixture did not enter with the girl present");
 }
 TeamBattle pair({{source.c,guest.c}});CampaignProgress campaign({{source.c,guest.c}});
 campaign.deferred=[](unsigned owner,const std::string& reason){fprintf(stderr,"Lostelle deferred player=%u: %s\n",owner,reason.c_str());};
 unsigned frames=0,completed=0,input1=0,input2=0;bool joined=false,finished=false;
 pair.completed=[&](unsigned leader,unsigned trainer,unsigned outcome,bool peerNative){requirePokemon(leader==0&&trainer==0&&outcome==1&&!peerNative,"Unexpected native Lostelle encounter completion");++completed;};
 for(;frames<16000;++frames) {
  if(!pair.active()) {
   campaign.sample();pair.sampleWorld();
   if(!joined&&inventoryRead32(source.c,MainCallbackAddress)==InitBattleCallback){
    requirePokemon(inventoryRead32(source.c,BattleFlagsAddress)==0x20000&&pair.tryBegin(0),"Original native Lostelle Hypno scene did not join");joined=true;
   }
  }
  if(pair.active()){input1+=pair.inputOwner()==0;input2+=pair.inputOwner()==1;const auto pulse=frames%30<3?1u:0u;pair.runBattleFrame({{pulse,pulse}});}
  else{source.c->setKeys(source.c,frames<12?0x40u:(frames%30<3?1u:0u));source.c->runFrame(source.c);guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);campaign.sample();}
  if(joined&&!pair.active()&&campaignFieldQuiescent(source.c)&&campaignFieldQuiescent(guest.c)) {
   const auto a=readProgress(source.c),b=readProgress(guest.c);
   if(a.mapGroup==33&&a.mapNum==0&&a.var(0x4079)>=3&&a.flag(0x2A3)&&b.flag(0x2A3)&&b.flag(0x07A)&&!activeObject(guest.c,1)){finished=true;break;}
  }
 }
 requirePokemon(finished&&completed==1&&input1&&input2,"Full optional berry stack blocked original or shared Lostelle rescue");
 for(auto* c:{source.c,guest.c}) {
  c->setKeys(c,0);requirePokemon(readInventory(c).quantity(147)==999,"Full optional berry stack changed during rescue");
  const auto proof=acceptance::verifyNativeSaveReload(c,rom.toStdString());acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned f=0;f<10000;++f){auto* n=fresh.c;const auto cb=inventoryRead32(n,MainCallbackAddress);n->setKeys(n,f<300?(f%120<3?8u:0u):(f%40<3?(cb==OverworldCallback?2u:1u):0u));n->runFrame(n);if(f>300&&campaignFieldQuiescent(n)){entered=true;break;}}
  requirePokemon(entered&&readProgress(fresh.c).flag(0x2A3)&&readProgress(fresh.c).flag(0x07A)&&readInventory(fresh.c).quantity(147)==999,"Native full-bag Lostelle rescue did not persist after fresh Continue");
 }
 // Native BerryForest has no deferred berry claim. Freeing space after rescue
 // and reconstructing a broker must not resurrect this optional gift or girl.
 auto bag=readInventory(guest.c);bag.setQuantity(147,998);writeInventory(guest.c,bag);
 CampaignProgress restarted({{source.c,guest.c}});restarted.sample();restarted.sample();
 requirePokemon(readInventory(guest.c).quantity(147)==998&&readProgress(guest.c).flag(0x2A3)&&!activeObject(guest.c,1),"Rescue replay invented a delayed berry claim or respawned Lostelle");
 return frames;
}
unsigned billTicketFullBag(Core& source,Core& guest,const QString& rom) {
 // The transformation is an explicitly authored prerequisite. The actual
 // ticket conversation and its receipts run in the original game engine.
 for(unsigned owner=0;owner<2;++owner) {
  auto* c=owner?guest.c:source.c;auto before=readProgress(c),p=before;
  p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);
  p.flag(0x233,true);p.flag(0x032,true);p.flag(0x033,false);
  p.flag(0x234,false);p.flag(0x235,false);p.inventory.setQuantity(265,0);
  if(owner) {
   p.inventory.pockets[KeyItems]={};
   for(unsigned item=259,count=0;item<ItemCount && count<PocketCapacities[KeyItems];++item)
    if(item!=265 && itemPocket(item)==int(KeyItems)){p.inventory.setQuantity(item,1);++count;}
   for(unsigned slot=0;slot<PocketCapacities[KeyItems];++slot)
    requirePokemon(p.inventory.pockets[KeyItems][slot].item!=0,"Recipient fixture key-item pocket is not full");
  }
  writeProgressDelta(c,before,p);
  const auto m=readSaveMemory(c);inventoryWrite16(c,m.block1,owner?8:7);inventoryWrite16(c,m.block1+2,6);
  c->busWrite8(c,m.block1+4,30);c->busWrite8(c,m.block1+5,0);
  c->busWrite8(c,0x03003478,0);c->busWrite32(c,MainCallbackAddress,0x0805673D);
  for(unsigned f=0;f<240;++f){c->setKeys(c,0);c->runFrame(c);}
  requirePokemon(campaignFieldQuiescent(c)&&activeObject(c,1),"Bill ticket fixture did not load his native human object");
 }
 for(unsigned f=0;f<12;++f){source.c->setKeys(source.c,0x40);source.c->runFrame(source.c);}
 for(unsigned f=0;f<30;++f){source.c->setKeys(source.c,0);source.c->runFrame(source.c);}
 CampaignProgress campaign({{source.c,guest.c}});unsigned deliveries=0,frames=0;
 campaign.event=[&](unsigned owner,const CampaignApplication&){if(owner==1)++deliveries;};
 campaign.deferred=[](unsigned owner,const std::string& reason){fprintf(stderr,"Bill deferred player=%u: %s\n",owner,reason.c_str());};
 for(;frames<16000;++frames) {
  source.c->setKeys(source.c,frames%30<3?1u:0u);source.c->runFrame(source.c);
  guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);campaign.sample();
  if(campaignFieldQuiescent(source.c)&&readProgress(source.c).flag(0x234))break;
 }
 requirePokemon(frames<16000,"Original Bill ticket dialogue did not complete");
 const auto sourceAfter=readProgress(source.c),guestFull=readProgress(guest.c);
 requirePokemon(sourceAfter.flag(0x233)&&sourceAfter.flag(0x234)&&sourceAfter.flag(0x235)&&
                sourceAfter.inventory.quantity(265)==1,"Original Bill ticket receipt is incomplete");
 requirePokemon(!guestFull.flag(0x234)&&!guestFull.flag(0x235)&&guestFull.inventory.quantity(265)==0,
                "Full recipient bag falsely claimed Bill's ticket");
 auto bag=readInventory(guest.c);bag.setQuantity(259,0);writeInventory(guest.c,bag);
 for(unsigned f=0;f<1200;++f) {
  source.c->setKeys(source.c,0);source.c->runFrame(source.c);guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);campaign.sample();
  if(campaignFieldQuiescent(guest.c)&&readProgress(guest.c).flag(0x234))break;
 }
 requirePokemon(readProgress(guest.c).flag(0x234)&&readProgress(guest.c).flag(0x235)&&
                readInventory(guest.c).quantity(265)==1&&deliveries==1,"Deferred Bill ticket was not delivered exactly once");
 CampaignProgress restarted({{source.c,guest.c}});restarted.sample();restarted.sample();
 requirePokemon(readInventory(guest.c).quantity(265)==1,"Campaign restart duplicated Bill's ticket");
 for(auto* c:{source.c,guest.c}) {
  c->setKeys(c,0);const auto proof=acceptance::verifyNativeSaveReload(c,rom.toStdString());
  acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned f=0;f<10000;++f){auto* n=fresh.c;const auto cb=inventoryRead32(n,MainCallbackAddress);
   n->setKeys(n,f<300?(f%120<3?8u:0u):(f%40<3?(cb==OverworldCallback?2u:1u):0u));n->runFrame(n);
   if(f>300&&campaignFieldQuiescent(n)){entered=true;break;}}
  const auto saved=readProgress(fresh.c);
  requirePokemon(entered&&saved.flag(0x233)&&saved.flag(0x234)&&saved.flag(0x235)&&
                 saved.inventory.quantity(265)==1,"Bill ticket was lost after native save and fresh Continue");
 }
 return frames;
}
}
int main(int argc,char** argv) {
 QCoreApplication app(argc,argv);
 if(argc!=5){fprintf(stderr,"verify-firered-progression ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_OUTPUT_DIRECTORY rocket-door|tower-grunt|lostelle-full-berry|bill-full|bill-full-leader2\n");return 2;}
 try {
  const QString rom=argv[1],state=argv[2],out=argv[3],scenario=argv[4];requirePokemon(scenario=="rocket-door"||scenario=="tower-grunt"||scenario=="lostelle-full-berry"||scenario=="bill-full"||scenario=="bill-full-leader2","Unknown progression scenario");
  requirePokemon(fileHash(rom)==RomHash,"ROM fingerprint mismatch");QFile f(state);requirePokemon(f.open(QIODevice::ReadOnly),"Checkpoint unreadable");const auto checkpoint=f.readAll(),stateHash=fileHash(state);
  requirePokemon(QDir().mkdir(out),"Output must be a new directory");
  mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
  Core source,guest;source.open(rom,checkpoint);guest.open(rom,checkpoint);
  const auto m=readSaveMemory(guest.c);for(unsigned i=0;i<4;++i)guest.c->busWrite8(guest.c,m.block2+0xA+i,uint8_t(0xCAFEBABEu>>(8*i)));guest.c->busWrite8(guest.c,m.block2,0xBC);guest.c->busWrite8(guest.c,m.block2+1,0xFF);
  const bool lostelle=scenario=="lostelle-full-berry",bill=scenario.startsWith("bill-full");
  const auto frames=bill?(scenario.endsWith("leader2")?billTicketFullBag(guest,source,rom):billTicketFullBag(source,guest,rom)):
      lostelle?lostelleFullBerry(source,guest,rom):exercise(source,guest,rom,scenario=="tower-grunt");source.capture(out+"/source-complete.png");guest.capture(out+"/recipient-complete.png");
  requirePokemon(fileHash(rom)==RomHash&&fileHash(state)==stateHash,"Private inputs changed");
  const QJsonObject report{{"scenario",scenario},{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(stateHash)},
   {"authored_map_and_quest_prerequisites",true},{"authored_party_prerequisite",!bill},{"authored_encounter_flags_enemy_callback_or_result",false},
   {"original_native_continued_trainer_scene",!lostelle&&!bill},{"original_native_lostelle_hypno_and_full_berry_scene",lostelle},
   {"original_native_bill_ticket_dialogue",bill},{"recipient_full_key_bag_deferred_ticket_without_false_receipt",bill},
   {"deferred_ticket_delivered_once_after_space_freed",bill},{"both_owned_battle_inputs",!bill},
   {"recipient_was_on_loaded_native_map",true},{"recipient_loaded_native_map_updated",!bill},
   {"native_save_and_separate_fresh_continue_both_owners",true},{"recipient_bag_unchanged",!lostelle&&!bill},{"no_delayed_optional_berry_after_rescue_replay",lostelle},
   {"original_inputs_unchanged",true},{"production_launcher",false},{"frames",int(frames)}};
  QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot create report");const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Cannot save report");printf("%s",json.data());return 0;
 }catch(const std::exception& e){fprintf(stderr,"Progression acceptance failed: %s\n",e.what());return 1;}
}
