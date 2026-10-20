# German edition: patch availability and compatibility

**English** · [Deutsch](de/german-compatibility.md) · [Documentation](README.md)

Historical research of 14 October 2026, updated 15 October. A **US-based German text alpha was implemented and boot-tested**; see [localization](german-localization.md). Full playthrough and world co-op were unverified at that time. Later gameplay results are in [Development status](development-status.md). A native CPUD engine port remains unimplemented.

The original German `CPUD` Revision 0 ROM had already booted in two real emulators; see the [boot report](rom-boot-check.md). That did not supply the missing ROM-side multiplayer mod.

The user then supplied `Build V0.4.5.zip`. Its guide requires USA Revision 1, and a real xdelta attempt on the original German ROM failed its checksum. Patch acquisition was resolved, but neither a CPUD patch nor ROM-mod source was provided. See the [package report](project-pm-045.md). The matching US base was subsequently supplied and patched with xdelta checks enabled; the English output booted in two real processes. German text transfer was implemented separately.

## Primary sources inspected at the time

| Source | What was actually found |
|---|---|
| [ComicartOlie on GitHub](https://github.com/ComicartOlie?tab=repositories) | Two public emulator repositories; no public ROM-mod repository. The Project PM branch remained at the pinned commit. |
| [Emulator v1.4.1-beta](https://github.com/ComicartOlie/melonDS-Project-PM/releases/tag/v1.4.1-beta) | Windows emulator archive and README, with no xdelta/BPS ROM patch. |
| [ComicartOlie's setup guide](https://www.youtube.com/watch?v=LgCSqOyCN3A) | Video metadata description listed developer Discord, an xdelta web patcher and four emulator links. No direct ROM patch or German-base compatibility statement. |
| [Developer's eight-player expansion post](https://www.reddit.com/r/PokemonROMhacks/comments/1wduy8y/project_platinum_multiplayer_8_player_expansion/) | Testing invitation at https://discord.com/invite/bTZfjHqbVd. The read post included no patch file or German compatibility promise. |
| [pret/pokeplatinum](https://github.com/pret/pokeplatinum) | README listed only US revisions 0/1. That does not establish supported languages of the unavailable PM ROM mod. |
| [Linux port linked by the tutorial](https://github.com/fdelavega02/melonDS-Project-PM/releases/tag/project-pm-linux-v1.4.1-beta) | An x86_64 emulator AppImage; no ROM patch/mod source in the inspected release/tree. |

A public repository search for `pokeplatinum multiplayer` found no matching mod source. This records the inspected sources, rather than proving no other source exists. The public Discord invitation endpoint returned HTTP 403; no channels/attachments were read. No developer was messaged and no Nintendo ROM was downloaded during this research.

## What a native German port needs

Either a CPUD-specific patch with exact base/output checksums, or ROM-mod source with usage terms and build instructions. A US xdelta is not automatically a German port. The inspected emulator bridge finds its mailbox dynamically through discovery magics and imposes no CPUE language requirement for that lookup, but the ROM mod must publish the mailbox and connect it to game code.

Mod source would allow adapting German resources, game hooks/overlays and added multiplayer text. An emulator release alone cannot provide a native CPUD game-code port. US-based text localization does not require that engine port. Changing the header or disabling xdelta checks does not establish language support.

## Local patch preparation

`scripts/prepare-rom.py` uses the existing xdelta3 decoder and checks:

- `CPUD` for unknown German patches, or cataloged `CPUE` codes for verified US PM 0.4.5;
- expected source, patch and result SHA-256 values;
- no overwriting existing outputs;
- preservation of the original, with privately permissioned output outside the repository;
- no partially published file after an invalid or mismatched patch.

Expected hashes must come from trusted release metadata or a reproducibly verified build. Missing hashes are a blocker; they are not replaced by invented values. CPUD identifies a version, rather than proving German text or working multiplayer. The helper includes **no Project PM ROM patch** and does not automatically port English code.

On Bazzite, the build script installs Fedora's `xdelta` package only inside Distrobox; it provides `xdelta3`. Ubuntu calls the package `xdelta3`. If a genuinely matching German patch becomes available:

```bash
distrobox enter platinum-coop-build -- python3 scripts/prepare-rom.py \
  --rom /path/to/your-german.nds \
  --patch /path/to/verified-german.xdelta \
  --output /path/outside-the-repo/platinum-project-pm-de.nds \
  --source-sha256 9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217 \
  --patch-sha256 "$PATCH_SHA256" --target-sha256 "$TARGET_SHA256"
```

This example requires a patch documented against exactly that German original. Both variables must contain genuine verified release hashes. Then boot with `tests/verify_user_rom.py` and perform [world acceptance](acceptance-tests.md): German introduction/names/dialogue, both characters, independent movement/interactions, save/load and reconnect. Only actual testing can establish German co-op support.

## Tests and remaining limitation

Real xdelta encoding/decoding of self-authored synthetic data tests reconstruction, spaces in paths, original preservation, permissions, existing-output refusal, incorrect source/patch/result hashes, English codes, broken patches and missing decoder. Tests contain no Nintendo game data. They verify preparation tooling, rather than a native German PM patch.

Supplied patch 0.4.5 requires CPUE USA Revision 1 and does not fit the German original. The matching US base was successfully patched and boot/TCP-tested. US-based German localization now exists as an alpha; a native CPUD port remains open. `scripts/prepare-rom.py --inspect` checks the base identity/fingerprint.
