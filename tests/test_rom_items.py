# SPDX-License-Identifier: GPL-3.0-or-later
"""ROM-free patch boundaries and execution of the authored item HP synchronization correction."""
import itertools
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from romfixes import pm045_items as fix

try:
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_MEM_WRITE, UC_HOOK_CODE
    from unicorn.arm_const import (UC_ARM_REG_SP, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4,
        UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
        UC_ARM_REG_R10, UC_ARM_REG_R11)
except ImportError:
    Uc = None


class ItemPatchBoundaries(unittest.TestCase):
    @staticmethod
    def fixture():
        start, table, fat = 0x2000, 0x200, 0x1400
        data = bytearray(start + fix.OVERLAY_ENTRY[2])
        struct.pack_into('<II', data, 0x50, table, 123 * 32)
        struct.pack_into('<II', data, 0x48, fat, 123 * 8)
        struct.pack_into('<8I', data, table + 122 * 32, *fix.OVERLAY_ENTRY)
        struct.pack_into('<II', data, fat + 122 * 8, start, len(data))
        for address, value in fix.LITERALS:
            struct.pack_into('<I', data, start + address - fix.OVERLAY_ENTRY[1], value)
        result = bytearray(data)
        offset = start + fix.CODE_START - fix.OVERLAY_ENTRY[1]
        for address, code in fix.PATCHES:
            at = start + address - fix.OVERLAY_ENTRY[1]
            result[at:at + len(code)] = code
        constants = dict(ROM_SIZE=len(data), SOURCE_SHA256=fix.digest(data),
            OVERLAY_SHA256=fix.digest(data[start:]), TARGET_SHA256=fix.digest(result))
        return bytes(data), bytes(result), constants, offset

    def test_only_the_two_authored_regions_change(self):
        data, expected, constants, offset = self.fixture()
        with patch.multiple(fix, **constants):
            self.assertEqual(fix.patched_rom(data), expected)
        self.assertEqual(len(fix.CODE), fix.CODE_END - fix.CODE_START)
        allowed = set()
        for address, code in fix.PATCHES:
            at = 0x2000 + address - fix.OVERLAY_ENTRY[1]
            allowed.update(range(at, at + len(code)))
        self.assertTrue(all(i in allowed for i, (a, b)
            in enumerate(zip(data, expected)) if a != b))

    def test_original_and_existing_output_are_preserved(self):
        data, expected, constants, _ = self.fixture()
        with tempfile.TemporaryDirectory() as folder, patch.multiple(fix, **constants):
            source = Path(folder) / 'Platin – Zielfix.nds'
            target = Path(folder) / 'Platin – Koopfix.nds'
            source.write_bytes(data)
            with self.assertRaises(ValueError):
                fix.write_new_rom(source, source)
            fix.write_new_rom(source, target)
            self.assertEqual(source.read_bytes(), data)
            self.assertEqual(target.read_bytes(), expected)
            with self.assertRaises(FileExistsError):
                fix.write_new_rom(source, target)
            self.assertEqual(target.read_bytes(), expected)

    def test_unknown_or_already_patched_input_creates_no_output(self):
        data, expected, constants, _ = self.fixture()
        with tempfile.TemporaryDirectory() as folder, patch.multiple(fix, **constants):
            source, target = Path(folder) / 'in.nds', Path(folder) / 'out.nds'
            for wrong in (data[:-1], b'X' + data[1:], expected):
                source.write_bytes(wrong)
                with self.assertRaises(ValueError):
                    fix.write_new_rom(source, target)
                self.assertFalse(target.exists())

    def test_mismatched_overlay_is_rejected(self):
        data, _, constants, _ = self.fixture()
        with patch.multiple(fix, **{**constants, 'OVERLAY_SHA256': '0' * 64}):
            with self.assertRaisesRegex(ValueError, 'Koop-Overlay'):
                fix.patched_rom(data)

    def test_final_fingerprint_is_required(self):
        data, _, constants, _ = self.fixture()
        with patch.multiple(fix, **{**constants, 'TARGET_SHA256': '0' * 64}):
            with self.assertRaisesRegex(ValueError, 'Ergebnisprüfung'):
                fix.patched_rom(data)


