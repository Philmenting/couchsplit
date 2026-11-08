// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedInventory.h"
#include "FireRedStorage.h"
#include "FireRedCapture.h"
#include "FireRedBench.h"
#include "FireRedExperience.h"
#include "FireRedRewards.h"
#include "FireRedBattleHooks.h"
#include "FireRedDex.h"
#include "FireRedPersonal.h"
#include "FireRedWorld.h"
#include "FireRedProgress.h"
#include "FireRedStatistics.h"
#include "FireRedEncounter.h"
#include "FireRedCampaignRuntime.h"
#include <functional>
#include <memory>
#include <string>

namespace coop::firered {

inline constexpr uint32_t MainCallbackAddress = 0x03003044;
inline constexpr uint32_t SavedCallbackAddress = 0x03003048;
inline constexpr uint32_t InitBattleCallback = 0x0800FD21;
inline constexpr uint32_t OverworldCallback = 0x080565D5;
inline constexpr uint32_t BattleFlagsAddress = 0x02022B4C;
inline constexpr uint32_t ControllerExecAddress = 0x02023BC8;
inline constexpr uint32_t ControllerBufferA = 0x02022BC4;
inline constexpr uint32_t ControllerBufferB = 0x020233C4;
inline constexpr uint32_t OutcomeAddress = 0x02023E8A;
inline constexpr uint32_t TrainerAddress = 0x020386AE;

inline bool recoverExhaustedTrainer(mCore* core) {
    // Let native post-battle story scripts finish first. Once normal field
    // control resumes, a trainer without a usable Pokemon takes the game's
    // own blackout path; otherwise the next encounter could start an empty
    // party battle. Read only unencrypted HP/egg fields at this live boundary.
    if (core->busRead32(core, MainCallbackAddress) != OverworldCallback ||
        core->busRead8(core, 0x03000F9C) || (core->busRead8(core, 0x03003479) & 2u)) return false;
    const auto count = core->busRead8(core, PlayerPartyCountAddress);
    requirePokemon(count <= 6, "Invalid FireRed field party count");
    if (!count) return false;
    for (unsigned slot = 0; slot < count; ++slot) {
        const auto mon = PlayerPartyAddress + slot * PartyMonSize;
        if (!(core->busRead8(core, mon + 19) & 4u) && core->busRead16(core, mon + 86)) return false;
    }
    core->busWrite32(core, MainCallbackAddress, 0x080566C5);
    core->busWrite8(core, 0x03003478, 0);
    return true;
}

// Shared native battle, with two real emulator/save contexts. This coordinator
// is deliberately independent of IPC and controller polling. Only its caller
// may advance the paused cores. It never opens or writes a battery save.
// Fingerprint BOTH cores before constructing it. Native link, tutorials, Safari
// and facilities are not converted into ordinary doubles. Reviewed scripted
// wild/Marowak/legendary kinds retain their native type and return script.
class TeamBattle {
    std::array<mCore*, 2> cores;
    std::array<std::vector<uint8_t>, 2> rollbackStates;
    std::array<std::vector<uint8_t>, 2> rollbackBatteries;
    BattleParty transaction;
    std::array<Party,2> initialParties;
    std::unique_ptr<BattleExperience> experience;
    std::unique_ptr<BattleHooks> hooks;
    std::array<Inventory, 2> bags;
    Inventory activeBag;
    std::array<uint32_t, 2> wallets{};
    BattleStatistics joiningStatistics;
    std::array<StorageSnapshot, 2> storage;
    std::array<TrainerIdentity, 2> trainers;
    std::array<PersonalCaptureState, 2> personal;
    std::optional<PartyMon> captureBeforeGift;
    std::array<WorldActor, 2> actors{};
    uint32_t returnCallback = 0, originalFlags = 0;
    uint16_t trainer = 0;
    unsigned leader = 0, bagOwner = 0, lastTextOwner = 0;
    bool battling = false, sawNativeBattle = false;
    bool shareTrainerWin = false, firstLabRival = false;
    bool singleWildBattle = false;
    bool singleTrainerBattle = false;
    bool peerWaitingEncounter = false;
    uint32_t peerReturnCallback = 0;
    unsigned battleFrames = 0;
    NativeRewardState rewardState{};
    int catcher = -1;
    std::optional<CaptureStagingPlan> captureStaging;
    std::array<std::array<unsigned, 13>, 2> initialBalls{}, executedBalls{};
    std::array<std::optional<std::pair<unsigned, uint32_t>>, 6> encountered{};
    int currentOwner = -1, evolutionOwner = -1;
    struct NativePartyMenu {
        PartyMenuSession session;
        uint32_t flags=0;
        unsigned battler=0;
        std::array<uint8_t,7> request{};
    };
    std::optional<NativePartyMenu> ownPartyMenu;
    std::optional<std::pair<unsigned,unsigned>> ownPartyMenuReply;
    void beginOwnPartyMenu(unsigned battler) {
        requirePokemon((battler==0 || battler==2) && !ownPartyMenu && !ownPartyMenuReply,
                       "Conflicting FireRed own-party menu request");
        auto* c=cores[leader];NativePartyMenu view;view.battler=battler;
        experience->observeBattle();
        const auto buffer=ControllerBufferA+battler*0x200;
        for(unsigned i=0;i<view.request.size();++i)view.request[i]=uint8_t(c->busRead8(c,buffer+i));
        requirePokemon(view.request[0]==22,"Unexpected native own-party menu command");
        const std::array<uint8_t,3> order{{view.request[4],view.request[5],view.request[6]}};
        view.session=planPartyMenu(transaction,readParty(c),battler/2,readBenchReferences(c,6,6),order);
        view.flags=c->busRead32(c,BattleFlagsAddress);ownPartyMenu=view;
        writeParty(c,view.session.menu);
        // Native menu-only single layout: the complete own roster fills all six
        // entries; partner party/active records remain in the saved working set.
        c->busWrite32(c,BattleFlagsAddress,view.flags&~1u);
        for(unsigned owner=0;owner<2;++owner)
            inventoryWrite16(c,0x02023BCE + owner*4,uint16_t(view.session.virtualActive[owner]));
        c->busWrite8(c,buffer+2,6); // Other human's queued choice is not an own original slot.
        for(unsigned i=0;i<3;++i) {
            c->busWrite8(c,buffer+4+i,IdentityPartyMenuOrder[i]);
            c->busWrite8(c,NativePartyMenuOrderAddress+i,IdentityPartyMenuOrder[i]);
        }
        c->busWrite8(c,NativePartyMenuExitChosenAddress,0);
        if(event)event("own party menu opened",int(battler/2));
    }
    void finishOwnPartyMenu() {
        if(!ownPartyMenu)return; // Bag/item menus also use the native reshow callback.
        auto* c=cores[leader];const auto view=*ownPartyMenu;
        const auto chosen=c->busRead8(c,NativePartyMenuExitChosenAddress);
        requirePokemon(chosen<=1,"Invalid FireRed native own-party menu completion flag");
        const unsigned original=chosen?c->busRead8(c,NativePartyMenuSelectedAddress):6u;
        const auto picked=planPartyMenuSelection(view.session,readParty(c),original,true);
        // Restore physical battle ownership BEFORE the native reshow callback,
        // including when a protected reference forces a full battle rollback.
        writeParty(c,picked.party);
        c->busWrite32(c,BattleFlagsAddress,view.flags);
        for(unsigned owner=0;owner<2;++owner)
            inventoryWrite16(c,0x02023BCE + owner*4,uint16_t(view.session.references.active[owner]));
        const auto battle=inventoryRead32(c,0x02023FE8);
        requirePokemon(validSaveBlock(battle,0xB0),"Invalid FireRed menu return battle structure");
        for(unsigned owner=0;owner<2;++owner)
            c->busWrite8(c,battle+0x5C+owner*2,uint8_t(view.session.references.queued[owner]));
        c->busWrite8(c,battle+0x8B,view.request[2]);
        const auto buffer=ControllerBufferA+view.battler*0x200;
        for(unsigned i=0;i<view.request.size();++i)c->busWrite8(c,buffer+i,view.request[i]);
        for(unsigned i=0;i<3;++i)c->busWrite8(c,NativePartyMenuOrderAddress+i,view.session.savedNativeOrder[i]);
        for(unsigned enemy=0;enemy<2;++enemy)c->busWrite8(c,0x02023F4E + enemy,picked.references.participation[enemy]);
        c->busWrite8(c,0x02023F49,picked.references.knockedOff);
        ownPartyMenu.reset();
        if(!picked.accepted)throw std::runtime_error("FireRed own reserve is protected by pending native battle references: "+picked.reason);
        transaction=picked.transaction;
        experience->projectReferences();
        c->busWrite8(c,NativePartyMenuSelectedAddress,uint8_t(picked.nativeSlot));
        ownPartyMenuReply=std::make_pair(view.battler,picked.nativeSlot);
        if(event)event(picked.canceled?"own party menu canceled":picked.imported?"own reserve selected":"own resident selected",
                       int(view.session.owner*6+(picked.canceled?0:picked.selectedOriginal)));
    }
    std::optional<unsigned> normalizeOwnPartyMenuReply(unsigned selected) {
        if(!ownPartyMenuReply)return std::nullopt;
        auto* c=cores[leader];
        requirePokemon(!ownPartyMenu && c->busRead8(c,0x02023BC4)==ownPartyMenuReply->first &&
                       selected==ownPartyMenuReply->second,"Unexpected FireRed native own-party reply");
        const auto slot=ownPartyMenuReply->second;ownPartyMenuReply.reset();return slot;
    }


