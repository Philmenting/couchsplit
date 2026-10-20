# Technical feasibility and source inspection

**English** · [Deutsch](de/technical-feasibility.md) · [Documentation](README.md)

Historical review date: 14 October 2026. Statements describe the pinned revisions; reference-project HEADs may change. See [Development status](development-status.md) for later implementation and gameplay results.

| Project | Inspected commit | License / implementation |
|---|---|---|
| ComicartOlie/melonDS-Project-PM, platinum-mp | `4d9785468bbefc8d08002f16df4d85a12f9611f7` | GPL-3.0-or-later, melonDS 1.1, C++17, Qt5/6 + SDL2 |
| Spuds0588/mgba-splitscreen | `b66511a35873bda9c961872813e58b35543bcfc5` | MPL-2.0 for mGBA; C core, Rust/Tauri, HTML/Canvas, WebSocket frames |
| hikaps/couchplay | `b8c8e47502bb0eac3471acb896dc69b8290de8c8` | Source SPDX says GPL-3.0-or-later; Qt/KDE, Gamescope, privileged helper |
| melonDS-emu/melonDS | `906e9ebb27da8c6a715cd7abab4abfe8a8d29427` | GPL-3.0-or-later; CMake, Qt/SDL, existing per-instance frontend configuration |

## Project PM

The repository/branch were accessible. The pinned commit of 1 October 2026 removes Windows DLL clutter; branches are recorded in `upstream-branches.txt`. GitHub returned no open issues on the inspection date. That does not establish an absence of defects.

`src/frontend/qt_sdl/EmuThread.cpp` contains both **Scripted test autopilot** and the bridge. They are distinct. `MELONDS_AP=host|join` can automatically navigate the ROM with button presses as well as start transport. The launcher therefore uses `PMCOOP_ROLE`, never `MELONDS_AP`, and removes inherited autopilot variables before starting workers.

The bridge uses global Net/BridgeSt objects per process. Two independent DS processes fit that model. Two EmuInstances inside one process would share globals without further changes, so this project uses two processes.

### Linux blockers and fixes

Although the fork describes Linux builds, added bridge code used `SOCKET`, WSA functions, `DWORD`, `GetTickCount` and `GetComputerNameA` outside Windows guards. Linux lacked those definitions. POSIX `select(0, ...)` ignores all descriptors, whereas Winsock ignores its first parameter. Nonblocking POSIX connect returns EINPROGRESS rather than Winsock EWOULDBLOCK; sending to a closed peer can raise SIGPIPE.

`SocketCompat.h` adapts the API, uses fd+1, normalizes EINPROGRESS and suppresses SIGPIPE with MSG_NOSIGNAL. SO_ERROR checks whether a writable connect actually succeeded. Existing frame/mailbox logic remains. Our patch binds local hosting to loopback, accepts a per-session port, disables local discovery and exposes IPC status/frames/input.

### Connection paths

| Path | Protocol | Purpose |
|---|---|---|
| Native melonDS LAN | UDP 7064 / ENet | Original LAN session, separate from world co-op |
| Project PM bridge | TCP 7820 by default | Cross-platform ROM mailbox synchronization |
| Bridge discovery | UDP 7821 | PLATMP discovery; local launcher disables broadcast |
| Online relay | TCP 7833 by default | PMRELAY1; unused by this project |
| Local launcher | Random free TCP port on 127.0.0.1 | Same bridge frames, direct host/client, no native LAN lobby |

`startFromEnv()` and `BridgePump()` can start TCP independently of the native LAN lobby; ROM discovery is searched later. Transport can therefore be tested without a ROM. `tests/test_transport.py` extracts the **actual patched namespace mpnet** into a C++ harness, checking roles, names, bidirectional frames, fragmentation, invalid lengths, reconnect, refused connection and SIGPIPE. It tests the production transport.

### ROM dependency

The inspected emulator source contains no suitable ROM patch. GitHub account research found two emulator ports but no clearly matching public patch repository. The emulator README calls the hack Project PM; that alone did not verify a current matching patch source/release.

The user subsequently supplied Build 0.4.5, whose setup requires USA Revision 1. Real xdelta rejected the German original. The matching US base was then supplied, patched successfully and boot-tested in two emulators; see [package verification](project-pm-045.md). `scripts/prepare-rom.py` supports ZIP/base inspection and reproducible US patch preparation. ROM-mod source was absent. German localization and world acceptance were still open at that stage and were investigated later.

The mailbox bridge uses Wire v2 (comment: eight players, ROM protocol v18), discovery magics `0xCAFE1234 / 0x5678CAFE` and control magic `BRG1`. The supplied ROM must actually publish that interface. Header CPU*, file size, SHA-256 and explicit confirmation can validate selection boundaries, but cannot prove arbitrary patch compatibility.

At that initial review the main acceptance criterion was open: both different characters must see each other in one Sinnoh world, move independently and save separately. A transport test cannot replace that gameplay check. Later targeted evidence is documented in [Development status](development-status.md).

## Architecture references

mGBA SplitScreen demonstrates multiple core frame outputs in one surface, link synchronization and gamepad assignment. Its GBA core cannot run DS ROMs. The browser/Tauri stack is not used here because melonDS already has Qt and RAM framebuffers. No reference-project code was copied.

CouchPlay isolates complex external games with separate users, device management and Gamescope processes. `GamescopeInstance.cpp`, `WindowManager.cpp` and `SessionRunner.cpp` show nested Gamescope and KWin/D-Bus positioning; installation needs a privileged helper. Two owned emulator processes need only independent profiles and direct SDL assignment, so this project introduces neither root helpers nor temporary system accounts. No code was copied.

Upstream melonDS supports CMake, Qt6, SDL2, `portable` configuration beside the binary and XDG paths. Explicit `PMCOOP_PROFILE` takes priority so workers reliably use different directories. Existing SaveManager/framebuffer functionality is reused.
