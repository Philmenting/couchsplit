# SPDX-License-Identifier: GPL-3.0-or-later
"""Release boundary: complete own-source tools, never extra game/profile files."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location('rom_tool_package',
    Path(__file__).resolve().parents[1] / 'scripts/package-rom-tools.py')
maker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(maker)


class RomToolPackage(unittest.TestCase):
    def test_allowlist_checksums_and_reproducibility(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / 'source'
            for name in maker.FILES:
                path = root/name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(('Fixture for ' + name).encode())
            (root/'private.nds').write_bytes(b'not game data: forbidden fixture')
            (root/'private.sav').write_bytes(b'not a save: forbidden fixture')
            a, b = Path(folder)/'a.zip', Path(folder)/'b.zip'
            maker.package(root, a)
            maker.package(root, b)
            self.assertEqual(a.read_bytes(), b.read_bytes())
            with zipfile.ZipFile(a) as archive:
                self.assertEqual(set(archive.namelist()), set(maker.FILES)|{'README.txt','SHA256SUMS'})
                self.assertIsNone(archive.testzip())
                for line in archive.read('SHA256SUMS').decode().splitlines():
                    expected, name = line.split('  ', 1)
                    self.assertEqual(hashlib.sha256(archive.read(name)).hexdigest(), expected)
            original = a.read_bytes()
            with self.assertRaises(FileExistsError):
                maker.package(root, a)
            self.assertEqual(a.read_bytes(), original)

    def test_missing_required_source_leaves_no_archive(self):
        with tempfile.TemporaryDirectory() as folder:
            target = Path(folder)/'tools.zip'
            with self.assertRaises(FileNotFoundError):
                maker.package(Path(folder), target)
            self.assertFalse(target.exists())
