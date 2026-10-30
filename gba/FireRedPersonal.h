// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedDex.h"

namespace coop::firered {
inline constexpr uint32_t CaptureCountOffset = 0x122C; // encrypted game stat11
inline constexpr uint32_t CaptureBoxVariableOffset = 0x106E; // VAR0x4037
inline constexpr unsigned ShownBoxFullFlag = 0x843;
inline constexpr uint32_t MaximumGameStat = 0xFFFFFF;
struct PersonalCaptureState {
    std::array<uint8_t,52> owned{};
    std::array<std::array<uint8_t,52>,3> seen{};
    uint32_t unown = 0, spinda = 0, captures = 0;
    uint16_t pcBox = 0;
    bool shownBoxFull = false;
    void validate() const {
        requirePokemon(pcBox < BoxCount && captures <= MaximumGameStat, "Invalid FireRed personal capture metadata");
    }
    bool operator==(const PersonalCaptureState& r) const {
        return owned==r.owned && seen==r.seen && unown==r.unown && spinda==r.spinda &&
               captures==r.captures && pcBox==r.pcBox && shownBoxFull==r.shownBoxFull;
    }
};
inline PersonalCaptureState readPersonalCapture(mCore* c) {
    const auto m=readSaveMemory(c);
    PersonalCaptureState r;
    for(unsigned i=0;i<52;++i) {
        r.owned[i]=uint8_t(c->busRead8(c,m.block2+DexOwnedOffset+i));
        r.seen[0][i]=uint8_t(c->busRead8(c,m.block2+DexSeenOffset+i));
        r.seen[1][i]=uint8_t(c->busRead8(c,m.block1+DexSeen1Offset+i));
        r.seen[2][i]=uint8_t(c->busRead8(c,m.block1+DexSeen2Offset+i));
    }
    r.unown=inventoryRead32(c,m.block2+DexUnownPersonalityOffset);
    r.spinda=inventoryRead32(c,m.block2+DexSpindaPersonalityOffset);
    r.captures=inventoryRead32(c,m.block1+CaptureCountOffset)^m.key;
    r.pcBox=inventoryRead16(c,m.block1+CaptureBoxVariableOffset);
    r.shownBoxFull=(c->busRead8(c,m.block1+0xEE0+ShownBoxFullFlag/8)&(1u<<(ShownBoxFullFlag%8)))!=0;
    checkSaveMemory(c,m);r.validate();return r;
}
// Writes only the explicitly reviewed fields. In particular the flag shares a
// byte with unrelated personal/world flags, so preserve all neighboring bits.
inline void writePersonalCapture(mCore* c,const PersonalCaptureState& r) {
    r.validate();requirePokemon(c && c->busWrite8,"Cannot write FireRed personal capture metadata");const auto m=readSaveMemory(c);
    const auto flagAddress=m.block1+0xEE0+ShownBoxFullFlag/8;
    const auto flag=uint8_t((c->busRead8(c,flagAddress)&~(1u<<(ShownBoxFullFlag%8)))|
                           (unsigned(r.shownBoxFull)<<(ShownBoxFullFlag%8)));
    checkSaveMemory(c,m);
    for(unsigned i=0;i<52;++i) {
        c->busWrite8(c,m.block2+DexOwnedOffset+i,r.owned[i]);
        c->busWrite8(c,m.block2+DexSeenOffset+i,r.seen[0][i]);
        c->busWrite8(c,m.block1+DexSeen1Offset+i,r.seen[1][i]);
        c->busWrite8(c,m.block1+DexSeen2Offset+i,r.seen[2][i]);
    }
    const std::array<std::pair<uint32_t,uint32_t>,3> words{{
        {m.block2+DexUnownPersonalityOffset,r.unown},
        {m.block2+DexSpindaPersonalityOffset,r.spinda},
        {m.block1+CaptureCountOffset,r.captures^m.key}}};
    for(const auto& word:words)for(unsigned i=0;i<4;++i)c->busWrite8(c,word.first+i,uint8_t(word.second>>(i*8)));
    c->busWrite8(c,m.block1+CaptureBoxVariableOffset,uint8_t(r.pcBox));
    c->busWrite8(c,m.block1+CaptureBoxVariableOffset+1,uint8_t(r.pcBox>>8));
    c->busWrite8(c,flagAddress,flag);
}
inline PersonalCaptureState planPersonalCapture(const PersonalCaptureState& before,
                                               std::optional<unsigned> actualPcBox,
                                               unsigned currentBox) {
    before.validate();requirePokemon(currentBox<BoxCount,"Invalid FireRed actual current box");
    auto r=before;r.captures=std::min(MaximumGameStat,r.captures+1);
    if(actualPcBox) {
        requirePokemon(*actualPcBox<BoxCount,"Invalid FireRed capture destination box");
        // Match native SendMonToPC and ShouldShowBoxWasFullMessage, using the
        // real owner's destination rather than the leader's scratch box.
        if(r.pcBox!=*actualPcBox)r.shownBoxFull=false;
        r.pcBox=uint16_t(*actualPcBox);
        if(currentBox!=*actualPcBox)r.shownBoxFull=true;
    }
    return r;
}
inline PartyMon preservePartyCapturePP(const PartyMon& nativeResult,const PartyMon& beforeGift) {
    nativeResult.validate();beforeGift.validate();
    requirePokemon(nativeResult.identity().personality==beforeGift.identity().personality &&
                   nativeResult.species()==beforeGift.species(),"FireRed captured PP record has a different identity");
    auto r=nativeResult;auto secure=r.secure();const auto original=beforeGift.secure();
    for(unsigned slot=0;slot<4;++slot) {
        requirePokemon(monU16(secure.data()+12+slot*2)==monU16(original.data()+12+slot*2),
                       "FireRed captured moves changed during native gift");
        secure[20+slot]=original[20+slot];
    }
    r.setSecure(secure);r.validate();return r;
}
} // namespace coop::firered
