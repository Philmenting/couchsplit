// SPDX-License-Identifier: GPL-3.0-or-later
// Authored campaign state only; no ROM/checkpoint or original game assets.
#include "../gba/FireRedCampaign.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks=0;
void check(bool value,const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
Progress ready() {
    Progress p;p.flag(FlagPokemonObtained,true);p.flag(FlagPokedexObtained,true);p.var(VarLabScene,6);
    p.var(0x4031,0);p.inventory.setQuantity(13,7);p.gcnLinkFlags=0x98765431;return p;
}
void stable(const Progress& source,const Progress& recipient) {
    const auto first=reconcileCampaign(source,recipient);
    const auto second=reconcileCampaign(source,first.progress);
    check(!second.changed,"Campaign replay was not idempotent");
    check(second.progress==first.progress,"Campaign replay changed persistent state");
}
void gyms() {
    for (const auto& gym:CampaignGyms) {
        auto source=ready();source.flag(gym.badge,true);source.flag(gym.defeated,true);
        auto own=ready();own.inventory.setQuantity(13,23);own.inventory.setQuantity(gym.machine,5);
        auto got=reconcileCampaign(source,own);
        check(got.changed && got.progress.flag(gym.badge) && got.progress.flag(gym.defeated),"Gym did not grant own badge");
        check(got.progress.flag(0x500+gym.trainer),"Gym did not set own trainer victory");
        check(got.progress.inventory.quantity(gym.machine)==6 && got.progress.flag(gym.receipt),"Gym reward or receipt missing");
        check(got.progress.inventory.quantity(13)==23,"Gym replaced personal inventory");
        const auto firstTM=reconcileCampaign(source,ready());
        check(firstTM.progress.inventory.quantity(TMCaseItem)==1,"Gym reward did not create TM Case");
        for (const auto trainer:CampaignGymTrainers[gym.badge-0x821])
            if (trainer) check(got.progress.flag(0x500+trainer),"Native subordinate gym trainer receipt missing");
        stable(source,own);
        auto spent=got.progress;spent.inventory.setQuantity(gym.machine,0);
        check(!reconcileCampaign(source,spent).changed,"Spent gym TM was recreated");
        own.inventory.setQuantity(gym.machine,999);const auto blocked=reconcileCampaign(source,own);
        check(blocked.changed && blocked.deferredRewards && blocked.progress.flag(gym.badge),"Full stack blocked badge instead of deferring TM");
        check(!blocked.progress.flag(gym.receipt) && blocked.progress.inventory.quantity(gym.machine)==999,"Undelivered gym TM receipt set");
        own=blocked.progress;own.inventory.setQuantity(gym.machine,998);
        const auto later=reconcileCampaign(source,own);
        check(later.progress.inventory.quantity(gym.machine)==999 && later.progress.flag(gym.receipt),"Deferred reward not recoverable after space freed");
        source.flag(gym.defeated,false);
        check(!reconcileCampaign(source,ready()).progress.flag(gym.badge),"Partial source gym scene granted badge");
    }
}
void rivals() {
    for(unsigned starter=0;starter<3;++starter) {
        auto source=ready();source.var(0x4031,(starter+1)%3);source.var(0x4052,1);source.flag(0x29B,true);
        auto own=ready();own.var(0x4031,starter);
        auto got=reconcileCampaign(source,own);
        check(got.progress.var(0x4031)==starter,"Rival overwrote own starter");
        check(got.progress.flag(0x500+332+2-starter),"Rival defeat variant did not follow own starter");
        check(got.progress.inventory.quantity(363)==1 && got.progress.flag(0x29B),"Own Fame Checker missing");
        stable(source,own);

        auto early=ready();early.var(0x4031,(starter+1)%3);early.var(0x4054,1);
        auto guest=ready();guest.var(0x4031,starter);guest.var(0x4054,1);
        const unsigned sourceVariant=329+2-early.var(0x4031), ownVariant=329+2-starter;
        early.flag(0x500+sourceVariant,true);
        check(!reconcileCampaign(early,guest).changed,"Early rival synced before its native scene finished");
        early.var(0x4054,2);early.flag(0x500+sourceVariant,false);
        check(!reconcileCampaign(early,guest).changed,"Route 22 stage alone invented a rival victory");
        early.flag(0x500+sourceVariant,true);
        const auto shared=reconcileCampaign(early,guest);
        check(shared.progress.var(0x4054)==2 && shared.progress.flag(0x500+ownVariant),
              "Completed Route 22 rival did not reach the guest's own variant");
        check(!shared.progress.flag(0x500+sourceVariant) && shared.progress.inventory==guest.inventory,
              "Route 22 rival copied another starter variant or awarded an item");
        stable(early,guest);
        early.flag(0x500+sourceVariant,false);early.var(0x4054,3);
        check(!reconcileCampaign(early,guest).changed,"Later Route 22 progress invented a skipped early rival");
    }
}
void keys() {
    auto source=ready();source.flag(0x233,true);source.flag(0x234,true);source.flag(0x235,true);
    auto own=ready();auto got=reconcileCampaign(source,own);
    check(got.progress.inventory.quantity(265)==1 && got.progress.flag(0x234),"SS ticket missing");
    check(got.progress.flag(0x032) && !got.progress.flag(0x033),"Bill transformation hide/show incorrect");
    stable(source,own);
    source=ready();for(const auto flag:{0x237u,0x238u,0x239u,0x23Au,0x23Bu,0x2EFu,0x1F1u}) source.flag(flag,true);
    own=ready();own.inventory.setQuantity(353,1);got=reconcileCampaign(source,own);
    for(unsigned hm=339;hm<=345;++hm)check(got.progress.inventory.quantity(hm)==1,"Campaign HM missing");
    check(got.progress.inventory.quantity(353)==0,"Strength exchange did not remove own Gold Teeth");
    check(got.progress.var(0x407E)==1,"Cut did not arm native personal ship departure");
    stable(source,own);
    source=ready();source.flag(0x241,true);source.flag(0x271,true);own=ready();own.inventory.setQuantity(352,1);
    got=reconcileCampaign(source,own);
    check(got.progress.inventory.quantity(360)==1 && got.progress.inventory.quantity(352)==0,"Bike exchange duplicated voucher");
    stable(source,own);
    source=ready();source.flag(0x2A6,true);source.var(0x4062,1);
    got=reconcileCampaign(source,ready());check(got.progress.inventory.quantity(369)==1 && got.progress.var(0x4062)==1,"Tea gate missing");
    source=ready();source.flag(0x192,true);source.flag(0x1A8,true);source.flag(0x189,true);source.flag(0x2A5,true);
    source.flag(0x036,true);source.inventory.setQuantity(359,1);source.inventory.setQuantity(355,1);
    for(unsigned id=0x27A;id<=0x28D;++id) source.flag(id,true);
    got=reconcileCampaign(source,ready());
    for(const auto item:{355u,351u,353u,356u,359u})check(got.progress.inventory.quantity(item)==1,"Campaign gating key missing");
    for(unsigned id=0x27A;id<=0x28D;++id)check(got.progress.flag(id),"Persistent Silph door missing");
    stable(source,ready());
    source=ready();source.flag(0x23C,true);source.flag(0x23D,true);source.flag(0x054,true);source.flag(0x080,true);source.var(0x4059,1);
    got=reconcileCampaign(source,ready());
    check(got.progress.inventory.quantity(350)==1 && got.progress.var(0x4059)==1,"Tower/Flute missing");
    check(got.progress.flag(0x054)&&got.progress.flag(0x080),"Cleared Snorlax roadblocks missing");
    stable(source,ready());
}
void sevii() {
    auto source=ready();source.flag(0x845,true);source.var(0x4076,1);
    auto own=ready();auto got=reconcileCampaign(source,own);
    check(got.progress.inventory.quantity(367)==1 && got.progress.inventory.quantity(280)==1,"Sevii assignment keys missing");
    check(got.progress.var(0x4076)==1 && got.progress.var(0x4075)==3 && !got.progress.flag(0x841),"Sevii assignment scene/outage missing");
    stable(source,own);
    own=got.progress;source.flag(0x2A3,true);source.var(0x4076,2);source.var(0x407B,4);
    got=reconcileCampaign(source,own);
    check(got.progress.var(0x4076)==2 && got.progress.inventory.quantity(280)==0,"Meteorite delivery failed");
    check(got.progress.flag(0x2A3) && got.progress.flag(0x07A) && got.progress.var(0x407B)==4,"Lostelle/bikers world missing");
    stable(source,own);
    own=got.progress;source.flag(0x2A1,true);source.var(0x4076,3);got=reconcileCampaign(source,own);
    check(got.progress.var(0x4076)==3 && !got.progress.flag(0x841),"Bill departure did not restore own PC");
    stable(source,own);
    own=got.progress;source.flag(0x82C,true);source.flag(0x840,true);source.var(0x404E,0x6258);source.var(0x4076,4);source.flag(0x2DD,true);
    got=reconcileCampaign(source,own);
    check(got.enableNationalDex && got.enterHallOfFame,"Native-only unlock obligations missing");
    check(!got.progress.flag(0x82C) && !got.progress.flag(0x840) && got.progress.var(0x404E)==0,"Native unlock was imitated by unsafe flags");
    check(got.progress.inventory.quantity(373)==0,"Ruby quest skipped own native postgame unlocks");
    stable(source,own);
    own=got.progress;own.flag(0x82C,true);own.flag(0x840,true);own.var(0x404E,0x6258);
    got=reconcileCampaign(source,own);
    check(got.progress.inventory.quantity(373)==1,"Ruby personal quest item missing");
    own=got.progress;source.flag(0x846,true);source.var(0x4076,5);got=reconcileCampaign(source,own);
    check(got.progress.inventory.quantity(368)==1 && got.progress.inventory.quantity(367)==0 && got.progress.inventory.quantity(373)==0,"Rainbow Pass exchange failed");
    check(got.progress.flag(0x846)&&got.progress.var(0x4076)==5,"Rainbow unlock missing");stable(source,own);
    own=got.progress;source.flag(0x2DC,true);source.flag(0x2D5,true);source.flag(0x2D6,true);source.flag(0x2E3,true);source.var(0x4080,1);
    got=reconcileCampaign(source,own);check(got.progress.inventory.quantity(374)==1,"Recovered Sapphire missing");
    check(got.progress.flag(0x2E3)&&got.progress.flag(0x2D6)&&got.progress.var(0x4080)==1,"Sevii cavern/warehouse gates missing");stable(source,own);
    own=got.progress;source.flag(0x844,true);source.var(0x4076,6);got=reconcileCampaign(source,own);
    check(got.progress.inventory.quantity(374)==1&&got.progress.var(0x4076)==5&&!got.progress.flag(0x05C),"Machine stage or Sapphire changed before native completion");
    check(got.completeNetwork&&!got.progress.flag(0x844),"Machine completion skipped native roamer/postgame operation");
    stable(source,own);
    // Simulate the independently verified native completion operation.
    got.progress.inventory.setQuantity(374,0);got.progress.var(0x4076,6);got.progress.flag(0x844,true);
    // A less advanced partner must never reintroduce consumed quest keys/outage.
    source.var(0x4076,1);source.flag(0x846,false);source.flag(0x844,false);source.flag(0x2DD,false);source.flag(0x2DC,false);
    const auto older=reconcileCampaign(source,got.progress);
    check(older.progress.inventory.quantity(367)==0&&older.progress.inventory.quantity(280)==0&&!older.progress.flag(0x841),"Older source regressed completed Sevii quest");
}
void deferred() {
    auto source=ready();source.flag(0x845,true);source.var(0x4076,2);source.flag(0x2A3,true);
    auto own=ready();own.inventory.setQuantity(147,999);own.inventory.setQuantity(94,999);
    auto blocked=reconcileCampaign(source,own);
    check(blocked.deferredRewards&&blocked.progress.flag(0x2A3)&&blocked.progress.flag(0x07A),"Full optional Lostelle berry blocked native rescue progress");
    check(!blocked.progress.flag(0x2FB)&&blocked.progress.flag(0x2FE),"Moon Stone native delayed claim lost");
    check(blocked.progress.inventory.quantity(147)==999&&blocked.progress.inventory.quantity(94)==999,"Full rewards changed existing own stacks");
    own=blocked.progress;own.inventory.setQuantity(147,998);own.inventory.setQuantity(94,998);
    auto delivered=reconcileCampaign(source,own);
    check(delivered.progress.flag(0x2A3)&&delivered.progress.flag(0x2FB),"Deferred Moon Stone receipt not reconstructible");
    check(delivered.progress.inventory.quantity(147)==998&&delivered.progress.inventory.quantity(94)==999,"Native Lostelle no-later-claim berry or deferred Moon Stone semantics changed");
    stable(source,delivered.progress);
    own=ready();unsigned keys=0;
    for(unsigned item=1;item<ItemCount&&keys<PocketCapacities[KeyItems];++item)
        if(itemPocket(item)==KeyItems&&item!=365&&item!=367&&item!=280){own.inventory.setQuantity(item,1);++keys;}
    check(keys==PocketCapacities[KeyItems],"Full key-pocket fixture incomplete");
    blocked=reconcileCampaign(source,own);
    check(blocked.deferredRewards&&blocked.progress.flag(0x2A3)&&!blocked.progress.flag(0x847),"Full key pocket blocked rescue or fabricated Berry Pouch receipt");
    check(blocked.progress.inventory.quantity(147)==0&&blocked.progress.inventory.quantity(365)==0,"Berry reward bypassed native pouch capacity");
    source=ready();source.flag(0x82F,true);auto shoes=reconcileCampaign(source,ready());
    check(shoes.progress.flag(0x82F)&&shoes.progress.flag(0x092),"Running Shoes did not grant own field ability");
    // Visiting Lavender may skip the SS Anne rival, but is not a battle win.
    source=ready();source.var(0x405B,1);auto skipped=reconcileCampaign(source,ready());
    check(skipped.progress.var(0x405B)==1&&!skipped.progress.flag(0x500+428),"Lavender bypass invented a rival win");
    // Native ThreeIsland_Port restores PC use before the Lostelle quest ends.
    source=ready();source.flag(0x845,true);source.flag(0x2A1,true);source.var(0x4076,1);
    own=ready();own.flag(0x841,true);auto pc=reconcileCampaign(source,own);
    check(!pc.progress.flag(0x841),"Third-island visit failed to restore native PC access");
    stable(source,pc.progress);
}
void boundaries() {
    auto source=ready();auto own=ready();source.flag(0x821,true);source.flag(0x4B1,true);
    auto unstarted=own;unstarted.flag(FlagPokemonObtained,false);
    check(!reconcileCampaign(source,unstarted).changed,"Campaign skipped own starter prerequisite");
    unstarted=own;unstarted.flag(FlagPokedexObtained,false);
    check(!reconcileCampaign(source,unstarted).changed,"Campaign skipped own Pokedex prerequisite");
    // Set all unreviewed source bits, including temp/private gift/League/HOF;
    // a source without a reviewed milestone may not silently union them.
    source=ready();
    for(unsigned flag:{1u,0x028u,0x154u,0x232u,0x246u,0x263u,0x272u,0x273u,0x278u,0x2C0u,0x4B8u,0x4BCu,0x82Cu,0x840u,0x890u})source.flag(flag,true);
    source.var(0x4000,15);source.var(0x4010,32);source.var(0x4031,2);source.var(0x4037,11);source.var(0x4064,2);source.var(0x4068,5);source.var(0x405E,1);
    const auto got=reconcileCampaign(source,own);
    check(!got.changed,"Private/resettable/source-only state leaked into campaign");
    check(got.enterHallOfFame && !got.enableNationalDex,"Native obligations ignored validity predicates");
    source=ready();source.var(0x4052,0xFFFF);const auto bad=reconcileCampaign(source,own);
    check(bad.conflict&&!bad.changed,"Invalid scene state was blindly max-copied");
    // Later own scene stages are never regressed by a late-arriving old event.
    source=ready();source.flag(0x826,true);source.flag(0x4B6,true);own=ready();own.var(0x4071,3);
    check(reconcileCampaign(source,own).progress.var(0x4071)==3,"Blaine reset later Bill scene");
    // Both queues retain independent bag ordering and unrelated progress.
    own=ready();own.mapGroup=12;own.mapNum=34;own.var(0x4037,5);own.flag(0x263,true);
    const auto personal=reconcileCampaign(source,own);
    check(personal.progress.mapGroup==12&&personal.progress.mapNum==34&&personal.progress.var(0x4037)==5&&personal.progress.flag(0x263),"Campaign overwrote own location or personal progress");
    check(personal.progress.gcnLinkFlags==own.gcnLinkFlags,"Campaign changed unrelated dex link bits");
}
void continuedRocketGates() {
    for (const unsigned trainer : {366u,367u}) {
        auto source=ready(),own=ready();source.flag(0x500+trainer,true);
        const auto got=reconcileCampaign(source,own);
        check(got.changed&&got.progress.flag(0x500+trainer),"Reviewed Rocket Hideout barrier win did not reach partner");
        check(got.progress.inventory==own.inventory,"Barrier win invented an item reward");
        check(!got.progress.flag(0x500+(trainer==366?367:366)),"One barrier grunt invented the other victory");
        stable(source,own);
    }
    for (const auto& grunt : std::array<std::pair<unsigned,unsigned>,3>{{{369,0x05E},{370,0x083},{371,0x084}}}) {
        auto source=ready(),own=ready();source.flag(0x500+grunt.first,true);
        check(!reconcileCampaign(source,own).changed,"Tower grunt defeat bypassed its unfinished native exit");
        source.flag(0x500+grunt.first,false);source.flag(grunt.second,true);
        check(!reconcileCampaign(source,own).changed,"Tower hide flag invented a trainer victory");
        source.flag(0x500+grunt.first,true);const auto got=reconcileCampaign(source,own);
        check(got.changed&&got.progress.flag(0x500+grunt.first)&&got.progress.flag(grunt.second),"Completed native Tower grunt exit did not reach partner");
        check(got.progress.inventory==own.inventory,"Tower grunt invented a gift or flute");
        check(!got.progress.flag(0x23C)&&!got.progress.flag(0x23D),"One Tower grunt prematurely rescued Fuji or granted the flute");
        stable(source,own);
    }
    auto source=ready();source.flag(0x500+368,true);
    check(!reconcileCampaign(source,ready()).changed,"Lift-key grunt defeat was generalized into an unreviewed gift receipt");
}
}
int main() {
    try {gyms();rivals();keys();sevii();deferred();boundaries();continuedRocketGates();std::cout<<checks<<" FireRed campaign checks passed\n";return 0;}
    catch(const std::exception& e) {std::cerr<<"FireRed campaign test failed: "<<e.what()<<"\n";return 1;}
}
