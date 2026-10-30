// SPDX-License-Identifier: GPL-3.0-or-later
// Authored emulator RAM only: no ROM, save, game text or personal data.
#include "../gba/FireRedInventory.h"
#include <iostream>
#include <limits>

using namespace coop::firered;
namespace {
unsigned checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F function, const char* message) {
    bool rejected = false;
    try { function(); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, message);
}
struct Bus {
    std::array<uint8_t, 0x40000> ram{};
    std::array<uint8_t, 0x8000> iwram{};
    mCore core{};
    unsigned reads = 0, writes = 0, moveAfterReads = 0;
    uint32_t block1 = 0x02028000, block2 = 0x02025000;
    Bus(uint32_t key = 0x87654321) {
        core.board = this;
        core.busRead8 = [](mCore* c, uint32_t address) -> uint32_t {
            auto& self = *static_cast<Bus*>(c->board);
            if (++self.reads == self.moveAfterReads)
                self.put32(SaveBlock1PointerAddress, self.block1 + 4);
            return self.at(address);
        };
        core.busWrite8 = [](mCore* c, uint32_t address, uint8_t value) {
            auto& self = *static_cast<Bus*>(c->board);
            const bool bag = address >= self.block1 + PocketOffsets[0] && address < self.block1 + 0x5F8;
            const bool wallet = address >= self.block1 + WalletOffset && address < self.block1 + WalletOffset + 4;
            check(bag || wallet, "Write escaped bag/wallet RAM");
            self.at(address) = value; ++self.writes;
        };
        put32(SaveBlock1PointerAddress, block1);
        put32(SaveBlock2PointerAddress, block2);
        put32(block2 + InventoryKeyOffset, key);
        writeInventory(&core, Inventory{}); writeWallet(&core, 0);
        reads = writes = 0;
    }
    uint8_t& at(uint32_t address) {
        if (address >= 0x02000000 && address < 0x02040000) return ram[address - 0x02000000];
        check(address >= 0x03000000 && address < 0x03008000, "Read escaped GBA RAM");
        return iwram[address - 0x03000000];
    }
    void put32(uint32_t address, uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) at(address + i) = uint8_t(value >> (i * 8));
    }
};
Inventory inventory() {
    Inventory result;
    result.setQuantity(13, 9); // Potion.
    result.setQuantity(14, 3);
    result.setQuantity(4, 20);
    result.setQuantity(133, 5);
    result.setQuantity(289, 1);
    result.setQuantity(364, 1);
    result.setQuantity(365, 1);
    result.setQuantity(361, 1);
    // Keep a genuine empty gap, as native RemoveBagItem does.
    std::swap(result.pockets[Items][1], result.pockets[Items][3]);
    return result;
}
void metadata() {
    check(itemPocket(0) == -1 && itemPocket(52) == -1 && itemPocket(375) == -1, "Unused IDs accepted");
    check(itemPocket(1) == int(PokeBalls) && itemPocket(133) == int(Berries) &&
          itemPocket(289) == int(Machines) && itemPocket(349) == int(KeyItems) &&
          itemPocket(13) == int(Items), "Incorrect item pocket metadata");
    check(battleConsumable(13) && battleConsumable(4) && battleConsumable(175) &&
          !battleConsumable(39) && !battleConsumable(364) && !battleConsumable(289), "Incorrect consumable metadata");
    auto full = Inventory{};
    for (unsigned item = 13; item < 13 + 42; ++item)
        if (itemPocket(item) == int(Items)) full.setQuantity(item, 1);
    unsigned next = 63;
    while (full.pockets[Items][41].item == 0) full.setQuantity(next++, 1);
    rejects([&] { full.setQuantity(179, 1); }, "Full pocket addition accepted");
    full.setQuantity(13, 0); full.setQuantity(179, 999);
    check(full.pockets[Items][0].item == 179 && full.quantity(179) == 999, "Empty gap not reused");
    rejects([&] { full.setQuantity(179, 1000); }, "Quantity overflow accepted");
    rejects([&] { full.quantity(0); }, "Empty item quantity query accepted");
    auto invalid = inventory(); invalid.pockets[Items][57] = {13, 1};
    rejects([&] { invalid.validate(); }, "Slot beyond pocket capacity accepted");
    invalid = inventory(); invalid.pockets[Items][2] = {13, 1};
    rejects([&] { invalid.validate(); }, "Duplicate stacks accepted");
    invalid = inventory(); invalid.pockets[Items][2] = {4, 1};
    rejects([&] { invalid.validate(); }, "Ball in wrong pocket accepted");
    invalid = inventory(); invalid.pockets[Items][2] = {0, 1};
    rejects([&] { invalid.validate(); }, "Empty item with quantity accepted");
}
void serialization() {
    const auto original = inventory();
    for (uint32_t key : {0u, 0x87654321u, 0xFFFFFFFFu}) {
        Bus bus(key);
        writeInventory(&bus.core, original); writeWallet(&bus.core, 123456);
        check(readInventory(&bus.core) == original, "Encrypted inventory round trip failed");
        check(readWallet(&bus.core) == 123456, "Encrypted wallet round trip failed");
        const auto slot = bus.block1 + PocketOffsets[Items];
        check(bus.at(slot) == 13 && bus.at(slot + 1) == 0, "Item ID was incorrectly encrypted");
        check(inventoryRead16(&bus.core, slot + 2) == uint16_t(9 ^ uint16_t(key)), "Wrong quantity encryption");
        check(inventoryRead32(&bus.core, bus.block1 + WalletOffset) == (123456 ^ key), "Wrong wallet encryption");
        const auto before = bus.ram;
        auto next = original; next.setQuantity(13, 7); next.setQuantity(289, 9); next.setQuantity(361, 0);
        writeInventory(&bus.core, next, BattlePockets);
        const auto actual = readInventory(&bus.core);
        check(actual.quantity(13) == 7 && actual.quantity(289) == 1 && actual.quantity(361) == 1,
              "Pocket mask overwrote foreign pockets");
        for (unsigned i = 0; i < bus.ram.size(); ++i) {
            const auto address = 0x02000000u + i;
            bool allowed = false;
            for (unsigned pocket : {unsigned(Items), unsigned(PokeBalls), unsigned(Berries)})
                allowed |= address >= bus.block1 + PocketOffsets[pocket] &&
                           address < bus.block1 + PocketOffsets[pocket] + PocketCapacities[pocket] * 4;
            if (!allowed) check(bus.ram[i] == before[i], "Foreign RAM changed during masked inventory write");
        }
        auto invalid = original; invalid.pockets[Items][0].quantity = 1000;
        const auto snapshot = bus.ram; const auto writes = bus.writes;
        rejects([&] { writeInventory(&bus.core, invalid); }, "Invalid inventory write accepted");
        rejects([&] { writeInventory(&bus.core, original, 32); }, "Invalid pocket mask accepted");
        rejects([&] { writeWallet(&bus.core, 1000000); }, "Invalid wallet write accepted");
        check(snapshot == bus.ram && writes == bus.writes, "Invalid write partially committed");
    }
    Bus donor(0x12345678), recipient(0x89ABCDEF);
    writeInventory(&donor.core, original);
    writeInventory(&recipient.core, readInventory(&donor.core));
    check(readInventory(&recipient.core) == original, "Per-owner encryption key leaked into canonical inventory");
}
void badMemory() {
    Bus bus;
    for (const auto invalid : {0u, 0x02000001u, 0x0203D000u, 0x08000000u, 0xFFFFFFFCu}) {
        bus.put32(SaveBlock1PointerAddress, invalid);
        const auto writes = bus.writes;
        rejects([&] { readInventory(&bus.core); }, "Invalid save pointer accepted for read");
        rejects([&] { writeInventory(&bus.core, inventory()); }, "Invalid save pointer accepted for write");
        rejects([&] { writeWallet(&bus.core, 10); }, "Invalid save pointer accepted for wallet");
        check(bus.writes == writes, "Invalid pointer caused partial write");
    }
    bus.put32(SaveBlock1PointerAddress, bus.block1);
    bus.put32(SaveBlock2PointerAddress, bus.block1 + 4);
    rejects([&] { readSaveMemory(&bus.core); }, "Overlapping save blocks accepted");
    bus.put32(SaveBlock2PointerAddress, bus.block2);
    bus.put32(bus.block1 + WalletOffset, 1000000 ^ 0x87654321);
    rejects([&] { readWallet(&bus.core); }, "Invalid encrypted wallet accepted");
    writeInventory(&bus.core, inventory());
    bus.at(bus.block1 + PocketOffsets[Items] + 3) ^= 0x80;
    rejects([&] { readInventory(&bus.core); }, "Corrupted encrypted quantity accepted");
    Bus moving;
    moving.reads = 0; moving.moveAfterReads = 13;
    const auto writes = moving.writes;
    rejects([&] { writeInventory(&moving.core, inventory()); }, "Moved save block accepted");
    check(writes == moving.writes, "Moved save block partially written");
    rejects([&] { readWallet(nullptr); }, "Null wallet core accepted");
    rejects([&] { writeInventory(nullptr, inventory()); }, "Null inventory core accepted");
}
void merging() {
    const auto owner = inventory();
    auto after = owner; after.setQuantity(13, 8); after.setQuantity(4, 19);
    // Menu compaction is not a new item or a change in ownership.
    std::swap(after.pockets[Items][0], after.pockets[Items][2]);
    const auto consumed = consumptionDelta(owner, after);
    check(consumed.amounts[13] == 1 && consumed.amounts[4] == 1, "Wrong native consumption delta");
    const auto result = applyConsumption(owner, consumed);
    check(result.quantity(13) == 8 && result.quantity(4) == 19, "Wrong consumed owner quantities");
    check(result.pockets[Machines] == owner.pockets[Machines] && result.pockets[KeyItems] == owner.pockets[KeyItems],
          "Foreign pockets changed during consumption merge");
    check(result.pockets[Items][3] == owner.pockets[Items][3], "Other item order changed during merge");
    auto otherOwner = owner; otherOwner.setQuantity(13, 100); otherOwner.setQuantity(4, 5);
    check(mergeBattleInventory(otherOwner, owner, after).quantity(13) == 99, "Delta used foreign absolute quantities");
    auto insufficient = owner; insufficient.setQuantity(4, 0);
    rejects([&] { applyConsumption(insufficient, consumed); }, "Consumption underflow accepted");
    auto invalid = owner; invalid.setQuantity(13, 10);
    rejects([&] { consumptionDelta(owner, invalid); }, "Unexpected native item gain accepted");
    invalid = owner; invalid.setQuantity(289, 0);
    rejects([&] { consumptionDelta(owner, invalid); }, "TM consumption accepted during battle");
    invalid = owner; invalid.setQuantity(361, 0);
    rejects([&] { consumptionDelta(owner, invalid); }, "Key item consumption accepted during battle");
    auto invalidDelta = consumed; invalidDelta.amounts[0] = 1;
    rejects([&] { applyConsumption(owner, invalidDelta); }, "Invalid delta accepted");
    check(walletDelta(1410, 1506) == 96 && walletDelta(1506, 1410) == -96, "Wallet delta failed");
    check(applyWalletDelta(100, -200) == 0 && applyWalletDelta(999990, 96) == 999999, "Wallet caps failed");
    const auto rewards = rewardWallets({{1410, 1460}}, 96);
    check(rewards[0] == 1506 && rewards[1] == 1556, "Full reward not credited to each owner");
    const auto saturated = rewardWallets({{999990, 100}}, 96);
    check(saturated[0] == 999999 && saturated[1] == 196, "Host wallet cap truncated guest reward");
    rejects([&] { walletDelta(1000000, 0); }, "Invalid wallet delta accepted");
    rejects([&] { applyWalletDelta(100, std::numeric_limits<int64_t>::max()); }, "Overflow delta accepted");
    rejects([&] { applyWalletDelta(100, std::numeric_limits<int64_t>::min()); }, "Underflow delta accepted");
}
}
int main() {
    try {
        metadata(); serialization(); badMemory(); merging();
        std::cout << "FireRed inventory ownership: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
