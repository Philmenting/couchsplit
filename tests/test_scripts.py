# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

spec=importlib.util.spec_from_file_location('installer',Path(__file__).resolve().parents[1]/'scripts/install-launcher.py')
installer=importlib.util.module_from_spec(spec);spec.loader.exec_module(installer)

class InstallerTests(unittest.TestCase):
    def test_quoting_and_isolated_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)/'project with spaces';binary=root/'build/bin/platinum-coop';binary.parent.mkdir(parents=True)
            binary.write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n');binary.chmod(0o755)
            wrapper,desktop=installer.install(root,Path(tmp)/'data',Path(tmp)/'bin')
            output=subprocess.check_output([str(wrapper),'argument with spaces'],text=True)
            self.assertEqual(output,'--fullscreen\nargument with spaces\n')
            self.assertIn('Exec="',desktop.read_text())
            self.assertFalse((Path(tmp)/'.steam').exists())
    def test_missing_binary(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(SystemExit):installer.install(Path(tmp),Path(tmp)/'data',Path(tmp)/'bin')
