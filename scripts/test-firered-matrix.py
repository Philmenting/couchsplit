#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run reproducible, private FireRed checks without distributing game data.

The native scenario programs author prerequisites in memory. Their success does
not imply that a natural campaign, other platforms, or real controllers work.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
ROM_HASH = 'eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507'

QUICK = {
    'pair': ('single', 'single-leader2', 'trainer', 'capture-leader2',
             'party-menu-forced', 'exp-share', 'exp-share-twelve', 'exp-share-enigma'),
    'campaign': ('cut', 'shared-champion-rendezvous'),
    'gyms': ('misty', 'giovanni'),
    'progression': ('bill-full',),
}
FULL = {
    'pair': ('wild', 'single', 'single-leader2', 'single-multiturn',
             'single-multiturn-leader2', 'trainer', 'trainer-single',
             'trainer-single-leader2', 'payday', 'owners', 'leader2', 'escape',
             'escape-leader2', 'escape-player2', 'scripted', 'legendary',
             'grass-rendezvous', 'deferred-script', 'action-stats',
             'capture', 'capture-leader2', 'capture-small', 'capture-scripted', 'capture-legendary',
             'capture-full-native', 'capture-party-space', 'capture-pc',
             'capture-denied', 'capture-both', 'capture-both-leader2',
             'capture-sequential', 'capture-unown', 'capture-spinda',
             'party-menu-single', 'party-menu-forced', 'party-menu-cancel',
             'party-menu-summary', 'party-menu-exp-share', 'party-menu-cycle', 'party-menu-rollback', 'bench', 'exhaust',
             'defeat', 'evolution', 'exp-share', 'exp-share-leader2',
             'exp-share-trainer', 'exp-share-trainer-leader2',
             'exp-share-evolution', 'exp-share-evolution-leader2',
             'exp-share-learn', 'exp-share-learn-leader2', 'exp-share-many',
             'exp-share-many-leader2', 'exp-share-overflow',
             'exp-share-overflow-leader2', 'exp-share-twelve', 'exp-share-twelve-leader2',
             'exp-share-twelve-evolution', 'exp-share-twelve-evolution-leader2',
             'exp-share-dex', 'exp-share-dex-leader2',
             'exp-share-enigma', 'exp-share-enigma-leader2', 'exp-share-enigma-both'),
    'campaign': ('cut', 'surf', 'strength', 'marowak', 'national-dex',
                 'network-0', 'network-1', 'network-2', 'network-loaded',
                 'network-guard', 'grass-rendezvous',
                 'shared-champion-rendezvous', 'hall-of-fame'),
    'gyms': ('misty', 'surge', 'erika', 'koga', 'sabrina', 'blaine', 'giovanni',
             'misty-full'),
    'progression': ('rocket-door', 'tower-grunt', 'lostelle-full-berry',
                    'bill-full', 'bill-full-leader2'),
}


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True, help='private original German FireRed ROM')
    parser.add_argument('--checkpoint', type=Path, required=True, help='private starter-complete mGBA state')
    parser.add_argument('--save-early', type=Path, help='independent early-game battery save')
    parser.add_argument('--save-late', type=Path, help='independent late-game battery save')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/launcher')
    parser.add_argument('--profile', choices=('quick', 'full'), default='quick')
    parser.add_argument('--report', type=Path, help='write a compact JSON report; failure diagnostics may contain private game details')
    parser.add_argument('--timeout', type=int, default=180, help='seconds per scenario')
    parser.add_argument('--jobs', type=int, default=1, help='build, component-test and native-scenario concurrency (1–8; default: 1)')
    parser.add_argument('--case', action='append', default=[], help='run a named native scenario, e.g. pair/capture; repeat to select more')
    parser.add_argument('--skip-build', action='store_true', help='use already built test executables')
    args = parser.parse_args()
    if bool(args.save_early) != bool(args.save_late):
        parser.error('--save-early and --save-late must be supplied together')
    if args.timeout <= 0:
        parser.error('--timeout must be positive')
    if not 1 <= args.jobs <= 8:
        parser.error('--jobs must be between 1 and 8')
    scenario_set = FULL if args.profile == 'full' else QUICK
    available = {f'{group}/{scenario}' for group, names in scenario_set.items() for scenario in names}
    available.update(('connections', 'map-setup'))
    if any(case not in available for case in args.case):
        parser.error('Unknown --case for the selected profile')
    inputs = [args.rom, args.checkpoint] + ([args.save_early, args.save_late] if args.save_early else [])
    try:
        inputs = [p.resolve(strict=True) for p in inputs]
        if any(not p.is_file() for p in inputs):
            raise ValueError('Every input must be a file')
        if args.report and args.report.resolve() in inputs:
            raise ValueError('The report cannot overwrite an input')
        if args.report and (args.report.exists() or not args.report.parent.is_dir()):
            raise ValueError('The report must be a new file in an existing directory')
        before = [digest(p) for p in inputs]
        if before[0] != ROM_HASH:
            raise ValueError('The ROM is not the exact supported German original')
    except (OSError, ValueError) as error:
        parser.error(str(error))
    rom, checkpoint = inputs[:2]
    build = args.build_dir.resolve()
    ext = '.exe' if os.name == 'nt' else ''
    environment = os.environ.copy()
    environment.setdefault('QT_QPA_PLATFORM', 'offscreen')
    environment.setdefault('SDL_AUDIODRIVER', 'dummy')
    results = []

    def run(name, command, timeout=None):
        start = time.monotonic()
        extra = {}
        try:
            completed = subprocess.run([str(x) for x in command], cwd=ROOT, env=environment,
                                       capture_output=True, text=True, errors='replace',
                                       timeout=timeout or args.timeout)
            status = 'pass' if completed.returncode == 0 else 'fail'
            if name == 'python-tests' and status == 'pass':
                summary = completed.stdout + completed.stderr
                total = re.search(r'Ran (\d+) tests? in ', summary)
                skipped = re.search(r'skipped=(\d+)', summary)
                if not total or int(total.group(1)) == 0:
                    status = 'fail'
                else:
                    extra = {'tests': int(total.group(1)),
                             'skipped_tests': int(skipped.group(1)) if skipped else 0}
                    if extra['skipped_tests']:
                        status = 'partial'
            # Scenario names and the last diagnostic lines are useful on failure;
            # successful output can contain private party and save details.
            detail = (completed.stderr or completed.stdout)[-4000:] if status == 'fail' else ''
        except (OSError, subprocess.TimeoutExpired) as error:
            status, detail = 'fail', str(error)
        result = {'name': name, 'status': status, 'seconds': round(time.monotonic() - start, 2)}
        result.update(extra)
        if detail:
            # Strip local input paths from diagnostics before persisting them.
            for path in inputs:
                detail = detail.replace(str(path), '<private-input>')
            result['diagnostic'] = detail
        results.append(result)
        print(f'{status.upper():4} {name} ({result["seconds"]} s)', flush=True)
        if detail:
            print(detail, file=sys.stderr, flush=True)
        return status == 'pass'

    def finish():
        try:
            unchanged = [digest(p) for p in inputs] == before
        except OSError:
            unchanged = False
        results.append({'name': 'private-inputs-unchanged', 'status': 'pass' if unchanged else 'fail'})
        report = {'format': 'couchsplit-firered-matrix-1', 'profile': args.profile,
                  'rom_sha256': before[0], 'checkpoint_sha256': before[1],
                  'selected_cases': args.case,
                  'private_save_sha256': before[2:], 'results': results,
                  'complete_natural_campaign': False, 'physical_controllers_verified': False,
                  'scenarios_use_authored_prerequisites': True,
                  'known_limitations': ['Active, queued and Wish references still constrain the physical working party.',
                                        'A held Enigma Berry requires explicit individual-battle recovery.',
                                        'Shedinja party addition and per-owner Pokerus spreading are not supported.']}
        if args.report:
            try:
                with args.report.open('x', encoding='utf-8') as file:
                    json.dump(report, file, indent=2)
                    file.write('\n')
            except OSError as error:
                print(f'Cannot write new report: {error}', file=sys.stderr)
                return 1
        counts = {status: sum(r['status'] == status for r in results) for status in ('pass', 'fail', 'partial', 'skipped')}
        print('Results: ' + ', '.join(f'{count} {status}' for status, count in counts.items()), flush=True)
        return 1 if counts['fail'] else 0

    targets = ['verify-firered-' + x for x in ('pair', 'campaign', 'gyms', 'progression',
               'connections', 'map-setup', 'public-pair', 'public-saves')]
    if not args.skip_build:
        if not run('build/default', ['cmake', '--build', build, '--parallel', str(args.jobs)], timeout=600):
            results.append({'name': 'scenarios', 'status': 'skipped', 'reason': 'Build failed'})
            return finish()
        if not run('build/private-scenarios', ['cmake', '--build', build, '--parallel', str(args.jobs), '--target',
                                               *targets, 'create-firered-checkpoint', 'verify-gba', 'gba-worker'], timeout=600):
            results.append({'name': 'scenarios', 'status': 'skipped', 'reason': 'Build failed'})
            return finish()
    run('native-component-and-launcher-tests', ['ctest', '--test-dir', build, '--no-tests=error', '--output-on-failure',
                                              '--parallel', str(args.jobs)], timeout=600)
    run('python-tests', [sys.executable, '-m', 'unittest', 'discover', '-s', 'tests',
                         '-p', 'test_*.py'], timeout=600)
    with tempfile.TemporaryDirectory(prefix='couchsplit-firered-matrix-') as temp:
        temporary = Path(temp)
        tasks = []
        for group, scenarios in scenario_set.items():
            binary = build / ('verify-firered-' + group + ext)
            for scenario in scenarios:
                tasks.append((f'{group}/{scenario}', [binary, rom, checkpoint, temporary / (group + '-' + scenario), scenario]))
        for group in ('connections', 'map-setup'):
            tasks.append((group, [build / ('verify-firered-' + group + ext), rom, checkpoint, temporary / group]))
        if args.case:
            tasks = [task for task in tasks if task[0] in args.case]
        # Each scenario owns distinct cores and a separate new output directory.
        # Save/launcher tests below run sequentially and use their own profiles.
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            list(pool.map(lambda task: run(*task), tasks))
        if args.save_early and not args.case:
            early, late = inputs[2:]
            run('public-saves/early-and-late', [build / ('verify-firered-public-saves' + ext), rom, early, late])
            for scenario in ('wild-0', 'wild-1', 'brock', 'sync'):
                run('public-pair/' + scenario, [build / ('verify-firered-public-pair' + ext), rom, early, late, scenario])
            run('public-pair/questlog', [build / ('verify-firered-public-pair' + ext), rom, late, early, 'questlog'])
        elif not args.case:
            results.append({'name': 'public-saves-and-pair', 'status': 'skipped',
                            'reason': 'Two independent battery saves were not supplied'})
            print('SKIP public-saves-and-pair (provide both saves)', flush=True)
        if sys.platform.startswith('linux') and not args.case:
            run('production-session/crash-and-backup', [sys.executable, ROOT / 'tests/smoke_firered_pair_session.py',
                build / ('verify-gba' + ext), build / 'gba' / ('gba-worker' + ext), rom])
        elif not args.case:
            results.append({'name': 'production-session/crash-and-backup', 'status': 'skipped',
                            'reason': 'Linux-only process inspection'})
    return finish()


if __name__ == '__main__':
    sys.exit(main())
