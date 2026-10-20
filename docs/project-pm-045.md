# Project PM 0.4.5: ROM requirements and package verification

**English** · [Deutsch](de/project-pm-045.md) · [Documentation](README.md)

This records the package tests of 14 October 2026 and the localization update of 15 October. Later gameplay results are in [Development status](development-status.md). A native German CPUD engine port remains unimplemented; the [German text alpha](german-localization.md) uses the US engine.

The supplied `Build V0.4.5.zip` contains a real binary ROM patch. **It requires Pokémon Platinum USA Revision 1 and cannot be applied directly to the original German ROM.** The initial German patch attempt produced no working output.

## Verified US base and successful patch

The user subsequently supplied the matching USA Rev 1 base. Its SHA-1 matches the required base exactly; its SHA-256 is `fbce4c4def0c7797f8dd238a3d7a5e48b4a7e3abd86890ac65f6321cef781bdb`.

Standard `xdelta3 -d -s ORIGINAL.nds PATCH.xdelta RESULT.nds` completed with exit code 0 and **all target-window checksums enabled**. No checksum bypass was needed. The result is 134217728 bytes, CPUE Revision 1, SHA-256 `8ef9d4eeee81513f2d6e13c36125cdbfa4c6f498b039ef4b99faa7095d39cfa5`. The original was unchanged. The preparation script reproduced it directly from the build ZIP with the same three SHA-256 fingerprints.

Two real patched emulator processes ran this ROM for 35 seconds: 2075 frames each, another 1797/1796 frames after second 5, connected TCP roles 1/2, A input directed only to player 1, different intro states and both SIGTERM exit codes 0. Their private images were visually checked. The ROM wireless bridge remained inactive before receiving the Pokédex. **That initial test did not verify shared-world gameplay, in-game saving or a German port.**

The English result was delivered privately with instructions and a checksum. The ROM, ZIP and patch bytes are not published in the repository. The following sections retain the earlier failed German-base test.

## Prepare it on Bazzite

Extract your original US ROM from its ZIP first. Pass the build ZIP directly; no Windows program is needed. The known-release catalog supplies the verified base, patch and output checksums. German and other incorrect bases are rejected.

```bash
distrobox enter platinum-coop-build -- python3 scripts/prepare-rom.py \
  --rom /path/to/Pokemon-Platinum-USA-Rev1.nds \
  --patch '/path/to/Build V0.4.5.zip' \
  --output /path/outside-the-repo/Pokemon-Platinum-PM-0.4.5-USA.nds
```

The output is **English**. Existing files are never overwritten. Unknown German patches still require independently verified source, patch and target checksums; arguments cannot override known release fingerprints.

## Inspected files

- `PlatinumMultiplayerV0.4.5.xdelta`: 2519982 bytes, SHA-256 `70c0808f2400e3fb2cbaa0624364b0ac1ae9ace1177043cae8a74a5b616349a0`.
- `SETUP_GUIDE.md`: section 1 explicitly requires clean **Pokémon Platinum Rev 1 USA**. SHA-256 of the inspected guide: `81d3ecebeeddf88af6748661988b7b3b091c7742905d68484045d146e385498c`.
- The 0.4.5 changelog, privacy description and third-party licenses were read. The remaining package includes a Windows setup program with Python/Tcl/Tk and assets. No ROM-mod source or German patch was found.
- No Windows program was executed. Only the patch and text documents were extracted privately for inspection. The package, Nintendo ROM and saves are not committed.

The corresponding [No-Intro entry](https://github.com/libretro/libretro-database/blob/master/metadat/no-intro/Nintendo%20-%20Nintendo%20DS.dat) for USA Revision 1 has code `CPUE`, size 134217728 bytes, SHA-1 `0862ec35b24de5c7e2dcb88c9eea0873110d755c`, CRC32 `69D628E8`. This base was unavailable during the first test. The German file is `CPUD`, Revision 0, SHA-1 `d40f19ac24d67c721a4ab25e1c013154443d75b4`.

## Actual German-base patch attempt

Real xdelta3 3.0.11, normal decoding with its checks unchanged:

```bash
xdelta3 -d -s /path/to/your-german.nds \
  /path/to/PlatinumMultiplayerV0.4.5.xdelta /private-test-folder/attempt.nds
```

Result: exit code 1, `target window checksum mismatch: XD3_INVALID_INPUT`. No data block was successfully published; the empty output was removed. The original German ROM retained SHA-256 `9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217`. This was an actual compatibility test with the supplied files.

`xdelta3 printhdr` reports VCDIFF v0, LZMA secondary compression, source-copy windows and Adler32 target-window checks. Window 0 already references source data spanning 66763370 bytes. This is a delta against exact base bytes, rather than a replacement ROM containing a language-independent multiplayer program.

The supplied guide recommends disabling xdelta checksums for manual patching against its required US base. That does not make the German ROM compatible. Checks were retained throughout this test; no incorrectly reconstructed result was presented as a port. The later successful US-base test above established that no special checksum bypass was required for the supplied matching files.

## Preparation script

`config/project-pm-patches.json` maps this exact patch fingerprint to its base requirements. `scripts/prepare-rom.py --inspect` reads the single xdelta entry in a build ZIP without fully extracting it or running the installer. It reports base code, revision and checksum. A known patch with the wrong base exits with code 1; unknown patches are explicitly unverified (code 2). Inspection changes no file.

```bash
python3 scripts/prepare-rom.py --inspect \
  --rom /path/to/your-german.nds --patch '/path/to/Build V0.4.5.zip'
```

The real supplied files were checked this way: patch 0.4.5 and the German base were recognized, compatibility was rejected. Patch application also validates the known base requirement before decoding. The script supports the verified US 0.4.5 build; unknown patches remain restricted to verified CPUD input/output. It does not automatically port the US mod to German.

## Enable multiplayer in the game

Receive the **Pokédex** first. Then open **Bag → Key Items → Wireless Play → Start Wireless**. **CO-OP**, needed for shared trainer battles, is disabled by default and must be enabled in the game's options. Both players need the same build and matching randomized co-op settings. These requirements come from the supplied setup guide. Its two-window instructions use separate emulator directories and `127.0.0.1`; CouchSplit manages the two profiles and processes itself.

The launcher includes the Pokédex/Wireless Play step in its connection guidance. TCP connecting before ROM activation does not prove shared-world synchronization.

## German engine-port limitation

A reliable native CPUD port still needs ROM-mod source or a patch actually authored for CPUD. A CPUE binary delta is insufficient to verify every game-code hook and German resource mapping. No complete native German engine port was implemented.

The matching US base was supplied, patched and tested for intro/TCP/process operation. A US-based German text alpha was then implemented. The original German ROM is not replaced. Later shared-world and battle checks are documented in [Development status](development-status.md) and the separate [target](rom-target-cursor-fix.md) and [item](rom-coop-item-fix.md) correction reports.
