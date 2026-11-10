// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedPokemon.h"
#include <array>
#include <cstdint>

namespace coop::firered {

// Memory facts for the exact profiled FireRed originals. These routines
// operate on paused emulator RAM, never on a battery save or native filesystem.
inline constexpr uint32_t SaveBlock1PointerAddress = 0x03004F58;
inline constexpr uint32_t SaveBlock2PointerAddress = 0x03004F5C;
inline constexpr uint32_t SaveBlock1Size = 0x3D68;
inline constexpr uint32_t SaveBlock2Size = 0xF24;
inline constexpr uint32_t InventoryKeyOffset = 0xF20;
inline constexpr uint32_t WalletOffset = 0x290;
inline constexpr uint32_t MaximumWallet = 999999;
inline constexpr unsigned ItemCount = 375;
inline constexpr unsigned PocketCount = 5;
enum Pocket : unsigned { Items = 0, KeyItems = 1, PokeBalls = 2, Machines = 3, Berries = 4 };
inline constexpr std::array<unsigned, PocketCount> PocketCapacities{{42, 30, 13, 58, 43}};
inline constexpr std::array<uint32_t, PocketCount> PocketOffsets{{0x310, 0x3B8, 0x430, 0x464, 0x54C}};
inline constexpr unsigned AllPockets = (1u << PocketCount) - 1;
inline constexpr unsigned BattlePockets = (1u << Items) | (1u << PokeBalls) | (1u << Berries);

// Numeric pocket metadata only; no descriptions, names, ROM data or executable
// game code are reproduced. Unused item IDs are deliberately rejected.
inline int itemPocket(unsigned item) {
    if (item >= 1 && item <= 12) return PokeBalls;
    if (item >= 133 && item <= 175) return Berries;
    if (item >= 289 && item <= 346) return Machines;
    if ((item >= 259 && item <= 266) || (item >= 268 && item <= 288) ||
        (item >= 349 && item <= 374)) return KeyItems;
    if ((item >= 13 && item <= 51) || (item >= 63 && item <= 71) ||
        (item >= 73 && item <= 81) || (item >= 83 && item <= 86) ||
        (item >= 93 && item <= 98) || (item >= 103 && item <= 104) ||
        (item >= 106 && item <= 111) || (item >= 121 && item <= 132) ||
        (item >= 179 && item <= 225) || (item >= 254 && item <= 258)) return Items;
    return -1;
}
inline bool battleConsumable(unsigned item) {
    return (item >= 1 && item <= 12) || (item >= 13 && item <= 38) || item == 44 ||
           (item >= 73 && item <= 81) || (item >= 133 && item <= 142) || item == 175;
}

struct ItemSlot {
    uint16_t item = 0;
    uint16_t quantity = 0; // Decrypted, independent of either player's key.
    bool operator==(const ItemSlot& other) const { return item == other.item && quantity == other.quantity; }
};
struct Inventory {
    // Fixed sizes retain slot ordering and empty gaps without changing native
    // menu cursor/ordering. Entries beyond each pocket's capacity remain zero.
    std::array<std::array<ItemSlot, 58>, PocketCount> pockets{};
    void validate() const {
        std::array<bool, ItemCount> seen{};
        for (unsigned pocket = 0; pocket < PocketCount; ++pocket)
            for (unsigned slot = 0; slot < pockets[pocket].size(); ++slot) {
                const auto& entry = pockets[pocket][slot];
                requirePokemon(entry.quantity <= 999, "FireRed bag quantity exceeds 999");
                if (slot >= PocketCapacities[pocket] || entry.item == 0) {
                    requirePokemon(entry.item == 0 && entry.quantity == 0, "Invalid empty FireRed bag slot");
                    continue;
                }
                requirePokemon(entry.item < ItemCount && itemPocket(entry.item) == int(pocket),
                               "Invalid FireRed item or bag pocket");
                requirePokemon(entry.quantity > 0, "Nonempty FireRed item has no quantity");
                requirePokemon(!seen[entry.item], "Duplicate FireRed bag item stacks");
                seen[entry.item] = true;
            }
    }
    uint16_t quantity(unsigned item) const {
        const int pocket = itemPocket(item);
        requirePokemon(pocket >= 0, "Invalid FireRed inventory item");
        for (const auto& slot : pockets[unsigned(pocket)]) if (slot.item == item) return slot.quantity;
        return 0;
    }
    void setQuantity(unsigned item, unsigned amount) {
        const int pocket = itemPocket(item);
        requirePokemon(pocket >= 0 && amount <= 999, "Invalid FireRed inventory quantity update");
        validate();
        auto& slots = pockets[unsigned(pocket)];
        for (unsigned i = 0; i < PocketCapacities[unsigned(pocket)]; ++i)
            if (slots[i].item == item) {
                slots[i] = amount == 0 ? ItemSlot{} : ItemSlot{uint16_t(item), uint16_t(amount)};
                return;
            }
        if (amount == 0) return;
        for (unsigned i = 0; i < PocketCapacities[unsigned(pocket)]; ++i)
            if (slots[i].item == 0) {
                slots[i] = {uint16_t(item), uint16_t(amount)};
                return;
            }
        throw std::runtime_error("FireRed bag pocket has no free slot");
    }
    bool operator==(const Inventory& other) const { return pockets == other.pockets; }
};

inline uint32_t inventoryRead32(mCore* core, uint32_t address) {
    uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i) result |= uint32_t(core->busRead8(core, address + i) & 0xFFu) << (i * 8);
    return result;
}
inline uint16_t inventoryRead16(mCore* core, uint32_t address) {
    return uint16_t(core->busRead8(core, address) & 0xFFu) |
           uint16_t((core->busRead8(core, address + 1) & 0xFFu) << 8);
}
inline void inventoryWrite16(mCore* core, uint32_t address, uint16_t value) {
    core->busWrite8(core, address, uint8_t(value)); core->busWrite8(core, address + 1, uint8_t(value >> 8));
}
struct SaveMemory {
    uint32_t block1 = 0, block2 = 0, key = 0;
    bool operator==(const SaveMemory& other) const {
        return block1 == other.block1 && block2 == other.block2 && key == other.key;
    }
};
inline bool validSaveBlock(uint32_t address, uint32_t length) {
    return (address & 3u) == 0 && address >= 0x02000000u && address <= 0x02040000u - length;
}
inline SaveMemory readSaveMemory(mCore* core) {
    requirePokemon(core && core->busRead8, "Cannot read FireRed inventory without an emulator core");
    SaveMemory result;
    result.block1 = inventoryRead32(core, abiAddress(core, SaveBlock1PointerAddress));
    result.block2 = inventoryRead32(core, abiAddress(core, SaveBlock2PointerAddress));
    requirePokemon(validSaveBlock(result.block1, SaveBlock1Size) && validSaveBlock(result.block2, SaveBlock2Size),
                   "FireRed save blocks are uninitialized or outside EWRAM");
    requirePokemon(result.block1 + SaveBlock1Size <= result.block2 || result.block2 + SaveBlock2Size <= result.block1,
                   "FireRed save blocks overlap");
    result.key = inventoryRead32(core, result.block2 + InventoryKeyOffset);
    return result;
}
inline void checkSaveMemory(mCore* core, const SaveMemory& memory) {
    requirePokemon(readSaveMemory(core) == memory, "FireRed save blocks or encryption key moved during transaction");
}
inline Inventory readInventory(mCore* core) {
    const auto memory = readSaveMemory(core);
    Inventory result;
    for (unsigned pocket = 0; pocket < PocketCount; ++pocket)
        for (unsigned slot = 0; slot < PocketCapacities[pocket]; ++slot) {
            const auto address = memory.block1 + PocketOffsets[pocket] + slot * 4;
            result.pockets[pocket][slot] = {inventoryRead16(core, address),
                                          uint16_t(inventoryRead16(core, address + 2) ^ uint16_t(memory.key))};
        }
    checkSaveMemory(core, memory);
    result.validate();
    return result;
}
inline void writeInventory(mCore* core, const Inventory& inventory, unsigned pocketMask = AllPockets) {
    inventory.validate();
    requirePokemon((pocketMask & ~AllPockets) == 0, "Invalid FireRed bag pocket write mask");
    requirePokemon(core && core->busRead8 && core->busWrite8, "Cannot write FireRed inventory without an emulator core");
    const auto memory = readSaveMemory(core);
    // Stage every encrypted byte and revalidate the location before any write.
    // Untargeted pockets, PC items, coins, registered items, flags and save keys
    // remain byte-for-byte untouched. The caller must keep the core paused.
    std::array<std::array<uint8_t, 58 * 4>, PocketCount> staged{};
    for (unsigned pocket = 0; pocket < PocketCount; ++pocket)
        for (unsigned slot = 0; slot < PocketCapacities[pocket]; ++slot) {
            monPut16(staged[pocket].data() + slot * 4, inventory.pockets[pocket][slot].item);
            monPut16(staged[pocket].data() + slot * 4 + 2,
                     inventory.pockets[pocket][slot].quantity ^ uint16_t(memory.key));
        }
    checkSaveMemory(core, memory);
    for (unsigned pocket = 0; pocket < PocketCount; ++pocket)
        if (pocketMask & (1u << pocket))
            for (unsigned offset = 0; offset < PocketCapacities[pocket] * 4; ++offset)
                core->busWrite8(core, memory.block1 + PocketOffsets[pocket] + offset, staged[pocket][offset]);
}
inline uint32_t readWallet(mCore* core) {
    const auto memory = readSaveMemory(core);
    const auto result = inventoryRead32(core, memory.block1 + WalletOffset) ^ memory.key;
    checkSaveMemory(core, memory);
    requirePokemon(result <= MaximumWallet, "Invalid FireRed wallet value");
    return result;
}
inline void writeWallet(mCore* core, uint32_t money) {
    requirePokemon(money <= MaximumWallet, "FireRed wallet exceeds 999999");
    requirePokemon(core && core->busRead8 && core->busWrite8, "Cannot write FireRed wallet without an emulator core");
    const auto memory = readSaveMemory(core);
    const auto encoded = money ^ memory.key;
    checkSaveMemory(core, memory);
    for (unsigned i = 0; i < 4; ++i)
        core->busWrite8(core, memory.block1 + WalletOffset + i, uint8_t(encoded >> (i * 8)));
}

