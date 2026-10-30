// SPDX-License-Identifier: GPL-3.0-or-later
// Private-ROM/save acceptance, excluded from normal builds and distribution.
// Public saves are read only into isolated memory; no original save is written.
#include "../gba/FireRedBattle.h"
#include "../gba/FireRedCampaignSync.h"
#include "FireRedSaveAcceptance.h"
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <iostream>
using namespace coop::firered;
namespace {
constexpr auto RomHash="eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
QByteArray fileHash(const QString& path){QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly),"Cannot read input");QCryptographicHash h(QCryptographicHash::Sha256);requirePokemon(h.addData(&f),"Cannot hash input");return h.result().toHex();}
std::vector<uint8_t> readBattery(const QString& path){QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly)&&f.size()==131072,"Save is not a complete 128 KiB battery");const auto b=f.readAll();requirePokemon(b.size()==131072,"Cannot read complete battery");std::vector<uint8_t> data(b.begin(),b.end());requirePokemon(bool(acceptance::save_detail::generation(data)),"Battery has no complete checksum-valid 14-section generation");return data;}
unsigned boot(acceptance::save_detail::FreshCore& fresh,const QString& rom,const std::vector<uint8_t>& battery){fresh.open(rom.toStdString(),battery);auto* c=fresh.c;
 for(unsigned n=0;n<12000;++n){const auto cb=inventoryRead32(c,0x03003044);const unsigned keys=n<300?(n%120<3?8u:0u):(n%60<3?(cb==0x080565D5?2u:1u):0u);c->setKeys(c,keys);c->runFrame(c);
  if(n>300&&campaignFieldQuiescent(c)&&readWorldActor(c).valid){c->setKeys(c,0);return n;}}
 throw std::runtime_error("Save did not reach an idle native overworld on the German ROM");}
