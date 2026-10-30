# GBA game support

**English** · [Deutsch](de/firered.md) · [Documentation](README.md)

The current development source can run GBA games and ROM hacks in the existing CouchSplit interface. Select a `.gba` file or ZIP containing exactly one game. The launcher checks the GBA fixed header byte, header checksum and a size between 512 bytes and 32 MiB for standard cartridges. Explicitly marked CouchSplit XROM v1 files can be up to 512 MiB. A valid header identifies the file format; it does not certify that every game or hack works. Oversized and malformed files are rejected. Existing Project PM / Platinum support and the DS worker are retained; other DS games are not enabled by this change.

## FireRed co-op development target — 28 October 2026

The development source includes an **experimental paired FireRed worker**: two real
mGBA cores with separate saves, a partner overlay on the same or directly connected outdoor maps, and shared native
battles with each human choosing their own actions. This remains an unfinished
implementation of the requested shared-world co-op. Public Alpha 8 is unchanged.
It is restricted to the exact unmodified German original; generic GBA games,
other languages and ROM hacks do not acquire co-op through this option.

Build the launcher and both workers together, then explicitly opt in. The
build script places the matching executables together in `build/bin`:

```sh
scripts/build.sh
COUCHSPLIT_FIRERED_EXPERIMENT=1 build/bin/platinum-coop
```

The selected worker must advertise `FIRERED_PAIR_EXPERIMENT=1`. One process owns
both emulator cores and both existing IPC mappings; controller polling remains
in the launcher. Normal GBA sessions still use two independent workers. In the
experiment the two views show the same native battle while the partner's own
core pauses. Outside battle, each core controls its own avatar; the partner is
currently a framebuffer overlay, with no native collision or terrain occlusion.

