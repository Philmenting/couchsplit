# GBA game support

**English** · [Deutsch](de/firered.md) · [Documentation](README.md)

Alpha 10 and the current source can run GBA games and ROM hacks in the existing CouchSplit interface. Select a `.gba` file or ZIP containing exactly one game. The launcher checks the GBA fixed header byte, header checksum and a size between 512 bytes and 32 MiB for standard cartridges. Explicitly marked CouchSplit XROM v1 files can be up to 512 MiB. A valid header identifies the file format; it does not certify that every game or hack works. Oversized and malformed files are rejected. Existing Project PM / Platinum support and the DS worker are retained; other DS games are not enabled by this change.

## FireRed co-op — Alpha 10

Alpha 10 includes an **experimental paired FireRed worker**: two real
mGBA cores with separate saves, a partner overlay on the same or directly connected outdoor maps, and shared native
battles with each human choosing their own actions. This remains an unfinished
implementation of the requested shared-world co-op. Older Alpha 8 assets are unchanged.
It is restricted to the exact unmodified German original; generic GBA games,
other languages and ROM hacks do not acquire co-op through this option.

Install Alpha 10, then explicitly opt in through **Settings → FireRed co-op (experimental)**. For a source build, build the launcher and both workers together; the
script places matching executables in `build/bin`:

```sh
COOP_BUILD_JOBS=1 scripts/build.sh
build/bin/platinum-coop
```

The choice is saved and applies on the next session start. Settings shows whether
the selected ROM is the supported German original. Other GBA games keep their
independent sessions. The old `COUCHSPLIT_FIRERED_EXPERIMENT=1` environment opt-in
is only a default when no mode preference has been saved; an explicit saved choice wins.

The selected worker must advertise `FIRERED_PAIR_EXPERIMENT=1` and `FIRERED_RECOVERY=1`. One process owns
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

A held Enigma Berry in either party makes **both games pause before party
conversion**. Their live progress, original parties and Flash buffers remain
in the running session. Choose **Continue individual battles** with the mouse,
keyboard, or player 1's controller A button. This explicit choice resumes the
original encounters and scripts with each player's own team and native rewards.
Shared co-op resumes when both reach an idle field. Remove the held Enigma Berry
afterwards to allow future shared battles. No automatic fallback occurs.

EXP Share no longer triggers the six-slot admission pause. Alpha 9 tracks all
owned participants and holders, pages recipients into the native working party,
and lets the original engine award EXP/EVs and run move-learning/evolutions.
Per-owner Enigma effects remain unsupported. Recovery does not create an in-game
save: save normally before closing. Unexpected worker errors still stop the
session with diagnostics. The play-view status shows whose turn it is, which
partner is awaited, and whether individual battles are in progress.
[Current evidence and limits](verification/firered-alpha9-2026-11-08.json).

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

The native switch menu shows each human's own six Pokémon. An unloaded reserve
may replace an unreferenced own bench slot, including a living one. A journal
retains its owned progress and participation. Physical party/index/order data
are restored before native battle reshow, including cancel and rollback paths.
Active Pokémon, queued switches and Wish targets still protect physical slots;
Alpha 10 rejects an unavailable reserve before changing party order, shows a
German in-game explanation and lets the owner pick a resident alternative or
cancel when the native request permits cancellation. Unexpected corruption still
fails closed; fully pinned forced refills or EXP paging can still require rollback.

Alpha 9 distributes native EXP/EV awards across all twelve original party slots.
Prepared two-core tests cover twelve EXP Share holders, twelve pending evolutions,
both battle initiators, move learning with conflicting other-player input, and
switching through both complete teams without losing retired participation.
Evolution checks use the evolving owner's National Dex status and input, including
when it differs from the initiator's. Native saves and fresh Continue preserve
the results. [Exact scope](verification/firered-alpha9-2026-11-08.json).

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

