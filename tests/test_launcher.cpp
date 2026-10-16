// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the production widget and SDL navigation, without physical hardware.
#define PLATINUM_COOP_NO_MAIN
#include "../launcher/main.cpp"
#include <QMouseEvent>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QThread>
#include <QTextLayout>
#include <cassert>
#include <iostream>

static void spin(int ms) {
    QElapsedTimer elapsed; elapsed.start();
    while(elapsed.elapsed()<ms) {QCoreApplication::processEvents();QThread::msleep(5);}
}
static void menuFits() {
    QTemporaryDir data;
    atomicWrite(data.path()+"/settings.json",QJsonDocument(QJsonObject{{"rom",QString(250,'r')+".nds"}}).toJson());
    Launcher launcher(data.path());launcher.show();launcher.activateWindow();spin(50);
    assert(theme::pixelFamily().contains("Silkscreen"));
    auto* scroll=launcher.findChild<QScrollArea*>("menuScroll");assert(scroll);
    for(auto size:{QSize(1280,720),QSize(1366,768),QSize(640,360),QSize(1040,720),QSize(1920,1080),QSize(3840,2160)}) {
        launcher.resize(size);spin(50);assert(launcher.size()==size);
        assert(scroll->horizontalScrollBar()->maximum()==0);
        for(auto* button:launcher.findChildren<QPushButton*>()) {
            const int textWidth=button->fontMetrics().horizontalAdvance(button->text());
            assert(textWidth+button->iconSize().width()+28<=button->width());
            button->setFocus();spin(20);
            auto position=button->mapTo(scroll->viewport(),QPoint());
            assert(scroll->viewport()->rect().contains(QRect(position,button->size())));
        }
    }
}
static void backgroundMenu() {
    for(bool hide:{false,true}) {
        QTemporaryDir data;Launcher launcher(data.path());
        int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
        assert(index>=0);auto* pad=SDL_JoystickOpen(index);assert(pad);
        launcher.show();launcher.activateWindow();spin(60);
        auto* exit=launcher.findChild<QPushButton*>("exitLauncher");assert(exit);
        int clicks=0;QObject::connect(exit,&QPushButton::clicked,[&]{clicks++;});exit->setFocus();spin(40);
        if(hide)launcher.hide();else launcher.showMinimized();
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,1);spin(100);
        assert(clicks==0);assert(hide?!launcher.isVisible():launcher.isMinimized());
        launcher.showNormal();launcher.activateWindow();spin(30);
        QApplication::setActiveWindow(&launcher);exit->setFocus();spin(100);
        assert(clicks==0&&launcher.isVisible()); // Held input must not fire on restore.
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,0);spin(60);
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,1);spin(100);
        assert(clicks==1&&!launcher.isVisible());
        SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,0);
        SDL_JoystickClose(pad);SDL_JoystickDetachVirtual(index);
    }
}
static void assignedMenu() {
    QTemporaryDir data;Launcher launcher(data.path());
    std::vector<SDL_Joystick*> pads;
    auto attach=[&]{int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);assert(index>=0);auto* pad=SDL_JoystickOpen(index);assert(pad);pads.push_back(pad);return index;};
    attach();const int firstPlayer=attach();attach();
    launcher.show();QApplication::setActiveWindow(&launcher);spin(50);
    auto assign=[&]{
        bool selected=false;QTimer choose;choose.setInterval(10);
        QObject::connect(&choose,&QTimer::timeout,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if(!dialog || dialog->windowTitle()!="Controller zuordnen")return;
            auto combos=dialog->findChildren<QComboBox*>();assert(combos.size()==2);
            combos[0]->setCurrentIndex(1);combos[1]->setCurrentIndex(2);selected=true;dialog->accept();
        });choose.start();
        for(auto* button:launcher.findChildren<QPushButton*>())if(button->text()=="Controller zuordnen")button->click();
        choose.stop();assert(selected);QApplication::setActiveWindow(&launcher);spin(50);
    };
    assign();auto* exit=launcher.findChild<QPushButton*>("exitLauncher");assert(exit);
    exit->setFocus();spin(40);assert(QApplication::focusWidget()==exit);
    for(int p:{0,2}) { // Neither the unassigned pad nor player 2 owns the menu.
        SDL_JoystickSetVirtualButton(pads[p],SDL_CONTROLLER_BUTTON_A,1);spin(70);assert(launcher.isVisible());
        SDL_JoystickSetVirtualButton(pads[p],SDL_CONTROLLER_BUTTON_A,0);spin(50);
    }
    auto* start=launcher.findChildren<QPushButton*>().first();start->setFocus();spin(30);
    SDL_JoystickSetVirtualButton(pads[1],SDL_CONTROLLER_BUTTON_DPAD_DOWN,1);spin(70);
    assert(QApplication::focusWidget()!=start);
    SDL_JoystickSetVirtualButton(pads[1],SDL_CONTROLLER_BUTTON_DPAD_DOWN,0);spin(50);exit->setFocus();
    assert(SDL_JoystickDetachVirtual(firstPlayer)==0);spin(60);
    attach(); // Device indices shifted; no automatic ownership transfer.
    for(int p:{0,2,3}) {
        SDL_JoystickSetVirtualButton(pads[p],SDL_CONTROLLER_BUTTON_A,1);spin(70);assert(launcher.isVisible());
        SDL_JoystickSetVirtualButton(pads[p],SDL_CONTROLLER_BUTTON_A,0);spin(50);
    }
    assign(); // Explicitly choose the remaining original pad as the new player 1.
    exit->setFocus();spin(50);SDL_JoystickSetVirtualButton(pads[2],SDL_CONTROLLER_BUTTON_A,1);spin(100);
    assert(!launcher.isVisible());
    for(auto* pad:pads)SDL_JoystickClose(pad);
    for(int i=SDL_NumJoysticks()-1;i>=0;i--)if(SDL_JoystickIsVirtual(i))SDL_JoystickDetachVirtual(i);
}
static void playHelpFits() {
    QTemporaryDir data;SaveStore saves(data.path());Session session(saves);Controllers controllers;Settings settings;
    PlayView view(session,controllers,settings);view.show();
    const QString expected="Touch: rechter Stick + Klick · Ansicht: Select + R · Ende: Start + Select (2 s)";
    for(auto size:{QSize(640,360),QSize(1280,720),QSize(1366,768),QSize(1920,1080),QSize(3840,2160)})for(bool horizontal:{false,true}) {
        settings.horizontal=horizontal;view.resize(size);view.update();spin(20);
        for(int p=0;p<2;p++) {
            auto* label=view.findChild<QLabel*>(QString("playHelp%1").arg(p+1));assert(label&&label->isVisible()&&label->wordWrap()&&label->text()==expected);
            const auto area=playerArea(view.rect(),p,horizontal);assert(area.contains(QRectF(label->geometry())));
            assert(label->geometry().top()>area.top()+64); // Space remains for the DS images.
            QTextLayout text(label->text(),label->font());QTextOption option;option.setWrapMode(QTextOption::WordWrap);text.setTextOption(option);
            qreal height=0;int characters=0;text.beginLayout();
            while(true) {auto line=text.createLine();if(!line.isValid())break;line.setLineWidth(label->contentsRect().width());assert(line.naturalTextWidth()<=label->contentsRect().width()+1);height+=line.height();characters+=line.textLength();}
            text.endLayout();assert(characters==expected.size()&&height<=label->contentsRect().height()+1);
        }
    }
}

