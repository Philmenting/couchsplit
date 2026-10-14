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
namespace coop {
inline void fail(QString msg) { throw std::runtime_error(msg.toStdString()); }
inline void atomicWrite(const QString& name, const QByteArray& bytes) {
    QSaveFile f(name);
    if(!f.open(QIODevice::WriteOnly) || f.write(bytes)!=bytes.size() || !f.commit()) fail("Datei konnte nicht gespeichert werden: "+name);
}
inline QString hashFile(const QString& name) {
    QFile f(name); if(!f.open(QIODevice::ReadOnly)) fail("Datei nicht lesbar: "+name);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if(!hash.addData(&f)) fail("Datei konnte nicht gehasht werden: "+name);
    return QString::fromLatin1(hash.result().toHex());
}
class SaveStore {
    QString base;
    std::unique_ptr<QLockFile> lock;
public:
    explicit SaveStore(QString path):base(std::move(path)) { if(!QDir().mkpath(base)) fail("Profilordner nicht anlegbar"); }
    QString root() const { return base; }
    void acquire() {
        if(lock) fail("Sitzung läuft bereits");
        lock=std::make_unique<QLockFile>(base+"/session.lock"); lock->setStaleLockTime(0);
        if(!lock->tryLock(0)) { lock.reset(); fail("Spielstände werden bereits von einer anderen Sitzung verwendet."); }
    }
    void release() { lock.reset(); }
    bool locked() const { return bool(lock); }
    QString profile(int p, const QString& game) const { return base+"/players/"+QString::number(p+1)+"/"+game; }
    QString backup(int p,const QString& game) {
        if(!locked()) fail("Backup benötigt Profilsperre");
        const auto source=profile(p,game)+"/saves";
        const auto target=profile(p,game)+"/backups/"+QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmsszzz")+"-"+QUuid::createUuid().toString(QUuid::Id128);
        if(!QDir().mkpath(source) || !QDir().mkpath(target)) fail("Backupordner nicht anlegbar");
        QJsonObject manifest;
        for(const auto& name:QDir(source).entryList({"*.sav"},QDir::Files|QDir::NoSymLinks)) {
            QFile f(source+"/"+name); if(!f.open(QIODevice::ReadOnly)) fail("Spielstand nicht lesbar");
            auto bytes=f.readAll(); atomicWrite(target+"/"+name,bytes);
            manifest[name]=hashFile(target+"/"+name);
        }
        atomicWrite(target+"/manifest.json",QJsonDocument(manifest).toJson());
        return target;
    }
    void restore(int p,const QString& game,const QString& backupPath) {
        if(!locked()) fail("Wiederherstellung benötigt Profilsperre");
        const auto allowed=QFileInfo(profile(p,game)+"/backups").canonicalFilePath();
        auto chosen=QFileInfo(backupPath).canonicalFilePath();
        if(allowed.isEmpty() || chosen.isEmpty() || QFileInfo(chosen).dir().canonicalPath()!=allowed) fail("Ungültiger Backupordner");
        QFile manifest(chosen+"/manifest.json");
        if(!manifest.open(QIODevice::ReadOnly)) fail("Backupmanifest fehlt");
        QJsonParseError error; auto doc=QJsonDocument::fromJson(manifest.readAll(),&error);
        if(error.error!=QJsonParseError::NoError || !doc.isObject() || doc.object().isEmpty()) fail("Backup enthält keine verifizierbaren Spielstände");
        const auto entries=doc.object();
        // Validate everything before writing any file.
        for(auto it=entries.begin();it!=entries.end();++it) {
            auto name=it.key();
            if(name!=QFileInfo(name).fileName() || !name.endsWith(".sav") || QFileInfo(chosen+"/"+name).isSymLink() || hashFile(chosen+"/"+name)!=it.value().toString()) fail("Beschädigtes Backup");
        }
        backup(p,game);
        for(auto it=entries.begin();it!=entries.end();++it) {
            QFile f(chosen+"/"+it.key()); if(!f.open(QIODevice::ReadOnly)) fail("Backup nicht lesbar");
            atomicWrite(profile(p,game)+"/saves/"+it.key(),f.readAll());
        }
    }
};
}
