# Single-file installation on Bazzite

**English** · [Deutsch](de/one-file-installer.md) · [Documentation](README.md)

`CouchSplit-Bazzite.run` contains both prebuilt Linux x86_64 programs and their complete corresponding GPL source. No ROM is included. You do not need a GitHub account, manual package installation or compilation to use it.

## Install and play

1. Download in Bazzite desktop mode. In Dolphin, right-click → Properties → Permissions → **Is executable**. Browsers usually do not preserve this Linux permission.
2. Double-click and start installation. You need internet access, existing Bazzite Distrobox/Podman and at least 3 GiB free space. Initial setup can take several minutes. An animated bar, current step and elapsed time indicate activity during long downloads.
3. Choose **English** or **Deutsch** at the first interactive launcher start, then select your prepared Project PM `.nds` or ROM ZIP. The ZIP is validated and exactly one contained DS ROM is imported. Verified PM 0.4.5 US/German-alpha fingerprints are recognized automatically; other mods require explicit confirmation.
4. Connect both controllers and choose **Start co-op game**. Initial assignment is automatic with exactly two controllers; change it through **Assign controllers**.

The ROM is copied with private permissions into your data folder; the downloaded copy can then be removed. Plain unpatched Platinum is rejected. The installer downloads no games and does not patch them automatically.

Later, open **CouchSplit** from the application menu. **Exit launcher** closes the application after stopping both emulators. Start + Select inside the game stops only the session and returns to the menu. The installer automatically requests a non-Steam entry for Gaming Mode. Sign in to Steam if needed and find it among non-Steam games. Disable Steam Input for this entry if inputs are duplicated or keyboard emulation interferes. External BIOS/firmware is unnecessary for the default free-BIOS direct boot.

## Steam registration

The installer invokes Bazzite's existing `steamos-add-to-steam` **on the host**, passing our `.desktop` entry. Steam handles registration for its signed-in account. A stable wrapper means the same Steam entry starts the updated program. Our code does not directly edit Steam files, controller settings or other games.

A lock and owned `steam-registration.json` prevent repeated automatic requests. The file confirms a **request**, rather than successful registration. Steam may already have received a timed-out request, so the installer does not silently repeat it. If the host helper is unavailable, the application remains installed and the completion message explains the fallback.

If the entry is missing, sign in to desktop Steam and check the library first. Then use the application-menu action to add the launcher to Steam again, or run:

```bash
~/.local/bin/pokemon-platinum-local-coop --add-to-steam
```

This explicitly repeats the request and can create a duplicate. Use it after changing Steam accounts or deliberately removing the entry as well. Manual **Add a Game → Add a Non-Steam Game** remains available. Automatic registration relies on Bazzite's native Steam integration; it does not configure a separately installed Flatpak Steam.

The source inspection on 15 October 2026 checked Bazzite's [KDE helper installation](https://github.com/ublue-os/bazzite/blob/main/spec_files/steamdeck-kde-presets/steamdeck-kde-presets-desktop.spec) and [GNOME equivalent](https://github.com/ublue-os/bazzite/blob/main/spec_files/steamdeck-gnome-presets/steamos-add-to-steam), which shows the Steam protocol and desktop-file support. The existing helper is invoked; no GPLv2 helper code is copied into this GPLv3 project. An actual Steam client was unavailable in the development environment.

## What installation creates

- An owned rootless Distrobox, `platinum-coop-runtime-v1`, running Ubuntu 24.04 to match the build environment. Bazzite remains the host system.
- Runtime libraries from signed Ubuntu repositories **inside that container**. `sudo` is used only there. Rootful Podman and foreign containers with the expected name are rejected. No rpm-ostree, root service, driver or NVIDIA configuration changes.
- Programs/source under `$XDG_DATA_HOME/pokemon-platinum-local-coop/software/releases/…`; `software/current` selects the active version.
- The owned `~/.local/bin/pokemon-platinum-local-coop` wrapper and application-menu entry under `$XDG_DATA_HOME/applications/`. The default XDG data root is `~/.local/share`.
- Existing settings and saves remain in their original data root. ROM imports go under `roms`, installation logs under `installer-logs`. Command output streams during execution, with an activity message every 15 seconds while waiting.

