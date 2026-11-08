# SPDX-License-Identifier: GPL-3.0-or-later
"""Authored format tests: no commercial ROM or save content is used."""
from pathlib import Path
import importlib.util
import struct
import unittest

SPEC = importlib.util.spec_from_file_location('firered_save_inspect', Path(__file__).with_name('firered_save_inspect.py'))
save = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(save)


def authored_battery(generation=7):
    block2, block1, storage = bytearray(0xF24), bytearray(0x3D68), bytearray(0x83D0)
    block1[4:6] = bytes((3, 19))
    block1[0x34] = 1
    struct.pack_into('<I', block1, 0x290, 1234)
    record, secure = bytearray(100), bytearray(48)
    struct.pack_into('<II', record, 0, 24, 123)
    struct.pack_into('<HHI', secure, 0, 25, 0, 125)
    struct.pack_into('<H', secure, 12, 33)
    secure[20], record[84] = 35, 5
    struct.pack_into('<HH', record, 86, 10, 10)
    struct.pack_into('<H', record, 28, sum(struct.unpack('<24H', secure)) & 0xFFFF)
    for i in range(12):
        struct.pack_into('<I', record, 32 + 4 * i, save.u32(secure, i * 4) ^ 24 ^ 123)
    block1[0x38:0x9C] = record
    chunks = [block2] + [block1[i:i + 0xF80] for i in range(0, len(block1), 0xF80)] + [storage[i:i + 0xF80] for i in range(0, len(storage), 0xF80)]
    result = bytearray(0x20000)
    for slot in range(2):
        counter = (generation - 1 + slot) & 0xFFFFFFFF
        for physical in range(14):
            identifier = (physical + slot * 3) % 14
            chunk, offset = chunks[identifier], (slot * 14 + physical) * 0x1000
            result[offset:offset + len(chunk)] = chunk
            total = sum(struct.unpack('<' + 'I' * (len(chunk) // 4), chunk)) & 0xFFFFFFFF
            struct.pack_into('<HHII', result, offset + 0xFF4, identifier, ((total >> 16) + total) & 0xFFFF, 0x08012025, counter)
    return result


class FireRedSaveInspectionTests(unittest.TestCase):
    def test_rotating_generations_and_counter_wrap(self):
        for generation in (7, 0, 0xFFFFFFFF):
            snapshot = save.inspect_save(authored_battery(generation))
            self.assertEqual(snapshot['generation'], generation)
            self.assertEqual(snapshot['map'], (3, 19))
            self.assertEqual(snapshot['party'][0]['identity'], (24, 123))
            self.assertEqual(snapshot['party'][0]['experience'], 125)
            self.assertEqual(snapshot['wallet'], 1234)

    def test_partial_corrupt_and_duplicate_sectors(self):
        data = authored_battery()
        data[14 * 0x1000] ^= 1
        self.assertEqual(save.inspect_save(data)['generation'], 6)
        data[0] ^= 1
        with self.assertRaisesRegex(ValueError, 'No complete'):
            save.inspect_save(data)
        data = authored_battery()
        for slot in range(2):
            source, target = (slot * 14) * 0x1000, (slot * 14 + 1) * 0x1000
            data[target:target + 0x1000] = data[source:source + 0x1000]
        with self.assertRaisesRegex(ValueError, 'No complete'):
            save.inspect_save(data)
        with self.assertRaisesRegex(ValueError, '128 KiB'):
            save.inspect_save(data[:-1])

    def test_owned_party_and_secure_checksum(self):
        initial = save.inspect_save(authored_battery())
        current = save.inspect_save(authored_battery(8))
        save.check_owned_progress(initial, current)
        current['party'][0]['identity'] = (48, 456)
        with self.assertRaisesRegex(ValueError, 'borrowed'):
            save.check_owned_progress(initial, current)
        record = bytearray.fromhex(initial['party'][0]['bytes'])
        record[32] ^= 1
        with self.assertRaisesRegex(ValueError, 'checksum'):
            save.pokemon(record)


if __name__ == '__main__':
    unittest.main()
