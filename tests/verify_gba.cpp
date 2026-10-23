// SPDX-License-Identifier: GPL-3.0-or-later
// Explicit private acceptance tool: requires the user's own German FireRed ROM.
// Uses the real worker, production Session/PlayView and virtual SDL controllers.
#define PLATINUM_COOP_NO_MAIN
#include "../launcher/main.cpp"
#include <QThread>
#include <iostream>
#include <future>

static void spin(int ms) {
    QElapsedTimer t;t.start();
    while(t.elapsed()<ms){QCoreApplication::processEvents();QThread::msleep(5);}
}
int main(int argc,char**argv) {
    QApplication app(argc,argv);
    if(argc!=6){fprintf(stderr,"verify-gba ROM WORKER PROFILE OUTPUT ACTIONS.json\n");return 2;}
    try {
        i18n::setLanguage("de");
        const QString root=QFileInfo(argv[3]).absoluteFilePath(),output=QFileInfo(argv[4]).absoluteFilePath();
        if(!QDir().mkpath(output))fail("Cannot create capture directory");
        const bool interactive=QString::fromLocal8Bit(argv[5])=="-";
        QJsonDocument actions;
        if(!interactive) {
            QFile actionFile(QString::fromLocal8Bit(argv[5]));
            if(!actionFile.open(QIODevice::ReadOnly))fail("Cannot read actions");
            QJsonParseError parse;actions=QJsonDocument::fromJson(actionFile.readAll(),&parse);
            if(parse.error!=QJsonParseError::NoError || !actions.isArray())fail("Invalid actions JSON");
        }
        SaveStore saves(root);Session session(saves);Controllers controllers;
        Settings settings;settings.rom=QFileInfo(argv[1]).absoluteFilePath();settings.gbaEmulator=QFileInfo(argv[2]).absoluteFilePath();
        std::array<SDL_Joystick*,2> pads{};
        std::array<SDL_JoystickID,2> owned{};
        for(int p=0;p<2;p++) {
            auto index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
            if(index<0 || !(pads[p]=SDL_JoystickOpen(index)))fail("Cannot attach virtual controller");
            owned[p]=SDL_JoystickInstanceID(pads[p]);
        }
        controllers.assign(SDL_JoystickInstanceID(pads[0]),SDL_JoystickInstanceID(pads[1]));
        QString error;session.error=[&](QString value){error=value;};
        session.start(settings);
        PlayView view(session,controllers,settings);view.resize(1280,720);view.show();view.reset();
        auto capture=[&](QString name){
            if(name.isEmpty() || name.contains('/') || name.contains(".."))fail("Invalid capture name");
            spin(50);if(!view.grab().save(output+"/"+name+".png"))fail("Capture failed");
            QJsonObject info;
            for(int p=0;p<2;p++) {
                QImage top,bottom;
                if(!session.copyFrame(p,top,bottom) || top.size()!=QSize(240,160) || !bottom.isNull())fail("Missing GBA framebuffer");
                top.save(output+"/"+name+"-player"+QString::number(p+1)+".png");
                info[QString("player%1_frames").arg(p+1)]=int(session.shared(p)->frame.load());
                info[QString("player%1_picture").arg(p+1)]=QString::fromLatin1(QCryptographicHash::hash(QByteArray(reinterpret_cast<const char*>(top.constBits()),top.sizeInBytes()),QCryptographicHash::Sha256).toHex());
            }
            atomicWrite(output+"/"+name+".json",QJsonDocument(info).toJson());
            std::cout<<"Captured "<<name.toStdString()<<std::endl;
        };
        auto execute=[&](const QJsonObject& a) {
            const auto kind=a["action"].toString();
            if(kind=="wait")spin(qBound(0,a["ms"].toInt(),60000));
            else if(kind=="capture")capture(a["name"].toString());
            else if(kind=="press") {
                int p=a["player"].toInt(),key=a["key"].toInt();
                if(p<0 || p>1 || key<0 || key>=12)fail("Invalid input");
                int button=controllers.buttons[key];if(button<0)fail("Unmapped test input");
                for(int i=0;i<qBound(1,a["count"].toInt(1),200);i++) {
                    SDL_JoystickSetVirtualButton(pads[p],button,1);spin(qBound(40,a["ms"].toInt(120),2000));
                    SDL_JoystickSetVirtualButton(pads[p],button,0);spin(qBound(40,a["gap"].toInt(120),2000));
                }
            } else if(kind=="restart") {
                session.stop();if(saves.locked())fail("Profile lock leaked");
                session.start(settings);view.reset();spin(1500);
            } else fail("Unknown test action");
            if(!error.isEmpty())fail(error);
            if(!session.running())fail("GBA worker stopped unexpectedly");
        };
        if(interactive) {
            std::cout<<"Ready for private acceptance actions"<<std::endl;
            std::string line;
            while(true) {
                auto input=std::async(std::launch::async,[&]{return bool(std::getline(std::cin,line));});
                while(input.wait_for(std::chrono::milliseconds(0))!=std::future_status::ready)spin(20);
                if(!input.get())break;
                QJsonParseError parse;auto command=QJsonDocument::fromJson(QByteArray::fromStdString(line),&parse);
                if(parse.error!=QJsonParseError::NoError || !command.isObject())fail("Invalid command JSON");
                if(command.object()["action"].toString()=="finish")break;
                execute(command.object());std::cout<<"Action complete"<<std::endl;
            }
        } else for(const auto& value:actions.array())execute(value.toObject());
        const auto logs=session.logDir;
        session.stop();
        if(!error.isEmpty() || saves.locked())fail(error.isEmpty()?"Profile lock leaked":error);
        QJsonObject report{{"real_mgba_workers",true},{"rom_sha256",validateRom(settings.rom)},{"logs",logs},{"clean_stop",true},{"virtual_controllers",true},{"world_coop",false}};
        for(int p=0;p<2;p++) {
            const auto save=saves.profile(p,validateRom(settings.rom))+"/saves/game.sav";
            report[QString("player%1_save_exists").arg(p+1)]=QFileInfo::exists(save);
            if(QFileInfo::exists(save))report[QString("player%1_save_sha256").arg(p+1)]=hashFile(save);
            SDL_JoystickClose(pads[p]);
        }
        controllers.clear();
        for(int i=SDL_NumJoysticks()-1;i>=0;i--)if(SDL_JoystickIsVirtual(i) && (SDL_JoystickGetDeviceInstanceID(i)==owned[0] || SDL_JoystickGetDeviceInstanceID(i)==owned[1]))SDL_JoystickDetachVirtual(i);
        atomicWrite(output+"/result.json",QJsonDocument(report).toJson());
        std::cout<<QJsonDocument(report).toJson().constData();return 0;
    } catch(const std::exception& e){fprintf(stderr,"Acceptance failed: %s\n",e.what());return 1;}
}
