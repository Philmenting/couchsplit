// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>
#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include "../launcher/Language.h"
namespace coop {
inline void fail(QString msg) { throw std::runtime_error(msg.toStdString()); }
inline void atomicWrite(const QString& name, const QByteArray& bytes) {
    QSaveFile f(name);
    if(!f.open(QIODevice::WriteOnly) || f.write(bytes)!=bytes.size() || !f.commit()) fail(i18n::text("Datei konnte nicht gespeichert werden: ")+name);
}
inline QString hashFile(const QString& name) {
    QFile f(name); if(!f.open(QIODevice::ReadOnly)) fail(i18n::text("Datei nicht lesbar: ")+name);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if(!hash.addData(&f)) fail(i18n::text("Datei konnte nicht gehasht werden: ")+name);
    return QString::fromLatin1(hash.result().toHex());
}
class SaveStore {
    QString base;
    std::unique_ptr<QLockFile> lock;
    using PairData=std::array<QByteArray,2>;
    using RecoveryData=std::array<std::optional<QByteArray>,2>;
    static constexpr qsizetype FireRedSaveSize=131072;
    static constexpr qsizetype MaxRecoverySaveSize=16*1024*1024;
    static QString bytesHash(const QByteArray& bytes) {
        return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
    }
    static QByteArray pairFile(const QString& path) {
        const QFileInfo info(path);
        if(info.isSymLink() || !info.isFile() || info.size()!=FireRedSaveSize)
            fail(i18n::text("Ungültiger Feuerrot-Spielstand im Paar-Backup."));
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) fail(i18n::text("Feuerrot-Paar-Backup nicht lesbar."));
        const auto bytes=file.readAll();
        if(bytes.size()!=FireRedSaveSize || file.error()!=QFileDevice::NoError)
            fail(i18n::text("Feuerrot-Paar-Backup nicht lesbar."));
        return bytes;
    }
    static std::optional<QByteArray> recoveryFile(const QString& path) {
        const QFileInfo info(path);
        if(info.isSymLink() || (info.exists() && (!info.isFile() || info.size()>MaxRecoverySaveSize)))
            fail(i18n::text("Aktueller Spielstand für Paar-Wiederherstellung nicht lesbar."));
        if(!info.exists())return std::nullopt;
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly))fail(i18n::text("Aktueller Spielstand für Paar-Wiederherstellung nicht lesbar."));
        const auto bytes=file.readAll();
        if(bytes.size()!=info.size() || file.error()!=QFileDevice::NoError)
            fail(i18n::text("Aktueller Spielstand für Paar-Wiederherstellung nicht lesbar."));
        return bytes;
    }
    PairData readPair(const QString& game,const QString& path) const {
        const auto allowed=QFileInfo(pairBackupsDir(game)).canonicalFilePath();
        const QFileInfo selected(path);
        const auto chosen=selected.canonicalFilePath();
        if(allowed.isEmpty() || selected.isSymLink() || chosen.isEmpty() ||
           QFileInfo(chosen).dir().canonicalPath()!=allowed)
            fail(i18n::text("Ungültiger Paar-Backupordner."));
        const QFileInfo metadata(chosen+"/manifest.json");
        QFile manifest(metadata.filePath());
        if(metadata.isSymLink() || !metadata.isFile() || metadata.size()>4096 || !manifest.open(QIODevice::ReadOnly))
            fail(i18n::text("Feuerrot-Paar-Backup ist beschädigt."));
        QJsonParseError error;const auto doc=QJsonDocument::fromJson(manifest.readAll(),&error);
        const auto json=doc.object();
        if(error.error!=QJsonParseError::NoError || !doc.isObject() ||
           json.value("format").toString()!="couchsplit-firered-pair-1" || json.value("rom").toString()!=game)
            fail(i18n::text("Feuerrot-Paar-Backup ist beschädigt."));
        PairData data{{pairFile(chosen+"/player1.sav"),pairFile(chosen+"/player2.sav")}};
        if(bytesHash(data[0])!=json.value("player1").toString() || bytesHash(data[1])!=json.value("player2").toString())
            fail(i18n::text("Feuerrot-Paar-Backup ist beschädigt."));
        return data;
    }
    void writePair(const QString& game,const PairData& data) {
        for(int p=0;p<2;++p) {
            const auto save=profile(p,game)+"/saves/game.sav";
            if(QFileInfo(save).isSymLink() || (QFileInfo(save).exists() && !QFileInfo(save).isFile()))
                fail(i18n::text("Ungültiger Feuerrot-Spielstand im Paar-Backup."));
            atomicWrite(save,data[p]);
        }
    }
    RecoveryData readRecovery(const QString& game,const QString& path) const {
        const auto allowed=QFileInfo(pairBackupsDir(game)).canonicalFilePath();
        const QFileInfo selected(path);const auto chosen=selected.canonicalFilePath();
        if(allowed.isEmpty() || selected.isSymLink() || chosen.isEmpty() || QFileInfo(chosen).dir().canonicalPath()!=allowed)
            fail(i18n::text("Ungültiger Paar-Backupordner."));
        const QFileInfo info(chosen+"/manifest.json");QFile file(info.filePath());
        if(info.isSymLink() || !info.isFile() || info.size()>4096 || !file.open(QIODevice::ReadOnly))
            fail(i18n::text("Feuerrot-Paar-Backup ist beschädigt."));
        QJsonParseError error;const auto doc=QJsonDocument::fromJson(file.readAll(),&error);const auto json=doc.object();
        if(error.error!=QJsonParseError::NoError || !doc.isObject() ||
           json.value("format").toString()!="couchsplit-firered-recovery-1" || json.value("rom").toString()!=game)
            fail(i18n::text("Feuerrot-Paar-Backup ist beschädigt."));
        RecoveryData data;
        for(int p=0;p<2;++p) {
            const auto name="player"+QString::number(p+1);
            const auto present=json.value(name+"_present");
            if(!present.isBool())fail(i18n::text("Feuerrot-Paar-Backup ist beschädigt."));
            const auto bytes=recoveryFile(chosen+"/"+name+".sav");
            if(present.toBool()!=bool(bytes) || (bytes && bytesHash(*bytes)!=json.value(name).toString()))
                fail(i18n::text("Feuerrot-Paar-Backup ist beschädigt."));
            data[p]=bytes;
        }
        return data;
    }
    void writeRecovery(const QString& game,const RecoveryData& data) {
        for(int p=0;p<2;++p) {
            const auto path=profile(p,game)+"/saves/game.sav";const QFileInfo info(path);
            if(info.isSymLink() || (info.exists() && !info.isFile()))
                fail(i18n::text("Aktueller Spielstand für Paar-Wiederherstellung nicht lesbar."));
            if(data[p])atomicWrite(path,*data[p]);
            else if(info.exists() && !QFile::remove(path))
                fail(i18n::text("Paar-Wiederherstellung konnte nicht abgeschlossen werden."));
        }
    }
