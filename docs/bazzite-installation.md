# Building on Bazzite

**English** · [Deutsch](de/bazzite-installation.md) · [Documentation](README.md)

For normal installation use the [single-file guide](one-file-installer.md): prebuilt programs, a private Ubuntu 24.04 runtime and first-run ROM/ZIP import. This page covers the optional developer build in a Fedora container.

## Requirements

Bazzite with its existing working NVIDIA drivers, Distrobox/Podman and two SDL-compatible USB/Bluetooth controllers. Supply your own Platinum ROM prepared with the Project PM patch matching the bridge. The project includes no games or Nintendo dumps and downloads none.

A normal Platinum save or ROM header does not establish multiplayer compatibility. See the [verified PM 0.4.5 requirements](project-pm-045.md); a matching patch source/version is required.

## Build and start

Clone or extract the source into a permanent directory in your home folder, then run:

```bash
cd couchsplit
./scripts/bazzite-build.sh
~/.local/bin/pokemon-platinum-local-coop
```

The build container is `platinum-coop-build`, based on Fedora 44. Development packages are installed with `sudo dnf` **inside the container only**. On the host, the script creates owned files in `~/.local/bin` and `$XDG_DATA_HOME/applications`. It does not use rpm-ostree or a root service, or alter NVIDIA, Gamescope or existing Steam configuration.

The wrapper runs the program inside the same Distrobox, avoiding assumptions about host Qt/SDL compatibility. Repeat `scripts/bazzite-build.sh` to rebuild. To remove the developer installation, remove its desktop entry/wrapper and optionally its container/project folder. Saves remain in the separate data root.

Initial setup in KDE desktop mode:

1. Connect both controllers and open **Assign controllers**. Live SDL instance IDs identify connected devices.
2. Use **Select ROM** to choose a prepared ROM and confirm its Project PM patch when requested.
3. Under **Settings**, check the patched `build/bin/melonDS-coop`. The build script places it beside the launcher.
4. Leave external BIOS/firmware fields empty for free-BIOS direct boot. A complete set of your own DS dumps is optional.
5. Choose **Screen layout**: side by side, large top screen with a smaller touchscreen.
6. Start and enable multiplayer inside the ROM when required. TCP status and ROM activation are reported separately.

Choose English or Deutsch at the first interactive launcher start; change it later in Settings.

## Steam Gaming Mode

The single-file installer requests a Steam entry through Bazzite's host helper. Sign in to desktop Steam if needed and look for **CouchSplit** under non-Steam games. If missing, right-click the launcher in the application menu and choose the action to add it to Steam again. Alternatively, use Steam's **Add a Game → Add a Non-Steam Game** and select the launcher entry. No global launch-option changes are needed. Disable Steam Input **for this entry** if Steam exposes duplicate physical/virtual controllers or emulates keyboard input.

Switch to Gaming Mode and start the entry. One Qt window contains both players; no foreign-window positioning or second Gamescope instance is required. Select 3840 × 2160 at 60 Hz in the existing Steam/TV settings. The application respects available window size.

Distrobox GUI/Wayland/XWayland, Bluetooth and Steam Gaming Mode have not been accepted on actual Bazzite target hardware in this development environment. If startup fails, first test `distrobox enter platinum-coop-build -- /full/path/build/bin/platinum-coop --fullscreen` from the desktop. If needed, test launcher-only XWayland with `QT_QPA_PLATFORM=xcb ~/.local/bin/pokemon-platinum-local-coop`; workers remain offscreen.

## Native development / CI

Ubuntu 24.04 dependencies and build:

```bash
sudo apt-get install -y --no-install-recommends g++ cmake ninja-build pkg-config extra-cmake-modules libsdl2-dev qt6-base-dev qt6-base-private-dev qt6-multimedia-dev qt6-svg-dev libarchive-dev libenet-dev libzstd-dev libcurl4-openssl-dev libfaad-dev libpcap-dev libegl-dev libwayland-dev xdelta3
./scripts/build.sh
./build/bin/platinum-coop
```

Fedora dependencies are listed in the build script. `COOP_BUILD_JOBS=4` limits parallelism; use 2 if memory is tight.

## Files and backups

Settings and player data are under `$XDG_DATA_HOME/pokemon-platinum-local-coop` or `~/.local/share/pokemon-platinum-local-coop`. `--data-dir /path` selects another root. Each ROM SHA-256 gets a profile per player. Backups are made before session start and are not deleted automatically. **Saves** opens profiles and restores verified backups while both emulators are stopped.

The application saves when you use the game's save function. Exiting alone does not create an in-game save. Save inside each game first; then hold Start + Select for two seconds to stop the session.
