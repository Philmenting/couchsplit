# Project: Pokémon Platinum Local Co-op

## Goal and evidence
Build a working Linux couch launcher for TWO separate Project PM processes. A successful TCP link or synthetic mailbox test is NOT evidence of shared Sinnoh gameplay. Never claim that commercial-ROM gameplay, controller hardware, Bazzite, KDE Wayland, Gamescope or hours of play were tested unless they actually were.

## Architecture decisions
- GPL-3.0-or-later project; preserve melonDS/Project PM attribution and full upstream license.
- Pinned upstream commit in config/upstream.lock.json. Apply our patch only to the owned .cache checkout. Never push to someone else's upstream.
- Use existing melonDS Qt/SDL frontend/core, free BIOS, generated firmware and SaveManager. No replacement emulator.
- Each worker has its own PMCOOP_PROFILE, game-hash directory, save files, firmware config and TCP role. PMCOOP_ROLE is deliberately distinct from MELONDS_AP (the latter drives scripted input).
- Direct loopback TCP bridge, unchanged wire v2. Local mode disables discovery broadcast and never uses an online relay. Do not infer native Nintendo wireless behavior from this bridge.
- One Qt window paints both workers' RAM framebuffers via Linux mmap IPC. This is an image frontend, not a new Wayland compositor. Workers use Qt offscreen and software 3D rendering; keep this baseline until actual performance tests justify GL transport.
- SDL GameController input exists ONLY in the launcher. Store transient instance IDs for live assignment, GUID/serial only as setup hints. No fallback to index 0 after hotplug. Neutral input on disconnected/hidden view.
- Profile lock held until BOTH children exit; SHA-256 manifests for backups. Never restart a crashed game automatically. Report failures and let the player review the save.
- Qt QProcess sends SIGTERM; Qt-loop signal polling permits SaveManager teardown. Parent-death signal protects against orphan workers on Linux. SIGKILL cannot guarantee pending save data.
- Bazzite host changes: owned desktop entry, wrapper and explicitly requested own Steam-library registration only. Build/run dependencies in Distrobox. No root helper, uinput grabs, rpm-ostree layers, driver changes, KWin scripts or direct Steam config rewriting. Invoke the host steamos-add-to-steam helper for our desktop entry; never edit shortcuts.vdf or other games.

## Build and checks
On Ubuntu 24.04 / supported Fedora container install dependencies from docs/bazzite-installation.md.
```
scripts/build.sh
cmake --build build/launcher --parallel 4
ctest --test-dir build/launcher --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tests/test_transport.py --source .cache/project-pm
QT_QPA_PLATFORM=offscreen SDL_AUDIODRIVER=dummy build/bin/platinum-coop --data-dir /tmp/coop-check --smoke-test /tmp/coop-check.png
python3 tests/smoke_emulator.py build/bin/melonDS-coop
```
No commercial ROM is needed for these checks. worker_fixture.cpp is TEST ONLY; never substitute it for the emulator in a release.
Install xdelta3 (Fedora package xdelta) to run tests/test_prepare_rom.py against the real decoder. scripts/prepare-rom.py uses verified source/patch/target SHA-256 values automatically for known releases; unknown German patches require all three hashes and CPUD inputs/outputs. Never disable these checks to force an English patch onto German bytes. A successful decode or CPUD header is not proof of German text or world co-op. See docs/german-compatibility.md.
The supplied PM 0.4.5 xdelta requires CPUE USA Rev 1. Its checked German decode failed; the later supplied exact US base decoded successfully with all xdelta window checks active. Known fingerprints and CPUE input/output are recorded in config/project-pm-patches.json. Both --inspect and decoding accept the build ZIP without running its installer. Wrong known bases fail; no checksum overrides. Two real US-PM workers passed 35-second intro/TCP/lifecycle testing; no ROM-mod source or world-sync verification yet. German US-based text alpha is now implemented (docs/german-localization.md); it is not a native CPUD engine port. ROM-side Wireless Play requires the Pokédex first (docs/project-pm-045.md); an inactive bridge during the introduction is expected.

## Coding rules
C++17, Qt6 >= 6.4, SDL2 >= 2.24; stdlib/Qt existing features before new dependencies. Use small modules already here. Validate file/process boundaries, report actionable UI errors in the selected English/German launcher language, use QSaveFile for owned metadata. No hardcoded home directories or shell concatenation of user paths. Keep IPC ABI synchronized with both processes. Do not use asserts as production validation. Test targets must keep asserts enabled in Release.

## Test requirements
After changing bridge code run the extracted PRODUCTION transport test, including reconnect/refused-connect/SIGPIPE. After input changes run virtual SDL isolation and disconnect tests. After saves changes check lock ownership, corruption rejection and preserved current saves. After lifecycle changes check repeated sessions, graceful stop and child failure. Check UI capture for clipping. Update docs/development-status.md with actual results and limitations.

## Distribution
Never commit Nintendo ROM, BIOS, firmware, saves, controller serials, personal profile paths or credentials. Build outputs/.cache ignored. Before shipping inspect archive listing. Source distribution must include this repo plus pinned upstream source if distributing emulator binaries. Desktop build artifacts here are not a verified portable AppImage; don't claim universal portability.

