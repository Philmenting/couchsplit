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
#include <QSaveFile>
#include <QUuid>
#include <memory>
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
    void restore(int p,const QString& game,const QString& backupPath,const QString& romName={}) {
        if(!locked()) fail(i18n::text("Wiederherstellung benötigt Profilsperre"));
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
