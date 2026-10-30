// SPDX-License-Identifier: GPL-3.0-or-later
// Storage/capture tests use authored records and RAM, never ROMs or saves.
#include "../gba/FireRedStorage.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks = 0;
void check(bool value, const char* message) {
    ++checks; if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F function, const char* message) {
    bool rejected = false;
    try { function(); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, message);
}
PartyMon partyMon(unsigned personality) {
    PartyMon result;
    monPut32(result.bytes.data(), personality); monPut32(result.bytes.data() + 4, 0xABCDEF01);
    result.bytes[8] = 0xBB; result.bytes[9] = 0xFF;
    result.bytes[18] = 5; result.bytes[19] = 2;
    result.bytes[20] = 0xBC; result.bytes[21] = 0xFF;
    result.bytes[84] = 10; result.bytes[85] = 0xFF;
    monPut16(result.bytes.data() + 86, 35); monPut16(result.bytes.data() + 88, 42);
    PartyMon::Secure plain{};
    monPut16(plain.data(), 1); monPut32(plain.data() + 4, 1000);
    plain[9] = 70; monPut16(plain.data() + 12, 33); plain[20] = 35;
    plain[36] = 0x12; plain[37] = 4; monPut16(plain.data() + 38, 0x310A);
    monPut32(plain.data() + 40, 0x3FFFFFFF);
    result.setSecure(plain); return result;
}
BoxMon boxMon(unsigned personality) { return BoxMon::fromParty(partyMon(personality)); }
StorageSnapshot storage() {
    StorageSnapshot result;
    result.bytes[0] = 5;
    result.bytes[1] = 0x77; result.bytes[2] = 0x88; result.bytes[3] = 0x99;
    for (unsigned i = StorageNamesOffset; i < 0x83C2; ++i) result.bytes[i] = uint8_t(i);
    for (unsigned i = 0; i < BoxCount; ++i) result.bytes[0x83C2 + i] = uint8_t(i);
    result.setMon(5, 0, boxMon(240)); result.setMon(13, 29, boxMon(480));
    return result;
}
struct Bus {
    std::array<uint8_t, 0x40000> ram{};
    std::array<uint8_t, 0x8000> iwram{};
    mCore core{};
    unsigned reads = 0, writes = 0, moveAfterReads = 0;
    uint32_t storageAddress = 0x02008000, block1 = 0x02028000, block2 = 0x02025000;
    Bus() {
        core.board = this;
        core.busRead8 = [](mCore* c, uint32_t address) -> uint32_t {
            auto& self = *static_cast<Bus*>(c->board);
            if (++self.reads == self.moveAfterReads)
                self.put32(PokemonStoragePointerAddress, self.storageAddress + 4);
            return self.at(address);
        };
        core.busWrite8 = [](mCore* c, uint32_t address, uint8_t value) {
            auto& self = *static_cast<Bus*>(c->board);
            check(address >= self.storageAddress + StorageBoxesOffset && address < self.storageAddress + StorageNamesOffset,
                  "Storage commit wrote outside boxes");
            self.at(address) = value; ++self.writes;
        };
        put32(SaveBlock1PointerAddress, block1); put32(SaveBlock2PointerAddress, block2);
        put32(block2 + InventoryKeyOffset, 0x87654321); put32(PokemonStoragePointerAddress, storageAddress);
    }
    uint8_t& at(uint32_t address) {
        if (address >= 0x02000000 && address < 0x02040000) return ram[address - 0x02000000];
        check(address >= 0x03000000 && address < 0x03008000, "RAM access outside valid regions");
        return iwram[address - 0x03000000];
    }
    void put32(uint32_t address, uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) at(address + i) = uint8_t(value >> (i * 8));
    }
};
void capture() {
    const auto before = storage();
    before.validate();
    check(!captureAddition(before, before), "Unchanged storage detected a capture");
    rejects([&] { detectCaptureAddition(before, before); }, "Expected capture allowed no addition");
    auto after = before;
    const auto caught = boxMon(720);
    after.setMon(5, 1, caught);
    const auto addition = detectCaptureAddition(before, after);
    check(addition.box == 5 && addition.slot == 1 && addition.pokemon == caught, "Wrong native capture detection");
    check(removeCaptureAddition(after, addition) == before, "Capture removal changed other bytes");
    const auto inserted = addCapture(before, caught);
    check(inserted.first == after && inserted.second.box == 5 && inserted.second.slot == 1, "Owner capture placement failed");
    check(before.mon(5, 0) == boxMon(240) && before.mon(5, 1).empty(), "Original PC was changed by pure helper");
    auto changed = after; changed.setMon(5, 2, boxMon(960));
    rejects([&] { detectCaptureAddition(before, changed); }, "Multiple native additions accepted");
    changed = after; changed.setMon(5, 0, BoxMon{});
    rejects([&] { detectCaptureAddition(before, changed); }, "Existing native Pokemon removal accepted");
    changed = after; changed.setMon(13, 29, boxMon(481));
    rejects([&] { detectCaptureAddition(before, changed); }, "Existing native Pokemon replacement accepted");
    changed = after; changed.bytes[StorageNamesOffset] ^= 1;
    rejects([&] { detectCaptureAddition(before, changed); }, "PC name mutation accepted during capture");
    changed = after; changed.bytes[0] = 6;
    check(detectCaptureAddition(before, changed).pokemon == caught, "Native current-box change rejected");
    rejects([&] { addCapture(before, boxMon(240)); }, "Existing identity inserted twice");
    rejects([&] { addCapture(before, BoxMon{}); }, "Empty capture inserted");
    auto full = before;
    for (unsigned box = 0; box < BoxCount; ++box)
        for (unsigned slot = 0; slot < BoxCapacity; ++slot)
            full.setMon(box, slot, boxMon(2000 + box * BoxCapacity + slot));
    rejects([&] { addCapture(full, caught); }, "Full PC accepted capture");
    full.setMon(4, 29, BoxMon{});
    const auto wrapped = addCapture(full, caught);
    check(wrapped.second.box == 4 && wrapped.second.slot == 29, "PC box wraparound failed");
    auto invalid = boxMon(24); invalid.bytes[32] ^= 1;
    rejects([&] { invalid.validate(); }, "Box checksum corruption accepted");
    auto invalidStorage = before; invalidStorage.bytes[0] = 14;
    rejects([&] { invalidStorage.validate(); }, "Invalid current box accepted");
    invalidStorage = before; invalidStorage.bytes[0x83C2] = 16;
    rejects([&] { invalidStorage.validate(); }, "Invalid wallpaper accepted");
    rejects([&] { before.mon(14, 0); }, "Invalid box index accepted");
    rejects([&] { before.mon(0, 30); }, "Invalid slot index accepted");
}
void nationalSpecies() {
    mCore core{};
    core.busRead8 = [](mCore*, uint32_t address) -> uint32_t {
        check(address >= NationalSpeciesTableAddress && address < NationalSpeciesTableAddress + 411 * 2,
              "Dex mapping read escaped its native table");
        const auto species = (address - NationalSpeciesTableAddress) / 2 + 1;
        unsigned national = 0;
        if (species <= 251) national = species;
        if (species == 277) national = 252;
        if (species == 301) national = 290;
        if (species == 409) national = 385;
        if (species == 410) national = 386;
        if (species == 411) national = 358;
        return address % 2 ? national >> 8 : national & 0xFFu;
    };
    for (unsigned species = 1; species <= 251; ++species)
        check(nativeNationalSpecies(&core, species) == species, "Kanto/Johto dex mapping changed");
    check(nativeNationalSpecies(&core, 301) == 290 && nativeNationalSpecies(&core, 410) == 386 &&
          nativeNationalSpecies(&core, 411) == 358, "Hoenn dex mapping incorrectly assumed constant offset");
    rejects([&] { nativeNationalSpecies(&core, 252); }, "Unused old Unown species accepted");
    rejects([&] { nativeNationalSpecies(&core, 412); }, "Egg species accepted as caught dex entry");
    rejects([&] { nativeNationalSpecies(&core, 302); }, "Invalid native dex mapping accepted");
}
void trainer() {
    const TrainerIdentity guest{0x01234567, {{0xCC, 0xCD, 0xCE, 0xFF, 0xFF, 0xFF, 0xFF}}, 1};
    for (unsigned pid = 0; pid < 24; ++pid) {
        const auto original = partyMon(pid);
        const auto changed = assignCaptureTrainer(original, guest);
        changed.validate();
        check(changed.identity().personality == original.identity().personality && changed.identity().trainer == guest.id,
              "Capture original trainer assignment failed");
        auto wanted = original.secure();
        monPut16(wanted.data() + 38, uint16_t(monU16(wanted.data() + 38) | 0x8000));
        check(changed.secure() == wanted, "Capture re-encryption damaged logical Pokemon data");
        check(std::equal(guest.name.begin(), guest.name.end(), changed.bytes.begin() + 20), "Capture OT name not changed");
        check(std::equal(original.bytes.begin() + 80, original.bytes.end(), changed.bytes.begin() + 80),
              "Capture reassignment changed party-only HP/status/stats");
        check(assignCaptureTrainer(BoxMon::fromParty(original), guest) == BoxMon::fromParty(changed), "Box and party OT assignments differ");
    }
    TrainerIdentity invalid = guest; invalid.gender = 2;
    rejects([&] { assignCaptureTrainer(boxMon(24), invalid); }, "Invalid trainer gender accepted");
    rejects([&] { assignCaptureTrainer(BoxMon{}, guest); }, "Empty Pokemon assigned OT");
    Bus bus;
    bus.put32(bus.block2 + 0xA, guest.id);
    for (unsigned i = 0; i < guest.name.size(); ++i) bus.at(bus.block2 + i) = guest.name[i];
    bus.at(bus.block2 + 8) = guest.gender;
    const auto read = readTrainerIdentity(&bus.core);
    check(read.id == guest.id && read.name == guest.name && read.gender == guest.gender, "Trainer memory layout incorrect");
}
void memory() {
    Bus bus;
    const auto desired = storage();
    // Keep native metadata distinct: writeStorage must commit box contents only.
    bus.at(bus.storageAddress) = 2;
    for (unsigned offset = StorageNamesOffset; offset < PokemonStorageSize; ++offset)
        bus.at(bus.storageAddress + offset) = desired.bytes[offset];
    const auto before = bus.ram;
    writeStorage(&bus.core, desired);
    const auto actual = readStorage(&bus.core);
    check(actual.currentBox() == 2, "Storage commit overwrote current box");
    check(actual.mon(5, 0) == desired.mon(5, 0) && actual.mon(13, 29) == desired.mon(13, 29), "Storage binary stride/offset incorrect");
    for (unsigned offset = 0; offset < bus.ram.size(); ++offset)
        if (offset < bus.storageAddress - 0x02000000 + StorageBoxesOffset ||
            offset >= bus.storageAddress - 0x02000000 + StorageNamesOffset)
            check(before[offset] == bus.ram[offset], "Storage commit changed foreign RAM");
    const auto bytes = bus.ram; const auto writes = bus.writes;
    auto invalid = desired; invalid.bytes[StorageBoxesOffset + 32] ^= 1;
    // The first slot is empty, so a nonzero raw encrypted body is corrupt.
    rejects([&] { writeStorage(&bus.core, invalid); }, "Corrupt storage write accepted");
    check(bytes == bus.ram && writes == bus.writes, "Corrupt storage partially committed");
    for (const auto pointer : {0u, 0x02000001u, 0x0203C000u, 0x08000000u, bus.block1}) {
        bus.put32(PokemonStoragePointerAddress, pointer);
        rejects([&] { readStorage(&bus.core); }, "Invalid PC pointer accepted");
        rejects([&] { writeStorage(&bus.core, desired); }, "Invalid PC write pointer accepted");
        check(writes == bus.writes, "Invalid PC pointer caused writes");
    }
    Bus moving;
    moving.reads = 0; moving.moveAfterReads = 17;
    rejects([&] { writeStorage(&moving.core, desired); }, "Moved PC storage accepted");
    check(moving.writes == 0, "Moved PC storage partially written");
    rejects([&] { readStorage(nullptr); }, "Null PC core accepted");
}
}
int main() {
    try {
        capture(); nationalSpecies(); trainer(); memory();
        std::cout << "FireRed capture ownership: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
