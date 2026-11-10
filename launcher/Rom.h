// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "SaveStore.h"
#include "../gba/FireRedVersion.h"

namespace coop {
enum class GameSystem { NintendoDS, GameBoyAdvance };
struct RomInfo { GameSystem system; QString sha256; bool extendedGba=false; };
inline constexpr qint64 GbaNativeLimit = 32LL*1024*1024;
inline constexpr qint64 GbaExtendedLimit = 512LL*1024*1024;
inline constexpr const char* GermanFireRedSha256 = firered::GermanSha256.data();
inline bool isSupportedFireRed(const QString& sha256) {
    return firered::fireRedVersion(sha256.toStdString()).has_value();
}
inline bool isEnglishFireRed(const QString& sha256) {
    const auto version=firered::fireRedVersion(sha256.toStdString());
    return version && *version!=firered::RomVersion::German;
}
inline RomInfo inspectRom(const QString& path) {
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly)) fail(i18n::text("ROM fehlt oder ist nicht lesbar."));
    if(f.size()<512 || f.size()>512*1024*1024) fail(i18n::text("Keine gültige Spiel-ROM (Dateigröße)."));
    const auto header=f.read(512);
    if(header.size()!=512 || f.error()!=QFileDevice::NoError)fail(i18n::text("ROM fehlt oder ist nicht lesbar."));
    if(header.mid(12,3)=="CPU") return {GameSystem::NintendoDS,hashFile(path)};
    if(static_cast<unsigned char>(header[0xB2])==0x96) {
        unsigned sum=0x19;
        for(int i=0xA0;i<=0xBD;i++) sum+=static_cast<unsigned char>(header[i]);
        const bool extended=header.mid(0xB5,7)=="CSXROM1";
        if(sum&255) fail(i18n::text("GBA-ROM hat eine ungültige Header-Prüfsumme."));
        if(extended ? (f.size()<=GbaNativeLimit || f.size()>GbaExtendedLimit || f.size()%4)
                    : f.size()>GbaNativeLimit)
            fail(i18n::text("GBA-ROM benötigt ein gültiges CouchSplit-XROM-Format: normale ROMs bis 32 MiB, XROM bis 512 MiB."));
        // A valid header identifies the format, not full game/hack compatibility.
        // Keep every build/language in its own hash-based save profile.
        return {GameSystem::GameBoyAdvance,hashFile(path),extended};
    }
    fail(i18n::text("Unterstützt werden Project-PM-Platin und GBA-ROMs mit gültigem Header."));
    return {};
}
inline QString validateRom(const QString& path) {return inspectRom(path).sha256;}
}