struct InventoryConsumption {
    std::array<uint16_t, ItemCount> amounts{};
    void validate() const {
        for (unsigned item = 0; item < ItemCount; ++item)
            requirePokemon(amounts[item] <= 999 && (amounts[item] == 0 || battleConsumable(item)),
                           "Invalid FireRed battle item consumption");
    }
};
inline InventoryConsumption consumptionDelta(const Inventory& before, const Inventory& after) {
    before.validate(); after.validate();
    InventoryConsumption result;
    for (unsigned item = 1; item < ItemCount; ++item) {
        if (itemPocket(item) < 0) continue;
        const auto oldAmount = before.quantity(item), newAmount = after.quantity(item);
        requirePokemon(newAmount <= oldAmount, "Native FireRed battle unexpectedly added a bag item");
        result.amounts[item] = uint16_t(oldAmount - newAmount);
    }
    result.validate();
    return result;
}
inline Inventory applyConsumption(const Inventory& owner, const InventoryConsumption& consumption) {
    owner.validate(); consumption.validate();
    auto result = owner;
    for (unsigned item = 1; item < ItemCount; ++item)
        if (consumption.amounts[item]) {
            const auto available = result.quantity(item);
            requirePokemon(available >= consumption.amounts[item], "FireRed battle consumed an item its owner did not have");
            result.setQuantity(item, available - consumption.amounts[item]);
        }
    result.validate();
    return result;
}
inline Inventory mergeBattleInventory(const Inventory& owner, const Inventory& before, const Inventory& after) {
    return applyConsumption(owner, consumptionDelta(before, after));
}
inline int32_t walletDelta(uint32_t before, uint32_t after) {
    requirePokemon(before <= MaximumWallet && after <= MaximumWallet, "Invalid FireRed wallet delta");
    return int32_t(after) - int32_t(before);
}
inline uint32_t applyWalletDelta(uint32_t ownerMoney, int64_t delta) {
    requirePokemon(ownerMoney <= MaximumWallet && delta >= -int64_t(MaximumWallet) && delta <= MaximumWallet,
                   "Invalid FireRed wallet merge");
    const auto result = int64_t(ownerMoney) + delta;
    return uint32_t(std::max<int64_t>(0, std::min<int64_t>(MaximumWallet, result)));
}
inline std::array<uint32_t, 2> rewardWallets(const std::array<uint32_t, 2>& original, uint32_t fullReward) {
    requirePokemon(fullReward <= MaximumWallet, "Invalid FireRed shared battle reward");
    return {{applyWalletDelta(original[0], fullReward), applyWalletDelta(original[1], fullReward)}};
}
// walletDelta reflects only the observable native change. If the native wallet
// hit its cap, obtain the uncapped reward from the battle engine instead of
// crediting this truncated difference to the other player. Blackout loss and
// Pay Day distribution are explicit policies owned by the battle coordinator.

} // namespace coop::firered
