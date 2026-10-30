// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedInventory.h"
#include "FireRedQuestLog.h"
#include <functional>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace coop::firered {

// Only these reviewed quest milestones are shared. Starter selection, personal
// purchases, pickups, party/dex data and all other flags/variables remain local.
inline constexpr unsigned ProgressFlagBytes = 0x120;
inline constexpr unsigned ProgressVariableCount = 0x100;
inline constexpr uint32_t GlobalScriptStatusAddress = 0x03000EA8;
inline constexpr uint32_t GlobalScriptContextAddress = 0x03000EB0;
inline constexpr uint32_t ImmediateScriptContextAddress = 0x03000F28;
inline constexpr uint32_t FieldControlsLockAddress = 0x03000F9C;
inline constexpr unsigned FlagPokemonObtained = 0x828, FlagPokedexObtained = 0x829;
inline constexpr unsigned FlagBoulderBadge = 0x820, FlagBrockDefeated = 0x4B0, FlagBrockTM = 0x254;
inline constexpr unsigned VarLabScene = 0x4055, VarMartScene = 0x4057;
inline constexpr unsigned OaksParcelItem = 349, RockTombItem = 327, PokeBallItem = 4, TMCaseItem = 364;

enum class StoryEvent : unsigned { Parcel = 0, Pokedex = 1, Brock = 2, LabRival = 3 };
struct Progress {
    std::array<uint8_t, ProgressFlagBytes> flags{};
    std::array<uint16_t, ProgressVariableCount> vars{};
    Inventory inventory;
    uint32_t gcnLinkFlags = 0;
    uint8_t mapGroup = 0, mapNum = 0;
    bool flag(unsigned id) const {
        requirePokemon(id < ProgressFlagBytes * 8, "Invalid FireRed progress flag");
        return (flags[id / 8] & (1u << (id % 8))) != 0;
    }
    void flag(unsigned id, bool value) {
        requirePokemon(id < ProgressFlagBytes * 8, "Invalid FireRed progress flag");
        flags[id / 8] = uint8_t((flags[id / 8] & ~(1u << (id % 8))) | (unsigned(value) << (id % 8)));
    }
    uint16_t var(unsigned id) const {
        requirePokemon(id >= 0x4000 && id < 0x4100, "Invalid FireRed progress variable"); return vars[id - 0x4000];
    }
    void var(unsigned id, uint16_t value) {
        requirePokemon(id >= 0x4000 && id < 0x4100, "Invalid FireRed progress variable"); vars[id - 0x4000] = value;
    }
    bool operator==(const Progress& p) const {
        return flags == p.flags && vars == p.vars && inventory == p.inventory &&
               gcnLinkFlags == p.gcnLinkFlags && mapGroup == p.mapGroup && mapNum == p.mapNum;
    }
};
inline Progress readProgress(mCore* core) {
    const auto memory = readSaveMemory(core);
    Progress result;
    for (unsigned i = 0; i < result.flags.size(); ++i) result.flags[i] = uint8_t(core->busRead8(core, memory.block1 + 0xEE0 + i));
    for (unsigned i = 0; i < result.vars.size(); ++i) result.vars[i] = inventoryRead16(core, memory.block1 + 0x1000 + i * 2);
    result.gcnLinkFlags = inventoryRead32(core, memory.block2 + 0xA8);
    result.mapGroup = uint8_t(core->busRead8(core, memory.block1 + 4));
    result.mapNum = uint8_t(core->busRead8(core, memory.block1 + 5));
    result.inventory = readInventory(core);
    checkSaveMemory(core, memory);
    return result;
}
inline bool scriptInProgress(mCore* core) {
    // WAITING is an owned script too: native tasks and battles later resume it.
    // A menu can lock field controls with neither script context running.
    return core->busRead8(core, GlobalScriptStatusAddress) != 2 ||
           core->busRead8(core, GlobalScriptContextAddress + 1) != 0 ||
           core->busRead8(core, ImmediateScriptContextAddress + 1) != 0;
}
inline bool progressFieldReady(mCore* core) {
    return questLogWorldAvailable(core) && inventoryRead32(core, 0x03003044) == 0x080565D5 &&
        !scriptInProgress(core) && core->busRead8(core, FieldControlsLockAddress) == 0 &&
        !(core->busRead8(core, 0x03003479) & 2);
}
inline bool tryAddStoryItem(Inventory& inventory, unsigned item, unsigned quantity) {
    inventory.validate();
    const auto old = inventory.quantity(item);
    if (old + quantity > 999) return false;
    auto proposed = inventory;
    try {
        // Native AddBagItem creates the TM Case when adding the first NEW TM
        // stack. Existing stacks return earlier in that native routine. Keep
        // this recipient's key pocket and TM delivery one atomic transaction.
        if (old == 0 && itemPocket(item) == Machines && proposed.quantity(TMCaseItem) == 0)
            proposed.setQuantity(TMCaseItem, 1);
        proposed.setQuantity(item, old + quantity);
    } catch (const std::runtime_error&) { return false; }
    inventory = proposed;
    return true;
}
inline bool storyEventComplete(const Progress& p, StoryEvent event) {
    if (!p.flag(FlagPokemonObtained)) return false;
    switch (event) {
    case StoryEvent::LabRival: return p.flag(0x258) && p.var(VarLabScene) >= 4;
    case StoryEvent::Parcel: return p.var(VarMartScene) >= 1 && p.var(VarLabScene) >= 5;
    case StoryEvent::Pokedex: return p.flag(FlagPokedexObtained) && p.var(VarLabScene) >= 6;
    case StoryEvent::Brock: return p.flag(FlagBrockDefeated) && p.flag(FlagBoulderBadge);
    }
    return false;
}
inline std::vector<StoryEvent> completedStoryEvents(const Progress& before, const Progress& after) {
    std::vector<StoryEvent> events;
    // The stage and map checks reject unrelated flag edits and incomplete scenes.
    if (before.mapGroup == 4 && before.mapNum == 3 && before.var(VarLabScene) == 3 &&
        !before.flag(0x258) && after.var(VarLabScene) == 4 && after.flag(0x258))
        events.push_back(StoryEvent::LabRival);
    if (before.mapGroup == 5 && before.mapNum == 3 &&
        before.var(VarMartScene) == 0 && after.var(VarMartScene) == 1 &&
        after.var(VarLabScene) == 5 && after.inventory.quantity(OaksParcelItem) == 1)
        events.push_back(StoryEvent::Parcel);
    if (before.mapGroup == 4 && before.mapNum == 3 &&
        before.var(VarLabScene) == 5 && before.inventory.quantity(OaksParcelItem) == 1 &&
        !before.flag(FlagPokedexObtained) && after.flag(FlagPokedexObtained) &&
        after.var(VarLabScene) == 6 && after.var(VarMartScene) == 2 &&
        after.inventory.quantity(OaksParcelItem) == 0 &&
        after.inventory.quantity(PokeBallItem) == before.inventory.quantity(PokeBallItem) + 5 &&
        (after.gcnLinkFlags & 0x31u) == 0x31u)
        events.push_back(StoryEvent::Pokedex);
    if (before.mapGroup == 6 && before.mapNum == 2 &&
        !before.flag(FlagBrockDefeated) && !before.flag(FlagBoulderBadge) &&
        after.flag(FlagBrockDefeated) && after.flag(FlagBoulderBadge) && after.var(0x406C) >= 1)
        events.push_back(StoryEvent::Brock);
    return events;
}
struct StoryApplication {
    Progress progress;
    bool applied = false;
    bool deferredItem = false;
};
inline StoryApplication applyStoryEvent(const Progress& recipient, StoryEvent event) {
    recipient.inventory.validate();
    StoryApplication result{recipient, false, false};
    auto& p = result.progress;
    if (storyEventComplete(p, event)) return result; // Persistent native ledger.
    if (!p.flag(FlagPokemonObtained) || p.var(VarLabScene) < 3) return result;
    switch (event) {
    case StoryEvent::LabRival:
        if (p.var(VarLabScene) != 3) return result;
        p.var(VarLabScene, 4); p.flag(0x258, true); p.flag(0x02D, true);
        break;
    case StoryEvent::Parcel:
        if (p.var(VarLabScene) != 4 || p.var(VarMartScene) != 0) return result;
        if (!p.inventory.quantity(OaksParcelItem) && !tryAddStoryItem(p.inventory, OaksParcelItem, 1)) return result;
        p.var(VarLabScene, 5); p.var(VarMartScene, 1);
        break;
    case StoryEvent::Pokedex:
        if (p.var(VarLabScene) != 5 || p.var(VarMartScene) != 1) return result;
        if (!tryAddStoryItem(p.inventory, PokeBallItem, 5)) return result;
        p.inventory.setQuantity(OaksParcelItem, 0);
        p.flag(FlagPokedexObtained, true); p.flag(0x03A, true); p.flag(0x02D, true);
        p.gcnLinkFlags |= 0x31u;
        p.var(VarLabScene, 6); p.var(VarMartScene, 2);
        for (const auto id : {0x407Cu, 0x4051u, 0x4058u, 0x4054u})
            if (p.var(id) == 0) p.var(id, 1);
        break;
    case StoryEvent::Brock:
        if (!p.flag(FlagPokedexObtained)) return result;
        p.flag(FlagBrockDefeated, true); p.flag(FlagBoulderBadge, true);
        p.flag(0x02E, true); p.flag(0x092, false);
        p.flag(0x500 + 414, true); p.flag(0x500 + 142, true);
        if (p.var(0x406C) == 0) p.var(0x406C, 1);
        // Native Brock lets a player claim the TM later when their bag is full.
        // Never set the receipt flag without delivering that player's own TM.
        if (!p.flag(FlagBrockTM)) {
            if (tryAddStoryItem(p.inventory, RockTombItem, 1)) p.flag(FlagBrockTM, true);
            else result.deferredItem = true;
        }
        break;
    }
    p.inventory.validate();
    result.applied = true;
    return result;
}
inline void writeProgressDelta(mCore* core, const Progress& before, const Progress& after) {
    before.inventory.validate(); after.inventory.validate();
    requirePokemon(before.mapGroup == after.mapGroup && before.mapNum == after.mapNum,
                   "Story transaction must not relocate a player");
    const auto memory = readSaveMemory(core);
    requirePokemon(readProgress(core) == before, "FireRed progress changed during story transaction");
    // No engine instruction runs between preflight and this bounded RAM commit.
    // The caller holds both cores stopped and must never save mid-transaction.
    writeInventory(core, after.inventory);
    checkSaveMemory(core, memory);
    for (unsigned i = 0; i < before.flags.size(); ++i)
        if (before.flags[i] != after.flags[i]) core->busWrite8(core, memory.block1 + 0xEE0 + i, after.flags[i]);
    for (unsigned i = 0; i < before.vars.size(); ++i)
        if (before.vars[i] != after.vars[i]) inventoryWrite16(core, memory.block1 + 0x1000 + i * 2, after.vars[i]);
    if (before.gcnLinkFlags != after.gcnLinkFlags)
        for (unsigned i = 0; i < 4; ++i) core->busWrite8(core, memory.block2 + 0xA8 + i, uint8_t(after.gcnLinkFlags >> (8 * i)));
}

