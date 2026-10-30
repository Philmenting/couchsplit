// SPDX-License-Identifier: GPL-3.0-or-later
#include "../gba/FireRedStatistics.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks=0;
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
struct Bus {
    mCore core{};std::array<uint8_t,0x40000> ram{};std::array<uint8_t,0x8000> iwram{};
    uint32_t first=0x02028000,second=0x02025000;unsigned writes=0;
    uint8_t& at(uint32_t address){
        if(address>=0x02000000 && address<0x02040000)return ram[address-0x02000000];
        check(address>=0x03000000 && address<0x03008000,"Statistic read outside GBA RAM");return iwram[address-0x03000000];
    }
    void put(uint32_t address,uint32_t value){for(unsigned i=0;i<4;++i)at(address+i)=uint8_t(value>>(i*8));}
    explicit Bus(uint32_t key){
        core.board=this;
        core.busRead8=[](mCore*c,uint32_t address)->uint32_t{return static_cast<Bus*>(c->board)->at(address);};
        core.busWrite8=[](mCore*c,uint32_t address,uint8_t value){
            auto& b=*static_cast<Bus*>(c->board);
            check(address>=b.first+0x1200 && address<b.first+0x12D0,"Statistic write changed another save field");
            b.at(address)=value;++b.writes;
        };
        put(SaveBlock1PointerAddress,first);put(SaveBlock2PointerAddress,second);put(second+InventoryKeyOffset,key);
    }
};
}
int main(){try{
 for(auto key:{0u,0x87654321u,0xFFFFFFFFu}){
    Bus bus(key);BattleStatistics before{120,90,30};writeBattleStatistics(&bus.core,before);
    check(bus.writes==12 && readBattleStatistics(&bus.core)==before,"Statistics cipher round trip failed");
    check(inventoryRead32(&bus.core,bus.first+0x121C)==(120^key),"Statistics were not encrypted with own key");
    writeBattleStatistics(&bus.core,recordSharedBattle(before,true));
    check(readBattleStatistics(&bus.core)==BattleStatistics{121,90,31},"Trainer encounter changed the wrong counters");
    check(recordSharedBattle(before,false)==BattleStatistics{121,91,30},"Wild encounter changed the wrong counters");
    const auto saved=bus.ram;const auto writes=bus.writes;bool rejected=false;
    try{writeBattleStatistics(&bus.core,{MaximumBattleStatistic+1,0,0});}catch(const std::runtime_error&){rejected=true;}
    check(rejected && bus.ram==saved && bus.writes==writes,"Invalid statistic partially changed save memory");
 }
 for(auto key:{0u,0x87654321u,0xFFFFFFFFu}) {
    Bus native(key),owner(key^0x12345678u);
    for(unsigned index:{14u,26u,27u}) {
        writeGameStatistic(&native.core,index,100+index);writeGameStatistic(&owner.core,index,200+index);
        const auto beforeNative=native.ram;const auto beforeOwner=owner.ram;
        incrementGameStatistic(&owner.core,index);
        check(native.ram==beforeNative && readGameStatistic(&owner.core,index)==201+index,"Action statistic changed native leader ownership");
        auto expected=beforeOwner;const auto encrypted=(201+index)^(key^0x12345678u);
        for(unsigned byte=0;byte<4;++byte)expected[owner.first-0x02000000+0x1200+index*4+byte]=uint8_t(encrypted>>(byte*8));
        check(owner.ram==expected,"Action statistic changed another save field");
        writeGameStatistic(&owner.core,index,MaximumBattleStatistic);incrementGameStatistic(&owner.core,index);
        check(readGameStatistic(&owner.core,index)==MaximumBattleStatistic,"Action statistic cap overflowed");
        owner.put(owner.first+0x1200+index*4,0xFFFFFFFFu^(key^0x12345678u));incrementGameStatistic(&owner.core,index);
        check(readGameStatistic(&owner.core,index)==MaximumBattleStatistic,"Native legacy statistic clamp differs");
    }
    const auto before=owner.ram;const auto writes=owner.writes;bool rejected=false;
    try{writeGameStatistic(&owner.core,52,0);}catch(const std::runtime_error&){rejected=true;}
    check(rejected&&owner.ram==before&&owner.writes==writes,"Invalid action statistic index wrote RAM");
    rejected=false;try{writeGameStatistic(&owner.core,14,MaximumBattleStatistic+1);}catch(const std::runtime_error&){rejected=true;}
    check(rejected&&owner.ram==before&&owner.writes==writes,"Invalid action statistic value wrote RAM");
 }
 check(recordSharedBattle({MaximumBattleStatistic,MaximumBattleStatistic,MaximumBattleStatistic},true)==
       BattleStatistics{MaximumBattleStatistic,MaximumBattleStatistic,MaximumBattleStatistic},"Native statistic cap overflowed");
 std::cout<<"FireRed shared battle statistics: "<<checks<<" checks passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
