# CouchSplit extended GBA ROM format

**English** · [Deutsch](de/extended-rom.md) · [GBA guide](firered.md)

CouchSplit XROM v1 permits one file up to 512 MiB without changing the launcher menu. This is an emulator extension for authored journey builds. Standard GBA cartridges remain limited to 32 MiB. Appending commercial ROMs does not combine their game engines or story content.

The first 32 MiB retain normal GBA mapping, code, mirrors, GPIO and save buses. File bytes from offset `0x02000000` are mapped as immutable data at guest `0x10000000`; up to 480 MiB are available, ending before `0x2E000000`. Code must stay in the native cartridge region. CPU byte/halfword/word and LDM reads, DMA sources and BIOS decompression read the same extra bytes. Unaligned loads retain native rotation; reads beyond actual extension length return FF. Writes do not alter the file. The host owns the read-only file mapping for the lifetime of the core, separately in each worker. Nothing is persisted as mutable bank state.

A file must have the normal fixed byte/checksum plus `CSXROM1` in reserved bytes `0xB5..0xBB`, a total size greater than 32 MiB and at most 512 MiB, and four-byte alignment. Raw and single-ROM ZIP import apply the same bounds. Old workers lacking `GBA_XROM=1` are rejected before profile locking. Saves retain their native device size and full-file SHA-256 player isolation; no implicit migration from another build.

mGBA 0.10.5 remains pinned. `scripts/prepare-mgba-xrom.py` verifies the five patched upstream files, prepares an owned source copy and verifies every patched result. CMake derives its output directory from the patch hash, permitting upgrades without overwriting previous generated sources. Source packages carry the pristine source, patch, preparation tool and licenses so an offline build reconstructs the matching core.

A linker can place a data section at VMA `0x10000000`, LMA `0x0A000000` alongside code based at `0x08000000`; objcopy then places the assets at file offset 32 MiB. Reserve the native span, emit actual extension data and set the marker/checksum. The marker does not convert a normal game into an extended game; all relocated data references must use the correct guest addresses. Ordinary GBA emulators and physical GBA hardware are not implicitly compatible.

Run `python3 tests/smoke_xrom.py build/bin/gba-worker` for self-authored, asset-free CPU/DMA/BIOS/capacity/input/save checks. `tests/smoke_gba.py` remains the native-cartridge regression. These tests are separate from any private game compatibility checks or complete playthroughs. Physical controllers, audible output and Windows/Bazzite execution are not verified for this extension.
