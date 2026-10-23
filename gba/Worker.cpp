// SPDX-License-Identifier: GPL-3.0-or-later
// A real mGBA core behind CouchSplit's existing framebuffer/input IPC.
#include <mgba/core/core.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/memory.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include <mgba/core/blip_buf.h>
#include <QCoreApplication>
#include <QThread>
#include <QElapsedTimer>
#include <SDL.h>
#include <array>
#include <csignal>
#include <memory>
#include "CoopIPC.h"
#include "Rom.h"

static bool validSaveSize(qsizetype size) {
    return size==512 || size==8192 || size==32768 || size==65536 || size==131072;
}
static volatile sig_atomic_t quitRequested=0;
static void requestQuit(int) {quitRequested=1;}
struct CoreLog {
    mLogFilter filter{};
    mLogger logger{};
    CoreLog() {
        mLogFilterInit(&filter);
        filter.defaultLevels=mLOG_FATAL|mLOG_ERROR|mLOG_WARN;
        logger.filter=&filter;
        logger.log=[](mLogger*,int category,mLogLevel,const char* format,va_list args) {
            fprintf(stderr,"[mGBA %s] ",mLogCategoryName(category));
            vfprintf(stderr,format,args);fputc('\n',stderr);
        };
        mLogSetDefaultLogger(&logger);
    }
    ~CoreLog(){mLogSetDefaultLogger(nullptr);mLogFilterDeinit(&filter);}
};
struct CoreOwner {
    mCore* core=nullptr;
    bool initialized=false, configured=false;
    ~CoreOwner() {
        if(!core)return;
        if(configured)mCoreConfigDeinit(&core->config);
        if(initialized)core->deinit(core); else free(core);
    }
};
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    if(app.arguments().contains("--coop-capabilities")) {
        puts("PMCOOP_ABI=2 STABLE_SAVE=1 GBA=1 MGBA=0.10.5 GBA_XROM=1 GBA_MAX_ROM_MIB=512");return 0;
    }
    std::signal(SIGTERM,requestQuit);std::signal(SIGINT,requestQuit);
    SDL_AudioDeviceID audio=0;
    try {
        CoreLog log;
        if(argc!=2)coop::fail("Usage: gba-worker game.gba (requires launcher IPC/profile)");
        const QString rom=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
        const auto romInfo=coop::inspectRom(rom);
        if(romInfo.system!=coop::GameSystem::GameBoyAdvance)coop::fail("Not a supported GBA ROM");
        const auto ipc=qEnvironmentVariable("PMCOOP_IPC"), profile=qEnvironmentVariable("PMCOOP_PROFILE");
        if(ipc.isEmpty() || profile.isEmpty() || !QFileInfo(profile).isDir())coop::fail("Missing launcher profile/IPC");
        coop::Mapping mapping;mapping.open(ipc.toUtf8().toStdString(),false);
        const QString savePath=profile+"/saves/game.sav";
        if(!QFileInfo(profile+"/saves").isDir())coop::fail("Missing save directory");
        if(QFileInfo(savePath).isSymLink())coop::fail("Unsafe save path");
        // The file mapping outlives the core. All extra data remain immutable and
        // directly addressable; no per-player bank state can leak between workers.
        QFile extendedFile(rom);
        const uchar* extendedData=nullptr;
        qint64 extendedSize=0;
        if(romInfo.extendedGba) {
            if(!extendedFile.open(QIODevice::ReadOnly))coop::fail("Cannot open extended ROM");
            extendedSize=extendedFile.size()-coop::GbaNativeLimit;
            extendedData=extendedFile.map(coop::GbaNativeLimit,extendedSize);
            if(!extendedData)coop::fail("Cannot map extended ROM data");
        }
        CoreOwner owner;owner.core=mCoreFind(rom.toUtf8().constData());
        if(!owner.core || owner.core->platform(owner.core)!=mPLATFORM_GBA || !owner.core->init(owner.core))coop::fail("mGBA initialization failed");
        owner.initialized=true;auto* core=owner.core;
        mCoreInitConfig(core,nullptr);owner.configured=true;
        mCoreOptions options{};options.useBios=false;options.skipBios=true;
        options.sampleRate=32768;options.audioBuffers=1024;options.volume=0x100;
        mCoreConfigLoadDefaults(&core->config,&options);mCoreLoadConfig(core);
        if(!mCoreLoadFile(core,rom.toUtf8().constData()))coop::fail("mGBA cannot load ROM");
        QByteArray lastSave;
        if(QFileInfo::exists(savePath)) {
            QFile save(savePath);
            if(!save.open(QIODevice::ReadOnly) || !validSaveSize(save.size()))coop::fail("Invalid GBA battery save size. Original preserved.");
            lastSave=save.readAll();
            if(lastSave.size()!=save.size() || save.error()!=QFileDevice::NoError)coop::fail("Cannot read complete save; original preserved");
            auto* vf=VFileMemChunk(lastSave.constData(),size_t(lastSave.size()));
            if(!vf)coop::fail("Cannot allocate save buffer");
            if(!core->loadSave(core,vf)){vf->close(vf);coop::fail("Cannot load save; original preserved");}
        }
        std::array<color_t,240*160> video{};
        core->setVideoBuffer(core,video.data(),240);
        core->reset(core);
        if(extendedData) GBACouchSplitSetXrom(static_cast<GBA*>(core->board),extendedData,size_t(extendedSize));
        auto flushSave=[&]{
            void* raw=nullptr;const size_t size=core->savedataClone(core,&raw);
            std::unique_ptr<void,decltype(&free)> buffer(raw,free);
            if(!size)return;
            if(!validSaveSize(qsizetype(size)) || !raw)coop::fail("Unexpected GBA battery save size; previous save preserved");
            if(!lastSave.isEmpty() && qsizetype(size)<lastSave.size())coop::fail("GBA save would be truncated; previous save preserved");
            QByteArray bytes(static_cast<const char*>(raw),qsizetype(size));
            if(bytes!=lastSave){coop::atomicWrite(savePath,bytes);lastSave=bytes;}
        };
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)==0) {
            SDL_AudioSpec spec{};spec.freq=32768;spec.format=AUDIO_S16SYS;spec.channels=2;spec.samples=1024;
            audio=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);
            if(audio)SDL_PauseAudioDevice(audio,0);
        }
        auto* left=core->getAudioChannel(core,0);auto* right=core->getAudioChannel(core,1);
        blip_set_rates(left,core->frequency(core),32768);blip_set_rates(right,core->frequency(core),32768);
        std::array<int16_t,4096> samples{};
        std::array<uint32_t,coop::Pixels> top{},bottom{};
        QElapsedTimer clock;clock.start();qint64 target=0;
        const qint64 frameNs=qRound64(1e9*double(core->frameCycles(core))/core->frequency(core));
        unsigned frames=0;
        // Validate a loaded game's detected save size before the first frame.
        // In particular, never pad a truncated known Flash/EEPROM save silently.
        if(!lastSave.isEmpty()) {
            void* raw=nullptr;const size_t size=core->savedataClone(core,&raw);free(raw);
            if(size!=size_t(lastSave.size()))coop::fail("GBA save size does not match this game's detected save type; original preserved");
        }
        fprintf(stdout,"GBA worker: mGBA 0.10.5; independent adventure, no world co-op\n");fflush(stdout);
        while(!quitRequested && !mapping.data->shutdown.load()) {
            core->setKeys(core,(~mapping.data->keys.load())&0x3FFu);
            core->runFrame(core);
            for(int y=0;y<160;y++)for(int x=0;x<240;x++) {
                const uint32_t c=video[size_t(y)*240+x];
                top[size_t(y)*coop::Width+x]=0xFF000000u|((c&0xFFu)<<16)|(c&0xFF00u)|((c>>16)&0xFFu);
            }
            mapping.publish(top.data(),bottom.data());
            int available=qMin(2048,blip_samples_avail(left));
            if(available>0) {
                int count=blip_read_samples(left,samples.data(),available,1);
                blip_read_samples(right,samples.data()+1,count,1);
                if(audio) {
                    if(SDL_GetQueuedAudioSize(audio)>32768/5*4)SDL_ClearQueuedAudio(audio);
                    SDL_QueueAudio(audio,samples.data(),Uint32(count*4));
                }
            }
            if(++frames%60==0)flushSave();
            target+=frameNs;
            const auto remaining=target-clock.nsecsElapsed();
            if(remaining>0)QThread::usleep(static_cast<unsigned long>(remaining/1000));
            else if(remaining < -frameNs*4)target=clock.nsecsElapsed();
        }
        flushSave();
        fprintf(stdout,"GBA worker: stopped cleanly after %u frames; save flushed\n",frames);fflush(stdout);
        if(audio)SDL_CloseAudioDevice(audio);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 0;
    } catch(const std::exception& e) {
        if(audio)SDL_CloseAudioDevice(audio);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        fprintf(stderr,"[GBA] %s\n",e.what());return 1;
    }
}
