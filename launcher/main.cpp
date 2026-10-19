// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <csignal>
static volatile sig_atomic_t quitRequested=0;
static void requestQuit(int) {quitRequested=1;}
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QFrame>
#include <QStyle>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QImage>
#include <QtMath>
#include "Session.h"
#include "Controllers.h"
#include "Layout.h"
#include "Theme.h"
using namespace coop;

class PlayView : public QWidget {
    Session& session; Controllers& controllers;
    std::array<QImage,2> tops,bottoms;
    std::array<bool,2> bottomOnly{{false,false}};
    std::array<uint32_t,2> lastFrames{{0,0}};
    QTimer timer; QElapsedTimer delta;
    std::array<QLabel*,2> helpLabels;
public:
    Settings& settings;
    std::function<void()> leave, endSession;
    PlayView(Session& s,Controllers& c,Settings& cfg):session(s),controllers(c),settings(cfg) {
        for(int p=0;p<2;p++) {
            auto* help=new QLabel("Touch: rechter Stick + Klick · Ansicht: Select + R · Ende: Start + Select (2 s)",this);
            help->setObjectName(QString("playHelp%1").arg(p+1));
            help->setWordWrap(true);help->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
            help->setStyleSheet("background:transparent; color:#c0d2d7; font-size:14pt;");
            helpLabels[p]=help;
        }
        setFocusPolicy(Qt::StrongFocus); timer.setInterval(16); delta.start();
        connect(&timer,&QTimer::timeout,this,[this]{
            if(!session.running()) return;
            if(!isVisible() || window()->isMinimized()) { neutralInput(); return; }
            float dt=qMin(qint64(50),delta.restart())/1000.0f;
            controllers.update();
            for(int p=0;p<2;p++) {
                auto input=controllers.read(p,dt);
                if(input.toggle) bottomOnly[p]=!bottomOnly[p];
                if(auto* shared=session.shared(p)) {
                    shared->keys.store(input.keys); shared->touch.store(input.touch);
                    auto frame=shared->frame.load();
                    if(frame!=lastFrames[p] && session.copyFrame(p,tops[p],bottoms[p])) lastFrames[p]=frame;
                }
                if(input.exit) { if(endSession) endSession(); return; }
            }
            update();
        }); timer.start();
    }
    void neutralInput() {
        for(int p=0;p<2;p++) if(auto* s=session.shared(p)) {s->keys.store(0xFFF);s->touch.store(0);}
        controllers.resetGestures(); delta.restart();
    }
    void reset() { lastFrames={{0,0}}; tops={}; bottoms={}; bottomOnly={{false,false}}; delta.restart(); }
protected:
    void keyPressEvent(QKeyEvent* e) override { if(e->key()==Qt::Key_Escape) { if(leave) leave(); } else QWidget::keyPressEvent(e); }
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this); painter.fillRect(rect(),QColor("#0e2330"));
        painter.setRenderHint(QPainter::SmoothPixmapTransform,false);
        for(int p=0;p<2;p++) {
            auto area=playerArea(rect(),p,settings.horizontal).adjusted(12,12,-12,-12);
            QRectF label(area.x(),area.y(),area.width(),60);
            painter.setPen(QColor(p==0?"#f59c89":"#91dce0"));
            QFont heading(theme::pixelFamily());heading.setPixelSize(18);painter.setFont(heading);
            painter.drawText(label.adjusted(0,0,0,-30),Qt::AlignCenter,QString("Spieler %1").arg(p+1));
            QFont detail=QApplication::font();detail.setPixelSize(14);painter.setFont(detail);
            painter.setPen(QColor(controllers.connected(p)?"#c0d2d7":"#ffb0a3"));
            painter.drawText(label.adjusted(0,30,0,0),Qt::AlignCenter,controllers.connected(p)?"Controller verbunden":"Controller getrennt");
            auto* help=helpLabels[p];
            const int helpWidth=qMax(1,qFloor(area.width()));
            const int helpHeight=qMax(help->fontMetrics().height(),help->heightForWidth(helpWidth));
            help->setGeometry(qCeil(area.left()),qFloor(area.bottom())-helpHeight,helpWidth,helpHeight);
            area.adjust(0,64,0,-helpHeight-8);
            auto dest=screens(area,settings.layout,bottomOnly[p]);
            if(!tops[p].isNull() && !dest[0].isEmpty()) painter.drawImage(dest[0],tops[p]);
            if(!bottoms[p].isNull() && !dest[1].isEmpty()) painter.drawImage(dest[1],bottoms[p]);
            if(!dest[1].isEmpty()) {
                auto cursor=controllers.cursor(p); QPointF point(dest[1].x()+cursor.x()/256*dest[1].width(),dest[1].y()+cursor.y()/192*dest[1].height());
                painter.setPen(QPen(QColor(p==0?"#f59c89":"#91dce0"),2)); painter.drawEllipse(point,8,8);
            }
        }
        painter.setPen(QPen(QColor("#3b5966"),2));
        if(settings.horizontal) painter.drawLine(0,height()/2,width(),height()/2); else painter.drawLine(width()/2,0,width()/2,height());
    }
};
class Launcher : public QWidget {
    QString root;
    SaveStore saves;
    Settings settings;
    Controllers controllers;
    Session session;
    QStackedWidget* pages;
    PlayView* play=nullptr;
    QLabel *statusLabel,*romLabel,*padLabel;
    QTimer uiTimer;
        bool menuHeld=false;
    QWidget* menuContent=nullptr;
    QBoxLayout* menuBody=nullptr;
    QGridLayout* actionGrid=nullptr;
    QVBoxLayout* contentLayout=nullptr;
    QVBoxLayout* gameLayout=nullptr;
    QList<QPushButton*> menuActions;
    void updateAppearance() {
        const bool large=width()>=2800 && height()>=1600;
        const bool compact=width()<760;
        const bool stacked=width()<1040;
        if(!menuContent)return;
        theme::apply(this,large,compact);
        menuContent->setMaximumWidth(large?2600:1440);
        menuBody->setDirection(stacked?QBoxLayout::TopToBottom:QBoxLayout::LeftToRight);
        const int spacing=large?32:20;
        menuBody->setSpacing(spacing);contentLayout->setSpacing(spacing);
        gameLayout->setContentsMargins(spacing,spacing,spacing,spacing);
        actionGrid->setHorizontalSpacing(large?20:12);actionGrid->setVerticalSpacing(large?16:10);
        for(int i=0;i<menuActions.size();i++) {
            auto* button=menuActions[i];actionGrid->removeWidget(button);
            actionGrid->addWidget(button,compact?i:i/2,compact?0:i%2);
        }
        for(auto* button:findChildren<QPushButton*>())button->setIconSize(QSize(large?40:24,large?40:24));
    }
    QJsonObject stored;
    void report(QString msg) { QMessageBox::warning(this,"Pokémon Platinum Local Co-op",msg); }
    void saveSettings() {
        if(settings.emulator==bundledEmulator()) stored.remove("emulator");
        else stored["emulator"]=settings.emulator;
        stored["rom"]=settings.rom;
        stored["bios9"]=settings.bios9; stored["bios7"]=settings.bios7; stored["firmware"]=settings.firmware;
        stored["layout"]=settings.layout; stored["horizontal"]=settings.horizontal;
        stored["patchedConfirmed"]=settings.patchedConfirmed;
        QJsonArray mapping; for(auto n:controllers.buttons) mapping.append(n); stored["buttons"]=mapping;
        atomicWrite(root+"/settings.json",QJsonDocument(stored).toJson());
    }
    void guarded(std::function<void()> fn) { try {fn();} catch(const std::exception& e) {report(QString::fromUtf8(e.what()));} }
    void assignPads() {
        auto pads=controllers.list();
        if(pads.size()<2) fail("Zwei SDL-kompatible Controller anschließen. In Steam die Controller-Reihenfolge und Steam Input prüfen.");
        QDialog dialog(this); dialog.setWindowTitle("Controller zuordnen"); QFormLayout form(&dialog);
        QComboBox a,b; for(auto pad:pads) { auto name=pad.name+" · Instanz "+QString::number(pad.id); a.addItem(name,pad.id); b.addItem(name,pad.id); }
        b.setCurrentIndex(1);
        for(int p=0;p<2;p++) {
            auto pref=stored.value(p?"pad2":"pad1").toObject(); int matches=0, match=-1;
            for(int i=0;i<int(pads.size());i++) if(pads[i].guid==pref["guid"].toString() && (!pref["serial"].toString().isEmpty()?pads[i].serial==pref["serial"].toString():true)) {matches++;match=i;}
            if(matches==1) (p?b:a).setCurrentIndex(match);
        }
        form.addRow("Spieler 1",&a);form.addRow("Spieler 2",&b);
        QLabel help("Zuordnung über SDL-Instanz-IDs. Nach dem erneuten Verbinden hier neu zuordnen. Identische Controller ohne Seriennummer benötigen eine ausdrückliche Auswahl."); help.setWordWrap(true); form.addRow(&help);
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);theme::labelButtons(ok);form.addRow(&ok);
        connect(&ok,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&ok,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()!=QDialog::Accepted) return;
        controllers.assign(a.currentData().toInt(),b.currentData().toInt());
        for(int p=0;p<2;p++) for(auto pad:pads) if(pad.id==controllers.id(p)) stored[p?"pad2":"pad1"]=QJsonObject{{"guid",pad.guid},{"serial",pad.serial},{"name",pad.name}};
        padLabel->setText("Spieler 1: "+pads[a.currentIndex()].name+"\nSpieler 2: "+pads[b.currentIndex()].name); saveSettings();
    }
    void configure() {
        QDialog dialog(this);dialog.setWindowTitle("Einstellungen");QFormLayout form(&dialog);
        QLineEdit emu(settings.emulator), b9(settings.bios9), b7(settings.bios7),fw(settings.firmware);
        auto row=[&](QString title,QLineEdit& line){ auto* box=new QWidget(&dialog);auto* layout=new QHBoxLayout(box); layout->addWidget(&line); auto* browse=new QPushButton("Auswählen",box);layout->addWidget(browse);connect(browse,&QPushButton::clicked,&dialog,[&,title]{auto file=QFileDialog::getOpenFileName(&dialog,title);if(!file.isEmpty())line.setText(file);});form.addRow(title,box); };
        row("Emulator",emu);row("ARM9 BIOS (optional)",b9);row("ARM7 BIOS (optional)",b7);row("DS-Firmware (optional)",fw);
        QLabel help("Ohne externe Dateien nutzt melonDS seine freie BIOS-Implementierung und erzeugte Firmware (Direktstart). Externe Dateien nur vollständig und als eigene rechtmäßig bereitgestellte Dumps auswählen.");help.setWordWrap(true);form.addRow(&help);
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);theme::labelButtons(ok);form.addRow(&ok);
        connect(&ok,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&ok,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()==QDialog::Accepted) {settings.emulator=emu.text();settings.bios9=b9.text();settings.bios7=b7.text();settings.firmware=fw.text();saveSettings();}
        // These line edits have automatic storage; remove them before parent teardown.
        for(auto* line:{&emu,&b9,&b7,&fw}) line->setParent(nullptr);
    }
    void remap() {
        QDialog dialog(this);dialog.setWindowTitle("Tastenbelegung");QFormLayout form(&dialog);
        const QStringList keys{"DS A","DS B","Select","Start","Rechts","Links","Oben","Unten","R","L","X","Y"};
        std::array<std::unique_ptr<QComboBox>,12> combos;
        for(int i=0;i<12;i++) {combos[i]=std::make_unique<QComboBox>(); combos[i]->addItem("Deaktiviert",-1);for(int b=0;b<SDL_CONTROLLER_BUTTON_MAX;b++) combos[i]->addItem(SDL_GameControllerGetStringForButton(SDL_GameControllerButton(b)),b);combos[i]->setCurrentIndex(combos[i]->findData(controllers.buttons[i]));form.addRow(keys[i],combos[i].get());}
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);theme::labelButtons(ok);form.addRow(&ok);connect(&ok,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&ok,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()==QDialog::Accepted) {for(int i=0;i<12;i++) controllers.buttons[i]=combos[i]->currentData().toInt();saveSettings();}
    }
    void layoutDialog() {
        QDialog dialog(this);dialog.setWindowTitle("Bildschirmaufteilung");QFormLayout form(&dialog);
        QComboBox split,ds;split.addItems({"Links / Rechts","Oben / Unten"});split.setCurrentIndex(settings.horizontal?1:0);
        ds.addItems({"Oberer Bildschirm groß + Touchscreen klein","DS-Bildschirme nebeneinander","Ein Bildschirm · Select + R wechseln"});ds.setCurrentIndex(settings.layout);
        form.addRow("Spieler",&split);form.addRow("DS-Ansicht",&ds);
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);theme::labelButtons(ok);form.addRow(&ok);connect(&ok,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&ok,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()==QDialog::Accepted) {settings.horizontal=split.currentIndex()==1;settings.layout=ds.currentIndex();saveSettings();}
    }
    void restoreDialog() {
        if(session.running()) fail("Vor einer Wiederherstellung die Sitzung beenden.");
        auto game=validateRom(settings.rom);
        QDialog dialog(this);dialog.setWindowTitle("Spielstände");QFormLayout form(&dialog);
        QComboBox player;player.addItems({"Spieler 1","Spieler 2"});form.addRow("Profil",&player);
        QPushButton folder("Profilordner öffnen"),restore("Backup wiederherstellen");form.addRow(&folder);form.addRow(&restore);
        connect(&folder,&QPushButton::clicked,&dialog,[&]{QDesktopServices::openUrl(QUrl::fromLocalFile(saves.profile(player.currentIndex(),game)));});
        connect(&restore,&QPushButton::clicked,&dialog,[&]{guarded([&]{
            auto path=QFileDialog::getExistingDirectory(&dialog,"Backupordner auswählen",saves.profile(player.currentIndex(),game)+"/backups");if(path.isEmpty())return;
            if(QMessageBox::question(&dialog,"Spielstand wiederherstellen","Der aktuelle Stand wird zuerst gesichert. Gewähltes Backup wiederherstellen?")!=QMessageBox::Yes)return;
            saves.acquire();try{saves.restore(player.currentIndex(),game,path,settings.rom);saves.release();}catch(...){saves.release();throw;}
            QMessageBox::information(&dialog,"Spielstände","Backup erfolgreich wiederhergestellt.");
        });});
        QDialogButtonBox close(QDialogButtonBox::Close);theme::labelButtons(close);form.addRow(&close);connect(&close,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();
    }
    void showLogs() {
        QDialog dialog(this);dialog.setWindowTitle("Logs");QVBoxLayout layout(&dialog);QTextEdit text; text.setReadOnly(true);
        auto path=session.logDir;if(path.isEmpty()) {auto dirs=QDir(root+"/logs").entryList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);if(!dirs.isEmpty())path=root+"/logs/"+dirs.last();}
        QString content;for(int p=1;p<=2;p++){QFile f(path+"/player"+QString::number(p)+".log");if(f.open(QIODevice::ReadOnly)){f.seek(qMax(qint64(0),f.size()-262144));content+="Spieler "+QString::number(p)+"\n"+QString::fromUtf8(f.readAll())+"\n";}}
        text.setPlainText(content.isEmpty()?"Noch keine Sitzungslogs vorhanden.":content);layout.addWidget(&text);dialog.resize(1100,700);dialog.exec();
    }
    bool autoAssign() {
        auto pads=controllers.list();
        if(pads.size()!=2) return false;
        int chosen[2]={0,1};
        bool saved=stored.contains("pad1")||stored.contains("pad2");
        if(saved) {
            for(int p=0;p<2;p++) {
                auto pref=stored.value(p?"pad2":"pad1").toObject();int matches=0;
                for(int i=0;i<2;i++) if(pads[i].guid==pref["guid"].toString() && (pref["serial"].toString().isEmpty()||pads[i].serial==pref["serial"].toString())) {chosen[p]=i;matches++;}
                if(matches!=1)return false;
            }
            if(chosen[0]==chosen[1])return false;
        }
        controllers.assign(pads[chosen[0]].id,pads[chosen[1]].id);
        for(int p=0;p<2;p++) {auto pad=pads[chosen[p]];stored[p?"pad2":"pad1"]=QJsonObject{{"guid",pad.guid},{"serial",pad.serial},{"name",pad.name}};}
        padLabel->setText("Spieler 1: "+pads[chosen[0]].name+"\nSpieler 2: "+pads[chosen[1]].name);
        saveSettings();return true;
    }
    void startGame() {
        if(session.running()) {pages->setCurrentWidget(play);play->setFocus();return;}
        if((!controllers.connected(0)||!controllers.connected(1)) && !autoAssign()) assignPads();
        if(!controllers.connected(0)||!controllers.connected(1))return;
        if(settings.rom.isEmpty()) chooseRom();
        if(settings.rom.isEmpty())return;
        saveSettings();session.start(settings);play->reset();pages->setCurrentWidget(play);play->setFocus();
    }
    void chooseRom() {
        if(session.running()) fail("Vor einem ROM-Wechsel die laufende Sitzung beenden. Im Spiel Start + Select zwei Sekunden gedrückt halten.");
        auto rom=QFileDialog::getOpenFileName(this,"Project-PM-ROM hinzufügen",settings.rom,"Nintendo DS oder ROM-ZIP (*.nds *.srl *.zip)");if(rom.isEmpty())return;
        auto importer=QCoreApplication::applicationDirPath()+"/../../scripts/import-rom.py";
        if(QFileInfo::exists(importer)) {
            QString python="python3";
#ifdef _WIN32
            python=QCoreApplication::applicationDirPath()+"/python/python.exe";
#endif
            QProcess process;process.start(python,{"-X","utf8",importer,"--rom",rom,"--into",root+"/roms"});
            QApplication::setOverrideCursor(Qt::WaitCursor);
            bool finished=process.waitForFinished(120000);
            QApplication::restoreOverrideCursor();
            if(!finished) {process.kill();process.waitForFinished();fail("ROM-Import dauert zu lange. Speicherplatz und Datei prüfen.");}
            if(process.exitCode()!=0) fail(QString::fromUtf8(process.readAllStandardError()).trimmed());
            rom=QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        } else if(QFileInfo(rom).suffix().compare("zip",Qt::CaseInsensitive)==0) fail("ROM-ZIP zuerst entpacken; Importskript fehlt in dieser Installation.");
        auto sha=validateRom(rom);requireMultiplayerPatch(sha);auto known=knownPreparedRom(sha);
        bool confirmed=!known.isEmpty() || QMessageBox::question(this,"Project PM ROM","Ist diese Platin-ROM mit dem passenden Project-PM-Multiplayer-Patch vorbereitet? Der Launcher kann Patchversion und Welt-Synchronisation nicht anhand des ROM-Headers bestätigen.") == QMessageBox::Yes;
        if(!confirmed)return;
        settings.rom=rom;settings.patchedConfirmed=true;romLabel->setText(known.isEmpty()?QFileInfo(rom).fileName():known);saveSettings();
    }
