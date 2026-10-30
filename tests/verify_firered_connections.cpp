// SPDX-License-Identifier: GPL-3.0-or-later
// Authored map/quest prerequisites; native connected-map walking and avatar
// decoding. No ROM edits, network collision, or full campaign certification.
#include "../gba/FireRedConnections.h"
#include "../gba/FireRedCampaignRuntime.h"
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
QByteArray hash(const QString& path){QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly),"Unreadable input");QCryptographicHash h(QCryptographicHash::Sha256);requirePokemon(h.addData(&f),"Hash failed");return h.result().toHex();}
struct Core {
 mCore* c=nullptr;bool initialized=false,configured=false;std::array<color_t,240*160> video{};
 ~Core(){if(c){if(configured)mCoreConfigDeinit(&c->config);if(initialized)c->deinit(c);else free(c);}}
 void open(const QString& rom,const QByteArray& state){c=mCoreFind(rom.toUtf8().constData());requirePokemon(c&&c->init(c),"Core failed");initialized=true;
  mCoreInitConfig(c,nullptr);configured=true;mCoreOptions o{};o.useBios=false;o.skipBios=true;mCoreConfigLoadDefaults(&c->config,&o);mCoreLoadConfig(c);
  requirePokemon(mCoreLoadFile(c,rom.toUtf8().constData()),"ROM failed");c->setVideoBuffer(c,video.data(),240);c->reset(c);
  requirePokemon(state.size()==qint64(c->stateSize(c))&&c->loadState(c,state.data()),"Checkpoint failed");}
 void enter(unsigned map,unsigned x,unsigned y){auto before=readProgress(c),p=before;p.var(VarLabScene,6);p.var(VarMartScene,2);p.flag(FlagPokedexObtained,true);p.gcnLinkFlags|=0x31;
  p.flag(0x258,true);p.flag(0x02D,true);p.flag(0x03A,true);writeProgressDelta(c,before,p);
  const auto m=readSaveMemory(c);inventoryWrite16(c,m.block1,x);inventoryWrite16(c,m.block1+2,y);c->busWrite8(c,m.block1+4,3);c->busWrite8(c,m.block1+5,map);
  c->busWrite8(c,0x03003478,0);c->busWrite32(c,0x03003044,0x0805673D);for(unsigned i=0;i<240;++i){c->setKeys(c,0);c->runFrame(c);}
  requirePokemon(campaignFieldQuiescent(c)&&readWorldActor(c).valid,"Map did not become native field");}
 void capture(const QString& file,const WorldActor& local,const WorldActor& projected){std::array<uint32_t,240*160> frame{};
  for(unsigned i=0;i<video.size();++i){const auto p=video[i];frame[i]=0xFF000000u|((p&255)<<16)|(p&0xFF00)|((p>>16)&255);}
  const auto before=frame;compositePartner(frame.data(),240,local,projected);requirePokemon(before!=frame,"Projected partner produced no screen pixels");
  QImage im(240,160,QImage::Format_ARGB32);for(unsigned i=0;i<frame.size();++i)im.setPixel(i%240,i/240,frame[i]);requirePokemon(im.save(file),"Capture failed");}
};
void verify(Core& a,Core& b,const QString& out,const QString& name){const auto aa=readWorldActor(a.c),bb=readWorldActor(b.c);const auto at=readWorldTopology(a.c,aa),bt=readWorldTopology(b.c,bb);
 requirePokemon(aa.valid&&bb.valid&&at.valid&&bt.valid,"Native topology unavailable");
 const auto ab=projectConnectedPartner(aa,bb,at,bt),ba=projectConnectedPartner(bb,aa,bt,at);requirePokemon(ab&&ba,"Native reciprocal connection rejected");
 fprintf(stdout,"%s: A(%u,%u) %d,%d B(%u,%u) %d,%d projected %d,%d / %d,%d\n",qPrintable(name),aa.mapGroup,aa.mapNum,aa.tileX,aa.tileY,bb.mapGroup,bb.mapNum,bb.tileX,bb.tileY,ab->tileX,ab->tileY,ba->tileX,ba->tileY);
 a.capture(out+"/"+name+"-a.png",aa,*ab);b.capture(out+"/"+name+"-b.png",bb,*ba);
 // Snapshot reading and framebuffer composition must never alter machine state.
 std::vector<uint8_t> state(a.c->stateSize(a.c)),after(state.size());requirePokemon(a.c->saveState(a.c,state.data()),"Snapshot failed");
 readWorldTopology(a.c,aa);projectConnectedPartner(aa,bb,at,bt);a.capture(out+"/"+name+"-read-only.png",aa,*ab);
 requirePokemon(a.c->saveState(a.c,after.data())&&state==after,"Presentation changed machine state");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);if(argc!=4){fprintf(stderr,"verify-firered-connections ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_OUTPUT_DIRECTORY\n");return 2;}
 try{const QString rom=argv[1],path=argv[2],out=argv[3];requirePokemon(hash(rom)==RomHash,"Exact original ROM required");QFile f(path);requirePokemon(f.open(QIODevice::ReadOnly),"Checkpoint unreadable");const auto state=f.readAll(),original=hash(path);requirePokemon(QDir().mkdir(out),"Output must be new");
 mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
 Core a,b;a.open(rom,state);b.open(rom,state);a.enter(0,13,1);b.enter(19,13,39);verify(a,b,out,"pallet-route1");
 // Actual native walking crosses the map connection; no forced destination.
 unsigned crossed=0;for(;crossed<240;++crossed){a.c->setKeys(a.c,0x40);a.c->runFrame(a.c);const auto m=readSaveMemory(a.c);if(a.c->busRead8(a.c,m.block1+5)==19)break;}
 if(crossed==240){const auto pos=readWorldActor(a.c);fprintf(stderr,"Crossing stopped map%u x%d y%d callback%08X\n",pos.mapNum,pos.tileX,pos.tileY,a.c->busRead32(a.c,0x03003044));}requirePokemon(crossed<240,"Native Pallet-to-Route1 walking failed");for(unsigned i=0;i<90;++i){a.c->setKeys(a.c,0);a.c->runFrame(a.c);}requirePokemon(readWorldActor(a.c).valid,"Native crossing did not settle");
 a.enter(19,13,0);b.enter(1,25,39);verify(a,b,out,"route1-viridian-offset");
 for(auto* c:{a.c,b.c})acceptance::verifyNativeSaveReload(c,rom.toStdString());
 requirePokemon(hash(rom)==RomHash&&hash(path)==original,"Input files changed");
 QJsonObject report{{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(original)},{"native_reciprocal_maps",2},{"native_walked_pallet_route1",true},{"signed_connection_offset",true},{"native_avatar_overlay_both_views",true},{"presentation_machine_snapshot_unchanged",true},{"both_native_save_fresh_continue",true},{"original_inputs_unchanged",true},{"authored_map_and_quest_prerequisites",true},{"production_launcher",false},{"partner_collision",false}};
 QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Report failed");const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Report write failed");printf("%s",json.data());return 0;
 }catch(const std::exception& e){fprintf(stderr,"Connections acceptance failed: %s\n",e.what());return 1;}}
