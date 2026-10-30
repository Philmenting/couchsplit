// SPDX-License-Identifier: GPL-3.0-or-later
// Private-ROM acceptance: authored map/party prerequisites, original native
// seven later Gym conversations, trainer parties, team battles and save UI.
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
#include <algorithm>
#include <set>
using namespace coop::firered;
namespace {
constexpr auto RomHash="eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
QByteArray fileHash(const QString& path) {
 QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly),"Input unreadable");QCryptographicHash h(QCryptographicHash::Sha256);
 requirePokemon(h.addData(&f),"Cannot hash input");return h.result().toHex();
}
struct Core {
 mCore* c=nullptr;bool initialized=false,configured=false;std::array<color_t,240*160> video{};
 ~Core(){if(c){if(configured)mCoreConfigDeinit(&c->config);if(initialized)c->deinit(c);else free(c);}}
 void open(const QString& rom,const QByteArray& state) {
  c=mCoreFind(rom.toUtf8().constData());requirePokemon(c&&c->init(c),"Cannot create core");initialized=true;
  mCoreInitConfig(c,nullptr);configured=true;mCoreOptions o{};o.useBios=false;o.skipBios=true;
  mCoreConfigLoadDefaults(&c->config,&o);mCoreLoadConfig(c);requirePokemon(mCoreLoadFile(c,rom.toUtf8().constData()),"Cannot load ROM");
  c->setVideoBuffer(c,video.data(),240);c->reset(c);requirePokemon(state.size()==qint64(c->stateSize(c))&&c->loadState(c,state.data()),"Checkpoint rejected");c->setKeys(c,0);c->runFrame(c);
 }
 void capture(const QString& path) {
  QImage im(240,160,QImage::Format_ARGB32);for(unsigned i=0;i<video.size();++i){auto p=video[i];im.setPixel(i%240,i/240,0xFF000000u|((p&255)<<16)|(p&0xFF00)|((p>>16)&255));}requirePokemon(im.save(path),"Cannot capture native screen");
 }
};
// Numeric metadata derived from pinned pret map_groups.json and each original
// Gym map.json leader object. Coordinates are authored positions BELOW the NPC,
// not a claim of navigating the campaign or its optional puzzles from Pallet.
struct GymMap {const char* name;unsigned group,num,x,y;};
constexpr std::array<GymMap,7> Maps{{
 {"misty",7,5,8,6},{"surge",9,6,5,2},{"erika",10,16,6,4},
 {"koga",11,3,7,13},{"sabrina",14,3,14,11},{"blaine",12,0,5,4},{"giovanni",5,1,2,2}
}};
void prepare(Core& owner,unsigned index,unsigned gymIndex,bool full,const QString& out) {
 const auto& gm=Maps[gymIndex];const auto& gym=CampaignGyms[gymIndex];auto before=readProgress(owner.c),p=before;
 p.var(0x4031,index%3);p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
 p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);
 for(unsigned flag=0x820;flag<gym.badge;++flag)p.flag(flag,true);
 p.flag(gym.badge,false);p.flag(gym.defeated,false);p.flag(0x500+gym.trainer,false);p.flag(gym.receipt,false);
 for(unsigned trainer:CampaignGymTrainers[gymIndex])if(trainer)p.flag(0x500+trainer,false);
 if(gymIndex==0)p.flag(0x500+150,true); // Authored Diana victory avoids immediate map-entry sight encounter.
 if(gymIndex==1)p.flag(0x264,index==0); // Guest's loaded beams deliberately CLOSED.
 if(gymIndex==5)for(unsigned flag:{0x265u,0x267u,0x268u,0x269u,0x26Au,0x26Bu})p.flag(flag,false);
 if(gymIndex==6){p.flag(0x055,false);p.var(0x405A,1);}
 p.inventory.setQuantity(gym.machine,index==1&&full?999:0);p.inventory.setQuantity(TMCaseItem,1);p.inventory.setQuantity(13,index?9:7);
 writeProgressDelta(owner.c,before,p);
 const auto identity=readTrainerIdentity(owner.c);
 Party party;party.count=1;party.mons[0]=acceptance::brockFixtureMon(index?48:24,identity.id);
 // Native obedience also checks the original trainer name, not only the ID.
 // A checkpoint with another name otherwise makes the authored level-100
 // fixture behave as traded and may stall against the early Gym leaders.
 std::copy(identity.name.begin(),identity.name.end(),party.mons[0].bytes.begin()+20);
 auto& mon=party.mons[0];auto secure=mon.secure();monPut32(secure.data()+4,1000000);mon.setSecure(secure);mon.bytes[84]=100;
 monPut16(mon.bytes.data()+86,999);monPut16(mon.bytes.data()+88,999);for(unsigned off=90;off<100;off+=2)monPut16(mon.bytes.data()+off,999);mon.validate();writeParty(owner.c,party);
 writeWallet(owner.c,index?5000:1000);
 const auto mem=readSaveMemory(owner.c);const unsigned playerX=gymIndex==2?gm.x:gm.x+index;
 const unsigned playerY=gymIndex==2?gm.y+1+index:gm.y+1;
 inventoryWrite16(owner.c,mem.block1,playerX);inventoryWrite16(owner.c,mem.block1+2,playerY);
 owner.c->busWrite8(owner.c,mem.block1+4,gm.group);owner.c->busWrite8(owner.c,mem.block1+5,gm.num);
 owner.c->busWrite8(owner.c,0x03003478,0);owner.c->busWrite32(owner.c,0x03003044,0x0805673D);
 for(unsigned n=0;n<240;++n){owner.c->setKeys(owner.c,0);owner.c->runFrame(owner.c);}
 owner.capture(out+"/entry-"+QString::number(index)+".png");
 if(!campaignFieldQuiescent(owner.c))fprintf(stderr,"entry owner=%u callback=%08X script=%u lock=%u state=%u tile=%u,%u\n",index,inventoryRead32(owner.c,MainCallbackAddress),owner.c->busRead8(owner.c,GlobalScriptStatusAddress),owner.c->busRead8(owner.c,FieldControlsLockAddress),owner.c->busRead8(owner.c,0x03003478),inventoryRead16(owner.c,readSaveMemory(owner.c).block1),inventoryRead16(owner.c,readSaveMemory(owner.c).block1+2));
 requirePokemon(campaignFieldQuiescent(owner.c),"Native authored Gym map entry did not become idle");
}
// Discover the native VMap layout structurally in paused RAM: dimensions equal
// the original live ROM map plus its documented border and map points to EWRAM.
// No engine address or Nintendo map bytes are bundled in this acceptance tool.
struct Grid {uint32_t at=0;unsigned width=0,height=0;};
Grid nativeGrid(mCore* c) {
 const auto layout=inventoryRead32(c,CampaignMapHeaderAddress);const unsigned w=inventoryRead32(c,layout)+15,h=inventoryRead32(c,layout+4)+14;
 std::set<uint32_t> found;
 for(const auto& range:std::array<std::pair<uint32_t,uint32_t>,2>{{{0x03000000,0x03008000},{0x02000000,0x02040000}}})
  for(uint32_t at=range.first;at+12<=range.second;at+=4) {
   if(inventoryRead32(c,at)!=w||inventoryRead32(c,at+4)!=h)continue;const auto map=inventoryRead32(c,at+8);
   if(map>=0x02000000&&map<=0x02040000-w*h*2)found.insert(map);
  }
 requirePokemon(found.size()==1,"Native Gym grid layout is ambiguous or absent");return {*found.begin(),w,h};
}
std::vector<uint16_t> gridBlocks(mCore* c,const Grid& grid) {
 std::vector<uint16_t> result(grid.width*grid.height);for(unsigned i=0;i<result.size();++i)result[i]=inventoryRead16(c,grid.at+2*i);return result;
}
void requireNativeDoors(mCore* c,const Grid& grid) {
 const auto p=readProgress(c);const auto scripts=inventoryRead32(c,CampaignMapHeaderAddress+8);
 const auto effects=campaign_runtime_detail::mapEffects(c,scripts,p);
 for(const auto& entry:effects.tiles) {
  const auto& tile=entry.second;const auto x=entry.first.first,y=entry.first.second;
  const auto value=inventoryRead16(c,grid.at+2*((y+7)*grid.width+x+7));
  // Native SetMetatile sets the supplied ID and requested collision bit,
  // retaining the current grid elevation. These are metadata comparisons.
  requirePokemon((value&0x3FFu)==(tile.value&0x3FFu),"Native Gym door metatile did not match its original script");
  requirePokemon((value&0xC00u)==(tile.impassable?0xC00u:(tile.value&0xC00u)),"Native Gym door collision did not match its original script");
 }
}
}
int main(int argc,char** argv) {
 QCoreApplication app(argc,argv);
 if(argc!=5){fprintf(stderr,"verify-firered-gyms ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_OUTPUT_DIRECTORY misty|surge|erika|koga|sabrina|blaine|giovanni|misty-full\n");return 2;}
 try {
  const QString rom=argv[1],state=argv[2],out=argv[3],scenario=argv[4];requirePokemon(fileHash(rom)==RomHash,"ROM fingerprint mismatch");
  QFile f(state);requirePokemon(f.open(QIODevice::ReadOnly),"Checkpoint unreadable");const auto bytes=f.readAll(),stateHash=fileHash(state);requirePokemon(QDir().mkdir(out),"Output directory must be new");
  unsigned gymIndex=7;const bool full=scenario=="misty-full";for(unsigned i=0;i<Maps.size();++i)if(scenario==Maps[i].name||(i==0&&full))gymIndex=i;requirePokemon(gymIndex<7,"Unknown Gym scenario");const auto& gym=CampaignGyms[gymIndex];
  mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
  Core source,guest;source.open(rom,bytes);guest.open(rom,bytes);
  const auto guestMemory=readSaveMemory(guest.c);for(unsigned i=0;i<4;++i)guest.c->busWrite8(guest.c,guestMemory.block2+0xA+i,uint8_t(0xCAFEBABEu>>(8*i)));
  guest.c->busWrite8(guest.c,guestMemory.block2,0xBC);guest.c->busWrite8(guest.c,guestMemory.block2+1,0xFF);
  prepare(source,0,gymIndex,full,out);prepare(guest,1,gymIndex,full,out);source.capture(out+"/source-before.png");guest.capture(out+"/guest-before.png");
  const auto grid=nativeGrid(guest.c);const auto oldGrid=gridBlocks(guest.c,grid);
  const std::array<PartyMon,2> originals{{readParty(source.c).mons[0],readParty(guest.c).mons[0]}};
  TeamBattle pair({{source.c,guest.c}});CampaignProgress campaign({{source.c,guest.c}});
  bool joined=false,done=false,sawDeferred=false;unsigned completed=0,frames=0,input1=0,input2=0,deliveries=0;
  campaign.event=[&](unsigned owner,const CampaignApplication& change){++deliveries;if(owner==1&&change.deferredRewards)sawDeferred=true;};
  campaign.deferred=[](unsigned owner,const std::string& reason){fprintf(stderr,"campaign deferred player=%u: %s\n",owner,reason.c_str());};
  pair.completed=[&](unsigned leader,unsigned trainer,unsigned outcome,bool peerNative){requirePokemon(leader==0&&trainer==gym.trainer&&outcome==1&&!peerNative,"Native Gym win callback did not match encounter");++completed;};
  for(unsigned n=0;n<12;++n){source.c->setKeys(source.c,0x40);source.c->runFrame(source.c);}for(unsigned n=0;n<30;++n){source.c->setKeys(source.c,0);source.c->runFrame(source.c);}
  for(;frames<24000;++frames) {
   if(!pair.active()) {
    campaign.sample();pair.sampleWorld();
    if(inventoryRead32(source.c,MainCallbackAddress)==InitBattleCallback) {
     fprintf(stdout,"native Gym flags=%08X trainer=%u\n",inventoryRead32(source.c,BattleFlagsAddress),inventoryRead16(source.c,0x020386AE));fflush(stdout);
     requirePokemon(!joined&&inventoryRead16(source.c,0x020386AE)==gym.trainer&&pair.tryBegin(0),"Original Gym NPC did not start the expected shared trainer battle");joined=true;
    }
   }
   if(pair.active()) {input1+=pair.inputOwner()==0;input2+=pair.inputOwner()==1;const unsigned pulse=frames%30<3?1u:0u;pair.runBattleFrame({{pulse,pulse}});}
   else {source.c->setKeys(source.c,frames%30<3?1u:0u);source.c->runFrame(source.c);guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);campaign.sample();}
   if(joined&&!pair.active()&&campaignFieldQuiescent(source.c)&&campaignFieldQuiescent(guest.c)) {
    const auto a=readProgress(source.c),b=readProgress(guest.c);
    if(a.flag(gym.badge)&&a.flag(gym.receipt)&&b.flag(gym.badge)&&(b.flag(gym.receipt)||full)){done=true;break;}
   }
  }
  fprintf(stdout,"Gym proof frames=%u joined=%u wins=%u owned=%u/%u deliveries=%u\n",frames,joined,completed,input1,input2,deliveries);fflush(stdout);
  if(!done) {
   source.capture(out+"/unfinished-source.png");guest.capture(out+"/unfinished-guest.png");
   fprintf(stderr,"Gym unfinished: active=%u input=%d callback=%08X outcome=%u\n",pair.active(),pair.inputOwner(),
       inventoryRead32(source.c,MainCallbackAddress),source.c->busRead8(source.c,OutcomeAddress));
  }
  requirePokemon(done&&completed==1&&input1&&input2,"Original Gym dialogue/battle/postscene or shared own badge failed");
  if(full) {
   auto p=readProgress(guest.c);requirePokemon(sawDeferred&&!p.flag(gym.receipt)&&p.inventory.quantity(gym.machine)==999,"Full guest TM stack did not preserve a deferred receipt");
   auto bag=readInventory(guest.c);bag.setQuantity(gym.machine,998);writeInventory(guest.c,bag); // Explicit authored space-release fixture.
   for(unsigned n=0;n<240;++n){campaign.sample();source.c->setKeys(source.c,0);guest.c->setKeys(guest.c,0);source.c->runFrame(source.c);guest.c->runFrame(guest.c);}
  }
  unsigned changedBlocks=0;const auto newGrid=gridBlocks(guest.c,grid);for(unsigned i=0;i<newGrid.size();++i)changedBlocks+=newGrid[i]!=oldGrid[i];
  if(gymIndex==1||gymIndex==5)requirePokemon(changedBlocks>0,"Already loaded guest Gym doors did not update");
  requireNativeDoors(guest.c,grid);
  for(unsigned owner=0;owner<2;++owner) {
   auto* core=owner?guest.c:source.c;core->setKeys(core,0);const auto p=readProgress(core);const auto party=readParty(core);
   requirePokemon(p.flag(gym.badge)&&p.flag(gym.defeated)&&p.flag(gym.receipt)&&p.flag(0x500+gym.trainer),"Own Gym badge/trainer/TM receipt missing");
   requirePokemon(p.inventory.quantity(gym.machine)==(owner&&full?999u:1u)&&p.inventory.quantity(TMCaseItem)==1&&p.inventory.quantity(13)==(owner?9:7),"Own machine or unrelated inventory incorrect");
   requirePokemon(party.count==1&&party.mons[0].identity()==originals[owner].identity(),"Shared Gym replaced own Pokemon identity");
   for(unsigned trainer:CampaignGymTrainers[gymIndex])if(trainer)requirePokemon(p.flag(0x500+trainer),"Original Gym subordinate trainer receipt missing");
   const auto proof=acceptance::verifyNativeSaveReload(core,rom.toStdString());acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
   for(unsigned n=0;n<10000;++n){auto* c=fresh.c;const auto cb=inventoryRead32(c,MainCallbackAddress);c->setKeys(c,n<300?(n%120<3?8u:0u):(n%40<3?(cb==OverworldCallback?2u:1u):0u));c->runFrame(c);if(n>300&&campaignFieldQuiescent(c)){entered=true;break;}}
   const auto resumed=readProgress(fresh.c);
   requirePokemon(entered&&resumed.flag(gym.badge)&&resumed.flag(gym.defeated)&&resumed.flag(gym.receipt)&&resumed.flag(0x500+gym.trainer)&&resumed.inventory==p.inventory,"Own Gym receipts or inventory lost after native save/fresh CONTINUE");
   for(unsigned trainer:CampaignGymTrainers[gymIndex])if(trainer)requirePokemon(resumed.flag(0x500+trainer),"Subordinate Gym trainer receipt lost on native reboot");
   requireNativeDoors(fresh.c,nativeGrid(fresh.c));
  }
  source.capture(out+"/source-complete.png");guest.capture(out+"/guest-complete.png");requirePokemon(fileHash(rom)==RomHash&&fileHash(state)==stateHash,"Private ROM/checkpoint changed");
  QJsonObject report{{"scenario",scenario},{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(stateHash)},
   {"authored_quest_prerequisites_map_entry_and_level100_stats999_parties",true},{"authored_misty_picnicker_receipt_to_avoid_entry_sight_encounter",gymIndex==0},{"original_native_leader_dialogue_trainer_party_team_battle_and_reward_script",true},
   {"two_real_cores_and_two_owned_battle_inputs",true},{"production_campaign_planner_and_live_map_refresh",true},
   {"own_badge_tm_receipt_subordinate_trainers_party_and_inventory_persist_native_save_fresh_continue",true},
   {"guest_full_tm_stack_deferred_then_delivered_once",full},{"loaded_guest_gym_door_blocks_changed",int(changedBlocks)},
   {"native_door_tiles_and_collision_match_original_script",true},{"rom_and_checkpoint_unchanged",true},{"production_launcher",false},{"frames",int(frames)}};
  QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot write report");const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Report write failed");printf("%s",json.data());return 0;
 }catch(const std::exception& e){fprintf(stderr,"Gym acceptance failed: %s\n",e.what());return 1;}
}
