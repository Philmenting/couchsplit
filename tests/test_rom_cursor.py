# SPDX-License-Identifier: GPL-3.0-or-later
"""ROM-free checks of the patch boundary and execution of our actual ARM code.

Install tests/requirements-romfix.txt for the required ARM regression job.
No original ROM instructions, text, graphics, or saves are test fixtures.
"""
import itertools
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from romfixes import pm045_target as fix

try:
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_MEM_WRITE
    from unicorn.arm_const import (UC_ARM_REG_SP, UC_ARM_REG_PC, UC_ARM_REG_R4,
        UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R12)
except ImportError:
    Uc = None


class PatchBoundaries(unittest.TestCase):
    @staticmethod
    def fixture():
        """Synthetic DS-shaped bytes containing only zeroes and our headers."""
        start, table, fat = 0x1000, 0x200, 0x500
        data = bytearray(start + 235360)
        struct.pack_into('<II', data, 0x50, table, 17 * 32)
        struct.pack_into('<II', data, 0x48, fat, 17 * 8)
        struct.pack_into('<8I', data, table + 16 * 32,
            16, fix.OVERLAY_ADDRESS, 235360, 64, 0x02283D9C, 0x02283DA4, 16, 0)
        struct.pack_into('<II', data, fat + 16 * 8, start, len(data))
        expected = bytearray(data)
        for address, code in fix.CHANGES:
            offset = start + address - fix.OVERLAY_ADDRESS
            expected[offset:offset + len(code)] = code
        constants = dict(ROM_SIZE=len(data), SOURCE_SHA256=fix.digest(data),
            TARGET_SHA256=fix.digest(expected), OVERLAY_SHA256=fix.digest(data[start:]))
        return bytes(data), bytes(expected), constants

    def test_unknown_input_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Prüfsumme'):
            fix.patched_rom(b'not a verified ROM')

    def test_only_two_owned_instruction_regions_change(self):
        data, expected, constants = self.fixture()
        with patch.multiple(fix, **constants):
            self.assertEqual(fix.patched_rom(data), expected)
        changed = {i for i, (a, b) in enumerate(zip(data, expected)) if a != b}
        allowed = {0x1000 + address - fix.OVERLAY_ADDRESS + i
            for address, code in fix.CHANGES for i in range(len(code))}
        self.assertTrue(changed)
        self.assertLessEqual(changed, allowed)

    def test_destination_and_source_are_preserved(self):
        data, expected, constants = self.fixture()
        with tempfile.TemporaryDirectory() as folder, patch.multiple(fix, **constants):
            source, target = Path(folder)/'Platin – Ä.nds', Path(folder)/'Korrigiert – Ü.nds'
            source.write_bytes(data)
            with self.assertRaises(ValueError):
                fix.write_new_rom(source, source)
            self.assertFalse(target.exists())
            fix.write_new_rom(source, target)
            self.assertEqual(source.read_bytes(), data)
            self.assertEqual(target.read_bytes(), expected)
            with self.assertRaises(FileExistsError):
                fix.write_new_rom(source, target)
            self.assertEqual(target.read_bytes(), expected)

    def test_wrong_or_already_patched_rom_creates_no_output(self):
        data, expected, constants = self.fixture()
        with tempfile.TemporaryDirectory() as folder, patch.multiple(fix, **constants):
            source, target = Path(folder)/'Platin – Ä.nds', Path(folder)/'Korrigiert – Ü.nds'
            for wrong in [data[:-1], bytes([1]) + data[1:], expected]:
                source.write_bytes(wrong)
                with self.assertRaises(ValueError):
                    fix.write_new_rom(source, target)
                self.assertFalse(target.exists())

    def test_final_fingerprint_is_checked(self):
        data, _, constants = self.fixture()
        constants['TARGET_SHA256'] = '0' * 64
        with patch.multiple(fix, **constants), self.assertRaisesRegex(ValueError, 'Ergebnisprüfung'):
            fix.patched_rom(data)


@unittest.skipIf(Uc is None, 'ARM engine missing: install tests/requirements-romfix.txt')
class CursorArmExecution(unittest.TestCase):
    def execute(self, mask, x, y, same_layout):
        cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        cpu.mem_map(0x02270000, 0x10000)
        for address, code in fix.CHANGES:
            cpu.mem_write(address, code)
        cpu.mem_write(0x0227F43C, struct.pack('<I', 0x68c))
        cpu.mem_map(0x02000000, 0x10000)
        sp, subscreen, cursor, saved = 0x02001000, 0x02002000, 0x02003000, 0x02004000
        cpu.mem_write(sp + 0x1c, struct.pack('<I', saved))
        cpu.mem_write(saved, bytes([0, 0, 0, 0, x, y, 0 if same_layout else 255, 0]))
        cpu.mem_write(subscreen + 0x68c, b'\0')
        cpu.mem_write(sp + 0x50, bytes([3, 1, 0, 2, 4, 4]))
        cpu.mem_write(sp + 0x5a, bytes(bool(mask & (1 << i)) for i in range(4)))
        preserved = {UC_ARM_REG_SP: sp, UC_ARM_REG_R4: 0x12345678,
            UC_ARM_REG_R5: 0xabcdef01, UC_ARM_REG_R6: subscreen,
            UC_ARM_REG_R7: cursor, UC_ARM_REG_R12: 2}
        for register, value in preserved.items():
            cpu.reg_write(register, value)
        writes = []
        cpu.hook_add(UC_HOOK_MEM_WRITE, lambda uc, access, address, size, value, user: writes.append((address, size)))
        cpu.emu_start(fix.CURSOR_START | 1, fix.CURSOR_END, count=100)
        self.assertEqual(cpu.reg_read(UC_ARM_REG_PC), fix.CURSOR_END, 'must reach the original cursor renderer')
        for register, value in preserved.items():
            self.assertEqual(cpu.reg_read(register), value)
        self.assertEqual(set(writes), {(cursor + 1, 1), (cursor + 2, 1)})
        result = cpu.mem_read(cursor, 4)
        return result[2], result[1]

    def test_all_presence_masks_and_previous_targets(self):
        slots = [3, 1, 0, 2]  # original screen order: two enemies, then own side
        for mask, x, y, same in itertools.product(range(16), range(2), range(2), [False, True]):
            with self.subTest(mask=mask, x=x, y=y, same=same):
                old = slots[y * 2 + x]
                if same and mask & (1 << old):
                    expected = (x, y)
                else:
                    expected = next(((i % 2, i // 2) for i, slot in enumerate(slots)
                        if mask & (1 << slot)), (1, 1))
                self.assertEqual(self.execute(mask, x, y, same), expected)

    def test_dead_left_enemy_moves_to_living_right_enemy(self):
        self.assertEqual(self.execute(0b0010, 0, 0, True), (1, 0))

    def test_valid_previous_right_enemy_is_kept(self):
        self.assertEqual(self.execute(0b1010, 1, 0, True), (1, 0))


if __name__ == '__main__':
    unittest.main()
