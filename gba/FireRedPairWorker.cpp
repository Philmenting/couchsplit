// SPDX-License-Identifier: GPL-3.0-or-later
#include "FireRedPairWorker.h"
#include "FireRedBattle.h"
#include "FireRedProgress.h"
#include "FireRedCampaignSync.h"
#include "FireRedCampaignCompletion.h"
#include "FireRedLeague.h"
#include "FireRedConnections.h"
#include "FireRedAudio.h"
#include "CoopIPC.h"
#include "Rom.h"
#include <mgba/core/config.h>
#include <mgba/core/blip_buf.h>
#include <mgba-util/vfs.h>
#include <QElapsedTimer>
#include <QThread>
#include <SDL.h>
#include <array>
#include <memory>

namespace {
struct PlayerCore {
    mCore* core = nullptr;
    bool initialized = false, configured = false;
    QString profile, savePath;
    QByteArray lastSave;
    std::array<color_t, 240 * 160> video{};
    std::array<uint32_t, coop::Pixels> top{}, bottom{};
    coop::Mapping mapping;
    ~PlayerCore() {
        if (!core) return;
        if (configured) mCoreConfigDeinit(&core->config);
        if (initialized) core->deinit(core); else free(core);
    }
    void preflight(const QString& profilePath, const QString& ipc) {
        const QFileInfo info(profilePath), directory(profilePath + "/saves");
        if (profilePath.isEmpty() || ipc.isEmpty() || !info.isDir() ||
            info.isSymLink() || !directory.isDir() || directory.isSymLink())
            coop::fail("Missing or unsafe FireRed profile, save directory or IPC");
        profile = info.canonicalFilePath();
        if (profile.isEmpty() || directory.canonicalFilePath() != profile + "/saves")
            coop::fail("Unsafe FireRed save directory");
        savePath = profile + "/saves/game.sav";
        if (QFileInfo(savePath).isSymLink()) coop::fail("Unsafe FireRed save path");
        if (QFileInfo::exists(savePath)) {
            QFile save(savePath);
            // The exact original FireRed uses 128 KiB Flash. Reject even a
            // different normally valid GBA size before either core starts.
            if (!save.open(QIODevice::ReadOnly) || !QFileInfo(savePath).isFile() || save.size() != 131072)
                coop::fail("Invalid FireRed battery save size; original preserved");
            lastSave = save.readAll();
            if (lastSave.size() != 131072 || save.error() != QFileDevice::NoError)
                coop::fail("Cannot read complete FireRed save; original preserved");
        }
        mapping.open(ipc.toUtf8().toStdString(), false);
    }
    void initialize(const QString& rom) {
        core = mCoreFind(rom.toUtf8().constData());
        if (!core || core->platform(core) != mPLATFORM_GBA || !core->init(core))
            coop::fail("mGBA FireRed pair initialization failed");
        initialized = true;
        mCoreInitConfig(core, nullptr); configured = true;
        mCoreOptions options{}; options.useBios = false; options.skipBios = true;
        options.sampleRate = 32768; options.audioBuffers = 1024; options.volume = 0x100;
        mCoreConfigLoadDefaults(&core->config, &options); mCoreLoadConfig(core);
        if (!mCoreLoadFile(core, rom.toUtf8().constData())) coop::fail("mGBA cannot load FireRed ROM");
        if (!lastSave.isEmpty()) {
            auto* file = VFileMemChunk(lastSave.constData(), size_t(lastSave.size()));
            if (!file) coop::fail("Cannot allocate FireRed save buffer");
            if (!core->loadSave(core, file)) { file->close(file); coop::fail("Cannot load FireRed save; original preserved"); }
        }
        core->setVideoBuffer(core, video.data(), 240);
        core->reset(core);
        void* raw = nullptr;
        const auto size = core->savedataClone(core, &raw); free(raw);
        if (size != 131072) coop::fail("Unexpected FireRed save type; original preserved");
        for (unsigned channel = 0; channel < 2; ++channel)
            blip_set_rates(core->getAudioChannel(core, channel), core->frequency(core), 32768);
    }
    void flushSave() {
        void* raw = nullptr;
        const auto size = core->savedataClone(core, &raw);
        std::unique_ptr<void, decltype(&free)> buffer(raw, free);
        if (!raw || size != 131072) coop::fail("Unexpected FireRed save size; previous save preserved");
        const QByteArray bytes(static_cast<const char*>(raw), qsizetype(size));
        if (bytes != lastSave) { coop::atomicWrite(savePath, bytes); lastSave = bytes; }
    }
    void render(const PlayerCore& source) {
        for (unsigned y = 0; y < 160; ++y) for (unsigned x = 0; x < 240; ++x) {
            const uint32_t color = source.video[y * 240 + x];
            top[y * coop::Width + x] = 0xFF000000u | ((color & 255u) << 16) |
                (color & 0xFF00u) | ((color >> 16) & 255u);
        }
    }
};
struct Audio {
    SDL_AudioDeviceID device = 0;
    coop::firered::AudioMode mode = coop::firered::AudioMode::Player1;
    Audio() {
        mode=coop::firered::parseAudioMode(qEnvironmentVariable("PMCOOP_FIRERED_AUDIO").toStdString());
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return;
        SDL_AudioSpec spec{}; spec.freq = 32768; spec.format = AUDIO_S16SYS;
        spec.channels = 2; spec.samples = 1024;
        device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
        if (device) SDL_PauseAudioDevice(device, 0);
    }
    ~Audio() { if (device) SDL_CloseAudioDevice(device); SDL_QuitSubSystem(SDL_INIT_AUDIO); }
    coop::firered::AudioBlock drain(mCore* core) {
        auto* left = core->getAudioChannel(core, 0); auto* right = core->getAudioChannel(core, 1);
        coop::firered::AudioBlock block;
        int available = qMin(2048, qMin(blip_samples_avail(left), blip_samples_avail(right)));
        if (available <= 0) return block;
        const int leftCount = blip_read_samples(left, block.samples.data(), available, 1);
        const int rightCount = blip_read_samples(right, block.samples.data() + 1, leftCount, 1);
        block.frames=unsigned(qMax(0,qMin(leftCount,rightCount)));
        return block;
    }
    void play(const coop::firered::AudioBlock& first,const coop::firered::AudioBlock& second,
              bool sharedBattle,unsigned leader) {
        const auto output=coop::firered::mixAudio(first,second,mode,sharedBattle,leader);
        if (device && output.frames) {
            if (SDL_GetQueuedAudioSize(device) > 32768 / 5 * 4) SDL_ClearQueuedAudio(device);
            SDL_QueueAudio(device, output.samples.data(), Uint32(output.frames * 4));
        }
    }
};
}

