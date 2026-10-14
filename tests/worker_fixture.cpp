// SPDX-License-Identifier: GPL-3.0-or-later
// TEST ONLY: process/IPC fixture, no emulation or multiplayer claims.
#include "CoopIPC.h"
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>
static volatile sig_atomic_t running=1;
void stop(int) {running=0;}
int main(int argc,char**argv) {
    if(argc==2&&!strcmp(argv[1],"--coop-capabilities")){puts("PMCOOP_ABI=1;TEST_FIXTURE=1");return 0;}
    signal(SIGTERM,stop);signal(SIGINT,stop);
    coop::Mapping mapping;mapping.open(getenv("PMCOOP_IPC"),false);
    uint32_t top[coop::Pixels]{},bottom[coop::Pixels]{};
    int count=0;
    while(running) {
        for(int i=0;i<coop::Pixels;i++){top[i]=0xFF0000FF;bottom[i]=0xFF00FF00;}
        mapping.publish(top,bottom);mapping.data->peers.store(1);
        if(getenv("COOP_FIXTURE_CRASH_PLAYER") && !strcmp(getenv("PMCOOP_ROLE"),"join") && ++count>15)return 7;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    puts("TEST_ONLY: graceful exit");return 0;
}
