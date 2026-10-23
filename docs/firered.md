# GBA game support

**English** · [Deutsch](de/firered.md) · [Documentation](README.md)

The local development build can run GBA games and ROM hacks in the existing CouchSplit interface. Select a `.gba` file or ZIP containing exactly one game. The launcher checks the GBA fixed header byte, header checksum and a size between 512 bytes and 32 MiB for standard cartridges. Explicitly marked CouchSplit XROM v1 files can be up to 512 MiB. A valid header identifies the file format; it does not certify that every game or hack works. Oversized and malformed files are rejected. Existing Project PM / Platinum support and the DS worker are retained; other DS games are not enabled by this change.

Use **Start split screen** after assigning two controllers. Each player has one 240×160 screen, independent input/audio and their own battery save. The menu layout is retained, with game-specific labels/help. There is no additional journey menu or emulator settings field. Shared-world co-op, GBA link trading/battles, a shared Pokémon bank and travel between editions are not implemented by this worker.

Save through the game's own menu and wait until its save indicator finishes. Start + Select held for two seconds stops the session. The worker snapshots changed battery data atomically every 60 frames and at orderly shutdown. Supported save sizes are EEPROM 512 bytes / 8 KiB, SRAM 32 KiB and Flash/SRAM 64 KiB / Flash 128 KiB. Games without battery storage do not create an empty save. Invalid sizes and detected size mismatches fail without overwriting the existing file; a snapshot is not a substitute for an in-game save. Saves and verified backups remain separate per player and exact ROM SHA-256. Different builds/languages never silently share or migrate a save.

The launcher chooses the bundled `gba-worker` automatically. The existing DS emulator and BIOS settings remain unchanged and do not affect GBA games. No external GBA BIOS is needed; mGBA uses its software implementation. Select + R is ordinary GBA input, because there is no second GBA screen to toggle. Native controller assignment/disconnect behaviour and the exit chord are retained.

Build with `scripts/build.sh`; mGBA 0.10.5 is pinned in `config/mgba.lock.json`. Source packages include pinned mGBA source and MPL-2.0 licenses under `gba-corresponding-source`, plus the reproducible XROM overlay in `patches/mgba-xrom.patch`. CMake applies the overlay to a separate owned build copy; the pinned checkout stays pristine. An offline build can set `-DCOUCHSPLIT_MGBA_SOURCE=/path/to/gba-corresponding-source`. The IPC ABI remains 2. Released Alpha 8 packages and the previous FireRed-only preview do not include this general GBA extension. Native Windows/Bazzite execution has not been verified for this change.

Production-worker acceptance without commercial game assets:

```sh
python3 tests/smoke_gba.py build/bin/gba-worker
```

The private `build/launcher/verify-gba` acceptance tool runs a separately supplied GBA ROM with production Session/PlayView and virtual SDL controllers. Authored ARM programs verify memory devices and persistence; actual German FireRed gameplay is checked separately. Neither is evidence of physical-controller support or a complete playthrough of every game.

## Extended journey ROMs

[CouchSplit XROM v1](extended-rom.md) keeps one `.gba` file and the same launcher UI. It exposes immutable assets after the native 32 MiB through an additional emulator data aperture. It is a custom format requiring the matching worker, not a standard cartridge that runs in arbitrary GBA emulators or hardware. The seven reserved header bytes at 0xB5 contain `CSXROM1`, the native header checksum is valid, and the file is four-byte aligned and greater than 32 MiB. Unknown oversized ROMs remain rejected. A larger container does not add missing game chapters.
