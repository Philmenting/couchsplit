// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedBench.h"
#include <string>

namespace coop::firered {

// Reviewed source: pret/pokefirered ce6406d8c2eaf46d543c8a2606de0c3cb1951982,
// src/{party_menu,battle_controller_player,battle_controllers}.c. Exact DE ROM
// SHA-256 eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507.
// Canonical German metadata; verified English mappings/preimages are in
// FireRedAddressData.h. These are ABI facts, not executable game code.
inline constexpr uint32_t NativePartyMenuReshowAddress = 0x080309DC;
inline constexpr std::array<uint16_t,4> NativePartyMenuReshowPreimage{{0xB500,0x4802,0xF7CF,0xFDB8}};
inline constexpr uint32_t NativePartyMenuExitChosenAddress = 0x0203B0C0;
inline constexpr uint32_t NativePartyMenuSelectedAddress = 0x0203B0C1;
inline constexpr uint32_t NativePartyMenuOrderAddress = 0x0203B0DC;
inline constexpr uint32_t NativeChosenMonReturnAddress = 0x0800E7F8;
inline constexpr std::array<uint16_t,4> NativeChosenMonReturnPreimage{{0xB530,0x1C14,0x0600,0x0E05}};
inline constexpr std::array<uint8_t,3> IdentityPartyMenuOrder{{0x01,0x23,0x45}};

inline void validatePartyMenuOrder(const std::array<uint8_t,3>& order) {
    unsigned mask=0;
    for (const auto byte:order) for (const unsigned slot:{unsigned(byte>>4),unsigned(byte&15)}) {
        requirePokemon(slot<PartyCapacity && !(mask&(1u<<slot)),"Invalid FireRed packed party-menu order");
        mask|=1u<<slot;
    }
    requirePokemon(mask==0x3Fu,"Incomplete FireRed packed party-menu order");
}

struct PartyMenuSession {
    // Rebased ownership/journal and original physical working set. Neither may
    // be replaced by the temporary menu party in the battle transaction.
    BattleParty transaction;
    Party before;
    Party menu;
    BenchReferences references;
    unsigned owner=0;
    unsigned ownActiveOriginal=6;
    // Native UI checks BOTH allied indexes. The foreign active slot must be a
    // sentinel, otherwise it can accidentally hide an unrelated own reserve.
    std::array<unsigned,2> virtualActive{{6,6}};
    std::array<uint8_t,3> savedNativeOrder=IdentityPartyMenuOrder;
};

inline PartyMenuSession planPartyMenu(const BattleParty& transaction,const Party& current,
                                     unsigned owner,const BenchReferences& references,
                                     const std::array<uint8_t,3>& nativeOrder=IdentityPartyMenuOrder) {
    requirePokemon(owner<2,"Invalid FireRed party-menu owner");
    references.exclusions(); validatePartyMenuOrder(nativeOrder);
    const auto journal=restoreParties(transaction,current);
    PartyMenuSession result;
    result.transaction=transaction;result.transaction.originals=journal;result.transaction.combined=current;
    result.before=current;result.menu=journal[owner];result.references=references;result.owner=owner;
    result.savedNativeOrder=nativeOrder;
    // Native summaries/menus may have permuted the physical working array.
    // Ownership remains the original PID+OT association, never slot parity.
    for (unsigned slot=0;slot<current.count;++slot) {
        unsigned old=0;
        while (old<transaction.combined.count && !sameOwnedMon(current.mons[slot],transaction.combined.mons[old])) ++old;
        requirePokemon(old<transaction.combined.count,"Foreign Pokemon in FireRed party-menu working set");
        result.transaction.owners[slot]=transaction.owners[old];
        result.transaction.originalSlots[slot]=transaction.originalSlots[old];
    }
    const auto active=references.active[owner];
    requirePokemon(active<current.count && result.transaction.owners[active]==int(owner),
                   "FireRed party-menu active Pokemon belongs to another trainer");
    result.ownActiveOriginal=unsigned(result.transaction.originalSlots[active]);
    result.virtualActive[owner]=result.ownActiveOriginal;
    requirePokemon(restoreParties(result.transaction,current)==journal,
                   "FireRed party-menu rebase changed a trainer's journal");
    return result;
}

struct PartyMenuSelection {
    BattleParty transaction;
    Party party;
    BenchReferences references;
    bool accepted=false;
    bool canceled=false;
    bool imported=false;
    unsigned nativeSlot=6;
    unsigned selectedOriginal=6;
    unsigned retiredNativeSlot=6;
    std::string reason;
    std::array<uint8_t,3> nativeOrder=IdentityPartyMenuOrder;
};

// Native UpdatePartyToFieldOrder runs before the menu return callback. Even if
// a hook is placed slightly earlier, validate a harmless UI permutation by
// identity and exact record equality. This is a SWITCH-only view: no native
// item use, captured Pokemon, EXP/evolution writes or foreign records permitted.
inline void validatePartyMenuSnapshot(const PartyMenuSession& session,const Party& menuAfter) {
    menuAfter.validate();session.menu.validate();
    requirePokemon(menuAfter.count==session.menu.count,"FireRed switch menu changed the own party size");
    for (unsigned slot=0;slot<menuAfter.count;++slot) {
        unsigned original=0;
        while(original<session.menu.count && !sameOwnedMon(menuAfter.mons[slot],session.menu.mons[original]))++original;
        requirePokemon(original<session.menu.count && menuAfter.mons[slot]==session.menu.mons[original],
                       "FireRed switch menu modified or substituted an owned Pokemon");
    }
}

// Call before the native controller emits/consumes a selected party ID. The
// caller restores this plan's physical party, native indexes, queued IDs and
// saved order before letting battle engine instructions execute again. Holding
// the other core stopped and snapshot/Flash rollback remain coordinator duties.
inline PartyMenuSelection planPartyMenuSelection(const PartyMenuSession& session,const Party& menuAfter,
                                                 unsigned selectedOriginal,bool journaledExperience=false) {
    validatePartyMenuSnapshot(session,menuAfter);
    requirePokemon(selectedOriginal<=6,"Invalid FireRed native switch-menu return value");
    session.references.exclusions();validatePartyMenuOrder(session.savedNativeOrder);
    const auto journal=restoreParties(session.transaction,session.before);
    requirePokemon(session.owner<2 && session.menu==journal[session.owner],
                   "FireRed switch-menu session no longer matches its ownership journal");
    PartyMenuSelection result;
    result.transaction=session.transaction;result.party=session.before;result.references=session.references;
    result.selectedOriginal=selectedOriginal;result.nativeOrder=session.savedNativeOrder;
    if (selectedOriginal==6) {result.accepted=true;result.canceled=true;return result;}
    requirePokemon(selectedOriginal<session.menu.count,"FireRed selected an absent own party slot");
    if (!session.menu.mons[selectedOriginal].viable()) {result.reason="This Pokemon cannot battle";return result;}
    // The current active Pokemon and any queued own selection remain excluded
    // even if a UI-only temporary index view would otherwise accept them.
    for(unsigned slot=0;slot<session.before.count;++slot) {
        if(session.transaction.owners[slot]!=int(session.owner) ||
           session.transaction.originalSlots[slot]!=int(selectedOriginal)) continue;
        for(unsigned active:session.references.active)
            if(slot==active){result.reason="This Pokemon is already in battle";return result;}
        for(unsigned queued:session.references.queued)
            if(slot==queued){result.reason="This Pokemon has already been selected";return result;}
        for(unsigned excluded:session.references.callerExcluded)
            if(slot==excluded){result.reason="This Pokemon is reserved by the native switch request";return result;}
        result.accepted=true;result.nativeSlot=slot;return result;
    }
    // An unloaded reserve needs one OWN, unreferenced physical bench slot. A
    // living retired Pokemon is journaled just like a fainted one; it may be
    // selected again later without losing HP, PP, EXP, status or held items.
    auto protectedSlots=session.references.exclusions();
    // Native participation masks refer to a PHYSICAL slot until that enemy's
    // EXP award. A living participant or resident EXP Share holder (item182)
    // must stay resident until its award, or
    // replacing it would silently discard that original Pokemon's earned EXP.
    const unsigned participating=session.references.participation[0] | session.references.participation[1];
    for(unsigned slot=0;slot<session.before.count;++slot)
        if(!journaledExperience && ((participating&(1u<<slot)) || session.before.mons[slot].heldItem()==ExpShareItem) && session.before.mons[slot].hp()!=0 &&
           std::find(protectedSlots.begin(),protectedSlots.end(),slot)==protectedSlots.end())protectedSlots.push_back(slot);
    unsigned destination=6;
    for(unsigned pass=0;pass<2 && destination==6;++pass)
        for(unsigned slot=0;slot<session.before.count;++slot) {
            if(session.transaction.owners[slot]!=int(session.owner) ||
               std::find(protectedSlots.begin(),protectedSlots.end(),slot)!=protectedSlots.end())continue;
            if(pass==0 && session.before.mons[slot].hp()!=0)continue;
            destination=slot;break;
        }
    if(destination==6) {
        result.reason="The native battle reserves every working slot for this trainer, including active, queued or Wish references";
        return result;
    }
    result.transaction.originalSlots[destination]=int(selectedOriginal);
    result.transaction.combined.mons[destination]=journal[session.owner].mons[selectedOriginal];
    result.party=result.transaction.combined;
    const uint8_t keep=uint8_t(~(1u<<destination));
    result.references.participation[0]&=keep;result.references.participation[1]&=keep;
    result.references.knockedOff&=keep;
    requirePokemon(restoreParties(result.transaction,result.party)==journal,
                   "FireRed reserve selection changed a trainer's original journal");
    requirePokemon(result.transaction.owners[destination]==int(session.owner) &&
                   std::find(protectedSlots.begin(),protectedSlots.end(),destination)==protectedSlots.end(),
                   "FireRed reserve selection overwrote a protected or foreign slot");
    result.accepted=true;result.imported=true;result.nativeSlot=destination;result.retiredNativeSlot=destination;
    return result;
}

} // namespace coop::firered
