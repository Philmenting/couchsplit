// SPDX-License-Identifier: GPL-3.0-or-later
// TEST ONLY: process/IPC fixture, no emulation or multiplayer claims.
#include "CoopIPC.h"
#include "../gba/FireRedRecovery.h"
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>
#include <QCoreApplication>
static volatile sig_atomic_t running=1;
void stop(int) {running=0;}
int main(int argc,char**argv) {
    QCoreApplication app(argc,argv);
    if(argc==3&&!strcmp(argv[1],"--ipc-write")) {coop::Mapping m;m.open(app.arguments()[2].toUtf8().toStdString(),false);m.data->keys.store(123);return 0;}
    if(argc==2&&!strcmp(argv[1],"--coop-capabilities")){
        puts("PMCOOP_ABI=2;STABLE_SAVE=1;GBA=1;FIRERED_PAIR_EXPERIMENT=1;TEST_FIXTURE=1");
        if(!getenv("COOP_FIXTURE_OLD_RECOVERY"))puts("FIRERED_RECOVERY=1");return 0;
    }
    if(!getenv("PMCOOP_SAVE_NAME") || strcmp(getenv("PMCOOP_SAVE_NAME"),"game")) return 9;
    signal(SIGTERM,stop);signal(SIGINT,stop);
    coop::Mapping mapping;mapping.open(getenv("PMCOOP_IPC"),false);
    coop::Mapping second;
    if(getenv("PMCOOP_IPC2"))second.open(getenv("PMCOOP_IPC2"),false);
    uint32_t top[coop::Pixels]{},bottom[coop::Pixels]{};
    int count=0;
    while(running&&!mapping.data->shutdown.load()) {
        for(int i=0;i<coop::Pixels;i++){top[i]=0xFF0000FF;bottom[i]=0xFF00FF00;}
        mapping.publish(top,bottom);mapping.data->peers.store(1);
        if(second.data) {
            second.publish(top,bottom);
            const auto command=mapping.data->touch.exchange(0), state=mapping.data->bridgeActive.load();
            if(coop::firered::battlePaused(state) && command==state) {
                mapping.data->bridgeActive.store(5);second.data->bridgeActive.store(5);
            }
        }
        if(getenv("COOP_FIXTURE_CRASH_PLAYER") && getenv("PMCOOP_ROLE") && !strcmp(getenv("PMCOOP_ROLE"),"join") && ++count>15)return 7;
        if(getenv("COOP_FIXTURE_EXIT_CODE") && ++count>15)return std::atoi(getenv("COOP_FIXTURE_EXIT_CODE"));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    puts("TEST_ONLY: graceful exit");return 0;
}
