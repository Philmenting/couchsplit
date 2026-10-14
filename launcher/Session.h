// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QCoreApplication>
#include <QImage>
#include <QFileInfo>
#include <QJsonArray>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTimer>
#include <QElapsedTimer>
#include <QTcpServer>
#include <functional>
#include <sys/prctl.h>
#include <signal.h>
#include "CoopIPC.h"
#include "SaveStore.h"
namespace coop {
struct Settings {
    QString emulator, rom, bios9, bios7, firmware;
    int layout=0; bool horizontal=false; bool patchedConfirmed=false;
};
inline QString quoted(QString value) {
    // JSON basic strings are valid TOML basic strings for these paths.
    return QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
}
inline QString validateRom(const QString& path) {
    QFile f(path); if(!f.open(QIODevice::ReadOnly)) fail("ROM fehlt oder ist nicht lesbar.");
    if(f.size()<512 || f.size()>512*1024*1024) fail("Keine gültige Nintendo-DS-ROM (Dateigröße).");
    auto header=f.read(16);
    if(header.size()<16 || header.mid(12,3)!="CPU") fail("Die ROM muss auf Pokémon Platin basieren (CPU*-Kennung).");
    return hashFile(path);
}
inline void requireMultiplayerPatch(const QString& sha256) {
    // Exact known-original fingerprint, checked against No-Intro metadata.
    // An unknown fingerprint is NOT proof of a compatible multiplayer patch.
    if(sha256=="9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217")
        fail("Unveränderte deutsche Platin-ROM erkannt. Für Koop wird eine mit dem passenden Project-PM-Patch vorbereitete ROM benötigt. Ob dieser Patch die deutsche Ausgabe unterstützt, muss anhand seiner Anleitung geprüft werden.");
    if(sha256=="fbce4c4def0c7797f8dd238a3d7a5e48b4a7e3abd86890ac65f6321cef781bdb")
        fail("Unveränderte USA-Rev-1-ROM erkannt. Zuerst Project PM anwenden: scripts/prepare-rom.py --rom ORIGINAL.nds --patch BUILD.zip --output GEPATCHT.nds. Die Ausgabe bleibt bei PM 0.4.5 englisch.");
}
inline void validateFirmware(const Settings& s) {
    bool any=!s.bios9.isEmpty()||!s.bios7.isEmpty()||!s.firmware.isEmpty();
    if(!any) return; // upstream's built-in free BIOS + generated firmware, direct boot
    if(s.bios9.isEmpty()||s.bios7.isEmpty()||s.firmware.isEmpty()) fail("Externe Firmware erfordert BIOS9, BIOS7 und Firmware zusammen.");
    if(QFileInfo(s.bios9).size()!=4096 || QFileInfo(s.bios7).size()!=16384) fail("DS-BIOS-Größen: ARM9 4096 Byte, ARM7 16384 Byte.");
    auto n=QFileInfo(s.firmware).size(); if(n!=131072&&n!=262144&&n!=524288) fail("Firmware muss 128, 256 oder 512 KiB haben.");
    for(const auto& path:{s.bios9,s.bios7,s.firmware}) { QFile f(path); if(!f.open(QIODevice::ReadOnly)) fail("Firmwaredatei nicht lesbar: "+path); }
}
class Session : public QObject {
    SaveStore& saves;
    std::array<std::unique_ptr<Mapping>,2> maps;
    std::array<std::unique_ptr<QProcess>,2> workers;
    std::unique_ptr<QTemporaryDir> runtime;
    std::array<std::unique_ptr<QFile>,2> logs;
    bool active=false, stopping=false, failed=false;
    QTimer monitor;
    QElapsedTimer elapsed;
    std::array<uint32_t,2> lastFrame{{0,0}};
    std::array<qint64,2> lastProgress{{0,0}};
    void cleanup() {
        active=false; monitor.stop();
        for(auto& p:workers) p.reset();
        for(auto& f:logs) f.reset();
        for(auto& m:maps) m.reset();
        runtime.reset(); saves.release();
        stopping=false;
    }
public:
    QString game, logDir;
    std::function<void(QString)> status;
    std::function<void(QString)> error;
    std::function<void()> ended;
    explicit Session(SaveStore& store):saves(store) {
        monitor.setInterval(500);
        QObject::connect(&monitor,&QTimer::timeout,this,[this]{
            if(stopping) return;
            for(int i=0;i<2;i++) {
                if(!workers[i] || workers[i]->state()==QProcess::NotRunning) { stop(); return; }
                auto n=maps[i]->data->frame.load();
                if(n!=lastFrame[i]) { lastFrame[i]=n; lastProgress[i]=elapsed.elapsed(); }
                if(elapsed.elapsed()-lastProgress[i]>15000) {
                    if(error) error("Spieler "+QString::number(i+1)+": seit 15 Sekunden kein Bild. ROM/Firmware und Logs prüfen.");
                    failed=true; stop(); return;
                }
            }
            const bool connected=maps[0]->data->peers.load()&&maps[1]->data->peers.load();
            const bool activated=maps[0]->data->bridgeActive.load()&&maps[1]->data->bridgeActive.load();
            if(status) status(connected?(activated?"TCP verbunden · ROM-Bridge aktiviert (Welt-Synchronisation im Spiel prüfen)":"TCP verbunden · Nach dem Pokédex: Wireless Play im Beutel aktivieren"):(elapsed.elapsed()>10000?"Verbindung ausstehend · Logs prüfen":"Lokale Verbindung wird aufgebaut …"));
        });
    }
    ~Session() { stop(); }
    bool running() const { return active; }
    Shared* shared(int p) const { return maps.at(p)?maps.at(p)->data:nullptr; }
    bool copyFrame(int p, QImage& top, QImage& bottom) {
        if(!maps[p] || !maps[p]->acquire()) return false;
        top=QImage(reinterpret_cast<const uchar*>(maps[p]->data->top),Width,Height,QImage::Format_RGB32).copy();
        bottom=QImage(reinterpret_cast<const uchar*>(maps[p]->data->bottom),Width,Height,QImage::Format_RGB32).copy();
        maps[p]->release(); return true;
    }
    void start(const Settings& s) {
        if(active) fail("Sitzung läuft bereits.");
        if(!s.patchedConfirmed) fail("Die vorbereitete Project-PM-ROM muss bestätigt werden. Eine normale Platin-ROM bietet keine gemeinsame Spielwelt.");
        game=validateRom(s.rom); requireMultiplayerPatch(game); validateFirmware(s);
        QFileInfo binary(s.emulator);
        if(!binary.isFile()||!binary.isExecutable()) fail("Gepatchtes Emulator-Binary fehlt: "+s.emulator);
        // Reject unrelated binaries before acquiring locks or launching sessions.
        QProcess capability; capability.start(s.emulator,{"--coop-capabilities"});
        if(!capability.waitForFinished(5000) || capability.exitCode()!=0 || !capability.readAllStandardOutput().contains("PMCOOP_ABI=1")) {
            capability.kill(); capability.waitForFinished(1000); fail("Emulator unterstützt die Co-op-Schnittstelle nicht. scripts/build.sh ausführen.");
        }
        saves.acquire();
        failed=false; stopping=false; elapsed.start(); lastFrame={{0,0}}; lastProgress={{0,0}};
        try {
            runtime=std::make_unique<QTemporaryDir>(saves.root()+"/run-XXXXXX");
            if(!runtime->isValid()) fail("Temporärer Sitzungsordner nicht anlegbar.");
            QFile::setPermissions(runtime->path(),QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
            // Reserve an available loopback port; bridge retries cover the brief bind handoff.
            QTcpServer probe;
            if(!probe.listen(QHostAddress::LocalHost,0)) fail("Kein lokaler TCP-Port verfügbar.");
            auto port=probe.serverPort(); probe.close();
            logDir=saves.root()+"/logs/"+QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmsszzz")+"-"+QUuid::createUuid().toString(QUuid::Id128);
            if(!QDir().mkpath(logDir)) fail("Logordner nicht anlegbar.");
            for(int i=0;i<2;i++) {
                const auto profile=saves.profile(i,game);
                for(const auto& name:{"saves","states","cheats"}) if(!QDir().mkpath(profile+"/"+name)) fail("Spielerprofil nicht anlegbar.");
                saves.backup(i,game);
                QString toml="LimitFPS = true\nTargetFPS = 60.0\n[Emu]\nDirectBoot = true\nExternalBIOSEnable = "+QString(s.bios9.isEmpty()?"false":"true")+"\n[Screen]\nUseGL = false\n[\"3D\"]\nRenderer = 0\n[DS]\nBIOS9Path = "+quoted(s.bios9)+"\nBIOS7Path = "+quoted(s.bios7)+"\nFirmwarePath = "+quoted(s.firmware)+"\n[Instance0]\nJoystickID = -1\nSaveFilePath = "+quoted(profile+"/saves")+"\nSavestatePath = "+quoted(profile+"/states")+"\nCheatFilePath = "+quoted(profile+"/cheats")+"\n[Instance0.Audio]\nVolume = 128\n[Instance0.Firmware]\nUsername = "+quoted("Spieler "+QString::number(i+1))+"\n";
                atomicWrite(profile+"/melonDS.toml",toml.toUtf8());
                auto ipc=runtime->path()+"/player"+QString::number(i+1)+".ipc";
                maps[i]=std::make_unique<Mapping>(); maps[i]->open(ipc.toStdString(),true);
                workers[i]=std::make_unique<QProcess>();
                workers[i]->setWorkingDirectory(profile);
                const pid_t parentPid=getpid();
                workers[i]->setChildProcessModifier([parentPid]{
                    if(prctl(PR_SET_PDEATHSIG,SIGTERM)!=0 || getppid()!=parentPid) _exit(127);
                });
                auto env=QProcessEnvironment::systemEnvironment();
                for(const auto& key:env.keys()) if(key.startsWith("MELONDS_AP")||key.startsWith("PMCOOP_")) env.remove(key);
                env.insert("PMCOOP_ROLE",i?"join":"host"); env.insert("PMCOOP_PORT",QString::number(port));
                env.insert("PMCOOP_IPC",ipc); env.insert("PMCOOP_PROFILE",profile);
                env.insert("QT_QPA_PLATFORM","offscreen");
                workers[i]->setProcessEnvironment(env);
                workers[i]->setProcessChannelMode(QProcess::MergedChannels);
                logs[i]=std::make_unique<QFile>(logDir+"/player"+QString::number(i+1)+".log");
                if(!logs[i]->open(QIODevice::WriteOnly)) fail("Logdatei nicht schreibbar.");
                QObject::connect(workers[i].get(),&QProcess::readyReadStandardOutput,this,[this,i]{ if(workers[i]&&logs[i]) { logs[i]->write(workers[i]->readAllStandardOutput()); logs[i]->flush(); } });
                QObject::connect(workers[i].get(),qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,i](int code,QProcess::ExitStatus how){
                    if(!stopping && active) {
                        failed=true;
                        if(error) error("Emulator "+QString::number(i+1)+" beendet (Code "+QString::number(code)+(how==QProcess::CrashExit?", Absturz":"")+"). Sitzung wird gestoppt.");
                        // Avoid deleting a QProcess inside its signal emission.
                        QTimer::singleShot(0,this,[this]{stop();});
                    }
                });
                workers[i]->start(binary.absoluteFilePath(),{QFileInfo(s.rom).absoluteFilePath()});
                if(!workers[i]->waitForStarted(5000)) fail("Emulator konnte nicht gestartet werden: "+workers[i]->errorString());
            }
            active=true; monitor.start();
        } catch(...) { stop(); throw; }
    }
    void stop() {
        if(stopping || (!active&&!saves.locked())) return;
        stopping=true; monitor.stop();
        for(auto& m:maps) if(m) { m->data->keys.store(0xFFF); m->data->touch.store(0); }
        for(auto& p:workers) if(p && p->state()!=QProcess::NotRunning) p->terminate();
        for(int i=0;i<2;i++) if(workers[i]) {
            if(!workers[i]->waitForFinished(8000) && workers[i]->state()!=QProcess::NotRunning) {
                workers[i]->kill(); workers[i]->waitForFinished(3000); failed=true;
                if(error) error("Emulator musste beendet werden. Letzten Spielstand und Backup prüfen.");
            }
            if(logs[i]) { logs[i]->write(workers[i]->readAllStandardOutput()); logs[i]->flush(); }
        }
        bool notify=active; cleanup();
        if(status) status(failed?"Sitzung mit Fehler beendet · Logs/Backups prüfen":"Sitzung beendet · Spielstände bleiben getrennt erhalten");
        if(notify&&ended) ended();
    }
};
}
