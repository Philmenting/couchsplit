<div align="center">

# Pokémon Platinum Local Co-op

**Two players. Two controllers. One shared screen.**

A native retro launcher for local Project PM co-op on Windows and Linux.

[![Release](https://img.shields.io/badge/Release-0.2.0%20Alpha%207-b83f35?style=flat-square)](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/tag/v0.2.0-alpha.7)
[![Native builds](https://github.com/Philmenting/pokemon-platinum-local-coop/actions/workflows/platforms.yml/badge.svg?branch=main)](https://github.com/Philmenting/pokemon-platinum-local-coop/actions/workflows/platforms.yml)
[![License](https://img.shields.io/badge/License-GPL--3.0--or--later-325666?style=flat-square)](LICENSE)

[Download](#download) · [Get started](#get-started) · [Screenshots](#play-together) · [Test status](#what-has-been-tested) · [Deutsch](#deutsch)

![The actual retro launcher, with a clear setup menu and a prominent co-op start button.](docs/images/launcher.png)

</div>

## Download

**[Get 0.2.0 Alpha 7](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/tag/v0.2.0-alpha.7)** — English/German language selection, the retro interface, independent saves and separate tools for the verified ROM battle bugs.

| Your system | Download | Start here |
|---|---|---|
| **Windows 10/11 · x64** | [Installer (.exe)](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/Pokemon-Platinum-Coop-Windows-Setup.exe) · [Portable ZIP](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/Pokemon-Platinum-Coop-Windows-Portable.zip) | Install, or fully extract the ZIP and open `Start.bat`. |
| **Bazzite · x86_64** | [Installer (.run)](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/Pokemon-Platinum-Coop-Bazzite.run) | Run from desktop mode after marking it executable. Uses its own Distrobox runtime. |
| **Zorin OS 18 / Ubuntu 24.04 · x86_64** | [Debian package (.deb)](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/Pokemon-Platinum-Coop-ZorinOS18-amd64.deb) | Install through your software manager. |

[Checksums](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/SHA256SUMS) · [ROM correction tools](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/Pokemon-PM-ROM-Korrekturen.zip) · [Release notes](docs/releases/v0.2.0-alpha.7.md) · [Platform guide](docs/platforms.md)

You need **two controllers and your own compatible Project-PM-patched Platinum ROM**. No games, Nintendo BIOS, proprietary firmware or user saves are included. The launcher does not add world co-op to an unmodified Platinum ROM. All packages are alpha releases. Choose **English or Deutsch** at first launch; change it later in **Settings / Einstellungen**. This does not change the language of your game ROM.

## Play together

Two separate Project PM/melonDS processes share one window. Each player controls their own game and keeps their own save. The launcher handles controller assignment, screen layout, the local connection and orderly shutdown.

![Two real emulator processes in a shared trainer battle, each with an independent battle menu.](docs/images/shared-battle.png)

*An actual trainer battle on Linux with virtual SDL controllers. This targeted healing test uses separate copied saves with a prepared level-25 team. It is not a mockup or evidence of physical-controller or Windows gameplay testing.*

![Both players and their followers on Route 202 immediately after their shared victory.](docs/images/shared-world.png)

*Two views of the same route, directly after victory and before any movement. Both players' own followers are visible. [Screenshot origins and test context](docs/images/README.md).*

### Built for a shared screen

- **One window, multiple layouts:** players side by side or stacked; large top screen with a smaller touch screen, adjacent DS screens, or a switchable single-screen view.
- **Independent input:** explicit controller assignment, remappable DS buttons and right-stick touch control. Game inputs are neutral while the launcher menu is open.
- **Separate progress:** player/ROM-specific profiles, backups before each start and verified backup restoration.
- **Local connection:** a direct loopback host/client bridge, with no public relay or port forwarding.
- **English and German:** choose at first launch, switch later in Settings without restarting a running session.
- **A readable retro menu:** pixel headings, native body text, visible keyboard/controller focus and layouts that scale from small windows to 4K.

## Get started

![The bilingual first-launch choice offers Deutsch and English.](docs/images/language-picker.png)

1. **Install your platform package and choose your language.** At first launch, select **English** or **Deutsch**. Use desktop mode for initial setup. Bazzite needs Distrobox/Podman, internet access and about 3 GiB of free space for its runtime.
2. **Select a prepared ROM.** Use **Select ROM / ROM auswählen** to add a compatible `.nds` or ROM ZIP. [PM 0.4.5 requirements](docs/project-pm-045.md) · [German localization](docs/german-localization.md).
3. **Assign two controllers.** Open **Assign controllers / Controller zuordnen**, assign players 1/2 and choose your screen layout under **Screen layout / Bildschirmaufteilung**.
4. **Enable co-op in the game.** Select **Start co-op game / Koop-Spiel starten**. PM 0.4.5 unlocks Wireless Play after receiving the Pokédex; shared trainer battles also require the in-game co-op setting.
5. **Save normally.** Each player saves their own progress in the game. Then end the session together.

| Input | Action |
|---|---|
| Left stick / D-pad | Move |
| Right stick + stick click | Move and press the touch cursor |
| Select + R | Switch the single-screen view |
| Start + Select · hold 2 seconds | Stop both emulators cleanly |
| Escape | Open launcher menu; **Start co-op game / Koop-Spiel starten** resumes the session |
| Tab / Space | Navigate and activate launcher menu actions |

If Steam produces duplicate inputs, disable Steam Input **for this game entry**. Bazzite requests its Steam entry through the host helper; the application-menu action can retry if it is missing. [Bazzite setup](docs/one-file-installer.md) · [Troubleshooting](docs/troubleshooting.md). Detailed platform documents are currently in German.

### ROM corrections and existing saves

Separate tools correct two reproduced bugs in the **exact verified German PM 0.4.5 ROM**: [target selection after an opponent faints](docs/rom-target-cursor-fix.md), followed by [HP synchronization after Potions](docs/rom-coop-item-fix.md). They check the input, overlay and result against fixed fingerprints and create a new ROM copy. Updating the launcher does not automatically apply a ROM correction.

**A new ROM fingerprint gets separate save directories.** Previous profiles remain intact and are not automatically merged. Back up both players' saves before switching. [Explicit save-transfer instructions](docs/save-transfer.md).

## What has been tested?

| Area | Evidence |
|---|---|
| Linux and Windows | Native builds, controller/session/UI tests, two real emulator processes and package/installation checks in [GitHub Actions](https://github.com/Philmenting/pokemon-platinum-local-coop/actions/workflows/platforms.yml). ROM-free smoke tests use our own homebrew. |
| Language selection | Native UI tests cover both languages, first launch, saved choice, cancellation, existing profiles, write failures and switching during a running fixture session; compact windows through 4K. |
| Actual Pokémon co-op | On Linux: shared-world views, movement, wild/trainer battles and independent commands. Virtual SDL controllers; no physical-hardware certification. |
| ROM battle corrections | Actual before/after gameplay checks, plus ARM instruction tests on Linux and Windows. [Technical results](docs/verification/coop-item-sync-2026-10-19.json). |
| Money and saving | In the tested shared victory over Sven, **each player receives 96 ₽**. Both balances and victory flags survive normal saving and a fresh launch. |

<details>
<summary>See both trainer cards after restarting</summary>

![Independent balances of 1506 and 1556 Pokédollars after saving and restarting.](docs/images/separate-saves.png)

+96 ₽ each: 1,410 → 1,506 and 1,460 → 1,556. The targeted victory test uses prepared level-25 team copies; wallet, inventory, story and trainer-victory flags were not edited during that preparation. The measured reward applies to this battle.

</details>

Still pending: physical controllers, actual Windows/Bazzite Pokémon gameplay, KDE Wayland/Gamescope on target hardware, trading, a complete German playthrough and long sessions. [Development status](docs/development-status.md) · [Manual acceptance tests](docs/acceptance-tests.md).

## Development and documentation

C++17, CMake, Qt 6 and SDL2; see the [build guide](docs/bazzite-installation.md) for dependencies. The script checks out the pinned upstream and applies our patches.

```bash
scripts/build.sh
python3 -m unittest discover -s tests -p 'test_*.py'
```

For all ARM tests, also install `tests/requirements-romfix.txt`; decoder tests require `xdelta3`. Output: `build/bin/platinum-coop` and `build/bin/melonDS-coop`. There is no verified universal AppImage; Bazzite uses its own runtime. Emulator packages include matching GPL source, including the pinned, patched upstream.

[Architecture](docs/architecture.md) · [Platforms](docs/platforms.md) · [ROM preparation](docs/project-pm-045.md) · [Localization](docs/german-localization.md) · [Report a bug](https://github.com/Philmenting/pokemon-platinum-local-coop/issues/new)

## Deutsch

Zwei Spieler, zwei Controller, ein gemeinsames Fenster: Der native Retro-Launcher startet zwei getrennte Project-PM/melonDS-Prozesse für Windows, Bazzite und ZorinOS 18 / Ubuntu 24.04. **[Alpha 7 herunterladen](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/tag/v0.2.0-alpha.7)**, eine eigene kompatible Project-PM-ROM auswählen und beide Controller zuordnen. Beim ersten Start zwischen **Deutsch und Englisch** wählen; später unter **Einstellungen → Sprache** ändern. Die Auswahl wird gespeichert und verändert die Spiel-ROM nicht. Die ausführlichen Anleitungen bleiben auf Deutsch.

Welt-Koop und gemeinsame Kämpfe wurden unter Linux mit virtuellen SDL-Controllern geprüft. Windows-/Linux-CI prüft Builds, native Tests und Pakete. Gezielte Heilungs-/Siegszenarien nutzen vorbereitete Level-25-Spielstandkopien. Physische Hardware und ein vollständiger Durchlauf bleiben offen. ROM-Korrekturen sind separate, hashgebundene Werkzeuge; ein neuer ROM-Hash verwendet eigene Spielstandordner. [Spielstände übernehmen](docs/save-transfer.md) · [Einrichtung](docs/platforms.md).

## Credits and license

Our launcher and changes: **[GPL-3.0-or-later](LICENSE)**. [Project PM / platinum-mp](https://github.com/ComicartOlie/melonDS-Project-PM/tree/platinum-mp) provides the emulator core, ROM mailbox bridge and SaveManager, based on [melonDS](https://github.com/melonDS-emu/melonDS). Silkscreen uses the [SIL Open Font License](assets/fonts/OFL.txt). mGBA SplitScreen and CouchPlay were architectural references; no code was taken from them.

An unofficial project, not affiliated with Nintendo, Game Freak or The Pokémon Company. Game screenshots document the tests; game-content rights remain with their respective owners.
