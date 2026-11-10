#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Linux-only, opt-in real-ROM paired Session crash/lock/save boundaries.

Uses the production launcher test driver and worker. Blank authored Flash images
are test data, never existing user saves. Does not certify shared-world gameplay.
"""
import hashlib
import json
import os
from pathlib import Path
import select
import signal
import subprocess
import sys
import tempfile
import time

SUPPORTED_ROMS = {
    'eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507': 'German',
    '3d0c79f1627022e18765766f6cb5ea067f6b5bf7dca115552189ad65a5c3a8ac': 'English 1.0',
    '729041b940afe031302d630fdbe57c0c145f3f7b6d9b8eca5e98678d0ca4d059': 'English 1.1',
}


def smoke(driver, worker, rom, output=None):
    assert sys.platform.startswith('linux'), 'This test uses owned Linux /proc children'
    driver, worker, rom = (Path(value).resolve() for value in (driver, worker, rom))
    rom_hash = hashlib.sha256(rom.read_bytes()).hexdigest()
    assert rom_hash in SUPPORTED_ROMS
    with tempfile.TemporaryDirectory(prefix='couch-firered-session-ä-') as temporary:
        root = Path(temporary)
        profile, captures = root / 'profiles', root / 'captures'
        authored = [bytes([255]) * 131071 + bytes([marker]) for marker in (0xA5, 0x5A)]
        save_paths = [profile / 'players' / str(player + 1) / rom_hash / 'saves/game.sav' for player in range(2)]
        for path, data in zip(save_paths, authored):
            path.parent.mkdir(parents=True)
            path.write_bytes(data)
        env = {key: value for key, value in os.environ.items() if not key.startswith(('PMCOOP_', 'MELONDS_AP'))}
        env.update(COUCHSPLIT_FIRERED_EXPERIMENT='1', QT_QPA_PLATFORM='offscreen', SDL_AUDIODRIVER='dummy')
        process = subprocess.Popen([str(driver), str(rom), str(worker), str(profile), str(captures), '-'],
                                   env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        transcript = bytearray()

        def until(message, timeout=15):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                if message in transcript:
                    return
                ready, _, _ = select.select([process.stdout], [], [], min(.2, max(0, deadline - time.monotonic())))
                if ready:
                    block = os.read(process.stdout.fileno(), 65536)
                    if not block:
                        break
                    transcript.extend(block)
            raise AssertionError(transcript.decode(errors='replace'))

        child_pid = None
        try:
            until(b'Ready for private acceptance actions')
            # At startup the launcher captures the two already saved battery
            # files together, without claiming they were saved at one frame.
            pairs = list((profile / 'paired-backups' / rom_hash).glob('*/manifest.json'))
            assert len(pairs) == 1, 'Launcher did not create a paired backup'
            snapshot = pairs[0].parent
            metadata = json.loads(pairs[0].read_text())
            assert metadata['rom'] == rom_hash and metadata['format'] == 'couchsplit-firered-pair-1'
            for index, data in enumerate(authored, 1):
                assert (snapshot / f'player{index}.sav').read_bytes() == data
                assert metadata[f'player{index}'] == hashlib.sha256(data).hexdigest()
            process.stdin.write(b'{"action":"wait","ms":2500}\n')
            process.stdin.flush()
            until(b'Action complete')
            expected_audio = env.get('COOP_VERIFY_FIRERED_AUDIO', 'player1')
            log = next((profile / 'logs').glob('*/player1.log')).read_text()
            assert f'FireRed pair: audio mode {expected_audio}' in log, 'Selected audio did not reach paired worker'
            # Select only an exact direct child with our requested executable.
            children = Path(f'/proc/{process.pid}/task/{process.pid}/children').read_text().split()
            owned = [int(pid) for pid in children if Path(f'/proc/{pid}/exe').resolve() == worker]
            assert len(owned) == 1, ('Expected one paired child', children)
            child_pid = owned[0]
            assert (profile / 'session.lock').exists(), 'Launcher released the live session lock'
            before = [path.read_bytes() for path in save_paths]
            os.kill(child_pid, signal.SIGKILL)  # This test's verified direct child only.
            process.stdin.write(b'{"action":"wait","ms":1000}\n')
            process.stdin.flush()
            rest = process.communicate(timeout=10)[0]
            transcript.extend(rest)
            assert process.returncode != 0, transcript.decode(errors='replace')
            assert b'Acceptance failed:' in transcript, transcript.decode(errors='replace')
            assert not (profile / 'session.lock').exists(), 'Paired failure leaked the save lock'
            assert not Path(f'/proc/{child_pid}').exists(), 'Paired worker was orphaned'
            assert [path.read_bytes() for path in save_paths] == before == authored, 'Crash overwrote either prior save'
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.communicate(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.communicate(timeout=5)
    assert hashlib.sha256(rom.read_bytes()).hexdigest() == rom_hash
    report = {'rom_sha256': rom_hash, 'rom_version': SUPPORTED_ROMS[rom_hash], 'production_session': True, 'paired_worker': True, 'owned_child_failure_detected': True,
              'lock_released_after_child_exit': True, 'both_prior_saves_preserved': True,
              'paired_startup_backup_verified': True,
              'audio_setting_reached_worker': expected_audio,
              'rom_unchanged': True, 'complete_world_coop': False}
    if output:
        Path(output).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    smoke(*sys.argv[1:])