In **Settings**, the experimental mode offers audio from player 1, player 2,
both, or mute. The choice applies on the next session start. A shared battle
plays its single native battle soundtrack once regardless of which player
initiated it (unless muted). The audio selection is inspired by
[SplitEmu](https://github.com/JosipFX/splitemu); the implementation here is separate.

At each paired-session start, if both FireRed battery saves already exist,
the launcher also creates a verified **paired backup** of those two files.
Both players should save normally in-game first, stop the session, and start
again to create a new pair. This records the last individual in-game saves,
not a simultaneous savestate; the two saves can still represent different
game moments. Under **Saved games**, select **Restore both players together**
and choose a paired backup folder. Both current saves are backed up first.
If the process is interrupted during restoration, the next session first
attempts to roll it back from a journal. Unreadable recovery data block the
session; a corrupt selected backup is rejected before either file is replaced.
Power-loss and hardware-failure recovery have not been tested.

If too many active leads and living EXP Share holders must fit in the native
six-slot battle party, the session stops before party conversion and explains
how to retry with fewer held EXP Shares. Progress since the last in-game save
may be lost. Other worker failures retain generic diagnostics and logs.

A held Enigma Berry is also rejected before the team is combined: its effect
comes from the holder's save, so using only the battle leader's data could give
the other player an incorrect effect or EXP award. The launcher explains this
separately. Until per-owner berry effects are implemented, reload the last saves
and remove held Enigma Berries before a shared battle. Unsaved progress may be
lost when the session stops.

The battle transaction snapshots both machines and their Flash buffers. It
routes actions, replacements and move-learning requests to the owning human,
preserves trainer identity and badge effects, transfers native HP/PP/status/EXP,
and returns items, catches, PC storage and Pokédex ownership to separate players.
Native money announcements use the initiator's share. Trainer prize money and Pay Day are split equally; an odd unit goes to the
initiator. Each wallet cap is applied independently. A lost team uses each
trainer's own native blackout calculation; an exhausted player can recover while
the surviving partner finishes. No combined party is flushed to a battery file.
Stopping during battle rolls back the entire transaction before normal shutdown.

Private actual-ROM checks cover ordinary wild/trainer wins, Pay Day, distinct
trainer identities, second-player initiation, exhaustion, defeat, catches with
full PCs, native capacity rejection, two queued balls, and each trainer's own
in-game save followed by a fresh battery-only reload. These use authored test
parties/encounters and virtual input; they are not a campaign or physical-controller
certification. Production Session/worker checks separately cover boot, both
framebuffers, input, restart, child failure and lock/save preservation.

## Campaign synchronization — 29 October 2026

The opt-in development worker now shares reviewed campaign milestones beyond the
initial lab/Parcel/Pokédex/Brock events. The policy covers the remaining seven
Gyms and their own badge/TM receipts, Bill and the ship ticket, story HMs and key
items, the Tower/Flute/Snorlax gates, Silph liberation, and the Sevii Ruby/Sapphire
quest. Personal starters, Pokémon gifts, fossil choices, ordinary pickups,
Pokédex contents, party, PC, wallet and purchases remain owned by each trainer.
A full bag defers that player's required reward without inventing its receipt.
Lostelle's optional Iapapa Berry follows the original best-effort gift: rescue
progress still completes with a full berry pocket, without a later invented claim.

Delivery waits for native scripts, menus and movement to yield the field. Both
machines and Flash buffers are snapshotted before a paired RAM commit. Loaded
NPCs, doors and collision tiles are refreshed through the original game's map
setup and native script VM. Unsupported executed map operations defer the whole
associated delta; they are not ignored. Temporary puzzles and League runs are
not merged as permanent campaign flags. Shared Elite Four wins open the peer's
own current-room exit, and native blackout resets the expedition normally.

A pending encounter waits for the partner's menu/cutscene. Simultaneous random
grass encounters choose one shared battle; different scripted objects keep their
own continuations and are handled in sequence. The first lab rival also waits
until both players can join. Scripted wild encounters, the revealed Marowak and
original legendary encounter flags retain their native battle rules and return
scripts. Safari/link/facility controllers remain separate native modes.

Both Champion scripts can rendezvous at their native battle entry, including
players with different starters. Each resumes their own post-battle scene and
records their own native Hall of Fame sectors, ribbons and first-clear data.
An idle partner receives the original Hall of Fame entry scene. National Dex
upgrades and Sapphire network completion run the native engine operations;
they do not copy a game-clear flag, Pokédex contents or another player's roamer.
Each starter initializes its own native roaming legendary exactly once.

Private exact-ROM checks cover all seven later native Gym conversations, trainer
teams, post-battle scripts, own rewards and separate native saves/fresh Continue;
Surge's beams and Blaine's doors include live collision checks. They also cover
Cut/Surf/Strength conversations, Marowak's coordinate script, both Champion
continuations, Hall of Fame, National Dex and the three own-starter network
completions. Prerequisite maps, stages and strong parties are explicitly authored
test fixtures. These are targeted native scenarios, **not a continuous natural
playthrough or production-launcher campaign certification**.
[Campaign verification scope](verification/firered-campaign-2026-10-29.json).
The earlier [27-scenario report](verification/firered-paired-2026-10-29.json) retains
its dated battle/story scope. Public Alpha 8 is unchanged.

### Additional launcher runtime work — 29 October 2026

An earlier launcher journey found that a single wild opponent became two active
battlers, and that a guest's ball-use message named the battle initiator. A
follow-up found the same duplication for a trainer with one Pokémon. The current
experimental worker suppresses the extra battler through the battle intro and
menu redraws, and identifies the actual ball user in the message. On Linux,
targeted tests covered both initiators, multiple turns, the bag and own-party
menus, captures, and trainer and wild encounters. A natural launcher encounter
using existing save teams also covered a guest catch, native save and restart.
See the [single-opponent follow-up](verification/firered-single-opponent-fix-2026-10-29.json)
for the exact evidence and remaining limits. The earlier
[journey report](verification/firered-launcher-journey-2026-10-29.json) records
the defects as they appeared before this follow-up.

Another defect made ordinary escape impossible in converted wild battles;
the worker now permits the original speed and random-chance calculation for
either player. A normal escape attempt can still fail. Native two-core tests
cover either battle initiator, a player-two-only attempt, preserved inventories
and separate saves; the production launcher was also exercised with real game
input and virtual controllers.

The two players also naturally reached and won the early Route 22 rival fight.
Its 144 prize units were divided into 72 per player. The guest initially kept
the pre-fight scene, so the worker now shares this completed encounter only
after both the native win receipt and finished scene exist, using the guest's
own starter-dependent rival variant. Real post-battle saves and fresh Continue
checks confirm the guest's progress and unchanged personal items and wallets.
After restarting the launcher, the guest walked onto the original trigger tile
without starting the rival scene again.

The Linux launcher was exercised with two virtual controllers and isolated
profiles based on public English/Japanese saves on the exact German ROM. Walking
across the Route 1/Viridian connection, seeing the other player indoors, opening
each own party, a shared wild win, a player-two catch, individual in-game saves
and launcher restarts were observed. A separate early/eight-badge pair retained
the synchronized eight badges after both saved and restarted. These imported
multi-language test saves are not a guarantee of cross-language compatibility.
See the [journey report](verification/firered-launcher-journey-2026-10-29.json).
Physical controllers and native Windows/Bazzite runs are still pending.
[Bazzite source-build test guide](bazzite-firered-test.md).

[Independent public-save checks and the quest-log playback fix](development-status.md#firered-public-save-checks-and-quest-log-fix-29-october-2026)
add targeted evidence with existing player teams and different campaign stages.
The files' original languages and the authored map placement are recorded there.

[Dated scope](verification/firered-launcher-runtime-2026-10-29.json): 20 native
suites, 60 Python tests without skips and 50 targeted ROM cases pass. Prerequisites
are authored fixtures; this is not a continuous natural campaign.

The native switch menu now shows each human's own six Pokémon. An unloaded
reserve may replace an unreferenced own bench slot, including a living one;
retired records stay in the ownership journal. Physical party/index/order data
are restored before native battle reshow, including cancel and rollback paths.
Living participants awaiting EXP, resident EXP Share holders, Wish targets,
queued switches and pending evolutions remain protected. Before an ordinary
shared battle, the worker now prioritizes living EXP Share holders from the
complete original parties, including late party slots. Native EXP/EV awards,
level-up learning and evolution were exercised with prepared two-core cases for
both initiators. If the two leads and eligible holders cannot fit in six native
slots, the experimental session stops before converting either party; the last
validated disk saves are retained. A fully pinned working set can still require
a rollback, and not every 12-Pokémon learning/evolution path is covered.
[Exact test scope](verification/firered-exp-share-2026-10-29.json).

The ROM file stays unchanged. The paired worker now projects partner avatars
across reciprocal native outdoor-map connections, including signed coordinate
offsets. This is read-only presentation; battle joining still requires the same
map, and collision/terrain occlusion remain incomplete. Native checks cover both
views at Pallet/Route 1 and Route 1/Viridian, a normal walking transition, and
separate saves/fresh Continue.

Reviewed continued Rocket battles now share the B4F Giovanni barrier guards and
the Tower 7F grunts after their original removal receipts. Lostelle's rescue no
longer waits for room for the optional berry. Actual native battle/post-scene
checks verify loaded doors/NPCs and both trainers' saved progress.

Loaded map refresh preserves the player's own Icefall ice state, existing
Seafoam layout and Lorelei doll collection. The Tanoby chambers apply their
original lighting update. Eleven native map cases verify actual SAVE and fresh
Continue. A change to a new Seafoam layout still waits for normal native map
re-entry; optional/link/facility helpers remain outside the reviewed interpreter.

**Still required before complete co-op sign-off:** a continuous two-player run
from the beginning through the campaign and postgame, remaining unsupported map
setup operations, complete EXP Share and all
learning/evolution paths, partner collision/terrain occlusion, special battle
modes, long sessions, physical controllers, audible output and native
Windows/Bazzite acceptance. The native battle working set usually loads up to
three viable Pokémon per player, but may allocate more slots to one player to
accommodate EXP Share holders. Safe exhaustion refills and the own switch menu
can use the full own roster subject to the protected-reference limit above.
Protected native references can still require a rollback. Identical PID/OT
Pokémon identities across profiles are rejected. The mode remains experimental.

ROM-dependent acceptance tools are excluded from normal builds, installation and
CTest. No ROM, save or raw checkpoint is distributed. Supply your own exact ROM,
a private initialized overworld checkpoint, and a new output directory:

For a repeatable suite, run the matrix below instead of the individual commands.
`quick` runs a smaller selection; `full` includes both initiators, captures,
switching, experience, Gym and campaign checkpoints, component and launcher
tests. The two optional saves must be the zero-badge and eight-badge saves
described in the verification report below: individual scenarios rely on those
specific teams and progress, so arbitrary saves are not interchangeable.
Omitting both saves explicitly marks their scenarios skipped. The Linux-only
production crash test is marked skipped on other systems. The script builds the
targets, verifies input hashes before and after, uses a private temporary
directory and only creates a report at a new path. Failure diagnostics can
contain private game details, so review a report before sharing it.
Add `--jobs 2` (up to eight) to run independent native scenarios concurrently.
For a focused rerun, add `--case pair/capture --case gyms/misty`; these names
select only the named native scenarios plus the component and Python tests.

```sh
python3 scripts/test-firered-matrix.py --profile full --rom ORIGINAL_DE.gba \
  --checkpoint PRIVATE_STARTER_STATE --save-early SAVE_0_BADGES.sav \
  --save-late SAVE_8_BADGES.sav --build-dir build/launcher --report result.json
```

The checkpoint and many scenario prerequisites are prepared; a successful suite
does not establish a complete natural campaign or physical-controller coverage.
The capacity and Enigma Berry scenarios check safe rejection, not support.
The [29 October matrix report](verification/firered-matrix-2026-10-29.json)
records 94 passing result groups on Linux, including the component and Python
suites, two independent public saves and a checkpoint created from the early
save. Scenarios author parts of the game state; no continuous playthrough,
physical controller or native Windows/Bazzite acceptance is claimed.

A private checkpoint can be created by booting a suitable 128 KiB save in an
isolated core. The source ROM and save stay unchanged; the checkpoint contains
private game state and must not be committed or shared as part of the project:

```sh
cmake --build build/launcher --target create-firered-checkpoint
build/launcher/create-firered-checkpoint ORIGINAL_DE.gba SAVE_0_BADGES.sav PRIVATE_STARTER_STATE
```

It must reach an idle overworld with a usable party; other saves may reach
different situations and individual checks can still fail.

Individual checks can also be run directly:

```sh
cmake --build build/launcher --target verify-firered-pair
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATE_OVERWORLD_STATE NEW_OUTPUT_DIRECTORY wild
cmake --build build/launcher --target verify-firered-campaign verify-firered-gyms
build/launcher/verify-firered-campaign ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_CAMPAIGN_OUTPUT shared-champion-rendezvous
build/launcher/verify-firered-gyms ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_GYM_OUTPUT surge
cmake --build build/launcher --target verify-firered-connections verify-firered-progression verify-firered-map-setup
build/launcher/verify-firered-connections ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_CONNECTION_OUTPUT
build/launcher/verify-firered-progression ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_STORY_GATE_OUTPUT lostelle-full-berry
build/launcher/verify-firered-map-setup ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_MAP_SETUP_OUTPUT
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_MENU_OUTPUT party-menu-cancel
cmake --build build/launcher --target verify-firered-story
build/launcher/verify-firered-story ORIGINAL_DE.gba PRIVATE_STARTER_STATE ANOTHER_NEW_OUTPUT_DIRECTORY brock-full
python3 tests/smoke_firered_pair_worker.py build/launcher/gba/gba-worker ORIGINAL_DE.gba
python3 tests/smoke_firered_pair_session.py build/launcher/verify-gba build/launcher/gba/gba-worker ORIGINAL_DE.gba
```

The separate public-save checks take caller-supplied 128 KiB saves and leave them
unchanged. These examples use the two specific files linked in the
[verification report](verification/firered-public-saves-2026-10-29.json): `questlog`
expects the eight-badge save first and the zero-badge save second; `brock` uses
the reverse order and its original Pokémon's second move.

```sh
cmake --build build/launcher --target verify-firered-public-saves verify-firered-public-pair
build/launcher/verify-firered-public-saves ORIGINAL_DE.gba SAVE_1.sav SAVE_2.sav
build/launcher/verify-firered-public-pair ORIGINAL_DE.gba SAVE_8_BADGES.sav SAVE_0_BADGES.sav questlog
build/launcher/verify-firered-public-pair ORIGINAL_DE.gba SAVE_0_BADGES.sav SAVE_8_BADGES.sav brock
```

The earlier single-core [team-input experiment](verification/firered-team-input-2026-10-28.json)
records only its historical scope. It is not the paired implementation's test report.

## Current split-screen mode

Use **Start split screen** after assigning two controllers. Each player has one 240×160 screen, independent input/audio and their own battery save. The menu layout is retained, with game-specific labels/help. There is no additional journey menu or emulator settings field. Shared-world co-op, GBA link trading/battles, a shared Pokémon bank and travel between editions are not implemented by this worker.

Save through the game's own menu and wait until its save indicator finishes. Start + Select held for two seconds stops the session. The worker snapshots changed battery data atomically every 60 frames and at orderly shutdown. Supported save sizes are EEPROM 512 bytes / 8 KiB, SRAM 32 KiB and Flash/SRAM 64 KiB / Flash 128 KiB. Games without battery storage do not create an empty save. Invalid sizes and detected size mismatches fail without overwriting the existing file; a snapshot is not a substitute for an in-game save. Saves and verified backups remain separate per player and exact ROM SHA-256. Different builds/languages never silently share or migrate a save.

The launcher chooses the bundled `gba-worker` automatically. The existing DS emulator and BIOS settings remain unchanged and do not affect GBA games. No external GBA BIOS is needed; mGBA uses its software implementation. Select + R is ordinary GBA input, because there is no second GBA screen to toggle. Native controller assignment/disconnect behaviour and the exit chord are retained.

Build with `scripts/build.sh`; mGBA 0.10.5 is pinned in `config/mgba.lock.json`. Source packages include pinned mGBA source and MPL-2.0 licenses under `gba-corresponding-source`, plus the reproducible XROM overlay in `patches/mgba-xrom.patch`. CMake applies the overlay to a separate owned build copy; the pinned checkout stays pristine. An offline build can set `-DCOUCHSPLIT_MGBA_SOURCE=/path/to/gba-corresponding-source`. The IPC ABI remains 2. Released Alpha 8 packages and the previous FireRed-only preview do not include this general GBA extension. Native Windows/Bazzite execution has not been verified for this change.

Production-worker acceptance without commercial game assets:

```sh
python3 tests/smoke_gba.py build/bin/gba-worker
```

The private `build/launcher/verify-gba` acceptance tool runs a separately supplied GBA ROM with production Session/PlayView and virtual SDL controllers. Authored ARM programs verify memory devices and persistence; actual German FireRed gameplay is checked separately. Neither is evidence of physical-controller support or a complete playthrough of every game.

## Extended journey ROMs

[CouchSplit XROM v1](extended-rom.md) keeps one `.gba` file and the same launcher UI. It exposes immutable assets after the native 32 MiB through an additional emulator data aperture. It is a custom format requiring the matching worker, not a standard cartridge that runs in arbitrary GBA emulators or hardware. The seven reserved header bytes at 0xB5 contain `CSXROM1`, the native header checksum is valid, and the file is four-byte aligned and greater than 32 MiB. Unknown oversized ROMs remain rejected. A larger container does not add missing game chapters.
