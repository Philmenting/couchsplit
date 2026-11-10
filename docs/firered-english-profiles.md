# English FireRed co-op profiles

**English** · [Deutsch](de/firered-english-profiles.md) · [GBA support](firered.md)

The development source adds experimental shared-world FireRed co-op for the
unmodified English 1.0 and 1.1 cartridges. This includes the existing partner
display, shared native battles with separate human input, and personal teams,
bags and saves. The published Alpha 11 installers still support the German
cartridge only. English requires a matching new launcher and worker built from
this source; native platform/package verification precedes a new binary release.

## Exact supported cartridges

| Cartridge | Header | SHA-256 |
|---|---|---|
| German original | BPRD, revision 0 | `eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507` |
| English 1.0 | BPRE, revision 0 | `3d0c79f1627022e18765766f6cb5ea067f6b5bf7dca115552189ad65a5c3a8ac` |
| English 1.1 | BPRE, revision 1 | `729041b940afe031302d630fdbe57c0c145f3f7b6d9b8eca5e98678d0ca4d059` |

The complete file fingerprint selects the profile before either paired core is
created. A language code, filename or valid GBA header alone cannot enable
co-op. Modified files and other revisions keep the ordinary independent GBA
mode. Each player and complete ROM hash has its own save directory, so selecting
English starts a separate save profile from an existing German session.

Build the launcher and workers together, select the English cartridge, and turn
on **Settings → FireRed co-op (experimental)**. The settings screen recognizes
supported English originals. Launcher language and cartridge language are
independent choices. The new worker advertises `FIRERED_ENGLISH=1`; an English
paired session rejects an older worker before taking the save lock.

## Address and instruction review

The original German locations remain canonical identifiers. `RomProfileScope`
binds each verified core to its cartridge ABI for the lifetime of its
coordinators. Fixed ABI addresses resolve explicitly at their uses; dynamic
native pointers and encrypted Pokémon data are passed through unchanged.
Known caller guards normalize only the native LR value. Unknown callers retain
native behavior. Profile bindings remain on the thread advancing both cores.

English profiles include separate callback/data locations, IWRAM globals and
all 38 battle hook sites with four-halfword instruction preimages. Runtime
installation checks the selected preimages before patching anything. Detach
and rollback restore the original in-memory instructions. The cartridge file
is opened read-only and remains byte-identical.

Function offsets alone are insufficient: English battle intro and player-name
cases differ inside the same functions, and the Wish message places its
placeholder at a different string position. These sites have explicit reviewed
mappings. Hall-of-Fame script pointers and their preimages also resolve per ROM.
The reserve-selection explanation follows the cartridge language.

Reviewed sources:

- [European decompilation](https://github.com/CompuMaxx/pokefirered-europe/tree/e184c5cf898cd29efebd33bc1bfe5994277e21ab), rebuilt with [agbcc](https://github.com/pret/agbcc/tree/da598c1d918402c42c0c0d7128ba14567f3175e9). The rebuilt German image matches the exact existing fingerprint.
- [English retail symbol maps](https://github.com/pret/pokefirered/tree/3b401eaf14f193946899eb7c056d5ac02a470126), checked against both exact English cartridges.
- Bounded Thumb disassembly checks cover all hook sites and 23 caller guards; 42 fixed EWRAM anchors have matching layouts. Original script preimages and text placeholder IDs were checked for all three cartridges.

No cartridge, battery save or checkpoint is included in source, normal tests,
installers or promotional downloads.

## Verification and limits

The standard 24 component/UI suites pass, including English profile isolation,
unknown-location rejection, caller guards, malformed preimages and hook removal.
Initial actual-ROM checks pass a shared wild battle with both inputs and each
player's own native SAVE followed by fresh battery-only Continue on English
1.0 and 1.1. The extended native matrix and production-launcher captures are
being completed separately; these initial results are not a campaign claim.

The existing experimental limits still apply: partner collision/occlusion,
special battle modes, full continuous campaigns, physical controllers and
actual Windows/Bazzite Pokémon sessions need further verification. Native
platform build checks alone do not establish those gameplay properties.

To reproduce a targeted check using private inputs:

```sh
cmake --build build/launcher --target create-firered-checkpoint verify-firered-pair --parallel 1
build/launcher/create-firered-checkpoint ENGLISH_ORIGINAL.gba PRIVATE_SAVE.sav PRIVATE_CHECKPOINT
build/launcher/verify-firered-pair ENGLISH_ORIGINAL.gba PRIVATE_CHECKPOINT NEW_OUTPUT single
```

These explicit tools are excluded from normal builds and installation. The
native scenario authors test parties and encounters in isolated paused cores;
its checks include transaction rollback and separate native saves/reboots.
Keep all inputs and generated game data outside the repository.
