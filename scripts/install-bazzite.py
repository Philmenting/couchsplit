#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Install bundled binaries in an owned rootless Ubuntu Distrobox runtime.

All sudo/apt commands execute INSIDE the container. Never layer host RPMs.
"""
import argparse
import fcntl
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import uuid

BOX = 'platinum-coop-runtime-v1'
IMAGE = 'docker.io/library/ubuntu:24.04'
LABEL = 'io.github.philmenting.platinum-coop'
PACKAGES = ['python3', 'libqt6widgets6', 'libqt6network6', 'libqt6multimedia6', 'libqt6svg6',
            'libqt6openglwidgets6', 'qt6-qpa-plugins', 'qt6-wayland', 'libsdl2-2.0-0',
            'libarchive13t64', 'libenet7', 'libzstd1', 'libfaad2', 'libegl1', 'libegl-mesa0',
            'libgl1-mesa-dri', 'libx11-6', 'libxcb-cursor0', 'libxkbcommon-x11-0', 'fonts-dejavu-core']
ENTER = ['distrobox', 'enter', '--name', BOX, '--no-tty', '--no-workdir', '--']
CLEAN = ['env', '-u', 'LD_PRELOAD', '-u', 'LD_LIBRARY_PATH', '-u', 'QT_PLUGIN_PATH', '-u', 'QML2_IMPORT_PATH']


class Dialogs:
    def __init__(self, silent=False):
        graphical = bool(os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')) and not silent
        self.kdialog = shutil.which('kdialog') if graphical else None
        self.zenity = shutil.which('zenity') if graphical and not self.kdialog else None
        candidates = ['qdbus6', 'qdbus-qt6', 'qdbus', '/usr/lib64/qt6/bin/qdbus', '/usr/lib/qt6/bin/qdbus']
        self.qdbus = next((shutil.which(x) for x in candidates if shutil.which(x)), None)
        if self.kdialog and not self.qdbus and shutil.which('zenity'):
            self.kdialog = None; self.zenity = shutil.which('zenity')
        self.ref = []; self.progress = None

    def message(self, text, error=False, question=False):
        print(text, flush=True)
        if self.kdialog:
            return subprocess.run([self.kdialog, '--title', 'Pokémon Platinum Local Co-op',
                                   '--yesno' if question else ('--error' if error else '--msgbox'), text]).returncode == 0
        if self.zenity:
            return subprocess.run([self.zenity, '--title=Pokémon Platinum Local Co-op',
                                   '--question' if question else ('--error' if error else '--info'), '--text=' + text]).returncode == 0
        if not question: return True
        try: return input('Installieren? [j/N] ').strip().lower() in ('j', 'ja', 'y', 'yes')
        except EOFError: return False

    def stage(self, number, text):
        print(f'{number}/5: {text}', flush=True)
        if self.kdialog and self.qdbus:
            if not self.ref:
                result = subprocess.run([self.kdialog, '--title', 'Pokémon Platinum Local Co-op', '--progressbar', text, '5'],
                                        capture_output=True, text=True, timeout=10)
                self.ref = result.stdout.split() if result.returncode == 0 else []
            if len(self.ref) == 2:
                for args in (['setLabelText', text], ['Set', '', 'value', str(number)]):
                    subprocess.run([self.qdbus, *self.ref, *args], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
        elif self.zenity:
            if self.progress is None:
                self.progress = subprocess.Popen([self.zenity, '--progress', '--pulsate', '--auto-close', '--no-cancel',
                                                 '--title=Pokémon Platinum Local Co-op', '--text=' + text], stdin=subprocess.PIPE, text=True)
            try: self.progress.stdin.write('# ' + text + '\n'); self.progress.stdin.flush()
            except (BrokenPipeError, OSError): pass

    def close(self):
        if len(self.ref) == 2:
            subprocess.run([self.qdbus, *self.ref, 'close'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
            self.ref = []
        if self.progress:
            try: self.progress.stdin.close()
            except (BrokenPipeError, OSError): pass
            self.progress.terminate(); self.progress.wait(timeout=5); self.progress = None


def command(args, log, check=True, timeout=1200):
    log.write('\n$ ' + shlex.join(map(str, args)) + '\n'); log.flush()
    env = dict(os.environ, DBX_CONTAINER_MANAGER='podman', DBX_NON_INTERACTIVE='1')
    for key in ('LD_PRELOAD', 'LD_LIBRARY_PATH'): env.pop(key, None)
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                            env=env, timeout=timeout)
    log.write(result.stdout); log.flush()
    if check and result.returncode:
        raise RuntimeError('Schritt fehlgeschlagen: ' + shlex.join(map(str, args[:8])))
    return result


def install(payload, root, data_home, bin_home, release_id, ui, log):
    if not release_id or any(c not in '0123456789abcdef' for c in release_id): raise ValueError('Ungültige Release-ID.')
    if any(c in str(root) for c in '\n\r:'): raise ValueError('Installationspfad enthält nicht unterstützte Zeichen.')
    ui.stage(1, 'Eigene Laufzeitumgebung vorbereiten …')
    rootless = command(['podman', 'info', '--format', '{{.Host.Security.Rootless}}'], log)
    if rootless.stdout.strip().lower() != 'true': raise RuntimeError('Podman muss ohne Root-Rechte laufen (rootless).')
    exists = command(['podman', 'container', 'exists', BOX], log, check=False)
    if exists.returncode not in (0, 1): raise RuntimeError('Rootless Podman startet nicht. Installationslog prüfen.')
    if exists.returncode == 0:
        owner = command(['podman', 'inspect', '--format', '{{ index .Config.Labels "' + LABEL + '" }}', BOX], log)
        if owner.stdout.strip() != 'runtime-v1': raise RuntimeError('Containername ist bereits fremd belegt: ' + BOX)
    else:
        command(['distrobox', 'create', '--yes', '--no-entry', '--name', BOX, '--image', IMAGE,
                 '--additional-flags', '--label ' + LABEL + '=runtime-v1', '--volume', f'{root}:{root}:rw'], log)
    ui.stage(2, 'Benötigte Laufzeitpakete herunterladen und einrichten. Bitte warten …')
    command([*ENTER, 'sudo', '-n', 'sh', '-c',
             'if [ -f /etc/apt/sources.list.d/ubuntu.sources ]; then '
             'sed -i "/^Components:/ { /universe/! s/$/ universe/; }" /etc/apt/sources.list.d/ubuntu.sources; fi'], log)
    command([*ENTER, 'sudo', '-n', 'env', 'DEBIAN_FRONTEND=noninteractive', 'apt-get', 'update'], log)
    command([*ENTER, 'sudo', '-n', 'env', 'DEBIAN_FRONTEND=noninteractive', 'apt-get', 'install', '-y', '--no-install-recommends', *PACKAGES], log)
    ui.stage(3, 'Programme und zugehörigen Quellcode installieren …')
    software = root / 'software'; releases = software / 'releases'; releases.mkdir(parents=True, exist_ok=True)
    release = releases / release_id
    current = software / 'current'
    if os.path.lexists(current) and not current.is_symlink(): raise RuntimeError('Installationsverknüpfung ist durch eine andere Datei belegt.')
    if not release.exists():
        with tempfile.TemporaryDirectory(prefix='.staging-', dir=releases) as temp:
            stage = Path(temp) / 'release'; shutil.copytree(payload, stage)
            stage.rename(release)
    manifest = json.loads((payload / 'release.json').read_text())
    for name, expected in manifest['files'].items():
        path = release / name
        if path.is_symlink() or not path.is_file(): raise RuntimeError('Installationsdatei fehlt: ' + name)
        with path.open('rb') as source:
            if hashlib.file_digest(source, 'sha256').hexdigest() != expected: raise RuntimeError('Installationsdatei verändert: ' + name)
    # A failed container/probe never changes current or the existing shortcuts.
    ui.stage(4, 'Emulator und Launcher in der Laufzeitumgebung prüfen …')
    binary = release / 'project/build/bin'
    result = command([*ENTER, *CLEAN, str(binary / 'melonDS-coop'), '--coop-capabilities'], log, timeout=30)
    if 'PMCOOP_ABI=1' not in result.stdout: raise RuntimeError('Emulator-Schnittstelle fehlt.')
    with tempfile.TemporaryDirectory(prefix='.install-check-', dir=root) as check:
        command([*ENTER, *CLEAN, 'QT_QPA_PLATFORM=offscreen', 'SDL_AUDIODRIVER=dummy',
                 str(binary / 'platinum-coop'), '--data-dir', str(Path(check) / 'profiles'),
                 '--smoke-test', str(Path(check) / 'launcher.png')], log, timeout=30)
        if not (Path(check) / 'launcher.png').is_file(): raise RuntimeError('Launcher-Startprüfung lieferte kein Bild.')
    ui.stage(5, 'Startmenü-Eintrag anlegen …')
    previous = os.readlink(current) if current.is_symlink() else None
    pending = software / ('.current-' + uuid.uuid4().hex)
    pending.symlink_to(Path('releases') / release_id); os.replace(pending, current)
    try:
        module_path = release / 'project/scripts/install-launcher.py'
        spec = importlib.util.spec_from_file_location('install_launcher', module_path)
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        return module.install(current / 'project', data_home, bin_home, BOX)[0]
    except Exception:
        if previous is None: current.unlink()
        else:
            pending.symlink_to(previous); os.replace(pending, current)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--payload', type=Path, required=True)
    parser.add_argument('--release-id', required=True)
    parser.add_argument('--yes', action='store_true'); parser.add_argument('--no-launch', action='store_true')
    parser.add_argument('--console', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    ui = Dialogs(silent=args.yes or args.console)
    if not args.yes and not args.console and (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')):
        if not ((ui.kdialog and ui.qdbus) or ui.zenity):
            terminal = shutil.which('konsole')
            if terminal:
                raise SystemExit(subprocess.run([terminal, '--nofork', '-e', sys.executable, __file__, *sys.argv[1:], '--console']).returncode)
            raise SystemExit('Für den grafischen Installer KDialog mit qdbus, Zenity oder Konsole verwenden.')
    data = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share')).resolve()
    root = data / 'pokemon-platinum-local-coop'; root.mkdir(parents=True, exist_ok=True, mode=0o700)
    logs = root / 'installer-logs'; logs.mkdir(exist_ok=True)
    log_path = logs / (time.strftime('%Y%m%d-%H%M%S') + '-' + uuid.uuid4().hex[:6] + '.log')
    try:
        if sys.platform != 'linux' or platform.machine() != 'x86_64': raise RuntimeError('Dieser Installer benötigt Bazzite Linux x86_64.')
        if os.geteuid() == 0: raise RuntimeError('Als normaler Benutzer starten, ohne sudo.')
        for dependency in ('podman', 'distrobox'):
            if not shutil.which(dependency): raise RuntimeError(dependency + ' fehlt. Bazzite-Desktop mit vorhandener Distrobox verwenden.')
        if shutil.disk_usage(root).free < 3 * 1024**3: raise RuntimeError('Mindestens 3 GiB freien Speicher bereitstellen.')
        if not args.yes and not ui.message('Pokémon Platinum Local Co-op installieren?\n\n'
                'Die Programme sind enthalten. Laufzeitpakete werden automatisch heruntergeladen.\n'
                'Internet und mindestens 3 GiB freier Speicher werden benötigt.\n'
                'Danach fügst du deine vorbereitete Project-PM-ROM oder deren ZIP hinzu.\n'
                'Spielstände bleiben erhalten. Einrichtung im Desktopmodus.', question=True): return
        with (root / '.installer.lock').open('a') as lock, log_path.open('w') as log:
            try: fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError: raise RuntimeError('Eine Installation läuft bereits.')
            wrapper = install(args.payload.resolve(), root, data, Path.home() / '.local/bin', args.release_id, ui, log)
        ui.close()
        if not args.yes: ui.message('Installation abgeschlossen.\n\nJetzt deine ROM oder das ROM-ZIP auswählen.\n'
                     'Später findest du das Spiel im Startmenü unter „Pokémon Platinum Local Co-op“.\n\n'
                     'Für Gaming Mode einmal in Steam: Spiel hinzufügen → Steam-fremdes Spiel.\n'
                     'Für diesen Eintrag Steam Input deaktivieren, falls doppelte Eingaben auftreten.')
        if not args.no_launch:
            with log_path.open('a') as log:
                subprocess.Popen([str(wrapper), '--first-run'], stdout=log, stderr=log, start_new_session=True, cwd=Path.home())
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        ui.close(); ui.message(str(error) + '\n\nDetails: ' + str(log_path), error=True)
        raise SystemExit(1)


if __name__ == '__main__': main()