public:
    void firstRun() {if(settings.rom.isEmpty())guarded([this]{chooseRom();});}
    Launcher(QString data):root(std::move(data)),saves(root),session(saves) {
        QFile f(root+"/settings.json");if(f.open(QIODevice::ReadOnly))stored=QJsonDocument::fromJson(f.readAll()).object();
        settings.emulator=stored.value("emulator").toString(bundledEmulator());settings.rom=stored["rom"].toString();
        if(!QFileInfo(settings.emulator).isExecutable()) settings.emulator=bundledEmulator();
        settings.bios9=stored["bios9"].toString();settings.bios7=stored["bios7"].toString();settings.firmware=stored["firmware"].toString();
        settings.layout=qBound(0,stored["layout"].toInt(),2);settings.horizontal=stored["horizontal"].toBool();settings.patchedConfirmed=stored["patchedConfirmed"].toBool();
        auto mapping=stored["buttons"].toArray();if(mapping.size()==12)for(int i=0;i<12;i++){int n=mapping[i].toInt(-1);controllers.buttons[i]=(n>=-1&&n<SDL_CONTROLLER_BUTTON_MAX)?n:-1;}
        setObjectName("launcherWindow");setAttribute(Qt::WA_StyledBackground,true);
        auto* outer=new QVBoxLayout(this);outer->setContentsMargins(0,0,0,0);
        pages=new QStackedWidget(this);outer->addWidget(pages);
        auto* menu=new QWidget;menu->setObjectName("menuCanvas");
        auto* canvas=new QVBoxLayout(menu);canvas->setContentsMargins(32,24,32,24);canvas->addStretch();
        auto* centered=new QHBoxLayout;centered->addStretch(1);
        menuContent=new QWidget;menuContent->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
        contentLayout=new QVBoxLayout(menuContent);contentLayout->setContentsMargins(0,0,0,0);
        centered->addWidget(menuContent,100);centered->addStretch(1);canvas->addLayout(centered);canvas->addStretch();
        auto* header=new QHBoxLayout;
        auto* title=new QLabel("Pokémon Platinum\nLocal Co-op");title->setObjectName("title");
        title->setWordWrap(true);title->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        header->addWidget(title,1);
        auto* alpha=new QLabel("ALPHA");alpha->setObjectName("alphaBadge");header->addWidget(alpha,0,Qt::AlignTop);
        contentLayout->addLayout(header);
        auto* subtitle=new QLabel("Zwei Spieler. Ein gemeinsames Abenteuer.");subtitle->setObjectName("subtitle");subtitle->setWordWrap(true);contentLayout->addWidget(subtitle);
        menuBody=new QBoxLayout(QBoxLayout::LeftToRight);contentLayout->addLayout(menuBody);
        auto* gameColumn=new QWidget;auto* gameColumnLayout=new QVBoxLayout(gameColumn);gameColumnLayout->setContentsMargins(0,0,0,0);gameColumnLayout->setSpacing(14);
        auto* gamePanel=new QFrame;gamePanel->setObjectName("gamePanel");gameLayout=new QVBoxLayout(gamePanel);gameLayout->setSpacing(14);
        auto section=[&](QString text,QVBoxLayout* into) {auto* label=new QLabel(text);label->setProperty("role","sectionTitle");into->addWidget(label);};
        section("Dein Spiel",gameLayout);
        romLabel=new QLabel(settings.rom.isEmpty()?"Noch keine Project-PM-ROM ausgewählt":QFileInfo(settings.rom).fileName());romLabel->setObjectName("romLabel");romLabel->setWordWrap(true);romLabel->setTextFormat(Qt::PlainText);gameLayout->addWidget(romLabel);
        gameLayout->addSpacing(12);section("Controller",gameLayout);
        padLabel=new QLabel("Noch nicht zugeordnet.\nVerbinde zwei Controller und wähle ihre Spieler.");padLabel->setObjectName("padLabel");padLabel->setWordWrap(true);padLabel->setTextFormat(Qt::PlainText);gameLayout->addWidget(padLabel);
        for(auto* label:{romLabel,padLabel}) {auto policy=label->sizePolicy();policy.setHorizontalPolicy(QSizePolicy::Ignored);label->setSizePolicy(policy);}
        gameLayout->addStretch();gameColumnLayout->addWidget(gamePanel,1);
        auto button=[&](QString name,theme::Glyph glyph,std::function<void()> fn){auto* b=new QPushButton(name);b->setIcon(theme::icon(glyph));b->setMinimumHeight(56);b->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);connect(b,&QPushButton::clicked,this,[this,fn]{guarded(fn);});return b;};
        auto* start=button("Koop-Spiel starten",theme::Glyph::Play,[this]{startGame();});start->setObjectName("startGame");start->setMinimumHeight(74);gameColumnLayout->addWidget(start);
        menuBody->addWidget(gameColumn,5);
        auto* actions=new QWidget;auto* actionLayout=new QVBoxLayout(actions);actionLayout->setContentsMargins(0,0,0,0);actionLayout->setSpacing(14);section("Menü",actionLayout);
        actionGrid=new QGridLayout;actionLayout->addLayout(actionGrid);actionLayout->addStretch();
        auto action=[&](QString name,theme::Glyph glyph,std::function<void()> fn){auto* b=button(name,glyph,fn);menuActions.append(b);actionGrid->addWidget(b,(menuActions.size()-1)/2,(menuActions.size()-1)%2);return b;};
        action("Controller zuordnen",theme::Glyph::Controller,[this]{assignPads();});
        action("Bildschirmaufteilung",theme::Glyph::Screens,[this]{layoutDialog();});
        action("ROM auswählen",theme::Glyph::Cartridge,[this]{chooseRom();});
        action("Spielstände",theme::Glyph::Save,[this]{restoreDialog();});
        action("Tastenbelegung",theme::Glyph::Buttons,[this]{remap();});
        action("Einstellungen",theme::Glyph::Settings,[this]{configure();});
        action("Logs anzeigen",theme::Glyph::Logs,[this]{showLogs();});
        action("Launcher beenden",theme::Glyph::Exit,[this]{close();})->setObjectName("exitLauncher");
        menuBody->addWidget(actions,6);
        QWidget* previous=start;for(auto* b:menuActions){QWidget::setTabOrder(previous,b);previous=b;}
        statusLabel=new QLabel("Noch kein Spiel gestartet. Ersteinrichtung im Desktopmodus empfohlen.");statusLabel->setObjectName("sessionStatus");statusLabel->setWordWrap(true);contentLayout->addWidget(statusLabel);
        auto* navigation=new QLabel("Steuerkreuz: Auswahl · A: Öffnen · Tastatur: Tab / Leertaste");navigation->setObjectName("navigationHelp");navigation->setWordWrap(true);contentLayout->addWidget(navigation);
        auto* menuScroll=new QScrollArea;
        menuScroll->setObjectName("menuScroll");menuScroll->setWidgetResizable(true);
        menuScroll->setFrameShape(QFrame::NoFrame);menuScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        menuScroll->setWidget(menu);pages->addWidget(menuScroll);
        connect(qApp,&QApplication::focusChanged,this,[menuScroll](QWidget*,QWidget* focused){
            if(focused && menuScroll->isAncestorOf(focused)) menuScroll->ensureWidgetVisible(focused,0,12);
        });
        play=new PlayView(session,controllers,settings);pages->addWidget(play);
        connect(pages,&QStackedWidget::currentChanged,this,[this](int page){if(page==0)play->neutralInput();});
        play->leave=[this]{pages->setCurrentIndex(0);};
        play->endSession=[this]{session.stop();pages->setCurrentIndex(0);};
        session.status=[this](QString text){statusLabel->setProperty("tone","normal");statusLabel->setText(text);statusLabel->style()->unpolish(statusLabel);statusLabel->style()->polish(statusLabel);};
        session.error=[this](QString text){statusLabel->setProperty("tone","error");statusLabel->setText(text);statusLabel->style()->unpolish(statusLabel);statusLabel->style()->polish(statusLabel);};
        session.ended=[this]{pages->setCurrentIndex(0);};
        setWindowTitle("Pokémon Platinum Local Co-op");resize(QSize(1280,800).boundedTo(screen()->availableGeometry().size()));
        updateAppearance();
        start->setFocus();
        // UI navigation works through mapped controllers after assignment.
        uiTimer.setInterval(30);connect(&uiTimer,&QTimer::timeout,this,[this]{
            if(!isVisible() || isMinimized() || pages->currentIndex()!=0 || QApplication::activeModalWidget()) {
                // Require release before accepting a held button after returning.
                menuHeld=true;return;
            }
            controllers.update();
            const auto input=controllers.menuState();
            bool a=input.accept,up=input.up,down=input.down;
            bool any=a||up||down;
            if(any&&!menuHeld){ if(a) {if(auto* b=qobject_cast<QPushButton*>(QApplication::focusWidget()))b->click();}else {QKeyEvent event(QEvent::KeyPress,up?Qt::Key_Backtab:Qt::Key_Tab,up?Qt::ShiftModifier:Qt::NoModifier);QApplication::sendEvent(QApplication::focusWidget(),&event);} }
            menuHeld=any;
        });uiTimer.start();
    }