@unittest.skipIf(Uc is None, 'ARM engine missing: install tests/requirements-romfix.txt')
class ItemHpArmExecution(unittest.TestCase):
    def execute(self, remote_slot, command, current, received, species, resync, maximum=23):
        cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        cpu.mem_map(0x02280000, 0x20000)
        cpu.mem_map(0x022c0000, 0x10000)
        cpu.mem_map(0x023f0000, 0x1000)
        cpu.mem_write(fix.CODE_START, fix.CODE)
        for address, value in fix.LITERALS:
            cpu.mem_write(address, struct.pack('<I', value))
        # Test-only stand-in for the existing remote-slot getter.
        cpu.mem_write(0x0228d540, bytes([remote_slot, 0x20, 0x70, 0x47]))
        context, stack = 0x022c0000, 0x023f0800
        target = context + remote_slot * 0xc0
        cpu.mem_write(target + 0x2d80, struct.pack('<H', 387))
        cpu.mem_write(target + 0x2dcc, struct.pack('<iI', current, maximum))
        cpu.mem_write(0x0229d5c8, struct.pack('<H', received))
        cpu.mem_write(0x0229d5ce, struct.pack('<H', species))
        cpu.mem_write(0x0229d55c, bytes([command]))
        cpu.mem_write(0x0229ce54, struct.pack('<I', resync))
        preserved = {UC_ARM_REG_R4: 0x44444444, UC_ARM_REG_R5: context,
            UC_ARM_REG_R8: 0x88888888,
            UC_ARM_REG_R9: 0x99999999, UC_ARM_REG_R10: 0xaaaaaaaa,
            UC_ARM_REG_R11: 0xbbbbbbbb, UC_ARM_REG_SP: stack}
        for register, value in preserved.items():
            cpu.reg_write(register, value)
        writes = []
        cpu.hook_add(UC_HOOK_MEM_WRITE,
            lambda _cpu, _access, address, size, value, _data:
                writes.append((address, size)))
        cpu.emu_start(fix.CODE_START | 1, fix.CODE_END, count=100)
        hp = struct.unpack('<i', cpu.mem_read(target + 0x2dcc, 4))[0]
        for register, value in preserved.items():
            self.assertEqual(cpu.reg_read(register), value)
        self.assertTrue(all(address == target + 0x2dcc and size == 4
            for address, size in writes), writes)
        return hp, bool(writes)

    def test_healing_and_safety_guards_for_both_remote_slots(self):
        # 320 actual CPU executions: both roles, item/move commands,
        # living/fainted receivers, HP limits, species and resync ownership.
        cases = itertools.product((0, 2), (13, 14), (-1, 0, 10, 23),
            (0, 11, 23, 24, 65535), (387, 396), (0, 1))
        for slot, command, current, received, species, resync in cases:
            with self.subTest(slot=slot, command=command, current=current,
                    received=received, species=species, resync=resync):
                hp, wrote = self.execute(slot, command, current, received, species, resync)
                apply = command != 14 and current > 0 and received > 0 and species == 387 and not resync
                self.assertEqual(hp, min(received, 23) if apply else current)
                self.assertEqual(wrote, bool(apply))

    def test_native_item_handler_remains_the_only_healer(self):
        # Non-clamped heal catches the rejected double-healing approach.
        for slot in (0, 2):
            hp, wrote = self.execute(slot, 14, 42, 62, 387, 0, maximum=67)
            self.assertEqual(hp, 42)
            self.assertFalse(wrote)


@unittest.skipIf(Uc is None, 'ARM engine missing: install tests/requirements-romfix.txt')
class ItemBarrierArmExecution(unittest.TestCase):
    def execute(self, slot, depth, parent_narc, parent_script, original_result):
        cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        cpu.mem_map(0x02280000, 0x20000)
        cpu.mem_map(0x022c0000, 0x10000)
        cpu.mem_map(0x023f0000, 0x1000)
        cpu.mem_write(fix.CODE_START, fix.CODE)
        cpu.mem_write(fix.HOOK_START, fix.HOOK)
        # Test-only original enablement callee, no Nintendo implementation.
        cpu.mem_write(fix.ORIGINAL_SYNC, bytes([original_result, 0x20, 0x70, 0x47]))
        context, system, stack = 0x022c0000, 0x022d0000, 0x023f0800
        for offset, value in ((0xb8, depth), (0xbc, parent_narc), (0xcc, parent_script)):
            cpu.mem_write(context + offset, struct.pack('<I', value))
        cpu.reg_write(UC_ARM_REG_R0, system)
        cpu.reg_write(UC_ARM_REG_R1, context)
        cpu.reg_write(UC_ARM_REG_R2, slot)
        preserved = {UC_ARM_REG_R3: 0x33333333, UC_ARM_REG_R4: system,
            UC_ARM_REG_R5: slot, UC_ARM_REG_R6: context,
            UC_ARM_REG_R7: 0x77777777, UC_ARM_REG_R8: 0x88888888,
            UC_ARM_REG_R9: 0x99999999, UC_ARM_REG_R10: 0xaaaaaaaa,
            UC_ARM_REG_R11: 0xbbbbbbbb, UC_ARM_REG_SP: stack}
        for register, value in preserved.items():
            cpu.reg_write(register, value)
        called, writes = [], []
        def visit(machine, address, _size, _user):
            if address == fix.ORIGINAL_SYNC:
                called.append(machine.reg_read(UC_ARM_REG_R0))
        cpu.hook_add(UC_HOOK_CODE, visit)
        cpu.hook_add(UC_HOOK_MEM_WRITE,
            lambda _cpu, _access, address, size, _value, _data:
                writes.append((address, size)))
        # Execute the actual authored BL, filter and return at their real
        # addresses to check relocation, stack balance and saved registers.
        cpu.emu_start(fix.HOOK_START | 1, fix.HOOK_START + 4, count=100)
        for register, value in preserved.items():
            self.assertEqual(cpu.reg_read(register), value)
        self.assertTrue(all(stack - 8 <= address and address + size <= stack
            for address, size in writes), writes)
        return cpu.reg_read(UC_ARM_REG_R0), called

    def test_only_direct_remote_item_hp_subroutine_skips_its_extra_barrier(self):
        # 240 CPU cases. Depth zero rejects stale parent metadata; deeper
        # calls and other NARCs/scripts retain the original sync guards.
        for slot, depth, narc, parent, original in itertools.product(
                (0, 2), (0, 1, 2, 4), (0, 1, 2), (0, 2, 304, 305, 306), (0, 1)):
            with self.subTest(slot=slot, depth=depth, narc=narc,
                    parent=parent, original=original):
                result, called = self.execute(slot, depth, narc, parent, original)
                skip = depth == 1 and narc == 1 and parent == 305
                self.assertEqual(result, 0 if skip else original)
                self.assertEqual(called, [] if skip else [0x022d0000])

    def test_next_damage_keeps_the_same_hp_sequence_on_both_peers(self):
        for slot in (0, 2):
            remote_item, _ = self.execute(slot, 1, 1, 305, 1)
            next_hit, called = self.execute(slot, 1, 1, 1, 1)
            self.assertEqual(5 + next_hit, 5 + remote_item + next_hit)
            self.assertEqual(called, [0x022d0000])
