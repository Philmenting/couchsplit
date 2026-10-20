# PM 0.4.5: co-op HP synchronization after Potions

**English** · [Deutsch](de/rom-coop-item-fix.md) · [Documentation](README.md)

Investigation of 19 October 2026 with the released Alpha 5 launcher and supplied German PM ROM with Target Fix 1. **Co-op Battle Fix 1** is a separate ROM correction, rather than an official Project PM release or a new launcher package.

## Reproduced defect

Two real emulator processes could show different HP after a shared trainer-battle turn. A targeted check with the existing Target Fix 1 ROM reproduced it without the first experimental correction: after Absorb and a Potion from player 2, player 1 had 47 HP locally and 48 HP on the other system. Both battle controllers had already reached the input menu for the same turn. TCP and data transfer remained active.

The trace identified a ROM sequencing problem. The owner applies a Potion in the local bag menu; the partner instead executes additional item subscript 305. Its HP-subscript call creates **one additional network HP synchronization only on the partner**. In the reproduced case, that side advanced HP sequence 5 → 6 for healing, then 7 for the next hit; the other side stayed at 5. Subsequent hit/HP updates no longer matched. Copying already-healed partner HP ahead of time would also be wrong because native item processing heals again.

The PM program locations/subscripts were inspected in the hash-verified supplied ROM. Unmodified PRET scripts do not describe these extra PM paths. Complete ROM-mod source remains unavailable.

## Correction

`romfixes/pm045_items.py` writes authored Thumb-1 instructions into only two regions of uncompressed overlay 122:

- `0x02294af8..0x02294b67`: rewrite the original HP-import checks compactly, retaining item exclusion, positive HP, species validation, resync priority and maximum-HP clamping. Freed space holds the filter.
- `0x02295f60..0x02295f63`: route the existing HP-sync authorization call through that filter first.

Only the direct HP subcall from the partner item script is filtered: script stack depth 1, parent script NARC 1, parent script 305. This removes precisely the one-sided extra synchronization. Other calls still invoke the existing authorization function and its checks. Native healing, bag consumption and animation remain the game's responsibility. The correction writes no money or save data.

Authored assembly is in `patches/pm045-item-hp.s`. Overlay size, BSS, header, FAT, existing literal values and all other ROM files remain intact. The earlier target correction is preserved. Both players must use the same corrected ROM.

## Apply

Python 3, no extra runtime packages:

```text
python scripts/fix-pm-coop-items.py --rom Pokemon-Platinum-PM-DE-Zielfix1.nds --output Pokemon-Platinum-PM-DE-Koopfix1.nds
```

Only the Target Fix 1 input below is accepted. Complete ROM, overlay and output checksums are mandatory. Unknown/already altered inputs, identical source/destination paths and existing outputs are rejected. The tool downloads no game data and replaces no existing file.

Select the new ROM in the launcher and confirm preparation when requested. **A new ROM hash gets separate save directories.** Existing saves are not transferred automatically. Retests explicitly copied owned checkpoints into isolated profiles. See [save transfer](save-transfer.md).

| File | SHA-256 |
|---|---|
| Input: German Alpha 2 + Target Fix 1 | `dfe675837c5b935a684eb2e18c446dbc7865ba45bcb2343d2158892f6b720520` |
| Output: additionally Co-op Battle Fix 1 | `e90a53f4dd7c24d45e74c1854cfd5cb17fb4dc636179350fea6df8b9bfbd4a7f` |

Both files are 141962768 bytes. Only the two described code regions, totaling 116 bytes, are written.

## Verification and limits

At this investigation, all 55 Python tests passed locally with the real xdelta decoder and ARM engine, without skips. New tests execute the actual authored instructions in at least 560 varied states: 320 HP-import cases and 240 filter cases across both player slots. They also check the actual BL call/return, register/stack preservation, only permitted writes, native item-healing ownership and subsequent sync sequences. The existing 128 cursor states remain covered. Windows/Linux ROM CI requires the ARM dependency and no Nintendo data.

An independent private assembler and the public standard-library tool produce the same ROM; the saved assembly also matches embedded code bytes. Active retests with two real Linux emulators confirm Absorb and Potions from each player in succession, with matching HP, status, PP and stat changes. Identified filter contexts occurred on both sides and HP sequences stayed aligned. Simultaneous Potions also passed: healing, without maximum-HP clamping, 42 → 62 and 40 → 60; the subsequent opponent hit left 62/59 HP on both sides, with both HP sequences at 1. Other battle values matched. See the [machine-readable results](verification/coop-item-sync-2026-10-19.json).

Targeted survivable trainer-battle scenarios use separate save copies with prepared level-25 teams. Money, inventory, story, victory flags and positions were not edited during preparation. This is a controlled functional test, rather than a naturally played level-25 run. A separate money test with the previous ROM confirmed a real shared victory over Sven: **each player received 96 ₽**, the balance difference stayed unchanged, and both balances/victory flags survived saving and restarting. Both own followers were visible immediately on returning to the field before moving; no follower code was changed.

The correction applies to the same ROM across platforms. Actual Pokémon gameplay evidence here comes from Linux with virtual SDL controllers. Physical controllers, native Windows/Bazzite gameplay and a complete playthrough remain separate acceptance tasks. The Alpha 5 launcher binaries needed no rebuild for this ROM-only correction.
