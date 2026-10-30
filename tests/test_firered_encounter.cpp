// SPDX-License-Identifier: GPL-3.0-or-later
#include <initializer_list>
#include "../gba/FireRedEncounter.h"
#include <stdexcept>
#include <iostream>
using namespace coop::firered;
int main() {
    unsigned checks = 0;
    auto check = [&](bool ok) { ++checks; if (!ok) throw std::runtime_error("FireRed encounter policy failed"); };
    for (unsigned flags = 0; flags < (1u << 20); ++flags) {
        const auto base = flags & ~5u;
        const bool expected = !(flags & ~0xDu) || base == 0xA000 || base == 0x20000 || base == 0x42000;
        check(sharedEncounterFlags(flags) == expected);
    }
    for (unsigned first : {326u,329u,332u,426u,429u,432u,435u,438u,739u}) {
        for (unsigned a=first;a<first+3;++a) for(unsigned b=first;b<first+3;++b) check(sameStoryTrainer(a,b));
        check(!sameStoryTrainer(first,first+3));
    }
    WaitingEncounter a{8,0x08000001,438,1,1,79,0}, b=a;
    b.trainer=440; check(matchingWaitingEncounters(a,b));
    b.trainer=739; check(!matchingWaitingEncounters(a,b));
    b=a; b.mapNum++; check(!matchingWaitingEncounters(a,b));
    b=a; b.callback+=2; check(!matchingWaitingEncounters(a,b));
    a.flags=b.flags=0xA000;a.species=b.species=105;a.scriptContinuation=0x08123456;b=a;check(matchingWaitingEncounters(a,b));
    b.species=19;check(!matchingWaitingEncounters(a,b));
    b=a;b.scriptDepth=a.scriptDepth=1;a.scriptReturns[0]=0x08124000;b.scriptReturns[0]=0x08125000;
    check(!matchingWaitingEncounters(a,b)); // Two Electrode object wrappers.
    a.flags=b.flags=0; b=a;b.species=16;check(matchingWaitingEncounters(a,b));
    std::cout << checks << " FireRed encounter checks passed\n";
}