int runFireRedPairWorker(const QString& rom, volatile std::sig_atomic_t& quitRequested) {
    // Fingerprint before creating cores or allowing any game-memory writes.
    const auto info = coop::inspectRom(rom);
    if (info.system != coop::GameSystem::GameBoyAdvance || info.sha256 != coop::GermanFireRedSha256)
        coop::fail("FireRed paired experiment requires the exact verified German original ROM");
    std::array<PlayerCore, 2> players;
    const std::array<QString, 2> ipcs{{qEnvironmentVariable("PMCOOP_IPC"), qEnvironmentVariable("PMCOOP_IPC2")}};
    if (QFileInfo(ipcs[0]).canonicalFilePath().isEmpty() ||
        QFileInfo(ipcs[0]).canonicalFilePath() == QFileInfo(ipcs[1]).canonicalFilePath())
        coop::fail("FireRed pair requires two distinct IPC mappings");
    players[0].preflight(qEnvironmentVariable("PMCOOP_PROFILE"), ipcs[0]);
    players[1].preflight(qEnvironmentVariable("PMCOOP_PROFILE2"), ipcs[1]);
    if (players[0].profile == players[1].profile) coop::fail("FireRed pair requires two distinct save profiles");
    for (auto& player : players) player.initialize(rom);
    coop::firered::TeamBattle battle({{players[0].core, players[1].core}});
    std::array<bool,2> hallOfFamePending{};
    std::array<unsigned,2> leagueVictoryPending{};
    battle.completed = [&](unsigned leader,unsigned trainer,unsigned outcome,bool peerHasNativeScene) {
        if(outcome==1 && !peerHasNativeScene &&
           ((trainer>=438 && trainer<=440) || (trainer>=739 && trainer<=741)))
            hallOfFamePending[1-leader]=true;
        if(outcome==1 && !peerHasNativeScene && coop::firered::leagueTrainerRoom(trainer)>=0)
            leagueVictoryPending[1-leader]=trainer;
    };
    battle.event = [](const char* event, int value) {
        fprintf(stdout, "FireRed pair: %s (%d)\n", event, value); fflush(stdout);
    };
    coop::firered::StoryProgress progress({{players[0].core, players[1].core}});
    coop::firered::CampaignProgress campaign({{players[0].core, players[1].core}});
    campaign.event = [](unsigned recipient,const coop::firered::CampaignApplication& applied) {
        fprintf(stdout,"FireRed pair: campaign transaction player %u (%zu flags, %zu stages, %zu own rewards)%s%s\n",
            recipient+1,applied.changedFlags.size(),applied.changedVars.size(),applied.deliveredReceipts.size(),
            applied.deferredRewards ? "; own bag reward pending" : "",
            applied.enableNationalDex ? "; native National Dex upgrade" : "");
        fflush(stdout);
    };
    campaign.deferred = [](unsigned recipient,const std::string& reason) {
        fprintf(stdout,"FireRed pair: campaign delivery waits for player %u: %s\n",recipient+1,reason.c_str());fflush(stdout);
    };
    auto finishChampion = [&] {
        for(unsigned owner=0;owner<2;++owner) if(leagueVictoryPending[owner]) {
            auto* c=players[owner].core;
            if(!coop::firered::campaignFieldQuiescent(c)) continue;
            coop::firered::requirePokemon(coop::firered::startNativeLeagueVictory(c,leagueVictoryPending[owner]),
                "Cannot complete the partner's own Elite Four room; previous saves preserved");
            leagueVictoryPending[owner]=0;
            fprintf(stdout,"FireRed pair: player %u receives their own native League exit\n",owner+1);fflush(stdout);
        }
        for(unsigned owner=0;owner<2;++owner) if(hallOfFamePending[owner]) {
            auto* c=players[owner].core;
            if(!coop::firered::progressFieldReady(c)) continue;
            coop::firered::requirePokemon(coop::firered::startNativeHallOfFameCompletion(c),
                "Cannot start the partner's own native Hall of Fame; previous saves preserved");
            hallOfFamePending[owner]=false;
            fprintf(stdout,"FireRed pair: player %u enters their own native Hall of Fame\n",owner+1);fflush(stdout);
        }
    };
    progress.event = [](coop::firered::StoryEvent event, unsigned recipient, bool deferredItem) {
        const char* name = "unknown";
        switch (event) {
        case coop::firered::StoryEvent::Parcel: name = "Oak's Parcel"; break;
        case coop::firered::StoryEvent::Pokedex: name = "Pokedex and five Poke Balls"; break;
        case coop::firered::StoryEvent::Brock: name = "Brock, Boulder Badge and own TM39"; break;
        case coop::firered::StoryEvent::LabRival: name = "initial rival scene and own party heal"; break;
        }
        fprintf(stdout, "FireRed pair: reviewed story milestone for player %u: %s%s\n", recipient + 1, name,
                deferredItem ? " (TM delivery deferred: own bag is full; claim it from Brock)" : "");
        fflush(stdout);
    };
    Audio audio;
    fprintf(stdout,"FireRed pair: audio mode %s\n",coop::firered::audioModeName(audio.mode));fflush(stdout);
    QElapsedTimer clock; clock.start(); qint64 target = 0;
    const auto* timing = players[0].core;
    const qint64 frameNs = qRound64(1e9 * double(timing->frameCycles(players[0].core)) / timing->frequency(players[0].core));
    unsigned frames = 0;
    fprintf(stdout, "FireRed pair: EXPERIMENTAL; two cores, separate saves; team transaction coordinator enabled\n"); fflush(stdout);
    try {
        while (!quitRequested && !players[0].mapping.data->shutdown.load() && !players[1].mapping.data->shutdown.load()) {
            std::array<uint32_t, 2> keys{};
            for (unsigned i = 0; i < 2; ++i) {
                keys[i] = (~players[i].mapping.data->keys.load()) & 0x3FFu;
                if(hallOfFamePending[i] || leagueVictoryPending[i]) keys[i]=0;
            }
            // Deliver a pending initial-rival heal before exhaustion recovery.
            // Native scripts and menus retain ownership until they finish.
            if (!battle.active()) { finishChampion(); progress.sample(); campaign.sample(); }
            battle.sampleWorld();
            if (!battle.active()) {
                for (unsigned i = 0; i < 2; ++i) {
                    if (!hallOfFamePending[i] && !leagueVictoryPending[i] && coop::firered::recoverExhaustedTrainer(players[i].core)) {
                        fprintf(stdout, "FireRed pair: player %u returns to their native heal location\n", i + 1); fflush(stdout);
                    }
                    // Observe the native InitBattle callback before that core
                    // runs it. A newly joined partner must remain frozen.
                    if (battle.tryBegin(i)) break;
                    // Keep the encounter at its original native entry while
                    // the same-map partner closes a menu/finishes a scene.
                    // The other core continues; no timeout forces solo play.
                    if (battle.waitingForPartner(i)) continue;
                    // Finish the peer's current step before joining. Menus
                    // and dialogue keep receiving that player's own buttons.
                    if(coop::firered::progressFieldReady(players[i].core) &&
                       players[i].core->busRead8(players[i].core,coop::firered::PlayerPartyCountAddress) &&
                       battle.waitingForPartner(1-i)) {
                        const auto a=coop::firered::readSaveMemory(players[i].core);
                        const auto b=coop::firered::readSaveMemory(players[1-i].core);
                        if(players[i].core->busRead8(players[i].core,a.block1+4)==players[1-i].core->busRead8(players[1-i].core,b.block1+4) &&
                           players[i].core->busRead8(players[i].core,a.block1+5)==players[1-i].core->busRead8(players[1-i].core,b.block1+5)) keys[i]=0;
                    }
                    players[i].core->setKeys(players[i].core, keys[i]);
                    players[i].core->runFrame(players[i].core);
                    if (battle.tryBegin(i)) break;
                }
            }
            const bool sharedBattleFrame = battle.active();
            const unsigned leader = battle.battleLeader();
            if (sharedBattleFrame) battle.runBattleFrame(keys);
            if (!battle.active()) { finishChampion(); progress.sample(); campaign.sample(); }
            int waitingFor=-1;
            if(!battle.active()) for(unsigned i=0;i<2;++i)
                if(battle.waitingForPartner(i)) { waitingFor=int(1-i);break; }
            battle.sampleWorld();
            const auto& actors = battle.worldActors();
            const std::array<coop::firered::WorldTopology,2> topology{{
                coop::firered::readWorldTopology(players[0].core,actors[0]),
                coop::firered::readWorldTopology(players[1].core,actors[1])}};
            for (unsigned i = 0; i < 2; ++i) {
                auto& player = players[i];
                player.render(sharedBattleFrame ? players[leader] : player);
                if (!sharedBattleFrame) {
                    const auto partner=coop::firered::projectConnectedPartner(actors[i],actors[1-i],topology[i],topology[1-i]);
                    if(partner) coop::firered::compositePartner(player.top.data(),coop::Width,actors[i],*partner);
                }
                // Paired FireRed only: 0 field, 1 shared battle, 2 waiting.
                player.mapping.data->bridgeActive.store(battle.active() ? 1u : waitingFor>=0 ? 2u : 0u);
                player.mapping.data->peers.store(actors[0].valid && actors[1].valid && actors[0].mapGroup == actors[1].mapGroup && actors[0].mapNum == actors[1].mapNum ? 1u : 0u);
                player.mapping.data->role.store(waitingFor>=0 ? unsigned(waitingFor)+1 : battle.inputOwner() < 0 ? 0u : unsigned(battle.inputOwner()) + 1);
                player.mapping.publish(player.top.data(), player.bottom.data());
            }
            // Drain both cores, including a muted/frozen one, so no stale
            // overworld audio plays after a shared battle or a mode change.
            const auto firstAudio=audio.drain(players[0].core);
            const auto secondAudio=audio.drain(players[1].core);
            audio.play(firstAudio,secondAudio,sharedBattleFrame,leader);
            // A synthetic combined party must never reach either battery file.
            if (++frames % 60 == 0 && !battle.active()) for (auto& player : players) player.flushSave();
            target += frameNs;
            const auto remaining = target - clock.nsecsElapsed();
            if (remaining > 0) QThread::usleep(static_cast<unsigned long>(remaining / 1000));
            else if (remaining < -frameNs * 4) target = clock.nsecsElapsed();
        }
        battle.rollback();
        for (auto& player : players) { player.core->setKeys(player.core, 0); player.flushSave(); }
    } catch (...) {
        // Transaction errors stop both contexts. Even after a successful
        // rollback, leave the last validated on-disk batteries untouched.
        battle.rollback();
        throw;
    }
    fprintf(stdout, "FireRed pair: stopped cleanly after %u paired frames; separate saves flushed\n", frames); fflush(stdout);
    return 0;
}
