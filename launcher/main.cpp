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
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QImage>
#include "Session.h"
#include "Controllers.h"
#include "Layout.h"
using namespace coop;

class PlayView : public QWidget {
    Session& session; Controllers& controllers;
    std::array<QImage,2> tops,bottoms;
    std::array<bool,2> bottomOnly{{false,false}};
    std::array<uint32_t,2> lastFrames{{0,0}};
    QTimer timer; QElapsedTimer delta;
public:
    Settings& settings;
    std::function<void()> leave, endSession;
    PlayView(Session& s,Controllers& c,Settings& cfg):session(s),controllers(c),settings(cfg) {
        setFocusPolicy(Qt::StrongFocus); timer.setInterval(16); delta.start();
        connect(&timer,&QTimer::timeout,this,[this]{
            if(!session.running()) return;
            if(!isVisible()) { for(int p=0;p<2;p++) if(auto* s=session.shared(p)) {s->keys.store(0xFFF);s->touch.store(0);} return; }
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
    void reset() { lastFrames={{0,0}}; tops={}; bottoms={}; bottomOnly={{false,false}}; delta.restart(); }
protected:
    void keyPressEvent(QKeyEvent* e) override { if(e->key()==Qt::Key_Escape) { if(leave) leave(); } else QWidget::keyPressEvent(e); }
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this); painter.fillRect(rect(),QColor("#080c14"));
        painter.setRenderHint(QPainter::SmoothPixmapTransform,false);
        for(int p=0;p<2;p++) {
            auto area=playerArea(rect(),p,settings.horizontal).adjusted(12,12,-12,-12);
            QRectF label(area.x(),area.y(),area.width(),60);
            painter.setPen(QColor("#dce8ff")); painter.setFont(QFont("Sans",22,QFont::Bold));
            painter.drawText(label,Qt::AlignCenter,QString("Spieler %1%2").arg(p+1).arg(controllers.connected(p)?"":" · Controller getrennt"));
            area.adjust(0,64,0,-44);
            auto dest=screens(area,settings.layout,bottomOnly[p]);
            if(!tops[p].isNull() && !dest[0].isEmpty()) painter.drawImage(dest[0],tops[p]);
            if(!bottoms[p].isNull() && !dest[1].isEmpty()) painter.drawImage(dest[1],bottoms[p]);
            if(!dest[1].isEmpty()) {
                auto cursor=controllers.cursor(p); QPointF point(dest[1].x()+cursor.x()/256*dest[1].width(),dest[1].y()+cursor.y()/192*dest[1].height());
                painter.setPen(QPen(QColor("#80ffe0"),2)); painter.drawEllipse(point,8,8);
            }
            painter.setFont(QFont("Sans",14)); painter.setPen(QColor("#a3b3ca"));
            painter.drawText(QRectF(area.x(),area.bottom()+8,area.width(),36),Qt::AlignCenter,"Touch: rechter Stick + Klick · Ansicht: Select + R · Ende: Start + Select (2 s)");
        }
        painter.setPen(QPen(QColor("#33415b"),2));
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
    PlayView* play;
    QLabel *statusLabel,*romLabel,*padLabel;
    QTimer uiTimer;
        bool menuHeld=false;
    bool largeUi=false;
    QJsonObject stored;
    void report(QString msg) { QMessageBox::warning(this,"Pokémon Platinum Local Co-op",msg); }
    void saveSettings() {
        stored["emulator"]=settings.emulator; stored["rom"]=settings.rom;
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
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(&ok);
        connect(&ok,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&ok,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()!=QDialog::Accepted) return;
        controllers.assign(a.currentData().toInt(),b.currentData().toInt());
        for(int p=0;p<2;p++) for(auto pad:pads) if(pad.id==controllers.id(p)) stored[p?"pad2":"pad1"]=QJsonObject{{"guid",pad.guid},{"serial",pad.serial},{"name",pad.name}};
        padLabel->setText("Spieler 1: "+a.currentText()+"\nSpieler 2: "+b.currentText()); saveSettings();
    }
    void configure() {
        QDialog dialog(this);dialog.setWindowTitle("Einstellungen");QFormLayout form(&dialog);
        QLineEdit emu(settings.emulator), b9(settings.bios9), b7(settings.bios7),fw(settings.firmware);
        auto row=[&](QString title,QLineEdit& line){ auto* box=new QWidget(&dialog);auto* layout=new QHBoxLayout(box); layout->addWidget(&line); auto* browse=new QPushButton("Auswählen",box);layout->addWidget(browse);connect(browse,&QPushButton::clicked,&dialog,[&,title]{auto file=QFileDialog::getOpenFileName(&dialog,title);if(!file.isEmpty())line.setText(file);});form.addRow(title,box); };
        row("Emulator",emu);row("ARM9 BIOS (optional)",b9);row("ARM7 BIOS (optional)",b7);row("DS-Firmware (optional)",fw);
        QLabel help("Ohne externe Dateien nutzt melonDS seine freie BIOS-Implementierung und erzeugte Firmware (Direktstart). Externe Dateien nur vollständig und als eigene rechtmäßig bereitgestellte Dumps auswählen.");help.setWordWrap(true);form.addRow(&help);
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(&ok);
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
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(&ok);connect(&ok,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&ok,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()==QDialog::Accepted) {for(int i=0;i<12;i++) controllers.buttons[i]=combos[i]->currentData().toInt();saveSettings();}
    }
    void layoutDialog() {
        QDialog dialog(this);dialog.setWindowTitle("Bildschirmaufteilung");QFormLayout form(&dialog);
        QComboBox split,ds;split.addItems({"Links / Rechts","Oben / Unten"});split.setCurrentIndex(settings.horizontal?1:0);
        ds.addItems({"Oberer Bildschirm groß + Touchscreen klein","DS-Bildschirme nebeneinander","Ein Bildschirm · Select + R wechseln"});ds.setCurrentIndex(settings.layout);
        form.addRow("Spieler",&split);form.addRow("DS-Ansicht",&ds);
        QDialogButtonBox ok(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(&ok);connect(&ok,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(&ok,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
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
            saves.acquire();try{saves.restore(player.currentIndex(),game,path);saves.release();}catch(...){saves.release();throw;}
            QMessageBox::information(&dialog,"Spielstände","Backup erfolgreich wiederhergestellt.");
        });});
        QDialogButtonBox close(QDialogButtonBox::Close);form.addRow(&close);connect(&close,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();
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
        padLabel->setText("Spieler 1: "+pads[chosen[0]].name+" · Instanz "+QString::number(pads[chosen[0]].id)+"\nSpieler 2: "+pads[chosen[1]].name+" · Instanz "+QString::number(pads[chosen[1]].id));
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
        auto rom=QFileDialog::getOpenFileName(this,"Vorbereitete Project-PM-ROM wählen",settings.rom,"Nintendo DS (*.nds *.srl)");if(rom.isEmpty())return;
        requireMultiplayerPatch(validateRom(rom));
        bool confirmed=QMessageBox::question(this,"Project PM ROM","Ist diese Platin-ROM mit dem passenden Project-PM-Multiplayer-Patch vorbereitet? Der Launcher kann Patchversion und Welt-Synchronisation nicht anhand des ROM-Headers bestätigen.") == QMessageBox::Yes;
        if(!confirmed)return;
        settings.rom=rom;settings.patchedConfirmed=true;romLabel->setText(QFileInfo(rom).fileName());saveSettings();
    }
public:
    Launcher(QString data):root(std::move(data)),saves(root),session(saves) {
        QFile f(root+"/settings.json");if(f.open(QIODevice::ReadOnly))stored=QJsonDocument::fromJson(f.readAll()).object();
        settings.emulator=stored.value("emulator").toString(QCoreApplication::applicationDirPath()+"/melonDS-coop");settings.rom=stored["rom"].toString();
        settings.bios9=stored["bios9"].toString();settings.bios7=stored["bios7"].toString();settings.firmware=stored["firmware"].toString();
        settings.layout=qBound(0,stored["layout"].toInt(),2);settings.horizontal=stored["horizontal"].toBool();settings.patchedConfirmed=stored["patchedConfirmed"].toBool();
        auto mapping=stored["buttons"].toArray();if(mapping.size()==12)for(int i=0;i<12;i++){int n=mapping[i].toInt(-1);controllers.buttons[i]=(n>=-1&&n<SDL_CONTROLLER_BUTTON_MAX)?n:-1;}
        auto* outer=new QVBoxLayout(this);pages=new QStackedWidget(this);outer->addWidget(pages);
        auto* menu=new QWidget;auto* layout=new QVBoxLayout(menu);layout->setContentsMargins(60,30,60,30);
        auto* title=new QLabel("Pokémon Platinum\nLocal Co-op");title->setObjectName("title");title->setStyleSheet("font-size: 42px; font-weight: bold; color: #9ff4d8;");layout->addWidget(title);
        romLabel=new QLabel(settings.rom.isEmpty()?"Noch keine Project-PM-ROM ausgewählt":QFileInfo(settings.rom).fileName());layout->addWidget(romLabel);
        padLabel=new QLabel("Controller noch nicht zugeordnet");layout->addWidget(padLabel);
        auto button=[&](QString name,std::function<void()> fn){auto* b=new QPushButton(name);b->setMinimumHeight(66);layout->addWidget(b);connect(b,&QPushButton::clicked,this,[this,fn]{guarded(fn);});return b;};
        auto* start=button("Koop-Spiel starten",[this]{startGame();});
        button("Controller zuordnen",[this]{assignPads();});button("Bildschirmaufteilung",[this]{layoutDialog();});
        button("ROM auswählen",[this]{chooseRom();});button("Spielstände",[this]{restoreDialog();});button("Tastenbelegung",[this]{remap();});
        button("Einstellungen",[this]{configure();});button("Logs anzeigen",[this]{showLogs();});
        button("Spiel beenden",[this]{session.stop();pages->setCurrentIndex(0);});
        statusLabel=new QLabel("Bereit · Ersteinrichtung im Desktopmodus empfohlen");statusLabel->setWordWrap(true);layout->addWidget(statusLabel);layout->addStretch();
        pages->addWidget(menu);play=new PlayView(session,controllers,settings);pages->addWidget(play);
        play->leave=[this]{pages->setCurrentIndex(0);};
        play->endSession=[this]{session.stop();pages->setCurrentIndex(0);};
        session.status=[this](QString text){statusLabel->setText(text);};
        session.error=[this](QString text){statusLabel->setText(text);};
        session.ended=[this]{pages->setCurrentIndex(0);};
        setWindowTitle("Pokémon Platinum Local Co-op");resize(1280,1000);
        setStyleSheet("QWidget {background:#111b2b; color:#e8efff; font-size:24px;} QPushButton {background:#24354e; border:2px solid #526b91; border-radius:8px; padding:10px;} QPushButton:focus {border-color:#9ff4d8; background:#345470;} QComboBox,QLineEdit {padding:8px; background:#24354e;}");
        start->setFocus();
        // UI navigation works through mapped controllers after assignment.
        uiTimer.setInterval(30);connect(&uiTimer,&QTimer::timeout,this,[this]{
            if(pages->currentIndex()!=0 || QApplication::activeModalWidget())return;
            controllers.update();
            SDL_GameController* h=nullptr;
            for(int i=0;i<SDL_NumJoysticks();i++) if(SDL_IsGameController(i)){h=SDL_GameControllerOpen(i);break;}
            if(!h)return;
            bool a=SDL_GameControllerGetButton(h,SDL_CONTROLLER_BUTTON_A),up=SDL_GameControllerGetButton(h,SDL_CONTROLLER_BUTTON_DPAD_UP),down=SDL_GameControllerGetButton(h,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            bool any=a||up||down;
            if(any&&!menuHeld){ if(a) {if(auto* b=qobject_cast<QPushButton*>(QApplication::focusWidget()))b->click();}else {QKeyEvent event(QEvent::KeyPress,up?Qt::Key_Backtab:Qt::Key_Tab,up?Qt::ShiftModifier:Qt::NoModifier);QApplication::sendEvent(QApplication::focusWidget(),&event);} }
            menuHeld=any;SDL_GameControllerClose(h);
        });uiTimer.start();
    }
protected:
    void resizeEvent(QResizeEvent* e) override {
        QWidget::resizeEvent(e);
        bool large=width()>=2800 && height()>=1600;
        if(large==largeUi)return;
        largeUi=large;
        setStyleSheet(QString("QWidget {background:#111b2b; color:#e8efff; font-size:%1px;} QPushButton {background:#24354e; border:2px solid #526b91; border-radius:8px; padding:10px; min-height:%2px;} QPushButton:focus {border-color:#9ff4d8; background:#345470;} QComboBox,QLineEdit {padding:8px; background:#24354e;}").arg(large?40:24).arg(large?106:44));
        if(auto* title=findChild<QLabel*>("title"))title->setStyleSheet(QString("font-size:%1px;font-weight:bold;color:#9ff4d8;").arg(large?72:42));
    }
    void closeEvent(QCloseEvent* e) override {session.stop();e->accept();}
};
int main(int argc,char** argv) {
    QApplication app(argc,argv); app.setApplicationName("platinum-coop");app.setOrganizationName("PlatinumLocalCoop");
    std::signal(SIGTERM,requestQuit);std::signal(SIGINT,requestQuit);
    QTimer quitTimer;QObject::connect(&quitTimer,&QTimer::timeout,&app,[&]{if(quitRequested)app.quit();});quitTimer.start(100);
    QStringList args=app.arguments();
    if(args.contains("--help")) {puts("platinum-coop [--fullscreen] [--data-dir PATH]\nFirst setup in desktop mode. See docs/bazzite-installation.md.");return 0;}
    QString root=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/pokemon-platinum-local-coop";
    int i=args.indexOf("--data-dir");if(i>=0){if(i+1>=args.size()){fputs("--data-dir requires a path\n",stderr);return 2;}root=QFileInfo(args[i+1]).absoluteFilePath();}
    try { Launcher launcher(root); if(args.contains("--fullscreen"))launcher.showFullScreen();else launcher.show();
        int smoke=args.indexOf("--smoke-test");
        if(smoke>=0) {
            if(smoke+1>=args.size())return 2;
            int sizeIndex=args.indexOf("--smoke-size");
            if(sizeIndex>=0 && sizeIndex+1<args.size()) {
                auto parts=args[sizeIndex+1].split('x');
                if(parts.size()==2 && parts[0].toInt()>=800 && parts[1].toInt()>=800)launcher.resize(parts[0].toInt(),parts[1].toInt());
            }
            QTimer::singleShot(300,&app,[&]{bool ok=launcher.grab().save(args[smoke+1]);app.exit(ok?0:1);});
        }
        return app.exec(); }
    catch(const std::exception& e){QMessageBox::critical(nullptr,"Local Co-op",e.what());return 1;}
}
