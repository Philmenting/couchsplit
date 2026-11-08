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
// Optional captures use real production widgets and no ROM or personal profile.
static void capture(QWidget& widget,const QString& name) {
    const auto directory=qEnvironmentVariable("COOP_UI_CAPTURE_DIR");
    if(directory.isEmpty())return;
    assert(QDir().mkpath(directory));assert(widget.grab().save(directory+"/"+name+".png"));
}
static QJsonObject preferences(const QString& root) {
    QFile file(root+"/settings.json");assert(file.open(QIODevice::ReadOnly));
    return QJsonDocument::fromJson(file.readAll()).object();
}
static bool chooseLanguage(Launcher& launcher,const QString& choice) {
    bool prompted=false;QTimer select;select.setInterval(10);
    QObject::connect(&select,&QTimer::timeout,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if(!dialog || dialog->objectName()!="languageDialog")return;
        prompted=true;capture(*dialog,"language-picker");
        if(choice.isEmpty())dialog->reject();
        else {auto* button=dialog->findChild<QPushButton*>("language-"+choice);assert(button);button->click();}
    });select.start();
    const bool accepted=launcher.ensureLanguageSelected();select.stop();
    assert(accepted==!choice.isEmpty());return prompted;
}
static void configureLanguage(Launcher& launcher,const QString& choice,bool accept=true) {
    bool prompted=false;QTimer select;select.setInterval(10);
    QObject::connect(&select,&QTimer::timeout,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if(!dialog)return;
        auto* combo=dialog->findChild<QComboBox*>("languageCombo");if(!combo)return;
        prompted=true;select.stop();spin(50);assert(combo->count()==2);combo->setCurrentIndex(combo->findData(choice));
        for(auto* label:dialog->findChildren<QLabel*>())if(label->wordWrap()) {
            assert(label->height()>=label->heightForWidth(label->width()));
        }
        capture(*dialog,"settings-"+i18n::language()+(launcher.width()<760?"-compact":""));
        auto* scroll=dialog->findChild<QScrollArea*>("settingsScroll");assert(scroll);
        assert(scroll->horizontalScrollBar()->maximum()==0);
        if(launcher.width()<760)assert(scroll->verticalScrollBar()->maximum()>0);
        if(accept)dialog->accept();else dialog->reject();
    });select.start();
    launcher.findChild<QPushButton*>("settingsAction")->click();select.stop();assert(prompted);spin(30);
}
static void languagePreferences() {
    for(const auto& choice:{QString("en"),QString("de")}) {
        QTemporaryDir data;
        const QJsonObject original{{"rom","unchanged-ROM.nds"},{"emulator","custom-emulator"},{"layout",2},{"horizontal",true},
            {"pad1",QJsonObject{{"guid","TEST_ONLY"},{"name","Test pad"}}},{"futureSetting",QJsonArray{1,"untouched"}}};
        atomicWrite(data.path()+"/settings.json",QJsonDocument(original).toJson());
        {Launcher launcher(data.path());assert(chooseLanguage(launcher,choice));
            auto expected=original;expected["language"]=choice;assert(preferences(data.path())==expected);
            assert(i18n::language()==choice);assert(launcher.findChild<QPushButton*>("settingsAction")->text()==(choice=="en"?"Settings":"Einstellungen"));}
        {Launcher again(data.path());assert(again.ensureLanguageSelected());assert(i18n::language()==choice);}
    }
    {
        QTemporaryDir data;Launcher launcher(data.path());assert(chooseLanguage(launcher,QString()));
        assert(!QFileInfo::exists(data.path()+"/settings.json"));assert(!QFileInfo::exists(data.path()+"/session.lock"));
    }
    {
        QTemporaryDir data;atomicWrite(data.path()+"/settings.json",QJsonDocument(QJsonObject{{"language","fr"}}).toJson());
        Launcher launcher(data.path());assert(chooseLanguage(launcher,"en"));assert(preferences(data.path())["language"]=="en");
        launcher.show();configureLanguage(launcher,"de",false);assert(i18n::language()=="en");assert(preferences(data.path())["language"]=="en");
        configureLanguage(launcher,"de");assert(i18n::language()=="de");assert(preferences(data.path())["language"]=="de");
        assert(launcher.findChild<QPushButton*>("startGame")->text()=="Koop-Spiel starten");
        assert(launcher.findChild<QLabel*>("playHelp1")->text().contains("rechter Stick"));
        configureLanguage(launcher,"en");assert(launcher.findChild<QPushButton*>("startGame")->text()=="Start co-op game");
        assert(launcher.findChild<QLabel*>("playHelp1")->text().contains("right stick"));
        assert(launcher.findChild<QLabel*>("navigationHelp")->text().contains("D-pad"));
        assert(launcher.findChild<QLabel*>("sessionStatus")->text().contains("No game running"));
        assert(launcher.findChild<QLabel*>("romLabel")->text()=="No game ROM selected");
        launcher.resize(640,360);spin(30);configureLanguage(launcher,"de");configureLanguage(launcher,"en");
    }
    {
        QTemporaryDir data;const QByteArray corrupt="{broken settings";atomicWrite(data.path()+"/settings.json",corrupt);
        bool rejected=false;try{Launcher launcher(data.path());}catch(const std::exception& e){rejected=QString::fromUtf8(e.what()).contains("Settings are corrupted");}
        assert(rejected);QFile file(data.path()+"/settings.json");assert(file.open(QIODevice::ReadOnly)&&file.readAll()==corrupt);
    }
    {
        QTemporaryDir data;Launcher launcher(data.path());assert(QDir().mkdir(data.path()+"/settings.json"));
        bool failed=false;try{chooseLanguage(launcher,"de");}catch(const std::exception&){failed=true;}
        assert(failed&&i18n::language()=="en");assert(QFileInfo(data.path()+"/settings.json").isDir());
        assert(QDir().rmdir(data.path()+"/settings.json"));assert(chooseLanguage(launcher,"de")); // rollback still prompts
    }
    // Validation messages follow the app language while diagnostic file paths stay intact.
    i18n::setLanguage("en");
    bool failed=false;try{validateRom("MISSING-TEST-ROM.nds");}catch(const std::exception& e){failed=QString::fromUtf8(e.what())=="ROM is missing or unreadable.";}assert(failed);
    QDialogButtonBox buttons(QDialogButtonBox::Cancel|QDialogButtonBox::Close);theme::labelButtons(buttons);
    assert(buttons.button(QDialogButtonBox::Cancel)->text()=="Cancel");
    i18n::setLanguage("de");theme::labelButtons(buttons);assert(buttons.button(QDialogButtonBox::Close)->text()=="Schließen");
}
static void gbaSelection() {
    QTemporaryDir data;const auto rom=data.path()+"/own-homebrew.gba";
    QByteArray header(1024,0);header.replace(0xAC,4,"TEST");header[0xB2]=char(0x96);
    unsigned sum=0x19;for(int i=0xA0;i<0xBD;i++)sum+=static_cast<unsigned char>(header[i]);header[0xBD]=char(-sum);
    atomicWrite(rom,header);
    atomicWrite(data.path()+"/settings.json",QJsonDocument(QJsonObject{{"language","de"},{"fireRedCoop",true}}).toJson());
    Launcher launcher(data.path());launcher.show();
    bool picked=false;QTimer picker;picker.setInterval(10);
    QObject::connect(&picker,&QTimer::timeout,[&]{
        auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        assert(dialog->nameFilters().join(" ").contains("*.gba"));
        dialog->selectFile(rom);picked=true;picker.stop();QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);
    });picker.start();
    for(auto* button:launcher.findChildren<QPushButton*>())if(button->text()=="ROM auswählen")button->click();
    picker.stop();assert(picked);
    auto selected=preferences(data.path())["rom"].toString();
    assert(inspectRom(selected).system==GameSystem::GameBoyAdvance && hashFile(selected)==hashFile(rom));
    assert(launcher.findChild<QPushButton*>("startGame")->text()=="Splitscreen starten");
    assert(launcher.findChild<QLabel*>("subtitle")->text()=="Zwei Spieler. Zwei eigene Abenteuer.");
    capture(launcher,"gba-menu-de");configureLanguage(launcher,"en");
    assert(launcher.findChild<QPushButton*>("startGame")->text()=="Start split screen");
    capture(launcher,"gba-menu-en");
    Launcher reopened(data.path());assert(reopened.findChild<QPushButton*>("startGame")->text()=="Start split screen");
}
static void fireRedAudioPreferences() {
    const bool present=qEnvironmentVariableIsSet("COUCHSPLIT_FIRERED_EXPERIMENT");
    const auto previous=qgetenv("COUCHSPLIT_FIRERED_EXPERIMENT");
    qputenv("COUCHSPLIT_FIRERED_EXPERIMENT","1");
    QTemporaryDir data;
    atomicWrite(data.path()+"/settings.json",QJsonDocument(QJsonObject{{"language","en"}}).toJson());
    auto choose=[&](Launcher& launcher,const QString& expected,const QString& selected,bool accept) {
        bool visited=false;QTimer timer;timer.setInterval(10);
        QObject::connect(&timer,&QTimer::timeout,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            auto* combo=dialog->findChild<QComboBox*>("fireRedAudioCombo");if(!combo)return;
            assert(combo->count()==4 && combo->currentData().toString()==expected);
            assert(combo->itemText(2)=="Both players" && combo->itemText(3)=="Mute");
            combo->setCurrentIndex(combo->findData(selected));visited=true;timer.stop();
            if(accept)dialog->accept();else dialog->reject();
        });timer.start();launcher.findChild<QPushButton*>("settingsAction")->click();timer.stop();assert(visited);
    };
    {Launcher launcher(data.path());launcher.show();
        choose(launcher,"player1","both",false);
        assert(!preferences(data.path()).contains("fireRedAudio"));
        choose(launcher,"player1","both",true);
        assert(preferences(data.path())["fireRedAudio"]=="both");}
    {Launcher launcher(data.path());launcher.show();choose(launcher,"both","mute",true);
        assert(preferences(data.path())["fireRedAudio"]=="mute");}
    if(present)qputenv("COUCHSPLIT_FIRERED_EXPERIMENT",previous);else qunsetenv("COUCHSPLIT_FIRERED_EXPERIMENT");
    i18n::setLanguage("de");
}
static void fireRedCoopPreferences() {
    const bool present=qEnvironmentVariableIsSet("COUCHSPLIT_FIRERED_EXPERIMENT");
    const auto previous=qgetenv("COUCHSPLIT_FIRERED_EXPERIMENT");qunsetenv("COUCHSPLIT_FIRERED_EXPERIMENT");
    QTemporaryDir data;const QJsonObject original{{"language","en"},{"futureSetting",42}};
    atomicWrite(data.path()+"/settings.json",QJsonDocument(original).toJson());
    auto choose=[&](Launcher& launcher,bool expected,bool selected,bool accept) {
        bool visited=false;QTimer timer;timer.setInterval(10);
        QObject::connect(&timer,&QTimer::timeout,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            if(auto* warning=qobject_cast<QMessageBox*>(dialog)){warning->accept();return;}
            auto* check=dialog->findChild<QCheckBox*>("fireRedCoopCheck");if(!check)return;
            assert(check->isChecked()==expected && check->text()=="FireRed co-op (experimental)");
            assert(dialog->findChild<QLabel*>("fireRedCompatibility")->text().contains("not compatible"));
            check->setChecked(selected);assert(dialog->findChild<QComboBox*>("fireRedAudioCombo")->isEnabled()==selected);
            visited=true;if(accept)dialog->accept();else dialog->reject();
        });timer.start();launcher.findChild<QPushButton*>("settingsAction")->click();timer.stop();assert(visited);
    };
    {Launcher launcher(data.path());launcher.show();choose(launcher,false,true,false);assert(preferences(data.path())==original);
        choose(launcher,false,true,true);assert(preferences(data.path())["fireRedCoop"].toBool() && preferences(data.path())["futureSetting"].toInt()==42);}
    {Launcher launcher(data.path());launcher.show();choose(launcher,true,false,true);assert(!preferences(data.path())["fireRedCoop"].toBool());}
    qputenv("COUCHSPLIT_FIRERED_EXPERIMENT","1");
    {Launcher launcher(data.path());launcher.show();choose(launcher,false,true,false); // Saved false overrides old environment opt-in.
        const auto saved=preferences(data.path());assert(QFile::remove(data.path()+"/settings.json"));assert(QDir().mkdir(data.path()+"/settings.json"));
        choose(launcher,false,true,true); // Failed persistence must not enable the mode in memory.
        assert(QDir().rmdir(data.path()+"/settings.json"));atomicWrite(data.path()+"/settings.json",QJsonDocument(saved).toJson());
        choose(launcher,false,true,false);assert(preferences(data.path())==saved);}
    if(present)qputenv("COUCHSPLIT_FIRERED_EXPERIMENT",previous);else qunsetenv("COUCHSPLIT_FIRERED_EXPERIMENT");
}
static void fireRedNoticeFits() {
    for(auto language:{"de","en"})for(int width:{360,616,1256}) {
        i18n::setLanguage(language);QWidget window;theme::apply(&window,false,width<760);
        auto* notice=new FireRedNotice(&window);int choices=0;
        notice->continueIndividually=[&](uint32_t value){assert(value==259);++choices;return true;};
        notice->showState(259,i18n::text("Feuerrot-Koop · Kampf pausiert: zu viele EP-Teiler"));
        notice->resize(width,notice->heightForWidth(width));window.resize(notice->size());window.show();spin(30);
        for(auto* label:notice->findChildren<QLabel*>()) {
            assert(label->isVisible() && label->height()>=label->heightForWidth(label->width()));
            assert(notice->rect().contains(label->geometry()));
        }
        auto* action=notice->findChild<QPushButton*>("fireRedContinueIndividual");assert(action&&action->isVisible());
        assert(action->fontMetrics().horizontalAdvance(action->text())+28<=action->width());
        capture(window,QString("firered-pause-%1-%2").arg(language).arg(width));
        action->click();action->click();assert(choices==1 && !action->isEnabled());
        notice->showState(259,"same pause");assert(!action->isEnabled());
        notice->showState(0,"field");assert(!action->isVisible());
        notice->showState(515,"next pause");assert(action->isVisible() && action->isEnabled());
    }
}
// Optional private integration: no ROM or real save is copied into the repo.
// The fixture only simulates a paired worker's exit, it does not emulate a battle.
static void fireRedRealRomUi(const QString& fixture) {
    const auto rom=qEnvironmentVariable("COOP_VERIFY_FIRERED_ROM");if(rom.isEmpty())return;
    assert(hashFile(rom)==QLatin1String(GermanFireRedSha256));
    const bool present=qEnvironmentVariableIsSet("COUCHSPLIT_FIRERED_EXPERIMENT");
    const auto previous=qgetenv("COUCHSPLIT_FIRERED_EXPERIMENT");
    qunsetenv("COUCHSPLIT_FIRERED_EXPERIMENT");
    for(const auto& language:{QString("de"),QString("en")}) {
        QTemporaryDir data;SaveStore store(data.path());store.acquire();
        const auto game=validateRom(rom);
        const std::array<QByteArray,2> original{{QByteArray(131072,'a'),QByteArray(131072,'b')}};
        const std::array<QByteArray,2> later{{QByteArray(131072,'c'),QByteArray(131072,'d')}};
        std::array<QString,2> saves;
        for(int p=0;p<2;p++) {saves[p]=store.profile(p,game)+"/saves/game.sav";assert(QDir().mkpath(QFileInfo(saves[p]).path()));atomicWrite(saves[p],original[p]);}
        const auto backup=store.backupPair(game);assert(!backup.isEmpty());
        for(int p=0;p<2;p++)atomicWrite(saves[p],later[p]);store.release();
        atomicWrite(data.path()+"/settings.json",QJsonDocument(QJsonObject{{"language",language},{"rom",rom},{"gbaEmulator",fixture},{"fireRedCoop",true}}).toJson());
        Launcher launcher(data.path());launcher.resize(640,360);launcher.show();spin(40);
        assert(launcher.findChild<QPushButton*>("startGame")->text()==i18n::text("Feuerrot-Testmodus starten"));
        int stage=0;QTimer drive;drive.setInterval(10);
        QObject::connect(&drive,&QTimer::timeout,[&]{
            auto* modal=QApplication::activeModalWidget();if(!modal)return;
            if(auto* file=qobject_cast<QFileDialog*>(modal)) {
                if(stage!=1)return;stage=2;file->setDirectory(backup);file->selectFile(backup);
                QMetaObject::invokeMethod(file,"accept",Qt::QueuedConnection);return;
            }
            if(auto* box=qobject_cast<QMessageBox*>(modal)) {
                if(stage==2 && box->standardButtons().testFlag(QMessageBox::Yes)) {
                    stage=3;QTimer::singleShot(0,box->button(QMessageBox::Yes),&QPushButton::click);
                } else if(stage==3 && box->standardButtons().testFlag(QMessageBox::Ok)) {
                    stage=4;QTimer::singleShot(0,box->button(QMessageBox::Ok),&QPushButton::click);
                }
                return;
            }
            auto* dialog=qobject_cast<QDialog*>(modal);if(!dialog)return;
            auto* restore=dialog->findChild<QPushButton*>("restorePairAction");if(!restore)return;
            if(stage==0) {stage=1;capture(*dialog,"paired-restore-"+language);QTimer::singleShot(0,restore,&QPushButton::click);}
            else if(stage==4) {stage=5;QTimer::singleShot(0,dialog,&QDialog::accept);}
        });drive.start();
        QTimer watchdog;watchdog.setSingleShot(true);
        QObject::connect(&watchdog,&QTimer::timeout,[&]{if(auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget()))dialog->reject();});watchdog.start(10000);
        for(auto* button:launcher.findChildren<QPushButton*>())if(button->text()==i18n::text("Spielstände"))button->click();
        drive.stop();watchdog.stop();assert(stage==5);
        for(int p=0;p<2;p++){QFile f(saves[p]);assert(f.open(QIODevice::ReadOnly));assert(f.readAll()==original[p]);}
        // Simulate the dedicated capacity exit through production widgets.
        int a=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
        int b=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);assert(a>=0&&b>=0);
        auto* ja=SDL_JoystickOpen(a);auto* jb=SDL_JoystickOpen(b);assert(ja&&jb);
        qputenv("COOP_FIXTURE_EXIT_CODE","71");launcher.findChild<QPushButton*>("startGame")->click();spin(1000);
        auto* status=launcher.findChild<QLabel*>("sessionStatus");assert(status && status->property("tone").toString()=="error");
        assert(status->text().contains(language=="de"?"EP-Teiler":"Exp. Shares"));
        auto* menuScroll=launcher.findChild<QScrollArea*>("menuScroll");assert(menuScroll);
        const QRect statusRect(status->mapTo(menuScroll->viewport(),QPoint()),status->size());
        if(!menuScroll->viewport()->rect().contains(statusRect)) {
            fprintf(stderr,"Error status outside viewport: status=(%d,%d %dx%d), viewport=%dx%d, scroll=%d/%d\n",statusRect.x(),statusRect.y(),statusRect.width(),statusRect.height(),menuScroll->viewport()->width(),menuScroll->viewport()->height(),menuScroll->verticalScrollBar()->value(),menuScroll->verticalScrollBar()->maximum());
            capture(launcher,"paired-error-failed-"+language);
        }
        assert(menuScroll->viewport()->rect().contains(statusRect));
        assert(!QFileInfo::exists(data.path()+"/session.lock"));qunsetenv("COOP_FIXTURE_EXIT_CODE");
        capture(launcher,"paired-error-"+language);
        const auto other=language=="de"?QString("en"):QString("de");
        configureLanguage(launcher,other);
        assert(status->property("tone").toString()=="error" && status->text().contains(other=="de"?"EP-Teiler":"Exp. Shares"));
        SDL_JoystickClose(ja);SDL_JoystickClose(jb);SDL_JoystickDetachVirtual(b);SDL_JoystickDetachVirtual(a);
    }
    if(present)qputenv("COUCHSPLIT_FIRERED_EXPERIMENT",previous);else qunsetenv("COUCHSPLIT_FIRERED_EXPERIMENT");
    assert(hashFile(rom)==QLatin1String(GermanFireRedSha256));i18n::setLanguage("de");
}
// Exercise the actual Session -> child IPC -> PlayView acknowledgement path.
// This child is a named TEST ONLY fixture, not a gameplay test.
static void fireRedRecoveryUi(const QString& fixture) {
    const auto rom=qEnvironmentVariable("COOP_VERIFY_FIRERED_ROM");if(rom.isEmpty())return;
    for(auto language:{"de","en"}) {
        i18n::setLanguage(language);QTemporaryDir data;SaveStore saves(data.path());Session session(saves);Controllers controls;
        Settings settings;settings.rom=rom;settings.gbaEmulator=fixture;settings.fireRedCoop=true;
        int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
        int other=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);assert(index>=0 && other>=0);
        auto* pad=SDL_JoystickOpen(index);auto* partner=SDL_JoystickOpen(other);assert(pad&&partner);
        controls.assign(SDL_JoystickInstanceID(pad),SDL_JoystickInstanceID(partner));
        QString failure;session.error=[&](QString text){failure=text;};
        qputenv("COOP_FIXTURE_OLD_RECOVERY","1");bool oldRejected=false;
        try {session.start(settings);}catch(const std::exception&){oldRejected=true;}
        qunsetenv("COOP_FIXTURE_OLD_RECOVERY");assert(oldRejected && !session.running() && !saves.locked());
        session.start(settings);
        PlayView view(session,controls,settings);view.resize(640,360);view.show();view.reset();spin(60);
        auto* action=view.findChild<QPushButton*>("fireRedContinueIndividual");assert(action);
        auto* notice=view.findChild<QFrame*>("fireRedNotice");assert(notice);
        session.shared(0)->bridgeActive.store(2);session.shared(0)->role.store(2);spin(40);
        assert(view.findChild<QLabel*>("fireRedPlayStatus")->text().contains(language==QString("de")?"Warte auf Spieler 2":"Waiting for Player 2"));
        for(uint32_t pause:{259u,516u}) {
            SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,1);spin(40);
            session.shared(0)->bridgeActive.store(pause);session.shared(1)->bridgeActive.store(pause);spin(80);
            assert(session.fireRedStatusCode()==pause && session.running() && saves.locked());
            assert(session.shared(0)->keys.load()==0xFFF && action->isVisible());
            const auto frame=session.shared(0)->frame.load();spin(100);assert(session.shared(0)->frame.load()>frame);
            assert(view.rect().contains(notice->geometry()));
            capture(view,QString("firered-live-pause-%1-%2").arg(language).arg(pause));
            assert(!session.continueFireRedIndividually(pause+256));
            view.hide();spin(30);view.show();spin(40);assert(session.fireRedStatusCode()==pause); // Held A cannot approve on restore.
            SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,0);spin(40);
            if(pause==259)SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,1);
            else {
                QKeyEvent down(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier),up(QEvent::KeyRelease,Qt::Key_Space,Qt::NoModifier);
                QApplication::sendEvent(action,&down);QApplication::sendEvent(action,&up);
            }
            spin(100);assert(session.fireRedStatusCode()==5 && session.running() && saves.locked());
            assert(!session.continueFireRedIndividually(pause));
            SDL_JoystickSetVirtualButton(pad,SDL_CONTROLLER_BUTTON_A,0);spin(40);
        }
        assert(failure.isEmpty());session.stop();assert(!saves.locked());
        SDL_JoystickClose(pad);SDL_JoystickClose(partner);SDL_JoystickDetachVirtual(other);SDL_JoystickDetachVirtual(index);
    }
}
static void menuFits() {
    QTemporaryDir data;
    atomicWrite(data.path()+"/settings.json",QJsonDocument(QJsonObject{{"rom",QString(250,'r')+".nds"}}).toJson());
    Launcher launcher(data.path());launcher.show();launcher.activateWindow();spin(50);
    assert(theme::pixelFamily().contains("Silkscreen"));
    auto* scroll=launcher.findChild<QScrollArea*>("menuScroll");assert(scroll);
    for(auto size:{QSize(1280,720),QSize(1366,768),QSize(640,360),QSize(1040,720),QSize(1920,1080),QSize(3840,2160)}) {
        launcher.resize(size);spin(50);assert(launcher.size()==size);
        capture(launcher,"menu-"+i18n::language()+"-"+QString::number(size.width())+"x"+QString::number(size.height()));
        assert(scroll->horizontalScrollBar()->maximum()==0);
        for(auto* button:launcher.findChildren<QPushButton*>()) {
            if(!scroll->isAncestorOf(button))continue;
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
    const QString expected=i18n::text("Touch: rechter Stick + Klick · Ansicht: Select + R · Ende: Start + Select (2 s)");
    for(auto size:{QSize(640,360),QSize(1280,720),QSize(1366,768),QSize(1920,1080),QSize(3840,2160)})for(bool horizontal:{false,true}) {
        settings.horizontal=horizontal;view.resize(size);view.update();spin(20);
        if(size.width()==1280)capture(view,"play-help-"+i18n::language()+"-"+(horizontal?"stacked":"side-by-side"));
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
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    QTemporaryDir mouseData,controllerData,sessionData;
    languagePreferences();
    for(auto language:{"en","de"}) {i18n::setLanguage(language);menuFits();playHelpFits();}
    gbaSelection();fireRedAudioPreferences();fireRedCoopPreferences();fireRedNoticeFits();fireRedRealRomUi(QFileInfo(argv[1]).absoluteFilePath());fireRedRecoveryUi(QFileInfo(argv[1]).absoluteFilePath());i18n::setLanguage("de");backgroundMenu();assignedMenu();
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
        const auto runBefore=QDir(sessionData.path()).entryList({"run-*"},QDir::Dirs|QDir::NoDotAndDotDot);
        configureLanguage(launcher,"en");
        assert(QDir(sessionData.path()).entryList({"run-*"},QDir::Dirs|QDir::NoDotAndDotDot)==runBefore);
        assert(QFileInfo::exists(sessionData.path()+"/session.lock"));
        launcher.findChild<QPushButton*>("startGame")->click();spin(60);assert(pages->currentIndex()==1);
        assert(launcher.findChild<QLabel*>("playHelp1")->text().contains("right stick"));
        QCoreApplication::sendEvent(pages->currentWidget(),&escape);assert(pages->currentIndex()==0);
        configureLanguage(launcher,"de");
        launcher.findChild<QPushButton*>("exitLauncher")->click();assert(!launcher.isVisible());
        SaveStore probe(sessionData.path());probe.acquire();probe.release();
        auto sessions=QDir(sessionData.path()+"/logs").entryList(QDir::Dirs|QDir::NoDotAndDotDot);assert(sessions.size()==1);
        for(int p=1;p<=2;p++) {
            QFile log(sessionData.path()+"/logs/"+sessions.first()+"/player"+QString::number(p)+".log");
            assert(log.open(QIODevice::ReadOnly));assert(log.readAll().contains("TEST_ONLY: graceful exit"));
        }
        SDL_JoystickClose(ja);SDL_JoystickClose(jb);SDL_JoystickDetachVirtual(b);SDL_JoystickDetachVirtual(a);
    }
    std::cout<<"PASS: first choice, cancel, invalid/malformed settings, atomic failure, legacy preservation, live language switch without worker restart; DE/EN menu and full wrapped play help at 360p/720p/768p/1080p/4K in both splits; assigned player-one menu input, no unassigned/player-two/replacement takeover after disconnect, explicit reassignment; focus scrolling, blocked hidden/minimized menu and held input on restore, blocked live ROM change, neutral minimized input and reset exit chord; fixture workers release saves\n";
    return 0;
}
