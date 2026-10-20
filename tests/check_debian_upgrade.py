#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Native CI upgrade from the published Alpha 7 package to CouchSplit."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import urllib.request


def run(*args):
    subprocess.run(args, check=True)


def main(package):
    if os.environ.get('GITHUB_ACTIONS') != 'true' or sys.platform != 'linux':
        raise SystemExit('Run this installation check only on the disposable Linux CI runner.')
    with tempfile.TemporaryDirectory(prefix='couchsplit-upgrade-', dir=os.environ['RUNNER_TEMP']) as folder:
        root = Path(folder)
        legacy = root / 'alpha7.deb'
        url = ('https://github.com/Philmenting/pokemon-platinum-local-coop/releases/download/v0.2.0-alpha.7/'
               'Pokemon-Platinum-Coop-ZorinOS18-amd64.deb')
        with urllib.request.urlopen(url, timeout=120) as response:
            legacy.write_bytes(response.read())
        assert hashlib.sha256(legacy.read_bytes()).hexdigest() == '1f1cee0a4a21b2748a5e244dd8b6d0d7f8ab3fed54696c4e391669a847af1a76'
        run('sudo', 'apt-get', 'install', '-y', str(legacy))
        profile = root / 'user-data'
        save = profile / 'players/1/TEST_ONLY/saves/game.sav'
        save.parent.mkdir(parents=True)
        save.write_bytes(b'TEST_ONLY owned upgrade sentinel\n')
        settings = profile / 'settings.json'
        settings.write_text(json.dumps({'language': 'de', 'layout': 2, 'futureSetting': {'preserve': True}}))
        before = {p: p.read_bytes() for p in (save, settings)}
        run('sudo', 'apt-get', 'install', '-y', str(Path(package).resolve()))
        version = subprocess.check_output(['dpkg-query', '-W', '-f=${Version}', 'pokemon-platinum-local-coop'], text=True)
        assert version == '0.2.0~alpha.8', version
        desktop = Path('/usr/share/applications/pokemon-platinum-local-coop.desktop').read_text()
        assert '\nName=CouchSplit\n' in desktop
        binary = '/opt/pokemon-platinum-local-coop/project/build/bin/platinum-coop'
        capture = root / 'launcher.png'
        run(binary, '--data-dir', str(profile), '--smoke-test', str(capture))
        assert capture.stat().st_size > 1000
        assert all(p.read_bytes() == content for p, content in before.items())
        run('sudo', 'apt-get', 'remove', '-y', 'pokemon-platinum-local-coop')
        assert not Path(binary).exists()
        assert all(p.read_bytes() == content for p, content in before.items())
        print('PASS: real Alpha 7 -> CouchSplit Debian upgrade, native launcher, retained profile/settings/save, uninstall preserves data')


if __name__ == '__main__':
    main(sys.argv[1])
