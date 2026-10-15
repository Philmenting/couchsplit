#!/usr/bin/env python3
"""Compile and test the ACTUAL patched mpnet transport, without a ROM or Qt.
This extracts the production namespace. It does not substitute a fake transport.
The only stubs are unused online relay resolution and SDL's monotonic clock.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
import os


def test(source, project):
    text = (source / 'src/frontend/qt_sdl/EmuThread.cpp').read_text()
    begin = text.index('namespace mpnet\n{')
    end = text.index('\nstruct BridgeSt', begin)
    transport = text[begin:end]
    prelude = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>
#include "SocketCompat.h"
#include "MpOnline.h"
namespace melonDS { using u8=uint8_t; using u16=uint16_t; using u32=uint32_t; }
uint32_t SDL_GetTicks() { return GetTickCount(); }
bool MpOnlineResolve(const char*,int,MpOnlineTarget*) { return false; }
'''
    checks = r'''
#ifdef _WIN32
int setenv(const char* key,const char* value,int) {return _putenv_s(key,value);}
#endif
int main() {
    WSADATA data;WSAStartup(MAKEWORD(2,2),&data);
    // Ephemeral port avoids collisions. Both production endpoints are nonblocking.
    auto probe=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in address{}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(probe,(sockaddr*)&address,sizeof(address))==0);
    #ifdef _WIN32
    int size=sizeof(address);
#else
    socklen_t size=sizeof(address);
#endif
     assert(getsockname(probe,(sockaddr*)&address,&size)==0);
    closesocket(probe);
    auto port=std::to_string(ntohs(address.sin_port));
    setenv("PMCOOP_PORT",port.c_str(),1);setenv("PMCOOP_ROLE","host",1);
    mpnet::Net host,client;host.startHost();client.startJoin("127.0.0.1");
    bool quiet=false;
    auto pump=[&](int begin,int end){for(int f=begin;f<end;f++){if(quiet){host.lastNameF=f;client.lastNameF=f;client.lastPingF=f;}host.tick(f);client.tick(f);std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
    pump(0,300);
    assert(host.anyUp() && client.anyUp()); assert(client.myRole()==2);
    assert(!strcmp(host.rname[2],client.myName));
    quiet=true; pump(300,330); // Drain outstanding pings before raw fragmentation.
    // Bidirectional game frames use the upstream LE length framing unchanged.
    uint8_t outbound[]={1,1,4,0,10,20,30,40};uint8_t buf[64]{};
    host.sendAll(outbound,sizeof(outbound));pump(300,330);
    assert(client.recvFrame(0,buf,sizeof(buf))==sizeof(outbound));assert(!memcmp(buf,outbound,sizeof(outbound)));
    outbound[1]=2;client.sendAll(outbound,sizeof(outbound));pump(330,360);
    assert(host.recvFrame(0,buf,sizeof(buf))==sizeof(outbound));assert(!memcmp(buf,outbound,sizeof(outbound)));
    // A fragmented frame must not be consumed early.
    quiet=true;
    uint8_t frame[]={8,0,1,1,4,0,11,22,33,44};
    assert(bridgeSend(host.peers[0].s,(char*)frame,1,0)==1);pump(360,370);
    assert(client.recvFrame(0,buf,sizeof(buf))==0);
    assert(bridgeSend(host.peers[0].s,(char*)frame+1,9,0)==9);pump(370,380);
    assert(client.recvFrame(0,buf,sizeof(buf))==8);assert(!memcmp(buf,frame+2,8));
    // Zero-length frames are invalid and drop only the bad peer.
    uint8_t bad[]={0,0};assert(bridgeSend(host.peers[0].s,(char*)bad,2,0)==2);pump(380,390);
    assert(client.recvFrame(0,buf,sizeof(buf))==0);assert(!client.anyUp());
    quiet=false;
    pump(390,700);assert(host.anyUp() && client.anyUp());assert(client.myRole()==2);
    host.shutdown();pump(700,730);assert(!client.anyUp());
    host.startHost();pump(730,1100);assert(host.anyUp() && client.anyUp());
    client.shutdown();host.shutdown();
    // POSIX select must use fd+1 and SO_ERROR must reject a refused connect.
    auto sock=socket(AF_INET,SOCK_STREAM,0);mpnet::setNonBlock(sock);
    address.sin_port=htons(1);connect(sock,(sockaddr*)&address,sizeof(address));
    fd_set wr,ex;FD_ZERO(&wr);FD_ZERO(&ex);FD_SET(sock,&wr);FD_SET(sock,&ex);timeval tv{1,0};
    assert(bridgeSelect(sock,&wr,&ex,&tv)>0);assert(!bridgeConnectOK(sock));closesocket(sock);
    // A dead peer must never kill the host with SIGPIPE.
#ifndef _WIN32
    int pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);close(pair[1]);
    assert(bridgeSend(pair[0],"x",1,0)==-1);close(pair[0]);
#endif
    puts("PASS: production bridge localhost, roles, framing, reconnect and refused connect");
}
'''
    with tempfile.TemporaryDirectory() as temp:
        cpp = Path(temp) / 'transport.cpp'; binary=Path(temp)/('transport.exe' if os.name=='nt' else 'transport')
        cpp.write_text(prelude+transport+checks)
        subprocess.run(['g++','-std=c++17','-O1','-Wall','-Wextra','-pthread','-I'+str(project/'multiplayer'),'-I'+str(source/'src/frontend/qt_sdl'),str(cpp),'-o',str(binary),*(['-lws2_32'] if os.name=='nt' else [])],check=True)
        subprocess.run([str(binary)],check=True,timeout=20)


if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);args=p.parse_args()
    test(args.source,Path(__file__).resolve().parents[1])
