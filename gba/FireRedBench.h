// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedPokemon.h"

namespace coop::firered {

// Six native party slots are a battle working set, not both players' complete
// rosters. This planner handles safe exhaustion refills; voluntary selection is
// planned separately in FireRedPartyMenu.h. Pending native references remain
// resident. Complete twelve-Pokemon virtualization is not yet implemented.
struct BenchReferences {
    std::array<unsigned, 2> active{{6, 6}};
    std::array<unsigned, 2> queued{{6, 6}};
    std::array<unsigned, 2> callerExcluded{{6, 6}};
    std::array<uint8_t, 4> wishCounters{};
    std::array<unsigned, 4> wishSlots{{0, 0, 0, 0}};
    uint8_t leveled = 0;
    std::array<uint8_t, 2> participation{};
    uint8_t knockedOff = 0;

    std::vector<unsigned> exclusions() const {
        requirePokemon((leveled & ~0x3Fu) == 0 &&
                       (participation[0] & ~0x3Fu) == 0 && (participation[1] & ~0x3Fu) == 0 &&
                       (knockedOff & ~0x3Fu) == 0, "Invalid FireRed bench bit mask");
        std::vector<unsigned> result;
        auto add = [&](unsigned slot) {
            requirePokemon(slot <= 6 || slot == 0xFF, "Invalid FireRed referenced party slot");
            if (slot < 6 && std::find(result.begin(), result.end(), slot) == result.end()) result.push_back(slot);
        };
        for (auto slot : active) add(slot);
        for (auto slot : queued) add(slot);
        for (auto slot : callerExcluded) add(slot);
        // Opponent Wish IDs refer to the enemy party and cannot pin our slots.
        for (unsigned battler : {0u, 2u}) if (wishCounters[battler]) add(wishSlots[battler]);
        for (unsigned slot = 0; slot < 6; ++slot) if (leveled & (1u << slot)) add(slot);
        return result;
    }
};

struct BenchRefill {
    BattleParty transaction;
    Party party;
    BenchReferences references;
    unsigned retiredNativeSlot = 0;
    unsigned importedOriginalSlot = 0;
    Party before{};
};

// A protected six-slot working set must never turn into a native defeat while
// the owner's journal still contains a usable, currently unloaded Pokemon.
inline bool wouldAbandonLivingReserve(const BattleParty& transaction,const Party& current,unsigned owner) {
    requirePokemon(owner<2,"Invalid FireRed reserve owner");
    const auto journal=restoreParties(transaction,current);
    bool anyAlive=false,loadedAlive=false;
    for(unsigned original=0;original<journal[owner].count;++original) {
        const auto& mon=journal[owner].mons[original];
        anyAlive |= mon.viable();
        for(unsigned slot=0;slot<current.count;++slot)
            loadedAlive |= current.mons[slot].identity()==mon.identity() && current.mons[slot].viable();
    }
    return anyAlive && !loadedAlive;
}

inline std::optional<BenchRefill> planBenchRefill(const BattleParty& transaction, const Party& current,
                                                 unsigned owner, const BenchReferences& references) {
    const auto excluded = references.exclusions();
    auto planned = transaction;
    const auto replacement = refillBattleBench(planned, current, owner, excluded);
    if (!replacement) return std::nullopt;
    unsigned changed = 0, destination = 0;
    for (unsigned slot = 0; slot < current.count; ++slot)
        if (current.mons[slot].identity() != replacement->mons[slot].identity()) { ++changed; destination = slot; }
    requirePokemon(changed == 1 && current.mons[destination].hp() == 0,
                   "FireRed refill must replace exactly one fainted bench Pokemon");
    requirePokemon(std::find(excluded.begin(), excluded.end(), destination) == excluded.end(),
                   "FireRed refill retired a referenced Pokemon");
    BenchRefill result{planned, *replacement, references, destination,
                       unsigned(planned.originalSlots[destination]), current};
    // A new Pokemon must not inherit the retired Pokemon's participation or
    // Knock Off restriction. Enemy-side masks are intentionally untouched.
    const uint8_t keep = uint8_t(~(1u << destination));
    result.references.participation[0] &= keep;
    result.references.participation[1] &= keep;
    result.references.knockedOff &= keep;
    return result;
}

// Exact verified German FireRed layout. Call only at the entry of native
// HasNoMonsToSwitch or Cmd_checkteamslost, before either accesses party data,
// while the CPU is paused.
// Generic frame boundaries can interrupt GetMonData while secure data is plain.
inline BenchReferences readBenchReferences(mCore* core, unsigned excludedOne, unsigned excludedTwo) {
    requirePokemon(core && core->busRead8 && core->busRead16 && core->busRead32,
                   "Cannot read FireRed native bench references");
    const uint32_t battle = core->busRead32(core, 0x02023FE8);
    requirePokemon(battle >= 0x02000000 && battle <= 0x0203FE00 && !(battle & 3u),
                   "Invalid FireRed battle structure for bench refill");
    BenchReferences result;
    result.active = {{core->busRead16(core, 0x02023BCE), core->busRead16(core, 0x02023BD2)}};
    result.queued = {{core->busRead8(core, battle + 0x5C), core->busRead8(core, battle + 0x5E)}};
    result.callerExcluded = {{excludedOne, excludedTwo}};
    result.leveled = uint8_t(core->busRead8(core, 0x03004F24));
    for (unsigned b = 0; b < 4; ++b) {
        result.wishCounters[b] = uint8_t(core->busRead8(core, 0x02023F40 + b));
        result.wishSlots[b] = core->busRead8(core, 0x02023F44 + b);
    }
    for (unsigned enemy = 0; enemy < 2; ++enemy)
        result.participation[enemy] = uint8_t(core->busRead8(core, 0x02023F4E + enemy));
    result.knockedOff = uint8_t(core->busRead8(core, 0x02023F49));
    result.exclusions();
    return result;
}

// This updates RAM only. Full machine/battery rollback and transaction publish
// belong to the paired coordinator. Validate the entire plan before any write.
inline void applyBenchRefill(mCore* core, const BenchRefill& plan) {
    requirePokemon(core && core->busWrite8, "Cannot write FireRed native bench refill");
    const auto excluded = plan.references.exclusions();
    plan.before.validate();
    requirePokemon(plan.retiredNativeSlot < plan.party.count && plan.before.count == plan.party.count &&
                   plan.importedOriginalSlot < 6 && plan.party == plan.transaction.combined &&
                   plan.transaction.originalSlots[plan.retiredNativeSlot] == int(plan.importedOriginalSlot) &&
                   std::find(excluded.begin(), excluded.end(), plan.retiredNativeSlot) == excluded.end(),
                   "Invalid FireRed native bench refill plan");
    restoreParties(plan.transaction, plan.party);
    requirePokemon(plan.before.mons[plan.retiredNativeSlot].hp() == 0 &&
                   plan.before.mons[plan.retiredNativeSlot].identity() != plan.party.mons[plan.retiredNativeSlot].identity(),
                   "FireRed native refill did not retire a fainted Pokemon");
    for (unsigned slot = 0; slot < plan.before.count; ++slot)
        requirePokemon(slot == plan.retiredNativeSlot || plan.before.mons[slot] == plan.party.mons[slot],
                       "FireRed native refill changed another party slot");
    const auto bit = 1u << plan.retiredNativeSlot;
    requirePokemon(!(plan.references.participation[0] & bit) && !(plan.references.participation[1] & bit) &&
                   !(plan.references.knockedOff & bit), "FireRed refill retained retired Pokemon's slot masks");
    requirePokemon(readParty(core) == plan.before, "FireRed refill plan has a stale native party snapshot");
    // Native callback entry guarantees no outstanding pointer into secure data.
    writeParty(core, plan.party);
    for (unsigned enemy = 0; enemy < 2; ++enemy)
        core->busWrite8(core, 0x02023F4E + enemy, plan.references.participation[enemy]);
    core->busWrite8(core, 0x02023F49, plan.references.knockedOff);
}
} // namespace coop::firered
