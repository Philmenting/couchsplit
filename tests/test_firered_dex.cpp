// SPDX-License-Identifier: GPL-3.0-or-later
// Authored RAM and numeric species mappings; no commercial game assets.
#include "../gba/FireRedDex.h"
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
struct Bus {
    std::array<uint8_t, 0x40000> ram{};
    std::array<uint8_t, 0x8000> iwram{};
    mCore core{};
    unsigned writes = 0, reads = 0, moveAfterReads = 0;
    uint32_t lastWrite = 0;
    uint32_t block1 = 0x02028000, block2 = 0x02025000;
    Bus() {
        core.board = this;
        core.busRead8 = [](mCore* core, uint32_t address) -> uint32_t {
            auto& self = *static_cast<Bus*>(core->board);
            if (++self.reads == self.moveAfterReads)
                self.put32(SaveBlock1PointerAddress, self.block1 + 4);
            if (address >= NationalSpeciesTableAddress && address < NationalSpeciesTableAddress + 411 * 2) {
                const auto species = (address - NationalSpeciesTableAddress) / 2 + 1;
                const auto national = species <= 251 ? species : species == 308 ? 327u : species == 410 ? 386u : 0u;
                return address % 2 ? national >> 8 : national & 0xFFu;
            }
            return self.at(address);
        };
        core.busWrite8 = [](mCore* core, uint32_t address, uint8_t value) {
            auto& self = *static_cast<Bus*>(core->board);
            const auto within = [&](uint32_t start, unsigned length) { return address >= start && address < start + length; };
            check(within(self.block2 + DexOwnedOffset, 52) || within(self.block2 + DexSeenOffset, 52) ||
                  within(self.block1 + DexSeen1Offset, 52) || within(self.block1 + DexSeen2Offset, 52) ||
                  within(self.block2 + DexUnownPersonalityOffset, 4) || within(self.block2 + DexSpindaPersonalityOffset, 4),
                  "Dex write escaped owned fields");
            self.at(address) = value; self.lastWrite = address; ++self.writes;
        };
        put32(SaveBlock1PointerAddress, block1); put32(SaveBlock2PointerAddress, block2);
        put32(block2 + InventoryKeyOffset, 0x87654321);
    }
    uint8_t& at(uint32_t address) {
        if (address >= 0x02000000 && address < 0x02040000) return ram[address - 0x02000000];
        check(address >= 0x03000000 && address < 0x03008000, "Dex memory access outside GBA RAM");
        return iwram[address - 0x03000000];
    }
    void put32(uint32_t address, uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) at(address + i) = uint8_t(value >> (i * 8));
    }
};
void purePlans() {
    const DexEntry before{{{0xA0, 0x40, 0x10}}, 0x80, 4, 123, false};
    const auto seen = planDexUpdate(before, 456, false);
    check(seen.entry.seen == std::array<uint8_t, 3>{{0xA4, 0x44, 0x14}} && seen.entry.owned == 0x80,
          "Seen plan altered unrelated bits or ownership");
    check(!seen.writePersonality && seen.entry.personality == 123 && seen.entry.isSeen(), "Ordinary species altered form personality");
    const auto caught = planDexUpdate(before, 456, true);
    check(caught.entry.isCaught() && caught.entry.owned == 0x84, "Caught plan did not set all four copies");
    auto special = before; special.formSpecies = true;
    const auto firstSeen = planDexUpdate(special, 0, false);
    check(firstSeen.writePersonality && firstSeen.entry.personality == 0, "Zero personality rejected as first form");
    const auto repeatedSeen = planDexUpdate(firstSeen.entry, 999, false);
    check(!repeatedSeen.writePersonality && repeatedSeen.entry.personality == 0, "Repeated seen changed first form");
    const auto firstCaught = planDexUpdate(firstSeen.entry, 321, true);
    check(firstCaught.writePersonality && firstCaught.entry.personality == 321, "First caught did not record native caught form");
    const auto repeatedCaught = planDexUpdate(firstCaught.entry, 999, true);
    check(!repeatedCaught.writePersonality && repeatedCaught.entry.personality == 321, "Repeated caught replaced recorded form");
    auto inconsistent = firstCaught.entry; inconsistent.seen[2] &= uint8_t(~inconsistent.mask);
    check(!inconsistent.isSeen() && !inconsistent.isCaught(), "Mismatched anti-cheat seen copies considered valid");
    check(planDexUpdate(inconsistent, 333, true).entry.isCaught(), "Inconsistent copies not repaired");
    auto invalid = before; invalid.mask = 3;
    rejects([&] { planDexUpdate(invalid, 1, false); }, "Multi-bit dex entry accepted");
}
void memoryUpdates() {
    Bus bus;
    for (unsigned offset = 0; offset < 52; ++offset) {
        bus.at(bus.block2 + DexOwnedOffset + offset) = 0x80;
        bus.at(bus.block2 + DexSeenOffset + offset) = 0x80;
        bus.at(bus.block1 + DexSeen1Offset + offset) = 0x80;
        bus.at(bus.block1 + DexSeen2Offset + offset) = 0x80;
    }
    const auto before = bus.ram;
    markSeen(&bus.core, 1, 0xDEADBEEF);
    const auto seen = readDexEntry(&bus.core, 1);
    check(seen.isSeen() && !seen.isCaught() && seen.owned == 0x80, "markSeen granted owned Pokemon");
    const auto addresses = dexEntryAddresses(readSaveMemory(&bus.core), 1);
    for (unsigned offset = 0; offset < bus.ram.size(); ++offset) {
        const auto address = 0x02000000 + offset;
        if (address == addresses[0] || address == addresses[1] || address == addresses[2])
            check(bus.ram[offset] == 0x81, "Seen flag used wrong bit");
        else check(bus.ram[offset] == before[offset], "markSeen overwrote foreign bytes");
    }
    markCaught(&bus.core, 1, 0xDEADBEEF);
    check(readDexEntry(&bus.core, 1).isCaught(), "markCaught not accepted by native anti-cheat copies");
    check(bus.lastWrite == addresses[3], "Owned bit was not committed last");
    const auto writes = bus.writes;
    markCaught(&bus.core, 1, 0x12345678);
    check(bus.writes == writes, "Repeated ordinary caught event produced unnecessary writes");
    markCaught(&bus.core, 410, 100);
    const auto deoxys = readDexEntry(&bus.core, 410);
    check(deoxys.isCaught() && deoxys.mask == 2, "Hoenn capture used wrong national dex entry");
    Bus other;
    markSeen(&other.core, 410, 100);
    check(!readDexEntry(&other.core, 410).isCaught(), "Shared seen event leaked caught ownership");
}
void specialForms() {
    Bus bus;
    markSeen(&bus.core, 201, 0);
    check(inventoryRead32(&bus.core, bus.block2 + DexUnownPersonalityOffset) == 0, "Unown first form not set");
    markSeen(&bus.core, 201, 111);
    check(inventoryRead32(&bus.core, bus.block2 + DexUnownPersonalityOffset) == 0, "Zero first-seen Unown form replaced");
    markCaught(&bus.core, 201, 222);
    check(inventoryRead32(&bus.core, bus.block2 + DexUnownPersonalityOffset) == 222, "First-caught Unown form not recorded");
    markSeen(&bus.core, 201, 333); markCaught(&bus.core, 201, 444);
    check(inventoryRead32(&bus.core, bus.block2 + DexUnownPersonalityOffset) == 222, "Established Unown form overwritten");
    markSeen(&bus.core, 308, 0x12345678);
    check(inventoryRead32(&bus.core, bus.block2 + DexSpindaPersonalityOffset) == 0x12345678, "Spinda native mapping/form not recorded");
    markCaught(&bus.core, 308, 0x87654321);
    check(inventoryRead32(&bus.core, bus.block2 + DexSpindaPersonalityOffset) == 0x87654321, "Spinda first-caught form not recorded");
}
void rejection() {
    Bus bus;
    const auto before = bus.ram;
    rejects([&] { markCaught(&bus.core, 0, 1); }, "Species zero accepted");
    rejects([&] { markSeen(&bus.core, 252, 1); }, "Unused species accepted");
    rejects([&] { markCaught(&bus.core, 309, 1); }, "Invalid native dex table value accepted");
    check(bus.ram == before && bus.writes == 0, "Invalid dex update partially committed");
    bus.put32(SaveBlock1PointerAddress, 0);
    rejects([&] { markCaught(&bus.core, 1, 1); }, "Invalid save pointer accepted");
    check(bus.writes == 0, "Invalid save pointer caused writes");
    Bus moving;
    moving.reads = 0; moving.moveAfterReads = 15;
    rejects([&] { markCaught(&moving.core, 1, 1); }, "Moved save block accepted");
    check(moving.writes == 0, "Moved save block partially committed");
    rejects([&] { markSeen(nullptr, 1, 0); }, "Null dex write core accepted");
}
}
int main() {
    try {
        purePlans(); memoryUpdates(); specialForms(); rejection();
        std::cout << "FireRed dex ownership: " << checks << " checks passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
