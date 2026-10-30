// SPDX-License-Identifier: GPL-3.0-or-later
// Private-ROM native campaign acceptance. Map/quest prerequisites are explicit
// fixtures; the original native Hall of Fame ceremony, saves and credits run.
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
void prepareChampionField(Core& owner,unsigned index,bool suppressEntrance=true) {
 auto before=readProgress(owner.c),p=before;
 p.var(0x4031,index%3);
 p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
 for(unsigned f=0x820;f<=0x827;++f)p.flag(f,true);
 // Native ON_RESUME sees the completed trainer receipt and suppresses a new
 // Champion entrance encounter. These are authored prerequisites, not a win.
 for(unsigned id:{438u,439u,440u,739u,740u,741u})p.flag(0x500+id,suppressEntrance);
 p.flag(0x4BC,suppressEntrance);p.flag(0x82C,false);p.flag(0x83B,false);
 writeProgressDelta(owner.c,before,p);
 const auto identity=readTrainerIdentity(owner.c);Party party;party.count=1;
 party.mons[0]=acceptance::brockFixtureMon(24+index*24,identity.id);party.mons[0].setHp(1);writeParty(owner.c,party);
 const auto m=readSaveMemory(owner.c);
 inventoryWrite16(owner.c,m.block1,6);inventoryWrite16(owner.c,m.block1+2,suppressEntrance?9:19);
 owner.c->busWrite8(owner.c,m.block1+4,1);owner.c->busWrite8(owner.c,m.block1+5,79);
 owner.c->busWrite8(owner.c,0x03003478,0);owner.c->busWrite32(owner.c,0x03003044,0x0805673D);
 for(unsigned f=0;f<240;++f){owner.c->setKeys(owner.c,0);owner.c->runFrame(owner.c);}
 if(suppressEntrance)requirePokemon(canStartNativeHallOfFameCompletion(owner.c),"Native Champion fixture did not become idle");
}
uint16_t checksum(const uint8_t* p,unsigned n) {
 uint32_t sum=0;for(unsigned i=0;i<n;i+=4)sum+=monU32(p+i);return uint16_t((sum>>16)+sum);
}
void requireOwnHallOfFame(const std::vector<uint8_t>& battery,const PartyMon& mon) {
 requirePokemon(battery.size()==0x20000,"Missing native Flash");
 for(unsigned sector=28;sector<30;++sector) {
  const auto* p=battery.data()+sector*0x1000;
  requirePokemon(monU32(p+0xFF8)==0x08012025 && monU16(p+0xFF4)==checksum(p,0xF80),"Native Hall of Fame sector checksum missing");
 }
 // Original HOF records use20bytes permon: trainerID,PID,species9/level7,nickname.
 const auto* first=battery.data()+28*0x1000;
 requirePokemon(monU32(first)==mon.identity().trainer && monU32(first+4)==mon.identity().personality,
                "Native Hall of Fame recorded the other player's identity");
 requirePokemon((monU16(first+8)&0x1FFu)==mon.species(),"Native Hall of Fame species mismatch");
}
unsigned verifyNativeMachineScene(Core& source,Core& guest,const QString& rom,const QString& scenario) {
 const unsigned receipt=scenario=="cut"?0x237u:scenario=="surf"?0x239u:0x23Au;
 const unsigned item=scenario=="cut"?339u:scenario=="surf"?341u:342u;
 const unsigned group=scenario=="strength"?11u:1u,number=scenario=="cut"?11u:scenario=="surf"?71u:7u;
 const unsigned x=scenario=="cut"?5u:scenario=="surf"?6u:3u,y=scenario=="cut"?5u:6u;
 for(auto* core:{source.c,guest.c}) {
  auto before=readProgress(core),p=before;p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);
  if(scenario=="strength"){p.flag(0x189,true);p.inventory.setQuantity(353,1);}
  writeProgressDelta(core,before,p);
 }
 auto memory=readSaveMemory(source.c);
 inventoryWrite16(source.c,memory.block1,x);inventoryWrite16(source.c,memory.block1+2,y);
 source.c->busWrite8(source.c,memory.block1+4,group);source.c->busWrite8(source.c,memory.block1+5,number);
 source.c->busWrite8(source.c,0x03003478,0);source.c->busWrite32(source.c,0x03003044,0x0805673D);
 for(unsigned i=0;i<240;++i){source.c->setKeys(source.c,0);source.c->runFrame(source.c);}
 requirePokemon(progressFieldReady(source.c),"Native machine map entry did not complete");
 for(unsigned i=0;i<12;++i){source.c->setKeys(source.c,0x40);source.c->runFrame(source.c);}
 for(unsigned i=0;i<30;++i){source.c->setKeys(source.c,0);source.c->runFrame(source.c);}
 unsigned frames=0,deliveries=0;
 CampaignProgress campaign({{source.c,guest.c}});
 campaign.event=[&](unsigned owner,const CampaignApplication&){requirePokemon(owner==1,"Machine reward unexpectedly changed source");++deliveries;};
 campaign.deferred=[](unsigned owner,const std::string& reason){fprintf(stderr,"campaign deferred player=%u: %s\n",owner,reason.c_str());};
 for(;frames<16000;++frames) {
  source.c->setKeys(source.c,frames%30<3?1u:0u);guest.c->setKeys(guest.c,0);
  source.c->runFrame(source.c);guest.c->runFrame(guest.c);
  if(campaignFieldQuiescent(source.c)&&campaignFieldQuiescent(guest.c)) {
   campaign.sample();
   if(readProgress(source.c).flag(receipt)&&readProgress(guest.c).flag(receipt))break;
  }
 }
 requirePokemon(frames<16000&&deliveries==1,"Original native machine dialogue or recipient delivery incomplete");
 for(auto* core:{source.c,guest.c}) {
  core->setKeys(core,0);for(unsigned n=0;n<180&&!progressFieldReady(core);++n)core->runFrame(core);
  const auto p=readProgress(core);requirePokemon(p.flag(receipt)&&p.inventory.quantity(item)==1&&p.inventory.quantity(TMCaseItem)==1,"Own machine/Case receipt missing");
  if(scenario=="strength")requirePokemon(p.inventory.quantity(353)==0,"Own Gold Teeth not consumed by Strength exchange");
  const auto proof=acceptance::verifyNativeSaveReload(core,rom.toStdString());
  acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned fno=0;fno<10000;++fno) {
   auto* c=fresh.c;const auto cb=inventoryRead32(c,0x03003044);
   c->setKeys(c,fno<300?(fno%120<3?8u:0u):(fno%40<3?(cb==0x080565D5?2u:1u):0u));c->runFrame(c);
   if(fno>300&&progressFieldReady(c)){entered=true;break;}
  }
  requirePokemon(entered&&readProgress(fresh.c).flag(receipt)&&readInventory(fresh.c).quantity(item)==1,"Own campaign machine receipt lost on native reboot");
 }
 return frames;
}
unsigned verifyNativeNationalDex(Core& source,Core& guest,const QString& rom) {
 for(auto* core:{source.c,guest.c}) {
  auto before=readProgress(core),p=before;p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);p.flag(0x82C,core==source.c);
  writeProgressDelta(core,before,p);
 }
 const auto ownDex=acceptance::save_detail::bytes(guest.c,readSaveMemory(guest.c).block2+0x28,0x68);
 // AUTHORED native-upgrade trigger: run the same original special as Oak's
 // scene, rather than fabricating any of the three National Dex receipt parts.
 constexpr std::array<uint8_t,6> code{{0x69,0x25,0x6F,0x01,0x6B,2}};
 constexpr auto ctx=GlobalScriptContextAddress;
 for(unsigned i=0;i<80;++i)source.c->busWrite8(source.c,ctx+12+i,i<code.size()?code[i]:0);
 for(unsigned i=0;i<12;++i)source.c->busWrite8(source.c,ctx+i,0);
 for(unsigned i=0;i<4;++i)source.c->busWrite8(source.c,ctx+8+i,uint8_t((ctx+12)>>(8*i)));
 source.c->busWrite8(source.c,ctx+1,1);source.c->busWrite8(source.c,FieldControlsLockAddress,1);source.c->busWrite8(source.c,GlobalScriptStatusAddress,0);
 for(unsigned n=0;n<90;++n){source.c->setKeys(source.c,0);source.c->runFrame(source.c);}
 requirePokemon(nativeNationalDexEnabled(source.c,readProgress(source.c)),"Original native National Dex special did not produce all three parts");
 CampaignProgress campaign({{source.c,guest.c}});campaign.sample();
 for(unsigned n=0;n<30;++n){source.c->runFrame(source.c);guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);campaign.sample();}
 requirePokemon(!nativeNationalDexEnabled(guest.c,readProgress(guest.c)),"National Dex bypassed own Hall-of-Fame requirement");
 auto before=readProgress(guest.c),p=before;p.flag(0x82C,true);writeProgressDelta(guest.c,before,p); // Explicit own-HOF prerequisite fixture.
 const auto sourceMemory=readSaveMemory(source.c);
 source.c->busWrite8(source.c,sourceMemory.block2+0x1B,0);
 campaign.sample();
 for(unsigned n=0;n<30;++n){source.c->runFrame(source.c);guest.c->runFrame(guest.c);campaign.sample();}
 requirePokemon(!nativeNationalDexEnabled(guest.c,readProgress(guest.c)),"An incomplete source National Dex receipt was accepted");
 source.c->busWrite8(source.c,sourceMemory.block2+0x1B,0xB9);
 unsigned frames=0;
 for(;frames<240;++frames){campaign.sample();source.c->runFrame(source.c);guest.c->runFrame(guest.c);if(nativeNationalDexEnabled(guest.c,readProgress(guest.c))&&progressFieldReady(guest.c))break;}
 requirePokemon(frames<240&&acceptance::save_detail::bytes(guest.c,readSaveMemory(guest.c).block2+0x28,0x68)==ownDex,"Own National Dex upgrade missing or replaced personal entries");
 for(auto* core:{source.c,guest.c}) {
  const auto proof=acceptance::verifyNativeSaveReload(core,rom.toStdString());
  acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned fno=0;fno<10000;++fno){auto* c=fresh.c;const auto cb=inventoryRead32(c,0x03003044);c->setKeys(c,fno<300?(fno%120<3?8u:0u):(fno%40<3?(cb==0x080565D5?2u:1u):0u));c->runFrame(c);if(fno>300&&progressFieldReady(c)){entered=true;break;}}
  requirePokemon(entered&&nativeNationalDexEnabled(fresh.c,readProgress(fresh.c)),"Three-part National Dex receipt lost after native save and fresh boot");
 }
 return frames;
}
void runOwnNativeProgram(mCore* c,const std::vector<uint8_t>& code) {
 requirePokemon(campaignFieldQuiescent(c)&&code.size()<=80,"Native acceptance program cannot start here");
 constexpr auto ctx=GlobalScriptContextAddress;
 for(unsigned i=0;i<80;++i)c->busWrite8(c,ctx+12+i,i<code.size()?code[i]:0);
 for(unsigned i=0;i<12;++i)c->busWrite8(c,ctx+i,0);
 for(unsigned i=0;i<4;++i)c->busWrite8(c,ctx+8+i,uint8_t((ctx+12)>>(8*i)));
 c->busWrite8(c,ctx+1,1);c->busWrite8(c,FieldControlsLockAddress,1);c->busWrite8(c,GlobalScriptStatusAddress,0);
 for(unsigned n=0;n<180;++n){c->setKeys(c,0);c->runFrame(c);if(campaignFieldQuiescent(c))return;}
 requirePokemon(false,"Native acceptance program did not finish");
}
unsigned verifyNativeNetwork(Core& source,Core& guest,const QString& rom,unsigned starter,const QString& location={}) {
 for(auto* core:{source.c,guest.c}) {
  auto before=readProgress(core),p=before;p.var(VarLabScene,9);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);p.flag(0x82C,true);p.flag(0x845,true);p.flag(0x846,true);p.flag(0x844,false);
  p.var(0x4031,core==guest.c?starter:(starter+1)%3);p.var(0x4076,5);p.inventory.setQuantity(368,1);p.inventory.setQuantity(374,1);
  writeProgressDelta(core,before,p);runOwnNativeProgram(core,{0x69,0x25,0x6F,1,0x6B,2});
 }
 const bool machineLoaded=location=="network-loaded",guardLoaded=location=="network-guard";
 unsigned guardId=0;
 auto activeGuard=[&] {
  for(unsigned i=0;i<16;++i) {
   const auto at=CampaignObjectsAddress+i*0x24;
   if((inventoryRead32(guest.c,at)&1u) && guest.c->busRead8(guest.c,at+8)==guardId &&
      guest.c->busRead8(guest.c,at+9)==3 && guest.c->busRead8(guest.c,at+10)==3)return true;
  }
  return false;
 };
 if(machineLoaded || guardLoaded) {
  const auto m=readSaveMemory(guest.c);
  inventoryWrite16(guest.c,m.block1,machineLoaded?8:2);inventoryWrite16(guest.c,m.block1+2,machineLoaded?8:14);
  guest.c->busWrite8(guest.c,m.block1+4,machineLoaded?32:3);guest.c->busWrite8(guest.c,m.block1+5,machineLoaded?0:3);
  guest.c->busWrite8(guest.c,0x03003478,0);guest.c->busWrite32(guest.c,0x03003044,0x0805673D);
  for(unsigned f=0;f<240;++f){guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);}
  requirePokemon(campaignFieldQuiescent(guest.c),"Native network recipient map did not become idle");
  if(guardLoaded) {
   const auto events=inventoryRead32(guest.c,CampaignMapHeaderAddress+4),templates=inventoryRead32(guest.c,events+4);
   for(unsigned i=0;i<guest.c->busRead8(guest.c,events);++i)
    if(inventoryRead16(guest.c,templates+i*0x18+0x14)==0x5C)guardId=guest.c->busRead8(guest.c,templates+i*0x18);
   requirePokemon(guardId && activeGuard(),"Native Cerulean guard was not initially active");
  }
 }
 const auto ownDex=acceptance::save_detail::bytes(guest.c,readSaveMemory(guest.c).block2+0x28,0x68);
 const auto sourceCommands=nativeNetworkCompletionCommands(source.c,readProgress(source.c));
 std::vector<uint8_t> sourceCode{0x69};sourceCode.insert(sourceCode.end(),sourceCommands.begin(),sourceCommands.end());sourceCode.insert(sourceCode.end(),{0x6B,2});
 runOwnNativeProgram(source.c,sourceCode); // Explicit authored source completion trigger, not a full Celio dialogue.
 CampaignProgress campaign({{source.c,guest.c}});
 unsigned frames=0;
 for(;frames<240;++frames){campaign.sample();source.c->runFrame(source.c);guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);if(readProgress(guest.c).flag(0x844)&&campaignFieldQuiescent(guest.c))break;}
 requirePokemon(frames<240,"Native shared network installation did not finish");
 if(machineLoaded) {
  const auto p=readProgress(guest.c);
  const auto effects=campaign_runtime_detail::mapEffects(guest.c,inventoryRead32(guest.c,CampaignMapHeaderAddress+8),p);
  const auto layout=inventoryRead32(guest.c,CampaignMapHeaderAddress),width=inventoryRead32(guest.c,layout),height=inventoryRead32(guest.c,layout+4);
  uint32_t grid=0;
  for(uint32_t at=0x03000000;at<0x03007FF4;at+=4)
   if(inventoryRead32(guest.c,at)==width+15 && inventoryRead32(guest.c,at+4)==height+14) {
    const auto pointer=inventoryRead32(guest.c,at+8);
    if(validSaveBlock(pointer,(width+15)*(height+14)*2)) {requirePokemon(!grid,"Ambiguous native network map grid");grid=pointer;}
   }
  requirePokemon(grid && effects.tiles.size()>=4,"Native network machine map entries missing");
  for(const auto& tile:effects.tiles) {
   const auto actual=inventoryRead16(guest.c,grid+((tile.first.second+7)*(width+15)+tile.first.first+7)*2)&0xFFF;
   const auto expected=tile.second.value|(tile.second.impassable?0xC00:0);
   requirePokemon(actual==expected,"Native completion left already loaded network machine tiles/collision stale");
  }
 }
 if(guardLoaded)requirePokemon(!activeGuard(),"Native completion left already loaded Cerulean Cave guard active");

 const std::array<unsigned,3> species{{244,243,245}};
 std::array<std::vector<uint8_t>,2> records;
 for(unsigned owner=0;owner<2;++owner) {
  auto* core=owner?guest.c:source.c;const auto m=readSaveMemory(core);const auto p=readProgress(core);const auto at=m.block1+0x30D0;
  requirePokemon(p.flag(0x844)&&p.flag(0x2DC)&&p.var(0x4076)>=6&&p.var(0x4076)<=7&&p.inventory.quantity(374)==0,"Native installation receipts or own Sapphire consumption missing");
  requirePokemon(core->busRead16(core,at+8)==species[p.var(0x4031)]&&core->busRead8(core,at+0x13)==1&&core->busRead8(core,at+0xC)==50&&core->busRead16(core,at+0xA)>0,
                 "Native roamer did not follow this owner's starter");
  requirePokemon((p.gcnLinkFlags&0x800Eu)==0x800Eu&&(core->busRead8(core,m.block2+9)&0x80u),"Original native postgame link/savewarp flags missing");
  records[owner]=acceptance::save_detail::bytes(core,at,0x1C);
 }
 requirePokemon(acceptance::save_detail::bytes(guest.c,readSaveMemory(guest.c).block2+0x28,0x68)==ownDex,"Native postgame installation changed personal dex entries");
 // Represent an already caught/fled roamer, then prove continuous sampling and
 // a new broker cannot reinitialize it from the source's completed flag.
 guest.c->busWrite8(guest.c,readSaveMemory(guest.c).block1+0x30D0+0x13,0);records[1][0x13]=0;
 CampaignProgress restarted({{source.c,guest.c}});
 for(unsigned n=0;n<30;++n){campaign.sample();restarted.sample();source.c->runFrame(source.c);guest.c->runFrame(guest.c);}
 for(unsigned owner=0;owner<2;++owner) {
  auto* core=owner?guest.c:source.c;requirePokemon(acceptance::save_detail::bytes(core,readSaveMemory(core).block1+0x30D0,0x1C)==records[owner],"Completed installation replayed and replaced an own roamer");
  const auto proof=acceptance::verifyNativeSaveReload(core,rom.toStdString());acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned fno=0;fno<10000;++fno){auto* c=fresh.c;const auto cb=inventoryRead32(c,0x03003044);c->setKeys(c,fno<300?(fno%120<3?8u:0u):(fno%40<3?(cb==0x080565D5?2u:1u):0u));c->runFrame(c);if(fno>300&&progressFieldReady(c)){entered=true;break;}}
  requirePokemon(entered&&readProgress(fresh.c).flag(0x844)&&readProgress(fresh.c).var(0x4076)>=6&&readProgress(fresh.c).var(0x4076)<=7&&acceptance::save_detail::bytes(fresh.c,readSaveMemory(fresh.c).block1+0x30D0,0x1C)==records[owner],"Own native roaming record/installation receipt lost on fresh boot");
 }
 return frames;
}
unsigned verifyNativeMarowak(Core& source,Core& guest,const QString& rom) {
 for(unsigned owner=0;owner<2;++owner) {
  auto* core=owner?guest.c:source.c;auto before=readProgress(core),p=before;
  p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);p.inventory.setQuantity(359,1);
  writeProgressDelta(core,before,p);Party party;party.count=1;party.mons[0]=acceptance::brockFixtureMon(24+owner*24,readTrainerIdentity(core).id);writeParty(core,party);
  const auto m=readSaveMemory(core);inventoryWrite16(core,m.block1,owner?10:11);inventoryWrite16(core,m.block1+2,14);
  core->busWrite8(core,m.block1+4,1);core->busWrite8(core,m.block1+5,93);core->busWrite8(core,0x03003478,0);core->busWrite32(core,0x03003044,0x0805673D);
  for(unsigned f=0;f<240;++f){core->setKeys(core,0);core->runFrame(core);}
  requirePokemon(progressFieldReady(core),"Native Tower6F map entry did not become idle");
 }
 TeamBattle pair({{source.c,guest.c}});CampaignProgress campaign({{source.c,guest.c}});bool joined=false,done=false;
 unsigned frames=0,input1=0,input2=0;
 for(;frames<18000;++frames) {
  if(!pair.active()) {
   campaign.sample();pair.sampleWorld();
   if(inventoryRead32(source.c,MainCallbackAddress)==InitBattleCallback) {
    fprintf(stdout,"native Marowak flags=%08X callback=%08X\n",inventoryRead32(source.c,BattleFlagsAddress),inventoryRead32(source.c,SavedCallbackAddress));fflush(stdout);
    requirePokemon(!joined&&inventoryRead32(source.c,BattleFlagsAddress)==0xA000&&pair.tryBegin(0),"Original native Marowak scene did not join as a shared spirit battle");joined=true;
   }
  }
  if(pair.active()) {
   input1+=pair.inputOwner()==0;input2+=pair.inputOwner()==1;const auto pulse=frames%30<3?1u:0u;pair.runBattleFrame({{pulse,pulse}});
  } else {
   source.c->setKeys(source.c,frames<12?0x80u:(frames%30<3?1u:0u));source.c->runFrame(source.c);guest.c->setKeys(guest.c,0);guest.c->runFrame(guest.c);campaign.sample();
  }
  if(joined&&!pair.active()&&campaignFieldQuiescent(source.c)&&campaignFieldQuiescent(guest.c)&&readProgress(source.c).var(0x4059)==1&&readProgress(guest.c).var(0x4059)==1){done=true;break;}
 }
 requirePokemon(done&&input1&&input2,"Original native Marowak encounter/postscene did not complete for both owners");
 for(auto* core:{source.c,guest.c}) {
  core->setKeys(core,0);const auto proof=acceptance::verifyNativeSaveReload(core,rom.toStdString());acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned fno=0;fno<10000;++fno){auto* c=fresh.c;const auto cb=inventoryRead32(c,0x03003044);c->setKeys(c,fno<300?(fno%120<3?8u:0u):(fno%40<3?(cb==OverworldCallback?2u:1u):0u));c->runFrame(c);if(fno>300&&progressFieldReady(c)){entered=true;break;}}
  requirePokemon(entered&&readProgress(fresh.c).var(0x4059)==1,"Own Marowak story gate did not persist after native save/fresh boot");
 }
 return frames;
}
unsigned verifyNativeSharedChampion(Core& source,Core& guest,const QString& rom,bool rendezvous=false) {
 const std::array<BattleStatistics,2> oldStats{{readBattleStatistics(source.c),readBattleStatistics(guest.c)}};
 prepareChampionField(source,0,false);prepareChampionField(guest,1,!rendezvous);
 for(auto* core:{source.c,guest.c}) {
  auto party=readParty(core);auto& mon=party.mons[0];auto secure=mon.secure();monPut32(secure.data()+4,1000000);mon.setSecure(secure);
  mon.bytes[84]=100;monPut16(mon.bytes.data()+86,999);monPut16(mon.bytes.data()+88,999);
  for(unsigned off=90;off<100;off+=2)monPut16(mon.bytes.data()+off,999);mon.validate();writeParty(core,party);
 }
 const std::array<PartyMon,2> originals{{readParty(source.c).mons[0],readParty(guest.c).mons[0]}};
 TeamBattle pair({{source.c,guest.c}});bool joined=false,pendingGuest=false,startedGuest=false;unsigned completed=0,frames=0,input1=0,input2=0;
 std::array<bool,2> waited{};
 pair.completed=[&](unsigned leader,unsigned trainer,unsigned outcome,bool peerNative){
  requirePokemon(leader==0&&trainer>=438&&trainer<=440&&outcome==1&&peerNative==rendezvous,"Shared Champion callback did not describe an owned native win");++completed;pendingGuest=!peerNative;
 };
 std::array<bool,2> saved{};std::array<std::vector<uint8_t>,2> batteries;
 for(;frames<24000;++frames) {
  if(!pair.active()) {
   pair.sampleWorld();
   if(!joined&&inventoryRead32(source.c,MainCallbackAddress)==InitBattleCallback) {
    if(pair.tryBegin(0))joined=true;
    else requirePokemon(rendezvous&&pair.waitingForPartner(0),"Original Champion entrance did not join or wait for the shared native battle");
   }
   if(pendingGuest&&campaignFieldQuiescent(guest.c)){requirePokemon(startNativeHallOfFameCompletion(guest.c),"Shared Champion callback could not start guest's own native Hall of Fame");pendingGuest=false;startedGuest=true;}
  }
  if(pair.active()){input1+=pair.inputOwner()==0;input2+=pair.inputOwner()==1;const auto pulse=frames%30<3?1u:0u;pair.runBattleFrame({{pulse,pulse}});}
  else for(unsigned owner=0;owner<2;++owner){auto* core=owner?guest.c:source.c;if(rendezvous&&!joined&&inventoryRead32(core,MainCallbackAddress)==InitBattleCallback){waited[owner]=true;continue;}const auto pulse=rendezvous&&owner==1&&!joined&&frames<1200?0u:(frames%30<3?1u:0u);core->setKeys(core,pulse);core->runFrame(core);}
  if(!pair.active()&&completed)for(unsigned owner=0;owner<2;++owner)if(!saved[owner]) {
   auto* core=owner?guest.c:source.c;auto flash=acceptance::save_detail::flash(core);
   try{requireOwnHallOfFame(flash,originals[owner]);if(acceptance::save_detail::generation(flash)&&readProgress(core).flag(0x82C)){saved[owner]=true;batteries[owner]=std::move(flash);}}catch(const std::runtime_error&){}
  }
  if(saved[0]&&saved[1])break;
 }
 fprintf(stdout,"champion proof frames=%u joined=%u completed=%u guestWarp=%u pending=%u owned=%u/%u saved=%u/%u waited=%u/%u\n",frames,joined,completed,startedGuest,pendingGuest,input1,input2,saved[0],saved[1],waited[0],waited[1]);fflush(stdout);
 requirePokemon(joined&&completed==1&&startedGuest!=rendezvous&&!pendingGuest&&input1&&input2&&saved[0]&&saved[1]&&(!rendezvous||(waited[0]||waited[1])),"Shared Champion win did not produce both own native HOF saves");
 for(unsigned owner=0;owner<2;++owner) {
  auto* core=owner?guest.c:source.c;const auto ownParty=readParty(core);
  requirePokemon(readBattleStatistics(core)==recordSharedBattle(oldStats[owner],true),"Shared Champion counted a trainer encounter more than once");
  requirePokemon(ownParty.count==1&&ownParty.mons[0].identity()==originals[owner].identity()&&(monU32(ownParty.mons[0].secure().data()+44)&(1u<<15)),"Champion victory did not preserve own party and award own native ribbon");
  acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),batteries[owner]);bool entered=false;
  for(unsigned fno=0;fno<10000;++fno){auto* c=fresh.c;const auto cb=inventoryRead32(c,0x03003044);c->setKeys(c,fno<300?(fno%120<3?8u:0u):(fno%40<3?(cb==OverworldCallback?2u:1u):0u));c->runFrame(c);if(fno>300&&progressFieldReady(c)){entered=true;break;}}
  requirePokemon(entered&&readParty(fresh.c)==ownParty&&readProgress(fresh.c).flag(0x82C)&&readGameStatistic(fresh.c,10)==1,"Shared Champion own-party/HOF result did not survive fresh native CONTINUE");
  requireOwnHallOfFame(acceptance::save_detail::flash(fresh.c),originals[owner]);
 }
 return frames;
}
unsigned verifyNativeGrassRendezvous(Core& source,Core& guest,const QString& rom) {
 const std::array<BattleStatistics,2> oldStats{{readBattleStatistics(source.c),readBattleStatistics(guest.c)}};
 for(unsigned owner=0;owner<2;++owner) {
  auto* c=owner?guest.c:source.c;auto before=readProgress(c),p=before;
  p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);writeProgressDelta(c,before,p);
  Party party;party.count=1;party.mons[0]=acceptance::brockFixtureMon(24+24*owner,readTrainerIdentity(c).id);writeParty(c,party);
  // AUTHORED native Route1 entry at a verified tall-grass patch. Neither an
  // encounter flag, opponent, callback nor wild-battle result is injected.
  const auto m=readSaveMemory(c);inventoryWrite16(c,m.block1,14);inventoryWrite16(c,m.block1+2,26);
  c->busWrite8(c,m.block1+4,3);c->busWrite8(c,m.block1+5,19);c->busWrite8(c,0x03003478,0);c->busWrite32(c,MainCallbackAddress,0x0805673D);
  for(unsigned n=0;n<240;++n){c->setKeys(c,0);c->runFrame(c);}
  requirePokemon(campaignFieldQuiescent(c),"Native Route1 grass entry did not become idle");
 }
 TeamBattle pair({{source.c,guest.c}});unsigned completed=0,input1=0,input2=0,frames=0;
 bool joined=false,returned=false;std::array<uint32_t,2> callbacks{};
 pair.completed=[&](unsigned leader,unsigned trainer,unsigned outcome,bool peerNative){
  requirePokemon(leader==0&&trainer==0&&outcome==1&&peerNative,"Native grass rendezvous did not commit one shared wild win");++completed;
 };
 for(;frames<12000;++frames) {
  if(!joined) {
   const bool first=inventoryRead32(source.c,MainCallbackAddress)==InitBattleCallback;
   const bool second=inventoryRead32(guest.c,MainCallbackAddress)==InitBattleCallback;
   if(first&&second) {
    for(unsigned owner=0;owner<2;++owner){auto* c=owner?guest.c:source.c;
     callbacks[owner]=inventoryRead32(c,SavedCallbackAddress);
     requirePokemon(inventoryRead32(c,BattleFlagsAddress)==0&&callbacks[owner]!=0x080567FD&&callbacks[owner]!=OverworldCallback,
                    "Grass fixture did not reach its original ordinary-wild return callback");
    }
    fprintf(stdout,"native grass callbacks=%08X/%08X\n",callbacks[0],callbacks[1]);fflush(stdout);
    pair.sampleWorld();requirePokemon(pair.tryBegin(0),"Both native grass encounters did not coalesce");joined=true;
   }
  }
  if(pair.active()){input1+=pair.inputOwner()==0;input2+=pair.inputOwner()==1;const auto pulse=frames%30<3?1u:0u;pair.runBattleFrame({{pulse,pulse}});}
  else for(auto* c:{source.c,guest.c}) {
   if(!joined&&inventoryRead32(c,MainCallbackAddress)==InitBattleCallback)continue;
   const unsigned key=joined?0u:(frames%96<48?0x40u:0x80u);c->setKeys(c,key);c->runFrame(c);
  }
  if(joined&&!pair.active()&&campaignFieldQuiescent(source.c)&&campaignFieldQuiescent(guest.c)){returned=true;break;}
 }
 requirePokemon(joined&&returned&&completed==1&&input1&&input2,"Native grass rendezvous did not return both owners to their own field");
 for(unsigned owner=0;owner<2;++owner) {
  auto* c=owner?guest.c:source.c;c->setKeys(c,0);const auto expected=recordSharedBattle(oldStats[owner],false);
  requirePokemon(readBattleStatistics(c)==expected,"Native grass battle participation counted more than once");
  requirePokemon(readProgress(c).mapGroup==3&&readProgress(c).mapNum==19,"Native EndWildBattle did not return to own Route1");
  const auto proof=acceptance::verifyNativeSaveReload(c,rom.toStdString());acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),proof.battery);bool entered=false;
  for(unsigned fno=0;fno<10000;++fno){auto* n=fresh.c;const auto cb=inventoryRead32(n,MainCallbackAddress);n->setKeys(n,fno<300?(fno%120<3?8u:0u):(fno%40<3?(cb==OverworldCallback?2u:1u):0u));n->runFrame(n);if(fno>300&&progressFieldReady(n)){entered=true;break;}}
  requirePokemon(entered&&readBattleStatistics(fresh.c)==expected&&readProgress(fresh.c).mapGroup==3&&readProgress(fresh.c).mapNum==19,
                 "Own grass-battle counts/map did not survive native save and fresh Continue");
 }
 return frames;
}
}
int main(int argc,char** argv) {
 QCoreApplication app(argc,argv);
 if(argc!=4&&argc!=5){fprintf(stderr,"verify-firered-campaign ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_OUTPUT_DIRECTORY [hall-of-fame|cut|surf|strength|national-dex|network-0|network-1|network-2|network-loaded|network-guard|marowak|shared-champion|shared-champion-rendezvous|grass-rendezvous]\n");return 2;}
 try {
  const QString rom=argv[1],state=argv[2],out=argv[3];requirePokemon(fileHash(rom)==RomHash,"ROM fingerprint mismatch");
  QFile f(state);requirePokemon(f.open(QIODevice::ReadOnly),"Checkpoint unreadable");const auto bytes=f.readAll();const auto stateHash=fileHash(state);
  requirePokemon(QDir().mkdir(out),"Output directory must be new");
  mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
  Core a,b;a.open(rom,bytes);b.open(rom,bytes);
  auto m=readSaveMemory(b.c);for(unsigned i=0;i<4;++i)b.c->busWrite8(b.c,m.block2+0xA+i,uint8_t(0xCAFEBABEu>>(8*i)));b.c->busWrite8(b.c,m.block2,0xBC);b.c->busWrite8(b.c,m.block2+1,0xFF);
  const auto scenario=argc==5?QString::fromLocal8Bit(argv[4]):QStringLiteral("hall-of-fame");
  const bool network=scenario=="network-0"||scenario=="network-1"||scenario=="network-2"||scenario=="network-loaded"||scenario=="network-guard";
  const bool marowak=scenario=="marowak";
  const bool rendezvous=scenario=="shared-champion-rendezvous";
  const bool champion=scenario=="shared-champion"||rendezvous;
  const bool grass=scenario=="grass-rendezvous";
  requirePokemon(scenario=="hall-of-fame"||scenario=="cut"||scenario=="surf"||scenario=="strength"||scenario=="national-dex"||network||marowak||champion||grass,"Unknown campaign scenario");
  if(scenario!="hall-of-fame") {
   const bool national=scenario=="national-dex";
   const auto frames=grass?verifyNativeGrassRendezvous(a,b,rom):champion?verifyNativeSharedChampion(a,b,rom,rendezvous):marowak?verifyNativeMarowak(a,b,rom):network?verifyNativeNetwork(a,b,rom,scenario.right(1).toUInt(),scenario):national?verifyNativeNationalDex(a,b,rom):verifyNativeMachineScene(a,b,rom,scenario);a.capture(out+"/source-complete.png");b.capture(out+"/recipient-complete.png");
   requirePokemon(fileHash(rom)==RomHash&&fileHash(state)==stateHash,"Private inputs changed");
   QJsonObject report{{"scenario",scenario},{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(stateHash)},
    {"authored_quest_prerequisites_and_map_entry",true},{"original_native_npc_dialogue_and_item_grant",!national&&!network&&!marowak&&!champion&&!grass},{"authored_source_upgrade_trigger",national||network},{"two_real_cores",true},
    {"production_campaign_planner_and_map_refresh",!champion&&!grass},{"own_machine_item_and_receipt_survive_native_save_and_fresh_boot",!national&&!network&&!marowak&&!champion&&!grass},
    {"own_hall_of_fame_required_before_national_dex",national},{"all_three_national_dex_receipts_and_personal_entries_preserved",national},
    {"own_starter_native_roamer_and_postgame_flags_persist",network},
    {"already_loaded_native_network_machine_refreshed",scenario=="network-loaded"},{"already_loaded_native_cerulean_guard_removed",scenario=="network-guard"},{"caught_or_fled_roamer_not_reinitialized_after_restart",network},
    {"original_native_scripted_marowak_trigger_and_two_owned_battle_inputs",marowak},{"own_marowak_scene_gate_survives_native_save_fresh_boot",marowak},
    {"original_champion_entrance_native_team_battle_and_win_callback",champion},{"own_hof_sectors_ribbons_and_both_native_fresh_continue",champion},
    {"authored_guest_prebattle_scene_suppression",champion&&!rendezvous},{"authored_level100_and_stats999_own_parties",champion},
    {"both_original_champion_scenes_rendezvous_with_own_starter_variants",rendezvous},{"battle_counter_once_each_and_hof_once_each",champion},
    {"first_native_encounter_waited_for_delayed_second_player",rendezvous},
    {"both_native_grass_encounters_and_original_endwild_callbacks",grass},{"grass_battle_counts_once_each_and_native_save_fresh_continue",grass},
    {"rom_and_checkpoint_unchanged",true},{"production_launcher",false},{"frames",int(frames)}};
   QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot write report");const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Report write failed");printf("%s",json.data());return 0;
  }
  prepareChampionField(a,0);prepareChampionField(b,1);
  auto beforeGate=readProgress(b.c),blockedGate=beforeGate;blockedGate.flag(0x827,false);writeProgressDelta(b.c,beforeGate,blockedGate);
  const auto untouchedContext=acceptance::save_detail::bytes(b.c,GlobalScriptContextAddress,0x74);
  requirePokemon(!canStartNativeHallOfFameCompletion(b.c)&&!startNativeHallOfFameCompletion(b.c)&&
                 acceptance::save_detail::bytes(b.c,GlobalScriptContextAddress,0x74)==untouchedContext,
                 "Hall of Fame started without eight own badges or mutated a rejected VM");
  writeProgressDelta(b.c,blockedGate,beforeGate);
  std::array<PartyMon,2> originals{{readParty(a.c).mons[0],readParty(b.c).mons[0]}};
  std::array<uint32_t,2> oldEntries{{readGameStatistic(a.c,10),readGameStatistic(b.c,10)}};
  for(auto* core:{a.c,b.c})requirePokemon(startNativeHallOfFameCompletion(core),"Native completion did not start");
  unsigned frames=0;std::array<bool,2> saved{};std::array<std::vector<uint8_t>,2> batteries;
  std::array<Core*,2> cores{{&a,&b}};
  for(;frames<22000;++frames) {
   for(unsigned owner=0;owner<2;++owner) {
    auto* core=cores[owner]->c;core->setKeys(core,frames%30<3?1u:0u);core->runFrame(core);
    if(!saved[owner]&&readGameStatistic(core,10)==oldEntries[owner]+1) {
     auto flash=acceptance::save_detail::flash(core);
     // Hall-of-Fame saves run asynchronously; the original generation and both
     // dedicated HOF-sector checksums must complete before taking evidence.
     try{requireOwnHallOfFame(flash,originals[owner]);if(acceptance::save_detail::generation(flash)){saved[owner]=true;batteries[owner]=std::move(flash);}}
     catch(const std::runtime_error&){}
    }
   }
   if(frames%1000==0){a.capture(out+"/source-"+QString::number(frames)+".png");b.capture(out+"/guest-"+QString::number(frames)+".png");}
   if(saved[0]&&saved[1])break;
  }
  requirePokemon(saved[0]&&saved[1],"Both native Hall-of-Fame saves did not complete");
  for(unsigned owner=0;owner<2;++owner) {
   auto* core=cores[owner]->c;const auto party=readParty(core);
   requirePokemon(party.count==1&&party.mons[0].identity()==originals[owner].identity(),"Hall of Fame replaced own party");
   requirePokemon(party.mons[0].hp()==party.mons[0].maxHp(),"Native Hall of Fame did not heal own party");
   const auto secure=party.mons[0].secure();requirePokemon(monU32(secure.data()+44)&(1u<<15),"Native champion ribbon missing");
   requirePokemon(readProgress(core).flag(0x82C)&&readProgress(core).flag(0x83B),"Native game-clear/ribbon receipt missing");
   acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),batteries[owner]);bool entered=false;
   for(unsigned fno=0;fno<10000;++fno) {
    auto* c=fresh.c;const auto cb=inventoryRead32(c,0x03003044);
    c->setKeys(c,fno<300?(fno%120<3?8u:0u):(fno%40<3?(cb==0x080565D5?2u:1u):0u));c->runFrame(c);
    if(fno>300&&progressFieldReady(c)){entered=true;break;}
   }
   requirePokemon(entered,"Fresh native CONTINUE after HOF did not reach field");
   const auto resumed=readParty(fresh.c);
   requirePokemon(resumed==party,"Own HOF party/ribbon not persisted");
   requirePokemon(readProgress(fresh.c).flag(0x82C)&&readGameStatistic(fresh.c,10)==oldEntries[owner]+1,"Own native game completion not persisted");
   requireOwnHallOfFame(acceptance::save_detail::flash(fresh.c),originals[owner]);
  }
  requirePokemon(fileHash(rom)==RomHash&&fileHash(state)==stateHash,"Private inputs changed");
  QJsonObject report{{"scenario","native-hall-of-fame-two-owners"},{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(stateHash)},
   {"authored_champion_map_badges_and_win_receipts",true},{"actual_champion_battle",false},{"two_real_cores",true},
   {"original_native_hall_of_fame_scene",true},{"own_party_champion_ribbons",true},{"own_hof_sectors_and_checksums",true},
   {"native_save_and_fresh_battery_reload_both_owners",true},{"rom_and_checkpoint_unchanged",true},{"production_launcher",false},{"frames",int(frames)}};
  QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot write report");const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Report write failed");printf("%s",json.data());return 0;
 }catch(const std::exception& e){fprintf(stderr,"Campaign acceptance failed: %s\n",e.what());return 1;}
}
