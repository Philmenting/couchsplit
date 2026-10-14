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
- Bazzite host changes: owned desktop entry and wrapper only. Build/run dependencies in Distrobox. No root helper, uinput grabs, rpm-ostree layers, driver changes, KWin scripts or Steam config rewriting.

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
The supplied PM 0.4.5 xdelta requires CPUE USA Rev 1. Its checked German decode failed; the later supplied exact US base decoded successfully with all xdelta window checks active. Known fingerprints and CPUE input/output are recorded in config/project-pm-patches.json. Both --inspect and decoding accept the build ZIP without running its installer. Wrong known bases fail; no checksum overrides. Two real US-PM workers passed 35-second intro/TCP/lifecycle testing; no ROM-mod source, German localization or world-sync verification yet. ROM-side Wireless Play requires the Pokédex first (docs/project-pm-045.md); an inactive bridge during the introduction is expected.

## Coding rules
C++17, Qt6 >= 6.4, SDL2 >= 2.24; stdlib/Qt existing features before new dependencies. Use small modules already here. Validate file/process boundaries, report actionable German UI errors, use QSaveFile for owned metadata. No hardcoded home directories or shell concatenation of user paths. Keep IPC ABI synchronized with both processes. Do not use asserts as production validation. Test targets must keep asserts enabled in Release.

## Test requirements
After changing bridge code run the extracted PRODUCTION transport test, including reconnect/refused-connect/SIGPIPE. After input changes run virtual SDL isolation and disconnect tests. After saves changes check lock ownership, corruption rejection and preserved current saves. After lifecycle changes check repeated sessions, graceful stop and child failure. Check UI capture for clipping. Update docs/development-status.md with actual results and limitations.

## Distribution
Never commit Nintendo ROM, BIOS, firmware, saves, controller serials, personal profile paths or credentials. Build outputs/.cache ignored. Before shipping inspect archive listing. Source distribution must include this repo plus pinned upstream source if distributing emulator binaries. Desktop build artifacts here are not a verified portable AppImage; don't claim universal portability.
