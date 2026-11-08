// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedProgress.h"
#include <map>
#include <set>
#include <string>

namespace coop::firered {

// A campaign flag is not enough to update an already loaded native map: its
// objects and metatiles were created before that flag changed. This module
// preflights the map's *own* original bytecode, then asks its native script VM
// to update those resources. Reviewed layout variants stage only preflighted
// complete grid entries inside the caller's whole-machine/Flash transaction.
// It never runs arbitrary map-load native callbacks,
// adopts another core, relocates the player, or copies graphics into our source.
// Every entry point requires the caller to have verified the exact German ROM.
inline constexpr uint32_t CampaignMapHeaderAddress = 0x02036DFCu;
inline constexpr uint32_t CampaignObjectsAddress = 0x02036E38u;
inline constexpr uint32_t CampaignAvatarAddress = 0x02037078u;

inline bool campaignFieldQuiescent(mCore* core) {
    if (!progressFieldReady(core) || !core->busRead16 || !core->busRead32 || !core->busWrite8) return false;
    const auto id = core->busRead8(core, CampaignAvatarAddress + 5);
    if (id >= 16 || core->busRead8(core, CampaignAvatarAddress + 2) != 0 ||
        core->busRead8(core, CampaignAvatarAddress + 3) != 0 ||
        core->busRead8(core, CampaignAvatarAddress + 6) != 0) return false;
    const auto object = CampaignObjectsAddress + id * 0x24;
    const auto flags = inventoryRead32(core, object);
    // Held movements retain their active bit after finishing until a later
    // script explicitly clears them. Native idle avatars therefore commonly
    // have both bits 6/7 set; only an unfinished held movement owns the tile.
    if (!(flags & 1u) || !(flags & 0x10000u) || (flags & (2u | 0x20u)) ||
        ((flags & 0x40u) && !(flags & 0x80u)) ||
        core->busRead8(core, object + 8) != 0xFFu) return false;
    return inventoryRead16(core, object + 0x10) == inventoryRead16(core, object + 0x14) &&
           inventoryRead16(core, object + 0x12) == inventoryRead16(core, object + 0x16);
}

struct CampaignObjectRefreshPlan {
    bool ready = false;
    std::string reason;
    std::vector<uint8_t> code;
    // Preimages contain only map/runtime RAM and original map pointers. They
    // protect against accidentally starting a plan on a different map/frame.
    std::map<uint32_t, uint8_t> preimages;
    Progress expected;
    unsigned programOffset = 12;
    unsigned capacity() const { return programOffset <= 92 ? 92 - programOffset : 0; }
    std::vector<uint32_t> originalTileLeaves;
    // Exact original-layout entry differences, preflighted against the loaded
    // grid. They are committed inside the caller's two-machine/Flash snapshot
    // before native SETMAPLAYOUTINDEX/DrawWholeMapView run. Never replace an
    // entire grid: unchanged cells may contain this player's own puzzle state.
    std::map<uint32_t,uint16_t> layoutEntries;
    unsigned removedObjects = 0, addedObjects = 0, movedObjects = 0, changedTiles = 0;
    bool changedWeather = false;
    bool changedLayout = false;
};

namespace campaign_runtime_detail {
inline bool rom(uint32_t address, unsigned size = 1) {
    return size <= 0x01000000u && address >= 0x08000000u && address <= 0x09000000u - size;
}
inline int signedHalf(uint16_t v) { return v & 0x8000u ? int(v) - 65536 : int(v); }
inline bool cableClub(unsigned group,unsigned map) {
    // The nineteen original Pokemon Center upper floors share the reviewed
    // CableClub_OnTransition. Link state and Wonder Cards stay personal.
    constexpr std::pair<unsigned,unsigned> maps[]={{5,5},{6,6},{7,4},{8,1},{9,2},
        {10,13},{11,6},{12,6},{13,1},{14,7},{16,1},{21,1},{31,4},{32,1},{33,3},
        {34,2},{35,2},{36,1},{37,1}};
    for(const auto& entry:maps)if(entry.first==group&&entry.second==map)return true;
    return false;
}
inline void word(std::vector<uint8_t>& out, unsigned v) {
    out.push_back(uint8_t(v)); out.push_back(uint8_t(v >> 8));
}
struct Tile {
    uint16_t value = 0;
    bool impassable = false;
    bool operator==(const Tile& t) const { return value == t.value && impassable == t.impassable; }
    bool operator!=(const Tile& t) const { return !(*this == t); }
};
struct Object {
    unsigned id = 0, flag = 0, kind = 0, graphics = 0;
    int x = 0, y = 0;
    unsigned movement = 0;
};
struct Effects {
    std::map<std::pair<unsigned,unsigned>, Tile> tiles;
    std::map<unsigned, std::pair<int,int>> coordinates;
    std::map<unsigned, unsigned> movements;
    std::map<unsigned, unsigned> variables;
    std::map<unsigned, bool> flags;
    std::map<uint32_t,std::map<std::pair<unsigned,unsigned>,Tile>> tileLeaves;
    std::map<unsigned,std::vector<unsigned>> retainedNativeInputs;
    std::optional<unsigned> layoutId;
    std::optional<unsigned> weather;
    bool applyWeather = false;
};

// Read-only evaluator of the small, synchronous native map setup language.
// Unsupported executed instructions fail before any game mutation. Evaluation
// CALLs use our bounded C++ stack. Emitted bytecode uses literal commands or a
// flat, revalidated original tile leaf with a separate reserved return area.
class Evaluator {
public:
    Evaluator(mCore* core, const Progress& progress, bool nativeNationalDexAfter = false)
        : c(core), p(progress), nationalDexAfter(nativeNationalDexAfter) {}
    Effects run(uint32_t address) {
        if (!address) return effects;
        unsigned steps = 0;
        std::vector<uint32_t> returns;
        while (++steps <= 4096) {
            const auto op = byte(address++);
            switch (op) {
            case 0: case 1: break;
            case 2: return effects;
            case 3:
                if (returns.empty()) throw std::runtime_error("Map refresh returned outside its bounded script");
                address = returns.back(); returns.pop_back(); break;
            case 4: case 5: {
                const auto target = longword(address); address += 4;
                if (op == 4) { recordLeaf(target); push(returns, address); }
                address = target; break;
            }
            case 6: case 7: {
                const auto condition = byte(address++);
                const auto target = longword(address); address += 4;
                requirePokemon(condition < 6 && comparison < 3, "Invalid map refresh condition");
                const bool take = condition == 0 ? comparison == 0 : condition == 1 ? comparison == 1 :
                    condition == 2 ? comparison == 2 : condition == 3 ? comparison != 2 :
                    condition == 4 ? comparison != 0 : comparison != 1;
                if (take) { if (op == 7) { recordLeaf(target); push(returns, address); } address = target; }
                break;
            }
            case 0x16: {
                const auto id = half(address), value = half(address + 2); address += 4;
                assign(id,value);
                break;
            }
            case 0x17: case 0x18: case 0x19: case 0x1A: {
                const auto id = half(address), raw = half(address + 2); address += 4;
                const unsigned value = op == 0x17 ? variable(id) + raw :
                    op == 0x18 ? variable(id) - (raw < 0x4000 ? raw : variable(raw)) :
                    op == 0x19 ? variable(raw) : raw < 0x4000 ? raw : variable(raw);
                assign(id,uint16_t(value));
                break;
            }
            case 0x21: case 0x22: {
                const auto a = variable(half(address));
                const auto operand = half(address + 2); address += 4;
                const auto b = op == 0x22 ? variable(operand) : operand;
                comparison = a < b ? 0 : a == b ? 1 : 2; break;
            }
            case 0x29: case 0x2A: {
                const auto id = half(address); address += 2;
                requirePokemon(id < ProgressFlagBytes * 8, "Map refresh writes an unsupported temporary flag");
                p.flag(id,op == 0x29); effects.flags[id] = op == 0x29;
                break;
            }
            case 0x2B: {
                const auto id = half(address); address += 2;
                requirePokemon(id < ProgressFlagBytes * 8, "Map refresh reads an unsupported temporary flag");
                comparison = unsigned(p.flag(id)); break;
            }
            case 0x63: { // SETOBJECTXYPERM: collected, then applied with native resource ownership.
                const auto id = operand(address), x = operand(address + 2), y = operand(address + 4); address += 6;
                requirePokemon(id && id < 255 && x < 2048 && y < 2048, "Invalid map object placement");
                effects.coordinates[id] = {int(x), int(y)}; break;
            }
            case 0x65: {
                const auto id = operand(address); const auto movement = byte(address + 2); address += 3;
                requirePokemon(id && id < 255, "Invalid map movement object");
                effects.movements[id] = movement; break;
            }
            case 0x60: { // native trainer flags use the ordinary 0x500 flag range.
                const auto trainer = operand(address); address += 2;
                requirePokemon(trainer < ProgressFlagBytes * 8 - 0x500, "Map setup trainer flag is out of range");
                comparison = unsigned(p.flag(0x500 + trainer)); break;
            }
            case 0x26: {
                const auto result = half(address), special = half(address + 2); address += 4;
                if(special==384 && result==0x800D && cableClub(p.mapGroup,p.mapNum)) {
                    // ValidateSavedWonderCard already ran on native entry.
                    // No campaign receipt changes a card, RAM script or the
                    // deliveryman receipt; retain that loaded native outcome
                    // instead of revalidating/replacing this player's card.
                    effects.retainedNativeInputs[special]={unsigned(p.flag(0x70)),p.var(0x406F)};
                    assign(result,!p.flag(0x70));break;
                }
                // Exact native read-only predicate. Native SPECIAL 367 owns
                // enabling all three National Dex fields; this evaluator only
                // observes the recipient's existing magic, flags and variable.
                requirePokemon(special == 403,("Map setup uses an unsupported native special getter ("+std::to_string(special)+")").c_str());
                const auto memory = readSaveMemory(c);
                assign(result,(nationalDexAfter || c->busRead8(c,memory.block2 + 0x1B) == 0xB9) &&
                       p.var(0x404E) == 0x6258 && p.flag(0x840));
                break;
            }
            case 0x25: {
                const auto special = half(address); address += 2;
                if(special == 309 && p.mapGroup == 1 && p.mapNum == 111) {
                    // Icefall's native ON_LOAD marks cracked ice from exactly
                    // nine OWN temporary flags. Campaign receipts must leave
                    // those flags and the already loaded holes/ice alone.
                    // Record the complete dependency rather than replay the
                    // helper, which would replace holes with cracked ice.
                    auto& inputs=effects.retainedNativeInputs[special];
                    for(unsigned flag=1;flag<=9;++flag)inputs.push_back(unsigned(p.flag(flag)));
                    break;
                }
                if(special == 441 && p.mapGroup == 35 && p.mapNum == 4) {
                    // Lorelei's dolls derive only from this player's encrypted
                    // Hall-of-Fame count (stat10). Native room entry already
                    // created that collection; shared quests never copy it.
                    const auto memory=readSaveMemory(c);
                    const auto clears=inventoryRead32(c,memory.block1+0x1228)^memory.key;
                    requirePokemon(clears<=0xFFFFFFu,"Invalid own Hall-of-Fame counter in Lorelei's house");
                    effects.retainedNativeInputs[special]={clears};
                    for(unsigned i=0;i<8;++i)if(clears>=25*(i+1)) {
                        p.flag(0xA5+i,false);effects.flags[0xA5+i]=false;
                    }
                    break;
                }
                // Reentering Vermilion's transition script would reroll its
                // random trash-can puzzle. The already loaded map stores its
                // own two choices in TEMP0/1. Preserve those choices and expose
                // them only to the read-only evaluator's matching COPYVARs.
                requirePokemon(special == 347 && p.mapGroup == 9 && p.mapNum == 6,
                               "Map setup uses an unsupported native special");
                const auto first = p.var(0x4000), second = p.var(0x4001);
                const auto distance = first > second ? first - second : second - first;
                requirePokemon(first >= 1 && first <= 15 && second >= 1 && second <= 15 &&
                    (distance == 5 || (distance == 1 && (first - 1) / 5 == (second - 1) / 5)),
                    "Loaded Vermilion puzzle choices are not initialized native neighbors");
                assign(0x8004,first); assign(0x8005,second);
                break;
            }
            case 0x33: // PLAYBGM: the campaign update is not a map-entry music event.
                (void)half(address); (void)byte(address + 2); address += 3; break;
            case 0x34: // SAVEBGM: retain the recipient's own active field music.
            case 0x9F: // SETRESPAWN: retain the recipient's own last healing location.
                (void)half(address); address += 2; break;
            case 0xC4: // SETESCAPEWARP: retain this player's own arrival/escape route.
                (void)byte(address); (void)byte(address + 1); (void)byte(address + 2);
                (void)operand(address + 3); (void)operand(address + 5); address += 7; break;
            case 0x42: { // GETPLAYERXY: read this player's own fixed native coordinates.
                const auto xVar = half(address), yVar = half(address + 2); address += 4;
                const auto memory = readSaveMemory(c);
                assign(xVar,inventoryRead16(c,memory.block1)); assign(yVar,inventoryRead16(c,memory.block1 + 2));
                break;
            }
            case 0xA2: {
                const auto x = operand(address), y = operand(address + 2), value = operand(address + 4), collision = operand(address + 6);
                address += 8;
                requirePokemon(x < 2048 && y < 2048 && value <= 0x0FFF && collision <= 1,
                               "Invalid map setup metatile");
                effects.tiles[{x,y}] = {uint16_t(value), bool(collision)}; break;
            }
            case 0xA4: {
                const auto weather=operand(address);address+=2;
                // The seven Tanoby chambers use only NONE/SHADE. Other weather
                // may change encounter/rain bookkeeping and is not covered by
                // this narrowly reviewed map-lighting operation.
                requirePokemon(p.mapGroup==2 && p.mapNum>=27 && p.mapNum<=33 && (weather==0 || weather==11),
                               "Map setup uses an unsupported native weather operation");
                effects.weather=weather;break;
            }
            case 0xA5:
                requirePokemon(p.mapGroup==2 && p.mapNum>=27 && p.mapNum<=33 && effects.weather,
                               "Map setup applies weather without reviewed chamber setup");
                effects.applyWeather=true;break;
            case 0xA7: {
                const auto layout=operand(address);address+=2;
                requirePokemon(p.mapGroup==1 && (p.mapNum==86 || p.mapNum==87) && layout && layout<=1024,
                               "Map setup uses an unsupported native layout operation");
                // A local solved Seafoam puzzle changes its layout on entry.
                // The planner separately validates its exact original/current
                // variant and any required live-grid entry transaction.
                effects.layoutId=layout;break;
            }
            case 0xD0:
                // World-map preview and quest-log entry bookkeeping has no
                // effect on current objects/metatiles. Do not replay it.
                (void)half(address); address += 2; break;
            default:
                throw std::runtime_error("Map setup uses an unsupported native/script operation (opcode " +
                                         std::to_string(op) + ")");
            }
        }
        throw std::runtime_error("Map refresh exceeded its script instruction budget");
    }
private:
    mCore* c;
    Progress p;
    bool nationalDexAfter = false;
    Effects effects;
    std::map<unsigned,unsigned> temporary;
    unsigned comparison = 3;
    unsigned byte(uint32_t a) const {
        requirePokemon(rom(a), "Map setup script pointer is outside the verified ROM");
        return c->busRead8(c, a);
    }
    unsigned half(uint32_t a) const { return byte(a) | (byte(a + 1) << 8); }
    uint32_t longword(uint32_t a) const { return half(a) | (uint32_t(half(a + 2)) << 16); }
    unsigned variable(unsigned id) const {
        if (id >= 0x4000 && id < 0x4100) return p.var(id);
        const auto found = temporary.find(id);
        requirePokemon(found != temporary.end(), "Map setup depends on an uninitialized special variable");
        return found->second;
    }
    unsigned operand(uint32_t a) const {
        const auto n = half(a); return n < 0x4000 ? n : variable(n);
    }
    void assign(unsigned id,unsigned value) {
        if (id >= 0x8000 && id <= 0x8014) temporary[id] = value;
        else {
            requirePokemon(id >= 0x4000 && id < 0x4100,"Map refresh writes an unsupported script variable");
            p.var(id,uint16_t(value)); effects.variables[id] = uint16_t(value);
        }
    }
    void recordLeaf(uint32_t target) {
        auto address = target;
        std::map<std::pair<unsigned,unsigned>,Tile> tiles;
        // This recognizer accepts ONLY a flat original-ROM sequence of literal
        // SETMETATILEs followed by RETURN. No nested calls, branches, native
        // routines, variable operands or other engine side effects qualify.
        for (unsigned count = 0; count < 64 && rom(address); ++count) {
            const auto op = byte(address++);
            if (op == 3) {
                if (!tiles.empty()) effects.tileLeaves[target] = std::move(tiles);
                return;
            }
            if (op != 0xA2 || !rom(address,8)) return;
            const auto x = half(address), y = half(address + 2), value = half(address + 4), collision = half(address + 6);
            address += 8;
            if (x >= 2048 || y >= 2048 || value > 0xFFF || collision > 1 || tiles.count({x,y})) return;
            tiles[{x,y}] = {uint16_t(value),bool(collision)};
        }
    }
    static void push(std::vector<uint32_t>& stack, uint32_t a) {
        requirePokemon(stack.size() < 20, "Map refresh exceeded its script call budget"); stack.push_back(a);
    }
};

inline uint32_t mapScript(mCore* core, uint32_t table, unsigned type) {
    if (!table) return 0;
    for (unsigned i = 0; i < 16; ++i) {
        requirePokemon(rom(table, 5), "Native map script table is outside its verified ROM");
        const auto tag = core->busRead8(core, table);
        if (!tag) return 0;
        requirePokemon(tag <= 7, "Invalid native map script type");
        if (tag == type) {
            const auto script = inventoryRead32(core, table + 1);
            requirePokemon(rom(script), "Native map setup pointer is outside its verified ROM"); return script;
        }
        table += 5;
    }
    throw std::runtime_error("Native map script table lacks a bounded terminator");
}

inline Effects mapEffects(mCore* core, uint32_t table, const Progress& state, bool nativeNationalDexAfter = false) {
    Evaluator evaluator(core,state,nativeNationalDexAfter);
    (void)evaluator.run(mapScript(core, table, 3));
    // Native transition precedes ON_LOAD; derived flags and variables must be
    // visible to its conditional door setup, without mutating the real core.
    return evaluator.run(mapScript(core, table, 1));
}
} // namespace campaign_runtime_detail

// Preflight only. A false-ready plan has performed no RAM writes. The caller
// must defer the ENTIRE associated campaign delta, including flags, on failure.
inline CampaignObjectRefreshPlan planCampaignObjectRefresh(mCore* core, const Progress& before, const Progress& after,
                                                          bool nativeNationalDexAfter = false) {
    CampaignObjectRefreshPlan plan;
    try {
        using namespace campaign_runtime_detail;
        requirePokemon(campaignFieldQuiescent(core), "Campaign refresh waits for a stationary idle overworld");
        requirePokemon(before.mapGroup == after.mapGroup && before.mapNum == after.mapNum,
                       "Campaign refresh cannot relocate a player");
        requirePokemon(readProgress(core) == before, "Campaign refresh baseline changed");
        plan.expected = after;
        if (before.flags == after.flags && before.vars == after.vars && !nativeNationalDexAfter) {
            // Bag-only receipts and metadata updates cannot change map setup.
            // Do not inspect/replay unrelated native map entry operations.
            plan.ready = true;
            return plan;
        }
        auto remember = [&](uint32_t address, unsigned n) {
            for (unsigned i = 0; i < n; ++i) plan.preimages[address + i] = uint8_t(core->busRead8(core, address + i));
        };
        remember(CampaignMapHeaderAddress, 0x1C); remember(CampaignObjectsAddress, 16 * 0x24);
        remember(CampaignAvatarAddress, 8);
        const auto memory = readSaveMemory(core);
        remember(memory.block1,4); remember(memory.block2 + 0x1B,1);
        remember(memory.block1+0x2E,1);remember(memory.block1+0x32,2);remember(memory.block1+0x1228,4);
        const auto events = inventoryRead32(core, CampaignMapHeaderAddress + 4);
        const auto scripts = inventoryRead32(core, CampaignMapHeaderAddress + 8);
        const auto layout = inventoryRead32(core, CampaignMapHeaderAddress);
        requirePokemon(rom(events, 20) && rom(layout, 28), "Native campaign map header is outside its verified ROM");
        const unsigned count = core->busRead8(core, events);
        const auto templates = inventoryRead32(core, events + 4);
        requirePokemon(count <= 64 && (!count || rom(templates, count * 0x18)), "Invalid native campaign object table");
        remember(memory.block1 + 0x8E0, count * 0x18);
        const auto oldEffects = mapEffects(core, scripts, before), newEffects = mapEffects(core, scripts, after,nativeNationalDexAfter);
        requirePokemon(oldEffects.retainedNativeInputs==newEffects.retainedNativeInputs,
                       "Campaign refresh must preserve this player's native ice/personal room state");
        requirePokemon(oldEffects.layoutId==newEffects.layoutId,
                       "Campaign refresh must preserve this player's native layout puzzle");
        for(unsigned flag:{0x46u,0x47u,0x4Cu,0x4Du,0x2D2u,0x2D3u})
            if(before.mapGroup==1 && (before.mapNum==86||before.mapNum==87))
                requirePokemon(before.flag(flag)==after.flag(flag),"Campaign refresh cannot copy another player's Seafoam puzzle");
        requirePokemon((!oldEffects.weather || oldEffects.applyWeather) && (!newEffects.weather || newEffects.applyWeather),
                       "Campaign refresh requires the chamber's complete native lighting setup");
        std::map<unsigned,Object> objects;
        for (unsigned i = 0; i < count; ++i) {
            const auto original = templates + i * 0x18, saved = memory.block1 + 0x8E0 + i * 0x18;
            Object o;
            o.id = core->busRead8(core, original); o.graphics = core->busRead8(core,original + 1);
            o.kind = core->busRead8(core, original + 2);
            requirePokemon(o.id && o.id < 255 && !objects.count(o.id), "Invalid or duplicate native campaign object ID");
            requirePokemon(core->busRead8(core, saved) == o.id, "Native saved object templates do not match their map");
            // Clone templates resolve flags/graphics in their connected map.
            // Their hidden-state mutations need that connected-map evidence.
            o.flag = inventoryRead16(core, saved + 0x14);
            requirePokemon(o.flag < ProgressFlagBytes * 8, "Campaign object uses an unsupported temporary hide flag");
            o.x = signedHalf(inventoryRead16(core, original + 4)); o.y = signedHalf(inventoryRead16(core, original + 6));
            o.movement = core->busRead8(core, original + 9);
            objects[o.id] = o;
        }
        std::map<unsigned,unsigned> active;
        unsigned activeCount = 0;
        for (unsigned i = 0; i < 16; ++i) {
            const auto a = CampaignObjectsAddress + i * 0x24;
            if (!(inventoryRead32(core, a) & 1u)) continue;
            ++activeCount;
            if (core->busRead8(core, a + 9) != before.mapNum || core->busRead8(core, a + 10) != before.mapGroup) continue;
            const auto id = core->busRead8(core, a + 8);
            requirePokemon(!active.count(id), "Native map has duplicate active object IDs"); active[id] = i;
        }
        const int cameraX = signedHalf(inventoryRead16(core, memory.block1)), cameraY = signedHalf(inventoryRead16(core, memory.block1 + 2));
        std::vector<uint8_t> derived, removals, placements, additions, tiles, weather, layoutCode;
        if(oldEffects.weather!=newEffects.weather || oldEffects.applyWeather!=newEffects.applyWeather) {
            const auto originalWeather=core->busRead8(core,CampaignMapHeaderAddress+0x16);
            requirePokemon(before.mapGroup==2 && before.mapNum>=27 && before.mapNum<=33 && originalWeather==0,
                           "Chamber lighting requires its original clear-weather map header");
            if(newEffects.weather) {weather.push_back(0xA4);word(weather,*newEffects.weather);}
            else weather.push_back(0xA3); // Native reset to the original map header weather.
            weather.push_back(0xA5);plan.changedWeather=true;
        }
        std::set<unsigned> changedGraphics;
        for (const auto& v : newEffects.variables) {
            const auto old = oldEffects.variables.find(v.first);
            const auto oldValue = old == oldEffects.variables.end() ? before.var(v.first) : old->second;
            if (v.second == oldValue || v.second == after.var(v.first)) continue;
            derived.push_back(0x16); word(derived,v.first); word(derived,v.second);
            if (v.first >= 0x4010 && v.first <= 0x401F) changedGraphics.insert(v.first - 0x4010);
        }
        for (const auto& f : newEffects.flags) {
            const auto old = oldEffects.flags.find(f.first);
            const auto oldValue = old == oldEffects.flags.end() ? before.flag(f.first) : old->second;
            if (f.second == oldValue || f.second == after.flag(f.first)) continue;
            derived.push_back(f.second ? 0x29 : 0x2A); word(derived,f.first);
        }
        for (const auto& p : newEffects.coordinates) requirePokemon(objects.count(p.first),"Map setup targets an unknown local object ID");
        for (const auto& p : newEffects.movements) requirePokemon(objects.count(p.first),"Map setup targets an unknown local object ID");
        for (const auto& p : oldEffects.coordinates) requirePokemon(objects.count(p.first),"Map setup targets an unknown local object ID");
        for (const auto& p : oldEffects.movements) requirePokemon(objects.count(p.first),"Map setup targets an unknown local object ID");
        const auto playerObject = CampaignObjectsAddress + core->busRead8(core,CampaignAvatarAddress + 5) * 0x24;
        const int playerX = signedHalf(inventoryRead16(core,playerObject + 0x10)) - 7;
        const int playerY = signedHalf(inventoryRead16(core,playerObject + 0x12)) - 7;
        uint32_t tileLayout=layout;
        if(newEffects.layoutId && *newEffects.layoutId!=inventoryRead16(core,memory.block1+0x32)) {
            // Only the two original Seafoam current-stopped variants are
            // interchangeable without loading graphics, reallocating a grid,
            // changing topology, or restarting this player's map scripts.
            const bool third=before.mapNum==86;
            const unsigned oldId=third?159:160,newId=third?278:279;
            constexpr uint32_t layouts=0x0834EAB0;
            requirePokemon(before.mapGroup==1 && (before.mapNum==86||before.mapNum==87) &&
                inventoryRead16(core,memory.block1+0x32)==oldId && *newEffects.layoutId==newId &&
                inventoryRead32(core,layouts+(oldId-1)*4)==layout,
                "Campaign refresh requires a reviewed original Seafoam layout pair");
            tileLayout=inventoryRead32(core,layouts+(newId-1)*4);
            requirePokemon(rom(tileLayout,28),"Seafoam target layout is outside the verified ROM");
            const auto width=inventoryRead32(core,layout),height=inventoryRead32(core,layout+4);
            requirePokemon(width && height && width<=128 && height<=128 &&
                inventoryRead32(core,tileLayout)==width && inventoryRead32(core,tileLayout+4)==height &&
                inventoryRead32(core,tileLayout+16)==inventoryRead32(core,layout+16) &&
                inventoryRead32(core,tileLayout+20)==inventoryRead32(core,layout+20) &&
                inventoryRead32(core,tileLayout+24)==inventoryRead32(core,layout+24),
                "Seafoam layout change requires identical dimensions, tilesets and border shape");
            const auto border=inventoryRead32(core,layout+8),nextBorder=inventoryRead32(core,tileLayout+8);
            const auto borderBytes=unsigned(core->busRead8(core,layout+24))*core->busRead8(core,layout+25)*2;
            requirePokemon(borderBytes && borderBytes<=128 && rom(border,borderBytes) && rom(nextBorder,borderBytes),
                           "Seafoam layout has an invalid native border");
            for(unsigned i=0;i<borderBytes;++i)
                requirePokemon(core->busRead8(core,border+i)==core->busRead8(core,nextBorder+i),
                               "Seafoam layout change cannot replace connected/border geometry");
            const auto source=inventoryRead32(core,layout+12),target=inventoryRead32(core,tileLayout+12);
            requirePokemon(rom(source,width*height*2)&&rom(target,width*height*2),"Seafoam native tile grid is invalid");
            const unsigned gridBytes=(width+15)*(height+14)*2;
            uint32_t grid=0;
            for(uint32_t a=0x03000000;a<0x03007FF4;a+=4)
                if(inventoryRead32(core,a)==width+15 && inventoryRead32(core,a+4)==height+14) {
                    const auto candidate=inventoryRead32(core,a+8);
                    if(!validSaveBlock(candidate,gridBytes))continue;
                    requirePokemon(!grid,"Seafoam loaded grid is ambiguous");grid=candidate;remember(a,12);
                }
            requirePokemon(grid && gridBytes<=0x5000,"Seafoam loaded native grid is unavailable");
            for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
                const auto index=(y*width+x)*2;
                const auto oldEntry=inventoryRead16(core,source+index),nextEntry=inventoryRead16(core,target+index);
                if(oldEntry==nextEntry)continue;
                const auto address=grid+((y+7)*(width+15)+x+7)*2;
                const auto live=inventoryRead16(core,address);
                requirePokemon(live==oldEntry || live==nextEntry,
                    "Seafoam layout refresh waits for a conflicting own puzzle tile");
                remember(address,2);
                if(live==nextEntry)continue;
                requirePokemon(int(x)!=playerX || int(y)!=playerY,
                    "Seafoam layout refresh waits until the player leaves a changed tile");
                for(const auto& object:active) {
                    const auto a=CampaignObjectsAddress+object.second*0x24;
                    requirePokemon(signedHalf(inventoryRead16(core,a+0x10))-7!=int(x) ||
                        signedHalf(inventoryRead16(core,a+0x12))-7!=int(y),
                        "Seafoam layout refresh waits for an object on a changed tile");
                }
                plan.layoutEntries[address]=nextEntry;
            }
            layoutCode.push_back(0xA7);word(layoutCode,newId);plan.changedLayout=true;
        }
        for (const auto& entry : objects) {
            const auto& o = entry.second;
            auto position = [&](const Effects& effects) {
                const auto found = effects.coordinates.find(o.id);
                return found == effects.coordinates.end() ? std::make_pair(o.x,o.y) : found->second;
            };
            auto movement = [&](const Effects& effects) {
                const auto found = effects.movements.find(o.id);
                return found == effects.movements.end() ? o.movement : found->second;
            };
            auto hidden = [&](const Effects& effects,const Progress& state) {
                const auto found = effects.flags.find(o.flag);
                return o.flag && (found == effects.flags.end() ? state.flag(o.flag) : found->second);
            };
            const bool hiddenBefore = hidden(oldEffects,before), hiddenAfter = hidden(newEffects,after);
            const auto oldPos = position(oldEffects), newPos = position(newEffects);
            const auto oldMovement = movement(oldEffects), newMovement = movement(newEffects);
            const bool moved = oldPos != newPos || oldMovement != newMovement ||
                (o.graphics >= 0xF0 && changedGraphics.count(o.graphics - 0xF0));
            if (hiddenBefore == hiddenAfter && !moved) continue;
            requirePokemon(o.kind == 0, "Changed connected-map clone object requires a reviewed refresh");
            const bool live = active.count(o.id);
            if (live && (hiddenAfter || moved)) {
                removals.push_back(0x53); word(removals, o.id); ++plan.removedObjects; --activeCount;
            }
            if (moved) {
                requirePokemon(newPos.first >= 0 && newPos.second >= 0 && newPos.first < 2048 && newPos.second < 2048,
                               "Changed native map object has invalid placement");
                placements.push_back(0x63); word(placements,o.id); word(placements,unsigned(newPos.first)); word(placements,unsigned(newPos.second));
                placements.push_back(0x65); word(placements,o.id); placements.push_back(uint8_t(newMovement)); ++plan.movedObjects;
            }
            // Native TrySpawnObjectEvents uses this exact camera window; far
            // objects appear normally when the player approaches them later.
            auto spawnPos = newPos;
            if (!moved) {
                for (unsigned i = 0; i < count; ++i) {
                    const auto a = memory.block1 + 0x8E0 + i * 0x18;
                    if (core->busRead8(core,a) != o.id) continue;
                    spawnPos = {signedHalf(inventoryRead16(core,a + 4)),signedHalf(inventoryRead16(core,a + 6))};
                    break;
                }
            }
            const bool visible = spawnPos.first + 7 >= cameraX - 2 && spawnPos.first + 7 <= cameraX + 17 &&
                                 spawnPos.second + 7 >= cameraY && spawnPos.second + 7 <= cameraY + 16;
            if (!hiddenAfter && visible && (!live || moved)) {
                requirePokemon(spawnPos.first != playerX || spawnPos.second != playerY,
                               "Campaign refresh waits until the player leaves an appearing object's tile");
                ++activeCount;
                additions.push_back(0x55); word(additions,o.id); ++plan.addedObjects;
            }
        }
        requirePokemon(activeCount <= 16, "Native campaign object slots are full");
        const auto width = inventoryRead32(core, tileLayout), height = inventoryRead32(core, tileLayout + 4);
        const auto mapTiles = inventoryRead32(core, tileLayout + 12);
        requirePokemon(width && height && width <= 2048 && height <= 2048 &&
                       uint64_t(width) * height <= 0x00800000u && rom(mapTiles, width * height * 2),
                       "Native campaign original map tiles are outside their verified ROM");
        std::set<std::pair<unsigned,unsigned>> positions;
        for (const auto& t : oldEffects.tiles) positions.insert(t.first);
        for (const auto& t : newEffects.tiles) positions.insert(t.first);
        std::map<std::pair<unsigned,unsigned>,Tile> tileChanges;
        for (const auto& pos : positions) {
            requirePokemon(pos.first < width && pos.second < height, "Map setup metatile is outside its original map");
            const Tile original{uint16_t(inventoryRead16(core,mapTiles + (pos.second * width + pos.first) * 2) & 0x0FFFu),false};
            const auto old = oldEffects.tiles.find(pos), next = newEffects.tiles.find(pos);
            const Tile a = old == oldEffects.tiles.end() ? original : old->second;
            const Tile b = next == newEffects.tiles.end() ? original : next->second;
            if (a == b) continue;
            requirePokemon(int(pos.first) != playerX || int(pos.second) != playerY || (!b.impassable && !(b.value & 0xC00u)),
                           "Campaign refresh waits until the player leaves a closing gate tile");
            tiles.push_back(0xA2); word(tiles,pos.first); word(tiles,pos.second); word(tiles,b.value); word(tiles,unsigned(b.impassable));
            tileChanges[pos] = b;
            ++plan.changedTiles;
        }
        if (!derived.empty() || !removals.empty() || !placements.empty() || !additions.empty() || !tiles.empty() || !weather.empty() || !layoutCode.empty()) {
            plan.code.push_back(0x69);
            for (const auto* code : {&removals,&derived,&placements,&additions,&layoutCode,&tiles,&weather}) plan.code.insert(plan.code.end(),code->begin(),code->end());
            if (!tiles.empty() || !layoutCode.empty()) plan.code.insert(plan.code.end(),{0x25,0x8E,0}); // native DrawWholeMapView
            plan.code.insert(plan.code.end(),{0x6B,0x02});
            if (plan.code.size() > 80 && derived.empty() && removals.empty() && placements.empty() && additions.empty() && weather.empty() && layoutCode.empty()) {
                auto uncovered = tileChanges;
                std::vector<uint8_t> calls;
                for (const auto& leaf : newEffects.tileLeaves) {
                    bool safe = true, useful = false;
                    for (const auto& t : leaf.second) {
                        const auto wanted = newEffects.tiles.find(t.first);
                        if (wanted == newEffects.tiles.end() || wanted->second != t.second) { safe = false; break; }
                        if (uncovered.count(t.first)) useful = true;
                    }
                    if (!safe || !useful) continue;
                    calls.push_back(0x04);
                    for (unsigned i = 0; i < 4; ++i) calls.push_back(uint8_t(leaf.first >> (8 * i)));
                    for (const auto& t : leaf.second) uncovered.erase(t.first);
                    plan.originalTileLeaves.push_back(leaf.first);
                }
                if (uncovered.empty() && !calls.empty()) {
                    // CALL stores its return pointer in ctx.stack[0]. Reserve
                    // the first five stack slots, and place our small wrapper
                    // AFTER them. Each verified leaf returns before the next
                    // CALL, so maximum actual depth is one; no bytecode or VM
                    // table can overlap a native return address.
                    plan.programOffset = 32;
                    plan.code = {0x69}; plan.code.insert(plan.code.end(),calls.begin(),calls.end());
                    plan.code.insert(plan.code.end(),{0x25,0x8E,0,0x6B,0x02});
                } else plan.originalTileLeaves.clear();
            }
            requirePokemon(plan.code.size() <= plan.capacity(), "Campaign map refresh exceeds its bounded native script workspace");
            const auto table = inventoryRead32(core,GlobalScriptContextAddress + 0x5C), end = inventoryRead32(core,GlobalScriptContextAddress + 0x60);
            requirePokemon(rom(table) && end > table && end <= 0x09000000 && end - table >= 0xA3 * 4,
                           "Native campaign command table is not initialized");
            remember(GlobalScriptContextAddress + 0x5C,8);
        }
        plan.ready = true;
    } catch (const std::runtime_error& failure) { plan.reason = failure.what(); plan.code.clear(); plan.layoutEntries.clear(); }
    return plan;
}

