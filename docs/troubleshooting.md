# Troubleshooting

**English** · [Deutsch](de/troubleshooting.md) · [Documentation](README.md)

| Symptom | Check / action |
|---|---|
| Emulator missing / wrong interface | Re-run `scripts/build.sh`; Settings must point to `melonDS-coop`, rather than an unchanged fork or upstream binary. Check `--coop-capabilities`; current packages require IPC ABI 2 and `STABLE_SAVE=1`. Update both binaries together. |
| ROM rejected | Select your own prepared DS ROM with a Platinum CPU* game code and valid size, or a ZIP containing exactly one supported DS ROM. Plain unpatched Platinum is not multiplayer-compatible. |
| Original German Platinum detected | The US PM patch cannot be applied directly to the original German base. Use the verified US-based preparation/localization procedure. See [ROM requirements](project-pm-045.md) and [German localization](german-localization.md). |
| TCP connected, no second character | Check the ROM patch/bridge version and in-game activation. Receive the Pokédex, then enable Wireless Play. An unmodified ROM or a socket connection alone is insufficient. |
| No connection | Open both logs. Look for host-bind/socket errors or another process using the selected port. Stop cleanly and restart. No port forwarding is needed. |
| No picture for 15 seconds | Check for ROM/firmware errors, a blocking upstream dialog or an emulator hang. Inspect logs; if appropriate, test a complete set of your own dumps. |
| Duplicate controllers / both players move together | Disable Steam Input for this entry. Check visible SDL devices; do not assign both a physical device and its Steam virtual counterpart. |
| No input after Bluetooth disconnect | This protects against incorrect reassignment. Press Escape, open **Assign controllers**, explicitly select the reconnected instance, then **Start co-op game** to resume. |
| Touchscreen hidden | Switch the single-screen view with Select + R. The right stick moves the cursor; clicking the stick touches. |
| Saves seem missing | Check the ROM hash and player profile. Another patch version gets a separate profile. Open it through **Saves**. Never point both players at one save file. See [save transfer](save-transfer.md). |
| Profile locked | Close the other launcher session. Do not remove locks while workers are running. After a crash, QLockFile can clean a demonstrably stale lock. |
| Forced exit | Check backups. SIGKILL cannot guarantee pending writes; unsaved progress cannot be reconstructed. |
| Distrobox starts only in desktop mode | Test the manual container command, Wayland/audio sockets and device access, then follow the [Gaming Mode acceptance checks](acceptance-tests.md). |
| Soft-looking 4K output | DS resolution remains 256 × 192 per screen. The launcher scales without stretching; high-resolution OpenGL framebuffers are not transported. |

Session logs are under `<data-root>/logs/<session-id>/player1.log` and `player2.log`. ROM contents are not uploaded. Local mode uses no telemetry or online relay.

## Installation appears unchanged for a long time

The first entry into a new Ubuntu Distrobox installs basic packages and can take several minutes. Older installers also held back command output until completion, so a copied log could stop immediately after `distrobox enter` while installation continued. That alone does not establish a network fault or crash.

Use the current installer. It shows an animated activity bar, current step and elapsed time, and streams output to the installation log. If initial startup fails, it adds container status and recent log lines. Existing owned runtimes are reused; updating does not require deleting the container or data folder. Do not start another installation while one is running.

## Exit returns only to the menu

Older builds used an exit action that stopped only the session and appeared ineffective when the menu was already open. **Exit launcher** now closes the application after stopping both emulators, for both mouse and controller confirmation. Start + Select inside the game still stops only the session. To update an old build, close its window or use Alt + F4 before running the new installer.

## Podman restart: “creating temporary passwd file … merged/etc/passwd: no such file or directory”

This occurs in Podman before the application starts. A report with Podman 5.8.4 showed it on a previously initialized but stopped container. The message identifies a missing path in the mounted container filesystem; it does not establish whether a stale mount or another storage error caused it.

Only for that exact error, state `exited`/`stopped`, explicit `Running=false` and a revalidated ownership label, the installer tries `podman container cleanup platinum-coop-runtime-v1` once and then retries entry once. The [Podman documentation](https://docs.podman.io/en/stable/markdown/podman-container-cleanup.1.html) describes cleanup of stopped-container mount/network state; without `--rm` it does not remove the container. The installer uses no `--all`, prune, system reset, forced removal or data-folder deletion. This bounded recovery attempt has not been confirmed to fix that specific PC. If it fails again, provide the current log; the existing installation and data are retained.