Loaded map refresh preserves each player's own Icefall ice and Lorelei dolls.
Seafoam B3F/B4F can apply their native water-current layout changes without leaving
the map, after validating the current grid, dimensions and tilesets. Changed
cells occupied by a player/object defer safely; personal puzzle flags remain
local. Cable Club/Wonder Card getters retain personal data, and Tanoby lighting
uses its original native update. Thirty-two targeted map cases include all
nineteen Pokémon Center upper floors, native SAVE and fresh Continue.

**Still required before complete co-op sign-off:** a continuous two-player run
through campaign and postgame, remaining Trainer Tower/e-Reader setup operations,
partner collision/terrain occlusion, special battle modes, multi-hour sessions,
physical controllers, audible output and native Windows/Bazzite gameplay.
Native Nincada evolution now appends Shedinja only to its owner's free party
slot, with owned Pokédex receipts; a full party or canceled evolution does not
create one. Ninjask/Shedinja siblings keep their authentic shared PID/OT and are
distinguished in subsequent battles and menu paging. Other ambiguous duplicates
remain rejected.

Correction to the Alpha 9 limitation: original FireRed's random Pokérus infection,
spreading and day-countdown functions are **stubs**, not mixed-party operations.
The worker preserves each Pokémon's existing active/cured status; the original
engine applies its EV bonus, including unloaded EXP Share recipients. It does
not add Ruby/Sapphire/Emerald infection mechanics. See the pinned
[original source](https://github.com/pret/pokefirered/blob/ce6406d8c2eaf46d543c8a2606de0c3cb1951982/src/pokemon.c#L5607)
and the [Alpha 10 checks](verification/firered-alpha10-2026-11-08.json). The working party usually contains up to three viable Pokémon per
player; the journal is not unlimited native battle virtualization. Protected
references can require rollback. Ambiguous duplicate identities across profiles are rejected, with the narrow
native Ninjask/Shedinja sibling exception. The mode remains experimental.

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
By default, builds, component tests and native scenarios run with one job.
Add `--jobs 2` (up to eight) to increase concurrency and CPU load.
For a focused rerun, add `--case pair/capture --case gyms/misty`; these names
select only the named native scenarios plus the component and Python tests.

```sh
python3 scripts/test-firered-matrix.py --profile full --rom ORIGINAL_DE.gba \
  --checkpoint PRIVATE_STARTER_STATE --save-early SAVE_0_BADGES.sav \
  --save-late SAVE_8_BADGES.sav --build-dir build/launcher --report result.json
```

The checkpoint and many scenario prerequisites are prepared; a successful suite
does not establish a complete natural campaign or physical-controller coverage.
The overflow scenarios now check shared awards, including twelve recipients.
The Enigma Berry scenarios check explicit recovery into individual battles.
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

## Recovery acceptance

The `exp-share-enigma` native scenario and its `-leader2` and `-both` variants
verify live state/Flash preservation, explicit individual combat, both native
saves/fresh Continue, and later shared admission after removing the berry.
`exp-share-overflow` instead verifies successful shared combat in Alpha 9.

For a separate production Session/PlayView/worker check, create private authored
Route 1 saves and then trigger an encounter through virtual-controller walking:

```sh
cmake --build build/launcher --target verify-firered-pair verify-gba gba-worker
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_PRIVATE_ENIGMA_FIXTURE exp-share-enigma-launcher
python3 tests/smoke_firered_recovery.py build/launcher/verify-gba build/launcher/gba/gba-worker ORIGINAL_DE.gba NEW_PRIVATE_ENIGMA_FIXTURE NEW_PRIVATE_ENIGMA_RESULT 1
```

The final argument selects player 1 (`0`) or player 2 (`1`). Keep all generated
saves, screenshots and game logs outside the repository. The check holds the
pause for 16 seconds, tests its heartbeat and retained saves/lock, clicks the real
UI action, and finishes native combat until co-op resumes. The starting parties,
items and location are authored fixtures; this is not a continuous campaign.

## Sustained production-session test

The private test below repeats natural grass encounters, alternating initiators,
Pallet/Route 1 walking transitions and in-game saves in one production
Session/PlayView/worker. It checks both input owners, complete native save
checksums/generations, owned parties, PC storage, money and battle counts. Only
at the end does it restart and verify fresh Continue. Strong authored level-100
parties keep the route repeatable; this is not a natural campaign or hardware test.

```sh
cmake --build build/launcher --parallel 1 --target verify-firered-pair verify-gba gba-worker
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATE_STARTER_STATE NEW_PRIVATE_SOAK_FIXTURE soak-launcher
python3 tests/smoke_firered_soak.py build/launcher/verify-gba build/launcher/gba/gba-worker ORIGINAL_DE.gba NEW_PRIVATE_SOAK_FIXTURE NEW_PRIVATE_SOAK_RESULT --cycles 10 --minimum-minutes 10
```

Keep generated saves, screenshots and logs outside Git. The test requires a new
output directory. For lighter local validation, the matrix defaults to one build
job/test/scenario at a time; use `--jobs 1` explicitly if desired. Linux users with
a user systemd session can run a command under `systemd-run --user --scope -p CPUQuota=70% nice -n 15 …` to cap the whole process group at 70% of one CPU.
This changes scheduling, not fan controls or the saved game.

## Independent GBA split-screen mode

Outside the FireRed experiment, use **Start split screen** after assigning two controllers. Each player has one 240×160 screen, independent input/audio and their own battery save. The menu layout is retained, with game-specific labels/help. There is no additional journey menu or emulator settings field. This independent mode does not add shared-world co-op, GBA link trading/battles, a shared Pokémon bank or travel between editions.

Save through the game's own menu and wait until its save indicator finishes. Start + Select held for two seconds stops the session. The worker snapshots changed battery data atomically every 60 frames and at orderly shutdown. Supported save sizes are EEPROM 512 bytes / 8 KiB, SRAM 32 KiB and Flash/SRAM 64 KiB / Flash 128 KiB. Games without battery storage do not create an empty save. Invalid sizes and detected size mismatches fail without overwriting the existing file; a snapshot is not a substitute for an in-game save. Saves and verified backups remain separate per player and exact ROM SHA-256. Different builds/languages never silently share or migrate a save.

The launcher chooses the bundled `gba-worker` automatically. The existing DS emulator and BIOS settings remain unchanged and do not affect GBA games. No external GBA BIOS is needed; mGBA uses its software implementation. Select + R is ordinary GBA input, because there is no second GBA screen to toggle. Native controller assignment/disconnect behaviour and the exit chord are retained.

Build with `scripts/build.sh`; mGBA 0.10.5 is pinned in `config/mgba.lock.json`. Source packages include pinned mGBA source and MPL-2.0 licenses under `gba-corresponding-source`, plus the reproducible XROM overlay in `patches/mgba-xrom.patch`. CMake applies the overlay to a separate owned build copy; the pinned checkout stays pristine. An offline build can set `-DCOUCHSPLIT_MGBA_SOURCE=/path/to/gba-corresponding-source`. The IPC ABI remains 2. Alpha 10 packages include this GBA extension. Native package tests and real Pokémon gameplay on Windows/Bazzite are separate checks; the latter remain unverified.

Production-worker acceptance without commercial game assets:

```sh
python3 tests/smoke_gba.py build/bin/gba-worker
```

The private `build/launcher/verify-gba` acceptance tool runs a separately supplied GBA ROM with production Session/PlayView and virtual SDL controllers. Authored ARM programs verify memory devices and persistence; actual German FireRed gameplay is checked separately. Neither is evidence of physical-controller support or a complete playthrough of every game.

## Extended journey ROMs

[CouchSplit XROM v1](extended-rom.md) keeps one `.gba` file and the same launcher UI. It exposes immutable assets after the native 32 MiB through an additional emulator data aperture. It is a custom format requiring the matching worker, not a standard cartridge that runs in arbitrary GBA emulators or hardware. The seven reserved header bytes at 0xB5 contain `CSXROM1`, the native header checksum is valid, and the file is four-byte aligned and greater than 32 MiB. Unknown oversized ROMs remain rejected. A larger container does not add missing game chapters.
