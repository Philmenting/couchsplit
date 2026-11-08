// SPDX-License-Identifier: GPL-3.0-or-later
// ROM-dependent acceptance, excluded from normal builds and installation.
#include "../gba/FireRedBattle.h"
#include "../gba/FireRedRecovery.h"
#include "FireRedSaveAcceptance.h"
#include "FireRedReserveAcceptance.h"
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonArray>
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
PartyMon mon(unsigned pid, unsigned ot, unsigned species, bool opponent = false) {
    PartyMon result;
    monPut32(result.bytes.data(),pid); monPut32(result.bytes.data()+4,ot);
    result.bytes[8]=0xBB; result.bytes[9]=0xFF; result.bytes[18]=5; result.bytes[19]=2;
    result.bytes[20]=0xBB; result.bytes[21]=0xFF;
    PartyMon::Secure plain{}; monPut16(plain.data(),species); monPut32(plain.data()+4,opponent?1000:125000);
    plain[9]=70; monPut16(plain.data()+12,33); monPut16(plain.data()+14,45); plain[20]=35; plain[21]=40;
    monPut32(plain.data()+40,0x3FFFFFFF); result.setSecure(plain);
    result.bytes[84]=opponent?10:50;
    monPut16(result.bytes.data()+86,opponent?24:150); monPut16(result.bytes.data()+88,opponent?24:150);
    for(unsigned offset=90;offset<100;offset+=2) monPut16(result.bytes.data()+offset,opponent?15:120);
    result.validate(); return result;
}
void enemies(mCore* core, unsigned count = 2, unsigned species = 19) {
    // Native wild CreateMon uses the initiating trainer's OT before capture.
    // A foreign-OT fixture would make native GiveMonToPlayer change the key
    // without re-encrypting (a path ordinary wild creation never exercises).
    const auto ot=readTrainerIdentity(core).id;
    const std::array<PartyMon,2> mons{{mon(72,ot,species,true), mon(96,ot,16,true)}};
    for(unsigned b=0;b<600;++b) core->busWrite8(core,0x0202402C+b,b<count*100?mons[b/100].bytes[b%100]:0);
    core->busWrite8(core,0x0202402A,uint8_t(count));
}
void trigger(mCore* core, const QString& scenario) {
    enemies(core,(scenario.startsWith("single") || scenario=="party-menu-single" || scenario=="grass-rendezvous" || scenario=="deferred-script" || scenario=="action-stats" || scenario=="marowak" || scenario=="scripted" || scenario=="legendary" || scenario.startsWith("capture") || scenario.startsWith("escape"))?1:2,scenario=="deferred-script"?101:scenario=="marowak"?105:scenario=="legendary"?146:scenario=="capture-sequential"?150:scenario.startsWith("capture-unown")?201:scenario.startsWith("capture-spinda")?308:19);
    if(scenario.startsWith("capture")) {
        PartyMon enemy;for(unsigned byte=0;byte<PartyMonSize;++byte)enemy.bytes[byte]=uint8_t(core->busRead8(core,0x0202402C+byte));
        auto secure=enemy.secure();secure[20]=7;secure[21]=11;enemy.setSecure(secure);
        for(unsigned byte=0;byte<PartyMonSize;++byte)core->busWrite8(core,0x0202402C+byte,enemy.bytes[byte]);
    }
    if(scenario.startsWith("single-multiturn")) {
        PartyMon enemy;for(unsigned byte=0;byte<PartyMonSize;++byte)enemy.bytes[byte]=uint8_t(core->busRead8(core,0x0202402C+byte));
        auto secure=enemy.secure();monPut16(secure.data()+14,0);secure[21]=0;enemy.setSecure(secure);
        for(unsigned byte=0;byte<PartyMonSize;++byte)core->busWrite8(core,0x0202402C+byte,enemy.bytes[byte]);
    }
    if(scenario=="bench" || scenario=="party-menu-single") for(unsigned slot=0;slot<(scenario=="bench"?2u:1u);++slot) {
        core->busWrite16(core,0x0202402C+slot*100+86,700);
        core->busWrite16(core,0x0202402C+slot*100+88,700);
    }
    if(scenario=="party-menu-forced")for(unsigned slot=0;slot<2;++slot) {
        PartyMon enemy;for(unsigned byte=0;byte<PartyMonSize;++byte)enemy.bytes[byte]=uint8_t(core->busRead8(core,0x0202402C+slot*100+byte));
        auto secure=enemy.secure();for(unsigned move=0;move<4;++move){monPut16(secure.data()+12+move*2,move==0?(slot?45:89):0);secure[20+move]=move==0?10:0;}enemy.setSecure(secure);
        for(unsigned byte=0;byte<PartyMonSize;++byte)core->busWrite8(core,0x0202402C+slot*100+byte,enemy.bytes[byte]);
    }
    core->busWrite32(core,BattleFlagsAddress,scenario=="trainer" || scenario.startsWith("trainer-single") || scenario.startsWith("exp-share-trainer") || scenario.startsWith("reserves-roar") || scenario=="rendezvous"?8:scenario=="marowak"?0xA000:scenario=="scripted" || scenario=="deferred-script" || scenario=="capture-scripted"?0x20000:scenario=="legendary" || scenario=="capture-legendary"?0x42000:0);
    if(scenario=="trainer" || scenario.startsWith("trainer-single") || scenario.startsWith("exp-share-trainer") || scenario.startsWith("reserves-roar") || scenario=="rendezvous")core->busWrite16(core,TrainerAddress,scenario.startsWith("trainer-single")?90:89);
    core->busWrite32(core,SavedCallbackAddress,scenario=="defeat"?0x080566C5:0x080567FD);
    core->busWrite32(core,MainCallbackAddress,InitBattleCallback);
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    if(argc!=4 && argc!=5){fprintf(stderr,"verify-firered-pair ORIGINAL_DE.gba PRIVATE_OVERWORLD_STATE NEW_OUTPUT_DIRECTORY [wild|single|single-leader2|single-multiturn|single-multiturn-leader2|trainer-single|trainer-single-leader2|trainer-single-multiturn|party-menu-single|escape|escape-leader2|escape-player2|marowak|scripted|legendary|rendezvous|grass-rendezvous|deferred-script|trainer|payday|capture|capture-leader2|capture-scripted|capture-legendary|capture-full-native|capture-party-space|capture-pc|capture-denied|capture-both|capture-both-leader2|capture-sequential|capture-unown|capture-spinda|capture-unown-first|owners|leader2|action-stats|evolution|bench|exhaust|defeat]\n");return 2;}
    try {
        const QString rom=QString::fromLocal8Bit(argv[1]),state=QString::fromLocal8Bit(argv[2]),out=QString::fromLocal8Bit(argv[3]);
        const auto scenario=argc==5?QString::fromLocal8Bit(argv[4]):QStringLiteral("wild");
        requirePokemon(scenario=="wild" || scenario.startsWith("reserves-roar") || scenario.startsWith("shedinja") || scenario.startsWith("pokerus") || scenario=="capture-small" || scenario=="soak-launcher" || scenario.startsWith("single") || scenario.startsWith("exp-share") || scenario=="escape" || scenario=="escape-leader2" || scenario=="escape-player2" || scenario=="grass-rendezvous" || scenario=="deferred-script" || scenario=="marowak" || scenario=="scripted" || scenario=="legendary" || scenario=="rendezvous" || scenario=="capture-scripted" || scenario=="capture-legendary" || scenario=="trainer" || scenario.startsWith("trainer-single") || scenario=="payday" || (scenario=="capture" || scenario=="capture-leader2" || scenario=="capture-full-native" || scenario=="capture-party-space" || scenario=="capture-pc" || scenario=="capture-denied" || scenario.startsWith("capture-both") || scenario=="capture-sequential" || scenario=="capture-unown" || scenario=="capture-spinda" || scenario=="capture-unown-first") || scenario=="owners" || scenario=="leader2" || scenario=="action-stats" || scenario=="evolution" || scenario=="bench" || scenario=="exhaust" || scenario=="defeat" || scenario.startsWith("party-menu"),"Unknown acceptance scenario");
        requirePokemon(hashFile(rom)==RomHash,"Only the exact German FireRed original is supported");
        QFile checkpoint(state); requirePokemon(checkpoint.open(QIODevice::ReadOnly),"Cannot open private checkpoint");
        const auto bytes=checkpoint.readAll(); const auto stateHash=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex();
        requirePokemon(QDir().mkdir(out),"Output directory must be new and have an existing parent");
        mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;
        mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
        Core first,second;first.open(rom,bytes);second.open(rom,bytes);
        auto* a=first.c;auto* b=second.c;
        if(scenario.startsWith("reserves-roar") || scenario.contains("wish") || scenario.startsWith("shedinja") || scenario.startsWith("pokerus") || scenario.startsWith("capture") || scenario.startsWith("exp-share") || scenario=="party-menu-exp-share" || scenario=="owners" || scenario=="leader2") {
            const auto memory=readSaveMemory(b);
            for(unsigned byte=0;byte<4;++byte)b->busWrite8(b,memory.block2+0xA+byte,uint8_t(0xCAFEBABEu>>(byte*8)));
            b->busWrite8(b,memory.block2,0xBC);b->busWrite8(b,memory.block2+1,0xFF);
        }
        const auto trainer0=readTrainerIdentity(a), trainer1=readTrainerIdentity(b);
        const unsigned captureSpecies=scenario=="capture-sequential"?150:scenario.startsWith("capture-unown")?201:scenario.startsWith("capture-spinda")?308:19;
        Party p0,p1;p0.count=p1.count=2;
        p0.mons[0]=mon(24,trainer0.id,25);p0.mons[1]=mon(120,trainer0.id,19);
        p1.mons[0]=mon(48,trainer1.id,25);p1.mons[1]=mon(144,trainer1.id,19);
        if(scenario=="soak-launcher") {
            // Private long-run fixture: level-capped, no held items or pending
            // learning/evolution, sufficient native PP for repeated encounters.
            for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<2;++slot) {
                auto& pokemon=(owner?p1:p0).mons[slot];
                pokemon=mon(24*(owner*6+slot+1),(owner?trainer1:trainer0).id,150);
                auto plain=pokemon.secure();monPut32(plain.data()+4,1250000);plain[8]=0xFF;
                for(unsigned move=0;move<4;++move) {
                    monPut16(plain.data()+12+move*2,std::array<unsigned,4>{{33,10,1,98}}[move]);
                    plain[20+move]=move==3?48:56;
                }
                pokemon.setSecure(plain);pokemon.bytes[84]=100;
                for(unsigned offset=86;offset<100;offset+=2)monPut16(pokemon.bytes.data()+offset,300);
            }
        }
        if(scenario.startsWith("shedinja") || scenario.startsWith("pokerus")) {
            for(unsigned owner=0;owner<2;++owner) {
                auto& party=owner?p1:p0;
                party.count=scenario.contains("multiple")?4:scenario.startsWith("pokerus") || scenario.contains("full")?6:5;
                for(unsigned slot=0;slot<party.count;++slot) {
                    auto& m=party.mons[slot];m=mon(24*(owner*6+slot+1),(owner?trainer1:trainer0).id,25);
                    auto plain=m.secure();
                    if(scenario.startsWith("pokerus")) {
                        monPut16(plain.data()+2,ExpShareItem);
                        plain[36]=std::array<uint8_t,3>{{0x23,0x20,0}}[(slot+owner)%3];
                    } else if(scenario.contains("multiple")?slot>=2:slot==4) {
                        if(scenario.contains("multiple")) {
                            // Native cloning clears ribbons/markings/status but
                            // retains Pokerus and the modern fateful bit.
                            plain[36]=0x23;monPut32(plain.data()+44,0x8FFFFFFF);
                            m.bytes[27]=7;monPut16(m.bytes.data()+30,0x1234);m.setStatus(8);
                            if(owner==1)m.bytes[18]=1;
                        }
                        // Nincada uses the erratic curve: level 20 is 12800 EXP.
                        monPut16(plain.data(),301);monPut32(plain.data()+4,12799);m.bytes[84]=19;
                        monPut16(plain.data()+2,ExpShareItem);
                        monPut16(m.bytes.data()+86,45);monPut16(m.bytes.data()+88,45);
                        for(unsigned off=90;off<100;off+=2)monPut16(m.bytes.data()+off,30);
                    }
                    m.setSecure(plain);
                }
                if(scenario.startsWith("shedinja")) {
                    auto* c=owner?b:a;const auto memory=readSaveMemory(c);auto progress=readProgress(c);
                    progress.flag(0x840,true);progress.var(0x404E,0x6258);
                    writeProgressDelta(c,readProgress(c),progress);c->busWrite8(c,memory.block2+0x1B,0xB9);
                }
            }
        }
        if(scenario=="action-stats") {
            p0.count=p1.count=1;p0.mons[1]=p1.mons[1]={};
            auto plain=p0.mons[0].secure();monPut16(plain.data()+12,150);plain[20]=40;p0.mons[0].setSecure(plain);
            monPut16(p0.mons[0].bytes.data()+94,150);monPut16(p1.mons[0].bytes.data()+94,100);
            for(unsigned slot=0;slot<4;++slot)p1.mons[0].setPp(slot,0);
        }
        if(scenario=="evolution") {
            p0.count=p1.count=1;p0.mons[1]=p1.mons[1]={};
            p0.mons[0]=mon(24,trainer0.id,1);p1.mons[0]=mon(48,trainer1.id,4);
            for(auto* party:{&p0,&p1}) {
                auto& m=party->mons[0];auto secure=m.secure();monPut32(secure.data()+4,2534);m.setSecure(secure);
                m.bytes[84]=15;monPut16(m.bytes.data()+86,41);monPut16(m.bytes.data()+88,41);
                for(unsigned offset=90;offset<100;offset+=2)monPut16(m.bytes.data()+offset,25);
            }
        }
        if(scenario.startsWith("exp-share")) {
            p0.count=p1.count=6;
            for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<6;++slot) {
                auto& pokemon=(owner?p1:p0).mons[slot];
                const unsigned species=slot==5 && scenario.contains("dex") ? 42u :
                    (slot==5 || scenario.contains("twelve")) && scenario.contains("evolution") ? (owner?4u:1u) :
                    slot==5 && owner==1 && scenario.contains("learn") ? 1u : 25u;
                pokemon=mon(24*(owner*6+slot+1),(owner?trainer1:trainer0).id,species);
                const bool share=scenario.contains("twelve") ? true : scenario.contains("overflow") ? ((owner==0 && slot>=2) || (owner==1 && slot==5)) :
                    scenario.contains("many") ? owner==0 && slot>=2 : slot==5;
                if(share) {
                    auto secure=pokemon.secure();monPut16(secure.data()+2,182);
                    if(scenario.contains("dex")) {monPut32(secure.data()+4,132650);secure[9]=255;}
                    if(scenario.contains("evolution")) {
                        monPut32(secure.data()+4,2534);pokemon.bytes[84]=15;
                        monPut16(pokemon.bytes.data()+86,41);monPut16(pokemon.bytes.data()+88,41);
                        for(unsigned offset=90;offset<100;offset+=2)monPut16(pokemon.bytes.data()+offset,25);
                    }
                    if(scenario.contains("learn") && owner==1 && (!scenario.contains("pinned") || slot==5)) {
                        monPut32(secure.data()+4,2034);pokemon.bytes[84]=14;
                        monPut16(pokemon.bytes.data()+86,38);monPut16(pokemon.bytes.data()+88,38);
                        for(unsigned offset=90;offset<100;offset+=2)monPut16(pokemon.bytes.data()+offset,24);
                        for(unsigned move=0;move<4;++move) {
                            monPut16(secure.data()+12+move*2,std::array<unsigned,4>{{33,45,73,22}}[move]);
                            secure[20+move]=10;
                        }
                    }
                    pokemon.setSecure(secure);
                }
            }
        }
        if(scenario.startsWith("exp-share-enigma")) {
            auto& holder=(scenario.endsWith("-leader2")?p0:p1).mons[5];
            auto secure=holder.secure();monPut16(secure.data()+2,EnigmaBerryItem);holder.setSecure(secure);
        }
        if(scenario.contains("exp-share-dex"))for(unsigned owner=0;owner<2;++owner) {
            auto* c=owner?b:a;const auto memory=readSaveMemory(c);auto progress=readProgress(c);
            progress.flag(0x840,owner==1);progress.var(0x404E,owner?0x6258:0);
            writeProgressDelta(c,readProgress(c),progress);c->busWrite8(c,memory.block2+0x1B,owner?0xB9:0);
        }
        if(scenario.contains("multiturn"))for(auto* party:{&p0,&p1}) {
            auto plain=party->mons[0].secure();monPut16(plain.data()+12,45);monPut16(plain.data()+14,33);
            plain[20]=40;plain[21]=35;party->mons[0].setSecure(plain);
        }
        if(scenario.startsWith("party-menu") || scenario.startsWith("reserves-roar")) {
            p0.count=p1.count=6;
            for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<6;++slot) {
                auto& pokemon=(owner?p1:p0).mons[slot];pokemon=mon(24*(owner*6+slot+1),(owner?trainer1:trainer0).id,25);
                if(slot==0) {auto plain=pokemon.secure();monPut16(plain.data()+12,45);plain[20]=40;pokemon.setSecure(plain);}
                if(scenario=="party-menu-exp-share" && slot==4) {
                    auto plain=pokemon.secure();monPut16(plain.data()+2,ExpShareItem);pokemon.setSecure(plain);
                }
                if(scenario=="party-menu-forced"&&slot<3)pokemon.setHp(1);
            }
        }
        if(scenario.startsWith("reserves-roar") || scenario.contains("wish")) {
            for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<6;++slot) {
                auto& pokemon=(owner?p1:p0).mons[slot];
                pokemon.bytes[8]=uint8_t(0xBB+owner*6+slot);pokemon.bytes[9]=0xFF;
                auto plain=pokemon.secure();monPut16(plain.data()+12,scenario.contains("wish")&&slot==0?273:45);
                monPut16(plain.data()+14,33);plain[20]=40;plain[21]=35;pokemon.setSecure(plain);
                if(scenario.contains("wish"))pokemon.setHp(slot?75:1);
            }
            if(scenario.contains("exhausted"))for(unsigned slot=1;slot<6;++slot)p0.mons[slot].setHp(0);
        }
        if(scenario=="party-menu-shedinja") {
            for(auto* p:{&p0,&p1}) {
                auto parent=p->mons[0].secure();monPut16(parent.data(),302);p->mons[0].setSecure(parent);
                auto shell=p->mons[5].secure();monPut16(shell.data(),303);
                monPut32(p->mons[5].bytes.data(),p->mons[0].identity().personality);
                p->mons[5].setSecure(shell);monPut16(p->mons[5].bytes.data()+86,1);monPut16(p->mons[5].bytes.data()+88,1);
            }
        }
        if(scenario=="bench") {
            p0.count=p1.count=6;
            for(unsigned owner=0;owner<2;++owner) for(unsigned slot=0;slot<6;++slot) {
                auto& pokemon=(owner?p1:p0).mons[slot];
                pokemon=mon(24*(owner*6+slot+1),(owner?trainer1:trainer0).id,25);
                if(slot<5) {
                    pokemon.setHp(1);auto plain=pokemon.secure();
                    monPut16(plain.data()+12,45);plain[20]=40;pokemon.setSecure(plain);
                }
            }
        }
        if(scenario=="payday") {
            for(auto* party:{&p0,&p1}) {auto plain=party->mons[0].secure();monPut16(plain.data()+12,6);plain[20]=20;party->mons[0].setSecure(plain);}
        }
        if(scenario.startsWith("capture")) {
            p0.count=p1.count=3;p0.mons[2]=mon(168,trainer0.id,25);p1.mons[2]=mon(192,trainer1.id,25);
            if(scenario=="capture-small") {p0.count=1;p0.mons[1]={};p0.mons[2]={};}
            const auto firstBall=[](Inventory& bag,unsigned item) {
                auto& entries=bag.pockets[PokeBalls];
                const auto found=std::find_if(entries.begin(),entries.begin()+PocketCapacities[PokeBalls],
                    [item](const ItemSlot& slot){return slot.item==item;});
                requirePokemon(found!=entries.begin()+PocketCapacities[PokeBalls],"Authored ball is missing");
                std::rotate(entries.begin(),found,found+1);
            };
            auto bag=readInventory(b);bag.setQuantity(1,2);firstBall(bag,1);writeInventory(b,bag);
            if(scenario=="capture-full-native" || scenario=="capture-party-space" || scenario=="capture-pc" || scenario=="capture-sequential") {
                auto full=readStorage(a);
                for(unsigned box=0;box<BoxCount;++box)for(unsigned slot=0;slot<BoxCapacity;++slot)
                    full.setMon(box,slot,BoxMon::fromParty(mon(10000+box*BoxCapacity+slot,trainer0.id,19)));
                writeStorage(a,full);
            }
            if(scenario=="capture-pc" || scenario=="capture-denied") {
                p1.count=6;for(unsigned slot=3;slot<6;++slot)p1.mons[slot]=mon(300+slot*24,trainer1.id,25);
            }
            if(scenario=="capture-denied" || scenario=="capture-party-space" || scenario=="capture-sequential") {
                auto full=readStorage(b);
                for(unsigned box=0;box<BoxCount;++box)for(unsigned slot=0;slot<BoxCapacity;++slot)
                    full.setMon(box,slot,BoxMon::fromParty(mon(20000+box*BoxCapacity+slot,trainer1.id,19)));
                writeStorage(b,full);
            }
            if(scenario.startsWith("capture-both") || scenario=="capture-sequential") {
                auto hostBag=readInventory(a);const auto item=scenario=="capture-sequential"?4u:1u;
                hostBag.setQuantity(item,2);firstBall(hostBag,item);writeInventory(a,hostBag);
            }
            for(unsigned owner=0;owner<2;++owner) {
                auto* c=owner?b:a;auto metadata=readPersonalCapture(c);
                metadata.captures=15+owner*12;metadata.pcBox=uint16_t(5+owner*4);metadata.shownBoxFull=true;
                writePersonalCapture(c,metadata);
                if(scenario=="capture-unown" || scenario=="capture-spinda")markSeen(c,captureSpecies,111+owner*111);
            }
        }
        if(scenario=="exhaust") {
            p0.count=1;p0.mons[1]={};p0.mons[0].setHp(1);
            for(auto* party:{&p0,&p1}) {
                auto plain=party->mons[0].secure();monPut16(plain.data()+12,45);monPut16(plain.data()+14,33);
                plain[20]=40;plain[21]=35;party->mons[0].setSecure(plain);
            }
        }
        if(scenario=="defeat") {
            p0.count=p1.count=1;p0.mons[1]=p1.mons[1]={};p0.mons[0].setHp(1);
            p1.mons[0]=mon(48,trainer1.id,25,true);p1.mons[0].setHp(1);
            for(auto* party:{&p0,&p1}) {
                auto plain=party->mons[0].secure();monPut16(plain.data()+12,45);plain[20]=40;party->mons[0].setSecure(plain);
            }
        }
        for(auto* party:{&p0,&p1})for(unsigned slot=0;slot<party->count;++slot) {
            const auto& identity=party==&p0?trainer0:trainer1;
            std::copy(identity.name.begin(),identity.name.end(),party->mons[slot].bytes.begin()+20);
        }
        const uint32_t initialMoney0=scenario=="trainer"?999998:1234;
        writeParty(a,p0);writeParty(b,p1);writeWallet(a,initialMoney0);writeWallet(b,9876);
        const auto originalBoxes0=readStorage(a),originalBoxes1=readStorage(b);
        const auto originalBag0=readInventory(a),originalBag1=readInventory(b);
        const std::array<DexEntry,2> originalDex{{readDexEntry(a,captureSpecies),readDexEntry(b,captureSpecies)}};
        const auto originalDex2b=readDexEntry(b,2), originalDex5a=readDexEntry(a,5);
        const auto originalCrobat0=readDexEntry(a,169);
        const std::array<PersonalCaptureState,2> originalPersonal{{readPersonalCapture(a),readPersonalCapture(b)}};
        const auto saveBlocks=[](mCore* c) {
            const auto m=readSaveMemory(c);
            return std::array<std::vector<uint8_t>,2>{{acceptance::save_detail::bytes(c,m.block1,SaveBlock1Size),acceptance::save_detail::bytes(c,m.block2,SaveBlock2Size)}};
        };
        const std::array<std::array<std::vector<uint8_t>,2>,2> beforeBlocks{{saveBlocks(a),saveBlocks(b)}};
        unsigned nativeThrows=0,capacityDenials=0;
        std::array<unsigned,2> nativeThrowsByOwner{};
        std::array<bool,2> nativeBallTextByOwner{};
        bool pendingThrowRollback=false;
        std::array<unsigned,2> benchRefills{};
        std::array<unsigned,2> evolutionStarts{};
        std::optional<std::pair<bool,uint32_t>> moneyText;
        bool moneyTextEncoded=false;
        const std::array<uint32_t,2> evolutionStatsBefore{{readGameStatistic(a,14),readGameStatistic(b,14)}};
        const std::array<uint32_t,2> splashStatsBefore{{readGameStatistic(a,26),readGameStatistic(b,26)}};
        const std::array<uint32_t,2> struggleStatsBefore{{readGameStatistic(a,27),readGameStatistic(b,27)}};
        std::array<std::array<bool,6>,2> benchEntered{};
        unsigned liveFrame=0;
        const auto nativeBreakpoint=static_cast<ARMCore*>(a->cpu)->irqh.bkpt16;
        std::array<unsigned,2> wishPicks{};
        std::array<unsigned,2> menuDeferrals{},menuDeferredAt{},menuPins{};
        std::array<unsigned,2> menuOpens{},menuCancels{},menuImports{},menuPicks{},menuAt{},menuActionAt{};
        std::array<bool,2> menuActionStarted{},menuScreens{},summaryScreens{};
        std::array<unsigned,2> summaryStage{},summaryAt{};
        bool menuRollback=false;
        TeamBattle pair({{a,b}});pair.event=[&](const char* text,int detail){
            fprintf(stdout,"%s: %d\n",text,detail);fflush(stdout);
            if(scenario.contains("wish") && (QString::fromLatin1(text)=="own resident selected" || QString::fromLatin1(text)=="own reserve selected")) {
                const unsigned owner=unsigned(detail)/6;
                requirePokemon(unsigned(detail)%6==(wishPicks[owner]?5u:1u),"Wish fixture selected wrong original");
                ++wishPicks[owner];menuActionStarted[owner]=false;
            }
            if(QString::fromLatin1(text)=="own reserve selection deferred") {++menuDeferrals[unsigned(detail)];menuDeferredAt[unsigned(detail)]=liveFrame;}
            if(scenario=="party-menu-protected" && QString::fromLatin1(text)=="own resident selected")++menuPicks[unsigned(detail)/6];
            if(QString::fromLatin1(text)=="own party menu opened") {++menuOpens[unsigned(detail)];menuAt[unsigned(detail)]=liveFrame;}
            if(QString::fromLatin1(text)=="own party menu canceled") {++menuCancels[unsigned(detail)/6];menuActionStarted[unsigned(detail)/6]=false;}
            if(QString::fromLatin1(text)=="own reserve selected") {requirePokemon(scenario.contains("wish") || scenario=="party-menu-cycle" || unsigned(detail)%6==5,"Native menu did not select own sixth original");++menuImports[unsigned(detail)/6];}
            if(scenario=="party-menu-cycle" && (QString::fromLatin1(text)=="own reserve selected" || QString::fromLatin1(text)=="own resident selected")) {
                const unsigned owner=unsigned(detail)/6;
                requirePokemon(unsigned(detail)%6==menuPicks[owner]+1,"Native cycle selected the wrong original");
                ++menuPicks[owner];menuActionStarted[owner]=false;
            }
            if(QString::fromLatin1(text)=="owner bench replenished" && detail>=0 && detail<2)++benchRefills[unsigned(detail)];
            if(QString::fromLatin1(text)=="owned evolution started" && detail>=0 && detail<2)++evolutionStarts[unsigned(detail)];
            if(QString::fromLatin1(text)=="native trainer money announced" || QString::fromLatin1(text)=="native payday share announced") {
                moneyText=std::make_pair(QString::fromLatin1(text)=="native payday share announced",uint32_t(detail));
                moneyTextEncoded=false;
            }
            if(QString::fromLatin1(text)=="native ball executed") {
                requirePokemon(detail==0 || detail==1,"Native throw event has an invalid owner");
                ++nativeThrows;++nativeThrowsByOwner[unsigned(detail)];
            }
            if(QString::fromLatin1(text)=="full recipient capture rejected")++capacityDenials;
            if(QString::fromLatin1(text).contains("mismatch")) {
                first.capture(out+"/before-error.png");
                fprintf(stderr,"Capture error outcome=%u cb=%08X count=%u storage=%08X\n",a->busRead8(a,OutcomeAddress),a->busRead32(a,MainCallbackAddress),a->busRead8(a,PlayerPartyCountAddress),a->busRead32(a,0x03004F60));
                for(unsigned slot=0;slot<6;++slot) {
                    PartyMon value;for(unsigned byte=0;byte<100;++byte)value.bytes[byte]=uint8_t(a->busRead8(a,PlayerPartyAddress+slot*100+byte));
                    fprintf(stderr,"slot%u pid=%08X ot=%08X checksum=%04X/%04X species=%u\n",slot,value.identity().personality,value.identity().trainer,monU16(value.bytes.data()+28),PartyMon::checksum(value.secure()),value.species());
                }
                const auto pc=a->busRead32(a,0x03004F60);PartyMon boxed;
                for(unsigned byte=0;byte<80;++byte)boxed.bytes[byte]=uint8_t(a->busRead8(a,pc+4+byte));
                fprintf(stderr,"pc0 pid=%08X ot=%08X checksum=%04X/%04X species=%u\n",boxed.identity().personality,boxed.identity().trainer,monU16(boxed.bytes.data()+28),PartyMon::checksum(boxed.secure()),boxed.species());
            }
        };
        unsigned completions=0,completedOutcome=0; bool peerNativeCompleted=false;
        pair.completed=[&](unsigned,unsigned,unsigned outcome,bool nativePeer){requirePokemon(outcome==1 || outcome==7 || scenario=="defeat" || (scenario.startsWith("escape")&&outcome==4),"Unexpected completed outcome");++completions;completedOutcome=outcome;peerNativeCompleted=nativePeer;};
        pair.sampleWorld();requirePokemon(pair.worldActors()[0].valid && pair.worldActors()[1].valid,"Checkpoint is not a playable overworld");
        first.capture(out+"/world.png",&pair.worldActors()[0],&pair.worldActors()[1]);
        // A complete machine/battery rollback must preserve both original teams,
        // wallet values and the second core's independent program counter.
        const unsigned initiator=(scenario=="leader2" || scenario.endsWith("-leader2") || scenario=="escape-leader2" || scenario=="single-leader2" || scenario=="single-multiturn-leader2" || scenario=="capture-leader2" || scenario=="capture-both-leader2" || scenario=="trainer-single-leader2")?1u:0u;
        auto* leading=initiator?b:a;
        if(scenario.endsWith("-launcher")) {
            // Private fixture export for the real process/UI acceptance path.
            // Only this opt-in test authors teams/items/map coordinates. The
            // production worker receives ordinary, natively written batteries.
            for(unsigned owner=0;owner<2;++owner) {
                auto* c=owner?b:a;const auto memory=readSaveMemory(c);
                inventoryWrite16(c,memory.block1,scenario=="soak-launcher"?12:14);
                inventoryWrite16(c,memory.block1+2,scenario=="soak-launcher"?38:26);
                c->busWrite8(c,memory.block1+4,3);c->busWrite8(c,memory.block1+5,19);
                c->busWrite8(c,0x03003478,0);c->busWrite32(c,MainCallbackAddress,0x0805673D);
                for(unsigned f=0;f<240;++f){c->setKeys(c,0);c->runFrame(c);}
                requirePokemon(campaignFieldQuiescent(c),"Launcher fixture did not enter Route 1");
                if(scenario=="soak-launcher" && owner==0) {
                    const auto layout=inventoryRead32(c,0x02036DFC);
                    const auto width=inventoryRead32(c,layout),height=inventoryRead32(c,layout+4);
                    const auto tiles=inventoryRead32(c,layout+12);
                    fprintf(stdout,"Route1 map=3/19 origin=12,38 size=%ux%u\n",width,height);
                    requirePokemon(width<=64 && height<=128,"Unexpected Route 1 fixture dimensions");
                    for(unsigned y=0;y<height;++y) {
                        fprintf(stdout,"%02u ",y);
                        for(unsigned x=0;x<width;++x)fprintf(stdout,"%04x ",c->busRead16(c,tiles+(y*width+x)*2));
                        fprintf(stdout,"\n");
                    }
                }
                const auto proof=acceptance::verifyNativeSaveReload(c,rom.toStdString());
                QFile file(out+QString("/player%1.sav").arg(owner+1));
                requirePokemon(file.open(QIODevice::WriteOnly|QIODevice::NewOnly) &&
                    file.write(reinterpret_cast<const char*>(proof.battery.data()),qint64(proof.battery.size()))==qint64(proof.battery.size()),"Cannot export private recovery fixture");
            }
            requirePokemon(hashFile(rom)==RomHash && hashFile(state)==stateHash,"Private inputs changed");
            const QJsonObject report{{"scenario",scenario},{"authored_parties_and_map_entry",true},{"private_native_battery_fixtures",true},{"rom_unchanged",true}};
            QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot write fixture report");
            result.write(QJsonDocument(report).toJson());return 0;
        }
        if(scenario=="rendezvous" || scenario=="grass-rendezvous" || scenario=="deferred-script") trigger(b,scenario);
        if(scenario=="grass-rendezvous")enemies(b,1,16);
        if(scenario=="deferred-script")for(unsigned owner=0;owner<2;++owner) {
            auto*c=owner?b:a;
            c->busWrite32(c,GlobalScriptContextAddress+8,0x08123456);
            c->busWrite8(c,GlobalScriptContextAddress,1);
            c->busWrite32(c,GlobalScriptContextAddress+12,owner?0x08125000:0x08124000);
        }
        trigger(leading,scenario);
        if(scenario.contains("exp-share-enigma")) {
            // Authored native single encounters, with actual unmodified game
            // execution after the user explicitly chooses individual battles.
            trigger(leading,"single");
            if(scenario.contains("both"))trigger(initiator?a:b,"single");
            auto snapshot=[](mCore* c){std::vector<uint8_t> data(c->stateSize(c));
                requirePokemon(c->saveState(c,data.data()),"Cannot snapshot pre-battle state");return data;};
            const auto beforeA=snapshot(a),beforeB=snapshot(b);
            const auto flashA=acceptance::save_detail::flash(a),flashB=acceptance::save_detail::flash(b);
            BattleRecovery recovery;
            requirePokemon(!recovery.tryBegin(pair,initiator) && recovery.paused(),"Expected a recoverable preflight pause");
            const auto pause=recovery.status();
            requirePokemon(pairState(pause)==(scenario.contains("enigma")?PairState::Enigma:PairState::ExpShare),"Wrong recovery explanation");
            for(unsigned f=0;f<120;++f)requirePokemon(!recovery.tryBegin(pair,initiator),"Paused battle advanced");
            requirePokemon(!pair.active() && snapshot(a)==beforeA && snapshot(b)==beforeB &&
                           acceptance::save_detail::flash(a)==flashA && acceptance::save_detail::flash(b)==flashB &&
                           readParty(a)==p0 && readParty(b)==p1 && readWallet(a)==initialMoney0 && readWallet(b)==9876,
                           "Recovery pause changed a machine, Flash, original party or unsaved wallet");
            requirePokemon(!recovery.accept(pause+256) && recovery.accept(pause) && !recovery.accept(pause),"Recovery consent was not scoped to this pause");
            std::array<bool,2> sawNative{};
            unsigned nativeFrames=0;
            for(;nativeFrames<12000 && recovery.individual();++nativeFrames) {
                for(unsigned owner=0;owner<2;++owner) {
                    auto* c=owner?b:a;
                    sawNative[owner]|=(c->busRead8(c,0x03003479)&2u)!=0;
                    c->setKeys(c,campaignFieldQuiescent(c)?0u:(nativeFrames%12<4?1u:0u));
                    c->runFrame(c);
                }
                recovery.returnToCoop(campaignFieldQuiescent(a) && campaignFieldQuiescent(b));
            }
            requirePokemon(recovery.status()==0 && sawNative[initiator] && (!scenario.contains("both") || sawNative[1-initiator]),"Native individual battles did not return to both fields");
            requirePokemon(readWallet(a)==initialMoney0 && readWallet(b)==9876,"Recovery lost pre-battle unsaved wallets");
            for(unsigned owner=0;owner<2;++owner) {
                const auto after=readParty(owner?b:a);const auto& before=owner?p1:p0;
                requirePokemon(after.count==before.count,"Recovery changed party capacity");
                for(unsigned slot=0;slot<after.count;++slot)
                    requirePokemon(after.mons[slot].identity()==before.mons[slot].identity() &&
                        after.mons[slot].heldItem()==before.mons[slot].heldItem(),"Recovery lost a Pokemon or held item");
                if(owner!=initiator && !scenario.contains("both"))requirePokemon(after==before,"Idle partner received the other player's battle changes");
            }
            const auto saved0=acceptance::verifyNativeSaveReload(a,rom.toStdString());
            const auto saved1=acceptance::verifyNativeSaveReload(b,rom.toStdString());
            // Model both trainers removing the held items after returning to
            // the field, then prove the next ordinary shared admission works.
            for(auto* c:{a,b}) {auto party=readParty(c);for(unsigned s=0;s<party.count;++s){auto plain=party.mons[s].secure();monPut16(plain.data()+2,0);party.mons[s].setSecure(plain);}writeParty(c,party);}
            pair.sampleWorld();trigger(leading,"single");
            requirePokemon(recovery.tryBegin(pair,initiator) && pair.active(),"Shared battles did not resume after resolving held items");pair.rollback();
            requirePokemon(hashFile(rom)==RomHash && hashFile(state)==stateHash,"Private inputs changed");
            const QJsonObject report{{"scenario",scenario},{"rom_sha256",RomHash},
                {"authored_test_parties",true},{"two_real_mgba_cores",true},
                {"unsupported_battle_configuration_rejected_before_conversion",true},
                {"both_machine_states_parties_and_wallets_unchanged",true},
                {"recoverable_pause_preserves_unsaved_progress_and_flash",true},
                {"explicit_individual_battles_return_to_coop",true},{"shared_admission_after_removing_items",true},
                {"both_players_native_save_and_fresh_continue",true},{"native_frames",int(nativeFrames)},
                {"save_generations",QJsonArray{int(saved0.generation),int(saved1.generation)}},
                {"battery_files_written",false},{"production_launcher",false},{"complete_world_coop",false}};
            QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot write overflow report");
            const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Cannot save overflow report");
            printf("%s",json.constData());return 0;
        }
        if(scenario=="scripted") {
            b->busWrite8(b,FieldControlsLockAddress,1);pair.sampleWorld();
            requirePokemon(!pair.tryBegin(initiator) && pair.waitingForPartner(initiator),"Busy partner did not hold scripted encounter");
            b->busWrite8(b,FieldControlsLockAddress,0);pair.sampleWorld();
        }
        requirePokemon(pair.tryBegin(initiator),"Could not join native battle");
        requirePokemon(readParty(leading).count==pair.partyTransaction().combined.count && readParty(initiator?a:b)==(initiator?p0:p1),"Combined team damaged the second core");
        for(unsigned f=0;f<20;++f)pair.runBattleFrame({{0,0}});
        pair.rollback();requirePokemon(readParty(a)==p0 && readParty(b)==p1,"Rollback changed a trainer's team");
        requirePokemon(readWallet(a)==initialMoney0 && readWallet(b)==9876,"Rollback changed a wallet");
        requirePokemon(pair.tryBegin(initiator),"Could not restart rolled-back encounter");
        acceptance::ReserveProbe reserveProbe(pair,leading,nativeBreakpoint,scenario.contains("pinned"),scenario.contains("pinned") || scenario.contains("wish") || scenario.startsWith("reserves-roar"));
        bool reserveEnemiesReady=false;
        std::array<std::optional<unsigned>,2> reserveFinishMenu;
        unsigned frames=0,owner0=0,owner1=0, captureStage=0, stageFrame=0;
        bool guestLearningStarted=false;
        // Observe the native opponent slots and the absent flag throughout an
        // authored single-wild battle, including the action menu.
        std::optional<std::array<unsigned,3>> singleOpponentProbe;
        bool singleAbsentObserved=false;
        unsigned initialEnemyPp=0,minimumEnemyPp=0,initialPhantomPp=0,minimumPhantomPp=0;
        bool opponentPpObserved=false;
        std::array<std::optional<unsigned>,2> finishingMoveMenu;
        std::array<bool,2> escapeMenuSeen{};
        std::array<unsigned,2> escapeMenuAt{};
        std::array<bool,2> escapeActionSeen{};
        auto captureKeys=[&]() -> std::array<uint32_t,2> {
            const auto pending=leading->busRead32(leading,ControllerExecAddress);
            const auto cmd0=leading->busRead8(leading,ControllerBufferA),cmd1=leading->busRead8(leading,ControllerBufferA+0x400);
            if(leading->busRead8(leading,OutcomeAddress)==7)return scenario.startsWith("capture-both")?std::array<uint32_t,2>{{frames%30<4?2u:0u,0}}:std::array<uint32_t,2>{{0,frames%30<4?2u:0u}};
            if(captureStage==0 && (pending&1u) && cmd0==18){captureStage=1;stageFrame=frames;}
            if(captureStage==1 && (scenario.startsWith("capture-both") || scenario=="capture-sequential")) {
                const auto elapsed=frames-stageFrame;
                if(cmd0==21 && (pending&1u)){captureStage=8;stageFrame=frames;}
                else return {{elapsed>=20 && elapsed<24?16u:(elapsed>=40 && elapsed<44?1u:0u),0}};
            }
            if(captureStage==8) {
                const auto elapsed=frames-stageFrame;
                if(elapsed>=120 && elapsed<124)return {{16,0}};
                if(elapsed>=170 && elapsed<174)return {{16,0}};
                if(elapsed>=220 && elapsed<224)return {{1,0}};
                if(elapsed>=270 && elapsed<274)return {{1,0}};
                if(elapsed>350){captureStage=3;stageFrame=frames;}
                else return {{0,0}};
            }
            if(captureStage==1 && cmd0==20 && (pending&1u)){captureStage=2;stageFrame=frames;}
            if(captureStage==2) {
                const auto elapsed=frames-stageFrame;
                if(elapsed>=60){captureStage=3;stageFrame=frames;}
                else return {{elapsed>=20 && elapsed<24?16u:(elapsed>=40 && elapsed<44?1u:0u),0}};
            }
            if(captureStage==3 && (pending&4u) && cmd1==18){captureStage=4;stageFrame=frames;}
            if(captureStage==4) {
                const auto elapsed=frames-stageFrame;
                if(cmd1==21 && (pending&4u)){captureStage=5;stageFrame=frames;}
                else return {{0,elapsed>=20 && elapsed<24?16u:(elapsed>=40 && elapsed<44?1u:0u)}};
            }
            if(captureStage==5) {
                const auto elapsed=frames-stageFrame;
                if(elapsed==100)first.capture(out+"/bag-before.png");
                if(elapsed==280)first.capture(out+"/bag-after.png");
                // Items -> key items -> balls; select the test Master Ball.
                if(elapsed>=120 && elapsed<124)return {{0,16}};
                if(elapsed>=170 && elapsed<174)return {{0,16}};
                if(elapsed>=220 && elapsed<224)return {{0,1}};
                if(elapsed>=270 && elapsed<274)return {{0,1}};
                if(elapsed>350){captureStage=6;stageFrame=frames;}
                else return {{0,0}};
            }
            if(scenario=="capture-denied" && captureStage==6) {
                if((pending&4u) && cmd1==18){captureStage=7;stageFrame=frames;}
                else {
                    const auto elapsed=frames-stageFrame;
                    return {{0,elapsed>=30 && elapsed<34?1u:(elapsed>60 && frames%30<4?2u:0u)}};
                }
            }
            if(scenario=="capture-denied" && captureStage==7) {
                const auto elapsed=frames-stageFrame;
                if(elapsed<40)return {{0,elapsed>=20 && elapsed<24?32u:0u}};
            }
            return {{frames%30<4?1u:0u,frames%30<4?1u:0u}};
        };
        while(pair.active() && frames<(scenario=="bench"?100000u:30000u)) {
            // Test-only automated A pulses choose the first move/target and
            // advance text. Production never chooses an action automatically.
            const unsigned keys=frames%30<4?1u:0u;
            auto inputs=scenario.startsWith("capture")?captureKeys():std::array<uint32_t,2>{{keys,keys}};
            if(scenario.startsWith("exp-share") && scenario.contains("learn")) {
                const auto pending=leading->busRead32(leading,ControllerExecAddress);
                for(unsigned battler:{0u,2u}) if(pending&(1u<<battler)) {
                    const auto buffer=ControllerBufferA+battler*0x200;
                    if(leading->busRead8(leading,buffer)==25) {
                        const auto nativeSlot=leading->busRead8(leading,buffer+1);
                        if(nativeSlot<6 && leading->busRead32(leading,PlayerPartyAddress+nativeSlot*100)==p1.mons[5].identity().personality)
                            guestLearningStarted=true;
                    }
                }
                // Once the guest's EXP update starts, the other person holds B
                // while the actual owner chooses A through the learning flow.
                if(guestLearningStarted)inputs={{frames%30<4?2u:0u,frames%30<4?1u:0u}};
            }
            if(scenario.contains("multiturn") && opponentPpObserved && initialEnemyPp>=minimumEnemyPp+2) {
                const auto pending=leading->busRead32(leading,ControllerExecAddress);
                for(unsigned owner=0;owner<2;++owner) {
                    const unsigned battler=owner*2;
                    if((pending&(1u<<battler)) && leading->busRead8(leading,ControllerBufferA+battler*0x200)==20) {
                        if(!finishingMoveMenu[owner])finishingMoveMenu[owner]=frames;
                        const unsigned age=frames-*finishingMoveMenu[owner];
                        inputs[owner]=age>=20&&age<24?16u:age>60&&frames%30<4?1u:0u;
                    }
                }
            }
            if(scenario.startsWith("escape")) {
                const int owner=pair.inputOwner();
                if(owner>=0) {
                    const unsigned player=unsigned(owner);
                    if(!escapeMenuSeen[player]) {escapeMenuSeen[player]=true;escapeMenuAt[player]=frames;}
                    const unsigned age=frames-escapeMenuAt[player];
                    inputs={{0,0}};
                    if(scenario=="escape-player2" && player==0) {
                        if(age>=20 && age<24)inputs[player]=1;        // fight
                        else if(age>=90 && age<94)inputs[player]=16;   // Growl
                        else if(age>=120 && age<124)inputs[player]=1;
                    } else {
                        if(age>=20 && age<24)inputs[player]=16;       // right
                        else if(age>=50 && age<54)inputs[player]=128; // down
                        else if(age>=80 && age<84)inputs[player]=1;   // A: FLUCHT
                    }
                }
            }
            if(scenario.startsWith("party-menu")) {
                const auto pending=leading->busRead32(leading,ControllerExecAddress);
                if(pair.ownPartyMenuActive()) {
                    const unsigned owner=*pair.ownPartyMenuOwner(),age=frames-menuAt[owner];
                    requirePokemon(leading->busRead8(leading,PlayerPartyCountAddress)==6,"Native own switch menu does not show all six Pokemon");
                    for(unsigned slot=0;slot<6;++slot) {
                        const auto pid=leading->busRead32(leading,PlayerPartyAddress+slot*100);
                        bool own=false;for(const auto& pokemon:(owner?p1:p0).mons)own|=pokemon.identity().personality==pid;
                        requirePokemon(own,"Native own switch menu exposed another human's Pokemon");
                    }
                    inputs={{0,0}};
                    if(age>=120&&!menuScreens[owner]){first.capture(out+"/own-six-player"+QString::number(owner+1)+".png");menuScreens[owner]=true;}
                    const unsigned pulse=frames%30<4?1u:0u;
                    if(scenario=="party-menu-protected" && menuDeferrals[owner]) {
                        const auto elapsed=frames-menuDeferredAt[owner];
                        if(elapsed==220)first.capture(out+"/protected-selection-player"+QString::number(owner+1)+".png");
                        // The native message returns automatically to selection.
                        // Extra A here would reopen the rejected mon's submenu.
                        if(elapsed>300)inputs[owner]=pulse?(leading->busRead8(leading,0x0203B0A9)==1u?1u:64u):0u;
                    } else if(scenario=="party-menu-summary"&&menuCancels[owner]==0) {
                        if(summaryStage[owner]==0&&age>120) {
                            if(leading->busRead8(leading,0x0203B0A9)!=5)inputs[owner]=pulse?128u:0u;
                            else if(pulse){inputs[owner]=1;summaryStage[owner]=1;summaryAt[owner]=frames;}
                        } else if(summaryStage[owner]==1) {
                            const auto elapsed=frames-summaryAt[owner];
                            if(elapsed>=40&&elapsed<44)inputs[owner]=128;
                            if(elapsed>=80&&elapsed<84)inputs[owner]=1;
                            if(elapsed>220){first.capture(out+"/summary-player"+QString::number(owner+1)+".png");summaryScreens[owner]=true;summaryStage[owner]=2;summaryAt[owner]=frames;}
                        } else if(summaryStage[owner]==2) {
                            const auto elapsed=frames-summaryAt[owner];if(elapsed>=20&&elapsed<24)inputs[owner]=2;
                            if(elapsed>150)summaryStage[owner]=3;
                        } else if(summaryStage[owner]==3)inputs[owner]=pulse?2u:0u;
                    } else if(scenario=="party-menu-cancel"&&menuCancels[owner]==0)inputs[owner]=age>120&&pulse?2u:0u;
                    else if(age>120)inputs[owner]=pulse?(leading->busRead8(leading,0x0203B0A9)==(scenario.contains("wish")?(wishPicks[owner]?5u:1u):scenario=="party-menu-cycle"?menuPicks[owner]+1:5u)?1u:128u):0u;
                } else if(scenario!="party-menu-forced") {
                    for(unsigned owner=0;owner<2;++owner) {
                        const unsigned battler=owner*2;
                        if((scenario.contains("wish")?(wishPicks[owner]==0 || wishPicks[owner]>=2):scenario=="party-menu-protected"?bool(menuPicks[owner]):scenario=="party-menu-cycle"?menuPicks[owner]>=5:bool(menuImports[owner])) || !(pending&(1u<<battler)) || leading->busRead8(leading,ControllerBufferA+battler*0x200)!=18)continue;
                        if(scenario=="party-menu-protected" && !menuPins[owner]) {
                            // Authored saturation fixture: both own bench slots
                            // hold live references. No production test API.
                            const auto& tx=pair.partyTransaction();unsigned wish=0;
                            for(unsigned slot=0;slot<tx.combined.count;++slot)if(tx.owners[slot]==int(owner) && tx.originalSlots[slot]!=0) {
                                leading->busWrite8(leading,0x02023F40+wish,2);leading->busWrite8(leading,0x02023F44+wish,uint8_t(slot));wish+=2;
                            }
                            requirePokemon(wish==4,"Protected menu fixture needs two own bench slots");menuPins[owner]=1;
                        }
                        if(!menuActionStarted[owner]){menuActionStarted[owner]=true;menuActionAt[owner]=frames;}
                        const unsigned elapsed=frames-menuActionAt[owner];
                        inputs[owner]=elapsed>=20&&elapsed<24?128u:elapsed>=40&&elapsed<44?1u:0u;
                    }
                }
            }
            if((scenario=="evolution" || scenario=="shedinja-cancel") && (evolutionStarts[0] || evolutionStarts[1])) {
                // Player one cancels their own Bulbasaur evolution. Their B
                // pulses continue through player two's scene and must be ignored.
                inputs={{frames%30<4?2u:0u,frames%30<4?1u:0u}};
            }
            if(scenario=="bench") {
                const auto pending=a->busRead32(a,ControllerExecAddress);
                for(unsigned battler:{0u,2u}) {
                    if((a->busRead8(a,0x03003479)&2u) && a->busRead32(a,MainCallbackAddress)==0x08011085u) {
                        const auto nativeSlot=a->busRead16(a,0x02023BCE + battler*2);
                        if(nativeSlot<6) {
                            const auto pid=a->busRead32(a,PlayerPartyAddress+nativeSlot*100);
                            const auto& original=battler?p1:p0;
                            for(unsigned i=0;i<original.count;++i)if(original.mons[i].identity().personality==pid)benchEntered[battler/2][i]=true;
                        }
                    }
                }
                for(unsigned battler:{0u,2u}) if((pending&(1u<<battler)) && a->busRead8(a,ControllerBufferA+battler*0x200)==22) {
                    // Read-only native party cursor; exercise ordinary menu
                    // buttons rather than injecting a chosen party index.
                    const auto slot=a->busRead8(a,0x0203B0A9);
                    bool own=false;
                    if(slot<6) {
                        const auto pid=a->busRead32(a,PlayerPartyAddress+slot*100);
                        const auto& original=battler?p1:p0;
                        for(unsigned i=0;i<original.count;++i)own|=original.mons[i].identity().personality==pid;
                    }
                    if(slot>=6 || !own || a->busRead16(a,PlayerPartyAddress+slot*100+86)==0)
                        inputs[battler/2]=frames%30<4?128u:0u;
                }
            }
            if(scenario=="exhaust") {
                const bool fainted=a->busRead16(a,PlayerPartyAddress+86)==0;
                if(fainted && (a->busRead32(a,ControllerExecAddress)&4) && a->busRead8(a,ControllerBufferA+0x400)==20 && a->busRead8(a,0x02023FFE)==0)inputs[1]=frames%30<4?16u:0u;
            }
            if(scenario.startsWith("reserves-roar") || scenario.contains("wish")) {
                const auto pending=leading->busRead32(leading,ControllerExecAddress);
                if(!reserveEnemiesReady && pending && leading->busRead8(leading,ControllerBufferA)==18) {
                    // Authored native enemy prerequisites; the original move,
                    // targeting, force-out, Wish and healing engines then run.
                    for(unsigned battler:{1u,3u}) {
                        const auto mon=0x02023BE4+battler*88;
                        leading->busWrite16(leading,mon+0xC,scenario.contains("wish")?(battler==1?89:45):scenario.contains("whirlwind")?18:46);
                        for(unsigned move=1;move<4;++move)leading->busWrite16(leading,mon+0xC+move*2,0);
                        leading->busWrite8(leading,mon+0x24,40);
                        leading->busWrite8(leading,mon+0x2A,scenario.contains("wish")?10:100);
                        leading->busWrite16(leading,mon+0x28,100);leading->busWrite16(leading,mon+0x2C,100);
                    }
                    reserveEnemiesReady=true;
                }
                const bool finish=scenario.contains("wish")?(reserveProbe.announcements[0] && reserveProbe.announcements[1]):
                    scenario.contains("exhausted")?(reserveProbe.forced[1]>=6 && reserveProbe.unloadedForced):
                    (reserveProbe.forced[0]>=3 && reserveProbe.forced[1]>=3 && reserveProbe.unloadedForced);
                if(finish)for(unsigned owner=0;owner<2;++owner) {
                    const unsigned battler=owner*2;
                    if((pending&(1u<<battler)) && leading->busRead8(leading,ControllerBufferA+battler*0x200)==20) {
                        if(!reserveFinishMenu[owner])reserveFinishMenu[owner]=frames;
                        const auto age=frames-*reserveFinishMenu[owner];
                        inputs[owner]=age>20 && keys?(leading->busRead8(leading,0x02023FFC+battler)==1?1u:16u):0u;
                    } else reserveFinishMenu[owner].reset();
                }
            }
            liveFrame=frames;pair.runBattleFrame(inputs);reserveProbe.check();
            if((scenario.startsWith("single") || scenario.startsWith("trainer-single") || scenario=="party-menu-single" ||
                scenario.startsWith("capture") || scenario.startsWith("escape")) && pair.active()) {
                const auto absent=leading->busRead8(leading,0x02023D70);
                if(absent & 8u)singleAbsentObserved=true;
                if(singleAbsentObserved) {
                    if(scenario.contains("multiturn")) {
                        unsigned real=0,phantom=0;
                        for(unsigned move=0;move<4;++move) {
                            real+=leading->busRead8(leading,0x02023BE4+88+0x24+move);
                            phantom+=leading->busRead8(leading,0x02023BE4+3*88+0x24+move);
                        }
                        if(!opponentPpObserved) {
                            initialEnemyPp=minimumEnemyPp=real;initialPhantomPp=minimumPhantomPp=phantom;
                            opponentPpObserved=true;
                        } else {minimumEnemyPp=std::min(minimumEnemyPp,real);minimumPhantomPp=std::min(minimumPhantomPp,phantom);}
                    }
                    requirePokemon((absent & 0xAu)==8u,"Single wild opponent became active twice");
                    requirePokemon(leading->busRead16(leading,0x02023BE4+3*88+0x28)==0 &&
                                   leading->busRead8(leading,0x02023BE4+3*88+0x20)==0 &&
                                   leading->busRead16(leading,0x02023BE4+3*88+0x2E)==0,
                                   "Absent wild opponent retained HP, ability or held item");
                }
            }
            if(scenario.startsWith("capture") && pair.active())for(unsigned owner=0;owner<2;++owner) {
                if(!nativeThrowsByOwner[owner])continue;
                const auto& name=owner?trainer1.name:trainer0.name;
                unsigned index=0;
                bool match=true;
                for(const auto character:name) {
                    if(character==0xFF)break;
                    match &= leading->busRead8(leading,0x0202298C+index++)==character;
                }
                // German native string: "<name> setzte\n<item> ein!".
                for(const uint8_t byte : {0x00,0xE7,0xD9,0xE8,0xEE,0xE8,0xFE})
                    match &= leading->busRead8(leading,0x0202298C+index++)==byte;
                if(match && !nativeBallTextByOwner[owner])
                    (initiator?second:first).capture(out+"/ball-name-player"+QString::number(owner+1)+".png");
                nativeBallTextByOwner[owner] |= match;
            }
            if(scenario.startsWith("escape") && pair.active())for(unsigned player=0;player<2;++player)
                if(escapeMenuSeen[player] && leading->busRead8(leading,0x02023D7C+player*2)==3)
                    escapeActionSeen[player]=true;
            if(scenario=="party-menu-rollback"&&!menuRollback&&pair.ownPartyMenuActive()&&frames-menuAt[*pair.ownPartyMenuOwner()]>150) {
                pair.rollback();requirePokemon(readParty(a)==p0&&readParty(b)==p1&&readWallet(a)==initialMoney0&&readWallet(b)==9876,"Rollback during temporary six-own menu damaged native teams or wallets");
                requirePokemon(!pair.ownPartyMenuActive()&&pair.tryBegin(initiator),"Cannot restart after temporary own-menu rollback");
                menuRollback=true;frames=owner0=owner1=liveFrame=0;menuOpens={};menuCancels={};menuImports={};menuAt={};menuActionAt={};menuActionStarted={};menuScreens={};continue;
            }
            if(moneyText) {
                const auto* announcement=&*moneyText;
                const unsigned width=announcement->first?2u:4u;
                const auto buffer=0x02022AB8u;
                uint32_t number=0;
                for(unsigned byte=0;byte<width;++byte)number|=leading->busRead8(leading,buffer+4+byte)<<(byte*8);
                if(leading->busRead8(leading,buffer)==0xFD && leading->busRead8(leading,buffer+1)==1 &&
                   leading->busRead8(leading,buffer+2)==width && number==announcement->second)moneyTextEncoded=true;
            }
            if(scenario.startsWith("capture") && scenario!="capture-denied" && nativeThrows && !pendingThrowRollback) {
                requirePokemon(pair.active(),"Capture completed before pending-throw rollback could be checked");
                pair.rollback();
                requirePokemon(readParty(a)==p0 && readParty(b)==p1 && readStorage(a)==originalBoxes0 && readStorage(b)==originalBoxes1 &&
                               readInventory(a)==originalBag0 && readInventory(b)==originalBag1,"Pending throw rollback damaged an original party, PC or bag");
                pendingThrowRollback=true;
                requirePokemon(pair.tryBegin(initiator),"Cannot restart rolled-back pending throw");
                frames=owner0=owner1=captureStage=stageFrame=nativeThrows=0;
                nativeThrowsByOwner={};nativeBallTextByOwner={};singleAbsentObserved=false;
                continue;
            }
            owner0+=pair.inputOwner()==0;owner1+=pair.inputOwner()==1;
            if((scenario.startsWith("single") || scenario.startsWith("trainer-single")) && pair.inputOwner()==0 && !singleOpponentProbe) {
                (initiator?second:first).capture(out+"/single-opponent-menu.png");
                singleOpponentProbe = std::array<unsigned,3>{{
                    leading->busRead16(leading,0x02023BCE + 2), leading->busRead16(leading,0x02023BCE + 6),
                    leading->busRead8(leading,0x02023D70)}};
            }
            if(frames==650)(initiator?second:first).capture(out+"/team-battle.png");
            ++frames;
        }
        if(pair.active()) {
            first.capture(out+"/unfinished.png");
            if(scenario=="bench") {
                std::vector<uint8_t> dump(a->stateSize(a));requirePokemon(a->saveState(a,dump.data()),"Cannot snapshot unfinished bench battle");
                QFile file(out+"/unfinished.state");file.open(QIODevice::WriteOnly|QIODevice::NewOnly);file.write(reinterpret_cast<const char*>(dump.data()),qint64(dump.size()));
                for(unsigned slot=0;slot<6;++slot) {
                    const auto mon=readParty(a).mons[slot];fprintf(stderr,"Benchslot%u pid=%u hp=%u move=%u pp=%u\n",slot,mon.identity().personality,mon.hp(),mon.move(0),mon.pp(0));
                }
                for(unsigned battler=0;battler<4;++battler)fprintf(stderr,"Battler%u hp=%u maxhp=%u move=%u pp=%u cursor=%u\n",battler,a->busRead16(a,0x02023BE4+battler*88+40),a->busRead16(a,0x02023BE4+battler*88+44),a->busRead16(a,0x02023BE4+battler*88+12),a->busRead8(a,0x02023BE4+battler*88+36),a->busRead8(a,0x02023FFC+battler));
            }
            fprintf(stderr,"Unfinished: cb=%08X main=%08X inbattle=%02X outcome=%u exec=%08X cmd0=%u cmd2=%u owners=%u/%u party=%u\n",
                a->busRead32(a,MainCallbackAddress),a->busRead32(a,0x03004ED4),a->busRead8(a,0x03003479),
                a->busRead8(a,OutcomeAddress),a->busRead32(a,ControllerExecAddress),a->busRead8(a,ControllerBufferA),a->busRead8(a,ControllerBufferA+0x400),owner0,owner1,readParty(a).count);
        }
        requirePokemon(!pair.active(),"Native battle did not finish");
        if(scenario=="deferred-script") {
            requirePokemon(!peerNativeCompleted && b->busRead32(b,MainCallbackAddress)==InitBattleCallback &&
                b->busRead32(b,GlobalScriptContextAddress+8)==0x08123456 &&
                b->busRead32(b,GlobalScriptContextAddress+12)==0x08125000,
                "Different scripted object consumed the first encounter result");
            for(unsigned f=0;f<1800 && !campaignFieldQuiescent(a);++f){a->setKeys(a,0);a->runFrame(a);}
            pair.sampleWorld();requirePokemon(pair.tryBegin(1),"Deferred scripted encounter did not become the next shared battle");
            unsigned nextFrames=0;
            while(pair.active() && nextFrames<30000){unsigned key=nextFrames%30<4?1u:0u;pair.runBattleFrame({{key,key}});++nextFrames;}
            requirePokemon(!pair.active(),"Deferred scripted battle did not finish");frames+=nextFrames;
        }
        requirePokemon(completions==(scenario=="deferred-script"?2u:1u) &&
            peerNativeCompleted==(scenario=="rendezvous" || scenario=="grass-rendezvous"),
            "Native completion was missing, duplicated or lost peer continuation");
        if(scenario=="marowak" || scenario=="scripted" || scenario=="legendary")
            requirePokemon(leading->busRead32(leading,BattleFlagsAddress)==(scenario=="marowak"?0xA000u:scenario=="scripted"?0x20000u:0x42000u),"Native scripted battle kind was not restored");
        requirePokemon(owner0 && owner1,"Both logical players did not receive owned requests");
        if(scenario.startsWith("capture")) {
            QJsonArray deltas;
            for(unsigned owner=0;owner<2;++owner) {
                const auto after=saveBlocks(owner?b:a);
                for(unsigned block=0;block<2;++block)for(unsigned start=0;start<after[block].size();) {
                    if(beforeBlocks[owner][block][start]==after[block][start]){++start;continue;}
                    unsigned end=start+1;
                    while(end<after[block].size() && beforeBlocks[owner][block][end]!=after[block][end])++end;
                    deltas.append(QJsonObject{{"player",int(owner+1)},{"save_block",int(block+1)},{"offset",int(start)},{"length",int(end-start)}});start=end;
                }
            }
            QFile detail(out+"/capture-saveblock-deltas.json");requirePokemon(detail.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot write private capture saveblock delta report");
            detail.write(QJsonDocument(deltas).toJson());
            const auto describe=[](const PersonalCaptureState& state) {
                return QJsonObject{{"captures",int(state.captures)},{"pc_box_variable",int(state.pcBox)},
                    {"shown_box_full",state.shownBoxFull},{"unown_pid",double(state.unown)},{"spinda_pid",double(state.spinda)}};
            };
            QJsonArray semantic;
            for(unsigned owner=0;owner<2;++owner)semantic.append(QJsonObject{{"player",int(owner+1)},
                {"before",describe(originalPersonal[owner])},{"after",describe(readPersonalCapture(owner?b:a))}});
            QFile meaning(out+"/capture-personal-before-after.json");requirePokemon(meaning.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot write private semantic capture delta report");
            meaning.write(QJsonDocument(semantic).toJson());
        }
        reserveProbe.detach();
        if(scenario.contains("pinned"))requirePokemon(reserveProbe.saturated && reserveProbe.restored &&
            (!scenario.contains("learn") || reserveProbe.awardReshows>0),"Pinned EXP award did not restore after native summary reshow");
        if(scenario.contains("pinned") && scenario.contains("learn")) {
            const auto recipient=readParty(b).mons[5];bool learned=false;
            for(unsigned move=0;move<4;++move)learned|=recipient.move(move)==77 || recipient.move(move)==79;
            requirePokemon(guestLearningStarted && recipient.level()==15 && learned,"Borrowed recipient did not retain its own newly learned move");
        }
        if(scenario.startsWith("reserves-roar"))requirePokemon(reserveProbe.unloadedForced &&
            (scenario.contains("exhausted")?reserveProbe.forced[0]==0 && reserveProbe.forced[1]>=6:
             reserveProbe.forced[0]>=3 && reserveProbe.forced[1]>=3),"Both owners' forced original reserve choices were not exercised");
        if(scenario.contains("wish"))requirePokemon(reserveProbe.sourceEvicted[0] && reserveProbe.sourceEvicted[1] &&
            reserveProbe.announcements[0] && reserveProbe.announcements[1],"Wish did not retain both evicted source names");
        auto r0=readParty(a),r1=readParty(b);
        const auto shedAddition=[&](unsigned owner){return scenario.startsWith("shedinja") && !scenario.contains("full") && !(scenario=="shedinja-cancel" && owner==0)?(scenario.contains("multiple")?2u:1u):0u;};
        requirePokemon(r0.count==p0.count+shedAddition(0)+(scenario.startsWith("capture-both")?1:0) && r1.count==p1.count+shedAddition(1)+(scenario.startsWith("capture") && scenario!="capture-pc" && scenario!="capture-denied" && !scenario.startsWith("capture-both")?1:0),"Battle changed original party sizes");
        for(unsigned s=0;s<p0.count;++s)requirePokemon(r0.mons[s].identity()==p0.mons[s].identity(),"Battle swapped player-one Pokemon ownership");
        for(unsigned s=0;s<p1.count;++s)requirePokemon(r1.mons[s].identity()==p1.mons[s].identity(),"Battle swapped player-two Pokemon ownership");
        if(scenario.startsWith("exp-share") && !scenario.contains("many") && !scenario.contains("twelve") && !scenario.contains("overflow")) {
            for(unsigned owner=0;owner<2;++owner) {
                const auto& before=owner?p1:p0;const auto& after=owner?r1:r0;
                requirePokemon(after.mons[5].experience()>before.mons[5].experience() && after.mons[5].heldItem()==182,
                               "Sixth owned EXP Share holder lost its native award or item");
                const auto oldEV=before.mons[5].secure(),newEV=after.mons[5].secure();
                requirePokemon(!std::equal(oldEV.begin()+24,oldEV.begin()+30,newEV.begin()+24),
                               "Sixth owned EXP Share holder did not receive native EVs");
                requirePokemon(after.mons[3]==before.mons[3] && after.mons[4]==before.mons[4],
                               "Unloaded nonparticipant changed during EXP Share award");
            }
            if(scenario.contains("learn")) {
                bool learned=false;
                for(unsigned move=0;move<4;++move)learned |= r1.mons[5].move(move)==77 || r1.mons[5].move(move)==79;
                requirePokemon(guestLearningStarted && r1.mons[5].level()==15 && learned,
                               "Actual owner could not learn a new move while the other player pressed B");
            } else if(!scenario.contains("evolution"))
                requirePokemon(r0.mons[5].experience()-p0.mons[5].experience()==r1.mons[5].experience()-p1.mons[5].experience(),
                               "Equivalent owned EXP Share recipients received different awards");
            else {
                requirePokemon(evolutionStarts[0]==1 && evolutionStarts[1]==1 &&
                               r0.mons[5].level()==16 && r0.mons[5].species()==2 &&
                               r1.mons[5].level()==16 && r1.mons[5].species()==5,
                               "Native EXP Share evolutions did not complete for both original owners");
                requirePokemon(readDexEntry(a,2).isCaught() && readDexEntry(b,5).isCaught() &&
                               readDexEntry(a,5).isCaught()==originalDex5a.isCaught() &&
                               readDexEntry(b,2).isCaught()==originalDex2b.isCaught(),
                               "EXP Share evolution ownership leaked between trainers");
            }
        }
        if(scenario.contains("exp-share-twelve") || scenario.contains("exp-share-overflow")) {
            for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<6;++slot) {
                const auto& before=(owner?p1:p0).mons[slot];const auto& after=(owner?r1:r0).mons[slot];
                const bool recipient=slot==0 || before.heldItem()==ExpShareItem;
                requirePokemon(after.heldItem()==before.heldItem() && (recipient?after.experience()>before.experience():after==before),
                               "Virtual experience omitted or changed an owned recipient");
                if(recipient) {
                    const auto x=before.secure(),y=after.secure();
                    requirePokemon(y[27]==x[27]+2,"Virtual recipient did not receive both native Speed EVs");
                }
                if(scenario.contains("twelve") && !scenario.contains("evolution"))
                    requirePokemon(after.experience()-before.experience()==(slot?6u:45u),"Twelve-holder native EXP split is incorrect");
                if(scenario.contains("evolution"))
                    requirePokemon(after.level()==16 && after.species()==(owner?5u:2u),"A queued virtual evolution was lost");
            }
            if(scenario.contains("evolution"))requirePokemon(evolutionStarts[0]==6 && evolutionStarts[1]==6,"All twelve native evolution scenes must complete");
        }
        if(scenario.contains("exp-share-many")) {
            for(unsigned slot=2;slot<6;++slot) {
                const auto before=p0.mons[slot].secure(),after=r0.mons[slot].secure();
                requirePokemon(r0.mons[slot].experience()>p0.mons[slot].experience() && r0.mons[slot].heldItem()==182 &&
                               !std::equal(before.begin()+24,before.begin()+30,after.begin()+24),
                               "A later owned EXP Share holder did not receive its native EXP and EV award");
            }
            requirePokemon(r0.mons[1]==p0.mons[1],"Unloaded nonparticipant changed during multiple EXP Share awards");
            for(unsigned slot=1;slot<6;++slot)requirePokemon(r1.mons[slot]==p1.mons[slot],
                "Multiple EXP Share awards changed the other trainer's unloaded party");
        }
        const bool pp0=r0.mons[0].pp(0)<p0.mons[0].pp(0),pp1=r1.mons[0].pp(0)<p1.mons[0].pp(0);
        if(!scenario.startsWith("capture") && !scenario.startsWith("escape") && scenario!="exhaust" && scenario!="defeat" && scenario!="bench" && scenario!="action-stats" && !scenario.startsWith("party-menu")) {
            requirePokemon((scenario.startsWith("single") || scenario.startsWith("trainer-single") || scenario=="grass-rendezvous" || scenario=="deferred-script" || scenario=="marowak" || scenario=="scripted" || scenario=="legendary")?(pp0||pp1):(pp0&&pp1),"Native attacks did not consume the acting players' PP");
            requirePokemon(r0.mons[0].experience()>p0.mons[0].experience() && r1.mons[0].experience()>p1.mons[0].experience(),"Native battle experience was not transferred to both owners");
        } else if(scenario.startsWith("capture")) {
            if(scenario=="capture-denied") {
                requirePokemon(capacityDenials && !nativeThrows,"Full recipient was not rejected through the native UI");
                requirePokemon(readStorage(b)==originalBoxes1 && readInventory(b).quantity(1)==2,"Full actual owner was allowed to spend a ball or change storage");
                requirePokemon(readDexEntry(b,19).isCaught()==originalDex[1].isCaught(),"Full owner received caught dex flag");
            } else if(scenario.startsWith("capture-both")) {
                requirePokemon(nativeThrows==1,"Both queued balls executed despite the first catching successfully");
                requirePokemon(r0.mons[3].species()==19 && r0.mons[3].identity().trainer==trainer0.id,"First actually executed ball did not own capture");
                requirePokemon(readInventory(a).quantity(1)==1 && readInventory(b).quantity(1)==2,"Unexecuted queued ball was spent");
            } else if(scenario=="capture-sequential") {
                requirePokemon(nativeThrows==2 && nativeThrowsByOwner[0]==1 && nativeThrowsByOwner[1]==1,"Sequential capture did not execute one native ball action per owner");
                requirePokemon(r1.mons[3].species()==150 && r1.mons[3].identity().trainer==trainer1.id,"Second actually executed ball did not own capture");
                requirePokemon(readInventory(a).quantity(4)==1 && readInventory(b).quantity(1)==1,"Sequential balls were not spent once per owner");
                requirePokemon(readDexEntry(b,150).isCaught() && readDexEntry(a,150).isCaught()==originalDex[0].isCaught() && readDexEntry(a,150).isSeen(),"Sequential capture dex ownership is incorrect");
            } else if(scenario=="capture-pc") {
                const auto caught=captureAddition(originalBoxes1,readStorage(b));
                requirePokemon(caught && caught->pokemon.asPartyRecord().species()==19 && caught->pokemon.identity().trainer==trainer1.id,"PC capture did not belong to actual ball user");
                requirePokemon(caught->pokemon.asPartyRecord().pp(0)==35 && caught->pokemon.asPartyRecord().pp(1)==40,"Actual PC capture did not retain native PP restoration");
            } else requirePokemon(r1.mons[3].species()==captureSpecies && r1.mons[3].identity().trainer==trainer1.id,"Capture did not belong to actual ball user");
            requirePokemon(readStorage(a)==originalBoxes0,"Native leader PC journal was not completely restored");
            if(scenario!="capture-pc")requirePokemon(readStorage(b)==originalBoxes1,"Party capture changed the recipient's original PC");
            for(unsigned owner=0;owner<2;++owner) {
                auto* c=owner?b:a;const auto metadata=readPersonalCapture(c);
                const bool actualCatcher=scenario!="capture-denied" && owner==(scenario.startsWith("capture-both")?0u:1u);
                const auto expected=actualCatcher?planPersonalCapture(originalPersonal[owner],scenario=="capture-pc"?std::optional<unsigned>(captureAddition(originalBoxes1,readStorage(b))->box):std::nullopt,(owner?originalBoxes1:originalBoxes0).currentBox()):originalPersonal[owner];
                requirePokemon(metadata.captures==expected.captures && metadata.pcBox==expected.pcBox && metadata.shownBoxFull==expected.shownBoxFull,"Capture personal stat or PC metadata leaked to another owner");
                if(scenario=="capture-unown-first") {
                    const auto entry=readDexEntry(c,captureSpecies);
                    requirePokemon(entry.personality==72 && entry.isSeen() && entry.isCaught()==actualCatcher,"First native Unown encounter did not keep owner-specific dex state");
                }
                if(scenario=="capture-unown" || scenario=="capture-spinda") {
                    const auto entry=readDexEntry(c,captureSpecies);
                    const auto expectedDex=planDexUpdate(originalDex[owner],72,actualCatcher).entry;
                    requirePokemon(entry.personality==expectedDex.personality,"Guest capture overwrote the other trainer's original dex form");
                }
            }
            if(scenario!="capture-denied" && scenario!="capture-pc") {
                const auto caught=scenario.startsWith("capture-both")?r0.mons[3]:r1.mons[3];
                requirePokemon(caught.pp(0)==7 && caught.pp(1)==11,"Temporary native PC storage healed an actual party capture's PP");
            }
            if(scenario!="capture-denied" && !scenario.startsWith("capture-both") && scenario!="capture-sequential") {
                if(readInventory(b).quantity(1)+1!=originalBag1.quantity(1) || readInventory(a).quantity(1)!=originalBag0.quantity(1)) {
                    for(unsigned item=1;item<=12;++item)fprintf(stderr,"Ball %u owner1 %u->%u owner2 %u->%u\n",item,
                        originalBag0.quantity(item),readInventory(a).quantity(item),originalBag1.quantity(item),readInventory(b).quantity(item));
                }
                requirePokemon(readInventory(b).quantity(1)+1==originalBag1.quantity(1) &&
                               readInventory(a).quantity(1)==originalBag0.quantity(1),
                               "Ball consumed from the wrong bag");
                requirePokemon(readDexEntry(b,captureSpecies).isCaught() &&
                    readDexEntry(a,captureSpecies).isCaught()==originalDex[0].isCaught() &&
                    readDexEntry(a,captureSpecies).isSeen(),"Capture dex ownership is incorrect");
            }
        } else if(scenario=="exhaust") {
            requirePokemon(r0.mons[0].hp()==0 && r1.mons[0].hp()>0 && r1.mons[0].pp(1)<p1.mons[0].pp(1),"Partner did not complete the battle after player-one exhaustion");
        }
        if(scenario=="party-menu-protected") {
            for(unsigned owner=0;owner<2;++owner) {
                requirePokemon(menuDeferrals[owner]==1 && menuPicks[owner]==1 && menuImports[owner]==0 && menuOpens[owner]==1,
                               "Protected selection did not stay in the same menu and accept a resident alternative");
                const auto& before=owner?p1:p0;const auto& after=owner?r1:r0;
                requirePokemon(after.mons[1].experience()>before.mons[1].experience() && after.mons[1].pp(0)<before.mons[1].pp(0),
                               "Selected resident did not complete the battle");
                requirePokemon(after.mons[5]==before.mons[5],"Rejected reserve was mutated");
            }
        } else if(scenario=="party-menu-cycle") {
            for(unsigned owner=0;owner<2;++owner) {
                requirePokemon(menuPicks[owner]==5,"All own originals were not selected natively");
                for(unsigned slot=0;slot<6;++slot) {
                    const auto& before=(owner?p1:p0).mons[slot];const auto& after=(owner?r1:r0).mons[slot];
                    requirePokemon(after.experience()-before.experience()==12,"Retired living participant lost or duplicated its native award");
                }
            }
        } else if(scenario.contains("wish")) {
            for(unsigned owner=0;owner<2;++owner) {
                const auto& before=owner?p1:p0;const auto& after=owner?r1:r0;
                requirePokemon(wishPicks[owner]==2 && after.mons[0].pp(0)==before.mons[0].pp(0)-1 &&
                    after.mons[5].pp(1)<before.mons[5].pp(1) && after.mons[5].experience()>before.mons[5].experience(),
                    "Native Wish/forced and voluntary switches did not complete for both owners");
            }
        } else if(scenario.startsWith("party-menu")) {
            if(scenario=="party-menu-single")
                requirePokemon(singleAbsentObserved,"Single opponent was not held absent through own-party menus");
            for(unsigned owner=0;owner<2;++owner) {
                requirePokemon(menuOpens[owner]&&menuScreens[owner]&&menuImports[owner]==1,"Both native own-six menus did not import the sixth original exactly once");
                const auto& result=owner?r1:r0;const auto& original=owner?p1:p0;
                requirePokemon(result.mons[5].pp(0)<original.mons[5].pp(0)&&result.mons[5].experience()>original.mons[5].experience(),"Selected sixth original did not actually fight and receive own EXP");
                requirePokemon(result.mons[1]==original.mons[1]&&result.mons[2]==original.mons[2],"Retired living bench record changed during free selection");
                if(scenario=="party-menu-exp-share")requirePokemon(result.mons[4].heldItem()==ExpShareItem &&
                    result.mons[4].experience()>original.mons[4].experience(),
                    "Selecting an unloaded reserve evicted the owned EXP Share holder before its native award");
                if(scenario=="party-menu-cancel"||scenario=="party-menu-summary")requirePokemon(menuCancels[owner]==1&&menuOpens[owner]==2,"Native cancel or reopened selection count incorrect");
            }
            if(scenario=="party-menu-summary")requirePokemon(summaryScreens[0]&&summaryScreens[1],"Both native owned summaries were not visited before cancellation");
            requirePokemon(!pair.ownPartyMenuActive(),"Temporary own party survived battle completion");
            if(scenario=="party-menu-rollback")requirePokemon(menuRollback,"Temporary own-menu rollback was not exercised");
        }
        if(scenario=="action-stats") {
            requirePokemon(readGameStatistic(a,26)==splashStatsBefore[0]+1 && readGameStatistic(b,26)==splashStatsBefore[1],"Splash statistic was granted to the wrong owner");
            requirePokemon(readGameStatistic(a,27)==struggleStatsBefore[0] && readGameStatistic(b,27)==struggleStatsBefore[1]+1,"Struggle statistic was granted to the wrong owner");
            requirePokemon(r0.mons[0].pp(0)==p0.mons[0].pp(0)-1 && r1.mons[0].hp()<p1.mons[0].hp(),"Native Splash PP or Struggle recoil was not transferred");
        }
        if(scenario=="evolution") {
            fprintf(stderr,"Evolved statistics before=%u/%u after=%u/%u\n",evolutionStatsBefore[0],evolutionStatsBefore[1],readGameStatistic(a,14),readGameStatistic(b,14));
            requirePokemon(readGameStatistic(a,14)==evolutionStatsBefore[0] && readGameStatistic(b,14)==std::min(evolutionStatsBefore[1]+1,MaximumBattleStatistic),"Evolution statistics were granted to the wrong owner");
            requirePokemon(evolutionStarts[0]==1 && evolutionStarts[1]==1,"Native evolution did not identify both holders");
            requirePokemon(r0.mons[0].level()==16 && r0.mons[0].species()==1,"Owner could not cancel own evolution");
            requirePokemon(r1.mons[0].level()==16 && r1.mons[0].species()==5,"Another owner's B canceled guest evolution");
            requirePokemon(readDexEntry(b,5).isCaught() && readDexEntry(a,5).isCaught()==originalDex5a.isCaught() && readDexEntry(a,5).isSeen(),"Evolution dex ownership leaked to native leader");
        }
        if(scenario=="bench") {
            for(unsigned owner=0;owner<2;++owner) {
                const auto& result=owner?r1:r0;const auto& original=owner?p1:p0;
                for(unsigned slot=0;slot<6;++slot)fprintf(stderr,"Owner%u original%u entered=%u hp=%u pp=%u pid=%u\n",owner,slot,benchEntered[owner][slot],result.mons[slot].hp(),result.mons[slot].pp(0),result.mons[slot].identity().personality);
                requirePokemon(benchRefills[owner]>=3,"Owner's fourth, fifth and sixth Pokemon were never loaded");
                for(unsigned slot=0;slot<5;++slot)requirePokemon(result.mons[slot].hp()==0 && benchEntered[owner][slot],"First five own Pokemon did not enter battle and faint");
                requirePokemon(result.mons[5].pp(0)<original.mons[5].pp(0) && benchEntered[owner][5],"Sixth own Pokemon did not battle");
            }
        }
        if(scenario=="trainer") {
            const auto prize=nativeTrainerPrize(a,89,8,1);
            requirePokemon(readWallet(a)==999999 && readWallet(b)==9876+prize/2,"Trainer money was inflated or truncated for the other player");
            requirePokemon(moneyText && moneyTextEncoded && moneyText->second==prize/2,
                           "Native prize announcement did not match the player's share");
        }
        if(scenario=="payday") {
            requirePokemon(readWallet(a)>initialMoney0 && readWallet(a)-initialMoney0==readWallet(b)-9876,"Pay Day was not shared evenly");
            requirePokemon(moneyText && moneyText->first && moneyTextEncoded && moneyText->second==readWallet(a)-initialMoney0,
                           "Native Pay Day announcement did not match the player's share");
        }
        if(scenario=="owners" || scenario=="leader2")requirePokemon(r0.mons[0].experience()-p0.mons[0].experience()==r1.mons[0].experience()-p1.mons[0].experience(),"Guest-owned Pokemon received a false traded experience bonus");
        if(scenario.startsWith("pokerus")) {
            for(unsigned owner=0;owner<2;++owner)for(unsigned slot=0;slot<6;++slot) {
                const auto& before=(owner?p1:p0).mons[slot];const auto& after=(owner?r1:r0).mons[slot];
                requirePokemon(after.pokerus()==before.pokerus(),"Pokerus status spread or changed across ownership");
                requirePokemon(after.secure()[27]-before.secure()[27]==(before.pokerus()?4:2),"Native active/cured Pokerus EV bonus lost or transferred");
                requirePokemon(after.experience()-before.experience()==(slot?6u:45u),"Pokerus changed native EXP split");
            }
        }
        if(scenario.startsWith("shedinja")) {
            for(unsigned owner=0;owner<2;++owner) {
                const auto& result=owner?r1:r0;
                const unsigned firstParent=scenario.contains("multiple")?2:4;
                const unsigned parentCount=scenario.contains("multiple")?2:1;
                requirePokemon(evolutionStarts[owner]==parentCount,"Native evolution scene count is incorrect");
                for(unsigned n=0;n<parentCount;++n) {
                    requirePokemon(result.mons[firstParent+n].species()==(scenario=="shedinja-cancel" && owner==0?301:302),
                                   "Native Nincada evolution/cancel ownership failed");
                    if(shedAddition(owner)) {
                        const unsigned shell=(owner?p1:p0).count+n;
                        requirePokemon(nativeShedinjaSiblings(result.mons[firstParent+n],result.mons[shell]) && result.mons[shell].hp()==1 &&
                                       result.mons[shell].heldItem()==0 && result.mons[firstParent+n].heldItem()==ExpShareItem,
                                       "Native Shedinja was not appended to its own party");
                        requirePokemon(readDexEntry(owner?b:a,303).isCaught(),"Shedinja caught receipt missing from owner");
                    }
                }
            }
            if(scenario=="shedinja-cancel")requirePokemon(!readDexEntry(a,303).isCaught() && readDexEntry(a,303).isSeen(),
                "Guest Shedinja leaked caught ownership to canceling leader");
            requirePokemon(readInventory(a)==originalBag0 && readInventory(b)==originalBag1,"FireRed Shedinja consumed an item");
        }
        (initiator?second:first).capture(out+"/committed.png");
        bool saveReload=false;
        std::array<unsigned,2> savedGenerations{};
        {
            for(unsigned player=0;player<2;++player) {
                auto* c=player?b:a;
                unsigned settling=0;
                while(settling++<((scenario=="defeat" || scenario=="exhaust")?10000u:1800u)){
                    if(acceptance::save_detail::fieldReady(c) && !recoverExhaustedTrainer(c))break;
                    c->setKeys(c,c->busRead8(c,0x03000F9C) && settling%30<4?2:0);c->runFrame(c);
                }
                if(!acceptance::save_detail::fieldReady(c)) {
                    std::vector<uint8_t> failed(c->stateSize(c));requirePokemon(c->saveState(c,failed.data()),"Cannot capture failed field state");
                    QFile file(out+"/failed-field.state");requirePokemon(file.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot create private failed field checkpoint");file.write(reinterpret_cast<const char*>(failed.data()),qint64(failed.size()));
                    (player?second:first).capture(out+"/field-not-ready-player"+QString::number(player)+".png");
                    fprintf(stderr,"Field notready player%u cb=%08X state=%u lock=%u money=%u hp=%u\n",player,c->busRead32(c,MainCallbackAddress),c->busRead8(c,0x03003478),c->busRead8(c,0x03000F9C),readWallet(c),readParty(c).mons[0].hp());
                }
                requirePokemon(acceptance::save_detail::fieldReady(c),"Committed battle did not return to a saveable field");
                const auto proof=acceptance::verifyNativeSaveReload(c,rom.toUtf8().toStdString());
                savedGenerations[player]=proof.generation;
            }
            saveReload=true;
        }
        if(scenario=="defeat")requirePokemon(readWallet(a)==834 && readWallet(b)==9796 && readParty(a).mons[0].hp()>0 && readParty(b).mons[0].hp()>0,"Blackout did not use each owner's own party level and wallet");
        if(scenario=="exhaust")requirePokemon(readWallet(a)==834 && readWallet(b)==9876 && readParty(a).mons[0].hp()>0,"Exhausted partner recovery damaged the surviving trainer's wallet");
        if(scenario.startsWith("escape")) {
            requirePokemon(completedOutcome==4 && escapeMenuSeen[0] && escapeMenuSeen[1] &&
                           escapeActionSeen[1] && (scenario=="escape-player2" || escapeActionSeen[0]),
                           "The selected human did not complete a native wild escape");
            requirePokemon(readWallet(a)==initialMoney0 && readWallet(b)==9876 &&
                           readInventory(a)==originalBag0 && readInventory(b)==originalBag1 &&
                           readStorage(a)==originalBoxes0 && readStorage(b)==originalBoxes1,
                           "Wild escape changed an owner's wallet, inventory or PC");
        }
        requirePokemon(hashFile(rom)==RomHash && hashFile(state)==stateHash,"Input ROM/checkpoint changed");
        if(scenario.contains("exp-share-dex")) {
            requirePokemon(evolutionStarts[0]==1 && evolutionStarts[1]==1 && r0.mons[5].species()==42 && r1.mons[5].species()==169,
                           "National Dex evolution restriction did not use the actual owner");
            requirePokemon(readDexEntry(a,169).isCaught()==originalCrobat0.isCaught() && readDexEntry(b,169).isCaught(),
                           "Crobat evolution dex receipt belongs to the wrong trainer");
        }
        QJsonObject report{{"rom_sha256",RomHash},{"checkpoint_sha256",QString::fromLatin1(stateHash)},{"scenario",scenario},{"two_real_mgba_cores",true},{"authored_test_parties",true},{"native_battle_completed",true},{"owned_inputs_both_players",true},{"party_ownership_preserved",true},{"pp_transferred_to_owners",!scenario.startsWith("capture") && scenario!="defeat"},{"experience_transferred_to_owners",!scenario.startsWith("capture") && scenario!="exhaust" && scenario!="defeat"},{"capture_and_bag_and_dex_ownership",scenario.startsWith("capture")},{"exhausted_owner_partner_continues",scenario=="exhaust"},{"blackout_uses_own_level_and_wallet",scenario=="defeat"},{"rollback_preserves_parties_and_wallets",true},{"wallet0",int(readWallet(a))},{"wallet1",int(readWallet(b))},{"frames",int(frames)},{"battery_files_written",false},{"production_launcher",false},{"complete_world_coop",false}};
        report["native_save_and_fresh_battery_reload_both_players"]=saveReload;
        if(scenario.contains("pinned")) {
            report["authored_all_six_active_queued_wish_pins"]=true;
            report["native_exp_restored_all_working_records_and_references"]=reserveProbe.restored;
            report["native_move_learning_summary_reshows"]=int(reserveProbe.awardReshows);
        }
        if(scenario.startsWith("reserves-roar")) {
            report["owned_native_roar_counts"]=QJsonArray{int(reserveProbe.forced[0]),int(reserveProbe.forced[1])};
            report["native_roar_selected_unloaded_own_original"]=reserveProbe.unloadedForced;
        }
        if(scenario.contains("wish"))report["native_wish_names_survive_source_eviction_both_owners"]=true;
        if(scenario.startsWith("pokerus"))report["all_twelve_owned_pokerus_status_and_native_active_cured_ev_bonus_verified"]=true;
        if(scenario.startsWith("shedinja"))report["native_shedinja_additions_dex_and_owner_capacity_verified"]=true;
        if(scenario=="party-menu-shedinja")report["native_switch_and_battle_with_same_identity_ninjask_shedinja_siblings"]=true;
        if(scenario=="party-menu-protected")report["both_protected_selections_rejected_in_place_and_battle_completed"]=true;
        if(scenario.startsWith("exp-share")) {
            report["both_sixth_party_holders_received_native_exp_and_evs"]=!scenario.contains("many");
            report["four_same_owner_holders_received_native_exp_and_evs"]=scenario.contains("many");
            report["exp_share_holders_kept_original_owner_and_item"]=true;
            report["exp_share_evolutions_owned_by_actual_players"]=scenario.contains("evolution");
            report["guest_move_learning_ignores_other_players_b"]=scenario.contains("learn");
        }
        if(scenario.startsWith("escape")) {
            report["native_wild_escape_outcome"]=int(completedOutcome);
            report["player_one_selected_escape"]=escapeActionSeen[0];
            report["player_two_selected_escape"]=escapeActionSeen[1];
            report["wallets_bags_and_storage_preserved"]=true;
            report["pp_transferred_to_owners"]=false;
            report["experience_transferred_to_owners"]=false;
        }
        if(scenario.startsWith("single")) {
            requirePokemon(bool(singleOpponentProbe) && singleAbsentObserved,"Single-opponent test did not reach the native action menu");
            const auto observed=*singleOpponentProbe;
            requirePokemon((observed[2]&0xAu)==8u,"Single wild opponent was not the only active enemy");
            report["opponent_party_slots"]=QJsonArray{int(observed[0]),int(observed[1])};
            report["opponent_absent_flags"]=int(observed[2]);
            report["two_active_opponents_alias_same_party_slot"]=
                !(observed[2] & (1u<<1)) && !(observed[2] & (1u<<3)) && observed[0]==observed[1];
            if(scenario.contains("multiturn")) {
                requirePokemon(opponentPpObserved && initialEnemyPp>=minimumEnemyPp+2 && minimumPhantomPp==initialPhantomPp,
                               "Single wild opponent did not act normally or its phantom acted");
                report["real_opponent_attacked_across_turns"]=true;
                report["phantom_opponent_never_spent_move_pp"]=true;
            }
        }
        if(scenario.startsWith("trainer-single")) {
            requirePokemon(bool(singleOpponentProbe) && singleAbsentObserved,"Single-trainer test did not reach the native action menu");
            const auto observed=*singleOpponentProbe;
            requirePokemon((observed[2]&0xAu)==8u,"Single trainer opponent was not the only active enemy");
            report["opponent_party_slots"]=QJsonArray{int(observed[0]),int(observed[1])};
            report["opponent_absent_flags"]=int(observed[2]);
            report["two_active_opponents_alias_same_party_slot"]=
                !(observed[2] & (1u<<1)) && !(observed[2] & (1u<<3)) && observed[0]==observed[1];
            if(scenario.contains("multiturn")) {
                requirePokemon(opponentPpObserved && initialEnemyPp>=minimumEnemyPp+2 && minimumPhantomPp==initialPhantomPp,
                               "Single trainer opponent did not act normally or its phantom acted");
                report["real_opponent_attacked_across_turns"]=true;
                report["phantom_opponent_never_spent_move_pp"]=true;
            }
        }
        if(scenario.startsWith("party-menu")) {
            report["native_full_six_own_party_menu_both_owners"]=true;
            report["native_voluntary_import_of_living_sixth_original_both_owners"]=scenario!="party-menu-forced" && scenario!="party-menu-protected";
            report["native_forced_replacement_selects_sixth_original_both_owners"]=scenario=="party-menu-forced";
            report["native_cancel_then_reopen_both_owners"]=scenario=="party-menu-cancel"||scenario=="party-menu-summary";
            report["native_owned_summary_then_cancel_reopen_both_owners"]=scenario=="party-menu-summary";
            report["temporary_menu_rollback_preserves_original_parties"]=menuRollback;
            report["living_retired_bench_data_preserved"]=true;
            report["selected_sixth_original_attacks_and_gains_exp_both_owners"]=scenario!="party-menu-protected";
            report["reserve_switch_preserves_resident_exp_share_awards"]=scenario=="party-menu-exp-share";
        }
        if(scenario=="action-stats") {
            report["splash_count_owned_by_player_one_only"]=true;
            report["struggle_count_owned_by_player_two_only"]=true;
        }
        if(scenario=="evolution") {
            report["own_evolution_cancel_works"]=true;
            report["other_owner_cancel_ignored"]=true;
            report["evolved_species_dex_owned_by_actual_holder"]=true;
            report["evolution_statistics_owned_by_actual_holder"]=true;
            report["evolution_started_player_one"]=int(evolutionStarts[0]);
            report["evolution_started_player_two"]=int(evolutionStarts[1]);
        }
        if(scenario=="bench") {
            report["first_five_own_pokemon_fought_and_fainted_both_owners"]=true;
            report["sixth_own_pokemon_fought_both_owners"]=true;
            report["bench_refills_player_one"]=int(benchRefills[0]);
            report["bench_refills_player_two"]=int(benchRefills[1]);
            report["complete_twelve_pokemon_exp_share_and_free_choice"]=false;
            for(unsigned owner=0;owner<2;++owner) {
                QJsonArray entered;for(bool value:benchEntered[owner])entered.append(value);
                report[owner?"original_pokemon_entered_player_two":"original_pokemon_entered_player_one"]=entered;
            }
        }
        if(scenario.startsWith("capture")) {
            for(unsigned owner=0;owner<2;++owner)if(nativeThrowsByOwner[owner])
                requirePokemon(nativeBallTextByOwner[owner],"Native ball message did not name the acting trainer");
            report["native_ball_text_correct_by_owner"]=QJsonArray{nativeBallTextByOwner[0],nativeBallTextByOwner[1]};
            report["native_ball_actions_executed"]=int(nativeThrows);
            report["native_ball_actions_player_one"]=int(nativeThrowsByOwner[0]);
            report["native_ball_actions_player_two"]=int(nativeThrowsByOwner[1]);
            report["native_full_recipient_rejections"]=int(capacityDenials);
            report["pending_throw_rollback_preserves_original_pc_party_and_bag"]=pendingThrowRollback;
            report["leader_original_pc_preserved"]=readStorage(a)==originalBoxes0;
            report["recipient_original_pc_preserved"]=readStorage(b)==originalBoxes1;
            report["personal_capture_count_and_pc_metadata_owned"]=true;
            report["actual_party_capture_pp_preserved"]=scenario!="capture-denied" && scenario!="capture-pc";
            report["previous_dex_form_preserved_for_noncatcher"]=scenario=="capture-unown" || scenario=="capture-spinda";
            report["first_special_form_seen_shared_and_owned_only_by_catcher"]=scenario=="capture-unown-first";
            if(scenario=="capture-sequential") {
                report["first_native_throw_failed_second_owner_caught"]=true;
                report["both_original_pcs_full"]=!storageHasCaptureSpace(originalBoxes0) && !storageHasCaptureSpace(originalBoxes1);
            }
        }
        report["player_one_save_generation"]=int(savedGenerations[0]);
        report["player_two_save_generation"]=int(savedGenerations[1]);
        report["native_money_announcement_matches_share"]=moneyTextEncoded;
        QFile result(out+"/result.json");requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly),"Cannot create acceptance report");
        const auto json=QJsonDocument(report).toJson();requirePokemon(result.write(json)==json.size(),"Cannot write acceptance report");
        printf("%s",json.constData());return 0;
    }catch(const std::exception& error){fprintf(stderr,"FireRed pair acceptance failed: %s\n",error.what());return 1;}
}
