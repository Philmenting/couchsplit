# SPDX-License-Identifier: GPL-3.0-or-later
"""Installer boundaries with explicit external-command doubles; no Bazzite claim."""
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
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


class OneFileTests(unittest.TestCase):
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
            def external(args, log, check=True, timeout=1200):
                calls.append(args)
                if args[:2] == ['podman', 'info']: return subprocess.CompletedProcess(args, 0, 'true\n')
                if args[:3] == ['podman', 'container', 'exists']:
                    return subprocess.CompletedProcess(args, 0 if state['exists'] else 1, '')
                if args[:2] == ['podman', 'inspect']:
                    return subprocess.CompletedProcess(args, 0, 'runtime-v1\n')
                if args[:2] == ['distrobox', 'create']: state['exists'] = True
                if '--coop-capabilities' in args:
                    if state['fail']: raise RuntimeError('TEST-ONLY probe failure')
                    return subprocess.CompletedProcess(args, 0, 'PMCOOP_ABI=1\n')
                if '--smoke-test' in args: Path(args[args.index('--smoke-test') + 1]).write_bytes(b'TEST-ONLY image boundary')
                return subprocess.CompletedProcess(args, 0, '')
            ui = SimpleNamespace(stage=lambda *args: None)
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
                                      SimpleNamespace(stage=lambda *args: None), io.StringIO())
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