The wrapper uses container libraries and explicitly sets the data folder. It removes interfering Steam/host library paths only for the launched process. It uses XWayland when DISPLAY is available, otherwise Qt Wayland. Software rendering requires no NVIDIA integration inside the container.

Before activation, the installer checks emulator ABI and the Qt launcher with a temporary test profile. It switches the active version only on success. Initial container entry explicitly runs setup. If it fails or times out, diagnostics include status and the last 200 container log lines; the container is not automatically removed. Only the exact missing-`merged/etc/passwd` Podman restart error, on a verified stopped owned container, allows one cleanup of temporary mounts and one retry. Other errors, running containers and foreign containers do not trigger this recovery. Failures retain the previous active version and saves. Restarting installation can resume setup; earlier versions remain available for recovery.

To remove the application, first close both games. Remove the owned application-menu entry and wrapper, and optionally remove the owned runtime through Distrobox. Back up the data folder separately and delete it only if you intend to delete your saves. There is no automatic uninstall routine that might accidentally remove them.

## Verify or build a package

```bash
bash CouchSplit-Bazzite.run --check
```

This checks the full embedded package and every file fingerprint without installing. It is an integrity check, rather than a digital publisher signature. Archive paths, links, sizes and the file list are validated before executing the installer. No downloaded shell script is executed.

For developers, after a clean Ubuntu 24.04 build:

```bash
scripts/build.sh
python3 -m unittest discover -s tests -p 'test_*.py'
python3 scripts/make-installer.py --upstream .cache/project-pm --output /tmp/CouchSplit-Bazzite.run
bash /tmp/CouchSplit-Bazzite.run --check
```

Commit changes before packaging. The builder reconstructs pinned upstream with our patch and compares it with the source actually used. Rebuild both binaries after C++ changes. Packages omit `.git`, credentials, ROMs, firmware and saves. They include the project, complete patched upstream source and build instructions, without copying runtime libraries from the host.

References: [Distrobox on Bazzite](https://docs.bazzite.gg/Installing_and_Managing_Software/Distrobox/), [Distrobox Create](https://distrobox.it/usage/distrobox-create/), [Distrobox Enter](https://distrobox.it/usage/distrobox-enter/), [KDE dialogs](https://develop.kde.org/docs/administration/kdialog/), [adding non-Steam games](https://help.steampowered.com/en/faqs/view/4B8B-9697-2338-40EC).

## Verification record

The original installer work passed 38 Python tests, native rebuild/component checks and an offscreen launcher start. Tests cover self-extraction/tamper rejection, archive paths, initial/reinstallation, pre-activation failure, foreign-container protection and import validation. Six additional Steam tests cover special-character arguments, repeat protection, explicit retry, missing helper, failure/timeout, corrupt metadata, root rejection and dispatch before container entry. Those calls use test doubles; they do not establish actual Distrobox installation or Steam registration.

Five regression tests cover streamed output before completion, incomplete UTF-8, timeout output preservation/process-group cleanup, noninteractive commands, container diagnostics and animated KDE progress. A native UI test activates the real exit button with mouse/virtual SDL input, stops two test workers and releases the save lock. Test workers do not emulate a game. Three further tests restrict Podman cleanup to the exact error, owned stopped container and one retry; the external recovery remains tested only through doubles. The real German Alpha 2 ROM was privately imported from ZIP and checked against its complete SHA-256.

User feedback on 15 October: installation completed on Bazzite and the launcher started, but progress was unclear and exit failed to close it. Both code defects were corrected and automatically tested. Actual target confirmation of those fixes, physical controllers, Steam registration, Gaming Mode and 4K TV operation remain separate. Podman/Distrobox were unavailable in this development environment. Later native CI/package checks and Linux gameplay results are listed in [Development status](development-status.md); they do not establish Bazzite target-hardware acceptance.
