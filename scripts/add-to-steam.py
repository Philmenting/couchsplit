#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ask Bazzite's host helper to register our desktop entry; never edit Steam VDF."""
import argparse
import fcntl
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def remember(path, desktop, status):
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=path.parent, delete=False) as file:
        temp = Path(file.name)
        try:
            json.dump({'desktop': str(desktop), 'status': status}, file)
            file.flush(); os.fsync(file.fileno()); os.replace(temp, path)
        finally:
            temp.unlink(missing_ok=True)


def register(desktop, state, retry=False):
    if not desktop.is_file():
        raise RuntimeError('Startmenü-Eintrag fehlt. Installer erneut ausführen.')
    state.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    with state.with_suffix('.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if state.exists() and not retry:
            previous = json.loads(state.read_text())
            if not isinstance(previous, dict) or previous.get('status') not in ('pending', 'requested'):
                raise ValueError('Steam-Anfragedatei beschädigt. Bibliothek prüfen; fehlt der Eintrag, erneut hinzufügen.')
            if previous.get('desktop') == str(desktop):
                if previous['status'] == 'pending':
                    return ('Eine frühere Steam-Anfrage blieb unbestätigt. Bibliothek prüfen; '
                            'fehlt der Eintrag, im Startmenü „Erneut zu Steam hinzufügen“ wählen.')
                return 'Steam-Anfrage wurde bereits gesendet. In der Bibliothek unter „Steam-fremd“ nachsehen.'
        helper = shutil.which('steamos-add-to-steam')
        if not helper:
            raise RuntimeError('Bazzites „steamos-add-to-steam“ fehlt. '
                               'In Steam über „Spiel hinzufügen → Steam-fremdes Spiel“ den Launcher auswählen.')
        # Record BEFORE invoking: a timeout/crash can happen after Steam accepted the
        # request. Never silently repeat an ambiguous request on reinstallation.
        remember(state, desktop, 'pending')
        environment = dict(os.environ)
        for key in ('LD_PRELOAD', 'LD_LIBRARY_PATH', 'QT_PLUGIN_PATH', 'QML2_IMPORT_PATH'):
            environment.pop(key, None)
        try:
            result = subprocess.run([helper, str(desktop)], capture_output=True, text=True,
                                    timeout=30, env=environment)
        except (OSError, subprocess.TimeoutExpired) as error:
            raise RuntimeError('Steam-Anfrage konnte nicht bestätigt werden. Steam im Desktop öffnen '
                               'und die Bibliothek prüfen. Falls der Eintrag fehlt, erneut hinzufügen.') from error
        if result.returncode:
            raise RuntimeError('Steam hat die Anfrage nicht bestätigt. Steam im Desktop öffnen; '
                               'falls der Eintrag fehlt, erneut hinzufügen.\n' + result.stderr.strip()[:1500])
        remember(state, desktop, 'requested')
        return ('Steam wurde zum Hinzufügen geöffnet. Der Launcher sollte unter „Steam-fremd“ erscheinen. '
                'Falls nötig, die Steam-Anmeldung abschließen. Die Aufnahme ist hier noch nicht bestätigt.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--desktop', type=Path, required=True)
    parser.add_argument('--state', type=Path, required=True)
    parser.add_argument('--retry', action='store_true')
    parser.add_argument('--dialog', action='store_true')
    args = parser.parse_args()
    code = 0
    try:
        if os.geteuid() == 0: raise RuntimeError('Als normaler Benutzer starten, ohne sudo.')
        message = register(args.desktop.absolute(), args.state.absolute(), args.retry)
    except (OSError, RuntimeError, ValueError) as error:
        code = 1; message = str(error)
    print(message, flush=True)
    if args.dialog:
        if shutil.which('kdialog'):
            subprocess.run(['kdialog', '--title', 'Zu Steam hinzufügen', '--error' if code else '--msgbox', message])
        elif shutil.which('zenity'):
            subprocess.run(['zenity', '--title=Zu Steam hinzufügen', '--error' if code else '--info', '--text=' + message])
    return code


if __name__ == '__main__': raise SystemExit(main())