int main(int argc,char** argv) {
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    QTemporaryDir mouseData,controllerData,sessionData;
    menuFits();backgroundMenu();assignedMenu();playHelpFits();
    {
        QTemporaryDir keyboardData;Launcher launcher(keyboardData.path());launcher.show();spin(40);
        auto* exit=launcher.findChild<QPushButton*>("exitLauncher");assert(exit);exit->setFocus();
        QKeyEvent press(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier),release(QEvent::KeyRelease,Qt::Key_Space,Qt::NoModifier);
        QCoreApplication::sendEvent(exit,&press);QCoreApplication::sendEvent(exit,&release);spin(30);
        assert(!launcher.isVisible()); // The advertised native keyboard activation must work.
    }
    {
        Launcher launcher(mouseData.path());launcher.resize(1280,720);launcher.show();spin(50);
        auto* exit=launcher.findChild<QPushButton*>("exitLauncher");assert(exit&&exit->isVisible());
        launcher.findChild<QScrollArea*>("menuScroll")->ensureWidgetVisible(exit);spin(30);
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
        launcher.resize(1280,720);launcher.show();launcher.activateWindow();spin(80);
        auto* exit=launcher.findChild<QPushButton*>("exitLauncher");assert(exit);
        auto* start=launcher.findChildren<QPushButton*>().first();start->setFocus();spin(50);
        // Drive the same timer used by physical controllers, including focus traversal.
        int steps=0;
        while(QApplication::focusWidget()!=exit&&steps++<20) {
            assert(SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_DPAD_DOWN,1)==0);spin(70);
            assert(SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_DPAD_DOWN,0)==0);spin(70);
        }
        assert(QApplication::focusWidget()==exit);
        auto* scroll=launcher.findChild<QScrollArea*>("menuScroll");// A complete menu can now fit at 720p; scrolling is required only when needed.
        if(scroll->widget()->height()>scroll->viewport()->height())assert(scroll->verticalScrollBar()->value()>0);
        assert(scroll->viewport()->rect().contains(QRect(exit->mapTo(scroll->viewport(),QPoint()),exit->size())));
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
        auto* ja=SDL_JoystickOpen(a);auto* jb=SDL_JoystickOpen(b);assert(ja&&jb);
        for(auto* button:launcher.findChildren<QPushButton*>())if(button->text()=="Koop-Spiel starten")button->click();
        spin(150);auto* pages=launcher.findChild<QStackedWidget*>();assert(pages&&pages->currentIndex()==1);
        assert(QFileInfo::exists(sessionData.path()+"/session.lock"));
        {
            const auto runs=QDir(sessionData.path()).entryList({"run-*"},QDir::Dirs|QDir::NoDotAndDotDot);assert(runs.size()==1);
            std::array<Mapping,2> input;
            for(int p=0;p<2;p++)input[p].open((sessionData.path()+"/"+runs.first()+"/player"+QString::number(p+1)+".ipc").toUtf8().toStdString(),false);
            SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_B,1);
            SDL_JoystickSetVirtualButton(jb,SDL_CONTROLLER_BUTTON_B,1);
            SDL_JoystickSetVirtualButton(jb,SDL_CONTROLLER_BUTTON_RIGHTSTICK,1);spin(100);
            assert(input[0].data->keys.load()!=0xFFF&&input[1].data->keys.load()!=0xFFF&&input[1].data->touch.load());
            launcher.showMinimized();assert(launcher.isMinimized());
            for(auto& map:input)assert(map.data->keys.load()==0xFFF&&map.data->touch.load()==0);
            SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_DPAD_RIGHT,1);spin(100);
            for(auto& map:input)assert(map.data->keys.load()==0xFFF&&map.data->touch.load()==0);
            launcher.showNormal();spin(100);assert(input[0].data->keys.load()!=0xFFF);
            // Time spent minimized must not count toward the two-second exit chord.
            SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_BACK,1);
            SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_START,1);spin(100);
            launcher.showMinimized();spin(2100);launcher.showNormal();spin(100);
            assert(pages->currentIndex()==1&&QFileInfo::exists(sessionData.path()+"/session.lock"));
            SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_BACK,0);
            SDL_JoystickSetVirtualButton(ja,SDL_CONTROLLER_BUTTON_START,0);
        }
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
        QCoreApplication::sendEvent(pages->currentWidget(),&escape);assert(pages->currentIndex()==0);
        // A live session must not silently diverge from the selected ROM/profile.
        QFile settingsFile(sessionData.path()+"/settings.json");assert(settingsFile.open(QIODevice::ReadOnly));
        const auto before=settingsFile.readAll();settingsFile.close();
        bool warning=false,fileDialog=false;QTimer dismiss;dismiss.setInterval(10);
        QObject::connect(&dismiss,&QTimer::timeout,[&]{
            if(auto* message=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
                warning=message->text().contains("Vor einem ROM-Wechsel")&&message->text().contains("Start + Select");message->accept();
            } else if(auto* file=qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) {fileDialog=true;file->reject();}
        });dismiss.start();
        for(auto* button:launcher.findChildren<QPushButton*>())if(button->text()=="ROM auswählen")button->click();
        dismiss.stop();assert(warning&&!fileDialog&&pages->currentIndex()==0);
        assert(settingsFile.open(QIODevice::ReadOnly));assert(settingsFile.readAll()==before);settingsFile.close();
        assert(QFileInfo::exists(sessionData.path()+"/session.lock"));
        for(auto* button:launcher.findChildren<QPushButton*>())if(button->text()=="Koop-Spiel starten")button->click();
        assert(pages->currentIndex()==1);
        QCoreApplication::sendEvent(pages->currentWidget(),&escape);assert(pages->currentIndex()==0);
        launcher.findChild<QPushButton*>("exitLauncher")->click();assert(!launcher.isVisible());
        SaveStore probe(sessionData.path());probe.acquire();probe.release();
        auto sessions=QDir(sessionData.path()+"/logs").entryList(QDir::Dirs|QDir::NoDotAndDotDot);assert(sessions.size()==1);
        for(int p=1;p<=2;p++) {
            QFile log(sessionData.path()+"/logs/"+sessions.first()+"/player"+QString::number(p)+".log");
            assert(log.open(QIODevice::ReadOnly));assert(log.readAll().contains("TEST_ONLY: graceful exit"));
        }
        SDL_JoystickClose(ja);SDL_JoystickClose(jb);SDL_JoystickDetachVirtual(b);SDL_JoystickDetachVirtual(a);
    }
    std::cout<<"PASS: menu and full wrapped play help at 360p/720p/768p/1080p/4K in both splits; assigned player-one menu input, no unassigned/player-two/replacement takeover after disconnect, explicit reassignment; focus scrolling, blocked hidden/minimized menu and held input on restore, blocked live ROM change, neutral minimized input and reset exit chord; fixture workers release saves\n";
    return 0;
}
