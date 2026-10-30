// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedCampaignRuntime.h"
#include <memory>
#include <cstring>

namespace coop::firered {
inline int leagueTrainerRoom(unsigned trainer) {
    if(trainer>=410 && trainer<=413) return int(trainer-410);
    if(trainer>=735 && trainer<=738) return int(trainer-735);
    return -1;
}
// Called ONLY for the idle partner of a just-committed shared WIN. Unlike the
// campaign catalog these flags describe a resettable expedition. Native room
// entry and WhiteOut still reset them normally; no flag is set on a loss.
inline bool startNativeLeagueVictory(mCore* core,unsigned trainer) {
    const auto room=leagueTrainerRoom(trainer);
    if(room<0 || !campaignFieldQuiescent(core)) return false;
    const auto before=readProgress(core);
    if(before.mapGroup!=1 || before.mapNum!=unsigned(75+room) || before.var(0x4068)!=unsigned(room+1)) return false;
    for(unsigned badge=0x820;badge<=0x827;++badge) if(!before.flag(badge)) return false;
    auto after=before; after.flag(unsigned(0x4B8+room),true); after.flag(0x500+trainer,true);
    const auto plan=planCampaignObjectRefresh(core,before,after);
    requirePokemon(plan.ready,"Cannot refresh the partner's native Elite Four door; previous saves preserved");
    std::vector<uint8_t> state(core->stateSize(core));
    requirePokemon(!state.empty() && core->saveState(core,state.data()),"Cannot snapshot the partner's League victory");
    void* raw=nullptr;const auto count=core->savedataClone(core,&raw);
    std::unique_ptr<void,decltype(&free)> saved(raw,free);
    requirePokemon(raw && count==131072,"Cannot snapshot the partner's League battery");
    try {
        writeProgressDelta(core,before,after);
        requirePokemon(startCampaignObjectRefresh(core,plan),"Cannot start the partner's native League door refresh");
    } catch(...) {
        bool restored=core->loadState(core,state.data());
        restored=core->savedataRestore(core,raw,count,true) && restored;
        void* check=nullptr;const auto size=core->savedataClone(core,&check);
        std::unique_ptr<void,decltype(&free)> checkOwned(check,free);
        restored=check && size==count && std::memcmp(raw,check,count)==0 && restored;
        requirePokemon(restored,"Cannot roll back the partner's League victory; previous disk saves preserved");
        throw;
    }
    return true;
}
} // namespace coop::firered
