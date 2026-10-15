# SPDX-License-Identifier: GPL-3.0-or-later
"""Installer boundaries with explicit external-command doubles; no Bazzite claim."""
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import threading
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / filename)
    result = importlib.util.module_from_spec(spec); spec.loader.exec_module(result); return result


maker = module('installer_maker', 'make-installer.py')
bootstrap = module('installer_bootstrap', 'installer-bootstrap.py')
installer = module('bazzite_installer', 'install-bazzite.py')
importer = module('rom_importer', 'import-rom.py')


class CommandTests(unittest.TestCase):
    def recovery_case(self, state, owner='runtime-v1', retry_ok=True):
        calls = []; enters = 0
        def external(args, log, **kwargs):
            nonlocal enters
            calls.append(args)
            if args[:len(installer.ENTER)] == installer.ENTER:
                enters += 1
                if enters == 1 or not retry_ok: raise RuntimeError('TEST-ONLY start error')
            output = json.dumps(state) if '{{json .State}}' in args else owner
            return subprocess.CompletedProcess(args, 0, output)
        log = io.StringIO()
        with patch.object(installer, 'command', external):
            try: installer.initialize_runtime(log)
            except RuntimeError: return calls, False
        return calls, True

    def broken_mount_state(self):
        return {'Status': 'exited', 'Running': False,
                'Error': 'creating temporary passwd file for container TEST: open /overlay/TEST/merged/etc/passwd: no such file or directory'}

    def test_exact_stopped_owned_mount_failure_gets_cleanup_and_one_retry(self):
        calls, success = self.recovery_case(self.broken_mount_state())
        self.assertTrue(success)
        self.assertEqual(sum(args == [*installer.ENTER, 'true'] for args in calls), 2)
        self.assertEqual(sum(args == ['podman', 'container', 'cleanup', installer.BOX] for args in calls), 1)
        self.assertFalse(any(flag in args for args in calls for flag in ('--all', '--rm', '--rmi', 'rm', 'prune', 'reset')))

    def test_failed_cleanup_retry_is_bounded(self):
        calls, success = self.recovery_case(self.broken_mount_state(), retry_ok=False)
        self.assertFalse(success)
        self.assertEqual(sum(args == [*installer.ENTER, 'true'] for args in calls), 2)
        self.assertEqual(sum('cleanup' in args for args in calls), 1)

    def test_running_foreign_or_unrelated_failure_never_gets_cleanup(self):
        for changes, owner in (({'Status': 'running', 'Running': True}, 'runtime-v1'),
                               ({'Error': 'TEST-ONLY DNS failure'}, 'runtime-v1'),
                               ({'Running': None}, 'runtime-v1'), ({}, 'foreign')):
            with self.subTest(changes=changes, owner=owner):
                state = dict(self.broken_mount_state(), **changes)
                calls, success = self.recovery_case(state, owner=owner)
                self.assertFalse(success)
                self.assertFalse(any('cleanup' in args for args in calls))
                self.assertEqual(sum(args == [*installer.ENTER, 'true'] for args in calls), 1)

    def test_kde_progress_is_animated_and_updates_during_a_command(self):
        def available(name):
            return '/TEST-ONLY/' + name if name in ('kdialog', 'qdbus6') else None
        def dbus(args, **kwargs):
            return subprocess.CompletedProcess(args, 0, 'org.kde.kdialog-test /ProgressDialog\n')
        with patch.dict(os.environ, {'DISPLAY': ':TEST-ONLY'}), \
                patch.object(installer.shutil, 'which', available):
            ui = installer.Dialogs()
        # Keep the actual child command real; only KDE/DBus is substituted.
        calls = []
        def display(args, **kwargs):
            calls.append(args); return dbus(args, **kwargs)
        with patch.object(installer.subprocess, 'run', display):
            ui.stage(2, 'Grundpakete laden …')
            ui.activity(42)
            ui.close()
        opening = next(args for args in calls if '--progressbar' in args)
        self.assertEqual(opening[-1], '0')
        self.assertIn('Schritt 2 von 5', opening[-2])
        self.assertTrue(any('setLabelText' in args and '42 Sekunden' in args[-1] for args in calls))
        self.assertFalse(any('Set' in args for args in calls))

    def test_output_is_logged_before_exit_and_split_utf8_is_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp); resume = base / 'resume'; output = base / 'live.log'; results = []; activity = []
            script = ('import sys,time,pathlib\n'
                      'sys.stdout.buffer.write(b"Gr\\xc3");sys.stdout.flush()\n'
                      f'while not pathlib.Path({str(resume)!r}).exists(): time.sleep(.01)\n'
                      'sys.stdout.buffer.write(b"\\xbc\\xc3\\x9fe\\n");sys.stdout.flush()\n')
            def run():
                with output.open('w', encoding='utf-8') as log:
                    results.append(installer.command([sys.executable, '-c', script], log, timeout=5, progress=activity.append))
            worker = threading.Thread(target=run); worker.start()
            try:
                deadline = time.monotonic() + 3
                while time.monotonic() < deadline:
                    if output.exists() and output.read_text().endswith('Gr'): break
                    time.sleep(.01)
                self.assertTrue(output.read_text().endswith('Gr'), 'Output was buffered until process exit')
                self.assertTrue(worker.is_alive())
            finally:
                resume.touch(); worker.join(timeout=6)
            self.assertFalse(worker.is_alive()); self.assertEqual(len(results), 1)
            self.assertEqual(results[0].stdout, 'Grüße\n')
            self.assertTrue(activity)
            self.assertIn('Exitstatus: 0', output.read_text())

    def test_timeout_keeps_partial_output_and_terminates_command_group(self):
        with tempfile.TemporaryDirectory() as temp:
            pidfile = Path(temp) / 'child.pid'; log = io.StringIO()
            script = ('import subprocess,sys,time,pathlib\n'
                      'child=subprocess.Popen([sys.executable,"-c","import time;time.sleep(30)"])\n'
                      f'pathlib.Path({str(pidfile)!r}).write_text(str(child.pid))\n'
                      'print("partial-before-timeout",flush=True);time.sleep(30)\n')
            with self.assertRaisesRegex(RuntimeError, 'Zeitlimit'):
                installer.command([sys.executable, '-c', script], log, timeout=.5)
            self.assertIn('partial-before-timeout', log.getvalue())
            self.assertIn('FEHLER:', log.getvalue())
            child = int(pidfile.read_text()); status = Path(f'/proc/{child}/stat')
            try:
                state = status.read_text().split()[2]
            except FileNotFoundError:
                pass  # The terminated child was reaped between inspection and read.
            else:
                self.assertEqual(state, 'Z')

    def test_noninteractive_stdin_environment_and_failure_exit_status(self):
        script = ('import os,sys\n'
                  'assert sys.stdin.read()==""\n'
                  'assert os.environ["DBX_CONTAINER_MANAGER"]=="podman"\n'
                  'assert os.environ["DBX_NON_INTERACTIVE"]=="1"\n'
                  'assert "LD_PRELOAD" not in os.environ\n'
                  'sys.stderr.buffer.write(b"failure\\xff");sys.exit(7)\n')
        log = io.StringIO()
        with patch.dict(os.environ, {'LD_PRELOAD': '/TEST-ONLY/bad.so'}):
            result = installer.command([sys.executable, '-c', script], log, check=False)
        self.assertEqual(result.returncode, 7); self.assertEqual(result.stdout, 'failure\ufffd')
        self.assertIn('Exitstatus: 7', log.getvalue())
        with self.assertRaisesRegex(RuntimeError, 'Schritt fehlgeschlagen'):
            installer.command([sys.executable, '-c', 'raise SystemExit(7)'], io.StringIO())

    def test_first_start_failure_collects_owned_container_diagnostics(self):
        log = io.StringIO(); calls = []
        def external(args, output, **kwargs):
            calls.append((args, kwargs))
            if args[:len(installer.ENTER)] == installer.ENTER: raise RuntimeError('TEST-ONLY initialization failure')
            output.write('TEST-ONLY container output\n')
            return subprocess.CompletedProcess(args, 0, '')
        with patch.object(installer, 'command', external):
            with self.assertRaisesRegex(RuntimeError, 'Ersteinrichtung'):
                installer.initialize_runtime(log)
        self.assertEqual(calls[0][0], [*installer.ENTER, 'true'])
        self.assertEqual(calls[1][0], ['podman', 'inspect', '--format', '{{json .State}}', installer.BOX])
        self.assertEqual(calls[2][0], ['podman', 'logs', '--tail', '200', installer.BOX])
        self.assertTrue(all(call[1] == {'check': False, 'timeout': 15} for call in calls[1:]))
        self.assertIn('TEST-ONLY container output', log.getvalue())


