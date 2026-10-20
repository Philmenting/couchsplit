# Platform packages

**English** · [Deutsch](de/platforms.md) · [Documentation](README.md)

## Bazzite

Run `CouchSplit-Bazzite.run` in desktop mode. Existing Distrobox/Podman is required. The installer creates its own Ubuntu runtime; see the [single-file installation guide](one-file-installer.md) for Steam registration and save locations.

## Zorin OS 18 / Ubuntu 24.04, x86_64

Download `CouchSplit-ZorinOS18-amd64.deb` and install it through the software manager, or run:

```bash
sudo apt install ./CouchSplit-ZorinOS18-amd64.deb
```

Open CouchSplit from the application menu. Choose English or Deutsch on first launch, then select your prepared ROM or ROM ZIP. No Distrobox container is used; APT resolves runtime dependencies. Zorin OS 17, based on Ubuntu 22.04, is not supported by this package.

Programs and source: `/opt/pokemon-platinum-local-coop`. Personal data: `$XDG_DATA_HOME/pokemon-platinum-local-coop`, normally `~/.local/share/pokemon-platinum-local-coop`. Removing the package does not remove personal data.

## Windows 10/11 x64

Run `CouchSplit-Windows-Setup.exe`. By default it installs in your local Programs folder without administrator privileges and creates Start menu/desktop shortcuts. Alternatively, fully extract `CouchSplit-Windows-Portable.zip` and open `Start.bat`. Do not launch it from inside the ZIP viewer.

Both real emulators and the Qt/SDL runtime libraries are included. An embedded CPython interpreter runs the verified ROM/ZIP import script; no separate Python installation is needed. Qt places personal data under `AppData/Local/pokemon-platinum-local-coop`. Uninstallation leaves ROMs and saves there.

Each player has a separate save. Normal exit requests cooperative IPC shutdown and waits for both workers. A Windows job object terminates remaining owned workers if the launcher crashes. Forced termination cannot recover unsaved progress.

## Updating from early releases

Update the launcher and emulator together. Since 0.2.0 Alpha 2 the launcher requires IPC ABI 2 and `STABLE_SAVE=1`. Within the existing player/ROM-hash profile, co-op saves use the fixed name `game.sav`, so renaming a ROM no longer hides progress. Before importing a uniquely identified old save, the launcher makes a backup and retains the source. If several old saves exist, select the originally used ROM; the launcher does not guess. Restoring old backups also uses the fixed save name. Different ROM hashes are not merged automatically.

Alpha 5 introduced the shared handheld/retro interface with pixel headings, readable system text, a clear start action and visible controller/keyboard focus. The font and its OFL license are included. ROMs, profiles, saves and controller assignments keep their existing locations. A launcher update does not install the separate ROM target correction.

## Language selection — Alpha 7 onward

**Language / Sprache** appears before ROM setup on the first interactive start, including after an update if the profile has no saved language. Choose English or Deutsch. The existing personal `settings.json` stores `language: en` or `language: de`. Cancel exits setup without saving a choice. A valid saved choice is used directly on subsequent starts.

Change language immediately under **Settings → Language / Einstellungen → Sprache**. Running sessions and independent saves remain intact. This translates launcher controls, help and owned errors; it does not change ROM language or raw operating-system/emulator logs. Native file dialogs may follow the system language. Corrupted settings are reported and not overwritten.

## Updating to CouchSplit — Alpha 8

The project is now CouchSplit. New downloads use `CouchSplit-…`; historical release assets retain their names. Install the new package over the existing installation. Legacy executable, profile and installer identifiers (`platinum-coop`, `pokemon-platinum-local-coop`, the Windows AppId/default folder and Bazzite runtime identity) remain for compatible upgrades. Existing settings, ROMs and saves stay in place; there is no automatic migration or merging. Launcher and application-menu names are CouchSplit. Existing Steam entries can retain the old name because their configuration is not directly rewritten.

## Steam and verification

Bazzite automatically requests a Steam entry. On Zorin OS and Windows, use Steam's **Add a Game → Add a Non-Steam Game** and select the launcher. Disable Steam Input for this entry if inputs are duplicated.

CI builds both operating-system families and runs native tests and ROM-free emulator smoke tests. Windows/Ubuntu checks install packages, start the installed launcher, upgrade from Alpha 7 and uninstall while retaining owned test data. Physical hardware and interactive target-system acceptance remain separate. Actual shared-world, wild-battle and trainer-battle evidence comes from Linux with virtual SDL controllers. A complete German playthrough and long sessions remain unverified.

[Alpha 6](https://github.com/Philmenting/couchsplit/releases/tag/v0.2.0-alpha.6) first bundled the earlier Alpha 2–5 improvements and authored ROM correction tools. Release publication requires native Linux/Windows and mandatory ARM checks; no game download is included. See [save transfer when changing ROM hashes](save-transfer.md).

References: [Zorin installation](https://help.zorin.com/docs/apps-games/install-apps/), [MSYS2 CI](https://www.msys2.org/docs/ci/), [Qt Windows deployment](https://doc.qt.io/qt-6/windows-deployment.html).
