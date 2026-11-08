#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Private sustained FireRed acceptance through the production launcher/worker.

Create `soak-launcher-border` batteries with verify-firered-pair first. No game
data is distributed. One real Session/PlayView and paired worker perform every
cycle, using virtual SDL controllers, ordinary native battles, map transitions
and the game's SAVE menu. Only the final verification restarts the worker.

Example (own ROM and private outputs required):
  python3 tests/smoke_firered_soak.py DRIVER WORKER ROM FIXTURES NEW_OUTPUT \
      --cycles 10 --minimum-minutes 10

This is a repeatable Route 1/Pallet stress test with authored strong parties,
not a campaign playthrough, physical-controller or target-desktop certification.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
from queue import Empty, Queue
import subprocess
from threading import Thread
import time

from firered_save_inspect import check_owned_progress, demand, inspect_save

ROM_HASH = 'eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507'
ROUTE, PALLET = (3, 19), (3, 0)


class Driver:
    def __init__(self, executable, worker, rom, profile, output):
        environment = {k: v for k, v in os.environ.items() if not k.startswith(('PMCOOP_', 'MELONDS_AP'))}
        environment.update(COUCHSPLIT_FIRERED_EXPERIMENT='1', QT_QPA_PLATFORM='offscreen', SDL_AUDIODRIVER='dummy')
        self.process = subprocess.Popen([str(executable), str(rom), str(worker), str(profile), str(output), '-'],
                                        env=environment, text=True, stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.lines, self.transcript = Queue(), []
        self.log = (output / 'driver.log').open('w')

        def collect():
            for line in self.process.stdout:
                self.lines.put(line)
            self.lines.put(None)

        self.reader = Thread(target=collect, daemon=True)
        self.reader.start()
        try:
            self.until('Ready for private acceptance actions', 60)
        except Exception:
            self.close()
            raise

    def until(self, marker, timeout):
        deadline, received = time.monotonic() + timeout, []
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=max(.01, deadline - time.monotonic()))
            except Empty:
                break
            if line is None:
                break
            received.append(line)
            self.transcript.append(line)
            self.log.write(line)
            self.log.flush()
            if marker in line:
                return received
        raise RuntimeError('Driver did not complete its action:\n' + ''.join(self.transcript[-20:]))

    def action(self, kind, timeout=300, **values):
        self.process.stdin.write(json.dumps(dict(action=kind, **values)) + '\n')
        self.process.stdin.flush()
        received = self.until('Action complete', timeout)
        states = [json.loads(line[6:]) for line in received if line.startswith('State ')]
        demand(len(states) == 1, 'Driver omitted its published IPC observation')
        return states[0]

    def finish(self):
        self.process.stdin.close()
        self.until('"clean_stop": true', 20)
        demand(self.process.wait(timeout=15) == 0, 'Driver did not stop cleanly')

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)
        self.reader.join(timeout=2)
        self.log.close()