class OneFileTests(unittest.TestCase):
    def test_gpl_freebios_makefile_is_source_but_dumps_are_rejected(self):
        self.assertFalse(maker.forbidden('corresponding-source/freebios/Makefile.bios'))
        for name in ('project/game.nds', 'project/user.bios', 'project/bios9.bin', 'corresponding-source/user.rom'):
            self.assertTrue(maker.forbidden(name))

    def test_self_extraction_and_corruption_rejection_without_install(self):
        with tempfile.TemporaryDirectory(prefix='installer path ') as temp:
            path = Path(temp) / 'Setup with spaces.run'
            files = {'project/OWN.txt': (b'own bytes\n', 0o644)}
            script = (ROOT / 'scripts/installer-bootstrap.py').read_text()
            maker.make_installer(files, {}, path, script)
            env = dict(__import__('os').environ, QT_QPA_PLATFORM='offscreen')
            result = subprocess.run(['bash', str(path), '--check'], capture_output=True, text=True, timeout=10, env=env)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('keine Installation', result.stdout)
            data = bytearray(path.read_bytes()); data[-8] ^= 1; path.write_bytes(data)
            # Replace optional error dialog with a short-lived PATH-local test stub.
            dialog = Path(temp) / 'kdialog'; dialog.write_text('#!/bin/sh\nexit 0\n'); dialog.chmod(0o755)
            env['PATH'] = temp + ':' + env['PATH']
            result = subprocess.run(['bash', str(path), '--check'], capture_output=True, text=True, timeout=10, env=env)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('beschädigt', result.stderr)

    def test_archive_traversal_and_symlink_rejected(self):
        for name, kind in (('../escape', tarfile.REGTYPE), ('link', tarfile.SYMTYPE)):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temp:
                stream = io.BytesIO()
                with tarfile.open(fileobj=stream, mode='w:gz') as archive:
                    info = tarfile.TarInfo(name); info.type = kind; info.linkname = '../escape'
                    archive.addfile(info, io.BytesIO())
                with self.assertRaisesRegex(ValueError, 'Unsicherer'):
                    bootstrap.unpack(stream.getvalue(), Path(temp))

    def payload(self, root):
        payload = root / 'payload'; scripts = payload / 'project/scripts'; scripts.mkdir(parents=True)
        shutil.copyfile(ROOT / 'scripts/install-launcher.py', scripts / 'install-launcher.py')
        binary = payload / 'project/build/bin'; binary.mkdir(parents=True)
        for name in ('platinum-coop', 'melonDS-coop'):
            path = binary / name; path.write_text('#!/bin/sh\n# TEST-ONLY, not an emulator\nexit 0\n'); path.chmod(0o755)
        files = {p.relative_to(payload).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in payload.rglob('*') if p.is_file()}
        (payload / 'release.json').write_text(json.dumps({'files': files}))
        return payload

    def test_rootless_install_reinstall_and_failure_preserve_previous_release(self):
        with tempfile.TemporaryDirectory(prefix='co-op $ path ') as temp:
            base = Path(temp); payload = self.payload(base); root = base / 'data/app'; root.mkdir(parents=True)
            calls = []; state = dict(exists=False, fail=False)
            def external(args, log, check=True, timeout=1200, progress=None):
                calls.append(args)
                if args[:2] == ['podman', 'info']: return subprocess.CompletedProcess(args, 0, 'true\n')
                if args[:3] == ['podman', 'container', 'exists']:
                    return subprocess.CompletedProcess(args, 0 if state['exists'] else 1, '')
                if args[:2] == ['podman', 'inspect']:
                    return subprocess.CompletedProcess(args, 0, 'runtime-v1\n')
                if args[:2] == ['distrobox', 'create']: state['exists'] = True
                if '--coop-capabilities' in args:
                    if state['fail']: raise RuntimeError('TEST-ONLY probe failure')
                    return subprocess.CompletedProcess(args, 0, 'PMCOOP_ABI=2\n')
                if '--smoke-test' in args: Path(args[args.index('--smoke-test') + 1]).write_bytes(b'TEST-ONLY image boundary')
                return subprocess.CompletedProcess(args, 0, '')
            ui = SimpleNamespace(stage=lambda *args: None, activity=lambda *args: None)
            with patch.object(installer, 'command', external):
                wrapper = installer.install(payload, root, base / 'data', base / 'bin', 'a' * 20, ui, io.StringIO())
                current = root / 'software/current'; self.assertTrue(wrapper.is_file()); previous = current.resolve()
                save = root / 'players/keep.sav'; save.parent.mkdir(); save.write_bytes(b'USER DATA')
                installer.install(payload, root, base / 'data', base / 'bin', 'a' * 20, ui, io.StringIO())
                self.assertEqual(current.resolve(), previous)
                state['fail'] = True
                with self.assertRaisesRegex(RuntimeError, 'probe failure'):
                    installer.install(payload, root, base / 'data', base / 'bin', 'b' * 20, ui, io.StringIO())
                self.assertEqual(current.resolve(), previous); self.assertEqual(save.read_bytes(), b'USER DATA')
            self.assertEqual(sum(c[:2] == ['distrobox', 'create'] for c in calls), 1)
            apt_calls = [c for c in calls if 'apt-get' in c]
            self.assertTrue(apt_calls)
            self.assertTrue(all(c[:len(installer.ENTER)] == installer.ENTER for c in apt_calls))
            self.assertIn('--no-tty', wrapper.read_text()); self.assertIn('LD_PRELOAD', wrapper.read_text())
            self.assertFalse((base / '.steam').exists())

    def test_foreign_container_is_not_modified(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp); payload = self.payload(base); calls = []
            def external(args, *a, **kw):
                calls.append(args)
                if args[:2] == ['podman', 'info']: return subprocess.CompletedProcess(args, 0, 'true\n')
                return subprocess.CompletedProcess(args, 0, 'foreign\n' if 'inspect' in args else '')
            with patch.object(installer, 'command', external):
                with self.assertRaisesRegex(RuntimeError, 'fremd belegt'):
                    installer.install(payload, base, base / 'data', base / 'bin', 'c' * 20,
                                      SimpleNamespace(stage=lambda *args: None, activity=lambda *args: None), io.StringIO())
            self.assertEqual(len(calls), 3)


