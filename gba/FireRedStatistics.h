// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedInventory.h"

namespace coop::firered {
inline constexpr uint32_t MaximumBattleStatistic = 0xFFFFFF;
// Native GetGameStat/SetGameStat use the current save's 32-bit encryption
// key. IncrementGameStat saturates at 0xFFFFFF, including older larger values.
inline uint32_t readGameStatistic(mCore* core, unsigned index) {
    requirePokemon(index < 52, "Invalid FireRed game statistic index");
    const auto memory = readSaveMemory(core);
    const auto value = inventoryRead32(core, memory.block1 + 0x1200 + index * 4) ^ memory.key;
    checkSaveMemory(core, memory);
    return value;
}
inline void writeGameStatistic(mCore* core, unsigned index, uint32_t value) {
    requirePokemon(index < 52 && value <= MaximumBattleStatistic,
                   "Invalid FireRed game statistic value");
    requirePokemon(core && core->busWrite8, "Cannot write FireRed game statistic");
    const auto memory = readSaveMemory(core);
    checkSaveMemory(core, memory);
    const auto encrypted = value ^ memory.key;
    for (unsigned byte = 0; byte < 4; ++byte)
        core->busWrite8(core, memory.block1 + 0x1200 + index * 4 + byte, uint8_t(encrypted >> (byte * 8)));
}
inline void incrementGameStatistic(mCore* core, unsigned index) {
    const auto value = readGameStatistic(core, index);
    writeGameStatistic(core, index, value < MaximumBattleStatistic ? value + 1 : MaximumBattleStatistic);
}
struct BattleStatistics {
    uint32_t total = 0, wild = 0, trainer = 0;
    void validate() const {
        requirePokemon(total <= MaximumBattleStatistic && wild <= MaximumBattleStatistic &&
                       trainer <= MaximumBattleStatistic, "Invalid FireRed battle statistics");
    }
    bool operator==(const BattleStatistics& r) const { return total==r.total && wild==r.wild && trainer==r.trainer; }
};
inline BattleStatistics readBattleStatistics(mCore* core) {
    const auto memory=readSaveMemory(core);
    BattleStatistics result{inventoryRead32(core,memory.block1+0x121C)^memory.key,
                            inventoryRead32(core,memory.block1+0x1220)^memory.key,
                            inventoryRead32(core,memory.block1+0x1224)^memory.key};
    checkSaveMemory(core,memory);result.validate();return result;
}
inline BattleStatistics recordSharedBattle(const BattleStatistics& before, bool trainer) {
    before.validate();auto result=before;
    auto increment=[](uint32_t value){return std::min(value+1,MaximumBattleStatistic);};
    result.total=increment(result.total);
    if(trainer)result.trainer=increment(result.trainer);else result.wild=increment(result.wild);
    return result;
}
inline void writeBattleStatistics(mCore* core,const BattleStatistics& statistics) {
    statistics.validate();requirePokemon(core && core->busWrite8,"Cannot write FireRed battle statistics");
    const auto memory=readSaveMemory(core);checkSaveMemory(core,memory);
    auto word=[&](uint32_t address,uint32_t value){
        for(unsigned i=0;i<4;++i)core->busWrite8(core,address+i,uint8_t(value>>(i*8)));
    };
    word(memory.block1+0x121C,statistics.total^memory.key);
    word(memory.block1+0x1220,statistics.wild^memory.key);
    word(memory.block1+0x1224,statistics.trainer^memory.key);
}
} // namespace coop::firered
