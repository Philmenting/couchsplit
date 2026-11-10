// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "FireRedQuestLog.h"
#include <mgba/core/core.h>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace coop::firered {

// A presentation snapshot, not an injected game object. Pixel rows have a fixed
// 64-pixel pitch, including when width is smaller. No game graphics are bundled.
// The caller MUST first fingerprint and bind an exact unmodified supported FireRed ROM.
struct WorldActor {
    bool valid = false;
    uint8_t mapGroup = 0, mapNum = 0, elevation = 0;
    int tileX = 0, tileY = 0, screenX = 0, screenY = 0;
    unsigned width = 0, height = 0;
    std::array<uint32_t, 64 * 64> pixels{};
    // Sub-tile displacement relative to currentCoords, which already points at
    // the destination while walking. Never use the remote camera's screenX/Y.
    int offsetX = 0, offsetY = 0;
};

namespace world_detail {
inline int signed16(uint32_t v) {
    return v & 0x8000u ? int(v & 0xFFFFu) - 65536 : int(v & 0xFFFFu);
}

// Independently expressed from the public game's struct layouts and movement
// semantics: current/previous tiles and normal-step counters. Special jumps,
// forced slides and other unrecognized movement fail closed rather than guess.
inline bool movementOffset(mCore* c, uint32_t object, uint32_t sprite, WorldActor& actor) {
    const int dx = actor.tileX - (signed16(c->busRead16(c, object + 0x14)) - 7);
    const int dy = actor.tileY - (signed16(c->busRead16(c, object + 0x16)) - 7);
    const int secondaryX = signed16(c->busRead16(c, sprite + 0x24));
    const int secondaryY = signed16(c->busRead16(c, sprite + 0x26));
    if (std::abs(secondaryX) > 32 || std::abs(secondaryY) > 32) return false;
    actor.offsetX = secondaryX;
    actor.offsetY = secondaryY;
    if (dx == 0 && dy == 0) return true;
    if (std::abs(dx) + std::abs(dy) != 1) return false;
    const unsigned action = c->busRead8(c, object + 0x1C);
    int pixels = -1;
    const unsigned steps = c->busRead16(c, sprite + 0x38); // Sprite.data[5]
    if (action >= 0x08 && action <= 0x13) { // slow and ordinary walking
        if (steps > 16) return false;
        pixels = int(steps);
    } else if ((action >= 0x1D && action <= 0x20) ||
               (action >= 0x3D && action <= 0x40)) { // fast walk / run
        if (steps > 8) return false;
        pixels = int(steps * 2);
    } else if (action >= 0x31 && action <= 0x34) { // fast-2 normal step
        constexpr int progress[] = {0, 2, 5, 8, 10, 13, 16};
        if (steps > 6) return false;
        pixels = progress[steps];
    } else if (action >= 0x35 && action <= 0x38) {
        if (steps > 4) return false;
        pixels = int(steps * 4);
    }
    if (pixels < 0) return false;
    actor.offsetX += dx * (pixels - 16);
    actor.offsetY += dy * (pixels - 16);
    return true;
}

inline uint32_t color(uint16_t bgr) {
    const unsigned r = bgr & 31, g = (bgr >> 5) & 31, b = (bgr >> 10) & 31;
    // Match the pinned software renderer's five-bit expansion exactly.
    return 0xFF000000u | (((r << 3) | (r >> 2)) << 16) |
        (((g << 3) | (g >> 2)) << 8) | ((b << 3) | (b >> 2));
}

inline bool bounded(const WorldActor& a) {
    return a.valid && a.width > 0 && a.width <= 64 && a.height > 0 && a.height <= 64 &&
        a.mapGroup < 64 && a.mapNum < 128 && a.elevation < 16 &&
        a.tileX >= -7 && a.tileX <= 2048 && a.tileY >= -7 && a.tileY <= 2048 &&
        a.screenX >= -64 && a.screenX < 240 && a.screenY >= -64 && a.screenY < 160 &&
        a.offsetX >= -48 && a.offsetX <= 48 && a.offsetY >= -48 && a.offsetY <= 48;
}

inline int signed8(uint32_t v) { return v & 128u ? int(v & 255u) - 256 : int(v & 255u); }
inline bool romRange(uint32_t p, unsigned n) {
    return p >= 0x08000000u && n <= 0x01000000u && p <= 0x09000000u - n;
}
constexpr unsigned widths[3][4] = {{8,16,32,64},{16,32,32,64},{8,8,16,32}};
constexpr unsigned heights[3][4] = {{8,16,32,64},{8,8,16,32},{16,32,32,64}};
struct Piece { int x = 0, y = 0; unsigned width = 0, height = 0, tile = 0; };
} // namespace world_detail

