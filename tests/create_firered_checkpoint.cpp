// SPDX-License-Identifier: GPL-3.0-or-later
// Private test fixture only: boot an existing save and snapshot a quiescent
// original game. Neither the input save nor ROM is written or distributed.
#include "../gba/FireRedCampaignSync.h"
#include "../gba/FireRedWorld.h"
#include "FireRedSaveAcceptance.h"
#include <mgba/core/log.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <iostream>

using namespace coop::firered;
namespace {
constexpr auto RomHash = "eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507";
QByteArray fileHash(const QString& path) {
    QFile file(path); requirePokemon(file.open(QIODevice::ReadOnly), "Cannot read input");
    QCryptographicHash h(QCryptographicHash::Sha256);
    requirePokemon(h.addData(&file), "Cannot hash input"); return h.result().toHex();
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        fprintf(stderr, "create-firered-checkpoint ORIGINAL_DE.gba PRIVATE_SAVE.sav NEW_CHECKPOINT\n");
        return 2;
    }
    try {
        const QString rom=QString::fromLocal8Bit(argv[1]), save=QString::fromLocal8Bit(argv[2]), out=QString::fromLocal8Bit(argv[3]);
        requirePokemon(!QFileInfo::exists(out) && !QFileInfo(out).isSymLink(), "Checkpoint path must be new");
        requirePokemon(fileHash(rom)==RomHash, "Exact German FireRed ROM required");
        const auto saveHash=fileHash(save);
        QFile file(save);requirePokemon(file.open(QIODevice::ReadOnly) && file.size()==131072,"Save must be 128 KiB");
        const auto raw=file.readAll();requirePokemon(raw.size()==131072,"Cannot read complete save");
        const std::vector<uint8_t> battery(raw.begin(),raw.end());
        requirePokemon(bool(acceptance::save_detail::generation(battery)),"Save has no complete checksum-valid generation");
        mLogFilter filter{};mLogFilterInit(&filter);filter.defaultLevels=mLOG_FATAL|mLOG_ERROR;
        mLogger logger{};logger.filter=&filter;logger.log=[](mLogger*,int,mLogLevel,const char*,va_list){};mLogSetDefaultLogger(&logger);
        acceptance::save_detail::FreshCore fresh;fresh.open(rom.toStdString(),battery);auto* c=fresh.c;
        bool ready=false;
        for(unsigned n=0;n<12000;++n) {
            const auto callback=inventoryRead32(c,0x03003044);
            const unsigned keys=n<300?(n%120<3?8u:0u):(n%60<3?(callback==0x080565D5?2u:1u):0u);
            c->setKeys(c,keys);c->runFrame(c);
            if(n>300 && progressFieldReady(c) && readWorldActor(c).valid) {ready=true;break;}
        }
        requirePokemon(ready,"Save did not reach a usable native overworld");
        c->setKeys(c,0);for(unsigned n=0;n<120;++n)c->runFrame(c);
        const auto party=readParty(c);bool viable=false;
        for(unsigned slot=0;slot<party.count;++slot) viable |= party.mons[slot].viable();
        requirePokemon(progressFieldReady(c) && readWorldActor(c).valid && viable,
                       "Save did not remain in an idle overworld with a usable party");
        std::vector<uint8_t> state(c->stateSize(c));
        requirePokemon(c->saveState(c,state.data()),"Cannot capture checkpoint");
        requirePokemon(fileHash(rom)==RomHash && fileHash(save)==saveHash,"An input changed");
        QFile result(out);requirePokemon(result.open(QIODevice::WriteOnly|QIODevice::NewOnly,
            QFileDevice::ReadOwner|QFileDevice::WriteOwner),"Cannot create checkpoint");
        if(result.write(reinterpret_cast<const char*>(state.data()),qint64(state.size()))!=qint64(state.size()) || !result.flush()) {
            result.close();result.remove();throw std::runtime_error("Cannot write complete checkpoint");
        }
        std::cout << "Private checkpoint created; ROM and source save unchanged.\n";
        return 0;
    } catch(const std::exception& error) {
        fprintf(stderr,"Checkpoint failed: %s\n",error.what());return 1;
    }
}
