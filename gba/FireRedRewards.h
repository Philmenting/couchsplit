// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedInventory.h"
#include "FireRedEncounter.h"
#include <optional>

namespace coop::firered {

inline constexpr uint32_t TrainerRecordsAddress = 0x0823E998;
inline constexpr uint32_t TrainerMoneyTableAddress = 0x0824F0F0;
inline constexpr unsigned NativeTrainerCount = 743;
inline constexpr unsigned NativeTrainerRecordSize = 40;
inline constexpr uint32_t BattleStructPointerAddress = 0x02023FE8;
inline constexpr uint32_t MoneyMultiplierOffset = 0x4A;
inline constexpr uint32_t PayDayMoneyAddress = 0x02023E7E;

inline uint32_t nativeBlackoutLoss(unsigned highestLevel,unsigned badges,uint32_t wallet) {
    constexpr unsigned factors[] = {2,4,6,9,12,16,20,25,30};
    requirePokemon(highestLevel>=1 && highestLevel<=100 && badges<=8 && wallet<=MaximumWallet,
                   "Invalid FireRed blackout factors");
    return std::min(wallet,uint32_t(highestLevel*4*factors[badges]));
}

inline bool sharedTrainerProgressAllowed(unsigned mode, unsigned trainerClass) {
    // Continued field scripts award story items/badges and move NPCs. Until
    // those are shared transactions, never mark them finished in the guest.
    if (mode != 0 && mode != 4 && mode != 5 && mode != 7) return false;
    switch (trainerClass) {
        case 81: case 82: case 83: case 84: case 85: case 87: case 89: case 90: return false;
        default: return trainerClass >= 57 && trainerClass <= 106;
    }
}

inline bool validRewardRomRange(uint32_t address, uint32_t length) {
    return length > 0 && length <= 0x01000000u && address >= 0x08000000u &&
           address <= 0x09000000u - length;
}
inline uint8_t rewardRomByte(mCore* core, uint32_t address) {
    requirePokemon(core && core->busRead8 && validRewardRomRange(address, 1),
                   "FireRed reward metadata points outside its verified ROM");
    return uint8_t(core->busRead8(core, address));
}

struct TrainerPrizeMetadata {
    uint8_t partyFlags = 0, partySize = 0, lastLevel = 0, trainerClass = 0, classFactor = 0;
    uint32_t partyAddress = 0;
};
inline TrainerPrizeMetadata readTrainerPrizeMetadata(mCore* core, unsigned trainer) {
    requirePokemon(trainer < NativeTrainerCount, "Invalid FireRed trainer reward index");
    const auto record = TrainerRecordsAddress + trainer * NativeTrainerRecordSize;
    requirePokemon(validRewardRomRange(record, NativeTrainerRecordSize), "Invalid FireRed trainer reward record");
    TrainerPrizeMetadata result;
    result.partyFlags = rewardRomByte(core, record);
    result.trainerClass = rewardRomByte(core, record + 1);
    result.partySize = rewardRomByte(core, record + 0x20);
    requirePokemon(result.partyFlags <= 3 && result.partySize >= 1 && result.partySize <= 6,
                   "Invalid FireRed trainer reward party");
    // Verified against the exact German getmoneyreward Thumb instructions:
    // 0x080258B2/0x080258C8 index 8-byte records; 0x080258DE indexes 16-byte
    // records. The original compiler rounds these structs to 4-byte boundaries;
    // inferring 6/14 from decomp field declarations produces incorrect levels.
    const unsigned stride = (result.partyFlags & 1u) ? 16 : 8;
    result.partyAddress = inventoryRead32(core, record + 0x24);
    requirePokemon((result.partyAddress & 3u) == 0 && validRewardRomRange(result.partyAddress, result.partySize * stride),
                   "FireRed trainer party reward data is outside ROM");
    result.lastLevel = rewardRomByte(core, result.partyAddress + (result.partySize - 1) * stride + 2);
    requirePokemon(result.lastLevel >= 1 && result.lastLevel <= 100, "Invalid FireRed trainer reward level");
    // Native code 0x080258F6/0x08025924 indexes four-byte records. A bounded
    // search avoids following missing/corrupt sentinel data outside the table.
    bool found = false;
    for (unsigned index = 0; index < 128; ++index) {
        const auto address = TrainerMoneyTableAddress + index * 4;
        const auto trainerClass = rewardRomByte(core, address);
        // Native code uses the sentinel entry's value as a default factor
        // for classes absent from the table (including two original records).
        if (trainerClass == 0xFF || trainerClass == result.trainerClass) {
            result.classFactor = rewardRomByte(core, address + 1);
            found = true; break;
        }
    }
    requirePokemon(found && result.classFactor > 0, "FireRed trainer class has no valid money factor");
    return result;
}
inline uint32_t trainerPrize(unsigned lastLevel, unsigned classFactor, unsigned multiplier, bool originalDouble) {
    requirePokemon(lastLevel >= 1 && lastLevel <= 100 && classFactor >= 1 && classFactor <= 255 &&
                   multiplier >= 1 && multiplier <= 2, "Invalid FireRed trainer prize factors");
    return uint32_t(4u * lastLevel * classFactor * multiplier * (originalDouble ? 2u : 1u));
}
inline uint32_t nativeTrainerPrize(mCore* core, unsigned trainer, uint32_t originalFlags, unsigned multiplier) {
    requirePokemon(sharedEncounterFlags(originalFlags) && multiplier >= 1 && multiplier <= 2,
                   "Unsupported FireRed reward battle flags or multiplier");
    if (!(originalFlags & 8u)) return 0;
    const auto metadata = readTrainerPrizeMetadata(core, trainer);
    return trainerPrize(metadata.lastLevel, metadata.classFactor, multiplier, bool(originalFlags & 1u));
}

struct NativeRewardState {
    uint16_t payDayMoney = 0;
    uint8_t moneyMultiplier = 1;
};
// Sample during the battle, before FreeBattleResources clears gBattleStruct.
// An absent or freshly allocated/uninitialized struct returns nullopt. Any
// established multiplier other than native 1/2 is rejected. Cache the last valid
// sample for the transaction boundary; never infer money from capped wallets.
inline std::optional<NativeRewardState> readNativeRewardState(mCore* core) {
    requirePokemon(core && core->busRead8, "Cannot inspect FireRed battle reward state");
    const auto pointer = inventoryRead32(core, BattleStructPointerAddress);
    if (pointer == 0) return std::nullopt;
    requirePokemon(validSaveBlock(pointer, MoneyMultiplierOffset + 1), "Invalid FireRed battle reward pointer");
    const auto multiplier = uint8_t(core->busRead8(core, pointer + MoneyMultiplierOffset));
    if (multiplier == 0) return std::nullopt;
    requirePokemon(multiplier <= 2, "Invalid FireRed battle money multiplier");
    NativeRewardState result{inventoryRead16(core, PayDayMoneyAddress), multiplier};
    requirePokemon(inventoryRead32(core, BattleStructPointerAddress) == pointer,
                   "FireRed battle reward pointer moved during sampling");
    return result;
}
inline uint32_t payDayReward(const NativeRewardState& state) {
    requirePokemon(state.moneyMultiplier >= 1 && state.moneyMultiplier <= 2,
                   "Invalid FireRed Pay Day money multiplier");
    return uint32_t(state.payDayMoney) * state.moneyMultiplier;
}

struct RewardShares {
    uint32_t trainer = 0, payDay = 0, total = 0;
    std::array<uint32_t, 2> shares{};
};
inline RewardShares splitBattleRewards(uint32_t originalTrainerPrize, uint32_t fullPayDayReward, unsigned initiator) {
    requirePokemon(initiator < 2 && originalTrainerPrize <= MaximumWallet && fullPayDayReward <= MaximumWallet,
                   "Invalid FireRed shared reward");
    RewardShares result;
    result.trainer = originalTrainerPrize; result.payDay = fullPayDayReward;
    result.total = originalTrainerPrize + fullPayDayReward;
    // Remove synthetic-double inflation from the trainer component BEFORE
    // calling this function. Pay Day is added at its full native value.
    result.shares = {{result.total / 2, result.total / 2}};
    result.shares[initiator] += result.total % 2;
    return result;
}
inline std::array<uint32_t, 2> applyBattleRewards(const std::array<uint32_t, 2>& wallets, const RewardShares& rewards) {
    requirePokemon(rewards.trainer <= MaximumWallet && rewards.payDay <= MaximumWallet &&
                   rewards.total == rewards.trainer + rewards.payDay &&
                   rewards.shares[0] <= MaximumWallet && rewards.shares[1] <= MaximumWallet &&
                   rewards.shares[0] + rewards.shares[1] == rewards.total,
                   "Invalid FireRed reward distribution");
    return {{applyWalletDelta(wallets[0], rewards.shares[0]), applyWalletDelta(wallets[1], rewards.shares[1])}};
}

} // namespace coop::firered
