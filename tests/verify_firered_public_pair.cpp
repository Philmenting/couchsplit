// SPDX-License-Identifier: GPL-3.0-or-later
// Opt-in acceptance with caller-supplied ROM and independent public saves.
// Files are read only. The only authored gameplay setup is each map position;
// parties, money, progress, native encounters and rewards are not manufactured.
#include "../gba/FireRedBattle.h"
#include "../gba/FireRedCampaignSync.h"
#include "FireRedSaveAcceptance.h"
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
using namespace coop::firered;
namespace {
constexpr auto RomHash="eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
QByteArray hash(const QString& path) {
 QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly),"Input unreadable");QCryptographicHash h(QCryptographicHash::Sha256);
 requirePokemon(h.addData(&f),"Cannot hash input");return h.result().toHex();
}
std::vector<uint8_t> battery(const QString& path) {
 QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly)&&f.size()==131072,"Save is not a complete 128 KiB battery");
 auto b=f.readAll();requirePokemon(b.size()==131072,"Cannot read complete battery");
 std::vector<uint8_t> bytes(b.begin(),b.end());requirePokemon(bool(acceptance::save_detail::generation(bytes)),"Save has no valid generation");return bytes;
}
void boot(acceptance::save_detail::FreshCore& f,const QString& rom,const std::vector<uint8_t>& b) {
 f.open(rom.toStdString(),b);auto* c=f.c;
 for(unsigned n=0;n<12000;++n) {
  const auto cb=inventoryRead32(c,MainCallbackAddress);
  c->setKeys(c,n<300?(n%120<3?8u:0u):(n%60<3?(cb==OverworldCallback?2u:1u):0u));c->runFrame(c);
  if(n>300&&campaignFieldQuiescent(c)&&readWorldActor(c).valid) {
   for(unsigned k=0;k<60;++k){c->setKeys(c,0);c->runFrame(c);}return;
  }
 }
 throw std::runtime_error("Save did not continue into an idle overworld");
}
void enter(mCore* c,unsigned group,unsigned map,unsigned x,unsigned y) {
 const auto m=readSaveMemory(c);inventoryWrite16(c,m.block1,x);inventoryWrite16(c,m.block1+2,y);
 c->busWrite8(c,m.block1+4,group);c->busWrite8(c,m.block1+5,map);
 c->busWrite8(c,0x03003478,0);c->busWrite32(c,MainCallbackAddress,0x0805673D);
 for(unsigned n=0;n<240;++n){c->setKeys(c,0);c->runFrame(c);}
 requirePokemon(campaignFieldQuiescent(c)&&readWorldActor(c).valid,"Native map entry did not become idle");
}
void sameOwners(const Party& before,const Party& after) {
 requirePokemon(before.count==after.count,"Battle changed a player's party count");
 for(unsigned i=0;i<before.count;++i)
  requirePokemon(before.mons[i].identity()==after.mons[i].identity(),"Battle replaced or moved a player's Pokemon");
}
QJsonObject wild(const QString& rom,std::array<mCore*,2> c,unsigned leader) {
 fprintf(stderr,"wild: entering Route 1\n");
 for(auto* core:c)enter(core,3,19,14,26); // Authored Route 1 grass entry only.
 fprintf(stderr,"wild: snapshotting original teams and bags\n");
 std::array<Party,2> parties{{readParty(c[0]),readParty(c[1])}};
 std::array<Inventory,2> bags{{readInventory(c[0]),readInventory(c[1])}};
 std::array<StorageSnapshot,2> pc{{readStorage(c[0]),readStorage(c[1])}};
 std::array<uint32_t,2> wallets{{readWallet(c[0]),readWallet(c[1])}};
 std::array<BattleStatistics,2> stats{{readBattleStatistics(c[0]),readBattleStatistics(c[1])}};
 TeamBattle pair(c);unsigned wins=0,frames=0;std::array<unsigned,2> inputs{};bool joined=false,returned=false;
 pair.completed=[&](unsigned who,unsigned trainer,unsigned outcome,bool peerNative) {
  requirePokemon(who==leader&&trainer==0&&outcome==1&&!peerNative,"Native wild encounter did not commit one owned shared win");++wins;
 };
 for(;frames<16000;++frames) {
  if(!joined&&inventoryRead32(c[leader],MainCallbackAddress)==InitBattleCallback) {
   fprintf(stderr,"wild: encounter at frame %u\n",frames);
   requirePokemon(inventoryRead32(c[leader],BattleFlagsAddress)==0,"Expected an original ordinary wild encounter");
   pair.sampleWorld();requirePokemon(pair.tryBegin(leader),"Native wild encounter could not join the partner");joined=true;fprintf(stderr,"wild: both teams joined\n");
  }
  if(pair.active()) {
   const int owner=pair.inputOwner();if(owner>=0&&owner<2)++inputs[unsigned(owner)];
   const auto key=frames%30<3?1u:0u;pair.runBattleFrame({{key,key}});
  } else for(unsigned p=0;p<2;++p) {
   auto* core=c[p];if(!joined&&p==leader&&inventoryRead32(core,MainCallbackAddress)==InitBattleCallback)continue;
   const auto key=(!joined&&p==leader)?(frames%96<48?0x40u:0x80u):0u;core->setKeys(core,key);core->runFrame(core);
  }
  if(joined&&!pair.active()&&campaignFieldQuiescent(c[0])&&campaignFieldQuiescent(c[1])){returned=true;break;}
 }
 requirePokemon(joined&&returned&&wins==1&&inputs[0]&&inputs[1],"Native team wild battle did not finish with both owners' inputs");
 fprintf(stderr,"wild: battle completed at frame %u\n",frames);
 for(unsigned p=0;p<2;++p) {
  sameOwners(parties[p],readParty(c[p]));const auto afterBag=readInventory(c[p]);const auto afterPC=readStorage(c[p]);const auto afterWallet=readWallet(c[p]);
  if(!(afterBag==bags[p])||!(afterPC==pc[p])||afterWallet!=wallets[p]) {
   fprintf(stderr,"wild: player %u bag=%u PC=%u wallet=%u->%u\n",p+1,unsigned(afterBag==bags[p]),unsigned(afterPC==pc[p]),wallets[p],afterWallet);
   for(unsigned pocket=0;pocket<PocketCount;++pocket)for(unsigned slot=0;slot<PocketCapacities[pocket];++slot)
    if(!(afterBag.pockets[pocket][slot]==bags[p].pockets[pocket][slot]))
     fprintf(stderr,"wild: pocket %u slot %u item/quantity %u/%u -> %u/%u\n",pocket,slot,bags[p].pockets[pocket][slot].item,bags[p].pockets[pocket][slot].quantity,afterBag.pockets[pocket][slot].item,afterBag.pockets[pocket][slot].quantity);
  }
  requirePokemon(afterBag==bags[p]&&afterPC==pc[p]&&afterWallet==wallets[p],"Wild battle altered unrelated own bag, PC or money");
  requirePokemon(readBattleStatistics(c[p])==recordSharedBattle(stats[p],false),"Wild battle was not counted once for each player");
  acceptance::verifyNativeSaveReload(c[p],rom.toStdString());
 }
 return {{"scenario","native-wild"},{"leader",int(leader)},{"frames",int(frames)},{"owner_input_frames_1",int(inputs[0])},{"owner_input_frames_2",int(inputs[1])},
  {"authored_map_entry_only",true},{"original_parties_and_native_encounter",true},{"one_shared_win",true},{"own_party_bag_pc_wallet_preserved",true},{"both_native_saves_and_fresh_continue",true}};
}
QJsonObject brock(const QString& rom,std::array<mCore*,2> c) {
 const auto firstProgress=readProgress(c[0]);
 requirePokemon(!firstProgress.flag(FlagBoulderBadge)&&!firstProgress.flag(FlagBrockDefeated),"Player one has already defeated Brock");
 for(auto* core:c)enter(core,6,2,6,6); // Authored direct entry only.
 std::array<Party,2> parties{{readParty(c[0]),readParty(c[1])}};
 const auto guestBag=readInventory(c[1]);
 std::array<StorageSnapshot,2> pc{{readStorage(c[0]),readStorage(c[1])}};
 std::array<uint32_t,2> wallets{{readWallet(c[0]),readWallet(c[1])}};
 std::array<BattleStatistics,2> stats{{readBattleStatistics(c[0]),readBattleStatistics(c[1])}};
 requirePokemon(parties[1].mons[0].move(1)==70&&parties[1].mons[0].pp(1)>0,"Player two needs its original second move Strength");
 const auto originalPrize=nativeTrainerPrize(c[0],414,8,1);
 TeamBattle pair(c);unsigned wins=0,frames=0;std::array<unsigned,2> inputs{};bool joined=false,finished=false;int announced=-1;
 pair.event=[&](const char* text,int amount){if(QString::fromLatin1(text)=="native trainer money announced")announced=amount;};
 pair.completed=[&](unsigned leader,unsigned trainer,unsigned outcome,bool peerNative){
  requirePokemon(leader==0&&trainer==414&&outcome==1&&!peerNative,"Original Brock encounter did not commit a shared trainer win");++wins;
 };
 for(unsigned n=0;n<12;++n){c[0]->setKeys(c[0],0x40);c[0]->runFrame(c[0]);}
 for(unsigned n=0;n<30;++n){c[0]->setKeys(c[0],0);c[0]->runFrame(c[0]);}
 for(;frames<30000;++frames) {
  if(!pair.active()&&!joined&&inventoryRead32(c[0],MainCallbackAddress)==InitBattleCallback) {
   requirePokemon(inventoryRead16(c[0],TrainerAddress)==414,"Native NPC did not start Brock's encounter");
   pair.sampleWorld();requirePokemon(pair.tryBegin(0),"Brock's original encounter could not join the partner");joined=true;
  }
  if(pair.active()) {
   const int owner=pair.inputOwner();if(owner>=0&&owner<2)++inputs[unsigned(owner)];
   const auto pulse=frames%30<3;std::array<uint32_t,2> keys{{pulse?1u:0u,pulse?1u:0u}};
   // Raichu's first move cannot hit Brock's Ground-type Pokemon. In the
   // original move menu, select its unmodified second move with RIGHT then A.
   if(owner==1&&(inventoryRead32(c[0],ControllerExecAddress)&4u)&&
      c[0]->busRead8(c[0],ControllerBufferA+0x400)==20&&c[0]->busRead8(c[0],0x02023FFE)==0)
    keys[1]=pulse?16u:0u;
   pair.runBattleFrame(keys);
  } else {
   c[0]->setKeys(c[0],frames%30<3?1u:0u);c[0]->runFrame(c[0]);c[1]->setKeys(c[1],0);c[1]->runFrame(c[1]);
  }
  if(joined&&!pair.active()&&campaignFieldQuiescent(c[0])&&campaignFieldQuiescent(c[1])&&
     storyEventComplete(readProgress(c[0]),StoryEvent::Brock)){finished=true;break;}
 }
 fprintf(stderr,"brock: frame=%u joined=%u wins=%u input=%u/%u\n",frames,unsigned(joined),wins,inputs[0],inputs[1]);
 requirePokemon(finished&&wins==1&&inputs[0]&&inputs[1],"Original Brock conversation and shared battle did not finish");
 const auto shares=splitBattleRewards(originalPrize,0,0);const auto expected=applyBattleRewards(wallets,shares);
 for(unsigned p=0;p<2;++p) {
  fprintf(stderr,"brock: verifying player %u native save\n",p+1);
  sameOwners(parties[p],readParty(c[p]));requirePokemon(readStorage(c[p])==pc[p],"Trainer battle changed own PC");
  requirePokemon(readWallet(c[p])==expected[p],"Trainer prize did not reach the correct owner");
  requirePokemon(readBattleStatistics(c[p])==recordSharedBattle(stats[p],true),"Trainer battle was not counted once for each owner");
  acceptance::verifyNativeSaveReload(c[p],rom.toStdString());
 }
 requirePokemon(readInventory(c[1])==guestBag,"Trainer battle changed partner's bag");
 requirePokemon(announced==int(shares.shares[0]),"Native reward text did not announce the initiator's share");
 return {{"scenario","native-brock"},{"frames",int(frames)},{"owner_input_frames_1",int(inputs[0])},{"owner_input_frames_2",int(inputs[1])},
  {"authored_map_entry_only",true},{"automatic_progress_sync_delayed_for_this_trainer_test",true},
  {"original_parties_and_native_trainer_encounter",true},{"native_trainer_prize",int(originalPrize)},
  {"player_1_before",int(wallets[0])},{"player_2_before",int(wallets[1])},{"player_1_after",int(expected[0])},{"player_2_after",int(expected[1])},
  {"native_announcement",announced},{"one_shared_win",true},{"own_parties_and_pc_preserved",true},{"both_native_saves_and_fresh_continue",true}};
}
QJsonObject rivalSync(const QString& rom,std::array<mCore*,2> c) {
 const std::array<Progress,2> start{{readProgress(c[0]),readProgress(c[1])}};
 requirePokemon(campaignReady(start[0])&&campaignReady(start[1]) && start[0].var(0x4054)==2 && start[1].var(0x4054)==1,
                "Rival-sync inputs are not the before/after early Route 22 saves");
 bool sourceWin=false;for(unsigned id=329;id<=331;++id)sourceWin|=start[0].flag(0x500+id);
 requirePokemon(sourceWin,"Completed Route 22 save lacks its native rival win receipt");
 const unsigned guestFlag=0x500+329+2-start[1].var(0x4031);
 const std::array<Party,2> parties{{readParty(c[0]),readParty(c[1])}};
 const std::array<StorageSnapshot,2> boxes{{readStorage(c[0]),readStorage(c[1])}};
 const std::array<Inventory,2> bags{{readInventory(c[0]),readInventory(c[1])}};
 const std::array<uint32_t,2> wallets{{readWallet(c[0]),readWallet(c[1])}};
 unsigned events=0;CampaignProgress campaign(c);campaign.event=[&](unsigned,const CampaignApplication&){++events;};
 for(unsigned n=0;n<1200;++n){campaign.sample();for(auto* core:c){core->setKeys(core,0);core->runFrame(core);}}
 const auto guest=readProgress(c[1]);
 requirePokemon(events==1 && guest.var(0x4054)==2 && guest.flag(guestFlag),"Route 22 victory did not reach the guest's own rival variant");
 for(unsigned player=0;player<2;++player)
  requirePokemon(readParty(c[player])==parties[player]&&readStorage(c[player])==boxes[player]&&
                 readInventory(c[player])==bags[player]&&readWallet(c[player])==wallets[player],
                 "Route 22 sync changed a personal team, PC, bag or wallet");
 std::array<std::vector<uint8_t>,2> batteries;
 for(unsigned player=0;player<2;++player)batteries[player]=acceptance::verifyNativeSaveReload(c[player],rom.toStdString()).battery;
 acceptance::save_detail::FreshCore first,second;boot(first,rom,batteries[0]);boot(second,rom,batteries[1]);
 std::array<mCore*,2> restarted{{first.c,second.c}};const auto reloaded=readProgress(restarted[1]);
 requirePokemon(reloaded.var(0x4054)==2 && reloaded.flag(guestFlag),"Guest rival victory did not survive native save and fresh Continue");
 unsigned repeated=0;CampaignProgress again(restarted);again.event=[&](unsigned,const CampaignApplication&){++repeated;};
 for(unsigned n=0;n<600;++n){again.sample();for(auto* core:restarted){core->setKeys(core,0);core->runFrame(core);}}
 requirePokemon(repeated==0 && readWallet(restarted[0])==wallets[0] && readWallet(restarted[1])==wallets[1],
                "Restart repeated a rival sync or changed a wallet");
 return {{"scenario","route22-rival-public-saves"},{"guest_own_rival_variant_and_stage",true},
         {"personal_team_pc_bag_and_wallet_preserved",true},{"both_native_saves_and_fresh_continue",true},
         {"no_repeat_after_restart",true}};
}
QJsonObject sync(const QString& rom,std::array<mCore*,2> c) {
 std::array<Party,2> before{{readParty(c[0]),readParty(c[1])}};
 std::array<StorageSnapshot,2> pc{{readStorage(c[0]),readStorage(c[1])}};
 std::array<uint32_t,2> wallets{{readWallet(c[0]),readWallet(c[1])}};
 const std::array<Progress,2> start{{readProgress(c[0]),readProgress(c[1])}};
 unsigned events=0,storyEvents=0,defers=0;std::string reason;StoryProgress story(c);CampaignProgress campaign(c);
 story.event=[&](StoryEvent,unsigned,bool){++storyEvents;};
 campaign.event=[&](unsigned,const CampaignApplication&){++events;};campaign.deferred=[&](unsigned,const std::string& why){++defers;reason=why;};
 for(unsigned n=0;n<1200;++n){story.sample();campaign.sample();for(auto* core:c){core->setKeys(core,0);core->runFrame(core);}}
 const std::array<Progress,2> finish{{readProgress(c[0]),readProgress(c[1])}};
 if(campaignReady(start[0])&&campaignReady(start[1])) {
  if(start[1].flag(FlagBoulderBadge)&&start[1].flag(FlagBrockDefeated))
   requirePokemon(finish[0].flag(FlagBoulderBadge),"Brock badge did not synchronize");
  if(start[1].flag(0x827)&&start[1].flag(0x4B7))
   requirePokemon(finish[0].flag(0x827),"Eighth badge did not synchronize");
 }
 for(unsigned p=0;p<2;++p){requirePokemon(readParty(c[p])==before[p]&&readStorage(c[p])==pc[p]&&readWallet(c[p])==wallets[p],"Campaign sync changed a player's party, PC or wallet");}
 QJsonObject result{{"scenario","different-public-progress"},{"ready_1",campaignReady(start[0])},{"ready_2",campaignReady(start[1])},
  {"badge_1_before",int(start[0].flag(FlagBoulderBadge))},{"badge_1_after",int(finish[0].flag(FlagBoulderBadge))},
  {"badge_8_before",int(start[0].flag(0x827))},{"badge_8_after",int(finish[0].flag(0x827))},
  {"committed_changes",int(events)},{"story_events",int(storyEvents)},{"deferred_reasons",int(defers)},{"last_deferred_reason",QString::fromStdString(reason)},
  {"own_party_pc_and_wallet_preserved",true}};
 // The two players stay on their original maps for this test. A normal native
 // save may be unavailable there; do not infer a passing save when it is not.
 std::array<std::vector<uint8_t>,2> saved;
 for(unsigned p=0;p<2;++p) {
  const auto proof=acceptance::verifyNativeSaveReload(c[p],rom.toStdString());
  saved[p]=proof.battery;
  acceptance::save_detail::FreshCore fresh;boot(fresh,rom,proof.battery);const auto reloaded=readProgress(fresh.c);
  for(unsigned flag=0x820;flag<=0x827;++flag)requirePokemon(reloaded.flag(flag)==finish[p].flag(flag),"Synchronized badge did not survive fresh native CONTINUE");
 }
 acceptance::save_detail::FreshCore first,second;boot(first,rom,saved[0]);boot(second,rom,saved[1]);
 std::array<mCore*,2> restarted{{first.c,second.c}};
 std::array<Inventory,2> reloadedBags{{readInventory(restarted[0]),readInventory(restarted[1])}};
 unsigned again=0;StoryProgress afterStory(restarted);CampaignProgress afterCampaign(restarted);
 afterStory.event=[&](StoryEvent,unsigned,bool){++again;};afterCampaign.event=[&](unsigned,const CampaignApplication&){++again;};
 for(unsigned n=0;n<600;++n){afterStory.sample();afterCampaign.sample();for(auto* core:restarted){core->setKeys(core,0);core->runFrame(core);}}
 for(unsigned p=0;p<2;++p)requirePokemon(readInventory(restarted[p])==reloadedBags[p],"Restart repeated a campaign reward");
 requirePokemon(again==0,"Restart unexpectedly repeated a completed campaign delivery");
 result.insert("no_repeated_rewards_after_both_restart",true);
 result.insert("both_native_saves_and_fresh_continue",true);return result;
}
QJsonObject questlog(const QString& rom,std::array<mCore*,2> c) {
 // Player one has continued; player two is still booting their own public
 // battery. Use the original game controls to skip each native recap scene.
 StoryProgress story(c);CampaignProgress campaign(c);unsigned events=0,historicalEvents=0,playbackFrames=0,stable=0,frames=0;
 auto event=[&] {++events;if(!questLogWorldAvailable(c[0])||!questLogWorldAvailable(c[1]))++historicalEvents;};
 story.event=[&](StoryEvent,unsigned,bool){event();};campaign.event=[&](unsigned,const CampaignApplication&){event();};
 for(;frames<15000&&stable<600;++frames) {
  if(!questLogWorldAvailable(c[1])) {
   ++playbackFrames;requirePokemon(!progressFieldReady(c[1])&&!readWorldActor(c[1]).valid,"Quest-log playback exposed a live player");
  }
  story.sample();campaign.sample();
  c[0]->setKeys(c[0],0);c[0]->runFrame(c[0]);
  const auto cb=inventoryRead32(c[1],MainCallbackAddress);
  c[1]->setKeys(c[1],frames<300?(frames%120<3?8u:0u):(frames%60<3?(cb==OverworldCallback?2u:1u):0u));c[1]->runFrame(c[1]);
  if(frames>300&&campaignFieldQuiescent(c[0])&&campaignFieldQuiescent(c[1]))++stable;else stable=0;
 }
 requirePokemon(playbackFrames&&stable==600&&historicalEvents==0,"Quest-log replay was not isolated from live co-op updates");
 requirePokemon(readTrainerIdentity(c[0]).id!=readTrainerIdentity(c[1]).id,"Profiles need distinct original trainer identities");
 const auto one=readProgress(c[0]),two=readProgress(c[1]);
 requirePokemon(one.flag(FlagBoulderBadge)&&one.flag(0x827)&&two.flag(FlagBoulderBadge)&&two.flag(0x827),"Live progress did not synchronize after replay finished");
 for(auto* core:c){core->setKeys(core,0);acceptance::verifyNativeSaveReload(core,rom.toStdString());}
 return {{"scenario","native-quest-log-replay"},{"frames",int(frames)},{"playback_frames",int(playbackFrames)},
  {"sync_events_during_playback",int(historicalEvents)},{"sync_events_after_playback",int(events)},
  {"historical_world_not_presented_or_synchronized",true},{"real_badges_synchronized_after_playback",true},{"both_native_saves_and_fresh_continue",true}};
}
}
int main(int argc,char** argv) {
 QCoreApplication app(argc,argv);
 if(argc!=5){fprintf(stderr,"verify-firered-public-pair ORIGINAL_DE.gba PLAYER1.sav PLAYER2.sav wild-0|wild-1|brock|sync|rival-sync|questlog\n");return 2;}
 try {
  const QString rom=argv[1],a=argv[2],b=argv[3],scenario=argv[4];requirePokemon(hash(rom)==RomHash,"Exact German FireRed ROM required");
  const auto hashA=hash(a),hashB=hash(b);const auto saveA=battery(a),saveB=battery(b);
  mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
  acceptance::save_detail::FreshCore first,second;boot(first,rom,saveA);
  if(scenario=="questlog")second.open(rom.toStdString(),saveB);else boot(second,rom,saveB);
  std::array<mCore*,2> cores{{first.c,second.c}};
  if(scenario!="questlog")requirePokemon(readTrainerIdentity(cores[0]).id!=readTrainerIdentity(cores[1]).id,"Profiles need distinct original trainer identities");
  QJsonObject out;if(scenario=="wild-0"||scenario=="wild-1")out=wild(rom,cores,scenario.right(1).toUInt());
  else if(scenario=="brock")out=brock(rom,cores);else if(scenario=="sync")out=sync(rom,cores);
  else if(scenario=="rival-sync")out=rivalSync(rom,cores);
  else if(scenario=="questlog")out=questlog(rom,cores);else throw std::runtime_error("Unknown scenario");
  requirePokemon(hash(rom)==RomHash&&hash(a)==hashA&&hash(b)==hashB,"Original input changed");
  out.insert("source_and_rom_unchanged",true);out.insert("player_1_save_sha256",QString::fromLatin1(hashA));out.insert("player_2_save_sha256",QString::fromLatin1(hashB));
  std::cout<<QJsonDocument(out).toJson().constData();return 0;
 }catch(const std::exception& e){fprintf(stderr,"Public paired acceptance failed: %s\n",e.what());return 1;}
}
