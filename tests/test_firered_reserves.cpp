// SPDX-License-Identifier: GPL-3.0-or-later
// Authored ownership journals and encoded names; no game ROM or save required.
#include "../gba/FireRedReserves.h"
#include <iostream>
using namespace coop::firered;
namespace {
unsigned checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F f,const char* message){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}check(failed,message);}
Party roster(unsigned owner,unsigned count=6) {
    Party party;party.count=uint8_t(count);
    for(unsigned slot=0;slot<count;++slot) {
        auto& mon=party.mons[slot];monPut32(mon.bytes.data(),24*(owner*6+slot+1));monPut32(mon.bytes.data()+4,owner+1);
        std::fill(mon.bytes.begin()+8,mon.bytes.begin()+18,0xFF);
        mon.bytes[8]=uint8_t(0xBB+owner);mon.bytes[9]=uint8_t(0xA1+slot);mon.bytes[18]=5;mon.bytes[19]=2;
        mon.bytes[20]=0xBB;mon.bytes[21]=0xFF;mon.bytes[84]=10;
        monPut16(mon.bytes.data()+86,30);monPut16(mon.bytes.data()+88,30);
        for(unsigned stat=90;stat<100;stat+=2)monPut16(mon.bytes.data()+stat,20);
        PartyMon::Secure plain{};monPut16(plain.data(),1);monPut32(plain.data()+4,1000);plain[9]=70;
        monPut16(plain.data()+12,33);plain[20]=35;monPut32(plain.data()+40,0x3FFFFFFF);mon.setSecure(plain);
    }
    party.validate();return party;
}
BenchReferences references() {
    BenchReferences refs;refs.active={{0,1}};refs.queued={{2,3}};
    refs.participation={{0x3F,0x3F}};refs.knockedOff=0x3F;return refs;
}
bool contains(const std::vector<unsigned>& slots,unsigned slot){return std::find(slots.begin(),slots.end(),slot)!=slots.end();}
void allForcedSelections() {
    const auto transaction=combineParties(roster(0),roster(1));const auto current=transaction.combined;
    const auto refs=references();const auto journal=restoreParties(transaction,current);
    for(unsigned owner=0;owner<2;++owner)for(unsigned original=0;original<6;++original) {
        const auto plan=planForcedSwitch(transaction,current,owner,refs,original);
        if(original==0){check(!plan.accepted,"Forced switch accepted the outgoing active Pokemon");continue;}
        check(plan.accepted&&!plan.canceled,"Forced switch omitted a living own original candidate");
        check(plan.nativeSlot<current.count&&plan.transaction.owners[plan.nativeSlot]==int(owner),"Forced switch selected a partner slot");
        check(plan.party.mons[plan.nativeSlot]==journal[owner].mons[original],"Forced switch used a native index as an original index");
        check(plan.imported==(original>=3),"Forced switch did not distinguish resident and unloaded candidates");
        check(plan.references.queued[owner]==6&&plan.references.queued[1-owner]==refs.queued[1-owner],"Forced switch changed the partner's queued choice");
        check(restoreParties(plan.transaction,plan.party)==journal,"Forced switch changed an owned journal");
        for(unsigned slot=0;slot<current.count;++slot)
            if(transaction.owners[slot]!=int(owner)||slot==refs.active[owner]||slot==refs.queued[1-owner])
                check(plan.party.mons[slot]==current.mons[slot],"Forced switch overwrote an active or partner record");
        if(original==1)check(plan.nativeSlot==refs.queued[owner],"Old own queued choice was incorrectly excluded from the new native draw");
    }
    // Even an unusual partner queue referencing an own reserve remains pinned.
    for(unsigned owner=0;owner<2;++owner) {
        auto r=refs;r.queued[owner]=4+owner;r.queued[1-owner]=2+owner;
        const auto plan=planForcedSwitch(transaction,current,owner,r,5);
        check(plan.accepted&&plan.nativeSlot==4+owner,"Forced switch retired a partner-queued record");
        r.callerExcluded[0]=4+owner;
        check(!planForcedSwitch(transaction,current,owner,r,5).accepted,"Forced switch bypassed a remaining caller reservation");
    }
    rejects([&]{planForcedSwitch(transaction,current,2,refs,1);},"Invalid forced-switch owner accepted");
    rejects([&]{planForcedSwitch(transaction,current,0,refs,6);},"Out-of-range forced-switch draw accepted");
}
void exhaustedAndShortRosters() {
    for(unsigned owner=0;owner<2;++owner) {
        auto transaction=combineParties(roster(0),roster(1));auto current=transaction.combined;const auto refs=references();
        for(unsigned original=3;original<6;++original)transaction.originals[owner].mons[original].setHp(0);
        for(unsigned slot=0;slot<current.mons.size();++slot)if(transaction.owners[slot]==int(owner))current.mons[slot].setHp(0);
        const auto journal=restoreParties(transaction,current);
        for(unsigned original=0;original<6;++original)
            check(!planForcedSwitch(transaction,current,owner,refs,original).accepted,"Healthy partner was offered to an exhausted owner");
        check(journal[1-owner].mons[0].viable()&&!wouldAbandonLivingReserve(transaction,current,owner),"Genuine exhaustion was confused with missing working capacity");
        // Having only the outgoing active Pokemon alive also cannot satisfy a
        // forced switch, despite the partner retaining a full healthy roster.
        current.mons[refs.active[owner]].setHp(30);
        for(unsigned original=0;original<6;++original)
            check(!planForcedSwitch(transaction,current,owner,refs,original).accepted,"Forced switch reused the sole living active Pokemon");
        const auto shortParty=roster(owner,2);
        const auto shortTransaction=owner?combineParties(roster(0),shortParty):combineParties(shortParty,roster(1));
        for(unsigned original=2;original<6;++original)
            check(!planForcedSwitch(shortTransaction,shortTransaction.combined,owner,refs,original).accepted,"Forced switch accepted a missing original slot");
    }
}
void liveRetirementKeepsProgress() {
    for(unsigned owner=0;owner<2;++owner) {
        auto transaction=combineParties(roster(0),roster(1));auto current=transaction.combined;const auto refs=references();
        auto& retiring=current.mons[2+owner];retiring.setHp(11);retiring.setPp(0,9);retiring.setStatus(8);
        auto plain=retiring.secure();monPut32(plain.data()+4,1023);monPut16(plain.data()+2,13);retiring.setSecure(plain);
        const auto expected=restoreParties(transaction,current);
        const auto out=planForcedSwitch(transaction,current,owner,refs,5);
        check(out.accepted&&out.imported&&out.retiredNativeSlot==2+owner,"Forced switch did not release the old own queued reserve");
        check(restoreParties(out.transaction,out.party)==expected,"Live retirement discarded HP, PP, status, EXP or held item");
        const auto back=planForcedSwitch(out.transaction,out.party,owner,refs,1);
        check(back.accepted&&back.imported&&back.party.mons[back.nativeSlot]==retiring,"Returning retired Pokemon lost its latest native progress");
        check(restoreParties(back.transaction,back.party)==expected,"Round trip changed either owner's journal");
    }
}
void wishEvictionAndRecast() {
    const auto transaction=combineParties(roster(0),roster(1));const auto current=transaction.combined;
    for(unsigned owner=0;owner<2;++owner) {
        const unsigned battler=owner*2,source=2+owner;WishJournal wish;
        auto refs=references();refs.queued[owner]=4+owner;refs.wishCounters[battler]=2;refs.wishSlots[battler]=source;
        const auto unknown=wish.protect(refs);
        check(!(unknown.journaledWish&(1u<<battler))&&contains(unknown.exclusions(),source),"Unknown Wish source was unpinned");
        auto stale=refs;stale.journaledWish=uint8_t(1u<<battler);
        const auto withoutReceipt=wish.protect(stale);
        check(!(withoutReceipt.journaledWish&(1u<<battler))&&contains(withoutReceipt.exclusions(),source),"Unbacked incoming Wish receipt allowed source eviction");
        const auto blocked=planPartyMenu(transaction,current,owner,unknown);
        check(!planPartyMenuSelection(blocked,blocked.menu,5,true).accepted,"Unknown Wish reference did not protect the saturated own bench");
        wish.capture(battler,source,transaction,current);const auto originalName=wish.name(battler,source);
        check(bool(originalName)&&!wish.name(2-battler,source),"Wish receipt leaked to the other owner");
        const auto protectedRefs=wish.protect(refs);
        check((protectedRefs.journaledWish&(1u<<battler))&&!contains(protectedRefs.exclusions(),source),"Recorded Wish source stayed pinned");
        const auto session=planPartyMenu(transaction,current,owner,protectedRefs);
        const auto selected=planPartyMenuSelection(session,session.menu,5,true);
        check(selected.accepted&&selected.imported&&selected.nativeSlot==source,"Recorded Wish source could not be evicted safely");
        check(wish.name(battler,source)==originalName,"Wish took the replacement Pokemon's nickname");
        refs.wishCounters[battler]=0;
        check(!(wish.protect(refs).journaledWish&(1u<<battler))&&wish.name(battler,source)==originalName,"Counter zero discarded the pending Wish message");
        wish.capture(battler,source,selected.transaction,selected.party);
        const auto recastName=wish.name(battler,source);
        check(recastName&&recastName!=originalName&&(*recastName)[1]==0xA6,"Recast at a reused slot kept the previous caster's nickname");
        wish.capture(battler,4+owner,selected.transaction,selected.party);
        check(wish.name(battler,4+owner)&&(*wish.name(battler,4+owner))[1]==0xA3,"Recast at another slot kept the previous receipt");
        rejects([&]{wish.name(battler,source);},"Changed Wish source returned an unrelated old name");
        refs.wishCounters[battler]=1;
        check(contains(wish.protect(refs).exclusions(),source),"Mismatched Wish receipt unpinned an unknown source");
    }
}
void authoredWishSaturationRefill() {
    // Authored saturation of all three own working slots, not a claim that
    // this active/queued/Wish combination naturally occurs in a campaign.
    for(unsigned owner=0;owner<2;++owner) {
        const auto transaction=combineParties(roster(0),roster(1));auto current=transaction.combined;
        const unsigned battler=owner*2,source=4+owner,originalSource=unsigned(transaction.originalSlots[source]);
        auto refs=references();refs.wishCounters[battler]=1;refs.wishSlots[battler]=source;
        WishJournal wish;wish.capture(battler,source,transaction,current);const auto originalName=wish.name(battler,source);
        for(unsigned slot=0;slot<current.mons.size();++slot)
            if(transaction.owners[slot]==int(owner))current.mons[slot].setHp(0);
        const auto journal=restoreParties(transaction,current);
        check(journal[owner].mons[3].viable()&&wouldAbandonLivingReserve(transaction,current,owner),"Authored saturation lacks healthy unloaded reserves");
        check(!planBenchRefill(transaction,current,owner,refs),"Unrecorded Wish source failed to pin the saturated fainted bench");
        const auto protectedRefs=wish.protect(refs);
        const auto plan=planBenchRefill(transaction,current,owner,protectedRefs);
        check(bool(plan),"Recorded Wish prevented refill of the authored saturated bench");
        check(plan->retiredNativeSlot==source&&plan->importedOriginalSlot==3&&
              plan->party.mons[source]==journal[owner].mons[3],"Refill did not reuse only the recorded Wish slot for an own unloaded reserve");
        for(unsigned slot=0;slot<current.mons.size();++slot)if(slot!=source)
            check(plan->party.mons[slot]==current.mons[slot]&&plan->transaction.owners[slot]==transaction.owners[slot]&&
                  plan->transaction.originalSlots[slot]==transaction.originalSlots[slot],"Wish refill changed an active, queued or foreign record");
        check(plan->references.active==refs.active&&plan->references.queued==refs.queued&&
              plan->references.wishCounters==refs.wishCounters&&plan->references.wishSlots==refs.wishSlots,
              "Wish refill changed native active, queued or Wish references");
        check(restoreParties(plan->transaction,plan->party)==journal&&
              plan->transaction.originals[owner].mons[originalSource]==current.mons[source]&&
              plan->transaction.originals[owner].mons[originalSource].hp()==0,"Wish refill revived or lost its fainted original source");
        check(originalName&&wish.name(battler,source)==originalName,"Wish refill replaced the original caster's nickname");
        refs.wishCounters[battler]=0;
        check(!(wish.protect(refs).journaledWish&(1u<<battler))&&wish.name(battler,source)==originalName,
              "Wish refill discarded the original name before its counter-zero announcement");
    }
}
void exactWishNames() {
    // Non-Japanese GetMonData copies all ten bytes; StringGet_Nickname keeps
    // the first terminator and adds byte ten for a full-length nickname.
    const std::array<std::array<uint8_t,10>,3> raw{{
        {{0xBB,0xBC,0xBD,0xBE,0xBF,0xC0,0xC1,0xC2,0xC3,0xC4}},
        {{0xBB,0xFF,0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7}},
        {{0xCA,0xCD,0xC8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}} // authored "PSN", not a status substitution
    }};
    for(unsigned owner=0;owner<2;++owner)for(const auto& encoded:raw) {
        auto a=roster(0),b=roster(1);auto& mon=(owner?b:a).mons[1];
        std::copy(encoded.begin(),encoded.end(),mon.bytes.begin()+8);
        const auto transaction=combineParties(a,b);WishJournal wish;wish.capture(owner*2,2+owner,transaction,transaction.combined);
        WishJournal::Name expected;std::copy(encoded.begin(),encoded.end(),expected.begin());expected[10]=0xFF;
        check(wish.name(owner*2,2+owner)==std::optional<WishJournal::Name>(expected),"Wish altered a full, short or status-looking nickname");
    }
    for(unsigned owner=0;owner<2;++owner)for(unsigned length:{0u,2u,6u,10u}) {
        auto a=roster(0),b=roster(1);auto& mon=(owner?b:a).mons[1];mon.bytes[18]=1;
        std::fill(mon.bytes.begin()+8,mon.bytes.begin()+18,0xFF);
        for(unsigned i=0;i<length;++i)mon.bytes[8+i]=uint8_t(0x10+i);
        WishJournal::Name expected;expected.fill(0xFF);expected[0]=0xFC;expected[1]=0x15;
        const auto copied=std::min(6u,length);for(unsigned i=0;i<copied;++i)expected[2+i]=uint8_t(0x10+i);
        expected[2+copied]=0xFC;expected[3+copied]=0x16;
        const auto transaction=combineParties(a,b);WishJournal wish;wish.capture(owner*2,2+owner,transaction,transaction.combined);
        check(wish.name(owner*2,2+owner)==std::optional<WishJournal::Name>(expected),"Wish changed Japanese truncation, language controls or terminator");
    }
}
void malformedWishReferences() {
    const auto transaction=combineParties(roster(0),roster(1));const auto current=transaction.combined;
    WishJournal wish;wish.capture(0,2,transaction,current);const auto before=wish.name(0,2);
    for(unsigned battler:{1u,3u,4u,255u}) {
        rejects([&]{wish.capture(battler,2,transaction,current);},"Invalid Wish caster accepted");
        rejects([&]{wish.name(battler,2);},"Invalid Wish message owner accepted");
    }
    rejects([&]{wish.capture(0,6,transaction,current);},"Absent Wish caster accepted");
    rejects([&]{wish.capture(0,3,transaction,current);},"Partner Pokemon accepted as own Wish caster");
    auto corrupt=current;corrupt.mons[2].bytes[33]^=1;
    rejects([&]{wish.capture(0,2,transaction,corrupt);},"Corrupt Wish caster accepted");
    auto eggTransaction=transaction;auto egg=eggTransaction.originals[0].mons[1];
    auto plain=egg.secure();monPut32(plain.data()+40,monU32(plain.data()+40)|0x40000000u);egg.setSecure(plain);egg.bytes[19]|=4;
    eggTransaction.originals[0].mons[1]=egg;eggTransaction.combined.mons[2]=egg;
    rejects([&]{wish.capture(0,2,eggTransaction,eggTransaction.combined);},"Consistently encoded Egg accepted as a Wish caster");
    check(wish.name(0,2)==before,"Rejected Wish capture destroyed the existing receipt");
    for(unsigned mask:{2u,8u,255u}) {
        auto refs=references();refs.journaledWish=uint8_t(mask);
        rejects([&]{wish.protect(refs);},"Malformed Wish journal mask accepted");
        rejects([&]{planForcedSwitch(transaction,current,0,refs,5);},"Forced switch accepted a malformed Wish journal mask");
    }
    for(unsigned slot:{6u,7u,255u}) {
        auto refs=references();refs.wishCounters[0]=1;refs.wishSlots[0]=slot;refs.journaledWish=1;
        rejects([&]{wish.protect(refs);},"Journal bit hid an invalid native Wish source");
    }
}
}
int main(){try{allForcedSelections();exhaustedAndShortRosters();liveRetirementKeepsProgress();wishEvictionAndRecast();authoredWishSaturationRefill();exactWishNames();malformedWishReferences();std::cout<<"FireRed reserve tests: "<<checks<<" checks passed\n";return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
