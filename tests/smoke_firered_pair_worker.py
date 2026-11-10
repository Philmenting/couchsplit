#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Opt-in real-ROM pair worker boundary/boot/lifecycle checks.

Requires an exact supported original; never copies it, accepts saves or
checkpoints, or claims that booting the introduction verifies team gameplay.
"""
import hashlib
import json
import mmap
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time


SUPPORTED_ROMS = {
    'eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507': 'German',
    '3d0c79f1627022e18765766f6cb5ea067f6b5bf7dca115552189ad65a5c3a8ac': 'English 1.0',
    '729041b940afe031302d630fdbe57c0c145f3f7b6d9b8eca5e98678d0ca4d059': 'English 1.1',
}
IPC_BYTES = struct.pack('<10I', 0x434F5031, 2, 0, 0xFFF, 0, 0, 0, 0, 0, 0) + bytes(256 * 192 * 8)


def smoke(binary, rom, report_path=None):
    binary, rom = Path(binary).resolve(), Path(rom).resolve()
    original_hash = hashlib.sha256(rom.read_bytes()).hexdigest()
    assert original_hash in SUPPORTED_ROMS, 'Requires an exact verified original'
    assert b'FIRERED_PAIR_EXPERIMENT=1' in subprocess.check_output([str(binary), '--coop-capabilities'], timeout=5)
    base_env = {k: v for k, v in os.environ.items() if not k.startswith(('PMCOOP_', 'MELONDS_AP'))}
    base_env.update(PMCOOP_FIRERED_PAIR='1', PMCOOP_FIRERED_RECOVERY='1', SDL_AUDIODRIVER='dummy')
    report = {'rom_sha256': original_hash, 'rom_version': SUPPORTED_ROMS[original_hash], 'experimental_pair_worker': True, 'team_gameplay_verified': False}
    with tempfile.TemporaryDirectory(prefix='couch-firered-pair-ä-') as temp:
        root = Path(temp)
        profiles = [root / ('player-' + str(i)) for i in range(2)]
        ipcs = [root / ('player-' + str(i) + '.ipc') for i in range(2)]
        for profile in profiles: (profile / 'saves').mkdir(parents=True)
        for ipc in ipcs: ipc.write_bytes(IPC_BYTES)
        env = dict(base_env, PMCOOP_PROFILE=str(profiles[0]), PMCOOP_PROFILE2=str(profiles[1]),
                   PMCOOP_IPC=str(ipcs[0]), PMCOOP_IPC2=str(ipcs[1]))

        def rejected(case_env, game=rom):
            result = subprocess.run([str(binary), str(game)], env=case_env, capture_output=True, timeout=8)
            assert result.returncode != 0, result.stdout.decode(errors='replace')
            return result.stderr.decode(errors='replace')

        assert 'matching launcher' in rejected(dict(env, PMCOOP_FIRERED_RECOVERY='0'))
        assert 'distinct save profiles' in rejected(dict(env, PMCOOP_PROFILE2=str(profiles[0])))
        assert 'distinct IPC' in rejected(dict(env, PMCOOP_IPC2=str(ipcs[0])))
        assert all(not (p / 'saves/game.sav').exists() for p in profiles)
        # Valid normally-supported GBA sizes are still wrong for this exact
        # Flash128 title. Both players' existing files must remain unchanged.
        for size in (7, 32768, 65536):
            corrupt = profiles[1] / 'saves/game.sav'
            data = b'\xA5' * size; corrupt.write_bytes(data)
            assert 'Invalid FireRed battery save size' in rejected(env)
            assert corrupt.read_bytes() == data
            assert not (profiles[0] / 'saves/game.sav').exists()
            corrupt.unlink()
        from smoke_gba import homebrew
        wrong = root / 'authored.gba'; wrong.write_bytes(homebrew())
        assert 'exact verified' in rejected(env, wrong)
        report['boundary_rejections'] = ['old_launcher_protocol', 'same_profile', 'same_ipc', 'corrupt_save', 'wrong_flash_size', 'wrong_rom']

        # Authored invalid/blank Flash images, never user saves. Distinct tail
        # markers exercise separately loaded VFileMemChunk buffers and ensure
        # one core's initial/restart snapshot cannot overwrite the other.
        seeded = [bytes([255]) * 131071 + bytes([marker]) for marker in (0xA5, 0x5A)]
        for profile, data in zip(profiles, seeded): (profile / 'saves/game.sav').write_bytes(data)

        def run(shutdown_player, terminate=False):
            for ipc in ipcs: ipc.write_bytes(IPC_BYTES)
            files = [ipc.open('r+b') for ipc in ipcs]
            memories = [mmap.mmap(file.fileno(), 0) for file in files]
            process = subprocess.Popen([str(binary), str(rom)], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 12
                while time.monotonic() < deadline:
                    assert process.poll() is None, process.communicate()[0].decode(errors='replace')
                    frames = [struct.unpack_from('<I', memory, 20)[0] for memory in memories]
                    if min(frames) >= 180: break
                    time.sleep(.03)
                else: raise AssertionError('Both real emulator contexts did not publish intro frames')
                # Only one IPC owns this input. Native intro animation is not
                # itself a controlled world/gameplay input-isolation proof.
                for press in range(16):
                    struct.pack_into('<I', memories[0], 12, 0xFFE if press % 2 == 0 else 0xFFF)
                    time.sleep(.06)
                if terminate: process.terminate()
                else: struct.pack_into('<I', memories[shutdown_player], 36, 1)
                output = process.communicate(timeout=8)[0].decode(errors='replace')
                assert process.returncode == 0, output
                assert 'stopped cleanly' in output, output
                assert min(frames) >= 180
                return frames
            finally:
                if process.poll() is None: process.kill(); process.communicate(timeout=5)
                for memory in memories: memory.close()
                for file in files: file.close()

        report['boot_frames'] = run(1)
        saved = [(p / 'saves/game.sav').read_bytes() for p in profiles]
        assert saved == seeded, 'Separate authored Flash markers changed or crossed between cores'
        run(0, terminate=True)
        assert [(p / 'saves/game.sav').read_bytes() for p in profiles] == saved
        report.update(two_core_intro=True, separate_flash_buffers=True, restart=True, second_ipc_shutdown=True, sigterm_shutdown=True)
    assert hashlib.sha256(rom.read_bytes()).hexdigest() == original_hash
    report['rom_unchanged'] = True
    if report_path: Path(report_path).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__': smoke(*sys.argv[1:])
