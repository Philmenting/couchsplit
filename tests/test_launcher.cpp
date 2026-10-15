// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the production widget and SDL navigation, without physical hardware.
#define PLATINUM_COOP_NO_MAIN
#include "../launcher/main.cpp"
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QThread>
#include <cassert>
#include <iostream>

static void spin(int ms) {
    QElapsedTimer elapsed; elapsed.start();
    while(elapsed.elapsed()<ms) {QCoreApplication::processEvents();QThread::msleep(5);}
}

int main(int argc,char** argv) {
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    QTemporaryDir mouseData,controllerData,sessionData;
    {
        Launcher launcher(mouseData.path());launcher.show();spin(50);
        auto* exit=launcher.findChild<QPushButton*>("exitLauncher");assert(exit&&exit->isVisible());
        QPointF local=exit->rect().center(),global=exit->mapToGlobal(exit->rect().center());
        QMouseEvent press(QEvent::MouseButtonPress,local,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease,local,global,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QCoreApplication::sendEvent(exit,&press);QCoreApplication::sendEvent(exit,&release);spin(50);
        assert(!launcher.isVisible());
    }
    {
        Launcher launcher(controllerData.path());
        int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
        assert(index>=0);auto* pad=SDL_JoystickOpen(index);assert(pad);
        launcher.show();launcher.activateWindow();spin(80);
        auto* exit=launcher.findChild<QPushButton*>("exitLauncher");assert(exit);
        auto* start=launcher.findChildren<QPushButton*>().first();start->setFocus();spin(50);
        // Drive the same timer used by physical controllers, including focus traversal.
        int steps=0;
        while(QApplication::focusWidget()!=exit&&steps++<20) {
            assert(SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_DPAD_DOWN,1)==0);spin(70);
            assert(SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_DPAD_DOWN,0)==0);spin(70);
        }
        assert(QApplication::focusWidget()==exit);
        assert(SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,1)==0);spin(100);
        assert(!launcher.isVisible());
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,0);
        SDL_JoystickClose(pad);SDL_JoystickDetachVirtual(index);
    }
    {
        assert(argc==2);auto rom=sessionData.path()+"/TEST-ONLY-header.nds";
        QByteArray header(1024,0);header.replace(12,4,"CPUE");atomicWrite(rom,header);
        QJsonObject settings{{"rom",rom},{"emulator",QFileInfo(argv[1]).absoluteFilePath()},{"patchedConfirmed",true}};
        atomicWrite(sessionData.path()+"/settings.json",QJsonDocument(settings).toJson());
        Launcher launcher(sessionData.path());launcher.show();spin(50);
        int a=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
        int b=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
        assert(a>=0&&b>=0);
        for(auto* button:launcher.findChildren<QPushButton*>())if(button->text()=="Koop-Spiel starten")button->click();
        spin(150);auto* pages=launcher.findChild<QStackedWidget*>();assert(pages&&pages->currentIndex()==1);
        assert(QFileInfo::exists(sessionData.path()+"/session.lock"));
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
        QCoreApplication::sendEvent(pages->currentWidget(),&escape);assert(pages->currentIndex()==0);
        launcher.findChild<QPushButton*>("exitLauncher")->click();assert(!launcher.isVisible());
        SaveStore probe(sessionData.path());probe.acquire();probe.release();
        auto sessions=QDir(sessionData.path()+"/logs").entryList(QDir::Dirs|QDir::NoDotAndDotDot);assert(sessions.size()==1);
        for(int p=1;p<=2;p++) {
            QFile log(sessionData.path()+"/logs/"+sessions.first()+"/player"+QString::number(p)+".log");
            assert(log.open(QIODevice::ReadOnly));assert(log.readAll().contains("TEST_ONLY: graceful exit"));
        }
        SDL_JoystickDetachVirtual(b);SDL_JoystickDetachVirtual(a);
    }
    std::cout<<"PASS: launcher closes via mouse and virtual controller; active fixture workers stop gracefully and release saves\n";
    return 0;
}