// Run the existing game's object-management commands on its ordinary script VM.
// The bytecode occupies the stopped context's otherwise unused stack, not ROM,
// heap, party/save data or an arbitrary EWRAM scratch address. No CALL/CALLSTD
// command is permitted: those would use this same stack. LOCKALL waits for the
// local avatar to stop, then native remove/add routines own sprite lifetimes.
inline bool startStoryObjectRefresh(mCore* core, StoryEvent event, uint8_t group, uint8_t num) {
    std::vector<uint8_t> code{0x69};
    auto object = [&](uint8_t command, uint16_t localId) {
        code.push_back(command); code.push_back(uint8_t(localId)); code.push_back(uint8_t(localId >> 8));
    };
    auto fame = [&](uint16_t person, uint16_t index, uint16_t special) {
        const uint8_t setup[] = {0x16,4,0x80,uint8_t(person),uint8_t(person >> 8),
                                0x16,5,0x80,uint8_t(index),uint8_t(index >> 8),
                                0x25,uint8_t(special),uint8_t(special >> 8)};
        code.insert(code.end(), std::begin(setup), std::end(setup));
    };
    if (event == StoryEvent::Pokedex) fame(0, 1, 371); // Native Oak flavor-text receipt.
    if (event == StoryEvent::Brock) {
        fame(2, 2, 372); // Native colored Brock entry, as at his first conversation.
        fame(2, 1, 371); // Native defeat detail; preserves other personal details.
    }
    if (event == StoryEvent::LabRival) {
        code.insert(code.end(), {0x25,0,0}); // Native HealPlayerParty (special 0).
        if (group == 4 && num == 3) object(0x53, 8);
    } else if (event == StoryEvent::Pokedex && group == 4 && num == 3) {
        for (unsigned id : {8u, 9u, 10u}) object(0x53, uint16_t(id));
    } else if (event == StoryEvent::Pokedex && group == 3 && num == 1) {
        object(0x53, 4);
        const uint8_t setup[] = {0x16,0x10,0x40,32,0, 0x63,4,0,21,0,8,0, 0x65,4,0,1};
        code.insert(code.end(), std::begin(setup), std::end(setup));
        object(0x55, 4);
    } else if (event == StoryEvent::Pokedex && group == 5 && num == 3) {
        // The native Mart ON_LOAD script hides its questionnaire before the
        // Pokedex is obtained. Restore the two ORIGINAL live ROM map entries
        // when a partner unlocks it while this player is already inside. No
        // metatile/graphics asset is bundled or hardcoded in CouchSplit.
        const auto layout = inventoryRead32(core, 0x02036DFC);
        requirePokemon(layout >= 0x08000000 && layout <= 0x08FFFFE4,
                       "Native Mart map layout is outside its verified ROM");
        const auto width = inventoryRead32(core, layout), height = inventoryRead32(core, layout + 4);
        const auto tiles = inventoryRead32(core, layout + 12);
        requirePokemon(width >= 2 && width <= 128 && height >= 5 && height <= 128 &&
                       tiles >= 0x08000000 && tiles <= 0x09000000 - width * height * 2,
                       "Native Mart original map entries are outside ROM");
        for (const unsigned y : {3u, 4u}) {
            const auto tile = inventoryRead16(core, tiles + (y * width + 1) * 2) & 0x0FFFu;
            // SETMETATILE with impassable=FALSE preserves the supplied native
            // collision bits and the current grid elevation.
            const uint8_t command[] = {0xA2,1,0,uint8_t(y),0,uint8_t(tile),uint8_t(tile >> 8),0,0};
            code.insert(code.end(), std::begin(command), std::end(command));
        }
        code.insert(code.end(), {0x25,0x8E,0}); // Native DrawWholeMapView.
    } else if (event == StoryEvent::Brock && group == 3 && num == 2) {
        object(0x53, 5); object(0x55, 7);
    } else if (event != StoryEvent::Pokedex && event != StoryEvent::Brock) return false;
    code.push_back(0x6B); code.push_back(0x02);
    requirePokemon(code.size() <= 80, "Native story refresh exceeds the unused script stack");
    requirePokemon(progressFieldReady(core) && core->busWrite8, "Native story refresh requires an idle writable overworld");
    const auto memory = readSaveMemory(core);
    requirePokemon(core->busRead8(core, memory.block1 + 4) == group && core->busRead8(core, memory.block1 + 5) == num,
                   "Native story refresh map changed");
    const auto context = GlobalScriptContextAddress;
    const auto table = inventoryRead32(core, context + 0x5C), end = inventoryRead32(core, context + 0x60);
    requirePokemon(table >= 0x08000000 && end > table && end <= 0x09000000 && end - table >= 0xA3 * 4,
                   "Native script command table is not initialized");
    for (unsigned i = 0; i < 80; ++i) core->busWrite8(core, context + 12 + i, i < code.size() ? code[i] : 0);
    // Initialize only the stopped VM's execution fields. Keep its verified
    // command tables; the program never reads ctx.data or comparisonResult.
    for (unsigned i = 0; i < 12; ++i) core->busWrite8(core, context + i, 0);
    for (unsigned i = 0; i < 4; ++i) core->busWrite8(core, context + 8 + i, uint8_t((context + 12) >> (i * 8)));
    core->busWrite8(core, context + 1, 1);
    core->busWrite8(core, FieldControlsLockAddress, 1);
    core->busWrite8(core, GlobalScriptStatusAddress, 0);
    return true;
}

