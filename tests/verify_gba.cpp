// SPDX-License-Identifier: GPL-3.0-or-later
// Explicit private acceptance tool: requires the user's own German FireRed ROM.
// Uses the real worker, production Session/PlayView and virtual SDL controllers.
#define PLATINUM_COOP_NO_MAIN
#include "../launcher/main.cpp"
#include <QThread>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <future>
#include <optional>

// Read-only observation of native battery output, never emulator RAM. A new
// complete checksum-valid generation is the acknowledgment for SAVE actions.
static std::optional<uint32_t> fireRedSaveGeneration(const QByteArray& bytes) {
    if(bytes.size()!=0x20000)return {};
    auto u16=[&](int at){return unsigned(uint8_t(bytes[at]))|(unsigned(uint8_t(bytes[at+1]))<<8);};
    auto u32=[&](int at){return uint32_t(u16(at))|(uint32_t(u16(at+2))<<16);};
    std::optional<uint32_t> latest;
    for(unsigned slot=0;slot<2;++slot) {
        unsigned mask=0;uint32_t generation=0;bool valid=true;
        for(unsigned sector=0;sector<14;++sector) {
            const unsigned at=(slot*14+sector)*0x1000,id=u16(at+0xFF4);
            if(id>=14 || (mask&(1u<<id)) || u32(at+0xFF8)!=0x08012025){valid=false;break;}
            const unsigned total=id==0?0xF24:id<5?0x3D68:0x83D0,chunk=id==0?0:id<5?id-1:id-5;
            const unsigned size=std::min(0xF80u,total-chunk*0xF80u);
            uint32_t sum=0;for(unsigned offset=0;offset<size;offset+=4)sum+=u32(at+offset);
            const auto counter=u32(at+0xFFC);
            if(u16(at+0xFF6)!=uint16_t((sum>>16)+sum) || (sector && counter!=generation)){valid=false;break;}
            generation=counter;mask|=1u<<id;
        }
        if(valid && mask==0x3FFF && (!latest || int32_t(generation-*latest)>0))latest=generation;
    }
    return latest;
}

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
        const auto romHash=validateRom(settings.rom);
        settings.fireRedAudio=qEnvironmentVariable("COOP_VERIFY_FIRERED_AUDIO","player1");
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
        std::array<bool,2> observedInputOwners{};
        auto advanceFrames=[&](unsigned count) {
            const auto start=session.shared(0)->frame.load();QElapsedTimer deadline;deadline.start();
            while(session.shared(0)->frame.load()-start<count) {
                spin(5);
                const auto owner=session.shared(0)->role.load();
                if(session.fireRedStatusCode()==1 && owner>=1 && owner<=2)observedInputOwners[owner-1]=true;
                if(!error.isEmpty() || !session.running())fail(error.isEmpty()?"Worker stopped during frame wait":error);
                if(deadline.elapsed()>15000+int(count)*100)fail("Worker frame heartbeat stalled");
            }
        };
        auto button=[&](int player,int key,bool down) {
            if(player<0 || player>2 || key<0 || key>=12 || controllers.buttons[key]<0)fail("Invalid native button action");
            for(int p=0;p<2;++p)if(player==2 || player==p)
                SDL_JoystickSetVirtualButton(pads[p],controllers.buttons[key],down);
        };
        auto pulse=[&](int player,int key,unsigned frames=4,unsigned gap=20) {
            button(player,key,true);advanceFrames(frames);button(player,key,false);advanceFrames(gap);
        };
        auto saveGeneration=[&](int player) {
            QFile file(saves.profile(player,romHash)+"/saves/game.sav");
            if(!file.open(QIODevice::ReadOnly))fail("Cannot observe native battery output");
            return fireRedSaveGeneration(file.readAll());
        };
        auto capture=[&](QString name){
            if(name.isEmpty() || name.contains('/') || name.contains(".."))fail("Invalid capture name");
            spin(50);if(!view.grab().save(output+"/"+name+".png"))fail("Capture failed");
            QJsonObject info;
            for(int p=0;p<2;p++) {
                QImage top,bottom;
                if(!session.copyFrame(p,top,bottom) || top.size()!=QSize(240,160) || !bottom.isNull())fail("Missing GBA framebuffer");
                top.save(output+"/"+name+"-player"+QString::number(p+1)+".png");
                info[QString("player%1_frames").arg(p+1)]=int(session.shared(p)->frame.load());
                info[QString("player%1_peers").arg(p+1)]=int(session.shared(p)->peers.load());
                info[QString("player%1_bridge_state").arg(p+1)]=int(session.shared(p)->bridgeActive.load());
                info[QString("player%1_input_owner").arg(p+1)]=int(session.shared(p)->role.load());
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
            } else if(kind=="press-frames") {
                for(int n=0;n<qBound(1,a["count"].toInt(1),100);++n)
                    pulse(a["player"].toInt(2),a["key"].toInt(),unsigned(qBound(2,a["frames"].toInt(8),120)),
                          unsigned(qBound(2,a["gap"].toInt(24),120)));
            } else if(kind=="field-step") {
                if(session.fireRedStatusCode()!=0)fail("Cannot step while a battle or recovery owns the field");
                const int key=a["key"].toInt();if(key<4 || key>7)fail("Field step needs a D-pad direction");
                // A map connection can start on this step. Let both native
                // fades finish before another pulse; checking peers here
                // would deadlock when one avatar crosses before the other.
                pulse(a["player"].toInt(2),key,4,90);
            } else if(kind=="seek-shared-battle") {
                if(session.fireRedStatusCode()!=0)fail("Expected shared field before seeking battle");
                const int player=a["player"].toInt();if(player<0 || player>1)fail("Invalid battle initiator");
                for(unsigned step=0;step<240 && session.fireRedStatusCode()==0;++step)
                    pulse(player,step%2?7:6,8,24);
                if(session.fireRedStatusCode()!=1)fail("Natural grass encounter did not become a shared battle");
            } else if(kind=="finish-shared-battle") {
                if(session.fireRedStatusCode()!=1)fail("Expected shared battle before native fight input");
                observedInputOwners={};
                for(unsigned tick=0;tick<1000;++tick) {
                    const auto owner=session.shared(0)->role.load();if(owner>=1 && owner<=2)observedInputOwners[owner-1]=true;
                    if(session.fireRedStatusCode()==0 && session.shared(0)->peers.load() && session.shared(1)->peers.load())break;
                    if(session.fireRedStatusCode()>2)fail("Shared battle entered an unexpected recovery state");
                    pulse(2,0,4,12);
                }
                if(session.fireRedStatusCode()!=0 || !session.shared(0)->peers.load() || !session.shared(1)->peers.load())fail("Shared fight did not return both players to the field");
                if(!observedInputOwners[0] || !observedInputOwners[1])fail("Shared fight never exposed both human input owners");
                advanceFrames(30);
            } else if(kind=="native-save-both") {
                if(session.fireRedStatusCode()!=0 || !session.shared(0)->peers.load() || !session.shared(1)->peers.load())fail("Both players must be on their common field before native SAVE");
                const std::array<std::optional<uint32_t>,2> before{{saveGeneration(0),saveGeneration(1)}};
                if(!before[0] || !before[1])fail("Native SAVE requires two valid starting batteries");
                pulse(2,3,4,30); // START, then the remembered menu position.
                for(int n=0;n<qBound(0,a["down"].toInt(),8);++n)pulse(2,7);
                pulse(2,0,4,30); // SAVE
                bool saved=false;
                for(unsigned tick=0;tick<200;++tick) {
                    const auto first=saveGeneration(0),second=saveGeneration(1);
                    saved=first && second && uint32_t(*first-*before[0])==1 && uint32_t(*second-*before[1])==1;
                    if(saved)break;
                    pulse(2,6,4,8);pulse(2,0,4,12); // Native YES, including overwrite confirmation.
                }
                if(!saved)fail("Native SAVE did not publish exactly one valid new generation for both players");
                for(unsigned tick=0;tick<80 && !(session.shared(0)->peers.load() && session.shared(1)->peers.load());++tick)
                    pulse(2,1,4,12); // Dismiss completed save without starting an interaction.
                if(!session.shared(0)->peers.load() || !session.shared(1)->peers.load())fail("Native SAVE did not return both players to their field");
                advanceFrames(70); // Production battery flush interval, not a test-only flush.
            } else if(kind=="restart") {
                session.stop();if(saves.locked())fail("Profile lock leaked");
                session.start(settings);view.reset();spin(1500);
            } else if(kind=="boot-pair-to-field") {
                QElapsedTimer elapsed;elapsed.start();
                while(elapsed.elapsed()<30000 && !(session.shared(0)->peers.load() && session.shared(1)->peers.load())) {
                    const auto t=elapsed.elapsed();
                    for(auto* pad:pads) {
                        SDL_JoystickSetVirtualButton(pad,controllers.buttons[3],t<3500 && t%500<80);
                        SDL_JoystickSetVirtualButton(pad,controllers.buttons[0],t>=3500 && t<16000 && t%350<90);
                        SDL_JoystickSetVirtualButton(pad,controllers.buttons[1],t>=16000 && t%350<90);
                    }
                    spin(30);if(!session.running())fail("Worker stopped during native Continue");
                }
                for(auto* pad:pads)for(int key:{0,1,3})SDL_JoystickSetVirtualButton(pad,controllers.buttons[key],0);
                spin(200);
                if(!session.shared(0)->peers.load() || !session.shared(1)->peers.load())fail("Both native saves did not Continue to their common field");
            } else if(kind=="walk-until-pause") {
                const int p=a["player"].toInt();if(p<0 || p>1)fail("Invalid walker");
                QElapsedTimer elapsed;elapsed.start();
                auto advance=[&](unsigned count) {
                    const auto start=session.shared(p)->frame.load();
                    while(session.shared(p)->frame.load()-start<count && elapsed.elapsed()<45000 &&
                          !firered::battlePaused(session.fireRedStatusCode())) {
                        spin(10);if(!session.running())fail("Worker stopped while seeking recovery pause");
                    }
                };
                // One completed tile in each direction keeps this authored
                // fixture inside its grass patch. Wall-time holds can drift
                // outside it when two acceptance workers compete for CPU.
                for(unsigned step=0;elapsed.elapsed()<45000 && !firered::battlePaused(session.fireRedStatusCode());++step) {
                    const int button=controllers.buttons[step%2 ? 7 : 6];
                    SDL_JoystickSetVirtualButton(pads[p],button,1);advance(8);
                    SDL_JoystickSetVirtualButton(pads[p],button,0);advance(24);
                }
                SDL_JoystickSetVirtualButton(pads[p],controllers.buttons[6],0);
                SDL_JoystickSetVirtualButton(pads[p],controllers.buttons[7],0);spin(50);
                if(!firered::battlePaused(session.fireRedStatusCode())) {
                    capture("encounter-timeout");fail("Native encounter did not pause");
                }
            } else if(kind=="continue-individually") {
                auto* button=view.findChild<QPushButton*>("fireRedContinueIndividual");
                if(!button || !button->isVisible() || !firered::battlePaused(session.fireRedStatusCode()))fail("Recovery action is unavailable");
                button->click();spin(100);
                if(firered::pairState(session.fireRedStatusCode())!=firered::PairState::Individual)fail("Recovery command did not reach worker");
            } else if(kind=="finish-individual-battles") {
                QElapsedTimer elapsed;elapsed.start();
                while(elapsed.elapsed()<45000 && firered::pairState(session.fireRedStatusCode())==firered::PairState::Individual) {
                    for(auto* pad:pads)SDL_JoystickSetVirtualButton(pad,controllers.buttons[0],elapsed.elapsed()%300<90);spin(30);
                    if(!session.running())fail("Worker stopped during native individual battle");
                }
                for(auto* pad:pads)SDL_JoystickSetVirtualButton(pad,controllers.buttons[0],0);spin(50);
                if(session.fireRedStatusCode()!=0)fail("Individual battles did not return to co-op");
            } else fail("Unknown test action");
            if(!error.isEmpty())fail(error);
            if(!session.running())fail("GBA worker stopped unexpectedly");
            QJsonObject state;
            for(int player=0;player<2;++player) {
                state[QString("player%1_frames").arg(player+1)]=int(session.shared(player)->frame.load());
                state[QString("player%1_state").arg(player+1)]=int(session.shared(player)->bridgeActive.load());
                state[QString("player%1_peers").arg(player+1)]=int(session.shared(player)->peers.load());
            }
            std::cout<<"State "<<QJsonDocument(state).toJson(QJsonDocument::Compact).constData()<<std::endl;
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
        QJsonObject report{{"real_mgba_workers",true},{"rom_sha256",validateRom(settings.rom)},{"logs",logs},{"clean_stop",true},{"virtual_controllers",true},{"paired_worker_experiment",session.isFireRedPair()},{"world_coop",false}};
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
