// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedProgress.h"
#include <algorithm>

namespace coop::firered {

// Reviewed against pret/pokefirered ce6406d8c2eaf46d543c8a2606de0c3cb1951982:
// include/constants/{flags,vars,items,opponents}.h and the named native map
// scripts below. Numeric metadata only; no original game scripts/assets.
// Runtime ROM precondition SHA-256:
// eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507
// The caller fingerprints and binds an exact supported ROM before using Progress and holds
// BOTH cores at an idle field boundary. This planner never touches emulator RAM.
//
// Exclusions are deliberate: temporary flags/vars, resettable boulder/trash-can/
// mansion puzzles, ordinary pickups, fossil/starter/Pokemon gifts, tutors, dex
// contents, clock/counters and Hall of Fame sectors are each player's own data.
// League room flags are a resettable expedition, not monotonic campaign bits.
inline constexpr const char* CampaignSourceRevision = "ce6406d8c2eaf46d543c8a2606de0c3cb1951982";

struct CampaignGym {
    unsigned badge, defeated, trainer, receipt, machine;
};
inline constexpr std::array<CampaignGym, 7> CampaignGyms{{
    {0x821,0x4B1,415,0x297,291}, // CeruleanCity_Gym: Misty, TM03
    {0x822,0x4B2,416,0x231,322}, // VermilionCity_Gym: Lt. Surge, TM34
    {0x823,0x4B3,417,0x293,307}, // CeladonCity_Gym: Erika, TM19
    {0x824,0x4B4,418,0x259,294}, // FuchsiaCity_Gym: Koga, TM06
    {0x825,0x4B5,420,0x29A,292}, // SaffronCity_Gym: Sabrina, TM04
    {0x826,0x4B6,419,0x24E,326}, // CinnabarIsland_Gym: Blaine, TM38
    {0x827,0x4B7,350,0x298,314}, // ViridianCity_Gym: Giovanni, TM26
}};
// Native data/scripts/set_gym_trainers.inc: defeating a leader also marks
// their ordinary gym trainers. Rows follow CampaignGyms, zero pads unused slots.
inline constexpr std::array<std::array<unsigned,8>,7> CampaignGymTrainers{{
    {{150,234,0,0,0,0,0,0}}, {{141,220,423,0,0,0,0,0}},
    {{132,133,160,265,266,267,402,0}}, {{294,295,288,289,292,293,0,0}},
    {{280,281,282,283,462,463,464,0}}, {{177,178,179,180,213,214,215,0}},
    {{296,297,322,323,324,392,400,401}},
}};
struct CampaignApplication {
    Progress progress;
    bool changed = false;
    bool deferredRewards = false;
    bool conflict = false;
    // These MUST be performed by native engine operations, with their own
    // rollback/save verification; setting flag 840/82C here would be incomplete.
    bool enableNationalDex = false;
    bool enterHallOfFame = false;
    bool completeNetwork = false; // Native SetPostgameFlags + own InitRoamer, then flag844.
    std::vector<unsigned> changedFlags;
    std::vector<unsigned> changedVars;
    std::vector<unsigned> deliveredReceipts;
};

inline bool campaignReady(const Progress& p) {
    return p.flag(FlagPokemonObtained) && p.flag(FlagPokedexObtained) &&
           p.var(VarLabScene) >= 6 && p.var(0x4031) <= 2;
}

inline CampaignApplication reconcileCampaign(const Progress& source, const Progress& recipient) {
    source.inventory.validate(); recipient.inventory.validate();
    CampaignApplication result; result.progress = recipient;
    if (!campaignReady(source) || !campaignReady(recipient)) return result;
    auto& p = result.progress;
    auto set = [&](unsigned flag) { p.flag(flag, true); };
    auto clear = [&](unsigned flag) { p.flag(flag, false); };
    auto atLeast = [&](unsigned id, unsigned stage) {
        if (p.var(id) < stage) p.var(id, uint16_t(stage));
    };
    auto sourceStage = [&](unsigned id, unsigned maximum) {
        const auto value = source.var(id);
        if (value > maximum || p.var(id) > maximum) { result.conflict = true; return 0u; }
        return unsigned(value);
    };
    // Each receipt is the recipient's original native persistent ledger. A
    // consumable reward can therefore be used/sold without being recreated on
    // the next frame or after a restart. Bag failures keep that receipt unset.
    auto grant = [&](unsigned receipt, unsigned item, bool unique = false) {
        if (p.flag(receipt)) return true;
        auto proposed = p.inventory;
        const bool add = !unique || proposed.quantity(item) == 0;
        const bool newPouch = add && itemPocket(item) == Berries && proposed.quantity(item) == 0 && proposed.quantity(365) == 0;
        try { if (newPouch) proposed.setQuantity(365,1); }
        catch (const std::runtime_error&) { result.deferredRewards = true; return false; }
        if (add && !tryAddStoryItem(proposed,item,1)) {
            result.deferredRewards = true; return false;
        }
        p.inventory = proposed;
        if (newPouch) set(0x847); // Native AddBagItem creates Berry Pouch + receipt.
        set(receipt); result.deliveredReceipts.push_back(receipt); return true;
    };
    auto key = [&](unsigned item) {
        if (p.inventory.quantity(item)) return true;
        if (!tryAddStoryItem(p.inventory,item,1)) { result.deferredRewards = true; return false; }
        return true;
    };
    auto remove = [&](unsigned item) { p.inventory.setQuantity(item,0); };
    auto rival = [&](unsigned firstTrainer) {
        // Native scripts choose Squirtle/Bulbasaur/Charmander for player
        // Charmander/Squirtle/Bulbasaur, respectively. Never borrow the host's
        // starter variable or the host's defeated variant.
        set(0x500 + firstTrainer + 2 - p.var(0x4031));
    };

    if (source.flag(0x82F)) { set(0x82F); set(0x092); } // Running Shoes; PewterCity native gift

    for (const auto& gym : CampaignGyms) {
        if (!source.flag(gym.badge) || !source.flag(gym.defeated)) continue;
        set(gym.badge); set(gym.defeated); set(0x500 + gym.trainer);
        for (const auto trainer:CampaignGymTrainers[gym.badge-0x821]) if (trainer) set(0x500+trainer);
        grant(gym.receipt,gym.machine);
        if (gym.badge == 0x822) { clear(0x0A0); clear(0x0A1); set(0x264); }
        if (gym.badge == 0x823) clear(0x09E);
        if (gym.badge == 0x824) clear(0x09F);
        if (gym.badge == 0x825) clear(0x0AE);
        if (gym.badge == 0x826 && p.var(0x4071) == 0) { p.var(0x4071,1); clear(0x062); }
        if (gym.badge == 0x827) { set(0x0AD); atLeast(0x4054,3); }
    }

    // The optional first Route 22 rival is a continued native scene. Wait for
    // both its trainer receipt and completed scene stage: a later badge or a
    // different rival scene can advance the stage without this fight. Each
    // recipient gets the variant selected by their own starter, no prize or
    // partner inventory, and never repeats an unfinished source scene.
    const bool earlyRoute22Win = source.flag(0x500 + 329) ||
        source.flag(0x500 + 330) || source.flag(0x500 + 331);
    if (earlyRoute22Win && sourceStage(0x4054,4) >= 2) {
        atLeast(0x4054,2); rival(329);
    }

    // Cerulean rival / Bill: progression and own reusable key reward. Bill's
    // transformation and his ticket are separate native conversations.
    if (source.flag(0x233)) { set(0x233); set(0x032); clear(0x033); }
    if (source.flag(0x234) && source.flag(0x235) && source.flag(0x233)) {
        if (grant(0x234,265,true)) { set(0x235); set(0x031); set(0x834); }
    }
    if (sourceStage(0x4052,1) == 1 && source.flag(0x29B)) {
        // The scene grants Fame Checker before ending. Do not suppress the
        // guest's native scene until the guest has its own item.
        if (grant(0x29B,363,true)) { atLeast(0x4052,1); atLeast(0x4054,2); set(0x03C); rival(332); }
    }
    if (sourceStage(0x406B,1) == 1) atLeast(0x406B,1); // Nugget Bridge Rocket
    if (sourceStage(0x408B,1) == 1) atLeast(0x408B,1); // Mt Moon Miguel; fossils remain personal

    // SS Anne Captain and later rival cutscenes. Ship departure itself remains
    // native on each profile, so a guest inside is never stranded by hiding it.
    if (source.flag(0x237) && grant(0x237,339,true)) {
        if (p.var(0x407E) == 0) p.var(0x407E,1);
    }
    if (sourceStage(0x405B,1) == 1) {
        atLeast(0x405B,1); set(0x03D);
        // LavenderTown also bypasses this rival without a battle.
        if (source.flag(0x500+426) || source.flag(0x500+427) || source.flag(0x500+428)) rival(426);
    }
    if (sourceStage(0x405D,1) == 1) { atLeast(0x405D,1); set(0x051); rival(429); }
    if (sourceStage(0x405C,1) == 1) { atLeast(0x405C,1); set(0x04E); rival(432); }
    if (source.flag(0x827) && sourceStage(0x4054,4) == 4) { atLeast(0x4054,4); rival(435); }

    // Source native receipt -> own HM. TM Case creation is the same atomic
    // operation as other native machine rewards (see tryAddStoryItem).
    for (const auto& entry : std::array<std::pair<unsigned,unsigned>,6>{{
            {0x238,340},{0x239,341},{0x23A,342},{0x23B,343},{0x2EF,344},{0x1F1,345}}}) {
        if (!source.flag(entry.first)) continue;
        if (grant(entry.first,entry.second,true) && entry.first == 0x23A) remove(353); // Gold Teeth returned
    }
    if (source.flag(0x241) && !p.flag(0x271)) grant(0x241,352,true);
    if (source.flag(0x271) && grant(0x271,360,true)) { set(0x241); remove(352); }
    if (source.flag(0x2A6)) grant(0x2A6,369,true);
    // Gate scene is one-way: guards have accepted Tea. Keep their personal
    // conversation/position variables out of the general monotonic list.
    if (source.flag(0x2A6) && sourceStage(0x4062,1) == 1) atLeast(0x4062,1);

    // Rocket key pickups are the few item-ball flags deliberately included:
    // their keys gate the campaign. All ordinary pickup flags remain private.
    if (source.flag(0x2A5) && (source.inventory.quantity(356) || source.flag(0x036))) {
        if (key(356)) { set(0x2A5); set(0x036); }
    }
    if (source.inventory.quantity(359)) { if (key(359)) set(0x037); }
    if (source.flag(0x192)) grant(0x192,355,true); // SilphCo_5F Card Key
    if (source.flag(0x1A8)) grant(0x1A8,351,true); // Mansion B1F Secret Key
    if (source.flag(0x189) && !p.flag(0x23A)) grant(0x189,353,true);
    if (source.flag(0x26D)) set(0x26D); // Game Corner poster entrance opened
    if (source.flag(0x05F)) { set(0x05F); set(0x038); set(0x500+348); }
    // RocketHideout_B4F: these two continued trainer scenes only open the
    // Giovanni barrier. Native source trainer flags are sampled after its
    // script releases field control; no item or additional prize is granted.
    // Both flags are required by that map's original ON_LOAD door setup.
    for (const unsigned trainer : {366u,367u})
        if (source.flag(0x500+trainer)) set(0x500+trainer);
    // Door bits are persistent opened doors, unlike boulders/mansion switches.
    // Owning Card Key prevents a partial source receipt from opening all doors.
    if (source.inventory.quantity(355) && p.inventory.quantity(355))
        for (unsigned flag=0x27A;flag<=0x28D;++flag) if (source.flag(flag)) set(flag);
    if (sourceStage(0x407D,1) == 1 && source.flag(0x23F)) {
        if (grant(0x23F,316)) { atLeast(0x407D,1); set(0x03B); }
    }
    if (sourceStage(0x4060,1) == 1 && source.flag(0x03E)) {
        atLeast(0x4060,1); set(0x03E); clear(0x03F); set(0x053); set(0x500+349);
    }
    if (source.flag(0x250) && source.var(0x4060) == 1) grant(0x250,1); // personal Master Ball

    // Tower rescue/Flute/roadblocks. No legendary/static Pokemon or their dex
    // receipts are copied. These two Snorlax bits represent roadblocks cleared;
    // the caught Pokemon remains only with the player who threw the ball.
    if (sourceStage(0x4059,1) == 1) atLeast(0x4059,1); // Marowak's spirit
    // PokemonTower_7F continued battles move each grunt away before native
    // REMOVEOBJECT writes its own hide receipt. Require BOTH receipts: a
    // defeated trainer alone does not prove the exit movement has completed.
    for (const auto& grunt : std::array<std::pair<unsigned,unsigned>,3>{{
            {369,0x05E},{370,0x083},{371,0x084}}}) {
        if (source.flag(0x500+grunt.first) && source.flag(grunt.second)) {
            set(0x500+grunt.first); set(grunt.second);
        }
    }
    if (source.flag(0x23C)) { set(0x23C); set(0x034); clear(0x035); }
    if (source.flag(0x23C) && source.flag(0x23D)) grant(0x23D,350,true);
    if (source.flag(0x23D)) {
        if (source.flag(0x054)) { set(0x054); if (source.flag(0x253)) set(0x253); }
        if (source.flag(0x080)) set(0x080);
    }

    // Sevii detour. OneIsland PokemonCenter stage is ordered 0..7: meet Celio,
    // Meteorite assignment, delivery, Bill leaves, Ruby request, Rainbow Pass,
    // Sapphire installed, next visit. Never max-copy the other map scene vars:
    // several are counters or are reset by an individual scene.
    const unsigned celio = sourceStage(0x4076,7);
    if (source.flag(0x845) && celio >= 1) {
        bool pass = true;
        if (!p.flag(0x846)) pass = key(367); // Keep pass until exchange succeeds.
        if (pass) {
            set(0x845); atLeast(0x4076,1); atLeast(0x4075,3); set(0x071); clear(0x072); clear(0x073);
            // PC outage 841 is cleared already on visiting ThreeIsland_Port;
            // it is not implied by Celio stage1 and must never be recreated.
            if (celio < 2 && p.var(0x4076) < 2) key(280);
        }
    }
    if (source.flag(0x2A3)) {
        // BerryForest always rescues Lostelle after the Hypno scene. Its
        // optional Iapapa Berry is best effort: the native full-bag branch
        // still hides Lostelle and completes the rescue, with no later claim.
        // Flag2A3 therefore acknowledges the scene even if this own gift fails.
        const bool earlierDeferred=result.deferredRewards;
        if (!grant(0x2A3,147)) result.deferredRewards=earlierDeferred;
        set(0x2A3);
        set(0x07A); clear(0x075);
        if (p.var(0x4079) < 2) p.var(0x4079,2);
    }
    if (source.flag(0x2FC)) grant(0x2FC,19); // Own Biker-rescue Full Restore receipt
    if (source.var(0x407B) == 4) { atLeast(0x407B,4); set(0x079); set(0x07E); }
    if (celio >= 2 && source.flag(0x2A3)) {
        atLeast(0x4076,2); remove(280); atLeast(0x4079,3); set(0x075); clear(0x076);
        if (!grant(0x2FB,94)) set(0x2FE); // Native delayed Moon Stone claim remains available.
    }
    if (source.flag(0x2A1)) { set(0x2A1); clear(0x841); } // ThreeIsland_Port transition
    if (source.flag(0x2A1) && celio >= 3) {
        atLeast(0x4076,3); atLeast(0x4071,3); set(0x072); clear(0x062);
    }
    const bool postgameReady = p.flag(0x82C) && p.flag(0x840) && p.var(0x404E) == 0x6258;
    if (postgameReady && celio >= 4 && source.flag(0x82C) && source.flag(0x840) && p.flag(0x845)) {
        atLeast(0x4076,4); atLeast(0x407F,1);
        if (source.var(0x407F) == 2) atLeast(0x407F,2); // First warehouse password scene
        if (source.flag(0x089)) set(0x089); // Both Ember grunts departed
    }
    if (postgameReady && source.flag(0x2DD) && celio == 4 && p.var(0x4076) < 5) {
        if (grant(0x2DD,373,true)) set(0x08A);
    }
    if (postgameReady && celio >= 5 && source.flag(0x846)) {
        // Exchange one key for another atomically; removing Tri Pass first
        // frees its pocket slot even when the recipient's key pocket is full.
        auto old = p.inventory; remove(367); remove(373);
        if (key(368)) {
            set(0x845); set(0x846); set(0x2DD); set(0x08A); set(0x089); atLeast(0x4076,5); clear(0x841);
        } else p.inventory = old;
    }
    if (postgameReady && source.var(0x4080) == 1) { atLeast(0x4080,1); set(0x08B); set(0x08D); set(0x08E); clear(0x08C); }
    if (postgameReady && source.flag(0x2E3)) set(0x2E3); // native Cut opened Dotted Hole
    if (postgameReady && source.flag(0x2D8)) { set(0x2D8); set(0x090); set(0x08F); }
    if (postgameReady && source.flag(0x2D6)) set(0x2D6); // both warehouse passwords learned
    if (postgameReady && source.flag(0x2D5)) { set(0x2D5); set(0x088); atLeast(0x4088,1); }
    if (postgameReady && source.flag(0x2DC) && celio == 5 && p.var(0x4076) < 6) grant(0x2DC,374,true);
    if (postgameReady && source.flag(0x844) && celio >= 6 && p.flag(0x846)) {
        result.completeNetwork = !p.flag(0x844);
        // The native worker transaction owns Sapphire removal, stage6,
        // Cerulean guard/Lorelei objects and receipt844 AFTER successful
        // SetPostgameFlags/InitRoamer. Leave every one untouched here.
    }
    if (postgameReady && source.flag(0x849)) set(0x849); // Tanoby Key solved; native local boulders stay local

    result.enableNationalDex = source.flag(0x840) && source.var(0x404E) == 0x6258 && !recipient.flag(0x840);
    result.enterHallOfFame = source.flag(0x82C) && !recipient.flag(0x82C);
    p.inventory.validate();
    for (unsigned flag=0;flag<ProgressFlagBytes*8;++flag)
        if (p.flag(flag) != recipient.flag(flag)) result.changedFlags.push_back(flag);
    for (unsigned id=0x4000;id<0x4100;++id)
        if (p.var(id) != recipient.var(id)) result.changedVars.push_back(id);
    result.changed = !(p == recipient);
    return result;
}

} // namespace coop::firered
