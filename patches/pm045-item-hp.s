/* SPDX-License-Identifier: GPL-3.0-or-later
 * German PM 0.4.5 + Zielfix 1, overlay 122. ARMv5 Thumb-1 only.
 * Link .coop_hp_space at 0x02294af8 and .coop_hp_call at 0x02295f60.
 * The full input/overlay fingerprints pin all reused literals and callees.
 *
 * The first 68 bytes preserve the original HP-import rules, including the
 * item-command exclusion: remote item script 305 already heals natively.
 * r5=BattleContext; r6 and r7 are local scratch registers. Subsequent code
 * reinitializes r7 before using it; the outer routine restores caller r7.
 * Existing literal locations: 0x02294ddc=import packet, 0x02294dc0=resync
 * state, 0x02294dd8=BattleMon current-HP offset. No extracted ROM bytes.
 *
 * The remaining space filters exactly the direct HP subroutine called by
 * remote-item subscript 305 (depth=1, parent NARC=1, parent script=305).
 * Local bag use already applied its heal in the UI and has no matching
 * scripted HP barrier. Skipping this one-sided extra barrier keeps the
 * subsequent per-hit HP sequence aligned. Native healing, animation,
 * ordinary damage barriers and all original sync-enablement guards remain.
 */
.syntax unified
.thumb
.section .coop_hp_space, "ax"

ldr r7, [pc, #0x2e0]
ldrb r0, [r7, #0x1c]
cmp r0, #14
beq #0x02294b68
adds r7, #0x80
ldrh r6, [r7, #8]
cmp r6, #0
ble #0x02294b68
ldr r0, [pc, #0x2b4]
ldr r0, [r0, #0x14]
cmp r0, #0
bne #0x02294b68
bl #0x0228d540
movs r1, #0xc0
muls r1, r0, r1
adds r1, r5, r1
ldr r0, [pc, #0x2bc]
ldr r2, [r1, r0]
cmp r2, #0
ble #0x02294b68
subs r0, #0x4c
ldrh r3, [r1, r0]
ldrh r2, [r7, #0xe]
cmp r2, r3
bne #0x02294b68
adds r0, #0x50
ldr r2, [r1, r0]
cmp r6, r2
ble store_hp
movs r6, r2
store_hp:
subs r0, #4
str r6, [r1, r0]
b #0x02294b68
item_hp_filter:
push {r3, lr}
movs r3, #0xb8
ldr r3, [r1, r3]
cmp r3, #1
bne normal_hp_sync
movs r3, #0xbc
ldr r3, [r1, r3]
cmp r3, #1
bne normal_hp_sync
movs r3, #0xcc
ldr r3, [r1, r3]
movs r2, #0x4c
lsls r2, r2, #2
adds r2, #1
cmp r3, r2
bne normal_hp_sync
movs r0, #0
pop {r3, pc}
normal_hp_sync:
bl #0x02295c54
pop {r3, pc}
mov r8, r8

.section .coop_hp_call, "ax"
/* Replaces only the HP-sync enablement call at 0x02295f60. */
bl #0x02294b3c
