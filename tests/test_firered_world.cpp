// SPDX-License-Identifier: GPL-3.0-or-later
// Authored synthetic memory fixtures; no ROM, graphics or game save required.
#include "../gba/FireRedWorld.h"
#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace {
std::unordered_map<uint32_t,uint8_t> memory;
uint32_t read8(mCore*, uint32_t address) { auto i = memory.find(address); return i == memory.end() ? 0 : i->second; }
uint32_t read16(mCore* c, uint32_t a) { return read8(c,a) | (read8(c,a+1) << 8); }
uint32_t read32(mCore* c, uint32_t a) { return read16(c,a) | (read16(c,a+2) << 16); }
void put(uint32_t a, uint32_t value, unsigned size = 4) { for (unsigned i=0;i<size;++i) memory[a+i] = uint8_t(value >> (8*i)); }
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
constexpr uint32_t Object = 0x02036E38, Sprite = 0x0202063C;
constexpr uint32_t Red = 0xFFFF0000u, Green = 0xFF00FF00u;

void reset() {
    memory.clear();
    put(0x03003044,0x080565D5); put(0x04000000,0x1040,2);
    put(0x03004F58,0x02030000); put(0x02030004,3,1);
    put(Object,0x10001); put(Object+8,255,1);
    put(Object+0x10,17,2); put(Object+0x12,16,2);
    put(Object+0x14,17,2); put(Object+0x16,16,2);
    put(Sprite,0x8000|56,2); put(Sprite+2,0x8000|112,2);
    put(Sprite+4,0x1000,2); put(Sprite+0x28,0xF0F8,2); put(Sprite+0x3E,3,1);
    put(0x05000222,31,2); put(0x05000224,31u<<5,2);
    for (unsigned i=0;i<256;++i) put(0x06010000+i,i<128?0x11:0x22,1);
}
void compound() {
    put(Sprite+0x18,0x08001000); put(Sprite+0x42,0x40,1);
    put(0x08001000,2,1); put(0x08001004,0x08002000);
    put(0x08002000,0x0004F0F8); // 16x16, tile offset 0, (-8,-16)
    put(0x08002004,0x004400F8); // 16x16, tile offset 4, (-8,0)
}
}

int main() {
    try {
        mCore core{}; core.busRead8=read8; core.busRead16=read16; core.busRead32=read32;
        reset(); auto actor=coop::firered::readWorldActor(&core);
        check(actor.valid && actor.width==16 && actor.height==32,"ordinary sprite dimensions");
        check(actor.mapGroup==3 && actor.mapNum==0 && actor.tileX==10 && actor.tileY==9,"map coordinates");
        check(actor.pixels[0]==Red && actor.pixels[31*64]==Green,"1D tile/palette decode");
        put(0x06010000,0x10,1); actor=coop::firered::readWorldActor(&core);
        check(actor.pixels[0]==0 && actor.pixels[1]==Red,"palette zero transparency");

        reset(); compound(); actor=coop::firered::readWorldActor(&core);
        check(actor.valid && actor.screenX==112 && actor.screenY==56,"compound sprite anchor");
        check(actor.width==16 && actor.height==32 && actor.pixels[0]==Red && actor.pixels[16*64]==Green,"compound tile offsets");
        put(Sprite+2,0xA000|112,2); actor=coop::firered::readWorldActor(&core);
        check(actor.valid && actor.pixels[0]==Green && actor.pixels[31*64]==Red,"compound vertical flip");
        reset(); compound(); put(0x06010000,0x21,1); put(Sprite+2,0x9000|112,2);
        actor=coop::firered::readWorldActor(&core);
        check(actor.valid && actor.pixels[15]==Red && actor.pixels[14]==Green,"compound horizontal flip");

        reset(); put(0x04000000,0x1000,2); // 2D layout: each tile row is 32 tiles
        for(unsigned i=0;i<32;++i)put(0x06010000+32*32+i,0x22,1);
        actor=coop::firered::readWorldActor(&core);
        check(actor.valid && actor.pixels[8*64]==Green,"2D tile row stride");

        reset(); put(Object+0x10,18,2); put(Object+0x1C,0x13,1); put(Sprite+0x38,8,2);
        actor=coop::firered::readWorldActor(&core);
        check(actor.valid && actor.offsetX==-8 && actor.offsetY==0,"subtile walking displacement");
        put(Object+0x1C,0x17,1); check(!coop::firered::readWorldActor(&core).valid,"unsupported movement rejection");

        reset(); put(0x03003044,0x08011085); check(!coop::firered::readWorldActor(&core).valid,"battle exclusion");
        reset(); put(0x03003044,0x0811EBD5); check(!coop::firered::readWorldActor(&core).valid,"menu exclusion");
        reset(); put(0x03000F9C,1,1); check(!coop::firered::readWorldActor(&core).valid,"START-menu/script ownership exclusion");
        reset(); put(coop::firered::QuestLogStateAddress,2,1); check(!coop::firered::readWorldActor(&core).valid,"historical replay world exclusion");
        reset(); put(coop::firered::QuestLogStateAddress,3,1); check(!coop::firered::readWorldActor(&core).valid,"final replay transition exclusion");
        reset(); put(coop::firered::QuestLogStateAddress,1,1); check(coop::firered::readWorldActor(&core).valid,"normal quest-log recording remains playable");
        reset(); put(Sprite,0xA000|56,2); check(!coop::firered::readWorldActor(&core).valid,"8bpp rejection");
        reset(); put(Sprite,0x8100|56,2); check(!coop::firered::readWorldActor(&core).valid,"affine rejection");
        reset(); compound(); put(0x08001000,17,1); check(!coop::firered::readWorldActor(&core).valid,"oversized compound rejection");
        reset(); compound(); put(Sprite+0x18,0xFFFFFFFF); check(!coop::firered::readWorldActor(&core).valid,"invalid table pointer rejection");
        reset(); compound(); put(0x08002000,0x3FF4F0F8); check(!coop::firered::readWorldActor(&core).valid,"VRAM tile overflow rejection");

        reset(); actor=coop::firered::readWorldActor(&core); auto remote=actor;
        remote.tileX+=2; remote.offsetX=-8;
        std::array<uint32_t,256*160> frame{};
        coop::firered::compositePartner(frame.data(),256,actor,remote);
        check(frame[56*256+136]==Red && frame[56*256+135]==0 && frame[56*256+152]==0,"relative placement");
        check(frame[56*256+240]==0,"framebuffer padding preserved");
        remote.tileX+=6; frame.fill(0); coop::firered::compositePartner(frame.data(),256,actor,remote);
        check(frame[56*256+239]==Red && frame[56*256+240]==0,"right edge clipping");
        remote.elevation++; frame.fill(0); coop::firered::compositePartner(frame.data(),256,actor,remote);
        for(auto pixel:frame)check(pixel==0,"elevation mismatch exclusion");
        remote=actor; remote.mapNum++; coop::firered::compositePartner(frame.data(),256,actor,remote);
        for(auto pixel:frame)check(pixel==0,"map mismatch exclusion");
        remote=actor; remote.width=65; coop::firered::compositePartner(frame.data(),256,actor,remote);
        for(auto pixel:frame)check(pixel==0,"untrusted snapshot bounds");
        std::cout<<"FireRed world synthetic fixtures passed\n";
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