inline WorldActor readWorldActor(mCore* c) {
    WorldActor actor;
    if (!c || !c->busRead8 || !c->busRead16 || !c->busRead32) return actor;
    if (!questLogWorldAvailable(c)) return actor;
    // Profiled CB2_Overworld only: no title, battle, menu, transition or stale
    // overworld RAM presented as a live avatar. Call after a completed frame.
    if (c->busRead32(c, abiAddress(c, 0x03003044)) != abiAddress(c, 0x080565D5u)) return actor;
    // START-menu and script text can retain that callback while owning the
    // screen. Avoid painting a partner over their windows or a map transition.
    if (c->busRead8(c, 0x03000F9C) != 0) return actor;
    const unsigned display = c->busRead16(c, 0x04000000);
    if ((display & 7u) != 0 || (display & 0x80u) || !(display & 0x1000u)) return actor;
    constexpr uint32_t avatar = 0x02037078, objects = 0x02036E38, sprites = 0x0202063C;
    const unsigned spriteId = c->busRead8(c, avatar + 4), objectId = c->busRead8(c, avatar + 5);
    if (spriteId >= 64 || objectId >= 16) return actor;
    const uint32_t object = objects + objectId * 0x24, sprite = sprites + spriteId * 0x44;
    const uint32_t objectFlags = c->busRead32(c, object);
    if (!(objectFlags & 1u) || !(objectFlags & 0x10000u) || (objectFlags & 0x6000u) ||
        c->busRead8(c, object + 4) != spriteId || c->busRead8(c, object + 8) != 0xFFu ||
        c->busRead16(c, sprite + 0x2E) != objectId) return actor;
    const unsigned spriteFlags = c->busRead8(c, sprite + 0x3E);
    if (!(spriteFlags & 1u) || (spriteFlags & 4u)) return actor;
    const uint32_t save = c->busRead32(c, abiAddress(c, 0x03004F58));
    if (save < 0x02000000u || save > 0x0203FFF0u || (save & 3u)) return actor;
    actor.mapGroup = uint8_t(c->busRead8(c, save + 4));
    actor.mapNum = uint8_t(c->busRead8(c, save + 5));
    actor.elevation = uint8_t(c->busRead8(c, object + 0x0B) & 15u);
    actor.tileX = world_detail::signed16(c->busRead16(c, object + 0x10)) - 7;
    actor.tileY = world_detail::signed16(c->busRead16(c, object + 0x12)) - 7;
    const unsigned attr0 = c->busRead16(c, sprite), attr1 = c->busRead16(c, sprite + 2);
    const unsigned attr2 = c->busRead16(c, sprite + 4);
    // Non-affine, normal, non-mosaic 4-bpp OBJ only. Hardware display effects
    // are deliberately not approximated.
    if (attr0 & 0x3F00u) return actor;
    const unsigned shape = attr0 >> 14, size = attr1 >> 14;
    if (shape >= 3) return actor;
    actor.width = world_detail::widths[shape][size]; actor.height = world_detail::heights[shape][size];
    actor.screenX = int(attr1 & 511u); if (actor.screenX >= 256) actor.screenX -= 512;
    actor.screenY = int(attr0 & 255u); if (actor.screenY >= 160) actor.screenY -= 256;
    std::array<world_detail::Piece,16> pieces{};
    unsigned pieceCount = 1;
    pieces[0] = {0, 0, actor.width, actor.height, attr2 & 1023u};
    const uint32_t tables = c->busRead32(c, sprite + 0x18);
    const unsigned tableState = c->busRead8(c, sprite + 0x42);
    const unsigned mode = tableState >> 6;
    if (mode > 2) return WorldActor{};
    if (tables && mode) {
        const uint32_t entry = tables + (tableState & 63u) * 8;
        if (!world_detail::romRange(tables, 8) || !world_detail::romRange(entry, 8)) return WorldActor{};
        pieceCount = c->busRead8(c, entry);
        const uint32_t data = c->busRead32(c, entry + 4);
        if (!pieceCount || pieceCount > pieces.size() || !world_detail::romRange(data, pieceCount * 4)) return WorldActor{};
        int minX = 128, minY = 128, maxX = -128, maxY = -128;
        for (unsigned i = 0; i < pieceCount; ++i) {
            const uint32_t descriptor = c->busRead16(c, data + i * 4 + 2);
            const unsigned pieceShape = descriptor & 3u, pieceSize = (descriptor >> 2) & 3u;
            if (pieceShape >= 3) return WorldActor{};
            auto& p = pieces[i];
            p.x = world_detail::signed8(c->busRead8(c, data + i * 4));
            p.y = world_detail::signed8(c->busRead8(c, data + i * 4 + 1));
            p.width = world_detail::widths[pieceShape][pieceSize];
            p.height = world_detail::heights[pieceShape][pieceSize];
            p.tile = (attr2 & 1023u) + ((descriptor >> 4) & 1023u);
            if (attr1 & 0x1000u) p.x = -p.x - int(p.width);
            if (attr1 & 0x2000u) p.y = -p.y - int(p.height);
            if (p.x < minX) minX = p.x;
            if (p.y < minY) minY = p.y;
            if (p.x + int(p.width) > maxX) maxX = p.x + int(p.width);
            if (p.y + int(p.height) > maxY) maxY = p.y + int(p.height);
        }
        if (maxX <= minX || maxY <= minY || maxX - minX > 64 || maxY - minY > 64) return WorldActor{};
        actor.screenX += minX - world_detail::signed8(c->busRead8(c, sprite + 0x28));
        actor.screenY += minY - world_detail::signed8(c->busRead8(c, sprite + 0x29));
        actor.width = unsigned(maxX - minX); actor.height = unsigned(maxY - minY);
        for (unsigned i = 0; i < pieceCount; ++i) { pieces[i].x -= minX; pieces[i].y -= minY; }
    }
    if (!world_detail::movementOffset(c, object, sprite, actor)) return WorldActor{};
    actor.valid = true;
    if (!world_detail::bounded(actor) || actor.screenX + int(actor.width) <= 0 ||
        actor.screenY + int(actor.height) <= 0) return WorldActor{};
    const unsigned palette = attr2 >> 12;
    std::array<uint32_t,16> colors{};
    for (unsigned i = 1; i < 16; ++i) colors[i] = world_detail::color(uint16_t(c->busRead16(c, 0x05000200 + palette * 32 + i * 2)));
    // Earlier pieces become earlier hardware OAM entries, hence win overlaps.
    for (unsigned i = pieceCount; i > 0; --i) {
        const auto& p = pieces[i - 1];
        const unsigned tileRowStride = (display & 0x40u) ? p.width / 8 : 32;
        for (unsigned y = 0; y < p.height; ++y) for (unsigned x = 0; x < p.width; ++x) {
            const unsigned tx = (attr1 & 0x1000u) ? p.width - 1 - x : x;
            const unsigned ty = (attr1 & 0x2000u) ? p.height - 1 - y : y;
            const unsigned tile = p.tile + (ty / 8) * tileRowStride + tx / 8;
            if (tile >= 1024) return WorldActor{};
            const unsigned packed = c->busRead8(c, 0x06010000 + tile * 32 + (ty % 8) * 4 + (tx % 8) / 2);
            const unsigned index = (packed >> ((tx & 1u) * 4)) & 15u;
            if (index) actor.pixels[(unsigned(p.y) + y) * 64 + unsigned(p.x) + x] = colors[index];
        }
    }
    return actor;
}

