# Supplied original German ROM: boot test, 14 October 2026

**English** · [Deutsch](de/rom-boot-check.md) · [Documentation](README.md)

Historical report. The supplied file was extracted and tested only outside the repository. ROMs, private game images, firmware, saves and complete emulator logs are not published here. See [Development status](development-status.md) for subsequent patched-ROM/gameplay checks.

## Identity

- Nintendo DS code `CPUD`, Revision 0, 134217728 bytes.
- SHA-256 `9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217`.
- SHA-1 `d40f19ac24d67c721a4ab25e1c013154443d75b4`.
- CRC32 `37E9D9E5`.

SHA-1, CRC32, size and code match **Pokemon - Platin-Edition (Germany)** in [libretro's No-Intro metadata](https://github.com/libretro/libretro-database/blob/master/metadat/no-intro/Nintendo%20-%20Nintendo%20DS.dat). This is the unchanged original German ROM, rather than a Project PM multiplayer ROM. The launcher recognizes this exact original fingerprint and explains the missing patch during selection. Other fingerprints do not prove patch compatibility and still require confirmation when not recognized.

## Actual test

Environment: Ubuntu container, Qt offscreen, dummy audio, software 3D, free melonDS BIOS implementation and generated firmware. Two real patched Project PM processes, independent profiles/save paths and a local TCP port. Duration 35 seconds. Between seconds 10 and 20, only player 1 received A presses through production IPC. No physical controllers.

| Observation | Result |
|---|---|
| Published frames | Player 1: 2050; player 2: 2053 |
| Progress after second 5 | A further 1796 / 1797 frames |
| TCP | Both connected, roles 1/2 |
| ROM bridge active | False on both sides throughout |
| Different states | Player 1 showed introduction dialogue; player 2 showed the Pokémon intro; images visually checked |
| Shutdown | SIGTERM, both exit codes 0 |

This establishes ROM execution, ongoing frames, directed IPC input, loopback transport and orderly shutdown. It does not establish complete game compatibility, in-game save/load, Sinnoh-world synchronization, 4K performance or Bazzite operation. Without a ROM mod the original supplies no PM world data, regardless of TCP connecting.

## Repeat

This opt-in test needs your own file and a new output directory outside the repository. It produces private logs and DS screenshots, copies no ROM and starts no public-network connection.

```bash
python3 tests/verify_user_rom.py build/bin/melonDS-coop /path/to/your.nds \
  --output /path/outside-the-repo/private-boot-test --seconds 35
```

CI remains ROM-free. These input sequences check startup rather than providing a universal automated playthrough.

## Dependency identified at the time

The missing component was a ROM patch matching the emulator bridge, with instructions and the required base checksum. German-edition support was unconfirmed. Applying another region's patch blindly was not justified. ComicartOlie's [PM testing invitation](https://www.reddit.com/r/PokemonROMhacks/comments/1vlgnlw/project_platinum_multiplayer_looking_for_more/) pointed to developer Discord at https://discord.gg/W6MRVgcKtQ. The inspected emulator release contained no ROM patch; no publicly accessible verified patch had been found in that research.

The user later supplied PM 0.4.5 and its matching US base; see [package verification](project-pm-045.md) and [German localization](german-localization.md). Those later results do not turn the original unmodified CPUD ROM into a multiplayer ROM. Use the [acceptance checklist](acceptance-tests.md) for actual world testing.
