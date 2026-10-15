# SPDX-License-Identifier: GPL-3.0-or-later
"""Host-helper boundary tests, NOT a Steam-client integration test."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def module(name, filename):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / filename)
    result = importlib.util.module_from_spec(spec); spec.loader.exec_module(result)
    return result


steam = module('steam_registration', 'add-to-steam.py')
installer = module('launcher_steam_install', 'install-launcher.py')


class SteamTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='Steam path ä $ ')
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.desktop = self.base / 'Pokémon 100%.desktop'; self.desktop.write_text('[Desktop Entry]\n')
        self.state = self.base / 'data/steam-registration.json'

    def test_native_helper_request_once_and_explicit_retry(self):
        with patch.object(steam.shutil, 'which', return_value='/usr/bin/steamos-add-to-steam'), \
                patch.object(steam.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, '', '')) as run, \
                patch.dict(os.environ, {'LD_PRELOAD': '/TEST-ONLY/bad.so'}):
            self.assertIn('geöffnet', steam.register(self.desktop, self.state))
            self.assertEqual(run.call_args.args[0], ['/usr/bin/steamos-add-to-steam', str(self.desktop)])
            self.assertNotIn('LD_PRELOAD', run.call_args.kwargs['env'])
            self.assertEqual(json.loads(self.state.read_text())['status'], 'requested')
            steam.register(self.desktop, self.state)
            self.assertEqual(run.call_count, 1)
            steam.register(self.desktop, self.state, retry=True)
            self.assertEqual(run.call_count, 2)
        self.assertEqual(self.state.stat().st_mode & 0o777, 0o600)
        self.assertFalse((self.base / '.steam').exists())

    def test_missing_helper_does_not_mark_request_and_can_retry_later(self):
        with patch.object(steam.shutil, 'which', return_value=None):
            with self.assertRaisesRegex(RuntimeError, 'fehlt'): steam.register(self.desktop, self.state)
        self.assertFalse(self.state.exists())
        with patch.object(steam.shutil, 'which', return_value='/TEST-ONLY/helper'), \
                patch.object(steam.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, '', '')):
            steam.register(self.desktop, self.state)
        self.assertTrue(self.state.is_file())

    def test_failed_or_timed_out_request_is_not_silently_duplicated(self):
        for error in (subprocess.CompletedProcess([], 1, '', 'TEST-ONLY failure'),
                      subprocess.TimeoutExpired('TEST-ONLY helper', 30)):
            self.state.unlink(missing_ok=True)
            with self.subTest(error=type(error).__name__), \
                    patch.object(steam.shutil, 'which', return_value='/TEST-ONLY/helper'), \
                    patch.object(steam.subprocess, 'run') as run:
                if isinstance(error, Exception): run.side_effect = error
                else: run.return_value = error
                with self.assertRaisesRegex(RuntimeError, 'bestätigt'): steam.register(self.desktop, self.state)
                self.assertEqual(json.loads(self.state.read_text())['status'], 'pending')
                steam.register(self.desktop, self.state)
                self.assertEqual(run.call_count, 1)

    def test_missing_desktop_and_corrupt_state_are_reported(self):
        self.desktop.unlink()
        with self.assertRaisesRegex(RuntimeError, 'fehlt'): steam.register(self.desktop, self.state)
        self.desktop.touch(); self.state.parent.mkdir()
        for text in ('INVALID TEST DATA', 'null', '[]', '{}'):
            self.state.write_text(text)
            with self.subTest(text=text), self.assertRaises(ValueError): steam.register(self.desktop, self.state)

    def test_retry_action_executes_on_host_before_container_entry(self):
        project = self.base / 'project'; binary = project / 'build/bin/platinum-coop'
        binary.parent.mkdir(parents=True); binary.touch()
        script = project / 'scripts/add-to-steam.py'; script.parent.mkdir()
        script.write_text('import sys,json\nprint(json.dumps(sys.argv[1:]))\n')
        wrapper, desktop = installer.install(project, self.base / 'data', self.base / 'bin', 'TEST-ONLY-container')
        # There is no distrobox in PATH here: successfully dispatching this action
        # proves that registration is handled on the host, outside the runtime.
        args = json.loads(subprocess.check_output([str(wrapper), '--add-to-steam'], text=True))
        self.assertEqual(args, ['--desktop', str(desktop), '--state',
                               str(self.base / 'data/pokemon-platinum-local-coop/steam-registration.json'), '--retry', '--dialog'])
        self.assertIn('[Desktop Action AddToSteam]', desktop.read_text())

    def test_cli_refuses_root(self):
        with patch.object(steam.os, 'geteuid', return_value=0), \
                patch('sys.argv', ['add-to-steam.py', '--desktop', str(self.desktop), '--state', str(self.state)]):
            self.assertEqual(steam.main(), 1)
        self.assertFalse(self.state.exists())