protected:
    void changeEvent(QEvent* e) override {
        QWidget::changeEvent(e);
        if(e->type()==QEvent::WindowStateChange && isMinimized()) {
            menuHeld=true;
            if(play) play->neutralInput();
        }
    }
    void resizeEvent(QResizeEvent* e) override {
        QWidget::resizeEvent(e);
        if(menuContent)updateAppearance();
    }
    void closeEvent(QCloseEvent* e) override {session.stop();e->accept();}
};
#ifndef PLATINUM_COOP_NO_MAIN
int main(int argc,char** argv) {
    QApplication app(argc,argv); app.setApplicationName("platinum-coop");app.setOrganizationName("PlatinumLocalCoop");
    std::signal(SIGTERM,requestQuit);std::signal(SIGINT,requestQuit);
    QTimer quitTimer;QObject::connect(&quitTimer,&QTimer::timeout,&app,[&]{if(quitRequested)app.quit();});quitTimer.start(100);
    QStringList args=app.arguments();
    if(args.contains("--help")) {puts("platinum-coop [--fullscreen] [--first-run] [--data-dir PATH]\nErsteinrichtung im Desktopmodus. Siehe docs/bazzite-installation.md.");return 0;}
    QString root=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/pokemon-platinum-local-coop";
    int i=args.indexOf("--data-dir");if(i>=0){if(i+1>=args.size()){fputs("--data-dir requires a path\n",stderr);return 2;}root=QFileInfo(args[i+1]).absoluteFilePath();}
    try { Launcher launcher(root); if(args.contains("--fullscreen"))launcher.showFullScreen();else launcher.show();
        int smoke=args.indexOf("--smoke-test");
        if(smoke>=0) {
            if(smoke+1>=args.size())return 2;
            int sizeIndex=args.indexOf("--smoke-size");
            if(sizeIndex>=0 && sizeIndex+1<args.size()) {
                auto parts=args[sizeIndex+1].split('x');
                if(parts.size()==2 && parts[0].toInt()>=320 && parts[1].toInt()>=240)launcher.resize(parts[0].toInt(),parts[1].toInt());
            }
            QTimer::singleShot(300,&app,[&]{bool ok=launcher.grab().save(args[smoke+1]);app.exit(ok?0:1);});
        }
        if(args.contains("--first-run") && !args.contains("--smoke-test")) QTimer::singleShot(400,&launcher,[&]{launcher.firstRun();});
        return app.exec(); }
    catch(const std::exception& e){QMessageBox::critical(nullptr,"Local Co-op",e.what());return 1;}
}
#endif
