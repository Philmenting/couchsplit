// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "FireRedWorld.h"
#include <optional>

namespace coop::firered {
// Presentation only. Read the live native connection table; never infer a warp
// from proximity, move an avatar, or modify collision/event/save memory.
struct WorldConnection {
    unsigned direction = 0, mapGroup = 0, mapNum = 0;
    int offset = 0;
};
struct WorldTopology {
    bool valid = false;
    unsigned mapGroup = 0, mapNum = 0, width = 0, height = 0, count = 0;
    std::array<WorldConnection,16> connections{};
};
inline WorldTopology readWorldTopology(mCore* core, const WorldActor& actor) {
    WorldTopology result;
    if (!core || !core->busRead8 || !core->busRead32 || !world_detail::bounded(actor) ||
        core->busRead32(core,0x03003044) != 0x080565D5 || core->busRead8(core,0x03000F9C)) return result;
    const auto save = core->busRead32(core,0x03004F58);
    if (save < 0x02000000 || save > 0x0203FFF8 || (save & 3) ||
        core->busRead8(core,save+4) != actor.mapGroup || core->busRead8(core,save+5) != actor.mapNum) return result;
    const auto layout = core->busRead32(core,0x02036DFC);
    if ((layout & 3) || !world_detail::romRange(layout,28)) return result;
    result.width = core->busRead32(core,layout);
    result.height = core->busRead32(core,layout+4);
    if (!result.width || !result.height || result.width > 2048 || result.height > 2048 ||
        result.width * result.height > 1048576) return {};
    const auto table = core->busRead32(core,0x02036E08);
    if (!table || (table & 3) || !world_detail::romRange(table,8)) return {};
    result.count = core->busRead32(core,table);
    const auto entries = core->busRead32(core,table+4);
    if (result.count > result.connections.size() || (entries & 3) ||
        !world_detail::romRange(entries,result.count*12)) return {};
    for (unsigned i=0;i<result.count;++i) {
        auto& edge=result.connections[i]; const auto p=entries+i*12;
        edge.direction=core->busRead8(core,p);
        const uint32_t raw=core->busRead32(core,p+4);
        // Avoid implementation-defined unsigned-to-signed conversion.
        const int64_t offset=raw & 0x80000000u ? int64_t(raw)-0x100000000LL : raw;
        if (edge.direction < 1 || edge.direction > 6 || offset < -2048 || offset > 2048) return {};
        edge.offset=int(offset); edge.mapGroup=core->busRead8(core,p+8); edge.mapNum=core->busRead8(core,p+9);
        if (edge.mapGroup >= 64 || edge.mapNum >= 128) return {};
    }
    result.mapGroup=actor.mapGroup; result.mapNum=actor.mapNum; result.valid=true;
    return result;
}
inline std::optional<WorldActor> projectConnectedPartner(const WorldActor& local, const WorldActor& remote,
                                                         const WorldTopology& a, const WorldTopology& b) {
    if (!world_detail::bounded(local) || !world_detail::bounded(remote)) return std::nullopt;
    if (local.mapGroup==remote.mapGroup && local.mapNum==remote.mapNum) return remote;
    const auto valid=[](const WorldTopology& t,const WorldActor& actor) {
        return t.valid && t.count<=t.connections.size() && t.width && t.height && t.width<=2048 && t.height<=2048 &&
            t.mapGroup==actor.mapGroup && t.mapNum==actor.mapNum && actor.tileX>=0 && actor.tileY>=0 &&
            actor.tileX<int(t.width) && actor.tileY<int(t.height);
    };
    if (!valid(a,local) || !valid(b,remote)) return std::nullopt;
    const WorldConnection* edge=nullptr;
    for(unsigned i=0;i<a.count;++i) {
        const auto& candidate=a.connections[i];
        if(candidate.mapGroup!=b.mapGroup || candidate.mapNum!=b.mapNum) continue;
        if(edge || candidate.direction<1 || candidate.direction>4 || candidate.offset < -2048 || candidate.offset>2048) return std::nullopt;
        edge=&candidate;
    }
    if(!edge) return std::nullopt;
    constexpr unsigned opposite[]={0,2,1,4,3}; unsigned reverseCount=0;
    for(unsigned i=0;i<b.count;++i) {
        const auto& reverse=b.connections[i];
        if(reverse.mapGroup!=a.mapGroup || reverse.mapNum!=a.mapNum) continue;
        if(reverse.direction!=opposite[edge->direction] || reverse.offset!=-edge->offset) return std::nullopt;
        ++reverseCount;
    }
    if(reverseCount!=1) return std::nullopt;
    auto result=remote;
    switch(edge->direction) {
    case 1: result.tileX+=edge->offset; result.tileY+=int(a.height); break;
    case 2: result.tileX+=edge->offset; result.tileY-=int(b.height); break;
    case 3: result.tileX-=int(b.width); result.tileY+=edge->offset; break;
    case 4: result.tileX+=int(a.width); result.tileY+=edge->offset; break;
    }
    // Native connected-map border is seven tiles. Do not present a partner
    // beyond that loaded border or through a nonoverlapping connection segment.
    if(result.tileX < -7 || result.tileY < -7 || result.tileX>=int(a.width)+7 || result.tileY>=int(a.height)+7) return std::nullopt;
    result.mapGroup=local.mapGroup; result.mapNum=local.mapNum;
    return result;
}
} // namespace coop::firered
