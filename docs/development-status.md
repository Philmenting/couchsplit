# Development status

**English** · [Deutsch](de/development-status.md) · [Documentation](README.md)

Updated 21 October 2026. Current public release: **[CouchSplit 0.2.0 Alpha 8](https://github.com/Philmenting/couchsplit/releases/tag/v0.2.0-alpha.8)**, supporting Project PM / Pokémon Platinum. The local development build additionally supports GBA games and ROM hacks as independent split-screen sessions. Historical results below apply to the versions and environments named; they are not full-playthrough or target-hardware certification.

## Extended GBA ROMs: 23 October 2026

The local build supports explicit CouchSplit XROM v1 files up to 512 MiB, retaining the UI and native DS/GBA paths. Standard GBA remains limited to 32 MiB. A reproducible mGBA overlay adds a read-only data aperture, CPU/LDM reads, DMA sources and BIOS decompression. It does not create game content or implement extra story chapters. See [format and limits](extended-rom.md).

Verified locally: authored ARM fixtures at 64 and 512 MiB read actual data at the end of the file, CPU byte/halfword/word and unaligned/LDM accesses, DMA16/32, BIOS CpuSet/LZ77 to RAM/VRAM, two-worker input isolation, SRAM cold reload and clean shutdown. Native component/UI tests and all 60 Python tests pass (7 optional dependency/environment checks skipped in this run); existing SRAM/Flash/EEPROM production-worker checks pass. The separate private 64 MiB journey build relocates real Pokémon assets beyond 32 MiB; Kanto/Johto/Hoenn openings, shared PC and normal save checks pass. Full campaigns and Windows/Bazzite/hardware/audio remain unverified.

## General GBA extension: 21 October 2026

The existing GitHub menu layout is retained. GBA is selected automatically from a validated header, with raw/ZIP import, a 32 MiB limit, one 240×160 screen per player and appropriate input/help. No journey/bank menu or additional emulator settings row is added. Project PM / Platinum remains on its existing DS worker and IPC ABI 2. See [GBA setup and limits](firered.md).

Verified on Zorin OS 18.1 using local Qt 6.4.2, SDL 2.30 and pinned mGBA 0.10.5: native component/UI tests, GBA selection and German/English menu captures, virtual controller isolation/disconnect and GBA Select+R pass. All 59 Python tests pass with xdelta3 and ARM dependencies enabled. Authored ARM cartridge programs run on two production workers for SRAM 32 KiB, Flash 64/128 KiB and EEPROM 512 bytes/8 KiB; each writes progress, preserves per-player input, reloads identical battery bytes after restart and exits cleanly. A game with no battery creates no save; invalid save lengths fail while preserving existing bytes.

Actual German FireRed separately reaches the existing authored test hero's room through Continue, survives restart/reload and retains the previous save hashes. ROM bytes remain unchanged. This is separate from the earlier in-game-save creation evidence below. DS production transport, save path/config checks and two-worker authored DS execution also pass. No shared-world GBA/link multiplayer, travel, bank, physical-controller, audible-output, Windows/Bazzite or complete-playthrough claim is made. Generic acceptance is format support, not certification of every ROM hack. Public releases are unchanged.

## Earlier FireRed-only prototype: 21 October 2026

The original German FireRed is accepted by exact fingerprint and runs through a separate pinned mGBA 0.10.5 worker. The existing launcher provides two 240×160 views, controller assignment and separate per-player/per-ROM saves, locks and backups. No external GBA BIOS is required. See [setup and scope](firered.md).

Verified locally on Zorin OS 18.1 with Qt offscreen, dummy audio and virtual SDL controllers: real raw/ZIP imports retain the original hash; two actual GBA workers render German game screens; player-one input progresses only that adventure; an in-game save creates all 14 save sections and survives clean shutdown/reload through **Continue**. Player two retains its separate blank save. The private acceptance tool uses production Session/PlayView code. Native core/UI checks and all 58 Python tests pass with real xdelta3 and ARM checks enabled. Production DS transport and the two-worker DS homebrew smoke check still pass.

One deliberately premature test restart happened during the game's saving indicator and left the first save incomplete. Repeating the test after saving finished succeeded. Atomic host file replacement does not make an interrupted in-game save complete.

Shared FireRed world co-op, link battles/trading, the shared Pokémon bank and travel between editions are not implemented. Physical controllers, audible output, Windows/Bazzite GBA execution and a full playthrough remain unverified. Existing public Alpha 8 assets do not include this worker and are unchanged.

## Current evidence

| Area | Verified scope |
|---|---|
| Linux and Windows native packages | Builds, component/UI/controller/session tests, real homebrew emulator workers and installed-package checks in CI |
| Alpha 7 → CouchSplit upgrade | Actual published old Windows/Debian installers upgraded to Alpha 8, native launcher started, known Windows shortcuts replaced, owned test settings/save bytes preserved through upgrade and uninstall |
| Launcher languages | English/German first choice and persistence, cancellation, corrupt settings, write failure, immediate language switching during a test-worker session, compact through 4K layouts |
| Actual Pokémon world co-op | Linux with virtual SDL controllers: Pokédex/Wireless activation, both characters/followers, independent movement, shared wild/trainer battles and separate commands |
| Pokémon saving | Both players' normal in-game save/reload, positions, balances and victory flags in targeted checks |
| ROM target correction | Reproduced empty-target bug, 128 ARM states and real corrected-ROM shared wild-battle/save/reload test |
| ROM item correction | Reproduced one-sided HP sequence, 320 HP-import + 240 filter/ABI states, actual sequential/simultaneous Potion retests |
| Money | Shared victory over Sven: +96 ₽ each; balances 1410 → 1506 and 1460 → 1556 survived restart. A separate regular level-6 defeat deducted 48 ₽ each. |

The final Alpha 8 [native/release workflow](https://github.com/Philmenting/couchsplit/actions/runs/39480215694), [build/ROM-free workflow](https://github.com/Philmenting/couchsplit/actions/runs/39480215786) and [Linux/Windows ARM workflow](https://github.com/Philmenting/couchsplit/actions/runs/39480215738) passed. Tag/release commit: `07025b313eab58f7afdb1fb9f903219e5961c01d`. All six public assets were downloaded and checked against API digests and the published SHA256SUMS; all 126 tracked project files matched that release source in Windows, Bazzite and Debian packages. The Bazzite integrity check verified 911 files without installing on the host.

## Implemented components

- POSIX bridge adaptation: nonblocking sockets, select fd+1, EINPROGRESS, SO_ERROR and MSG_NOSIGNAL.
- Local host/client mode with independent `PMCOOP_ROLE`, no inherited autopilot, relay or broadcast; per-session TCP port.
- Independent process profiles/saves per player and ROM hash, using melonDS SaveManager for actual saving.
- Shared mapped frame/input interface, one Qt surface, side-by-side/stacked player layouts and 4:3 DS layouts: smaller touch, adjacent screens or switchable single-screen view.
- SDL instance assignment, automatic initial assignment only for exactly two/unambiguous remembered devices, explicit handling of ambiguity, button mapping, stick touch and disconnect protection.
- Controller/keyboard menu navigation, scalable retro interface, logs, settings and verified backup/restore.
- Session lock, SHA-256 backup manifests, atomic metadata, joint shutdown, failure handling, Linux parent-death behavior and native Windows mappings/job ownership.
- Distrobox developer/runtime wrappers, owned desktop entries and a Steam registration request through Bazzite's host helper without directly rewriting Steam configuration.
- Fingerprint-checked ROM preparation/localization/correction tools; no ROM, Nintendo dump or game download included.

## Open acceptance work

Physical USB/Bluetooth controllers, real Steam Input conflicts/reconnect behavior, native Windows/Bazzite Pokémon gameplay, KDE Wayland/Gamescope, Bazzite GUI/audio/device access, desktop/Gaming Mode switching, actual 4K/60 Hz TV performance and RTX 4060 thermal behavior remain target-system checks. Menu geometry/rendering at 4K does not prove TV output or game performance.

Trading, long gameplay sessions, gameplay disconnect/reconnect and a full German playthrough remain unverified. High-resolution DS 3D texture transport, rumble and a broad game library are not implemented. A native German CPUD engine port is not implemented; the German alpha uses the US engine. Use the [manual acceptance checklist](acceptance-tests.md).

## Historical baseline: 14 October 2026

Environment: Ubuntu 24.04 x86_64, GCC 13.3, CMake 3.28.3, Qt 6.4.2, SDL 2.30.0; development libraries supplied locally from Ubuntu packages. GUI checks used Qt offscreen and smoke audio was dummy. This was not a Bazzite environment.

The initial [GitHub run 38607299933](https://github.com/Philmenting/couchsplit/actions/runs/38607299933) passed in 4 minutes 43 seconds: 223 Project PM build steps including Qt/SDL and final link, launcher/tests, production TCP extraction, Python checks, launcher startup and two production homebrew workers. Component checks covered separate-process IPC/ABI/file boundaries, 4:3 geometry, profile locks, backup/restore and corruption rejection. Virtual SDL checks covered independent buttons/touch, toggle edges, disconnect and shifted device indices. Session checks used explicitly TEST-ONLY workers for three repeated starts, graceful stop, worker failure and lock release.

The real-worker homebrew check executes authored ARM code in both DS cores, publishes RAM frames, connects TCP roles and exits cleanly. It contains no Nintendo game/logo/BIOS/firmware bytes and does not establish ROM mailbox synchronization or Pokémon gameplay. The transport harness extracts production bridge code and checks roles/names, both directions, fragmented/invalid frames, reconnect, refused connection and SIGPIPE.

The supplied unmodified German ROM ran for 35 seconds, over 2000 frames per player, TCP roles 1/2 and directed player-1 input, then both exit codes 0. Its ROM bridge stayed inactive. [Full boot record](rom-boot-check.md). Offscreen launcher images at 1280 × 1000 and 3840 × 2160 were checked, and the patch was reversibly applicable to pinned source.

Original-ROM recognition also passed [run 38610385288](https://github.com/Philmenting/couchsplit/actions/runs/38610385288). Real xdelta3 3.0.11 synthetic preparation tests then passed seven Python cases and [run 38612252475](https://github.com/Philmenting/couchsplit/actions/runs/38612252475). Build ZIP inspection, ambiguous entries and Pokédex guidance increased the suite to nine cases. The supplied 0.4.5 patch rejected the German base. After the matching US base was supplied, known-release ZIP preparation reproduced its output twice with checks enabled and no override; ten Python tests passed. Two real US PM workers ran 35 seconds, 2075 frames each, connected roles and exited cleanly. Introduction behavior was tested; multiplayer activation/world gameplay was still open at that point. [Package requirements](project-pm-045.md).

## German text and graphics: 15 October

The text alpha implements standard-library Nitro/NARC/message processing, three-way merging, 857 authored translation fingerprints (1208 occurrences), original German text/font transfer, exact inputs and atomic non-overwriting output. PM program/overlay/script bytes stay unchanged; the regionally different unused Diamond/Pearl credits bank is preserved. Five new synthetic format/merge/control/byte-invariant tests brought the suite to 15 passing cases with real xdelta and no skips.

Two actual Alpha 1 builds produced SHA-256 `8b11f36e7bf4a15932eb364358fba183c2067ac8265cfbc97611ca53e3c90949`. A 40-second run produced 2369/2370 frames, another 2097 each after second 5, roles 1/2 and clean exits. The bridge remained inactive before the Pokédex. The German greeting “Schön, dich kennenzulernen!” was visually checked. A further 105-second player-1-input run produced 6272/6273 frames and clean exits; German instructions appeared in both views. Full story/menu widths/US grammar, save migration and Bazzite hardware were still unverified. YES/NO remained English at that stage.

Alpha 2 transfers the original German touch texture from its regional path to the US path. The other nine members (palette/eight animation maps), decoded size and PM code bytes stay unchanged. Two protection/transfer tests brought the suite to 17. Two builds matched SHA-256 `800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c`. A 110-second real-worker test yielded 6566/6571 frames, connected roles and clean exits; JA/NEIN was visible, and touching JA advanced to Professor Rowan/Eibe. This did not prove world co-op. [Localization details](german-localization.md).

## Installer and recovery work

The single-file installer passed 38 Python tests, native components and offscreen startup; real Alpha 2 ZIP import retained its hash. Six Steam tests use doubles for host-helper arguments, repeat protection, explicit retry and errors. Five regression checks cover live output, partial UTF-8, timeout/process groups, diagnostics and animated progress. A native widget test checks exit via mouse/virtual SDL, orderly test-worker shutdown and save-lock release.

User feedback on 15 October confirmed Bazzite installation/startup but identified unclear progress and an exit action that failed to close the launcher. Both were fixed and automatically checked; actual target confirmation remains separate. The narrow Podman missing-`merged/etc/passwd` restart recovery adds three tests for success, one-retry bounds and rejecting running/foreign/other-error containers. It has not established that cleanup fixes the reported Podman 5.8.4 storage issue. No ROM/save or foreign-container removal occurs. [Installer](one-file-installer.md) · [Troubleshooting](troubleshooting.md).

## Native platforms and Alpha 2 fixes

Native packages added Windows mappings, cooperative shutdown with IPC ABI 2, Unicode imports and job ownership, plus Debian/Windows installers and matching source. Native install/start/uninstall CI preserves personal data; it does not replace interactive hardware acceptance. The Windows local mode explicitly skips the upstream firewall helper, avoiding loopback firewall rules/admin prompts. Original Bazzite 0.1.0-alpha.1 assets remain immutable.

Alpha 2 introduced fixed co-op `game.sav` inside the existing profile. A uniquely identified legacy save is copied after backup, retaining its source; ambiguous saves require the originally used ROM. Older-backup restore uses the fixed name. `STABLE_SAVE=1` prevents mixing old workers; IPC stays ABI 2. Only co-op DS saves change, rather than ordinary emulator, GBA, state or cheat paths.

Scrollable menus follow focus and tolerate long filenames/controllers. Minimizing immediately neutralizes input/touch; hidden time does not advance the exit chord. On Zorin OS 18.1, 38 Python tests with xdelta, both native CTest targets and production transport/save-path checks passed, including migration, renamed ROMs, old backups, ambiguity protection, separate hashes/players and 360p through 4K.

A repeated real session with renamed homebrew exposed concurrent TOML serialization: getters created defaults while another thread saved. The extracted production `Config.cpp` test reproduced it. A shared recursive mutex now protects public table/array access and load/save. Concurrent getter/setter/array/serialization tests and two full real-worker sessions with renamed identical test ROMs/separate save markers passed. File operations in that harness are explicit local platform-test boundaries.

## Alpha 3 and Alpha 4: active input/session regressions

Alpha 3 corrected three real-worker/virtual-controller findings: hidden menus still accepted actions, selecting a new ROM changed the display during an old session, and remapped chord buttons leaked as DS input. Hidden menus now suppress actions and require release on return; ROM selection is blocked until normal session end; chords consume their physical buttons before DS mappings. Both native targets/38 Python tests passed. Regression coverage includes every DS mapping for both chords and unrelated Start/R inputs.

An isolated test with production widgets/session functions and two actual emulators also checked the three fixes, valid ROM B import/start after session A, disconnect without takeover, minimizing, worker failure and twelve full start/stop cycles. Owned save sentinels stayed identical; homebrew writes no Pokémon saves. Fresh offscreen and virtual-X11 repeats passed.

Alpha 4 fixed menu ownership after assignment and clipped 720p help. After successful assignment, only player 1 controls the menu; disconnect, shifted indices, another device or a failed reassignment cannot restore fallback. Explicit successful reassignment restores control. Before initial assignment setup navigation remains available. Two wrapped Qt labels reserve sufficient help height per player. Native tests covered both splits at 360p/720p/768p/1080p/4K, remaining image space, ownership/reconnect behavior and clean session exit. Both CTest targets and 38 Python tests passed; a real-worker homebrew integration test confirmed the behavior. Hardware/gameplay claims remained separate.

## Actual ROM co-op and target correction: 16 October

Alpha 4 was played to the Pokédex with the supplied German PM ROM. Characters/followers, independent movement, shared wild battles, separate commands, matching opponents, victory/experience/levels and normal saving/reloading were checked on Linux.

An empty target after the left opponent fainted was reproduced again. A separate exact-fingerprint correction updates cursor initialization in overlay 16; localization's program-byte invariants remain unchanged. All 46 Python tests, including 128 actual ARM instruction states, and both native targets passed. The corrected ROM selected the remaining Starly without extra direction input, accepted each player's A independently and completed victory/experience/level 7. A proper save/reload repeat preserved changed positions. An earlier test driver stopped before save completion and loaded its older checkpoint; this was not classified as a product defect. Sessions left no workers/locks.

Suspected permanent follower loss was withdrawn: followers reappeared while walking and remained visible after another battle with the unchanged ROM. No follower code change was justified. [Target correction and full limits](rom-target-cursor-fix.md).

## Alpha 5: retro launcher

The native Qt interface adopted embedded OFL-licensed Silkscreen headings, system body text, dark casing, light information panels, coral start and gold focus. Icons are original geometric drawings; no Nintendo font/art is used. At 1280 × 720 all actions are reachable without scrolling; narrow layouts stack/scroll with focus and 4K scales text/controls. Dialogs follow the same style. Player views use coral/cyan and complete wrapped help.

Both native targets and 46 Python tests with actual xdelta/ARM engine passed. Additional checks cover embedded font, label widths and Space activation. Batched X11 captures include 360p/720p/1080p/4K, empty/configured states and dialogs. Two real workers supply authored red/green ARM-homebrew frames for both splits and stop cleanly; these are not Pokémon evidence. Independent design review accepted the menu. The separate ROM correction is not silently installed by launcher updates.

## Item HP synchronization and money: 19 October

The existing Target Fix 1 ROM reproduced different HP after a trainer-battle Potion. The partner's extra script HP call advances only one network sequence. [Co-op Battle Fix 1](rom-coop-item-fix.md) filters precisely that context, retaining native healing, item exclusion and all other import checks. It writes 116 authored bytes in two overlay-122 regions, rather than money/saves. Nintendo ROM/code bytes are not published.

All 55 Python tests passed with real xdelta/Unicorn, including 320 HP-import, 240 filter/ABI and 128 cursor states. Independent assembler, saved assembly and public patcher matched exactly. Real Linux workers confirmed Absorb, sequential and simultaneous Potions with matching HP/status/PP/stat changes. Simultaneous healing was 42 → 62 and 40 → 60 before a hit, then 62/59 HP on both sides with sequence 1.

The targeted scenarios use separate copied saves with prepared level-25 teams. Money, inventory, story, victory flags and positions were not edited during preparation. The separate shared victory over Sven paid 96 ₽ each, retained the balance difference and survived saving/reloading with both victory flags. Own followers were visible immediately after victory before moving. Original ROM/saves remained unchanged; the Alpha 5 launcher needed no change for the ROM correction.

## Alpha 6, Alpha 7 and CouchSplit Alpha 8

Alpha 6 brought the earlier platform/launcher fixes and allowlisted authored ROM tools into a release gated on native Linux/Windows and ARM jobs. Draft uploads are checked for complete matching names/sizes; published releases are not overwritten, and matching partial drafts can resume. The English README/German summary uses actual captures with provenance and test limits.

Alpha 7 added English/German selection before setup, persistence in existing profiles and live Settings switching. ROM language, firmware names, profile hashes and workers remain independent. Tests cover both languages, first choice/cancel/restart, invalid language, corrupt JSON without overwriting, atomic-write failure and unknown-field preservation. Switching during TEST-ONLY worker execution retains the same session directory/lock. Compact settings scroll. Both native targets, all 57 Python tests with actual xdelta/Unicorn and two real homebrew workers passed; actual corrected-ROM ZIP import kept its SHA-256. Captures cover both splits from 640 × 360 through 4K. OS file dialogs/raw foreign logs may retain system/original language.

Alpha 8 renamed the project to CouchSplit. Visible launcher/dialog/installer/menu labels and new download names changed; current game support remains Project PM / Platinum. Existing profiles, hashes, save paths, controller assignments, runtime ownership and installer identities remain. Windows replaces only its known old application shortcuts. Native tests, 57 Python cases, homebrew workers, 23 EN/DE captures and private ROM ZIP hash checks passed. Actual Alpha-7-to-8 Windows/Debian upgrade/uninstall CI and public package verification completed on 20 October as recorded above. No new Pokémon gameplay or physical hardware certification was performed for the naming change.

## Repository history

The repository was first created privately through GitHub on 14 October with user approval. Owned source/test evidence was transferred through the connected GitHub app with matching Git trees; no upstream repository was modified. The original three local commits remain on `local-implementation`, while main follows published history. The repository is now public and named `Philmenting/couchsplit`.

`scripts/publish-github.sh` is only for initially publishing an independent copy with no existing origin; it is unnecessary for this repository:

```bash
./scripts/publish-github.sh            # private
./scripts/publish-github.sh --public   # public, if intended
```

It refuses uncommitted changes or an existing origin and neither requests nor stores a token. `AGENTS.md` records continuation rules.
