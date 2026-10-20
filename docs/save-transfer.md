# Transfer saves when changing ROMs

**English** · [Deutsch](de/save-transfer.md) · [Documentation](README.md)

The launcher separates saves by player and the complete ROM checksum. Renaming a byte-identical ROM keeps the same profile; a corrected ROM with different bytes gets a separate profile. Previous profiles remain intact.

These instructions describe an explicit copy from the verified German PM 0.4.5 ROM to its matching target-corrected or co-op-battle-corrected version. They do not promise compatibility between unrelated ROM mods or game versions.

1. Save normally inside both existing games. Stop the session cleanly. No worker may be running during the copy.
2. Select the **old ROM**. Under **Saves → Open profile folder**, copy each player's `saves/game.sav` to a safe, separate location. Keep the player assignments and retain the originals.
3. Select the **new ROM**, start a session once and stop it cleanly. This creates the new player profiles.
4. With the new ROM selected, open its profiles under **Saves → Open profile folder**. Place the corresponding copy at `saves/game.sav`: player 1 to player 1, player 2 to player 2. Back up any existing destination files separately first.
5. Restart and check both saves, balances and positions. If anything is wrong, select the old ROM to continue using the preserved old profiles.

**Restore backup** manages backups within the selected player/ROM profile. It does not automatically transfer a backup belonging to another ROM hash. Never point both players at the same active save file.

Installers do not replace your ROM with a corrected version. The [ROM tools](rom-coop-item-fix.md) do not alter saves or overwrite existing output files.
