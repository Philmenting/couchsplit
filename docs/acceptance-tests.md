# Manual acceptance on the target PC

**English** · [Deutsch](de/acceptance-tests.md) · [Documentation](README.md)

For each test, record the date, build commit, Bazzite version, display backend, controllers, ROM SHA-256 and result. Do not attach ROMs or firmware to GitHub.

1. **Shared Sinnoh world:** run the matching Project PM ROM in both instances, choose different names/characters, enable ROM multiplayer and visit the same map. Both characters must see each other, move independently and change maps. Check NPC/object interactions and the hack's supported battles/trading separately. Observe changes; a screenshot alone does not prove synchronization.
2. **Saves:** make different progress with each player, save inside each game, stop both cleanly and restart. Neither save may overwrite the other. Restore a backup and check that only the selected player changes.
3. **Controllers:** test every DS button separately. Moving controller 1 must not affect player 2. Test both touch cursors, swap assignments, disconnect/reconnect Bluetooth. The second controller must not take over automatically. Compare Steam Input enabled/disabled and record the suitable setting.
4. **Layouts:** test 3840 × 2160 at 60 Hz, side by side (1920 × 2160 per player), stacked and every DS layout. Check 4:3 proportions, cursors, text and Select + R. Images must not stretch.
5. **KDE Wayland / Steam Gamescope:** test desktop, Gaming Mode and switching between them. Check fullscreen, focus, sound, 60 Hz output and controller access.
6. **Interruption:** stop a worker deliberately, lose/re-establish TCP, and exit through Steam. No processes should remain. After a forced termination, check saves and locks.
7. **Long sessions:** start with 30 minutes, then at least two hours of movement/battles/saving. Monitor CPU, memory, temperatures, sound, disconnects and frame rate. The project must not change NVIDIA driver or PCIe settings.
8. **Missing files:** remove/rename the emulator path, ROM and one external BIOS/firmware file at a time. Errors must be understandable, existing saves preserved and a subsequent corrected start successful.
9. **Repeated starts:** start/save/stop ten sessions; check backup assignments, separate profiles and process cleanup.

ROM-free tests cover only the components explicitly described in [Development status](development-status.md). They do not replace acceptance on target hardware.