## German localization
`python3 scripts/localize-pm.py --us ORIGINAL_US.nds --german ORIGINAL_DE.nds --pm PM_US.nds --output OUTSIDE_REPO.nds` requires the three exact pinned input hashes. Preserve all PM executable/script/overlay bytes and message IDs. Never force the US xdelta onto CPUD. Three-way localization changes five named text/font/button archives, appends new resources and updates FAT/header fields. Fail on unresolved PM messages or lost/reordered control tokens. Compressed names use 9-bit codes in 15-bit words. Preserve legacy msg.narc bank 493 credits: German inserts shift those IDs. Keep original text dumps and ROM screenshots outside git; dictionary contains only source hashes, own German translations and explicit neutral hashes. Run tests/test_localization.py plus complete Python suite with xdelta3. Verify actual ROM byte invariants, reproducible hashes and emulator intro before delivering. Entry coverage is not a full-playthrough or world-sync test. New ROM hashes create separate save directories; never silently migrate saves.

Alpha 2 imports original German JA/NEIN texture from resource/ger/touch_subwindow/touch_subwindow.narc under US path graphic/touch_subwindow.narc. Regional filenames differ: compare actual paths, not just their intersection. Require PM==US and ten members, unchanged palette/eight animation maps, equal LZ10 decoded texture size. No authored sprite replacement or bundled Nintendo bytes. tests/test_localization.py covers transfer and rejection. Other localized graphics are not implicitly approved by this one proven mapping.

## Single-file installer
Build both binaries on Ubuntu 24.04 x86_64, commit source, then `python3 scripts/make-installer.py --upstream .cache/project-pm --output /tmp/Pokemon-Platinum-Coop-Bazzite.run`; `bash /tmp/Pokemon-Platinum-Coop-Bazzite.run --check`. Bundle complete matching patched upstream source and all owned source, never .git or protected files. Rebuild/copy build/bin after C++ edits before packaging. Runtime is a separate rootless Ubuntu 24.04 Distrobox with an ownership label; apt/sudo only inside it, no host packages, NVIDIA import or direct Steam config editing. Register our own desktop entry through Bazzite's host helper, with a locked request marker to prevent repeated automatic requests. A helper exit code is not proof of a visible Steam entry; keep explicit retry and honest status. Probe ABI/UI before switching software/current. Keep previous releases and all saves. Test doubles verify installer boundary behavior, never describe them as a real Podman/Bazzite test. Run complete Python suite, native tests, actual ROM ZIP import hash and native GUI smoke. Every interactive first launch asks for English/German when no valid choice is saved. Launcher --first-run then opens ROM selection; known exact PM fingerprints avoid redundant confirmation. Import helper snapshots one ROM from raw/ZIP with size/path/corruption protections; no game downloads. Follow docs/one-file-installer.md. Installer preserves existing custom emulator settings; default bundled path is not persisted across versioned releases.

## Installer progress / launcher exit regression
Installer commands must stream and flush output before completion and preserve partial output on timeout. Use a bounded first-enter probe and owned-container diagnostic logs; never infer a network fault from an incomplete log. No automatic container deletion. Show an animated indeterminate indicator with real stage/elapsed time; do not invent download percentages. Keep Steam-client processes out of command-group timeout killing. The launcher's exit button must CLOSE the app via closeEvent (which stops both workers), while the in-game exit chord ends only the session. Run ctest including launcher-ui: production widget, mouse events, virtual SDL menu navigation and active TEST-ONLY fixture cleanup. Rebuild and copy build/bin/platinum-coop before packaging any C++ fix. User reports of target execution must be attributed; they do not replace hardware confirmation of subsequent fixes.

Podman restart recovery: only the exact missing merged/etc/passwd + temporary-passwd error, exited/stopped with Running=false, and revalidated runtime-v1 ownership allow one named `podman container cleanup` and one retry. Never use --rm/--all, prune/reset, remove containers or touch saved games. Generic errors and live/foreign containers must not trigger it. Test doubles prove guards, not that cleanup fixes the user's Podman storage.

## Native platforms
IPC ABI is now 2; rebuild both workers and launcher together. prepare-platform.py adds verified cooperative shutdown/Unicode support after the pinned base patch and must also run in source-package reconstruction. Windows uses native file mapping and a kill-on-close job; do not substitute TerminateProcess for normal saving. Linux preserves parent-death/SIGTERM handling. Zorin package targets Ubuntu 24.04/ZorinOS 18 only. Windows packages must deploy and verify all runtime DLLs, preserve dynamic LGPL replacement and include dependency licenses/source directions. Release only after native platform jobs pass; GUI/hardware coverage remains separate. Never update or overwrite the v0.1.0-alpha.1 assets.


## Launcher languages
User-selected `de`/`en` is independent of ROM/firmware language. Keep both in the built-in Qt catalog; no Linguist runtime dependency. First choice must preserve existing settings fields, invalid/missing choices prompt once, cancel writes nothing, and persistence failures must not change the active language. Settings retranslation must not restart workers, alter profile/ROM hashes, controller IDs or firmware usernames. Test both languages at compact/desktop/4K sizes and retain a native scrolling settings form for wrapped guidance.