// Called after the associated flags/vars transaction, with both cores still
// stopped. No instruction may run between planning, the delta and this start.
// False means the caller must roll the ENTIRE paired transaction back.
inline bool startCampaignObjectRefresh(mCore* core, const CampaignObjectRefreshPlan& plan) {
    if (!plan.ready || !campaignFieldQuiescent(core) || !(readProgress(core) == plan.expected)) return false;
    for (const auto& image : plan.preimages)
        if (core->busRead8(core,image.first) != image.second) return false;
    if (plan.code.empty()) return plan.layoutEntries.empty();
    if ((plan.programOffset != 12 && plan.programOffset != 32) || plan.code.size() > plan.capacity()) return false;
    for(const auto& entry:plan.layoutEntries) {
        if((entry.first&1u) || entry.first<0x02000000 || entry.first>0x0203FFFE ||
           !plan.preimages.count(entry.first) || !plan.preimages.count(entry.first+1))return false;
    }
    for(const auto& entry:plan.layoutEntries)inventoryWrite16(core,entry.first,entry.second);
    const auto context = GlobalScriptContextAddress;
    for (unsigned i = 0; i < 80; ++i) core->busWrite8(core,context + 12 + i,0);
    for (unsigned i = 0; i < plan.code.size(); ++i) core->busWrite8(core,context + plan.programOffset + i,plan.code[i]);
    for (unsigned i = 0; i < 12; ++i) core->busWrite8(core,context + i,0);
    for (unsigned i = 0; i < 4; ++i) core->busWrite8(core,context + 8 + i,uint8_t((context + plan.programOffset) >> (8 * i)));
    core->busWrite8(core,context + 1,1);
    core->busWrite8(core,FieldControlsLockAddress,1);
    core->busWrite8(core,GlobalScriptStatusAddress,0);
    return true;
}
} // namespace coop::firered
