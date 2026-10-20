# German PM 0.4.5 text alpha

**English** · [Deutsch](de/german-localization.md) · [Documentation](README.md)

Historical build report of 15 October 2026, **Alpha 2**. It implements German text for the verified US PM ROM and original German JA/NEIN graphics, including all eight animation states. Two identical builds, archive/text checks, unchanged PM program bytes and startup in two real emulators passed. German greeting text with an umlaut and JA/NEIN were visually checked; touching JA advanced the introduction. That test did not verify a complete playthrough, every menu, in-game saving or a shared world, or Bazzite/TV/controller hardware. See [Development status](development-status.md) for later gameplay and saving evidence.

## Build

Python 3.11+, no extra Python packages. Supply all three ROMs yourself and extract ZIPs first. Prepare the PM ROM from USA Rev 1 and the actual PM 0.4.5 patch with `scripts/prepare-rom.py`; retain all xdelta checks.

```bash
python3 scripts/localize-pm.py \
  --us /path/original-usa-rev1.nds \
  --german /path/original-platinum-german.nds \
  --pm /path/platinum-pm-045-us.nds \
  --output /path/outside-the-repo/platinum-pm-045-de-alpha2.nds \
  > /path/outside-the-repo/localization-report.json
```

Exact verified SHA-256 inputs are required. The script reads immutable bytes, refuses existing outputs and outputs inside the repository, and fails on unresolved PM messages or changed control sequences. Originals are unchanged. Select the new `.nds` in the launcher. It intentionally retains `CPUE` and US game code: a German text version of the US mod, rather than a German-engine port.

| Input / output | SHA-256 |
|---|---|
| USA Rev 1, CPUE | `fbce4c4def0c7797f8dd238a3d7a5e48b4a7e3abd86890ac65f6321cef781bdb` |
| Original German edition, CPUD Rev 0 | `9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217` |
| Patched US PM 0.4.5 | `8ef9d4eeee81513f2d6e13c36125cdbfa4c6f498b039ef4b99faa7095d39cfa5` |
| German Alpha 1 (previous), 141960288 bytes | `8b11f36e7bf4a15932eb364358fba183c2067ac8265cfbc97611ca53e3c90949` |
| German Alpha 2, 141962768 bytes | `800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c` |

## Implementation

`localization/formats.py` reads Nitro filesystems, NARC archives and encrypted message banks. `text.py` handles western DS characters, 9-bit names in 15-bit words and lossless controls. `merge.py` compares original US, original German and PM by matching IDs or uniquely identified content. New PM entries survive. Authored translations in `de_pm045.json` are keyed only by the SHA-256 of PM input messages; no Nintendo original text or ROM dump is supplied.

Format facts were checked against primary sources: [message reader](https://github.com/pret/pokeplatinum/blob/main/src/message.c), [character table](https://github.com/pret/pokeplatinum/blob/main/charmap.txt) and [string implementation](https://github.com/pret/pokeplatinum/blob/main/src/string_gf.c). No code or assets were copied from them.

Only `msgdata/msg.narc`, `msgdata/pl_msg.narc`, `graphic/font.narc`, `graphic/pl_font.narc` and `graphic/touch_subwindow.narc` change. New resources are appended; corresponding FAT entries, ROM size/capacity and header CRC are updated. Byte comparison confirms all original bytes outside those FAT/header fields remain identical. Every FAT resource, including unnamed overlays, is checked. ARM9, ARM7, overlays, scripts and PM mailbox logic remain unchanged.

| Category | Entries |
|---|---:|
| Original German text at matching IDs | 72586 |
| Uniquely mapped original text at shifted IDs | 2701 |
| Authored PM translations, including repeated occurrences | 1208 |
| Intentionally neutral entries, blanks, proper names and individual Japanese diagnostic labels | 421 |
| Unused Diamond/Pearl credits retained unchanged | 216 |
| Unresolved PM entries | 0 |
| Total across both message archives | 77132 |

There are 857 translation fingerprints and 40 explicitly retained fingerprints. Repeated text needs one translation. The credits bank has 216 US versus 237 German entries with shifted IDs; retaining it avoids importing incorrect names/blanks. `msgdata/scenario/scr_msg.narc` is identical in all inputs, uses another format and is not rewritten. German font archives are imported only because PM left them unchanged from US originals.

## Original JA/NEIN graphics in Alpha 2

The German edition stores the archive at `resource/ger/touch_subwindow/touch_subwindow.narc`; the US engine loads `graphic/touch_subwindow.narc`. Filename intersections alone therefore miss the language difference. `localization/graphics.py` imports original German bytes at the US path, without distributing Nintendo graphics.

The PM archive was checked byte-identical to the US original. Both archives contain ten resources; only compressed NCGR texture member 1 differs. The palette and all eight animation maps are identical. The decoded texture remains 3776 bytes, so VRAM use, button geometry, touch areas and program routines do not change. The script rejects PM modifications or mismatched palette/animation/memory size.

Primary references: [loader](https://github.com/pret/pokeplatinum/blob/1dc13dcea40c04ab53f1f607de46023dc409ef20/src/yes_no_touch_menu.c) and [archive order](https://github.com/pret/pokeplatinum/blob/1dc13dcea40c04ab53f1f607de46023dc409ef20/res/graphics/yes_no_touch_menu/yes_no_touch_menu.order). Actual graphics come only from the user's own German ROM.

Two ROM-free tests check transfer and incompatible-resource rejection; the suite then had 17 passing tests. Two builds produced the same Alpha 2 fingerprint. A 110-second run with two real processes produced 6566/6571 frames, connected roles 1/2 and both exit codes 0. Input: A pulses only to player 1 during seconds 10–65, then neutral; one touch at (128, 83) at second 90. The preceding picture showed JA/NEIN; the touch led to Professor Rowan (Eibe in German) and the next dialogue. This verifies introduction display/JA action, rather than shared-world gameplay. Alpha 1 remains historical.

## Limits and acceptance

Entry counts describe processed message archives, rather than every visible pixel. The previously English YES/NO buttons become German in Alpha 2. Other text embedded in graphics or stored elsewhere, developer proper names, credits and US grammar routines can remain. Check text widths, long menus and formatting while playing. Other graphics and the title image are not localized.

A new ROM hash creates new save directories. Back up existing saves first; compatibility across arbitrary versions is not established. Both players need the same alpha. See the explicit [save-transfer procedure](save-transfer.md) for the verified corrections.

Check the introduction, Pokémon/trainer/place names, moves/items, PC lab, summaries, placeholders and multipage dialogue. After receiving the Pokédex, enable **Bag → Key Items → Wireless Play → Start Wireless** (German ROM: **Beutel → Basis-Items → Funkspiel → Funkspiel starten**). Test actual joining, movement, battles, trading, independent saves and reconnect using the [acceptance checklist](acceptance-tests.md). TCP alone proves none of these.

ROM-free checks: `python3 -m unittest discover -s tests -p 'test_*.py'`, with `xdelta3` in PATH for decoder tests. The opt-in `tests/verify_user_rom.py` checks startup, frames, separate input, TCP and shutdown with your ROM; it does not automatically unlock Wireless Play.
