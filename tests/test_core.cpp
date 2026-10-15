// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QTemporaryDir>
#include <QThread>
#include <cassert>
#include <functional>
#include <iostream>

#include "Controllers.h"
#include "Session.h"
#include "Layout.h"
using namespace coop;
void rejects(std::function<void()> f) {bool caught=false;try{f();}catch(const std::exception&){caught=true;}assert(caught);}
void spin(int ms){QElapsedTimer t;t.start();while(t.elapsed()<ms){QCoreApplication::processEvents();QThread::msleep(5);}}
void ipcTest(const QString& fixture) {
    QTemporaryDir temp;auto path=(temp.path()+QString::fromUtf8("/map-ä")).toUtf8().toStdString();Mapping a;a.open(path,true);
    Mapping b;b.open(path,false);b.data->keys.store(42);assert(a.data->keys.load()==42);
    a.data->touch.store(touch(255,191,true));assert((b.data->touch.load()&0xFFFF)==(255|(191<<8)));
    assert(a.acquire());assert(!b.acquire());a.release();assert(b.acquire());b.release();
    uint32_t top[Pixels]{},bottom[Pixels]{};top[0]=0xFF123456;bottom[0]=0xFFABCDEF;a.publish(top,bottom);
    assert(b.data->frame.load()==1&&b.data->top[0]==top[0]&&b.data->bottom[0]==bottom[0]);
    QProcess child;child.start(fixture,{"--ipc-write",QString::fromUtf8(path)});
    assert(child.waitForFinished(5000));assert(child.exitCode()==0);assert(a.data->keys.load()==123);
    atomicWrite(temp.path()+"/short",QByteArray("bad"));Mapping shortFile;rejects([&]{shortFile.open((temp.path()+"/short").toStdString(),false);});
#ifndef _WIN32
    assert(symlink(path.c_str(),(temp.path()+"/link").toStdString().c_str())==0);Mapping link;rejects([&]{link.open((temp.path()+"/link").toStdString(),false);});
#endif
}
void layoutTest() {
    for(auto size:{QSize(3840,2160),QSize(1920,1080),QSize(1280,720),QSize(800,1200)})for(bool horizontal:{false,true})for(int p=0;p<2;p++)for(int layout=0;layout<3;layout++)for(bool bottom:{false,true}) {
        auto area=playerArea({0,0,double(size.width()),double(size.height())},p,horizontal);
        auto positions=screens(area,layout,bottom);
        for(auto r:positions)if(!r.isEmpty()){assert(area.contains(r));assert(std::abs(r.width()/r.height()-4.0/3.0)<1e-8);}
        if(layout<2)assert(!positions[0].intersects(positions[1]));
    }
    auto area=playerArea({0,0,3840,2160},1,false);assert(area==QRectF(1920,0,1920,2160));
}
void saveTest() {
    QTemporaryDir temp;SaveStore a(temp.path()),b(temp.path());a.acquire();rejects([&]{b.acquire();});
    auto game=QString(64,'a');auto save=a.profile(0,game)+"/saves";QDir().mkpath(save);
    atomicWrite(save+"/game.sav","original");auto backup=a.backup(0,game);atomicWrite(save+"/game.sav","new");
    a.restore(0,game,backup);QFile f(save+"/game.sav");assert(f.open(QIODevice::ReadOnly));assert(f.readAll()=="original");f.close();
    assert(!QFileInfo::exists(a.profile(1,game)+"/saves/game.sav"));
    atomicWrite(backup+"/game.sav","corrupt");rejects([&]{a.restore(0,game,backup);});
    assert(f.open(QIODevice::ReadOnly));assert(f.readAll()=="original");f.close();
    rejects([&]{a.restore(0,game,temp.path());});a.release();b.acquire();b.release();
}
void controllerTest() {
    Controllers c;
    int a=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
    int b=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
    assert(a>=0&&b>=0);auto* ja=SDL_JoystickOpen(a);auto* jb=SDL_JoystickOpen(b);assert(ja&&jb);
    auto ia=SDL_JoystickInstanceID(ja),ib=SDL_JoystickInstanceID(jb);c.assign(ia,ib);
    rejects([&]{c.assign(ia,ia);});assert(c.id(0)==ia&&c.id(1)==ib);
    SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_B,1);c.update();
    assert((c.read(0).keys&1)==0);assert((c.read(1).keys&1)==1);
    SDL_JoystickSetVirtualButton(jb,SDL_CONTROLLER_BUTTON_RIGHTSTICK,1);c.update();assert(c.read(1).touch&0x80000000u);assert(!c.read(0).touch);
    SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_BACK,1);SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,1);c.update();assert(c.read(0).toggle);assert(!c.read(0).toggle);
    SDL_JoystickDetachVirtual(a);c.update();assert(!c.connected(0));assert(c.read(0).keys==0xFFF);assert(!c.read(0).touch);assert(c.connected(1));
    // Device indices shifted, but player 1 must never adopt player 2.
    assert(c.id(0)==ia);assert(c.id(1)==ib);
    SDL_JoystickClose(ja);SDL_JoystickClose(jb);c.clear();SDL_JoystickDetachVirtual(0);
}
void validationTest() {
    QTemporaryDir temp;auto rom=temp.path()+"/game.nds";
    QByteArray header(1024,0);header.replace(12,4,"CPUE");atomicWrite(rom,header);assert(validateRom(rom).size()==64);
    rejects([&]{validateRom(temp.path()+"/missing.nds");});header.replace(12,4,"ADAE");atomicWrite(rom,header);rejects([&]{validateRom(rom);});
    Settings s;validateFirmware(s);s.bios9="missing";rejects([&]{validateFirmware(s);});
    rejects([&]{requireMultiplayerPatch("9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217");});
    requireMultiplayerPatch(QString(64,'a')); // Unknown hash still needs explicit user confirmation.
    assert(knownPreparedRom(QString(64,'a')).isEmpty());
    assert(knownPreparedRom("800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c").contains("Deutsch Alpha 2"));
    assert(quoted("a\"b\\c")=="\"a\\\"b\\\\c\"");
}
void processTest(const QString& fixture) {
    QTemporaryDir temp;SaveStore store(temp.path());Session session(store);Settings s;s.emulator=fixture;
    s.rom=temp.path()+"/legal-test-header.nds";QByteArray header(1024,0);header.replace(12,4,"CPUE");atomicWrite(s.rom,header);s.patchedConfirmed=true;
    int errors=0;session.error=[&](QString){errors++;};
    for(int i=0;i<3;i++) {session.start(s);spin(120);assert(session.running());assert(session.shared(0)->frame.load()>0&&session.shared(1)->frame.load()>0);session.stop();assert(!session.running()&&!store.locked());}
    qputenv("COOP_FIXTURE_CRASH_PLAYER","1");session.start(s);spin(700);assert(!session.running()&&!store.locked()&&errors>0);qunsetenv("COOP_FIXTURE_CRASH_PLAYER");
    s.emulator=temp.path()+"/missing";rejects([&]{session.start(s);});assert(!store.locked());
}
int main(int argc,char**argv) {
    QApplication app(argc,argv);assert(argc==2);auto fixture=QFileInfo(argv[1]).absoluteFilePath();
    ipcTest(fixture);layoutTest();saveTest();controllerTest();validationTest();processTest(fixture);
    std::cout<<"PASS: IPC across processes, 4:3 layouts, profile locks, verified restore, corruption rejection, virtual controller isolation/hotplug/touch, validation, repeated starts, graceful stop, worker failure cleanup\n";
}