class RomImportTests(unittest.TestCase):
    def test_raw_zip_import_and_idempotent_private_copy(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp); source = base / 'OWN synthetic header.nds'
            data = bytearray(4096); data[12:16] = b'CPUE'; source.write_bytes(data)
            archive = base / 'own.zip'
            with zipfile.ZipFile(archive, 'w') as z: z.write(source, '../../' + source.name)
            imported = importer.import_rom(archive, base / 'roms')
            self.assertEqual(imported.read_bytes(), data); self.assertEqual(imported.stat().st_mode & 0o777, 0o600)
            self.assertEqual(importer.import_rom(source, base / 'roms'), imported)
            self.assertEqual(source.read_bytes(), data)
            imported.write_bytes(b'corrupt')
            with self.assertRaisesRegex(ValueError, 'beschädigt'): importer.import_rom(source, base / 'roms')

    def test_ambiguous_zip_and_invalid_game_are_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp); archive = base / 'bad.zip'
            with zipfile.ZipFile(archive, 'w') as z:
                z.writestr('one.nds', b'bad'); z.writestr('two.nds', b'bad')
            with self.assertRaisesRegex(ValueError, 'genau eine'): importer.import_rom(archive, base / 'roms')
            source = base / 'other.nds'; source.write_bytes(bytes(1024))
            with self.assertRaisesRegex(ValueError, 'CPU'): importer.import_rom(source, base / 'roms')
            self.assertFalse(list((base / 'roms').glob('*.nds')))


if __name__ == '__main__': unittest.main()
