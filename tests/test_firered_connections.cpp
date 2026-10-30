// SPDX-License-Identifier: GPL-3.0-or-later
// Authored geometry/memory fixtures; no commercial ROM or graphics.
#include "../gba/FireRedConnections.h"
#include <iostream>
#include <stdexcept>
#include <unordered_map>
using namespace coop::firered;
namespace {
unsigned checks=0;
void check(bool ok,const char* why) { ++checks;if(!ok)throw std::runtime_error(why); }
std::unordered_map<uint32_t,uint8_t> memory;
uint32_t read8(mCore*,uint32_t p) { const auto i=memory.find(p);return i==memory.end()?0:i->second; }
uint32_t read32(mCore* c,uint32_t p) { uint32_t v=0;for(unsigned i=0;i<4;++i)v|=read8(c,p+i)<<(i*8);return v; }
void put(uint32_t p,uint32_t v,unsigned n=4) {for(unsigned i=0;i<n;++i)memory[p+i]=uint8_t(v>>(i*8));}
WorldActor actor(unsigned num,int x,int y) {WorldActor a;a.valid=true;a.mapGroup=3;a.mapNum=num;a.tileX=x;a.tileY=y;
 a.width=16;a.height=32;a.screenX=112;a.screenY=56;a.offsetX=-8;a.pixels[0]=0xFF00FF00;return a;}
WorldTopology map(unsigned num,unsigned w,unsigned h) {WorldTopology t;t.valid=true;t.mapGroup=3;t.mapNum=num;t.width=w;t.height=h;return t;}
void connect(WorldTopology& a,WorldTopology& b,unsigned direction,int offset) {
 constexpr unsigned inverse[]={0,2,1,4,3};a.count=b.count=1;
 a.connections[0]={direction,b.mapGroup,b.mapNum,offset};b.connections[0]={inverse[direction],a.mapGroup,a.mapNum,-offset};
}
}
int main(){try{
 auto pallet=map(0,24,20),route=map(19,24,40);connect(pallet,route,2,0);
 auto a=actor(0,10,0),b=actor(19,10,39);
 auto p=projectConnectedPartner(a,b,pallet,route);
 check(p && p->tileX==10 && p->tileY==-1 && p->mapNum==0,"north Pallet projection");
 check(p->pixels==b.pixels && p->offsetX==b.offsetX && p->screenX==b.screenX,"native sprite/subtile preserved");
 p=projectConnectedPartner(b,a,route,pallet);check(p&&p->tileY==40,"south reciprocal projection");
 auto city=map(1,48,40);connect(route,city,2,-12);a=actor(19,10,0);b=actor(1,22,39);
 p=projectConnectedPartner(a,b,route,city);check(p&&p->tileX==10&&p->tileY==-1,"signed north offset");
 p=projectConnectedPartner(b,a,city,route);check(p&&p->tileX==22&&p->tileY==40,"signed south offset");
 auto west=map(20,48,24);connect(city,west,3,8);a=actor(1,0,12);b=actor(20,47,4);
 p=projectConnectedPartner(a,b,city,west);check(p&&p->tileX==-1&&p->tileY==12,"west offset");
 p=projectConnectedPartner(b,a,west,city);check(p&&p->tileX==48&&p->tileY==4,"east reciprocal offset");
 for(int distance=0;distance<48;++distance){b.tileX=47-distance;p=projectConnectedPartner(a,b,city,west);
  check(bool(p)==(distance<7),"loaded seven-tile border only");}
 b=actor(20,47,4);auto broken=west;broken.connections[0].offset=1;
 check(!projectConnectedPartner(a,b,city,broken),"nonreciprocal offset rejected");
 broken=west;broken.count=0;check(!projectConnectedPartner(a,b,city,broken),"missing reciprocal rejected");
 broken=city;broken.connections[1]=broken.connections[0];broken.count=2;
 check(!projectConnectedPartner(a,b,broken,west),"ambiguous edge rejected");
 broken=city;broken.connections[0].direction=5;check(!projectConnectedPartner(a,b,broken,west),"dive/warp unsupported");
 broken=city;broken.count=17;check(!projectConnectedPartner(a,b,broken,west),"oversized table rejected");
 broken=city;broken.mapNum++;check(!projectConnectedPartner(a,b,broken,west),"stale topology rejected");
 b.tileX=-1;check(!projectConnectedPartner(a,b,city,west),"transition actor excluded");
 b=a;check(projectConnectedPartner(a,b,{},{}).has_value(),"same-map path independent of topology");
 b.valid=false;check(!projectConnectedPartner(a,b,city,west),"invalid actor excluded");
 mCore core{};core.busRead8=read8;core.busRead32=read32;a=actor(19,10,0);
 put(0x03003044,0x080565D5);put(0x03004F58,0x02030000);put(0x02030004,3,1);put(0x02030005,19,1);
 put(0x02036DFC,0x08001000);put(0x08001000,24);put(0x08001004,40);
 put(0x02036E08,0x08002000);put(0x08002000,1);put(0x08002004,0x08003000);
 put(0x08003000,2,1);put(0x08003004,0xFFFFFFF4);put(0x08003008,3,1);put(0x08003009,1,1);
 auto topology=readWorldTopology(&core,a);check(topology.valid&&topology.width==24&&topology.height==40&&
 topology.connections[0].offset==-12&&topology.connections[0].mapNum==1,"native memory layout and signed offset");
 const auto before=memory;readWorldTopology(&core,a);check(before==memory,"topology read performs no writes");
 put(0x03000F9C,1,1);check(!readWorldTopology(&core,a).valid,"menu ownership excluded");put(0x03000F9C,0,1);
 put(0x03003044,0x0800FD21);check(!readWorldTopology(&core,a).valid,"battle excluded");put(0x03003044,0x080565D5);
 put(0x08002000,17);check(!readWorldTopology(&core,a).valid,"untrusted connection count");put(0x08002000,1);
 put(0x08003004,0x80000000);check(!readWorldTopology(&core,a).valid,"extreme signed offset rejected");put(0x08003004,0);
 put(0x08002004,0x08FFFFFC);check(!readWorldTopology(&core,a).valid,"ROM-end table overrun rejected");put(0x08002004,0x08003000);
 put(0x02036DFC,0x02001000);check(!readWorldTopology(&core,a).valid,"non-ROM layout rejected");
 std::cout<<"FireRed connections: "<<checks<<" checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
