#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Execute self-authored ARM GBA programs in the production worker.

No Nintendo logo, game content, BIOS or firmware bytes. Tests SRAM/Flash/EEPROM save
devices, isolated input, restart persistence, save rejection and IPC shutdown.
"""
from pathlib import Path
import json
import mmap
import os
import struct
import subprocess
import sys
import tempfile
import time


def homebrew(kind='sram'):
    code = []; literals = []; loads = []; branches = []; labels = {}
    def emit(word): code.append(word)
    def load(reg, value):
        loads.append((len(code), reg, len(literals))); emit(0); literals.append(value)
    def branch(label, condition=14):
        branches.append((len(code), label, condition)); emit(0)
    def write_byte(address, value):
        load(0, address); emit(0xE3A01000 | value); emit(0xE5C01000)
    def flash_command(command):
        write_byte(0x0E005555, 0xAA); write_byte(0x0E002AAA, 0x55); write_byte(0x0E005555, command)
    eeprom = kind.startswith('eeprom')
    address_bits = 14 if kind == 'eeprom8k' else 6
    block = 64 if kind == 'eeprom8k' else 0
    bits = [(block >> shift) & 1 for shift in reversed(range(address_bits))]
    streams = {0x1000: [1, 1] + bits + [0]}
    for offset, value in ((0x1100, 0x20), (0x1200, 0x5A)):
        streams[offset] = [1, 0] + bits + [(value >> shift) & 1 for _ in range(8) for shift in reversed(range(8))] + [0]
    def dma(source, destination, control):
        for address, value in ((0x040000D4, source), (0x040000D8, destination), (0x040000DC, control)):
            load(0, address); load(1, value); emit(0xE5801000)
    def store(value=0x20):
        if eeprom:
            offset = 0x1100 if value == 0x20 else 0x1200
            dma(0x08000000 + offset, 0x0D000000, 0x80400000 | len(streams[offset]))
            return
        if kind.startswith('flash'): flash_command(0xA0)
        if kind != 'none': emit(0xE5C67000)  # STRB r7,[r6]
    load(0, 0x04000000); load(1, 0x403); emit(0xE1C010B0)  # Mode 3 / BG2
    load(5, 0x04000130); load(6, 0x0E000000)
    if kind.startswith('flash'):
        flash_command(0x90); flash_command(0xF0)
        if kind == 'flash128':
            flash_command(0xB0); write_byte(0x0E000000, 1)
            flash_command(0xB0); write_byte(0x0E000000, 0)
    if eeprom:
        dma(0x08001000, 0x0D000000, 0x80400000 | len(streams[0x1000]))
        dma(0x0D000000, 0x02000000, 0x81000000 | 68)
        load(2, 0x02000008); emit(0xE3A07000)
        for _ in range(8):
            emit(0xE0D210B2); emit(0xE1A07087); emit(0xE1877001)
        # The authored game recognises exactly two progress markers.
        emit(0xE3570020); branch('initialised', 0)
        emit(0xE357005A); branch('initialised', 0)
        emit(0xE3A07020); store()
        labels['initialised'] = len(code)
    elif kind == 'none': emit(0xE3A07020)
    else:
        emit(0xE5D67000); emit(0xE35700FF); emit(0x03A07020)  # Load battery byte, initialise FF.
        store()
    labels['loop'] = len(code)
    load(4, 0x06000000); load(3, 240 * 160)
    labels['draw'] = len(code)
    emit(0xE0C470B2); emit(0xE2533001); branch('draw', 1)  # STRH postincrement / SUBS / BNE
    emit(0xE1D580B0); emit(0xE3180001); branch('loop', 1)  # KEYINPUT, active-low A.
    emit(0xE3A0705A); store(0x5A); branch('loop')
    for index, reg, slot in loads:
        delta = 4 * (len(code) + slot - index) - 8
        assert 0 <= delta < 4096
        code[index] = 0xE59F0000 | reg << 12 | delta
    for index, label, condition in branches:
        code[index] = condition << 28 | 0x0A000000 | ((labels[label] - index - 2) & 0xFFFFFF)
    # Larger than multiboot RAM so mGBA maps the program as a cartridge ROM.
    rom = bytearray(1024 * 1024)
    struct.pack_into('<I', rom, 0, 0xEA00007E)  # Branch to our own code at 0x200.
    rom[0xA0:0xAC] = b'COUCH TEST  '
    rom[0xAC:0xB0] = b'CSHT'; rom[0xB0:0xB2] = b'00'; rom[0xB2] = 0x96
    rom[0xBD] = (-sum(rom[0xA0:0xBD]) - 0x19) & 255
    payload = struct.pack('<' + 'I' * (len(code) + len(literals)), *(code + literals))
    rom[0x200:0x200 + len(payload)] = payload
    if eeprom:
        for offset, stream in streams.items():
            raw = struct.pack('<' + 'H' * len(stream), *stream)
            rom[offset:offset + len(raw)] = raw
    return rom


def smoke(binary, output=None):
    binary = Path(binary).resolve()
    features = subprocess.check_output([str(binary), '--coop-capabilities'], timeout=5)
    assert b'GBA=1' in features and b'STABLE_SAVE=1' in features
    report = {'production_worker': True, 'self_authored_arm': True, 'kinds': [], 'world_coop': False}
    with tempfile.TemporaryDirectory(prefix='couch-gba-ä-') as temp:
        root = Path(temp)
        for kind, expected_size in (('sram', 32768), ('flash64', 65536), ('flash128', 131072), ('eeprom512', 512), ('eeprom8k', 8192), ('none', 0)):
            rom = root / (kind + '.gba'); rom.write_bytes(homebrew(kind))
            profiles = [root / (kind + str(p)) for p in range(2)]
            for profile in profiles: (profile / 'saves').mkdir(parents=True)
            def run(restart=False):
                children = []; files = []; memories = []; logs = []
                try:
                    for p, profile in enumerate(profiles):
                        ipc = profile / 'control.ipc'
                        ipc.write_bytes(struct.pack('<10I', 0x434F5031, 2, 0, 0xFFF, 0, 0, 0, 0, 0, 0) + bytes(256 * 192 * 4 * 2))
                        file = ipc.open('r+b'); files.append(file); memories.append(mmap.mmap(file.fileno(), 0))
                        env = {k: v for k, v in os.environ.items() if not k.startswith(('PMCOOP_', 'MELONDS_AP'))}
                        env.update(PMCOOP_IPC=str(ipc), PMCOOP_PROFILE=str(profile), SDL_AUDIODRIVER='dummy')
                        log = (profile / ('restart.log' if restart else 'first.log')).open('wb'); logs.append(log)
                        children.append(subprocess.Popen([str(binary), str(rom)], env=env, stdout=log, stderr=subprocess.STDOUT))
                    def until(predicate):
                        end = time.monotonic() + 8
                        while time.monotonic() < end:
                            assert all(c.poll() is None for c in children), 'Worker exited early'
                            if predicate(): return
                            time.sleep(.02)
                        raise AssertionError(('Expected GBA frame/save did not arrive', kind, restart, [frames(p) for p in range(2)], [hex(pixel(p)) for p in range(2)]))
                    def pixel(p): return struct.unpack_from('<I', memories[p], 40)[0]
                    def frames(p): return struct.unpack_from('<I', memories[p], 20)[0]
                    until(lambda: all(frames(p) > 10 and pixel(p) not in (0, 0xFF000000) for p in range(2)))
                    if restart and kind != 'none': assert pixel(0) != pixel(1), 'Saved progress was not loaded'
                    if not restart:
                        until(lambda: pixel(0) == pixel(1))
                        initial = pixel(0)
                        struct.pack_into('<I', memories[0], 12, 0xFFE)  # Only player 1 presses A.
                        until(lambda: pixel(0) != initial and pixel(1) == initial)
                        struct.pack_into('<I', memories[0], 12, 0xFFF)
                    for memory in memories: struct.pack_into('<I', memory, 36, 1)
                    for child in children: assert child.wait(timeout=5) == 0
                except Exception:
                    for log in logs: log.flush()
                    for profile in profiles:
                        for path in profile.glob('*.log'): print(path.read_text(errors='replace')[-3000:])
                    raise
                finally:
                    for child in children:
                        if child.poll() is None: child.terminate(); child.wait(timeout=5)
                    for memory in memories: memory.close()
                    for file in files: file.close()
                    for log in logs: log.close()
            run()
            before = []
            for p, profile in enumerate(profiles):
                save = profile / 'saves/game.sav'
                if not expected_size: assert not save.exists(); continue
                data = save.read_bytes(); assert len(data) == expected_size
                offset = 512 if kind == 'eeprom8k' else 0
                assert data[offset] == (0x5A if p == 0 else 0x20), (kind, p, data[offset:offset+8].hex())
                before.append(data)
            run(True)
            for p, profile in enumerate(profiles):
                if expected_size: assert (profile / 'saves/game.sav').read_bytes() == before[p]
            # Reject unsupported save sizes before emulation and preserve the file.
            corrupt = profiles[0] / 'saves/game.sav'; corrupt.write_bytes(b'not a battery save')
            env = dict(os.environ, PMCOOP_IPC=str(profiles[0] / 'control.ipc'), PMCOOP_PROFILE=str(profiles[0]))
            failed = subprocess.run([str(binary), str(rom)], env=env, capture_output=True, timeout=5)
            assert failed.returncode != 0 and corrupt.read_bytes() == b'not a battery save'
            report['kinds'].append({'kind': kind, 'save_bytes': expected_size, 'restart': True, 'input_isolation': True})
    if output: Path(output).write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: production GBA worker executed authored ARM programs; SRAM, Flash 64/128 KiB, EEPROM 512 B/8 KiB saves, isolated input, restart persistence, no-save games, invalid-save rejection and clean shutdown')


if __name__ == '__main__': smoke(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