def run(arguments):
    executable, worker, rom, fixtures, output = (Path(p).resolve() for p in
                                               (arguments.driver, arguments.worker, arguments.rom, arguments.fixtures, arguments.output))
    demand(arguments.cycles >= 1 and arguments.maximum_cycles >= arguments.cycles and arguments.minimum_minutes >= 0,
           'Invalid cycle count, fixture budget or duration')
    demand(hashlib.sha256(rom.read_bytes()).hexdigest() == ROM_HASH, 'Only the verified German original is supported')
    binary_hashes = {name: hashlib.sha256(path.read_bytes()).hexdigest()
                     for name, path in (('driver', executable), ('worker', worker))}
    source = [fixtures / f'player{p + 1}.sav' for p in range(2)]
    originals = [path.read_bytes() for path in source]
    initial = [inspect_save(data) for data in originals]
    demand(all(s['map'] == ROUTE and s['position'] == (12, 38) for s in initial), 'Use the authored soak-launcher-border native fixtures')
    demand(all(s['facing'] in (1, 2, 3, 4) for s in initial), 'Fixture has no valid native player object direction')
    demand(not ({(m['identity'], m['species'] == 303) for m in initial[0]['party']} & {(m['identity'], m['species'] == 303) for m in initial[1]['party']}), 'Private fixtures must have disjoint owned Pokemon identities')
    output.mkdir()
    profile = output / 'private-profiles'
    batteries = [profile / 'players' / str(p + 1) / ROM_HASH / 'saves/game.sav' for p in range(2)]
    for path, data in zip(batteries, originals):
        path.parent.mkdir(parents=True)
        path.write_bytes(data)
    driver = Driver(executable, worker, rom, profile, output)
    snapshots, checkpoints, input_battles = initial, [], 0
    first_save = True
    restarted = False

    def native_save(label):
        nonlocal snapshots, first_save
        previous = snapshots
        observation = driver.action('native-save-both', down=3 if first_save else 0)
        first_save = False
        snapshots = [inspect_save(path.read_bytes()) for path in batteries]
        for owner in range(2):
            check_owned_progress(initial[owner], snapshots[owner])
            demand((snapshots[owner]['generation'] - previous[owner]['generation']) & 0xFFFFFFFF == 1,
                   'SAVE did not advance exactly one generation')
        demand((profile / 'session.lock').exists(), 'Live session lost its profile lock')
        if checkpoints and not restarted:
            demand(all(observation[f'player{p + 1}_frames'] > checkpoints[-1]['frames'][p] for p in range(2)),
                   'A frame heartbeat stopped or reset inside the sustained session')
        checkpoints.append(dict(label=label, map=[list(s['map']) for s in snapshots],
                                position=[list(s['position']) for s in snapshots],
                                generations=[s['generation'] for s in snapshots],
                                battle_counts=[list(s['battles']) for s in snapshots],
                                frames=[observation[f'player{p + 1}_frames'] for p in range(2)]))
        return observation

    def complete_if_needed(observation):
        nonlocal input_battles
        states = {observation[f'player{p + 1}_state'] for p in range(2)}
        if states == {1}:
            driver.action('finish-shared-battle')
            input_battles += 1
        else:
            demand(states == {0}, f'Unexpected native movement state {states}')

    def face(players, key):
        native_direction = {4: 4, 5: 3, 6: 2, 7: 1}[key]
        turning = [p for p in players if snapshots[p]['facing'] != native_direction]
        if turning:
            complete_if_needed(driver.action('field-step', player=2 if len(turning) == 2 else turning[0], key=key))

    def settle_route():
        # Re-read native saves between corrections; never infer avatar position
        # from elapsed wall time or access emulator RAM from the test process.
        for attempt in range(8):
            demand(all(s['map'] == ROUTE and s['position'][0] == 12 for s in snapshots), 'Unexpected authored Route 1 corridor')
            pending = [p for p in range(2) if snapshots[p]['position'][1] != 38]
            if not pending:
                return
            for player in pending:
                y = snapshots[player]['position'][1]
                demand(34 <= y <= 39, 'Walking escaped the repeatable border grass corridor')
                direction = 7 if y < 38 else 6
                face([player], direction)
                complete_if_needed(driver.action('field-step', player=player, key=direction))
            native_save(f'normalize-{len(checkpoints)}')
        raise RuntimeError('Native walking did not settle both actors at the border fixture position')

    def cross(direction, destination, label):
        face([0, 1], direction)
        for _ in range(3):
            complete_if_needed(driver.action('field-step', player=2, key=direction))
        native_save(label)
        demand(all(s['map'] == destination for s in snapshots), f'Native map transition did not reach {destination}')
        driver.action('capture', name=label)

    started = time.monotonic()
    cycles = 0
    try:
        driver.action('boot-pair-to-field')
        native_save('initial-native-save')
        driver.action('capture', name='initial-field')
        while cycles < arguments.cycles or time.monotonic() - started < arguments.minimum_minutes * 60:
            demand(cycles < arguments.maximum_cycles, 'Requested duration exceeded the bounded native fixture cycle budget')
            demand(all(s['party'][0]['pp'][0] >= 4 for s in snapshots), 'Private fixture needs native PP replenishment before another cycle')
            leader = cycles % 2
            observation = driver.action('seek-shared-battle', player=leader)
            demand(observation['player1_state'] == observation['player2_state'] == 1, 'Wild encounter did not enter shared co-op')
            complete_if_needed(observation)
            native_save(f'cycle-{cycles + 1:02d}-battle')
            settle_route()
            cross(7, PALLET, f'cycle-{cycles + 1:02d}-pallet')
            cross(6, ROUTE, f'cycle-{cycles + 1:02d}-route')
            settle_route()
            cycles += 1
            for owner in range(2):
                demand(snapshots[owner]['battles'][1] - initial[owner]['battles'][1] == input_battles,
                       'Native wild-battle count differs from completed shared fights')
            print(f'Completed {cycles} cycles, {input_battles} shared battles, {time.monotonic() - started:.1f}s in one session', flush=True)
            (output / 'progress.json').write_text(json.dumps(dict(cycles=cycles, shared_battles=input_battles,
                                                                elapsed_seconds=round(time.monotonic() - started, 2), checkpoints=checkpoints), indent=2) + '\n')
        sustained_seconds = time.monotonic() - started
        before_restart = snapshots
        demand(binary_hashes['worker'] == hashlib.sha256(worker.read_bytes()).hexdigest(), 'Worker executable changed during the sustained test')
        driver.action('restart')
        restarted = True
        driver.action('boot-pair-to-field')
        first_save = True
        native_save('fresh-continue-and-native-save')
        driver.action('capture', name='fresh-continue')
        for before, after in zip(before_restart, snapshots):
            for field in ('map', 'position', 'party', 'wallet', 'battles', 'storage_sha256', 'trainer'):
                demand(after[field] == before[field], f'Native fresh Continue changed {field}')
        driver.finish()
        demand(not (profile / 'session.lock').exists(), 'Final stop leaked the profile lock')
        logs = sorted((profile / 'logs').glob('*/player1.log'))
        demand(len(logs) == 2, 'The sustained section unexpectedly restarted its worker')
        for log in logs:
            contents = log.read_text()
            demand(contents.count('EXPERIMENTAL; two cores') == 1 and 'stopped cleanly' in contents,
                   'Worker lifecycle log does not prove one clean session')
        demand('battle committed' in logs[0].read_text(), 'No production shared battle commits were logged')
    finally:
        driver.close()
    demand([path.read_bytes() for path in source] == originals, 'Source fixtures changed')
    demand(hashlib.sha256(rom.read_bytes()).hexdigest() == ROM_HASH, 'ROM bytes changed')
    demand(binary_hashes == {name: hashlib.sha256(path.read_bytes()).hexdigest()
                             for name, path in (('driver', executable), ('worker', worker))}, 'A tested executable changed')
    report = dict(production_session_playview_and_paired_worker=True, authored_native_fixture=True,
                  virtual_controllers=True, qt_platform='offscreen', audio_driver='dummy',
                  runtime_ram_writes_from_test=False, cycles=cycles, sustained_seconds=round(sustained_seconds, 2),
                  shared_battles=input_battles, battle_initiators_alternate=cycles >= 2,
                  cycle_battle_initiators=[1, 2] if cycles >= 2 else [1], both_human_input_owners_observed=True,
                  native_map_transitions=cycles * 2, native_saves_per_player=len(checkpoints),
                  complete_save_generations_and_party_checksums=True, owned_parties_and_pc_preserved=True,
                  native_battle_counters_exact=True, one_worker_session_during_cycles=True,
                  fresh_continue_retains_owned_progress=True, profile_locks_and_clean_shutdown=True,
                  rom_and_source_saves_unchanged=True, physical_controllers=False, full_campaign=False,
                  minimum_ten_cycles_met=cycles >= 10, tested_binary_sha256=binary_hashes)
    (output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for argument in ('driver', 'worker', 'rom', 'fixtures', 'output'):
        parser.add_argument(argument)
    parser.add_argument('--cycles', type=int, default=10, help='Minimum repeated cycles (default: 10)')
    parser.add_argument('--minimum-minutes', type=float, default=10, help='Minimum active sustained duration (default: 10)')
    parser.add_argument('--maximum-cycles', type=int, default=40, help='Fail rather than exhaust the authored fixture budget (default: 40)')
    run(parser.parse_args())
