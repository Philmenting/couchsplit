// SPDX-License-Identifier: GPL-3.0-or-later
// Authored, independent Pokemon and journals. No game ROM or save is required.
#include "../gba/FireRedPartyMenu.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f,const char* message){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}check(failed,message);}
Party roster(unsigned owner,unsigned count=6) {
 Party p;p.count=uint8_t(count);
 for(unsigned slot=0;slot<count;++slot) {
  auto& m=p.mons[slot];monPut32(m.bytes.data(),24*(owner*6+slot+1));monPut32(m.bytes.data()+4,owner+1);
  m.bytes[8]=0xBB;m.bytes[9]=0xFF;m.bytes[18]=5;m.bytes[19]=2;m.bytes[20]=0xBB;m.bytes[21]=0xFF;m.bytes[84]=10;
  monPut16(m.bytes.data()+86,30);monPut16(m.bytes.data()+88,30);for(unsigned i=90;i<100;i+=2)monPut16(m.bytes.data()+i,20);
  PartyMon::Secure s{};monPut16(s.data(),1);monPut32(s.data()+4,1000);s[9]=70;
  monPut16(s.data()+12,33);s[20]=35;monPut32(s.data()+40,0x3FFFFFFF);m.setSecure(s);
 }
 p.validate();return p;
}
BenchReferences references(){BenchReferences r;r.active={{0,1}};r.participation={{0x03,0x03}};r.knockedOff=0x3F;return r;}
void nativeOrders() {
 std::array<unsigned,6> ids{{0,1,2,3,4,5}};
 do {
  std::array<uint8_t,3> order{};for(unsigned i=0;i<3;++i)order[i]=uint8_t((ids[i*2]<<4)|ids[i*2+1]);
  validatePartyMenuOrder(order);++checks;
 }while(std::next_permutation(ids.begin(),ids.end()));
 for(const auto& bad:std::array<std::array<uint8_t,3>,4>{{{{0x01,0x23,0x44}},{{0x01,0x23,0x46}},{{0x01,0x23,0x4F}},{{0x00,0x23,0x45}}}})
  rejects([&]{validatePartyMenuOrder(bad);},"Malformed native packed party order accepted");
}
void eachOwnSelection() {
 const auto t=combineParties(roster(0),roster(1));auto current=t.combined;auto refs=references();
 // Preserve the latest actual native HP, PP, status and EXP in the full menu.
 current.mons[2].setHp(13);current.mons[2].setPp(0,7);current.mons[2].setStatus(8);
 auto sec=current.mons[2].secure();monPut32(sec.data()+4,1010);current.mons[2].setSecure(sec);
 const auto beforeJournal=restoreParties(t,current);
 for(unsigned owner=0;owner<2;++owner) {
  const auto session=planPartyMenu(t,current,owner,refs,{{0x10,0x32,0x54}});
  check(session.menu==beforeJournal[owner]&&session.menu.count==6,"Full own party view is incomplete or stale");
  check(session.virtualActive[owner]==0&&session.virtualActive[1-owner]==6,"Virtual menu leaked another trainer's active index");
  for(unsigned own=0;own<6;++own) {
   auto menu=session.menu;std::swap(menu.mons[0],menu.mons[5]); // Native UI reorder is harmless.
   const auto picked=planPartyMenuSelection(session,menu,own);
   if(own==0){check(!picked.accepted&&!picked.reason.empty(),"Menu accepted the already active own Pokemon");continue;}
   check(picked.accepted&&!picked.canceled,"Living own party member could not be selected");
   check(picked.nativeSlot<6&&picked.transaction.owners[picked.nativeSlot]==int(owner),"Own selection mapped to partner's physical slot");
   check(picked.party.mons[picked.nativeSlot].identity()==session.menu.mons[own].identity(),"Selection mapped to the wrong own original slot");
   check(picked.nativeOrder==session.savedNativeOrder,"Menu return leaked virtual party order");
   check(restoreParties(picked.transaction,picked.party)==beforeJournal,"Switch selection lost or changed a journaled Pokemon");
   check(picked.party.mons[refs.active[1-owner]]==current.mons[refs.active[1-owner]],"Switch replaced the other human's active Pokemon");
   if(own>=3) {
    check(picked.imported&&picked.retiredNativeSlot<6,"Unloaded living own reserve was not imported");
    const auto bit=1u<<picked.nativeSlot;
    check(!(picked.references.participation[0]&bit)&&!(picked.references.participation[1]&bit)&&!(picked.references.knockedOff&bit),"New reserve inherited old occupant's masks");
   }else check(!picked.imported,"Existing resident was unnecessarily evicted");
  }
  const auto canceled=planPartyMenuSelection(session,session.menu,6);
  check(canceled.accepted&&canceled.canceled&&canceled.nativeSlot==6&&canceled.party==current,"Cancel mutated the physical battle party");
  check(restoreParties(canceled.transaction,canceled.party)==beforeJournal,"Cancel lost journal state");
 }
}
void protectedReferences() {
 const auto t=combineParties(roster(0),roster(1));auto current=t.combined;auto refs=references();
 // Slot2 is pending own Wish, slot4 pending evolution: both stay resident.
 refs.wishCounters[0]=2;refs.wishSlots[0]=2;refs.leveled=1u<<4;
 auto session=planPartyMenu(t,current,0,refs);auto blocked=planPartyMenuSelection(session,session.menu,5);
 check(!blocked.accepted&&!blocked.reason.empty()&&blocked.party==current,"Protected working set was overwritten");
 check(restoreParties(blocked.transaction,blocked.party)==restoreParties(t,current),"Blocked selection changed journal");
 // A resident Wish/evolution Pokemon can still enter battle without eviction.
 auto existing=planPartyMenuSelection(session,session.menu,1);
 check(existing.accepted&&!existing.imported&&existing.nativeSlot==2,"Resident Wish target was incorrectly prohibited from switching in");
 refs.leveled=0;session=planPartyMenu(t,current,0,refs);auto picked=planPartyMenuSelection(session,session.menu,5);
 check(picked.accepted&&picked.nativeSlot==4&&picked.references.wishSlots==refs.wishSlots&&picked.references.wishCounters==refs.wishCounters,"Menu retired or changed a live Wish reference");
 refs.queued[0]=4;session=planPartyMenu(t,current,0,refs);blocked=planPartyMenuSelection(session,session.menu,5);
 check(!blocked.accepted,"Menu retired a queued selection");
 refs.queued[0]=6;refs.callerExcluded[0]=4;session=planPartyMenu(t,current,0,refs);blocked=planPartyMenuSelection(session,session.menu,5);
 check(!blocked.accepted,"Menu retired a caller-excluded slot");
 refs.callerExcluded[0]=6;refs.wishCounters[0]=0;refs.wishCounters[1]=3;refs.wishSlots[1]=2;
 session=planPartyMenu(t,current,0,refs);picked=planPartyMenuSelection(session,session.menu,5);
 check(picked.accepted&&picked.nativeSlot==2,"Enemy Wish incorrectly pinned an allied party slot");
 refs=references();refs.participation={{0x15,0x15}};
 session=planPartyMenu(t,current,0,refs);blocked=planPartyMenuSelection(session,session.menu,5);
 check(!blocked.accepted&&blocked.party==current,"Living participant was retired before native EXP award");
 current.mons[4].setHp(0);session=planPartyMenu(t,current,0,refs);picked=planPartyMenuSelection(session,session.menu,5);
 check(picked.accepted&&picked.nativeSlot==4&&!(picked.references.participation[0]&(1u<<4)),"Fainted participant did not retire safely without EXP eligibility");
 current=t.combined;
 for(unsigned slot:{2u,4u}){auto share=current.mons[slot].secure();monPut16(share.data()+2,182);current.mons[slot].setSecure(share);}
 refs=references();session=planPartyMenu(t,current,0,refs);blocked=planPartyMenuSelection(session,session.menu,5);
 check(!blocked.accepted,"Resident living EXP Share holder was retired before its native award");
 current=t.combined;
 refs=references();refs.queued[0]=2;session=planPartyMenu(t,current,0,refs);blocked=planPartyMenuSelection(session,session.menu,1);
 check(!blocked.accepted,"Menu accepted an already queued resident");
 current.mons[4].setHp(0);refs=references();session=planPartyMenu(t,current,0,refs);picked=planPartyMenuSelection(session,session.menu,5);
 check(picked.accepted&&picked.nativeSlot==4,"Menu did not prefer retiring an unreferenced fainted bench Pokemon");
}
void livingJournalRoundTrips() {
 auto t=combineParties(roster(0),roster(1));auto current=t.combined;auto refs=references();
 current.mons[2].setHp(11);current.mons[2].setPp(0,9);current.mons[2].setStatus(8);
 auto desired=restoreParties(t,current);
 for(unsigned owner=0;owner<2;++owner) {
  for(unsigned round=0;round<3;++round)for(unsigned own=1;own<6;++own) {
   const auto session=planPartyMenu(t,current,owner,refs);const auto picked=planPartyMenuSelection(session,session.menu,own);
   check(picked.accepted,"Repeated arbitrary reserve selection failed");
   t=picked.transaction;current=picked.party;refs=picked.references;
   check(restoreParties(t,current)==desired,"Retiring a living bench member lost its latest native state");
  }
 }
 // Native party ordering is not ownership parity. Swap non-leading opponents
 // and ensure both active references still follow their original identities.
 t=combineParties(roster(0),roster(1));current=t.combined;std::swap(current.mons[2],current.mons[3]);
 refs=references();auto session=planPartyMenu(t,current,0,refs);auto picked=planPartyMenuSelection(session,session.menu,1);
 check(picked.accepted&&picked.nativeSlot==3&&!picked.imported,"Physical reordering confused ownership");
 check(picked.transaction.owners[2]==1&&picked.transaction.owners[3]==0,"Working ownership was not rebased by identity");
}
void activeSwitchesAndProtectionMatrix() {
 auto t=combineParties(roster(0),roster(1));auto current=t.combined;auto refs=references();
 // Native execution changes the active *physical* index after each reply. On
 // reopening, that index must be translated to the own original roster again.
 for(unsigned round=0;round<2;++round)for(unsigned owner=0;owner<2;++owner)
  for(unsigned own:std::array<unsigned,6>{{5,0,4,1,3,2}}) {
   const auto session=planPartyMenu(t,current,owner,refs);
   check(session.virtualActive[owner]==unsigned(session.transaction.originalSlots[refs.active[owner]]),"Reopened menu has a stale active original index");
   const auto picked=planPartyMenuSelection(session,session.menu,own);
   if(session.ownActiveOriginal==own){check(!picked.accepted,"Already active selection accepted after switching");continue;}
   check(picked.accepted,"A selectable original became unavailable after switching");
   const unsigned otherActive=refs.active[1-owner];const auto other=current.mons[otherActive];
   t=picked.transaction;current=picked.party;refs=picked.references;refs.active[owner]=picked.nativeSlot;
   check(current.mons[otherActive]==other,"Changing native active slots overwrote partner's active record");
  }
 t=combineParties(roster(0),roster(1));current=t.combined;
 for(unsigned a:std::array<unsigned,3>{{0,2,4}})for(unsigned b:std::array<unsigned,3>{{1,3,5}})
  for(unsigned owner=0;owner<2;++owner)for(unsigned leveled=0;leveled<64;++leveled) {
   refs=references();refs.active={{a,b}};refs.leveled=uint8_t(leveled);
   auto session=planPartyMenu(t,current,owner,refs);auto picked=planPartyMenuSelection(session,session.menu,5);
   auto protectedSlots=refs.exclusions();
   for(unsigned slot=0;slot<6;++slot)if((refs.participation[0]|refs.participation[1])&(1u<<slot))protectedSlots.push_back(slot);
   bool possible=false;
   for(unsigned slot=0;slot<6;++slot)possible|=t.owners[slot]==int(owner)&&std::find(protectedSlots.begin(),protectedSlots.end(),slot)==protectedSlots.end();
   check(picked.accepted==possible,"Native level/active reference matrix has a wrong availability decision");
   check(picked.party.mons[refs.active[1-owner]]==current.mons[refs.active[1-owner]],"Protection matrix overwrote partner's active Pokemon");
   check(restoreParties(picked.transaction,picked.party)==restoreParties(t,current),"Protection matrix lost an own or partner journal entry");
  }
}
void shortAndBadRosters() {
 auto t=combineParties(roster(0,2),roster(1,1));auto refs=references();auto session=planPartyMenu(t,t.combined,0,refs);
 check(session.menu.count==2&&session.menu.mons[2].empty(),"Short own party exposed foreign/empty records");
 check(planPartyMenuSelection(session,session.menu,1).accepted,"Short party resident selection failed");
 rejects([&]{planPartyMenuSelection(session,session.menu,2);},"Absent original slot selected");
 rejects([&]{planPartyMenuSelection(session,session.menu,7);},"Invalid native selected slot accepted");
 rejects([&]{planPartyMenu(t,t.combined,2,refs);},"Foreign party-menu owner accepted");
 refs.active={{1,0}};rejects([&]{planPartyMenu(t,t.combined,0,refs);},"Foreign trainer's active party index accepted");
 refs=references();auto full=combineParties(roster(0),roster(1));session=planPartyMenu(full,full.combined,0,refs);
 auto stale=session.menu;stale.mons[1].setHp(1);rejects([&]{planPartyMenuSelection(session,stale,1);},"Switch menu HP mutation accepted");
 stale=session.menu;stale.mons[1]=roster(1).mons[1];rejects([&]{planPartyMenuSelection(session,stale,1);},"Foreign Pokemon substituted through own menu");
 stale=session.menu;stale.count=5;stale.mons[5]={};rejects([&]{planPartyMenuSelection(session,stale,1);},"Menu changed own party size");
 auto own=roster(0);own.mons[5].setHp(0);full=combineParties(own,roster(1));session=planPartyMenu(full,full.combined,0,refs);
 check(!planPartyMenuSelection(session,session.menu,5).accepted,"Menu accepted a fainted unloaded reserve");
 own=roster(0);auto egg=own.mons[5].secure();monPut32(egg.data()+40,monU32(egg.data()+40)|0x40000000u);own.mons[5].bytes[19]|=4;own.mons[5].setSecure(egg);own.validate();
 full=combineParties(own,roster(1));session=planPartyMenu(full,full.combined,0,refs);
 check(!planPartyMenuSelection(session,session.menu,5).accepted,"Menu accepted an Egg reserve");
}
}
int main(){try{nativeOrders();eachOwnSelection();protectedReferences();livingJournalRoundTrips();activeSwitchesAndProtectionMatrix();shortAndBadRosters();std::cout<<checks<<" FireRed party-menu checks passed\n";return 0;}catch(const std::exception&e){std::cerr<<"FireRed party-menu test failed: "<<e.what()<<"\n";return 1;}}
