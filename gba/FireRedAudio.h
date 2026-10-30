// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace coop::firered {
// Independent implementation inspired by SplitEmu's selectable audio. Each
// block is interleaved stereo PCM; unavailable frames contribute silence.
enum class AudioMode { Player1, Player2, Both, Mute };
inline AudioMode parseAudioMode(std::string_view value) {
    if(value=="player2") return AudioMode::Player2;
    if(value=="both") return AudioMode::Both;
    if(value=="mute") return AudioMode::Mute;
    return AudioMode::Player1; // Safe, backwards-compatible default.
}
inline const char* audioModeName(AudioMode mode) {
    switch(mode) {
        case AudioMode::Player2:return "player2";
        case AudioMode::Both:return "both";
        case AudioMode::Mute:return "mute";
        default:return "player1";
    }
}
struct AudioBlock {
    static constexpr unsigned MaxFrames=2048;
    std::array<int16_t,MaxFrames*2> samples{};
    unsigned frames=0;
};
inline AudioBlock mixAudio(const AudioBlock& first,const AudioBlock& second,
                           AudioMode mode,bool sharedBattle,unsigned leader) {
    AudioBlock out;
    if(mode==AudioMode::Mute) return out;
    // Both views show the same battle. Play its native audio exactly once,
    // even when the partner normally has the selected overworld audio.
    if(sharedBattle) {
        const auto& source=leader==1?second:first;
        out.frames=std::min(source.frames,AudioBlock::MaxFrames);
        std::copy_n(source.samples.begin(),out.frames*2,out.samples.begin());
        return out;
    }
    if(mode==AudioMode::Player1 || mode==AudioMode::Player2) {
        const auto& source=mode==AudioMode::Player1?first:second;
        out.frames=std::min(source.frames,AudioBlock::MaxFrames);
        std::copy_n(source.samples.begin(),out.frames*2,out.samples.begin());
        return out;
    }
    const auto a=std::min(first.frames,AudioBlock::MaxFrames);
    const auto b=std::min(second.frames,AudioBlock::MaxFrames);
    out.frames=std::max(a,b);
    for(unsigned frame=0;frame<out.frames;++frame) for(unsigned channel=0;channel<2;++channel) {
        const unsigned index=frame*2+channel;
        const int left=frame<a?first.samples[index]:0;
        const int right=frame<b?second.samples[index]:0;
        out.samples[index]=int16_t((left+right)/2);
    }
    return out;
}
}
