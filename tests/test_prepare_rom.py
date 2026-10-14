# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import sys
import zipfile

spec = importlib.util.spec_from_file_location('prepare_rom', Path(__file__).resolve().parents[1] / 'scripts/prepare-rom.py')
patcher = importlib.util.module_from_spec(spec); spec.loader.exec_module(patcher)


@unittest.skipUnless(shutil.which('xdelta3'), 'Install xdelta3 to test the real decoder (required in CI)')
class PatchTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='patch test ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'own synthetic header.nds'
        self.target = self.root / 'own modified header.nds'
        self.delta = self.root / 'own patch.xdelta'
        self.output = self.root / 'result.nds'
        # Synthetic buffers only. No game, Nintendo logo, BIOS or firmware.
        data = bytearray(16384); data[12:16] = b'CPUD'
        self.source.write_bytes(data)
        data[4096:4105] = b'OWN PATCH'; self.target.write_bytes(data)
        self.encode()

    def encode(self):
        self.delta.unlink(missing_ok=True)
        env = dict(os.environ); env.pop('XDELTA', None)
        subprocess.run(['xdelta3', '-e', '-s', str(self.source), str(self.target), str(self.delta)], env=env, check=True, timeout=10)

    def apply(self, **overrides):
        args = dict(rom=self.source, patch=self.delta, output=self.output,
                    source_sha256=patcher.sha256(self.source),
                    patch_sha256=patcher.sha256(self.delta), target_sha256=patcher.sha256(self.target))
        args.update(overrides)
        return patcher.prepare(**args)

    def test_real_decode_original_preserved_and_no_overwrite(self):
        before = self.source.read_bytes()
        self.apply()
        self.assertEqual(self.output.read_bytes(), self.target.read_bytes())
        self.assertEqual(self.source.read_bytes(), before)
        self.assertEqual(self.output.stat().st_mode & 0o777, 0o600)
        with self.assertRaises(FileExistsError): self.apply()
        self.assertEqual(self.output.read_bytes(), self.target.read_bytes())

    def test_wrong_base_patch_and_result_publish_nothing(self):
        for name in ('source_sha256', 'patch_sha256', 'target_sha256'):
            with self.subTest(name=name):
                with self.assertRaises(ValueError): self.apply(**{name: '0' * 64})
                self.assertFalse(self.output.exists())
                self.assertFalse(list(self.root.glob('.platinum-patch-*')))

    def test_english_base_rejected_even_if_checksums_match(self):
        data = bytearray(self.source.read_bytes()); data[12:16] = b'CPUE'; self.source.write_bytes(data)
        with self.assertRaises(ValueError): self.apply()
        self.assertFalse(self.output.exists())

    def test_patch_cannot_change_to_english_identifier(self):
        data = bytearray(self.target.read_bytes()); data[12:16] = b'CPUE'; self.target.write_bytes(data)
        self.encode()
        with self.assertRaises(ValueError): self.apply()
        self.assertFalse(self.output.exists())

    def test_bad_patch_and_missing_decoder_leave_no_output(self):
        self.delta.write_bytes(b'not an xdelta patch')
        with self.assertRaises(ValueError): self.apply()
        # Valid signature but corrupt VCDIFF body: exercise the actual decoder's failure.
        self.delta.write_bytes(b'\xd6\xc3\xc4\x00invalid VCDIFF data')
        with self.assertRaises(ValueError): self.apply()
        self.encode()
        with self.assertRaises(FileNotFoundError): self.apply(decoder=str(self.root / 'missing decoder'))
        self.assertFalse(self.output.exists())

    def test_inspect_unknown_zip_is_explicit_and_read_only(self):
        archive = self.root / 'own build package.zip'
        with zipfile.ZipFile(archive, 'w') as package:
            package.write(self.delta, 'Build/own.xdelta')
            package.writestr('Build/ignored.exe', b'not a program')
        before = set(self.root.iterdir())
        result = subprocess.run([sys.executable, str(Path(patcher.__file__)), '--inspect',
                                 '--rom', str(self.source), '--patch', str(archive)],
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, 2)
        report = json.loads(result.stdout)
        self.assertEqual(report['patch_sha256'], patcher.sha256(self.delta))
        self.assertIsNone(report['expected_base'])
        self.assertFalse(report['matches_documented_base'])
        self.assertEqual(set(self.root.iterdir()), before)

    def test_ambiguous_package_and_short_rom_are_rejected(self):
        archive = self.root / 'ambiguous.zip'
        with zipfile.ZipFile(archive, 'w') as package:
            package.writestr('first.xdelta', b'one')
            package.writestr('second.xdelta', b'two')
        with self.assertRaises(ValueError): patcher.inspect_patch(self.source, archive)
        self.source.write_bytes(b'bad header')
        with self.assertRaises(ValueError): patcher.inspect_patch(self.source, self.delta)
