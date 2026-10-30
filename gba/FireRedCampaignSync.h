// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedCampaign.h"
#include "FireRedCampaignRuntime.h"
#include "FireRedCampaignCompletion.h"
#include <memory>

namespace coop::firered {
inline bool nativeNationalDexEnabled(mCore* core, const Progress& progress) {
    const auto memory = readSaveMemory(core);
    return progress.flag(0x840) && progress.var(0x404E) == 0x6258 &&
        core->busRead8(core,memory.block2+0x1B) == 0xB9;
}

// Native receipts are the durable event ledger. No frame-local queue is needed
// to recover missed key rewards when a player saved while the other had a menu
// open. Never accept one player's arbitrary flags as the other's entire save.
class CampaignProgress {
    std::array<mCore*,2> cores_;
    std::array<std::string,2> lastReason_{};
    static std::vector<uint8_t> machine(mCore* core) {
        std::vector<uint8_t> bytes(core->stateSize(core));
        requirePokemon(!bytes.empty() && core->saveState(core,bytes.data()),"Cannot snapshot FireRed campaign transaction");
        return bytes;
    }
    static std::vector<uint8_t> battery(mCore* core) {
        void* raw=nullptr; const auto count=core->savedataClone(core,&raw);
        std::unique_ptr<void,decltype(&free)> owned(raw,free);
        requirePokemon(raw && count==131072,"Cannot snapshot FireRed campaign battery");
        return {static_cast<uint8_t*>(raw),static_cast<uint8_t*>(raw)+count};
    }
public:
    // Callbacks report committed plans / changed defer reasons only. They must
    // not advance cores or save a synthetic intermediate native state.
    std::function<void(unsigned,const CampaignApplication&)> event;
    std::function<void(unsigned,const std::string&)> deferred;
    explicit CampaignProgress(std::array<mCore*,2> cores) : cores_(cores) {
        requirePokemon(cores[0] && cores[1] && cores[0]!=cores[1],"Campaign sync requires two separate cores");
    }
    void sample() {
        if (!campaignFieldQuiescent(cores_[0]) || !campaignFieldQuiescent(cores_[1])) return;
        const std::array<Progress,2> before{{readProgress(cores_[0]),readProgress(cores_[1])}};
        std::array<CampaignApplication,2> changes{{reconcileCampaign(before[1],before[0]),reconcileCampaign(before[0],before[1])}};
        std::array<CampaignObjectRefreshPlan,2> plans;
        std::array<bool,2> apply{};
        for(unsigned owner=0;owner<2;++owner) {
            auto& change=changes[owner];
            // Room victories belong to the CURRENT League expedition. Native
            // entry/whiteout resets remain authoritative: never union these
            // flags in the global campaign catalog or copy the other's stage.
            const auto& own=before[owner]; const auto& source=before[1-owner];
            if(campaignReady(own) && campaignReady(source) && own.mapGroup==1 &&
               own.mapNum>=75 && own.mapNum<=78 && source.mapGroup==1 &&
               source.mapNum>=own.mapNum && source.mapNum<=79 &&
               own.var(0x4068)==own.mapNum-74 && source.var(0x4068)>=own.var(0x4068) &&
               source.var(0x4068)<=4) {
                const unsigned room=own.mapNum-75, flag=0x4B8+room;
                if(source.flag(flag) && !own.flag(flag)) {
                    change.progress.flag(flag,true);change.changedFlags.push_back(flag);change.changed=true;
                    const auto trainer=(source.flag(0x844)?735u:410u)+room;
                    change.progress.flag(0x500+trainer,true);change.changedFlags.push_back(0x500+trainer);
                }
            }
            // National Dex is a native three-part receipt, not a flag OR. The
            // other player's completed upgrade is the shared quest receipt;
            // the recipient must first have their OWN native Hall-of-Fame save.
            change.enableNationalDex = campaignReady(before[owner]) && before[owner].flag(0x82C) &&
                nativeNationalDexEnabled(cores_[1-owner],before[1-owner]) && !nativeNationalDexEnabled(cores_[owner],before[owner]);
            if(nativeNationalDexEnabled(cores_[owner],before[owner]) &&
               nativeNationalDexEnabled(cores_[1-owner],before[1-owner]) &&
               before[1-owner].var(VarLabScene)>=8 && before[owner].var(VarLabScene)==7) {
                change.progress.var(VarLabScene,8); change.changed=true; change.changedVars.push_back(VarLabScene);
            }
            if (!change.changed && !change.enableNationalDex && !change.completeNetwork) continue;
            auto reason=change.conflict ? std::string("Campaign contains an unreviewed scene stage") : std::string{};
            const bool requestedNationalDex=change.enableNationalDex;
            const bool requestedNetwork=change.completeNetwork && nativeNationalDexEnabled(cores_[owner],before[owner]);
            change.completeNetwork=requestedNetwork;
            // Preview native metadata ONLY to plan this already-loaded map.
            // Actual save/RAM changes still belong to the original specials.
            // Their instructions run before object/tile refresh, so flags set
            // by Sapphire installation cannot leave a stale machine or guard.
            auto prepare = [&](bool nationalDex,bool network) {
                auto preview=change.progress;
                std::vector<uint8_t> commands;
                if(nationalDex) {
                    commands.insert(commands.end(),{0x25,0x6F,0x01}); // Native SPECIAL367.
                    preview.flag(0x840,true);preview.var(0x404E,0x6258);
                }
                if(network) {
                    const auto original=nativeNetworkCompletionCommands(cores_[owner],change.progress);
                    commands.insert(commands.end(),original.begin(),original.end());
                    preview.flag(0x844,true);preview.flag(0x05C,true);preview.flag(0x08C,true);preview.flag(0x2DC,true);
                    preview.var(0x4076,6);preview.gcnLinkFlags|=0x800Eu;
                    const auto sapphire=preview.inventory.quantity(374);
                    if(sapphire)preview.inventory.setQuantity(374,sapphire-1);
                }
                auto plan=planCampaignObjectRefresh(cores_[owner],before[owner],preview,nationalDex);
                if(!plan.ready) {reason=plan.reason;return false;}
                // writeProgressDelta commits only the pure planner result.
                // Native completion commands subsequently create the preview;
                // start must validate that original, pre-special state.
                plan.expected=change.progress;
                if(!commands.empty()) {
                    const auto table=inventoryRead32(cores_[owner],GlobalScriptContextAddress+0x5C);
                    const auto end=inventoryRead32(cores_[owner],GlobalScriptContextAddress+0x60);
                    requirePokemon(table>=0x08000000 && end>table && end<=0x09000000 && end-table>=0xA3*4,
                                   "Campaign completion requires the initialized native command table");
                    auto& code=plan.code;
                    if((code.empty()?3:code.size())+commands.size()>plan.capacity()) {
                        reason="Native completion waits for a separate map refresh";return false;
                    }
                    if(code.empty())code={0x69,0x6B,0x02};
                    requirePokemon(code.size()>=3 && code.front()==0x69 && code[code.size()-2]==0x6B && code.back()==2,
                                   "Campaign refresh lacks native lock/release boundaries");
                    code.insert(code.begin()+1,commands.begin(),commands.end());
                }
                plans[owner]=std::move(plan);return true;
            };
            if(reason.empty() && !prepare(requestedNationalDex,requestedNetwork) && change.changed &&
               (requestedNationalDex || requestedNetwork)) {
                // A large shared receipt/map update may consume the workspace.
                // Commit that independent pure change first; the native receipt
                // remains unset and reconstructs the completion next idle tick.
                change.enableNationalDex=false;change.completeNetwork=false;
                reason.clear();prepare(false,false);
            }
            if(reason.empty() && !change.changed && !change.enableNationalDex && !change.completeNetwork)
                reason="Network completion waits for the own native National Dex";
            if(!reason.empty()) {
                if(lastReason_[owner]!=reason && deferred) deferred(owner,reason);
                lastReason_[owner]=std::move(reason); continue;
            }
            lastReason_[owner].clear();apply[owner]=true;
        }
        if(!apply[0] && !apply[1]) return;
        // Snapshot BOTH machines and Flash buffers before the first mutation.
        // Synchronous commit/start failures restore both, including VM stacks.
        const std::array<std::vector<uint8_t>,2> states{{machine(cores_[0]),machine(cores_[1])}};
        const std::array<std::vector<uint8_t>,2> saves{{battery(cores_[0]),battery(cores_[1])}};
        try {
            for(unsigned owner=0;owner<2;++owner) if(apply[owner])
                writeProgressDelta(cores_[owner],before[owner],changes[owner].progress);
            for(unsigned owner=0;owner<2;++owner) if(apply[owner])
                requirePokemon(startCampaignObjectRefresh(cores_[owner],plans[owner]),"Cannot start native FireRed campaign map refresh");
        } catch (...) {
            bool restored=true;
            for(unsigned owner=0;owner<2;++owner) {
                auto* c=cores_[owner];
                restored=c->loadState(c,states[owner].data()) && restored;
                restored=c->savedataRestore(c,saves[owner].data(),saves[owner].size(),true) && restored;
                restored=(battery(c)==saves[owner]) && restored;
            }
            requirePokemon(restored,"Cannot roll back FireRed campaign; previous disk saves preserved");
            throw;
        }
        for(unsigned owner=0;owner<2;++owner) if(apply[owner] && event) event(owner,changes[owner]);
    }
};
} // namespace coop::firered
