/* SPDX-License-Identifier: GPL-3.0-or-later
 * Own Thumb replacement for PM 0.4.5 overlay 16 cursor initialization.
 * Assemble each block separately at the stated address (ARMv5 Thumb-1).
 * Do not overwrite the literal pool at 0x0227f42c..0x0227f43f.
 * Equivalent behavior: keep saved target iff layout matches and it is present;
 * otherwise select the first present slot in existing order 3, 1, 0, 2.
 * No-present-slot fallback remains the original bottom-right slot.
 * Context: pret/pokeplatinum 1dc13dcea40c04ab53f1f607de46023dc409ef20,
 * src/battle/battle_subscreen.c, BattleSystem_Cursor_Battler.
 */
.syntax unified
.thumb
/* 0x0227f3ec; r6=BattleSubscreen*, r7=MenuCursor*;
 * sp+0x1c=saved cursor*, sp+0x50=slot layout, sp+0x5a=present flags.
 * Existing literal load resolves to 0x0227f43c (targetingLayout offset 0x68c).
 */

ldr r0, [sp, #0x1c]
ldrb r1, [r0, #6]
ldr r0, [pc, #0x48]
ldrb r0, [r6, r0]
cmp r1, r0
bne fallback
ldr r0, [sp, #0x1c]
ldrb r1, [r0, #4]
ldrb r2, [r0, #5]
lsls r3, r2, #1
adds r3, r3, r1
add r0, sp, #0x50
ldrb r3, [r0, r3]
adds r0, #10
ldrb r3, [r0, r3]
cmp r3, #1
bne fallback
strb r1, [r7, #2]
strb r2, [r7, #1]
b #0x227f456
fallback:
add r0, sp, #0x50
add r3, sp, #0x58
adds r3, #2
movs r1, #0
loop:
ldrb r2, [r0, r1]
ldrb r2, [r3, r2]
cmp r2, #1
beq #0x227f440
adds r1, #1
cmp r1, #3
blt loop
b #0x227f440

/* 0x0227f440, following the preserved literal pool. */
movs r0, #1
ands r0, r1
strb r0, [r7, #2]
lsrs r1, r1, #1
strb r1, [r7, #1]
b #0x227f456
