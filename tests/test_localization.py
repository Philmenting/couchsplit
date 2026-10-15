# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic binary fixtures only. No copyrighted game assets needed."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from localization.formats import Bank, Narc, Rom, crc16
from localization.text import compress, controls, decode, encode, expand
from localization.merge import localize, text_hash
from localization.graphics import localize_buttons, TOUCH_BUTTONS, GERMAN_TOUCH_BUTTONS

spec = importlib.util.spec_from_file_location('localize_cli', ROOT / 'scripts/localize-pm.py')
cli = importlib.util.module_from_spec(spec); spec.loader.exec_module(cli)


def bank(texts):
    return Bank(struct.pack('<HH', 0, 1234)).save([encode(t) for t in texts])


def narc(files):
    # Valid empty named archive, populated through the production writer.
    empty = (b'NARC\xff\xfe\x00\x01' + struct.pack('<IHH', 48, 16, 3)
             + b'BTAF' + struct.pack('<IHH', 12, 0, 0)
             + b'BTNF' + struct.pack('<I', 12) + b'\x04\x00\x00\x00'
             + b'GMIF' + struct.pack('<I', 8))
    return Narc(empty).save(files)


class FixtureRom:
    def __init__(self, texts):
        self.files = {path: narc([bank(texts)]) for path in ('msgdata/msg.narc', 'msgdata/pl_msg.narc')}
        self.files.update({path: b'unchanged font' for path in ('graphic/font.narc', 'graphic/pl_font.narc')})
    def file(self, path): return self.files[path]


def tiny_rom():
    image = bytearray(1024)
    image[12:16] = b'CPUE'
    struct.pack_into('<4I', image, 0x20, 600, 0, 0, 4)
    struct.pack_into('<4I', image, 0x30, 604, 0, 0, 4)
    # A named resource followed by an unnamed overlay member.
    fnt = struct.pack('<IHH', 8, 0, 1) + b'\x04text\x00'
    struct.pack_into('<4I', image, 0x40, 512, len(fnt), 560, 16)
    image[512:512 + len(fnt)] = fnt
    struct.pack_into('<4I', image, 560, 700, 703, 704, 708)
    image[600:608] = b'ARM9ARM7'
    image[700:703] = b'old'; image[704:708] = b'ovly'
    return Rom(image)


