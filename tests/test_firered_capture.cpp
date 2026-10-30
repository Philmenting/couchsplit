// SPDX-License-Identifier: GPL-3.0-or-later
// Authored storage/party fixtures only. No emulator, ROM, save or file writes.
#include "../gba/FireRedCapture.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks = 0;
void check(bool value, const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F function, const char* message) {
    bool rejected = false;
    try { function(); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, message);
}
PartyMon pokemon(unsigned personality) {
    PartyMon result;
    monPut32(result.bytes.data(), personality); monPut32(result.bytes.data() + 4, 0xABCDEF01);
    result.bytes[18] = 5; result.bytes[19] = 2;
    result.bytes[8] = 0xBB; result.bytes[9] = 0xFF; result.bytes[20] = 0xBC; result.bytes[21] = 0xFF;
    result.bytes[84] = 10; monPut16(result.bytes.data() + 86, 35); monPut16(result.bytes.data() + 88, 42);
    PartyMon::Secure plain{}; monPut16(plain.data(), 1); monPut32(plain.data() + 4, 1000);
    plain[9] = 70; monPut16(plain.data() + 12, 33); plain[20] = 35;
    monPut32(plain.data() + 40, 0x3FFFFFFF); result.setSecure(plain); return result;
}
Party party(unsigned count) {
    Party result; result.count = uint8_t(count);
    for (unsigned slot = 0; slot < count; ++slot) result.mons[slot] = pokemon(2000 + slot);
    return result;
}
StorageSnapshot storage(bool full) {
    StorageSnapshot result; result.bytes[0] = 5;
    result.bytes[1] = 17; result.bytes[2] = 18; result.bytes[3] = 19;
    for (unsigned offset = StorageNamesOffset; offset < 0x83C2; ++offset) result.bytes[offset] = 0xFF;
    if (full)
        for (unsigned box = 0; box < BoxCount; ++box)
            for (unsigned slot = 0; slot < BoxCapacity; ++slot)
                result.setMon(box, slot, BoxMon::fromParty(pokemon(1000 + box * BoxCapacity + slot)));
    return result;
}
void capacity() {
    const auto empty = storage(false), full = storage(true);
    check(storageHasCaptureSpace(empty) && !storageHasCaptureSpace(full), "Incorrect PC capacity");
    check(ownerHasCaptureCapacity(party(5), full), "Original party space not counted");
    check(ownerHasCaptureCapacity(party(6), empty), "Original PC space not counted");
    check(!ownerHasCaptureCapacity(party(6), full), "Full actual recipient incorrectly accepted");
    const auto denied = prepareCaptureStaging(full, party(6), full);
    check(!denied.actualOwnerHasCapacity && !denied.scratchSlot && denied.stagedNative == full,
          "Full recipient displaced native PC Pokemon");
    const auto nativeFree = prepareCaptureStaging(empty, party(6), full);
    check(!nativeFree.actualOwnerHasCapacity && nativeFree.stagedNative == empty, "Leader capacity leaked to full recipient");
    auto forbidden = empty; forbidden.setMon(5, 0, BoxMon::fromParty(pokemon(9000)));
    rejects([&] { recoverCaptureStaging(nativeFree, forbidden); }, "Capture given to full actual recipient");
}
void staging() {
    const auto full = storage(true), recipientStorage = storage(false);
    const auto originalBytes = full.bytes;
    const auto plan = prepareCaptureStaging(full, party(5), storage(true));
    check(plan.actualOwnerHasCapacity && bool(plan.scratchSlot), "Full leader PC did not stage for owner's party space");
    check(plan.scratchSlot->first == 5 && plan.scratchSlot->second == 0, "Scratch not in native current box");
    check(full.bytes == originalBytes, "Pure staging mutated original storage");
    unsigned changed = 0;
    for (unsigned box = 0; box < BoxCount; ++box)
        for (unsigned slot = 0; slot < BoxCapacity; ++slot)
            if (!(plan.stagedNative.mon(box, slot) == full.mon(box, slot))) {
                ++changed; check(box == 5 && slot == 0 && plan.stagedNative.mon(box, slot).empty(), "Staging displaced unjournaled PC record");
            }
    check(changed == 1, "Staging did not create exactly one transient RAM slot");
    const auto caught = BoxMon::fromParty(pokemon(9000));
    auto nativeAfter = plan.stagedNative; nativeAfter.setMon(5, 0, caught);
    const auto result = recoverCaptureStaging(plan, nativeAfter);
    check(result.restoredNative == full && result.captured && result.captured->pokemon == caught,
          "Captured record not extracted while restoring all original leader records");
    for (unsigned box = 0; box < BoxCount; ++box)
        for (unsigned slot = 0; slot < BoxCapacity; ++slot)
            check(result.restoredNative.mon(box, slot) == full.mon(box, slot), "Leader original PC Pokemon lost after capture");
    const auto transferred = addCapture(recipientStorage, result.captured->pokemon);
    check(transferred.first.mon(5, 0) == caught, "Extracted native capture not transferable to recipient");
    const auto failedThrow = recoverCaptureStaging(plan, plan.stagedNative);
    check(!failedThrow.captured && failedThrow.restoredNative == full, "Failed/canceled throw did not restore displaced record");
    auto metadata = nativeAfter; metadata.bytes[0] = 6;
    check(recoverCaptureStaging(plan, metadata).restoredNative == full, "Native current-box change lost original journal");
    auto changedOriginal = nativeAfter; changedOriginal.setMon(4, 29, BoxMon::fromParty(pokemon(9001)));
    rejects([&] { recoverCaptureStaging(plan, changedOriginal); }, "Capture changed another original leader PC record");
    auto duplicate = nativeAfter; duplicate.setMon(5, 0, full.mon(5, 0));
    rejects([&] { recoverCaptureStaging(plan, duplicate); }, "Displaced original record mistaken for new capture");
    auto corrupt = plan; corrupt.stagedNative.setMon(5, 1, BoxMon{});
    rejects([&] { recoverCaptureStaging(corrupt, nativeAfter); }, "Two displaced original records accepted");
    corrupt = plan; corrupt.actualOwnerHasCapacity = false;
    rejects([&] { validateCaptureStaging(corrupt); }, "Forged capacity permits staging");
    corrupt = plan; corrupt.stagedNative.bytes[StorageNamesOffset] ^= 1;
    rejects([&] { validateCaptureStaging(corrupt); }, "Staging changed native PC metadata");
    corrupt = plan; corrupt.scratchSlot.reset();
    rejects([&] { validateCaptureStaging(corrupt); }, "Unjournaled staging removal accepted");
}
void existingSpace() {
    auto native = storage(false);
    native.setMon(5, 0, BoxMon::fromParty(pokemon(1000)));
    const auto plan = prepareCaptureStaging(native, party(6), storage(false));
    check(plan.actualOwnerHasCapacity && !plan.scratchSlot && plan.stagedNative == native,
          "Existing native space displaced an original Pokemon");
    auto after = plan.stagedNative;
    after.setMon(5, 1, BoxMon::fromParty(pokemon(9000)));
    const auto captured = recoverCaptureStaging(plan, after);
    check(captured.captured && captured.captured->slot == 1 && captured.restoredNative == native,
          "Normal native capture recovery changed leader originals");
    after.setMon(5, 2, BoxMon::fromParty(pokemon(9001)));
    rejects([&] { recoverCaptureStaging(plan, after); }, "Multiple additions accepted");
    const auto ownerOne = prepareCaptureStaging(storage(true), party(6), storage(false));
    const auto ownerTwo = prepareCaptureStaging(ownerOne.originalNative, party(6), storage(true));
    check(ownerOne.scratchSlot && !ownerTwo.scratchSlot && ownerTwo.stagedNative == ownerOne.originalNative,
          "Restaging from immutable original lost displaced record");
}
}
int main() {
    try {
        capacity(); staging(); existingSpace();
        std::cout << "FireRed capture staging: " << checks << " checks passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