QJsonObject inspect(const QString& rom,const QString& path){
 const auto beforeHash=fileHash(path);const auto battery=readBattery(path);acceptance::save_detail::FreshCore fresh;
 const unsigned frames=boot(fresh,rom,battery);auto* c=fresh.c;
 std::array<unsigned,7> counts{};unsigned valid=0,invalid=0;
 for(unsigned n=0;n<120;++n){c->setKeys(c,0);c->runFrame(c);const unsigned count=c->busRead8(c,PlayerPartyCountAddress);
  if(count<=6)++counts[count];try{readParty(c);++valid;}catch(const std::exception&){++invalid;}}
 const auto memory=readSaveMemory(c);
 QJsonObject result{{"source_sha256",QString::fromLatin1(beforeHash)},{"valid_save_generation",int(*acceptance::save_detail::generation(battery))},
  {"fresh_continue_frames",int(frames)},{"map_group",int(c->busRead8(c,memory.block1+4))},{"map_number",int(c->busRead8(c,memory.block1+5))},
  {"x",int(inventoryRead16(c,memory.block1))},{"y",int(inventoryRead16(c,memory.block1+2))},{"party_count",int(c->busRead8(c,PlayerPartyCountAddress))},
  {"native_quest_log_state_at_idle_field",int(c->busRead8(c,QuestLogStateAddress))},
  {"stable_party_read_frames",int(valid)},{"rejected_party_read_frames",int(invalid)}};
 QJsonArray histogram;for(unsigned v:counts)histogram.append(int(v));result.insert("party_count_histogram",histogram);
 QJsonArray raw;for(unsigned i=0;i<6;++i){const auto at=PlayerPartyAddress+i*PartyMonSize;PartyMon m;
  for(unsigned j=0;j<PartyMonSize;++j)m.bytes[j]=uint8_t(c->busRead8(c,at+j));
  QJsonObject r{{"slot",int(i)},{"empty",m.empty()},{"sanity",int(m.bytes[19])}};
  try{m.validate();r.insert("valid",true);if(!m.empty())r.insert("species",int(m.species()));}catch(const std::exception& e){r.insert("error",QString::fromUtf8(e.what()));}raw.append(r);
 }result.insert("raw_party",raw);
 try{
  const auto party=readParty(c);const auto progress=readProgress(c);const auto trainer=readTrainerIdentity(c);const auto storage=readStorage(c);
  QJsonArray mons;for(unsigned i=0;i<party.count;++i){const auto& m=party.mons[i];QJsonArray moves,pp;for(unsigned j=0;j<4;++j){moves.append(int(m.move(j)));pp.append(int(m.pp(j)));}
   const bool ownTrainer=m.identity().trainer==trainer.id &&
       std::equal(trainer.name.begin(),trainer.name.end(),m.bytes.begin()+20);
   mons.append(QJsonObject{{"species",int(m.species())},{"level",int(m.level())},{"hp",int(m.hp())},{"maximum_hp",int(m.maxHp())},{"moves",moves},{"pp",pp},{"held_item",int(m.heldItem())},{"original_trainer_matches_save",ownTrainer}});}
  QJsonArray badges;for(unsigned flag=0x820;flag<=0x827;++flag)badges.append(progress.flag(flag));
  unsigned occupied=0;for(unsigned b=0;b<BoxCount;++b)for(unsigned s=0;s<BoxCapacity;++s)occupied+=!storage.mon(b,s).empty();
  result.insert("badges",badges);result.insert("party",mons);result.insert("pc_count",int(occupied));result.insert("wallet",int(readWallet(c)));
  result.insert("poke_balls",int(readInventory(c).quantity(PokeBallItem)));
  result.insert("route22_rival_scene_stage",int(progress.var(0x4054)));
  QJsonArray rivalFlags;for(unsigned i=329;i<=331;++i)rivalFlags.append(progress.flag(0x500+i));
  result.insert("route22_early_rival_trainer_flags",rivalFlags);
  // A completed CONTINUE can leave a prior button held at the frame boundary.
  // Release it and allow the native field and avatar animation to settle.
  for(unsigned i=0;i<60;++i){c->setKeys(c,0);c->runFrame(c);}
  // Some saves resume into an original auto-start field dialogue. Let that
  // native scene finish before attempting SAVE; the source file stays intact.
  unsigned settle=0;
  while(!progressFieldReady(c)&&settle<4000) {c->setKeys(c,settle%40<3?1u:0u);c->runFrame(c);++settle;}
  c->setKeys(c,0);result.insert("native_scene_settle_frames",int(settle));
  const auto proof=acceptance::verifyNativeSaveReload(c,rom.toStdString());
  result.insert("native_save_and_fresh_continue",true);result.insert("save_generation_after",int(proof.generation));
 }catch(const std::exception& e){result.insert("error",QString::fromUtf8(e.what()));
  result.insert("callback2",QString::number(c->busRead32(c,0x03003044),16));result.insert("menu_callback",QString::number(c->busRead32(c,0x020370F0),16));
  result.insert("field_lock",int(c->busRead8(c,0x03000F9C)));result.insert("script_status",int(c->busRead8(c,0x03000EA8)));
  if(QString::fromUtf8(e.what()).contains("START menu did not become ready"))try {
   // Follow-up only: keep original party, progress, bags and money, but move to
   // a known neutral field before attempting the game's native save again.
   const auto m=readSaveMemory(c);inventoryWrite16(c,m.block1,14);inventoryWrite16(c,m.block1+2,26);
   c->busWrite8(c,m.block1+4,3);c->busWrite8(c,m.block1+5,19);c->busWrite8(c,0x03003478,0);c->busWrite32(c,MainCallbackAddress,0x0805673D);
   for(unsigned n=0;n<240;++n){c->setKeys(c,0);c->runFrame(c);}
   acceptance::verifyNativeSaveReload(c,rom.toStdString());result.insert("native_save_after_authored_route1_entry",true);
  }catch(const std::exception& followup){result.insert("relocation_save_error",QString::fromUtf8(followup.what()));}
 }
 requirePokemon(fileHash(path)==beforeHash&&fileHash(rom)==RomHash,"Input file changed");result.insert("source_and_rom_unchanged",true);return result;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);if(argc<3){fprintf(stderr,"verify-firered-public-saves ORIGINAL_DE.gba SAVE.sav [SAVE.sav ...]\n");return 2;}
 try{const QString rom=argv[1];requirePokemon(fileHash(rom)==RomHash,"Exact German FireRed ROM required");mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
 QJsonArray results;bool failed=false;for(int i=2;i<argc;++i){try{auto r=inspect(rom,argv[i]);if(r.contains("error"))failed=true;r.insert("input",QFileInfo(QString::fromLocal8Bit(argv[i])).fileName());results.append(r);}
 catch(const std::exception& e){failed=true;results.append(QJsonObject{{"input",QFileInfo(QString::fromLocal8Bit(argv[i])).fileName()},{"error",QString::fromUtf8(e.what())}});}}
 std::cout<<QJsonDocument(results).toJson().constData();return failed?1:0;
 }catch(const std::exception& e){fprintf(stderr,"Public-save acceptance failed: %s\n",e.what());return 1;}}