class LocalizationTests(unittest.TestCase):
    def button_roms(self):
        us, de, pm = FixtureRom([]), FixtureRom([]), FixtureRom([])
        # Self-authored tiny literal LZ10 packets, no Nintendo graphics.
        packet = b'\x10\x10\0\0' + b'\0ABCDEFGH\0IJKLMNOP'
        palette = b'OWN PALETTE'
        members = [palette, packet] + [b'OWN ANIMATION ' + bytes([i]) for i in range(8)]
        us.files[TOUCH_BUTTONS] = pm.files[TOUCH_BUTTONS] = narc(members)
        members[1] = packet[:-1] + b'Z'
        de.files[GERMAN_TOUCH_BUTTONS] = narc(members)
        return us, de, pm

    def test_original_german_button_texture_import_and_animation_preservation(self):
        us, de, pm = self.button_roms()
        result = localize_buttons(us, de, pm)[TOUCH_BUTTONS]
        self.assertEqual(result, de.file(GERMAN_TOUCH_BUTTONS))
        a, b = Narc(pm.file(TOUCH_BUTTONS)), Narc(result)
        self.assertNotEqual(a.files[1], b.files[1])
        self.assertEqual([x for i, x in enumerate(a.files) if i != 1],
                         [x for i, x in enumerate(b.files) if i != 1])

    def test_button_import_rejects_pm_changes_and_incompatible_resources(self):
        us, de, pm = self.button_roms()
        pm.files[TOUCH_BUTTONS] += b'changed'
        with self.assertRaisesRegex(ValueError, 'PM hat'):
            localize_buttons(us, de, pm)
        for index, content, error in ((0, b'OTHER PALETTE', 'Palette'),
                                      (2, b'OTHER ANIMATION', 'Palette'),
                                      (1, b'\x10\x20\0\0different size', 'VRAM')):
            us, de, pm = self.button_roms()
            archive = Narc(de.file(GERMAN_TOUCH_BUTTONS)); archive.files[index] = content
            de.files[GERMAN_TOUCH_BUTTONS] = archive.save()
            with self.assertRaisesRegex(ValueError, error): localize_buttons(us, de, pm)

    def test_glyphs_controls_and_packed_names(self):
        text = 'ÄÖÜ äöü ß é Pokémon!\n{FFFE,0100,0002,0000,0000}{25BC}'
        self.assertEqual(decode(encode(text)), text)
        self.assertEqual(controls(encode(text)), ('FFFE,0100,0002,0000,0000', '25BC'))
        for length in range(40):  # Covers every 9-bit/15-bit boundary alignment.
            codes = encode(('Abä09' * 8)[:length])
            self.assertEqual(expand(compress(codes)), codes)
        self.assertEqual(compress((0x12B, 0xFFFF)), (0xF100, 0x7F2B, 0x7FFF, 0xFFFF))
        with self.assertRaises(ValueError): expand((0xF100, 0))
        with self.assertRaises(ValueError): encode('🙂')
        with self.assertRaises(ValueError): decode((0xFFFE, 0x100, 2, 0xFFFF))

    def test_bank_and_archive_roundtrip_and_bounds(self):
        # Fixed vector: one entry, seed 0x1234, glyphs A/v/end. Locks both
        # metadata XOR and per-character cipher independently of round-tripping.
        golden = bytes.fromhex('010034126865646567656465f81a4a64b251')
        self.assertEqual(Bank(golden).entries, [(0x12B, 0x15A, 0xFFFF)])
        self.assertEqual(Bank(golden).save(), golden)
        raw = bank(['Hallo!', 'Äöü', '{FFFE,0100,0002,0000,0000}{25BC}'])
        b = Bank(raw)
        self.assertEqual(b.save(), raw)
        self.assertEqual([decode(x) for x in b.entries], ['Hallo!', 'Äöü', '{FFFE,0100,0002,0000,0000}{25BC}'])
        n = Narc(narc([raw, b'', b'abcd']))
        self.assertEqual(Narc(n.save()).files, n.files)
        with self.assertRaises(ValueError): Bank(raw[:-1])
        with self.assertRaises(ValueError): Narc(n.save()[:-1])

    def test_three_way_reordered_and_new_pm_entries(self):
        us = FixtureRom(['Hello', 'Goodbye'])
        de = FixtureRom(['Hallo', 'Tschüss'])
        pm = FixtureRom(['Goodbye', 'Hello', 'Wireless'])
        key = text_hash(encode('Wireless'))
        files, stats, missing = localize(us, de, pm, {'messages': {key: 'Funkspiel'}})
        self.assertEqual(missing, [])
        result = Bank(Narc(files['msgdata/pl_msg.narc']).files[0])
        self.assertEqual([decode(x) for x in result.entries], ['Tschüss', 'Hallo', 'Funkspiel'])
        self.assertEqual(stats['explicit_pm_translation'], 2)
        _, _, missing = localize(us, de, pm, {})
        self.assertEqual(len(missing), 2)
        # Identical English words can have different German context meanings.
        # An added entry must not choose arbitrarily between such candidates.
        _, _, missing = localize(FixtureRom(['Hi', 'Hi']),
                                 FixtureRom(['Hallo', 'Grüß dich']),
                                 FixtureRom(['Hi', 'Hi', 'Hi']), {})
        self.assertEqual(len(missing), 2)

    def test_placeholder_loss_and_region_index_mismatch_rejected(self):
        message = 'Hi {FFFE,0100,0002,0000,0000}{25BC}'
        pm = FixtureRom([message])
        key = text_hash(encode(message))
        with self.assertRaisesRegex(ValueError, 'Control sequence'):
            localize(FixtureRom(['Hello']), FixtureRom(['Hallo']), pm, {'messages': {key: 'Hallo'}})
        with self.assertRaisesRegex(ValueError, 'Regional message indexes'):
            localize(FixtureRom(['Hello']), FixtureRom(['Hallo', 'Extra']), pm, {})
        # Exception: shifted legacy regional credits keep the PM bank intact.
        us, de, pm = FixtureRom(['Hi']), FixtureRom(['Hallo']), FixtureRom(['Hi'])
        prefix = [bank(['Hi'])] * 493
        for rom, tail in ((us, ['US credit']), (de, ['DE credit', 'Extra']), (pm, ['PM credit'])):
            rom.files['msgdata/msg.narc'] = narc(prefix + [bank(tail)])
        result, stats, _ = localize(us, de, pm, {})
        self.assertEqual(Narc(result['msgdata/msg.narc']).files[493], bank(['PM credit']))
        self.assertEqual(stats['legacy_credit_entries_retained'], 1)

    def test_append_preserves_code_unnamed_files_and_audit_detects_damage(self):
        rom = tiny_rom()
        changed = {'text': b'new localized resource'}
        result = Rom(rom.replace(changed))
        self.assertEqual(result.file('text'), changed['text'])
        self.assertEqual(result.data[600:608], b'ARM9ARM7')
        self.assertEqual(result.data[704:708], b'ovly')
        self.assertEqual(struct.unpack_from('<H', result.data, 0x15E)[0], crc16(result.data[:0x15E]))
        cli.audit(rom, result, changed)
        corrupted = bytearray(result.data); corrupted[600] ^= 1
        with self.assertRaisesRegex(ValueError, 'Unexpected original ROM byte'):
            cli.audit(rom, Rom(corrupted), changed)


if __name__ == '__main__': unittest.main()
