// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedStorage.h"

namespace coop::firered {

inline bool storageHasCaptureSpace(const StorageSnapshot& storage) {
    storage.validate();
    for (unsigned box = 0; box < BoxCount; ++box)
        for (unsigned slot = 0; slot < BoxCapacity; ++slot)
            if (storage.mon(box, slot).empty()) return true;
    return false;
}
inline bool ownerHasCaptureCapacity(const Party& party, const StorageSnapshot& storage) {
    party.validate(); storage.validate();
    return party.count < PartyCapacity || storageHasCaptureSpace(storage);
}

struct CaptureStagingPlan {
    StorageSnapshot originalNative{};
    StorageSnapshot stagedNative{};
    bool actualOwnerHasCapacity = false;
    // A copy of the temporarily displaced native record remains in the full
    // originalNative journal. The optional stores coordinates only.
    std::optional<std::pair<unsigned, unsigned>> scratchSlot;
};
inline void validateCaptureStaging(const CaptureStagingPlan& plan) {
    plan.originalNative.validate(); plan.stagedNative.validate();
    if (!plan.scratchSlot) {
        requirePokemon(plan.originalNative == plan.stagedNative, "Unjournaled FireRed PC capture staging change");
        return;
    }
    requirePokemon(plan.actualOwnerHasCapacity, "Cannot stage FireRed capture for a full recipient");
    const auto box = plan.scratchSlot->first, slot = plan.scratchSlot->second;
    requirePokemon(box < BoxCount && slot < BoxCapacity && !plan.originalNative.mon(box, slot).empty() &&
                   plan.stagedNative.mon(box, slot).empty(), "Invalid FireRed PC capture scratch slot");
    requirePokemon(!storageHasCaptureSpace(plan.originalNative), "FireRed capture displaced a Pokemon despite available native PC space");
    auto expected = plan.originalNative;
    expected.setMon(box, slot, BoxMon{});
    requirePokemon(expected == plan.stagedNative, "FireRed capture staging altered another PC record or metadata");
}

// PURE planning only: no emulator or save is mutated. Apply stagedNative to the
// paused leader using writeStorage ONLY inside an active TeamBattle transaction
// whose complete CPU/RAM/battery rollback and original storage are retained.
// Never flush a battery while staged. Always derive a new owner's plan from the
// immutable pre-battle original, never from already staged emulator contents.
inline CaptureStagingPlan prepareCaptureStaging(const StorageSnapshot& originalNative,
                                               const Party& actualOwnerParty,
                                               const StorageSnapshot& actualOwnerStorage) {
    originalNative.validate();
    CaptureStagingPlan result;
    result.originalNative = originalNative;
    result.stagedNative = originalNative;
    result.actualOwnerHasCapacity = ownerHasCaptureCapacity(actualOwnerParty, actualOwnerStorage);
    if (result.actualOwnerHasCapacity && !storageHasCaptureSpace(originalNative)) {
        // All420 original records remain in originalNative. Free exactly one
        // transient RAM slot in the current native box, then let the unmodified
        // capture engine create its genuine BoxPokemon there.
        const auto box = originalNative.currentBox();
        result.scratchSlot = std::make_pair(box, 0u);
        result.stagedNative.setMon(box, 0, BoxMon{});
    }
    validateCaptureStaging(result);
    return result;
}
struct CaptureStagingResult {
    StorageSnapshot restoredNative{};
    std::optional<CaptureAddition> captured;
};
inline CaptureStagingResult recoverCaptureStaging(const CaptureStagingPlan& plan,
                                                  const StorageSnapshot& nativeAfter) {
    validateCaptureStaging(plan);
    const auto capture = captureAddition(plan.stagedNative, nativeAfter);
    requirePokemon(!capture || plan.actualOwnerHasCapacity, "Native FireRed capture exceeded its actual owner's capacity");
    if (capture) {
        requirePokemon(!capture->pokemon.asPartyRecord().egg(), "Native FireRed capture unexpectedly created an Egg");
        // Include the journaled scratch record in identity checks. It was only
        // hidden transiently, and must not reappear as a new duplicate capture.
        for (unsigned box = 0; box < BoxCount; ++box)
            for (unsigned slot = 0; slot < BoxCapacity; ++slot) {
                const auto existing = plan.originalNative.mon(box, slot);
                requirePokemon(existing.empty() || existing.identity() != capture->pokemon.identity(),
                               "Native FireRed capture duplicates an original PC Pokemon identity");
            }
    }
    // Even a canceled/failed throw restores every displaced original record.
    // Transfer only captured->pokemon to its real owner's validated party/PC,
    // reassigning its OT via assignCaptureTrainer before the final commit.
    return {plan.originalNative, capture};
}

} // namespace coop::firered