// Pure framebuffer overlay; never changes ROM, game RAM, collision, events or
// saves. Connected-map projection is supplied separately; terrain/roof occlusion
// and native partner collision still require engine-level work.
inline void compositePartner(uint32_t* argb, unsigned stride, const WorldActor& local, const WorldActor& remote) {
    if (!argb || stride < 240 || stride > 4096 || !world_detail::bounded(local) ||
        !world_detail::bounded(remote) || local.mapGroup != remote.mapGroup ||
        local.mapNum != remote.mapNum || local.elevation != remote.elevation) return;
    const int x0 = local.screenX + int(local.width / 2) - int(remote.width / 2) +
        (remote.tileX - local.tileX) * 16 + remote.offsetX - local.offsetX;
    const int y0 = local.screenY + int(local.height) - int(remote.height) +
        (remote.tileY - local.tileY) * 16 + remote.offsetY - local.offsetY;
    const bool behind = y0 + int(remote.height) <= local.screenY + int(local.height);
    for (unsigned y = 0; y < remote.height; ++y) for (unsigned x = 0; x < remote.width; ++x) {
        const int destX = x0 + int(x), destY = y0 + int(y);
        if (destX < 0 || destX >= 240 || destY < 0 || destY >= 160) continue;
        const uint32_t pixel = remote.pixels[y * 64 + x];
        if (!(pixel >> 24)) continue;
        const int lx = destX - local.screenX, ly = destY - local.screenY;
        if (behind && lx >= 0 && ly >= 0 && lx < int(local.width) && ly < int(local.height) &&
            (local.pixels[unsigned(ly) * 64 + unsigned(lx)] >> 24)) continue;
        argb[unsigned(destY) * stride + unsigned(destX)] = pixel;
    }
}
} // namespace coop::firered
