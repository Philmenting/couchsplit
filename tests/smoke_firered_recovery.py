#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Opt-in production Session/PlayView/worker recovery with private native saves.

Create the two authored Route 1 saves using verify-firered-pair and scenario
exp-share-enigma-launcher. EXP Share overflow now succeeds in shared battles.
No ROM/saves are shipped.
This targeted acceptance uses virtual SDL controllers, not physical hardware.
"""
import hashlib
import json
import os
from pathlib import Path
from queue import Empty, Queue
import subprocess
import sys
import tempfile
from threading import Thread
import time

ROM_HASH = 'eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507'


def smoke(driver, worker, rom, fixtures, output, leader='0'):
    driver, worker, rom, fixtures, output = map(lambda p: Path(p).resolve(), (driver, worker, rom, fixtures, output))
    leader = int(leader)
    assert leader in (0, 1)
    assert hashlib.sha256(rom.read_bytes()).hexdigest() == ROM_HASH
    inputs = [fixtures / f'player{p + 1}.sav' for p in range(2)]
    original = [p.read_bytes() for p in inputs]
    assert all(len(b) == 131072 for b in original)
    output.mkdir()
    with tempfile.TemporaryDirectory(prefix='couch-firered-recovery-ä-') as temporary:
        profile = Path(temporary)
        saves = [profile / 'players' / str(p + 1) / ROM_HASH / 'saves/game.sav' for p in range(2)]
        for path, data in zip(saves, original):
            path.parent.mkdir(parents=True)
            path.write_bytes(data)
        env = {k: v for k, v in os.environ.items() if not k.startswith(('PMCOOP_', 'MELONDS_AP'))}
        env.update(COUCHSPLIT_FIRERED_EXPERIMENT='1', QT_QPA_PLATFORM='offscreen', SDL_AUDIODRIVER='dummy')
        process = subprocess.Popen([str(driver), str(rom), str(worker), str(profile), str(output), '-'],
                                   env=env, text=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        lines, transcript = Queue(), []
        def collect():
            for line in process.stdout:
                lines.put(line)
            lines.put(None)
        reader = Thread(target=collect, daemon=True)
        reader.start()
        def until(marker, timeout=60):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                try:
                    line = lines.get(timeout=max(.01, deadline - time.monotonic()))
                except Empty:
                    break
                if line is None:
                    break
                transcript.append(line)
                if marker in line:
                    return
            raise AssertionError(''.join(transcript[-30:]))
        def action(kind, **values):
            process.stdin.write(json.dumps(dict(action=kind, **values)) + '\n')
            process.stdin.flush()
            until('Action complete')
        def capture(name):
            action('capture', name=name)
            return json.loads((output / f'{name}.json').read_text())
        try:
            until('Ready for private acceptance actions')
            action('boot-pair-to-field')
            capture('field-before')
            action('walk-until-pause', player=leader)
            before = capture('paused-before')
            pause = before['player1_bridge_state']
            assert pause >> 8 and pause & 255 in (3, 4), before
            assert before['player1_input_owner'] == leader + 1, before
            action('wait', ms=16000)  # Longer than the launcher's frame watchdog.
            after = capture('paused-after')
            for player in (1, 2):
                assert after[f'player{player}_bridge_state'] == pause
                assert after[f'player{player}_picture'] == before[f'player{player}_picture']
                assert after[f'player{player}_frames'] > before[f'player{player}_frames']
            assert [p.read_bytes() for p in saves] == original
            assert (profile / 'session.lock').exists()
            action('continue-individually')
            capture('individual-battle')
            action('finish-individual-battles')
            resumed = capture('coop-resumed')
            assert resumed['player1_bridge_state'] == resumed['player2_bridge_state'] == 0
            log = next((profile / 'logs').glob('*/player1.log')).read_text()
            assert 'recoverable preflight pause' in log and 'both native fields ready; shared co-op resumed' in log
            process.stdin.close()
            assert process.wait(timeout=15) == 0
            assert not (profile / 'session.lock').exists()
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            reader.join(timeout=2)
            (output / 'driver.log').write_text(''.join(transcript))
            for log in (profile / 'logs').glob('*/player1.log'):
                (output / 'worker.log').write_text(log.read_text())
    assert [p.read_bytes() for p in inputs] == original
    assert hashlib.sha256(rom.read_bytes()).hexdigest() == ROM_HASH
    report = dict(production_session_playview_and_worker=True, authored_native_save_inputs=True,
                  virtual_controllers=True, battle_initiator=leader + 1, pause_reason=pause & 255,
                  unchanged_paused_pictures_and_live_heartbeat=True, prior_saves_preserved=True,
                  pause_survives_watchdog=True, explicit_ui_approval=True,
                  native_individual_battle_returns_to_coop=True, clean_shutdown=True,
                  rom_and_inputs_unchanged=True, full_campaign=False)
    (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    smoke(*sys.argv[1:])
