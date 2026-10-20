# Project PM 0.4.5: target selection after an opponent faints

**English** · [Deutsch](de/rom-target-cursor-fix.md) · [Documentation](README.md)

Investigation of 16 October 2026 with the supplied German Alpha 2 ROM and Alpha 4 launcher. The corrected ROM is **German Alpha 2 + Target Fix 1**; this is a separate correction, rather than an official Project PM release.

## Reproduced defect and cause

In two real shared wild battles on Route 201, the previous target remained on the empty left slot after that opponent fainted. A confirmed no command; moving right to the living opponent and pressing A worked. General input and network transport were still functioning.

The original logic can be followed in [PRET's source](https://github.com/pret/pokeplatinum/blob/1dc13dcea40c04ab53f1f607de46023dc409ef20/src/battle/battle_subscreen.c#L3534): `BattleSystem_Cursor_Battler` restores the saved cursor when the layout is unchanged without checking presence. A-button processing checks presence and discards an empty target. The same pattern was verified in unchanged battle overlay 16 of the supplied ROM. Complete Project PM ROM-mod source remains unavailable; public emulator source does not replace it.

## Correction

`romfixes/pm045_target.py` replaces only two code regions in this verified uncompressed overlay. Authored Thumb-1 assembly is in `patches/pm045-target-cursor.s`.

- Preserve the previous target if its layout matches and it is still present.
- Otherwise retain the original available-target priority: upper left, upper right, lower left, lower right (battler slots 3, 1, 0, 2).
- Area/self-target menus, manual navigation, moves, opponents, network data and saves are unchanged.
- Existing literal pool, overlay size/address, BSS, file table, header, text and all other ROM files remain byte-identical.
- Both players need the same corrected ROM. The correction applies wherever that ROM is emulated; the actual gameplay test here was on Linux. No Windows/Bazzite gameplay acceptance is implied.

The requirement to preserve all program bytes during **localization alone** still applies to `localize-pm.py`. This separately commissioned correction intentionally changes the specified program regions. It overwrites neither the Alpha 2 input nor existing outputs.

## Apply

Python 3, no extra runtime packages:

```text
python scripts/fix-pm-target-cursor.py --rom Pokemon-Platinum-PM-0.4.5-DE-Alpha2.nds --output Pokemon-Platinum-PM-0.4.5-DE-Zielfix1.nds
```

Only the exact Alpha 2 fingerprint is accepted. Unknown, altered, English or already corrected ROMs are rejected, as are existing outputs and identical source/destination paths. There is no checksum override. The original and saves are retained.

Select the new file in the launcher and confirm preparation when requested. **The new ROM hash gets separate save directories.** Existing saves are not automatically transferred. Private retests explicitly copied owned checkpoints into isolated test profiles; this does not implement automatic user-profile migration. See [save transfer](save-transfer.md).

| File | SHA-256 |
|---|---|
| Input: German Alpha 2, 141962768 bytes | `800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c` |
| Battle overlay 16, 235360 bytes | `c6acaf7747783d49d2a1bc549efaf009ddb0f9f584a1569d41ed76b90a34d716` |
| Output: German Alpha 2 + Target Fix 1, same size | `dfe675837c5b935a684eb2e18c446dbc7865ba45bcb2343d2158892f6b720520` |

The replaced ROM regions begin at `0x2300ec` (64 bytes) and `0x230140` (12 bytes), runtime addresses `0x0227f3ec` and `0x0227f440`. Nintendo program bytes, ROMs, saves, original text and private test images are not included with the correction source.

## Regression tests and actual gameplay

ROM-free tests execute the authored ARM instructions with Unicorn: all 16 presence masks × four previous cursor positions × matching/changed layout = 128 combinations. They check valid selection, preservation of a valid previous target, unchanged stack/preserved registers and exactly two allowed cursor-byte writes. Separate tests check file/hash boundaries, protected inputs/outputs and output fingerprints. Linux/Windows CI requires the ARM tests; only tests need `tests/requirements-romfix.txt`.

The original instructions also reproduced the defect privately. The same state selected the living right opponent after correction; original game code remains private.

**The active corrected-ROM retest passed:** two real Alpha 4 emulator processes, virtual SDL controllers, the same ROM on both sides, a shared wild battle against two level-2 Starly (Staralili in the German ROM). After the left one fainted, both cursors selected the living right one. One A press from player 1 was accepted and showed the waiting state while player 2 retained independent selection. After player 2 confirmed, the round continued; both won, received experience and reached level 7. Own/remote followers were visible on return to the field. No extra directional input was needed.

Saving/loading also passed: move both characters on Route 201, save normally, wait for completion, stop both processes and restart. Both loaded the new positions. Each save is 524288 bytes and remained unchanged on loading. An earlier test driver stopped during the save indicator and reloaded the old level-6 checkpoint; that was a test-sequencing error, rather than a demonstrated product defect. The proper repeat used that checkpoint, rather than the unsaved level-7 state. Original/checkpoint saves were retained. All sessions stopped without workers or profile locks left behind.

## Second observation: followers

A suspected permanent follower loss was not reproduced. After loading the unchanged old checkpoint, both Turtwig (Chelast) became visible while walking, without changing maps/settings or applying a repair. Both also followed in grass. After another complete shared wild battle with the **unchanged** ROM, own and remote followers were visible.

No follower code change was justified. The initial missing display's exact cause, such as overlap/position or a temporary state, remains uncertain. The permanent-loss hypothesis was withdrawn; this does not establish that the entire follower feature is defect-free.
