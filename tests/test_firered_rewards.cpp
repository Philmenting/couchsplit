// SPDX-License-Identifier: GPL-3.0-or-later
// Authored trainer metadata and emulator RAM: no commercial ROM required.
#include "../gba/FireRedRewards.h"
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
    std::vector<uint8_t> rom = std::vector<uint8_t>(0x1000000);
    std::array<uint8_t, 0x40000> ram{};
    mCore core{};
    Bus() {
        core.board = this;
        core.busRead8 = [](mCore* core, uint32_t address) -> uint32_t {
            return static_cast<Bus*>(core->board)->at(address);
        };
        at(TrainerMoneyTableAddress) = 57; at(TrainerMoneyTableAddress + 1) = 4;
        at(TrainerMoneyTableAddress + 2) = 0xCC; at(TrainerMoneyTableAddress + 3) = 0xDD;
        at(TrainerMoneyTableAddress + 4) = 58; at(TrainerMoneyTableAddress + 5) = 3;
        at(TrainerMoneyTableAddress + 8) = 0xFF;
    }
    uint8_t& at(uint32_t address) {
        if (address >= 0x08000000 && address < 0x09000000) return rom[address - 0x08000000];
        check(address >= 0x02000000 && address < 0x02040000, "Reward read outside verified memory");
        return ram[address - 0x02000000];
    }
    void put32(uint32_t address, uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) at(address + i) = uint8_t(value >> (i * 8));
    }
    void trainer(unsigned id, unsigned flags, unsigned count = 3) {
        const auto record = TrainerRecordsAddress + id * NativeTrainerRecordSize;
        at(record) = uint8_t(flags); at(record + 1) = 58; at(record + 0x20) = uint8_t(count);
        const auto party = 0x08200000u + id * 0x100;
        put32(record + 0x24, party);
        const auto stride = flags & 1 ? 16 : 8;
        for (unsigned i = 0; i < count; ++i) at(party + i * stride + 2) = uint8_t(3 + i);
    }
};
void metadata() {
    Bus bus;
    for (unsigned flags = 0; flags < 4; ++flags) {
        const auto id = flags + 1; bus.trainer(id, flags);
        const auto meta = readTrainerPrizeMetadata(&bus.core, id);
        check(meta.partyFlags == flags && meta.partySize == 3 && meta.lastLevel == 5 && meta.classFactor == 3,
              "Wrong trainer record or native stride");
        check(nativeTrainerPrize(&bus.core, id, 8, 1) == 60, "Ordinary trainer reward failed");
        check(nativeTrainerPrize(&bus.core, id, 9, 1) == 120, "Original double reward failed");
        check(nativeTrainerPrize(&bus.core, id, 8, 2) == 120, "Amulet Coin reward failed");
    }
    check(nativeTrainerPrize(&bus.core, 0, 0, 1) == 0, "Wild battle read nonexistent trainer");
    for(unsigned flags:{0xA000u,0x20000u,0x42000u})
        check(nativeTrainerPrize(&bus.core,743,flags,1)==0,"Scripted wild battle read stale trainer metadata or rejected its native type");
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 743); }, "Out-of-table trainer accepted");
    rejects([&] { nativeTrainerPrize(&bus.core, 1, 0x100, 1); }, "Unsupported battle reward type accepted");
    const auto record = TrainerRecordsAddress + NativeTrainerRecordSize;
    bus.at(record) = 4;
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 1); }, "Unknown party flags accepted");
    bus.trainer(1, 0); bus.at(record + 0x20) = 0;
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 1); }, "Empty trainer party accepted");
    bus.trainer(1, 0); bus.put32(record + 0x24, 0x08FFFFF8);
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 1); }, "Trainer party crossing ROM end accepted");
    bus.trainer(1, 0); bus.put32(record + 0x24, 0x02000000);
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 1); }, "Trainer party in RAM accepted");
    bus.trainer(1, 0); bus.at(0x08200100 + 18) = 101;
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 1); }, "Invalid trainer level accepted");
    bus.trainer(1, 0); bus.at(record + 1) = 59;
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 1); }, "Zero fallback class factor accepted");
    bus.at(TrainerMoneyTableAddress + 9) = 5;
    check(readTrainerPrizeMetadata(&bus.core, 1).classFactor == 5, "Native sentinel fallback factor ignored");
    for (unsigned index = 0; index < 128; ++index) bus.at(TrainerMoneyTableAddress + index * 4) = 60;
    rejects([&] { readTrainerPrizeMetadata(&bus.core, 1); }, "Unterminated factor table accepted");
    rejects([&] { rewardRomByte(nullptr, TrainerMoneyTableAddress); }, "Null reward core accepted");
}
void nativeState() {
    Bus bus;
    check(!readNativeRewardState(&bus.core), "Absent battle struct accepted as active");
    bus.put32(BattleStructPointerAddress, 0x02010000);
    check(!readNativeRewardState(&bus.core), "Fresh uninitialized reward state accepted");
    bus.at(0x02010000 + MoneyMultiplierOffset) = 2;
    bus.at(PayDayMoneyAddress) = 0xFE; bus.at(PayDayMoneyAddress + 1) = 0xFF;
    const auto state = readNativeRewardState(&bus.core);
    check(state && state->moneyMultiplier == 2 && state->payDayMoney == 65534, "Wrong native Pay Day state");
    check(payDayReward(*state) == 131068, "Pay Day multiplier was truncated");
    bus.at(0x02010000 + MoneyMultiplierOffset) = 3;
    rejects([&] { readNativeRewardState(&bus.core); }, "Invalid multiplier accepted");
    bus.put32(BattleStructPointerAddress, 0x0203FFF0);
    rejects([&] { readNativeRewardState(&bus.core); }, "Out-of-range battle struct accepted");
    bus.put32(BattleStructPointerAddress, 0x02010001);
    rejects([&] { readNativeRewardState(&bus.core); }, "Unaligned battle struct accepted");
    rejects([&] { payDayReward({1, 0}); }, "Invalid pure Pay Day state accepted");
}
void distribution() {
    check(nativeBlackoutLoss(50,0,1234)==400 && nativeBlackoutLoss(10,0,9876)==80,
          "Blackout did not use the owner's level");
    check(nativeBlackoutLoss(100,8,999999)==12000 && nativeBlackoutLoss(100,8,17)==17,
          "Badge multiplier or wallet cap for blackout is wrong");
    rejects([&]{nativeBlackoutLoss(0,0,10);},"Invalid blackout party accepted");
    rejects([&]{nativeBlackoutLoss(10,9,10);},"Invalid blackout badge count accepted");
    check(sharedTrainerProgressAllowed(0,57), "Ordinary route trainer progress was blocked");
    check(!sharedTrainerProgressAllowed(2,57), "Continued field script progress was shared without its rewards");
    check(!sharedTrainerProgressAllowed(0,84) && !sharedTrainerProgressAllowed(0,81), "Gym/rival progress was shared without story transactions");
    for (unsigned initiator = 0; initiator < 2; ++initiator) {
        const auto rewards = splitBattleRewards(96, 0, initiator);
        const auto money = applyBattleRewards({{1410, 1460}}, rewards);
        check(money[0] == 1458 && money[1] == 1508, "Ordinary trainer money not shared equally");
        const auto odd = splitBattleRewards(96, 3, initiator);
        check(odd.total == 99 && odd.shares[initiator] == 50 && odd.shares[1 - initiator] == 49,
              "Initiator did not receive odd reward unit");
        const auto mixed = splitBattleRewards(trainerPrize(5, 3, 1, false), payDayReward({13, 1}), initiator);
        check(mixed.trainer == 60 && mixed.payDay == 13 && mixed.total == 73,
              "Synthetic double correction reduced Pay Day reward");
        const auto doubleMixed = splitBattleRewards(trainerPrize(5, 3, 2, true), payDayReward({13, 2}), initiator);
        check(doubleMixed.total == 266, "Original double or Amulet Coin lost reward");
        const auto capped = applyBattleRewards({{999990, 100}}, splitBattleRewards(96, 0, initiator));
        check(capped[0] == 999999 && capped[1] == 148, "One wallet cap reduced the other owner's share");
        const auto richHost = applyBattleRewards({{999999, 0}}, splitBattleRewards(96, 0, initiator));
        check(richHost[1] == 48, "Capped native wallet erased guest reward");
    }
    const auto wild = splitBattleRewards(0, payDayReward({7, 2}), 0);
    check(wild.total == 14 && wild.shares[0] == 7 && wild.shares[1] == 7, "Wild Pay Day sharing failed");
    check(trainerPrize(100, 255, 2, true) == 408000, "Trainer prize overflow");
    rejects([&] { trainerPrize(0, 4, 1, false); }, "Invalid trainer level accepted");
    rejects([&] { trainerPrize(5, 4, 3, false); }, "Invalid multiplier accepted");
    rejects([&] { splitBattleRewards(0, 0, 2); }, "Invalid initiator accepted");
    auto forged = splitBattleRewards(96, 0, 0); forged.shares[0] = 97;
    rejects([&] { applyBattleRewards({{0, 0}}, forged); }, "Inconsistent reward shares accepted");
}
}
int main() {
    try {
        metadata(); nativeState(); distribution();
        std::cout << "FireRed reward ownership: " << checks << " checks passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
