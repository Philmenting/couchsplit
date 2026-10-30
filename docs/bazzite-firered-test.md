# Bazzite: FireRed development test

**English** · [Deutsch](de/bazzite-firered-test.md) · [Documentation](README.md)

This is a source-build test, not the published Alpha 8 installer: Alpha 8 does
not include the experimental FireRed worker. Use desktop mode and your own
unmodified German FireRed ROM. Consult [the FireRed guide](firered.md) for the
experimental scope and remaining limitations. This procedure has not yet been
tested on a Bazzite machine.

Run the following in a terminal. The new source directory must not already
exist. This creates a dedicated Distrobox, clones the current GitHub source and
uses a separate data directory. Dependency installation uses `sudo` **inside
that container** and needs internet access.

```bash
distrobox create --name couchsplit-firered-dev --image registry.fedoraproject.org/fedora:44 --yes
distrobox enter couchsplit-firered-dev -- sudo dnf install -y gcc-c++ cmake ninja-build git pkgconf-pkg-config extra-cmake-modules SDL2-devel libarchive-devel enet-devel libzstd-devel faad2-devel qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtmultimedia-devel qt6-qtsvg-devel wayland-devel libcurl-devel libpcap-devel python3 xdelta
distrobox enter couchsplit-firered-dev -- git clone --branch main https://github.com/Philmenting/couchsplit.git "$HOME/CouchSplit-FireRed"
cd "$HOME/CouchSplit-FireRed"
distrobox enter couchsplit-firered-dev -- bash "$PWD/scripts/build.sh"
distrobox enter couchsplit-firered-dev -- env COUCHSPLIT_FIRERED_EXPERIMENT=1 "$PWD/build/bin/platinum-coop" --data-dir "$HOME/.local/share/couchsplit-firered-dev"
```

On later launches, use the final command from the same source directory. Select
the ROM and assign two controllers if available. Check startup, both views,
controller input, saving and restarting in desktop mode. The program and game
logs can help diagnose a failure, but do not share ROMs or save files. The source
repository contains neither. The normal installed launcher and its saves use a
different data directory.

In Settings, try the experimental audio choice. To test paired backups, both
players must save in-game, stop and start a new session; a pair is created at
that next start. Stop again before choosing **Saved games → Restore both players
together**. The backup contains the last in-game saves, not an instant savestate.
