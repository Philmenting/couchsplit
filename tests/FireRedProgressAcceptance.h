// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Private-ROM acceptance helper, never installed or called by production.
// Caller fingerprints the exact German original and supplies TWO distinct cores
// restored from a private natural starter checkpoint, before the lab rival.
#include "../gba/FireRedProgress.h"
#include "../gba/FireRedWorld.h"
#include "../gba/FireRedRewards.h"
#include "FireRedSaveAcceptance.h"

namespace coop::firered::acceptance {
struct NativeParcelSceneProof {
    unsigned frames = 0, deliveries = 0;
    NativeSaveProof sourceSave, recipientSave;
};
inline NativeParcelSceneProof verifyNativeParcelScene(mCore* source, mCore* recipient,
                                                     const std::string& verifiedRomPath) {
    using save_detail::demand;
    demand(source && recipient && source != recipient, "Parcel scene requires two separate cores");
    for (auto* core : {source, recipient}) {
        demand(progressFieldReady(core), "Parcel fixture needs an idle starter checkpoint");
        const auto before = readProgress(core);
        demand(before.mapGroup == 4 && before.mapNum == 3 && before.var(VarLabScene) == 3 &&
               !before.flag(FlagPokedexObtained), "Parcel fixture must start before the lab rival");
        auto prepared = before;
        prepared.var(VarLabScene, 4); prepared.flag(0x258, true); prepared.flag(0x2D, true);
        writeProgressDelta(core, before, prepared);
        startStoryObjectRefresh(core, StoryEvent::LabRival, before.mapGroup, before.mapNum);
        for (unsigned f = 0; f < 30; ++f) { core->setKeys(core, 0); core->runFrame(core); }
        demand(progressFieldReady(core), "Native fixture rival cleanup did not finish");
    }
    NativeParcelSceneProof proof;
    StoryProgress progress({{source, recipient}});
    progress.event = [&](StoryEvent type, unsigned owner, bool) {
        demand(type == StoryEvent::Parcel && owner == 1, "Unexpected Parcel test milestone"); ++proof.deliveries;
    };
    progress.sample();
    // AUTHORED map entry, not a playthrough from Pallet to Viridian. The native
    // CB2_LoadMap runs the genuine map header/on-frame scene. No parcel item,
    // Mart/lab quest stage, script pointer or dialogue result is injected.
    const auto memory = readSaveMemory(source);
    inventoryWrite16(source, memory.block1, 3); inventoryWrite16(source, memory.block1 + 2, 6);
    source->busWrite8(source, memory.block1 + 4, 5); source->busWrite8(source, memory.block1 + 5, 3);
    source->busWrite8(source, 0x03003478, 0); source->busWrite32(source, 0x03003044, 0x0805673D);
    bool completed = false;
    for (unsigned f = 0; f < 15000; ++f) {
        source->setKeys(source, f % 30 == 0 ? 1 : 0); recipient->setKeys(recipient, 0);
        progress.sample(); source->runFrame(source); recipient->runFrame(recipient); progress.sample(); ++proof.frames;
        if (progressFieldReady(source) && readProgress(source).var(VarMartScene) == 1) { completed = true; break; }
    }
    source->setKeys(source, 0); recipient->setKeys(recipient, 0);
    demand(completed && proof.deliveries == 1, "Original native Parcel scene or shared delivery did not finish");
    for (auto* core : {source, recipient}) {
        const auto result = readProgress(core);
        demand(result.inventory.quantity(OaksParcelItem) == 1 && result.var(VarLabScene) == 5 && result.var(VarMartScene) == 1,
               "Original or recipient Parcel item/quest stage missing");
    }
    proof.sourceSave = verifyNativeSaveReload(source, verifiedRomPath);
    proof.recipientSave = verifyNativeSaveReload(recipient, verifiedRomPath);
    StoryProgress restarted({{source, recipient}}); restarted.sample(); restarted.sample();
    demand(readInventory(source).quantity(OaksParcelItem) == 1 && readInventory(recipient).quantity(OaksParcelItem) == 1,
           "Story restart duplicated the Parcel");
    return proof;
}
struct NativePokedexSceneProof {
    unsigned frames = 0, parcelDeliveries = 0, pokedexDeliveries = 0;
    unsigned sourceBalls = 0, recipientBalls = 0;
    NativeSaveProof sourceSave, recipientSave;
};
inline NativePokedexSceneProof verifyNativePokedexScene(mCore* source, mCore* recipient,
                                                       const std::string& verifiedRomPath) {
    using save_detail::demand;
    demand(source && recipient && source != recipient, "Pokedex scene requires two separate cores");
    // These are AUTHORED prerequisites, not evidence of playing to the Mart.
    // The actual Oak conversation, complete native cutscene, Pokedex/ball
    // grants, and subsequent native saves/reboots below are executed normally.
    for (auto* core : {source, recipient}) {
        demand(progressFieldReady(core), "Pokedex fixture needs an idle starter checkpoint");
        const auto before = readProgress(core);
        demand(before.mapGroup == 4 && before.mapNum == 3 && before.var(VarLabScene) == 3 &&
               !before.flag(FlagPokedexObtained), "Pokedex fixture must start before the lab rival");
        auto prepared = before;
        prepared.var(VarLabScene, 4); prepared.flag(0x258, true); prepared.flag(0x2D, true);
        writeProgressDelta(core, before, prepared);
        startStoryObjectRefresh(core, StoryEvent::LabRival, before.mapGroup, before.mapNum);
        for (unsigned f = 0; f < 30; ++f) { core->setKeys(core, 0); core->runFrame(core); }
        demand(progressFieldReady(core), "Native fixture rival cleanup did not finish");
    }
    auto before = readProgress(source), prepared = before;
    prepared.var(VarLabScene, 5); prepared.var(VarMartScene, 1);
    prepared.inventory.setQuantity(OaksParcelItem, 1);
    writeProgressDelta(source, before, prepared);
    before = readProgress(recipient); prepared = before;
    prepared.inventory.setQuantity(PokeBallItem, 7); prepared.inventory.setQuantity(13, 9);
    prepared.flag(0x600, true); // Authored unrelated personal trainer receipt.
    writeProgressDelta(recipient, before, prepared);
    NativePokedexSceneProof proof;
    StoryProgress progress({{source, recipient}});
    progress.event = [&](StoryEvent event, unsigned owner, bool) {
        demand(owner == 1, "Pokedex test unexpectedly changed the source's quest ownership");
        if (event == StoryEvent::Parcel) ++proof.parcelDeliveries;
        if (event == StoryEvent::Pokedex) ++proof.pokedexDeliveries;
    };
    progress.sample();
    demand(readProgress(recipient).inventory.quantity(OaksParcelItem) == 1,
           "Recipient did not receive their own prerequisite Parcel");
    auto frame = [&](unsigned key) {
        source->setKeys(source, key); recipient->setKeys(recipient, 0);
        progress.sample(); source->runFrame(source); recipient->runFrame(recipient); progress.sample();
    };
    // Natural checkpoint begins at (8,5). These held buttons walk to (6,4),
    // immediately below Oak. All following conversation presses are native A.
    auto step = [&](unsigned key) {
        for (unsigned f = 0; f < 12; ++f) frame(key);
        for (unsigned f = 0; f < 30; ++f) frame(0);
    };
    step(0x20); step(0x20); step(0x40);
    const auto actor = readWorldActor(source);
    demand(actor.valid && actor.tileX == 6 && actor.tileY == 4,
           "Native walking did not reach Oak; checkpoint is unsuitable");
    bool completed = false;
    for (unsigned f = 0; f < 20000; ++f) {
        frame(f % 30 == 0 ? 1 : 0); ++proof.frames;
        if (progressFieldReady(source) && readProgress(source).var(VarLabScene) == 6) { completed = true; break; }
    }
    demand(completed, "Original Oak Pokedex cutscene did not finish");
    for (unsigned f = 0; f < 30; ++f) frame(0);
    const auto original = readProgress(source), received = readProgress(recipient);
    proof.sourceBalls = original.inventory.quantity(PokeBallItem);
    proof.recipientBalls = received.inventory.quantity(PokeBallItem);
    demand(original.flag(FlagPokedexObtained) && received.flag(FlagPokedexObtained) &&
           original.var(VarLabScene) == 6 && received.var(VarLabScene) == 6,
           "Original cutscene or recipient quest stages are incomplete");
    demand(proof.sourceBalls == 5 && proof.recipientBalls == 12 &&
           received.inventory.quantity(13) == 9 && received.flag(0x600),
           "Quest reward replaced personal inventory or an unrelated flag");
    demand(received.inventory.quantity(OaksParcelItem) == 0 && (received.gcnLinkFlags & 0x31u) == 0x31u,
           "Recipient Parcel removal or native Pokedex link flags missing");
    demand(proof.parcelDeliveries == 1 && proof.pokedexDeliveries == 1,
           "Quest rewards were omitted or duplicated");
    for (auto* core : {source, recipient}) {
        const auto memory = readSaveMemory(core);
        demand((inventoryRead16(core, memory.block1 + 0x3A54) & 0xBu) == 0xAu,
               "Own native Oak FameChecker detail was not unlocked");
    }
    for (unsigned i = 0; i < 16; ++i) {
        const auto object = 0x02036E38 + i * 0x24;
        const auto id = recipient->busRead8(recipient, object + 8);
        demand(!(recipient->busRead8(recipient, object) & 1) || !(id == 8 || id == 9 || id == 10),
               "Native VM left a stale recipient rival/Pokedex object");
    }
    // The helper compares the audited persistent quest flags/stages/GCN bits,
    // personal bag/party/dex/PC/wallet after native save and fresh battery boot.
    proof.sourceSave = verifyNativeSaveReload(source, verifiedRomPath);
    proof.recipientSave = verifyNativeSaveReload(recipient, verifiedRomPath);
    // A new coordinator reconstructs pending work from native receipts. Neither
    // continuous sampling nor a session restart may duplicate either reward.
    StoryProgress restarted({{source, recipient}}); restarted.sample(); restarted.sample();
    demand(readInventory(source).quantity(PokeBallItem) == proof.sourceBalls &&
           readInventory(recipient).quantity(PokeBallItem) == proof.recipientBalls,
           "Story restart duplicated a Pokedex reward");
    return proof;
}

struct NativeBrockSceneProof {
    unsigned frames = 0, firstInputFrames = 0, secondInputFrames = 0, deliveries = 0;
    uint32_t originalPrize = 0, sourceMoney = 0, recipientMoney = 0;
    bool fullBagDeferred = false, laterNativeClaim = false;
    NativeSaveProof sourceSave, recipientSave, laterClaimSave;
};
inline PartyMon brockFixtureMon(unsigned personality, uint32_t trainerId) {
    PartyMon result;
    monPut32(result.bytes.data(), personality); monPut32(result.bytes.data() + 4, trainerId);
    result.bytes[8] = 0xBB; result.bytes[9] = 0xFF; result.bytes[18] = 5; result.bytes[19] = 2;
    result.bytes[20] = 0xBB; result.bytes[21] = 0xFF;
    PartyMon::Secure plain{};
    monPut16(plain.data(), 25); monPut32(plain.data() + 4, 125000); plain[9] = 70;
    monPut16(plain.data() + 12, 55); monPut16(plain.data() + 14, 45); plain[20] = 25; plain[21] = 40;
    monPut32(plain.data() + 40, 0x3FFFFFFF); result.setSecure(plain);
    result.bytes[84] = 50; monPut16(result.bytes.data() + 86, 150); monPut16(result.bytes.data() + 88, 150);
    for (unsigned offset = 90; offset < 100; offset += 2) monPut16(result.bytes.data() + offset, 120);
    result.validate(); return result;
}
// Pair is the production TeamBattle, supplied by the caller to avoid including
// the battle coordinator from this acceptance-only header. No native encounter
// flags, enemy party, trainer ID, battle callback or script result are authored.
template<class Pair>
inline NativeBrockSceneProof verifyNativeBrockScene(mCore* source, mCore* recipient, Pair& pair,
                                                   const std::string& verifiedRomPath, bool fullBag = false) {
    using save_detail::demand;
    demand(source && recipient && source != recipient, "Brock scene requires two separate cores");
    NativeBrockSceneProof proof;
    for (auto* core : {source, recipient}) {
        demand(progressFieldReady(core), "Brock fixture needs an idle starter checkpoint");
        const auto before = readProgress(core);
        demand(before.var(VarLabScene) == 3 && !before.flag(FlagBrockDefeated) && !before.flag(FlagBoulderBadge),
               "Brock fixture must start from the private natural starter checkpoint");
        // AUTHORED quest prerequisites, strong own-party fixture, wallets and
        // direct native Gym entry. The actual Brock dialogue/encounter and full
        // reward script subsequently execute through ordinary native controls.
        auto prepared = before;
        prepared.var(VarLabScene, 6); prepared.var(VarMartScene, 2);
        prepared.flag(0x258, true); prepared.flag(0x2D, true); prepared.flag(0x3A, true);
        prepared.flag(FlagPokedexObtained, true); prepared.gcnLinkFlags |= 0x31u;
        writeProgressDelta(core, before, prepared);
        const auto identity = readTrainerIdentity(core);
        Party party; party.count = 1;
        party.mons[0] = brockFixtureMon(core == source ? 24 : 48, identity.id); writeParty(core, party);
        writeWallet(core, core == source ? 1000 : 5000);
        if (core == recipient && fullBag) {
            auto bag = readInventory(core); bag.setQuantity(TMCaseItem, 1); bag.setQuantity(RockTombItem, 999); writeInventory(core, bag);
        }
        const auto memory = readSaveMemory(core);
        inventoryWrite16(core, memory.block1, 6); inventoryWrite16(core, memory.block1 + 2, 6);
        core->busWrite8(core, memory.block1 + 4, 6); core->busWrite8(core, memory.block1 + 5, 2);
        core->busWrite8(core, 0x03003478, 0); core->busWrite32(core, 0x03003044, 0x0805673D);
        for (unsigned f = 0; f < 240; ++f) { core->setKeys(core, 0); core->runFrame(core); }
        demand(progressFieldReady(core), "Native Brock Gym entry did not finish");
    }
    StoryProgress progress({{source, recipient}});
    progress.event = [&](StoryEvent type, unsigned owner, bool deferred) {
        demand(type == StoryEvent::Brock && owner == 1, "Unexpected Brock story delivery");
        ++proof.deliveries; proof.fullBagDeferred = deferred;
    };
    progress.sample();
    for (unsigned f = 0; f < 12; ++f) { source->setKeys(source, 0x40); source->runFrame(source); }
    for (unsigned f = 0; f < 30; ++f) { source->setKeys(source, 0); source->runFrame(source); }
    bool joined = false, completed = false;
    for (unsigned f = 0; f < 20000; ++f) {
        if (!pair.active()) {
            progress.sample(); pair.sampleWorld();
            if (inventoryRead32(source, 0x03003044) == 0x0800FD21) {
                demand(inventoryRead16(source, 0x020386AE) == 414, "Native conversation did not trigger Brock");
                demand(!joined && pair.tryBegin(0), "Native Brock encounter could not join");
                joined = true;
            }
        }
        if (pair.active()) {
            proof.firstInputFrames += pair.inputOwner() == 0;
            proof.secondInputFrames += pair.inputOwner() == 1;
            const auto pulse = f % 30 < 4 ? 1u : 0u;
            pair.runBattleFrame({{pulse, pulse}});
        } else {
            source->setKeys(source, f % 30 < 4 ? 1u : 0u); source->runFrame(source);
            recipient->setKeys(recipient, 0); recipient->runFrame(recipient); progress.sample();
        }
        ++proof.frames;
        if (joined && !pair.active() && progressFieldReady(source) && progressFieldReady(recipient) &&
            storyEventComplete(readProgress(source), StoryEvent::Brock) &&
            storyEventComplete(readProgress(recipient), StoryEvent::Brock)) { completed = true; break; }
    }
    source->setKeys(source, 0); recipient->setKeys(recipient, 0);
    demand(completed && proof.firstInputFrames && proof.secondInputFrames && proof.deliveries == 1,
           "Original Brock encounter/postbattle scene did not finish for both owners");
    const auto original = readProgress(source), received = readProgress(recipient);
    demand(original.inventory.quantity(RockTombItem) == 1 && original.flag(FlagBrockTM) &&
           original.inventory.quantity(TMCaseItem) == 1 && received.inventory.quantity(TMCaseItem) == 1,
           "Original or recipient native TM39/TM Case missing");
    demand(received.inventory.quantity(RockTombItem) == (fullBag ? 999u : 1u) && received.flag(FlagBrockTM) == !fullBag &&
           proof.fullBagDeferred == fullBag, "Recipient TM39 delivery/full-bag receipt incorrect");
    for (const auto* p : {&original, &received}) {
        demand(p->flag(FlagBoulderBadge) && p->flag(FlagBrockDefeated) && p->flag(0x500 + 414) && p->flag(0x500 + 142),
               "Brock badge or Gym trainer receipts missing");
        demand(p->flag(0x2E) && !p->flag(0x92) && p->var(0x406C) == 1, "Brock world/NPC progress missing");
    }
    for (auto* core : {source, recipient}) {
        const auto memory = readSaveMemory(core);
        demand((inventoryRead16(core, memory.block1 + 0x3A5C) & 0xBu) == 0xAu,
               "Own native Brock FameChecker color/detail was not unlocked");
    }
    // Read original native trainer metadata, never infer reward from synthetic
    // doubles or from capped wallet deltas. This fixture has no Amulet Coin.
    proof.originalPrize = nativeTrainerPrize(source, 414, 8, 1);
    proof.sourceMoney = readWallet(source); proof.recipientMoney = readWallet(recipient);
    const auto expected = applyBattleRewards({{1000,5000}}, splitBattleRewards(proof.originalPrize,0,0));
    demand(proof.sourceMoney == expected[0] && proof.recipientMoney == expected[1], "Brock reward was duplicated or allocated incorrectly");
    proof.sourceSave = verifyNativeSaveReload(source, verifiedRomPath);
    proof.recipientSave = verifyNativeSaveReload(recipient, verifiedRomPath);
    if (fullBag) {
        // AUTHORED capacity change, followed by the original native Brock
        // conversation. Receipt must remain unset across the preceding reboot.
        auto bag = readInventory(recipient); bag.setQuantity(RockTombItem, 998); writeInventory(recipient, bag);
        for (unsigned f = 0; f < 12; ++f) { recipient->setKeys(recipient, 0x40); recipient->runFrame(recipient); }
        for (unsigned f = 0; f < 3000; ++f) {
            recipient->setKeys(recipient, f % 30 < 4 ? 1u : 0u); recipient->runFrame(recipient);
            if (progressFieldReady(recipient) && readProgress(recipient).flag(FlagBrockTM)) { proof.laterNativeClaim = true; break; }
        }
        recipient->setKeys(recipient, 0);
        demand(proof.laterNativeClaim && readInventory(recipient).quantity(RockTombItem) == 999,
               "Native later TM39 claim did not deliver the deferred item");
        proof.laterClaimSave = verifyNativeSaveReload(recipient, verifiedRomPath);
    }
    StoryProgress restarted({{source, recipient}}); restarted.sample(); restarted.sample();
    demand(readInventory(source).quantity(RockTombItem) == 1 &&
           readInventory(recipient).quantity(RockTombItem) == (fullBag ? 999u : 1u), "Brock restart duplicated TM39");
    return proof;
}

} // namespace coop::firered::acceptance