    static std::vector<uint8_t> saveState(mCore* core) {
        std::vector<uint8_t> bytes(core->stateSize(core));
        requirePokemon(!bytes.empty() && core->saveState(core, bytes.data()), "Cannot snapshot FireRed battle transaction");
        return bytes;
    }
    static std::vector<uint8_t> saveBattery(mCore* core) {
        void* raw = nullptr;
        const auto size = core->savedataClone(core, &raw);
        std::unique_ptr<void, decltype(&free)> allocation(raw, free);
        requirePokemon(raw && size == 131072, "Cannot snapshot FireRed battery buffer");
        const auto* bytes = static_cast<const uint8_t*>(raw);
        std::vector<uint8_t> result(bytes, bytes + size);
        return result;
    }
    int ownerOfSlot(unsigned slot) const {
        if(experience && experience->evolutionOwner()) {
            const auto owner=*experience->evolutionOwner();
            requirePokemon(slot<cores[leader]->busRead8(cores[leader],PlayerPartyCountAddress),"Invalid FireRed evolution party slot");
            return int(owner);
        }
        requirePokemon(slot < transaction.combined.count, "Invalid FireRed battle party selection");
        auto* c = cores[leader];
        const auto address = PlayerPartyAddress + slot * PartyMonSize;
        const MonIdentity identity{inventoryRead32(c, address), inventoryRead32(c, address + 4)};
        for (unsigned original = 0; original < transaction.combined.count; ++original)
            if (transaction.combined.mons[original].identity() == identity) return transaction.owners[original];
        throw std::runtime_error("Foreign FireRed Pokemon appeared in the combined party");
    }
    void refillOwnerBench(unsigned owner, unsigned excludedOne = 6, unsigned excludedTwo = 6) {
        auto* c = cores[leader];
        experience->observeBattle();
        const auto references = readBenchReferences(c, excludedOne, excludedTwo);
        const auto current = readParty(c);
        if (auto plan = planBenchRefill(transaction, current, owner, references)) {
            applyBenchRefill(c, *plan);
            transaction = std::move(plan->transaction);
            experience->projectReferences();
            if (event) event("owner bench replenished", int(owner));
        } else {
            // Never declare defeat while a protected native slot prevents a
            // healthy original reserve from entering the six-slot working set.
            requirePokemon(!wouldAbandonLivingReserve(transaction, current, owner),
                "FireRed reserve is blocked by native battle references; rolling back the experimental battle");
        }
    }
    bool ownerHasNoReplacement(unsigned battler, unsigned excludedOne, unsigned excludedTwo) {
        requirePokemon(battler == 0 || battler == 2, "Invalid allied FireRed replacement request");
        refillOwnerBench(battler / 2, excludedOne, excludedTwo);
        auto* c = cores[leader];
        if (excludedOne == 6) excludedOne = c->busRead16(c, 0x02023BCE);
        if (excludedTwo == 6) excludedTwo = c->busRead16(c, 0x02023BD2);
        const auto battle = inventoryRead32(c, 0x02023FE8);
        requirePokemon(validSaveBlock(battle, 0x60), "Invalid FireRed replacement battle context");
        const auto chosenOne = c->busRead8(c, battle + 0x5C), chosenTwo = c->busRead8(c, battle + 0x5E);
        const auto party = readParty(c);
        for (unsigned slot = 0; slot < party.count; ++slot)
            if (party.mons[slot].viable() && ownerOfSlot(slot) == int(battler / 2) &&
                slot != excludedOne && slot != excludedTwo && slot != chosenOne && slot != chosenTwo) return false;
        return true;
    }
    void swapBag(unsigned owner) {
        if (owner == bagOwner) return;
        bags[bagOwner] = mergeBattleInventory(bags[bagOwner], activeBag, readInventory(cores[leader]));
        // Keep the leader's key-item and TM pockets. They cannot be consumed in
        // an ordinary battle; this prevents another trainer's story inventory
        // from leaking into their save or their native menu state.
        auto next = readInventory(cores[leader]);
        for (unsigned p : {unsigned(Items), unsigned(PokeBalls), unsigned(Berries)}) next.pockets[p] = bags[owner].pockets[p];
        writeInventory(cores[leader], next, BattlePockets);
        activeBag = next;
        bagOwner = owner;
    }
    void finish() {
        if (hooks) hooks->check();
        requirePokemon(!ownPartyMenu && !ownPartyMenuReply,"FireRed battle ended inside a temporary own-party menu");
        requirePokemon(!experience->temporaryParty(),"FireRed battle ended with an unfinished experience transaction");
        auto* core = cores[leader];
        const auto outcome = core->busRead8(core, OutcomeAddress);
        requirePokemon(outcome >= 1 && outcome <= 7, "Unsupported FireRed battle outcome");
        auto battleParty = readParty(core);
        std::optional<PartyMon> captured;
        auto boxes = storage;
        const auto nativeBoxes = readStorage(core);
        const auto pcCapture = captureStaging ? recoverCaptureStaging(*captureStaging, nativeBoxes).captured :
                                               captureAddition(storage[leader], nativeBoxes);
        if (outcome == 7) {
            requirePokemon(catcher == 0 || catcher == 1, "FireRed capture has no controller owner");
            if (battleParty.count == transaction.combined.count + 1) {
                requirePokemon(!pcCapture, "FireRed capture appeared in both party and PC");
                captured = battleParty.mons[--battleParty.count];
                battleParty.mons[battleParty.count] = {};
            } else {
                requirePokemon(bool(pcCapture), "FireRed capture appeared in neither party nor PC");
                // Native PC storage strips the20 party bytes. Recover these
                // from the actual caught opponent, never from invented stats.
                for (unsigned slot = 0; slot < 6; ++slot) {
                    PartyMon candidate;
                    for (unsigned b = 0; b < PartyMonSize; ++b)
                        candidate.bytes[b] = uint8_t(core->busRead8(core, 0x0202402C + slot * PartyMonSize + b));
                    if (candidate.identity().personality != pcCapture->pokemon.identity().personality || candidate.empty()) continue;
                    requirePokemon(!captured, "Ambiguous native captured Pokemon personality");
                    std::copy(pcCapture->pokemon.bytes.begin(), pcCapture->pokemon.bytes.end(), candidate.bytes.begin());
                    candidate.validate(); captured = candidate;
                }
                requirePokemon(bool(captured), "Cannot recover native captured Pokemon party stats");
            }
            requirePokemon(bool(captureBeforeGift), "FireRed capture has no native pre-gift snapshot");
            if (transaction.originals[unsigned(catcher)].count < PartyCapacity)
                *captured = preservePartyCapturePP(*captured, *captureBeforeGift);
            *captured = assignCaptureTrainer(*captured, trainers[unsigned(catcher)]);
        } else requirePokemon(!pcCapture, "FireRed battle unexpectedly changed PC storage");
        auto results = restoreParties(transaction, battleParty);
        auto personalResults=personal;
        if (captured) {
            const auto owner = unsigned(catcher);
            if (results[owner].count < 6) results[owner].mons[results[owner].count++] = *captured;
            else {
                const auto addition=addCapture(boxes[owner], BoxMon::fromParty(*captured));
                boxes[owner]=addition.first;
                personalResults[owner]=planPersonalCapture(personal[owner],addition.second.box,storage[owner].currentBox());
            }
            if (transaction.originals[owner].count < PartyCapacity)
                personalResults[owner]=planPersonalCapture(personal[owner],std::nullopt,storage[owner].currentBox());
        }
        bags[bagOwner] = mergeBattleInventory(bags[bagOwner], activeBag, readInventory(core));
        for (unsigned owner = 0; owner < 2; ++owner) for (unsigned item = 1; item <= 12; ++item) {
            // Native bag selection consumes a ball before turn order runs. If
            // an earlier capture ends the battle, refund the still-queued ball
            // to its owner; only actually executed native throws spend balls.
            requirePokemon(bags[owner].quantity(item) <= initialBalls[owner][item], "FireRed battle unexpectedly gained a ball");
            const auto spent = initialBalls[owner][item] - bags[owner].quantity(item);
            requirePokemon(executedBalls[owner][item] <= spent, "FireRed executed a ball absent from its owner's bag");
            bags[owner].setQuantity(item, initialBalls[owner][item] - executedBalls[owner][item]);
        }
        // Prepare/validate everything before the first commit to either core.
        for (const auto& result : results) result.validate();
        for (const auto& bag : bags) bag.validate();
        for (const auto& box : boxes) box.validate();
        std::array<uint32_t, 2> money = wallets;
        if (outcome == 1) {
            // Sample the uncapped components before native battle resources
            // are freed. A capped leader wallet must not reduce the guest's
            // share, and synthetic-double normalization must not halve Pay Day.
            const auto prize = nativeTrainerPrize(core, trainer, originalFlags & ~0x10u, rewardState.moneyMultiplier);
            money = applyBattleRewards(wallets, splitBattleRewards(prize, payDayReward(rewardState), leader));
        }
        for (unsigned player = 0; player < 2; ++player) {
            writeParty(cores[player], results[player]);
            writeInventory(cores[player], bags[player]);
            writeWallet(cores[player], money[player]);
            writeStorage(cores[player], boxes[player]);
            // Undo the leader's native capture/dex side effects, then replay
            // seen encounters and the actual owner's catch in native order.
            // Restoring seen copies as well as PID is essential for a first
            // special-form encounter to record its proper personal form.
            writePersonalCapture(cores[player],personalResults[player]);
            for (const auto& seen : encountered) if (seen) markSeen(cores[player], seen->first, seen->second);
            // Evolution executes on the leader's native engine. Its personal
            // dex writes were reset above; grant ownership to the actual holder.
            for (unsigned owner = 0; owner < 2; ++owner)
                for (unsigned slot = 0; slot < initialParties[owner].count; ++slot) {
                    const auto& before = initialParties[owner].mons[slot];
                    const auto& after = results[owner].mons[slot];
                    if (before.species() == after.species()) continue;
                    if (player == owner) markCaught(cores[player], after.species(), after.identity().personality);
                    else markSeen(cores[player], after.species(), after.identity().personality);
                }
            if (captured && int(player)==catcher)
                markCaught(cores[player],captured->species(),captured->identity().personality);
        }
        // BattleSetup counted the initiator before InitBattle. The paused
        // partner has not run that path: record just their participation once.
        if (!peerWaitingEncounter)
            writeBattleStatistics(cores[1-leader],recordSharedBattle(joiningStatistics,(originalFlags&8u)!=0));
        if (outcome == 1 && (originalFlags & 8u) && shareTrainerWin) {
            requirePokemon(trainer < 768, "FireRed trainer flag is out of range");
            for (auto* c : cores) {
                const auto save = readSaveMemory(c);
                const unsigned flag = 0x500u + trainer;
                const auto address = save.block1 + 0xEE0 + flag / 8;
                c->busWrite8(c, address, uint8_t(c->busRead8(c, address) | (1u << (flag % 8))));
            }
        }
        // Restore the original type before the game's own return/blackout
        // callback runs, with each trainer's own party and wallet restored.
        core->busWrite32(core, BattleFlagsAddress, originalFlags);
        if (peerWaitingEncounter) {
            // This core already ran its OWN encounter script and counted its
            // participation. Resume its native return callback once with the
            // shared outcome, preserving its trainer variant/script stack.
            auto* peer = cores[1-leader];
            peer->busWrite8(peer, OutcomeAddress, outcome);
            peer->busWrite32(peer, MainCallbackAddress, peerReturnCallback);
            peer->busWrite8(peer, 0x03003478, 0);
        } else if (firstLabRival) {
            // The first rival encounter continues its field script even on a
            // loss. Its own script heals the initiator; use the same native
            // HealPlayerParty special for the paused partner, never WhiteOut.
            const auto peer = 1 - leader;
            const auto save = readSaveMemory(cores[peer]);
            requirePokemon(startStoryObjectRefresh(cores[peer], StoryEvent::LabRival,
                uint8_t(cores[peer]->busRead8(cores[peer], save.block1 + 4)),
                uint8_t(cores[peer]->busRead8(cores[peer], save.block1 + 5))),
                "Cannot start the partner's native first-rival recovery");
        } else if (outcome == 2 || outcome == 3) {
            cores[1 - leader]->busWrite32(cores[1 - leader], MainCallbackAddress, 0x080566C5);
            // Match native SetMainCallback2: its state byte must start at0.
            cores[1 - leader]->busWrite8(cores[1 - leader], 0x03003478, 0);
        }
        battling = false;
        hooks.reset();
        currentOwner = evolutionOwner = -1;
        // Drop a held battle/evolution key before either native field resumes.
        // The worker supplies each trainer's own fresh input on the next tick.
        for (auto* c : cores) c->setKeys(c, 0);
        for (auto& state : rollbackStates) state.clear();
        for (auto& battery : rollbackBatteries) battery.clear();
        if (completed) completed(leader, (originalFlags & 8u) ? trainer : 0u, outcome, peerWaitingEncounter);
        if (event) event("team battle committed", int(outcome));
    }
public:
    std::function<void(const char*, int)> event;
    // Notification only: schedule any later native ceremony at a field-safe
    // boundary. Do not throw or advance a core from this callback.
    std::function<void(unsigned, unsigned, unsigned, bool)> completed;
    explicit TeamBattle(std::array<mCore*, 2> pair) : cores(pair) {
        requirePokemon(pair[0] && pair[1] && pair[0] != pair[1], "FireRed co-op requires two distinct emulator cores");
    }
    bool active() const { return battling; }
    unsigned battleLeader() const { return leader; }
    int inputOwner() const { return currentOwner; }
    bool ownPartyMenuActive() const {return bool(ownPartyMenu);}
    std::optional<unsigned> ownPartyMenuOwner() const {return ownPartyMenu?std::optional<unsigned>(ownPartyMenu->session.owner):std::nullopt;}
    const BattleParty& partyTransaction() const { return transaction; }
    const std::array<WorldActor, 2>& worldActors() const { return actors; }
    void sampleWorld() {
        if (battling) return;
        for (unsigned i = 0; i < 2; ++i) actors[i] = readWorldActor(cores[i]);
    }
    bool waitingForPartner(unsigned initiator) const {
        requirePokemon(initiator < 2,"Invalid FireRed waiting player");
        auto* c = cores[initiator]; auto* peer = cores[1-initiator];
        if (battling || inventoryRead32(c,MainCallbackAddress) != InitBattleCallback) return false;
        const auto flags = inventoryRead32(c,BattleFlagsAddress);
        const bool lab = flags == 0x18 && inventoryRead16(c,0x020386AC) == 9 &&
            inventoryRead16(c,TrainerAddress) >= 326 && inventoryRead16(c,TrainerAddress) <= 328 &&
            inventoryRead32(c,SavedCallbackAddress) == 0x08080409;
        if (!sharedEncounterFlags(flags) && !lab) return false;
        if (!questLogWorldAvailable(c)) return false;
        // The partner's playback location is historical. Hold this real
        // encounter until their current map has actually loaded.
        if (!questLogWorldAvailable(peer)) return true;
        const auto a = inventoryRead32(c,SaveBlock1PointerAddress), b = inventoryRead32(peer,SaveBlock1PointerAddress);
        if (!validSaveBlock(a,SaveBlock1Size) || !validSaveBlock(b,SaveBlock1Size) ||
            c->busRead8(c,a+4) != peer->busRead8(peer,b+4) || c->busRead8(c,a+5) != peer->busRead8(peer,b+5)) return lab;
        auto viable=[](mCore* player) {
            // A busy native menu can be paused inside GetMonData with secure
            // bytes temporarily decrypted. HP/egg/count remain unencrypted;
            // do not validate/read its secure party until it yields the field.
            const auto count=player->busRead8(player,PlayerPartyCountAddress);
            requirePokemon(count<=6,"Invalid FireRed waiting party count");
            for(unsigned s=0;s<count;++s) {
                const auto mon=PlayerPartyAddress+s*PartyMonSize;
                if(!(player->busRead8(player,mon+19)&4u) && player->busRead16(player,mon+86)) return true;
            }
            return false;
        };
        if (!viable(c)) return false;
        if (!viable(peer)) return lab;
        // Two DIFFERENT pending encounters cannot share a script result. Stop
        // with the validated batteries preserved, instead of silently turning
        // either fight into a solo battle or deadlocking both native scenes.
        requirePokemon(inventoryRead32(peer,MainCallbackAddress) != InitBattleCallback,
                       "Both FireRed players reached different encounters; leave one encounter before retrying the session");
        return true;
    }
    bool tryBegin(unsigned initiator) {
        requirePokemon(initiator < 2, "Invalid FireRed battle initiator");
        if (battling) return false;
        auto* core = cores[initiator];
        if (core->busRead32(core, MainCallbackAddress) != InitBattleCallback) return false;
        if (!questLogWorldAvailable(core) || !questLogWorldAvailable(cores[1-initiator])) return false;
        const auto flags = core->busRead32(core, BattleFlagsAddress);
        // Oak's first-rival tutorial controllers only support a single human.
        // Convert precisely this native encounter to ordinary human doubles,
        // then restore its original flags before its own continuation runs.
        const bool labRival = flags == 0x18u &&
            core->busRead16(core, 0x020386AC) == 9 &&
            core->busRead16(core, TrainerAddress) >= 326 &&
            core->busRead16(core, TrainerAddress) <= 328 &&
            core->busRead32(core, SavedCallbackAddress) == 0x08080409u;
        if (!sharedEncounterFlags(flags) && !labRival) return false;
        auto* peer = cores[1 - initiator];
        const bool waiting = peer->busRead32(peer, MainCallbackAddress) == InitBattleCallback;
        bool matchingPeer = false;
        const auto& other = actors[1 - initiator];
        const auto save = readSaveMemory(core);
        if (waiting) {
            auto encounter = [](mCore* c) {
                const auto m = readSaveMemory(c);
                WaitingEncounter result{inventoryRead32(c,BattleFlagsAddress),inventoryRead32(c,SavedCallbackAddress),
                    inventoryRead16(c,TrainerAddress),inventoryRead16(c,0x020386AC),
                    uint8_t(c->busRead8(c,m.block1+4)),uint8_t(c->busRead8(c,m.block1+5)),
                    uint16_t(0)};
                result.scriptContinuation=inventoryRead32(c,GlobalScriptContextAddress+8);
                result.scriptDepth=uint8_t(c->busRead8(c,GlobalScriptContextAddress));
                requirePokemon(result.scriptDepth<=20,"Invalid FireRed pending script stack");
                for(unsigned i=0;i<result.scriptDepth;++i)
                    result.scriptReturns[i]=inventoryRead32(c,GlobalScriptContextAddress+12+i*4);
                return result;
            };
            auto own = encounter(core), theirs = encounter(peer);
            if (!(flags & 8u) && !(theirs.flags & 8u) && (flags & ~5u) &&
                (flags & ~5u)==(theirs.flags & ~5u)) {
                // Only native encrypted wild records are inspected at the
                // stopped InitBattle boundary, never during GetMonData.
                for (const auto& entry : {std::make_pair(core,&own),std::make_pair(peer,&theirs)}) {
                    PartyMon mon;
                    for(unsigned b=0;b<PartyMonSize;++b) mon.bytes[b]=uint8_t(entry.first->busRead8(entry.first,0x0202402C+b));
                    mon.validate(); entry.second->species=mon.species();
                }
            }
            if(own.mapGroup!=theirs.mapGroup || own.mapNum!=theirs.mapNum) return false;
            if(!sharedEncounterFlags(theirs.flags) && !(labRival && theirs.flags==0x18)) return false;
            matchingPeer=matchingWaitingEncounters(own,theirs);
            // A different native scripted encounter remains paused at its OWN
            // InitBattle. Borrow its team now, then play that encounter with
            // both humans once this leader's native field script has returned.
            // Never feed one object/trainer's win to a different continuation.
            if(labRival && !matchingPeer) return false;
        } else {
            if (!other.valid || !campaignFieldQuiescent(peer)) return false;
            if (core->busRead8(core, save.block1 + 4) != other.mapGroup ||
                core->busRead8(core, save.block1 + 5) != other.mapNum) return false;
        }
        auto first = readParty(cores[0]), second = readParty(cores[1]);
        if (!first.count || !second.count) return false;
        bool viable0 = false, viable1 = false;
        for (unsigned s = 0; s < first.count; ++s) viable0 |= first.mons[s].viable();
        for (unsigned s = 0; s < second.count; ++s) viable1 |= second.mons[s].viable();
        if (!viable0 || !viable1) return false;
        const auto combined = combinePagedParties(first, second);
        unsigned viableWild=0;
        if (!(flags & 8u)) for (unsigned slot=0;slot<6;++slot) {
            const auto mon=0x0202402Cu+slot*PartyMonSize;
            const auto nativeFlags=core->busRead8(core,mon+19);
            if ((nativeFlags & 2u) && !(nativeFlags & 5u) && core->busRead16(core,mon+86)) ++viableWild;
        }
        singleWildBattle=!(flags & 8u) && viableWild==1;
        singleTrainerBattle=(flags & 8u) && !(flags & 1u) &&
            readTrainerPrizeMetadata(core,core->busRead16(core,TrainerAddress)).partySize==1;
        std::array<Inventory, 2> inventories{{readInventory(cores[0]), readInventory(cores[1])}};
        const std::array<uint32_t, 2> money{{readWallet(cores[0]), readWallet(cores[1])}};
        const auto callback = core->busRead32(core, SavedCallbackAddress);
        requirePokemon((callback & 1u) && callback >= 0x08000001u && callback < 0x09000000u,
                       "FireRed battle has no valid native return callback");
        std::array<std::vector<uint8_t>, 2> states{{saveState(cores[0]), saveState(cores[1])}};
        std::array<std::vector<uint8_t>, 2> batteries{{saveBattery(cores[0]), saveBattery(cores[1])}};
        std::array<StorageSnapshot, 2> boxes{{readStorage(cores[0]), readStorage(cores[1])}};
        std::array<TrainerIdentity, 2> identities{{readTrainerIdentity(cores[0]), readTrainerIdentity(cores[1])}};
        for (unsigned p = 0; p < 2; ++p) personal[p]=readPersonalCapture(cores[p]);
        transaction = combined; initialParties={{first,second}}; bags = inventories; wallets = money;
        executedBalls = {};
        for (unsigned owner = 0; owner < 2; ++owner) for (unsigned item = 1; item <= 12; ++item)
            initialBalls[owner][item] = bags[owner].quantity(item);
        rollbackStates = std::move(states);
        rollbackBatteries = std::move(batteries); storage = boxes; trainers = identities;
        leader = initiator; bagOwner = initiator; lastTextOwner = initiator;
        activeBag = bags[leader]; returnCallback = callback; originalFlags = flags;
        trainer = uint16_t(core->busRead16(core, TrainerAddress)); firstLabRival = labRival;
        peerWaitingEncounter = matchingPeer;
        peerReturnCallback = matchingPeer ? peer->busRead32(peer,SavedCallbackAddress) : 0;
        joiningStatistics = readBattleStatistics(cores[1-leader]);
        shareTrainerWin = (flags & 8u) && sharedTrainerProgressAllowed(core->busRead16(core, 0x020386AC), readTrainerPrizeMetadata(core, trainer).trainerClass);
        rewardState = {}; battleFrames = 0; sawNativeBattle = false; evolutionOwner = -1;
        catcher = -1; captureStaging.reset(); captureBeforeGift.reset(); encountered = {};
        battling = true;
        try {
            experience=std::make_unique<BattleExperience>(core,transaction);
            experience->ownerChanged=[this](unsigned owner){lastTextOwner=owner;currentOwner=int(owner);};
            experience->evolutionBag=[this](unsigned owner){swapBag(owner);};
            writeParty(core, transaction.combined);
            core->busWrite32(core, BattleFlagsAddress, (flags & ~0x10u) | 1u);
            hooks = std::make_unique<BattleHooks>(core, [this](unsigned battler, unsigned first, unsigned second) {
                return ownerHasNoReplacement(battler, first, second);
            }, [this](unsigned slot, uint32_t ot, uint32_t name) {
                const auto& trainerIdentity = trainers[unsigned(ownerOfSlot(slot))];
                requirePokemon(validSaveBlock(name & ~3u, 8) || (name >= 0x03000000u && name <= 0x03007FF8u), "Invalid FireRed trainer-name buffer");
                if (ot != trainerIdentity.id) return true;
                for (unsigned i = 0; i < trainerIdentity.name.size(); ++i) {
                    const auto character = cores[leader]->busRead8(cores[leader], name + i);
                    if (character != trainerIdentity.name[i]) return true;
                    if (character == 0xFF) return false;
                }
                return false;
            }, [this](unsigned battler, unsigned flag) {
                requirePokemon((battler == 0 || battler == 2) && flag >= 0x820 && flag <= 0x827, "Invalid FireRed owner badge request");
                const auto owner = unsigned(ownerOfSlot(cores[leader]->busRead16(cores[leader], 0x02023BCE + battler * 2)));
                const auto save = readSaveMemory(cores[owner]);
                return (cores[owner]->busRead8(cores[owner], save.block1 + 0xEE0 + flag / 8) & (1u << (flag % 8))) != 0;
            }, [this]() {
                requirePokemon(currentOwner == 0 || currentOwner == 1, "FireRed capture capacity has no bag owner");
                const bool full = !ownerHasCaptureCapacity(transaction.originals[unsigned(currentOwner)], storage[unsigned(currentOwner)]);
                if (full && event) event("full recipient capture rejected", currentOwner);
                return full;
            }, [this](unsigned battler, unsigned item) {
                const auto owner = battler / 2;
                if (captureStaging) {
                    // A previous failed throw may still have its scratch slot.
                    // Validate that it did not create a capture before replacing
                    // it with a plan derived from the immutable original PC.
                    requirePokemon(!recoverCaptureStaging(*captureStaging, readStorage(cores[leader])).captured,
                                   "FireRed executed another throw after a completed capture");
                }
                auto next = prepareCaptureStaging(storage[leader], transaction.originals[owner], storage[owner]);
                requirePokemon(next.actualOwnerHasCapacity, "FireRed executed a throw for a full owner");
                writeStorage(cores[leader], next.stagedNative);
                captureStaging = std::move(next);
                catcher = int(owner);
                ++executedBalls[owner][item];
                if (event) event("native ball executed", int(owner));
            }, [this]() {
                refillOwnerBench(0);
                refillOwnerBench(1);
            }, [this](uint32_t pointer) {
                requirePokemon(catcher==0 || catcher==1,"FireRed native caught-mon gift has no owner");
                requirePokemon(pointer>=0x0202402C && pointer<0x0202402C+600 && (pointer-0x0202402C)%100==0,
                               "Invalid FireRed native caught-mon pointer");
                requirePokemon(!captureBeforeGift,"FireRed native caught-mon gift ran twice");
                PartyMon mon;
                for(unsigned byte=0;byte<PartyMonSize;++byte)mon.bytes[byte]=uint8_t(cores[leader]->busRead8(cores[leader],pointer+byte));
                mon.validate();requirePokemon(!mon.empty() && !mon.egg(),"Invalid FireRed native caught-mon record");
                captureBeforeGift=mon;
            }, [this](unsigned slot) {
                evolutionOwner = ownerOfSlot(slot);
                lastTextOwner = unsigned(evolutionOwner);
                currentOwner = evolutionOwner;
                if (event) event("owned evolution started", evolutionOwner);
            }, [this](unsigned statistic) {
                int owner = -1;
                if (statistic == 14) owner = evolutionOwner;
                else if (statistic == 26 || statistic == 27) {
                    const auto battler = cores[leader]->busRead8(cores[leader], 0x02023D6B);
                    if (battler != 0 && battler != 2) return false;
                    owner = ownerOfSlot(cores[leader]->busRead16(cores[leader], 0x02023BCE + battler * 2));
                } else return false;
                requirePokemon(owner == 0 || owner == 1, "FireRed personal action statistic has no owner");
                incrementGameStatistic(cores[unsigned(owner)], statistic);
                return true;
            }, [this](bool payDay,uint32_t nativeAmount) {
                auto* c=cores[leader];
                const auto outcome=c->busRead8(c,OutcomeAddress);
                uint32_t amount=nativeAmount;
                if(outcome==1) {
                    if(payDay)amount=splitBattleRewards(0,nativeAmount,leader).shares[leader];
                    else {
                        const auto state=readNativeRewardState(c);
                        requirePokemon(bool(state),"Cannot read native trainer money announcement");
                        amount=splitBattleRewards(nativeTrainerPrize(c,trainer,originalFlags&~0x10u,state->moneyMultiplier),0,leader).shares[leader];
                    }
                } else if(!payDay && (outcome==2 || outcome==3)) {
                    if(firstLabRival)amount=0;
                    else {
                        const auto parties=restoreParties(transaction,readParty(c));
                        unsigned highest=1,badges=0;
                        for(unsigned slot=0;slot<parties[leader].count;++slot)
                            if(!parties[leader].mons[slot].egg())highest=std::max(highest,unsigned(parties[leader].mons[slot].level()));
                        const auto save=readSaveMemory(c);
                        for(unsigned flag=0x820;flag<=0x827;++flag)
                            badges+=(c->busRead8(c,save.block1+0xEE0+flag/8)&(1u<<(flag%8)))!=0;
                        amount=nativeBlackoutLoss(highest,badges,wallets[leader]);
                    }
                }
                requirePokemon(amount<=MaximumWallet,"Invalid shared money announcement");
                if(event)event(payDay?"native payday share announced":"native trainer money announced",int(amount));
                return amount;
            }, [this](unsigned battler){beginOwnPartyMenu(battler);},
               [this]{finishOwnPartyMenu();},
               [this](unsigned selected){return normalizeOwnPartyMenuReply(selected);},
               [this](unsigned battler)->std::optional<std::array<uint8_t,7>> {
                   const unsigned owner=battler/2;
                   if (catcher != int(owner) || owner == leader) return std::nullopt;
                   return trainers[owner].name;
               }, [this]{return singleWildBattle;}, [this]{return singleTrainerBattle;},
               [this]{return experience->experienceCommand();}, [this]{experience->evolutionGate();},
               [this](bool ending){return experience->evolutionBoundary(ending);},
               [this](unsigned battler){experience->opponentReset(battler);},
               [this](unsigned slot)->std::optional<bool> {
                   if(!experience->evolutionOwner())return std::nullopt;
                   auto* c=cores[unsigned(ownerOfSlot(slot))];const auto memory=readSaveMemory(c);
                   const auto progress=readProgress(c);
                   return c->busRead8(c,memory.block2+0x1B)==0xB9 && progress.var(0x404E)==0x6258 && progress.flag(0x840);
               });
        } catch (...) { rollback(); throw; }
        if (event) event("team battle joined", int(initiator));
        return true;
    }
    // Restores the complete pre-conversion machine state, including both
    // battery buffers. Never flush while active; callers flush only after this
    // or after a validated commit. A failed rollback is a fatal session error.
    void rollback() {
        if (!battling) return;
        hooks.reset();
        bool restored = true;
        for (unsigned i = 0; i < 2; ++i) {
            restored = cores[i]->loadState(cores[i], rollbackStates[i].data()) && restored;
            restored = cores[i]->savedataRestore(cores[i], rollbackBatteries[i].data(), rollbackBatteries[i].size(), true) && restored;
            // mGBA0.10.5 does not propagate GBASavedataLoad's result. Verify
            // the actual buffer rather than trusting the wrapper's bool alone.
            restored = (saveBattery(cores[i]) == rollbackBatteries[i]) && restored;
        }
        requirePokemon(restored, "Cannot roll back FireRed battle; stop without saving");
        battling = false; currentOwner = evolutionOwner = -1;
        ownPartyMenu.reset();ownPartyMenuReply.reset();
        for (auto& state : rollbackStates) state.clear();
        for (auto& battery : rollbackBatteries) battery.clear();
        if (event) event("team battle rolled back", 0);
    }
    void runBattleFrame(const std::array<uint32_t, 2>& keys) {
        requirePokemon(battling, "No FireRed team battle is active");
        auto* core = cores[leader];
        try {
            const auto pending = core->busRead32(core, ControllerExecAddress);
            int owner = -1, battler = -1; unsigned command = 0;
            for (unsigned b : {0u, 2u}) {
                if (!(pending & (1u << b))) continue;
                const auto cmd = core->busRead8(core, ControllerBufferA + b * 0x200);
                if (cmd == 18 || cmd == 19 || cmd == 20 || cmd == 21 || cmd == 22 || cmd == 25) {
                    requirePokemon(owner == -1, "Ambiguous simultaneous FireRed input requests");
                    owner = int(b / 2); battler = int(b); command = cmd;
                    if (cmd == 25) owner = ownerOfSlot(core->busRead8(core, ControllerBufferA + b * 0x200 + 1));
                }
            }
            if (evolutionOwner >= 0 && core->busRead32(core, MainCallbackAddress) != 0x08011085u) {
                requirePokemon(owner < 0 || owner == evolutionOwner, "Conflicting FireRed evolution input owners");
                owner = evolutionOwner;
            }
            if(const auto recipient=experience->inputOwner())owner=int(*recipient);
            if (owner >= 0) lastTextOwner = unsigned(owner);
            currentOwner = owner;
            if (sawNativeBattle) if (const auto state = readNativeRewardState(core)) rewardState = *state;
            if (command == 21) swapBag(unsigned(owner));
            // Selection is translated at the native menu-return boundary,
            // before the engine consumes a reply. A later mismatch is fatal to
            // this transaction: never rewind CPU state without its C++ journal.
            core->setKeys(core, keys[owner < 0 ? lastTextOwner : unsigned(owner)] & 0x3FFu);
            if (core->busRead32(core, 0x03004ED4) == 0x08015ADDu) {
                // The return callback can execute and finish loading the field
                // within ONE mGBA frame. Intercept its exact native boundary,
                // then finish that same frame with the original parties back.
                const auto frame = core->frameCounter(core);
                unsigned instructions = 0;
                while (core->frameCounter(core) == frame) {
                    core->step(core);
                    if (battling && core->busRead32(core, MainCallbackAddress) == returnCallback &&
                        !(core->busRead8(core, 0x03003479) & 2u)) finish();
                    requirePokemon(++instructions < 1000000, "FireRed return callback did not complete a frame");
                }
                if (!battling) return;
            } else core->runFrame(core);
            if (hooks) hooks->check();
            if (command == 22 && !(core->busRead32(core, ControllerExecAddress) & (1u << battler))) {
                const auto selected = core->busRead8(core, ControllerBufferB + unsigned(battler) * 0x200 + 1);
                requirePokemon(selected>=PartyCapacity || ownerOfSlot(selected)==owner,
                               "Native FireRed party reply belongs to another trainer");
            }
            sawNativeBattle |= (core->busRead8(core, 0x03003479) & 2u) != 0;
            if (sawNativeBattle && core->busRead32(core, MainCallbackAddress) == 0x08011085u) {
                for (unsigned battler : {1u, 3u}) {
                    if (core->busRead8(core, 0x02023D70) & (1u << battler)) continue;
                    const auto slot = core->busRead16(core, 0x02023BCE + battler * 2);
                    if (slot >= 6 || encountered[slot]) continue;
                    PartyMon mon;
                    for (unsigned byte = 0; byte < PartyMonSize; ++byte)
                        mon.bytes[byte] = uint8_t(core->busRead8(core, 0x0202402C + slot * PartyMonSize + byte));
                    if (!mon.empty()) {
                        // Frame boundaries can interrupt a native GetMonData
                        // call between decrypt/re-encrypt. An observational
                        // sample may be transient: retry it next frame. Final
                        // transaction validation still rejects bad records.
                        try {mon.validate();encountered[slot] = std::make_pair(unsigned(mon.species()), mon.identity().personality);}
                        catch (const std::runtime_error&) {}
                    }
                }
            }
            if (sawNativeBattle) if (const auto state = readNativeRewardState(core)) rewardState = *state;
            ++battleFrames;
            // CB2's return address is the transaction boundary, before field
            // scripts, blackout healing or native save commands run again.
            if (sawNativeBattle && core->busRead32(core, MainCallbackAddress) == returnCallback &&
                !(core->busRead8(core, 0x03003479) & 2u)) finish();
        } catch (const std::exception& error) {
            if (event) event(error.what(), int(battleFrames));
            rollback(); throw;
        } catch (...) { rollback(); throw; }
    }
};
} // namespace coop::firered