public:
    explicit SaveStore(QString path):base(std::move(path)) { if(!QDir().mkpath(base)) fail(i18n::text("Profilordner nicht anlegbar")); }
    QString root() const { return base; }
    void acquire() {
        if(lock) fail(i18n::text("Sitzung läuft bereits"));
        lock=std::make_unique<QLockFile>(base+"/session.lock"); lock->setStaleLockTime(0);
        if(!lock->tryLock(0)) { lock.reset(); fail(i18n::text("Spielstände werden bereits von einer anderen Sitzung verwendet.")); }
    }
    void release() { lock.reset(); }
    bool locked() const { return bool(lock); }
    QString profile(int p, const QString& game) const { return base+"/players/"+QString::number(p+1)+"/"+game; }
    // Keep the filename independent of the imported ROM's original name.
    // Legacy files remain intact; different ROM hashes never share a profile.
    void prepareSave(int p, const QString& game, const QString& romName) {
        if(!locked()) fail(i18n::text("Spielstandübernahme benötigt Profilsperre"));
        const auto directory=profile(p,game)+"/saves";
        const auto target=directory+"/game.sav";
        QFileInfo stable(target);
        if(stable.isSymLink() || (stable.exists() && !stable.isFile())) fail(i18n::text("Ungültige Spielstanddatei: ")+target);
        if(stable.exists()) return;
        const auto preferred=QFileInfo(romName).completeBaseName()+".sav";
        auto candidates=QDir(directory).entryList({"*.sav"},QDir::Files|QDir::NoSymLinks);
        if(QFileInfo(directory+"/"+preferred).isSymLink()) fail(i18n::text("Ungültige ältere Spielstanddatei."));
        if(candidates.isEmpty()) return;
        QString selected;
        if(candidates.contains(preferred)) selected=preferred;
        else if(candidates.size()==1) selected=candidates.first();
        else fail(i18n::text("Mehrere ältere Spielstände gefunden. Zuerst die ursprünglich verwendete ROM auswählen; die Dateien bleiben erhalten: ")+directory);
        QFile source(directory+"/"+selected);
        if(!source.open(QIODevice::ReadOnly)) fail(i18n::text("Älterer Spielstand nicht lesbar: ")+source.fileName());
        const auto bytes=source.readAll();
        if(source.error()!=QFileDevice::NoError) fail(i18n::text("Älterer Spielstand konnte nicht vollständig gelesen werden."));
        atomicWrite(target,bytes);
    }
    QString backup(int p,const QString& game) {
        if(!locked()) fail(i18n::text("Backup benötigt Profilsperre"));
        const auto source=profile(p,game)+"/saves";
        const auto target=profile(p,game)+"/backups/"+QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmsszzz")+"-"+QUuid::createUuid().toString(QUuid::Id128);
        if(!QDir().mkpath(source) || !QDir().mkpath(target)) fail(i18n::text("Backupordner nicht anlegbar"));
        QJsonObject manifest;
        for(const auto& name:QDir(source).entryList({"*.sav"},QDir::Files|QDir::NoSymLinks)) {
            QFile f(source+"/"+name); if(!f.open(QIODevice::ReadOnly)) fail(i18n::text("Spielstand nicht lesbar"));
            auto bytes=f.readAll(); atomicWrite(target+"/"+name,bytes);
            manifest[name]=hashFile(target+"/"+name);
        }
        atomicWrite(target+"/manifest.json",QJsonDocument(manifest).toJson());
        return target;
    }
    QString pairBackupsDir(const QString& game) const {
        if(game.size()!=64 || game.contains(QRegularExpression("[^0-9a-f]")))
            fail(i18n::text("Ungültiger Paar-Backupordner."));
        return base+"/paired-backups/"+game;
    }
    // Restore an interrupted two-file operation before any worker or individual
    // restoration may read it. The journal always points to pre-restore data.
    void recoverPair(const QString& game) {
        if(!locked()) fail(i18n::text("Wiederherstellung benötigt Profilsperre"));
        const auto journal=pairBackupsDir(game)+"/restore-pending.json";
        const QFileInfo info(journal);
        if(!info.exists() && !info.isSymLink()) return;
        QFile file(journal);
        if(info.isSymLink() || !info.isFile() || info.size()>4096 || !file.open(QIODevice::ReadOnly))
            fail(i18n::text("Unterbrochene Paar-Wiederherstellung prüfen; Spielstände bleiben gesperrt."));
        QJsonParseError error;const auto doc=QJsonDocument::fromJson(file.readAll(),&error);
        const auto data=doc.object();const auto name=data.value("backup").toString();
        if(error.error!=QJsonParseError::NoError || !doc.isObject() || data.value("format").toString()!="couchsplit-firered-restore-1" ||
           data.value("rom").toString()!=game || name.isEmpty() || name!=QFileInfo(name).fileName() || name=="." || name=="..")
            fail(i18n::text("Unterbrochene Paar-Wiederherstellung prüfen; Spielstände bleiben gesperrt."));
        const auto before=readRecovery(game,pairBackupsDir(game)+"/"+name);
        writeRecovery(game,before);
        if(!QFile::remove(journal)) fail(i18n::text("Unterbrochene Paar-Wiederherstellung prüfen; Spielstände bleiben gesperrt."));
    }
    // A pair records the two existing battery saves at one launcher boundary;
    // this is not a savestate or proof the players saved on the same frame.
    QString backupPair(const QString& game) {
        if(!locked()) fail(i18n::text("Backup benötigt Profilsperre"));
        recoverPair(game);
        const auto first=profile(0,game)+"/saves/game.sav",second=profile(1,game)+"/saves/game.sav";
        if((!QFileInfo::exists(first) && !QFileInfo(first).isSymLink()) ||
           (!QFileInfo::exists(second) && !QFileInfo(second).isSymLink())) return {};
        const PairData data{{pairFile(first),pairFile(second)}};
        const auto root=pairBackupsDir(game);
        if(!QDir().mkpath(root)) fail(i18n::text("Backupordner nicht anlegbar"));
        const auto target=root+"/"+QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmsszzz")+"-"+QUuid::createUuid().toString(QUuid::Id128);
        if(!QDir().mkpath(target)) fail(i18n::text("Backupordner nicht anlegbar"));
        try {
            atomicWrite(target+"/player1.sav",data[0]);atomicWrite(target+"/player2.sav",data[1]);
            const QJsonObject manifest{{"format","couchsplit-firered-pair-1"},{"rom",game},
                                       {"player1",bytesHash(data[0])},{"player2",bytesHash(data[1])}};
            atomicWrite(target+"/manifest.json",QJsonDocument(manifest).toJson());
        } catch(...) {QDir(target).removeRecursively();throw;}
        return target;
    }
    // Internal safety point also preserves absent or damaged current files,
    // so a valid old pair can recover a missing/corrupt current save.
    QString backupPairBeforeRestore(const QString& game) {
        if(!locked()) fail(i18n::text("Backup benötigt Profilsperre"));
        const RecoveryData data{{recoveryFile(profile(0,game)+"/saves/game.sav"),
                                 recoveryFile(profile(1,game)+"/saves/game.sav")}};
        const auto root=pairBackupsDir(game);
        if(!QDir().mkpath(root))fail(i18n::text("Backupordner nicht anlegbar"));
        const auto target=root+"/.restore-safety-"+QUuid::createUuid().toString(QUuid::Id128);
        if(!QDir().mkpath(target))fail(i18n::text("Backupordner nicht anlegbar"));
        try {
            QJsonObject manifest{{"format","couchsplit-firered-recovery-1"},{"rom",game}};
            for(int p=0;p<2;++p) {
                const auto name="player"+QString::number(p+1);
                manifest[name+"_present"]=bool(data[p]);
                if(data[p]) {atomicWrite(target+"/"+name+".sav",*data[p]);manifest[name]=bytesHash(*data[p]);}
            }
            atomicWrite(target+"/manifest.json",QJsonDocument(manifest).toJson());
        }catch(...){QDir(target).removeRecursively();throw;}
        return target;
    }
    void restorePair(const QString& game,const QString& chosen) {
        if(!locked()) fail(i18n::text("Wiederherstellung benötigt Profilsperre"));
        recoverPair(game);
        const auto desired=readPair(game,chosen); // validate both before either write
        // Retain every existing save in the ordinary individual backup history
        // as well as a byte-exact safety point (including absent files).
        backup(0,game);backup(1,game);
        const auto before=backupPairBeforeRestore(game);
        const auto journal=pairBackupsDir(game)+"/restore-pending.json";
        if(QFileInfo(journal).isSymLink()) fail(i18n::text("Ungültiger Paar-Backupordner."));
        const QJsonObject record{{"format","couchsplit-firered-restore-1"},{"rom",game},{"backup",QFileInfo(before).fileName()}};
        atomicWrite(journal,QJsonDocument(record).toJson());
        try {
            writePair(game,desired);
            if(!QFile::remove(journal)) fail(i18n::text("Paar-Wiederherstellung konnte nicht abgeschlossen werden."));
        } catch(...) {
            try {recoverPair(game);} catch(...) {
                fail(i18n::text("Unterbrochene Paar-Wiederherstellung prüfen; Spielstände bleiben gesperrt."));
            }
            throw;
        }
    }
    void restore(int p,const QString& game,const QString& backupPath,const QString& romName={}) {
        if(!locked()) fail(i18n::text("Wiederherstellung benötigt Profilsperre"));
        recoverPair(game);
        const auto allowed=QFileInfo(profile(p,game)+"/backups").canonicalFilePath();
        auto chosen=QFileInfo(backupPath).canonicalFilePath();
        if(allowed.isEmpty() || chosen.isEmpty() || QFileInfo(chosen).dir().canonicalPath()!=allowed) fail(i18n::text("Ungültiger Backupordner"));
        QFile manifest(chosen+"/manifest.json");
        if(!manifest.open(QIODevice::ReadOnly)) fail(i18n::text("Backupmanifest fehlt"));
        QJsonParseError error; auto doc=QJsonDocument::fromJson(manifest.readAll(),&error);
        if(error.error!=QJsonParseError::NoError || !doc.isObject() || doc.object().isEmpty()) fail(i18n::text("Backup enthält keine verifizierbaren Spielstände"));
        const auto entries=doc.object();
        // Validate everything before writing any file.
        for(auto it=entries.begin();it!=entries.end();++it) {
            auto name=it.key();
            if(name!=QFileInfo(name).fileName() || !name.endsWith(".sav") || QFileInfo(chosen+"/"+name).isSymLink() || hashFile(chosen+"/"+name)!=it.value().toString()) fail(i18n::text("Beschädigtes Backup"));
        }
        // Older backups contain ROM-dependent names. Restoring one must also
        // update the stable save used by the current launcher.
        QString selected="game.sav";
        if(!entries.contains(selected)) {
            const auto preferred=QFileInfo(romName).completeBaseName()+".sav";
            if(entries.contains(preferred)) selected=preferred;
            else if(entries.size()==1) selected=entries.begin().key();
            else fail(i18n::text("Backup enthält mehrere ältere Spielstände. Zuerst die ursprünglich verwendete ROM auswählen."));
        }
        backup(p,game);
        for(auto it=entries.begin();it!=entries.end();++it) {
            QFile f(chosen+"/"+it.key()); if(!f.open(QIODevice::ReadOnly)) fail(i18n::text("Backup nicht lesbar"));
            atomicWrite(profile(p,game)+"/saves/"+it.key(),f.readAll());
        }
        if(selected!="game.sav") {
            QFile source(chosen+"/"+selected);
            if(!source.open(QIODevice::ReadOnly)) fail(i18n::text("Backup nicht lesbar"));
            atomicWrite(profile(p,game)+"/saves/game.sav",source.readAll());
        }
    }
};
}