// Observe only complete native scenes; queues wait while a recipient is in a
// menu, a script, a battle, or lacks the required personal starter/quest stage.
// Only the four reviewed, ordered milestones are reconciled from native saves.
// This also rebuilds pending delivery after quitting while a partner had a menu
// open. Each player's own receipt flags/stages are the durable event ledger;
// personal starters, later quest variables and ordinary pickups stay separate.
class StoryProgress {
public:
    explicit StoryProgress(std::array<mCore*, 2> cores) : cores_(cores) {
        requirePokemon(cores[0] && cores[1] && cores[0] != cores[1], "Story progress requires two separate cores");
    }
    std::function<void(StoryEvent, unsigned, bool)> event;
    void sample() {
        // A replay contains historical flags, bags and position. Discard
        // volatile observations; durable receipts rebuild pending deliveries
        // from the current saves once both real fields have loaded.
        if (!questLogWorldAvailable(cores_[0]) || !questLogWorldAvailable(cores_[1])) {
            baseline_ = {}; observedScript_ = {}; pending_ = {}; return;
        }
        std::array<bool, 2> ready{};
        for (unsigned owner = 0; owner < 2; ++owner) {
            auto* core = cores_[owner];
            ready[owner] = progressFieldReady(core);
            if (!ready[owner]) {
                if (baseline_[owner] && scriptInProgress(core)) observedScript_[owner] = true;
                continue;
            }
            auto now = readProgress(core);
            if (baseline_[owner] && observedScript_[owner])
                for (const auto completed : completedStoryEvents(*baseline_[owner], now))
                    pending_[1 - owner] |= 1u << unsigned(completed);
            observedScript_[owner] = false;
            baseline_[owner] = std::move(now);
        }
        if (baseline_[0] && baseline_[1]) {
            for (unsigned recipient = 0; recipient < 2; ++recipient)
                for (unsigned id = 0; id < 4; ++id) {
                    const auto type = StoryEvent(id);
                    if (storyEventComplete(*baseline_[1 - recipient], type) &&
                        !storyEventComplete(*baseline_[recipient], type)) pending_[recipient] |= 1u << id;
                }
        }
        for (unsigned recipient = 0; recipient < 2; ++recipient) {
            if (!ready[recipient]) continue;
            for (unsigned id = 0; id < 4; ++id) {
                const unsigned bit = 1u << id;
                if (!(pending_[recipient] & bit)) continue;
                const auto type = StoryEvent(id);
                const auto before = readProgress(cores_[recipient]);
                const auto applied = applyStoryEvent(before, type);
                if (storyEventComplete(before, type)) { pending_[recipient] &= ~bit; continue; }
                if (!applied.applied) continue;
                writeProgressDelta(cores_[recipient], before, applied.progress);
                pending_[recipient] &= ~bit;
                baseline_[recipient] = applied.progress;
                if (event) event(type, recipient, applied.deferredItem);
                if (startStoryObjectRefresh(cores_[recipient], type, before.mapGroup, before.mapNum)) break;
            }
        }
    }
    unsigned pending(unsigned owner) const {
        requirePokemon(owner < 2, "Invalid story progress owner"); return pending_[owner];
    }
private:
    std::array<mCore*, 2> cores_;
    std::array<std::optional<Progress>, 2> baseline_;
    std::array<bool, 2> observedScript_{};
    std::array<unsigned, 2> pending_{};
};

} // namespace coop::firered
