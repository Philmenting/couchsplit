// SPDX-License-Identifier: GPL-3.0-or-later
#include "../gba/FireRedAudio.h"
#include <cassert>
#include <iostream>
using namespace coop::firered;
int main() {
    AudioBlock first,second;
    first.frames=2;first.samples[0]=32767;first.samples[1]=-32768;first.samples[2]=200;first.samples[3]=-100;
    second.frames=1;second.samples[0]=32767;second.samples[1]=-32768;
    auto mixed=mixAudio(first,second,AudioMode::Both,false,0);
    assert(mixed.frames==2 && mixed.samples[0]==32767 && mixed.samples[1]==-32768);
    assert(mixed.samples[2]==100 && mixed.samples[3]==-50);
    auto one=mixAudio(first,second,AudioMode::Player1,false,0);
    auto two=mixAudio(first,second,AudioMode::Player2,false,0);
    assert(one.frames==2 && one.samples[2]==200 && one.samples[3]==-100);
    assert(two.frames==1 && two.samples[0]==32767 && two.samples[1]==-32768);
    assert(mixAudio(first,second,AudioMode::Mute,false,0).frames==0);
    assert(mixAudio(first,second,AudioMode::Mute,true,1).frames==0);
    auto battle=mixAudio(first,second,AudioMode::Both,true,1);
    assert(battle.frames==1 && battle.samples[0]==32767 && battle.samples[1]==-32768);
    assert(mixAudio(first,second,AudioMode::Player1,true,1).frames==1);
    assert(mixAudio(first,second,AudioMode::Player2,true,0).frames==2);
    assert(parseAudioMode("player1")==AudioMode::Player1 && parseAudioMode("player2")==AudioMode::Player2);
    assert(parseAudioMode("both")==AudioMode::Both && parseAudioMode("mute")==AudioMode::Mute);
    assert(parseAudioMode("invalid")==AudioMode::Player1);
    first.frames=AudioBlock::MaxFrames+100;
    assert(mixAudio(first,second,AudioMode::Both,false,0).frames==AudioBlock::MaxFrames);
    std::cout<<"PASS: isolated and mixed stereo, missing frames, mute, safe sample range and shared-battle audio\n";
}
