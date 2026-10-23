#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Install bundled binaries in an owned rootless Ubuntu Distrobox runtime.

All sudo/apt commands execute INSIDE the container. Never layer host RPMs.
"""
import argparse
import codecs
import fcntl
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import selectors
import shlex
import shutil
import signal
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
        self.label = ''; self.last_activity = 0

    def message(self, text, error=False, question=False):
        print(text, flush=True)
        if self.kdialog:
            return subprocess.run([self.kdialog, '--title', 'CouchSplit',
                                   '--yesno' if question else ('--error' if error else '--msgbox'), text]).returncode == 0
        if self.zenity:
            return subprocess.run([self.zenity, '--title=CouchSplit',
                                   '--question' if question else ('--error' if error else '--info'), '--text=' + text]).returncode == 0
        if not question: return True
        try: return input('Installieren? [j/N] ').strip().lower() in ('j', 'ja', 'y', 'yes')
        except EOFError: return False

    def stage(self, number, text):
        print(f'{number}/5: {text}', flush=True)
        self.label = f'Schritt {number} von 5\n{text}'
        self.last_activity = 0
        self.display_progress(self.label)

    def activity(self, elapsed):
        now = time.monotonic()
        if now - self.last_activity < 1: return
        self.last_activity = now
        self.display_progress(self.label + f'\n\nDieser Vorgang läuft seit {int(elapsed)} Sekunden.')

    def display_progress(self, text):
        if self.kdialog and self.qdbus:
            if not self.ref:
                # Downloads and first-time initialization have no known percentage.
                # A zero range animates continuously instead of freezing at 1/5.
                result = subprocess.run([self.kdialog, '--title', 'CouchSplit', '--progressbar', text, '0'],
                                        capture_output=True, text=True, timeout=10)
                self.ref = result.stdout.split() if result.returncode == 0 else []
            if len(self.ref) == 2:
                subprocess.run([self.qdbus, *self.ref, 'setLabelText', text],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
        elif self.zenity:
            if self.progress is None:
                self.progress = subprocess.Popen([self.zenity, '--progress', '--pulsate', '--auto-close', '--no-cancel',
                                                 '--title=CouchSplit', '--text=' + text], stdin=subprocess.PIPE, text=True)
            try: self.progress.stdin.write('# ' + text.replace('\n', ' · ') + '\n'); self.progress.stdin.flush()
            except (BrokenPipeError, OSError): pass

    def close(self):
        if len(self.ref) == 2:
            subprocess.run([self.qdbus, *self.ref, 'close'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5)
            self.ref = []
        if self.progress:
            try: self.progress.stdin.close()
            except (BrokenPipeError, OSError): pass
            self.progress.terminate(); self.progress.wait(timeout=5); self.progress = None


def command(args, log, check=True, timeout=1200, progress=None, kill_group=True):
    log.write('\n$ ' + shlex.join(map(str, args)) + '\n'); log.flush()
    env = dict(os.environ, DBX_CONTAINER_MANAGER='podman', DBX_NON_INTERACTIVE='1')
    for key in ('LD_PRELOAD', 'LD_LIBRARY_PATH'): env.pop(key, None)
    chunks = []; decoder = codecs.getincrementaldecoder('utf-8')(errors='replace')
    started = time.monotonic(); heartbeat = started + 15
    def emit(text):
        if text:
            chunks.append(text); log.write(text); log.flush()
            print(text, end='', flush=True)
    # Drain while the process runs: the previous subprocess.run(capture_output)
    # hid the entire first-start output, including on timeout.
    with subprocess.Popen(args, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, env=env, start_new_session=True) as process:
        try:
            with selectors.DefaultSelector() as streams:
                streams.register(process.stdout, selectors.EVENT_READ)
                while streams.get_map() or process.poll() is None:
                    remaining = timeout - (time.monotonic() - started)
                    if remaining <= 0: raise subprocess.TimeoutExpired(args, timeout)
                    if streams.get_map():
                        for key, _ in streams.select(min(1, remaining)):
                            block = os.read(key.fd, 65536)
                            if block: emit(decoder.decode(block))
                            else: streams.unregister(key.fileobj)
                    else:
                        try: process.wait(timeout=min(1, remaining))
                        except subprocess.TimeoutExpired: pass
                    if time.monotonic() >= heartbeat:
                        status = f'\n[Installer] Befehl läuft seit {int(time.monotonic() - started)} Sekunden.\n'
                        log.write(status); log.flush(); print(status, end='', flush=True)
                        heartbeat = time.monotonic() + 15
                    if progress: progress(time.monotonic() - started)
        except BaseException as error:
            # Terminate this command's group, including inherited pipe writers.
            # Keep Podman containers intact. Steam may launch an independent
            # client, so its registration call explicitly disables group killing.
            try:
                if kill_group: os.killpg(process.pid, signal.SIGKILL)
                else: process.kill()
            except ProcessLookupError: pass
            tail, _ = process.communicate(timeout=5)
            emit(decoder.decode(tail, final=True))
            if isinstance(error, subprocess.TimeoutExpired):
                message = f'Zeitlimit nach {timeout:g} Sekunden: ' + shlex.join(map(str, args[:8]))
                log.write('\nFEHLER: ' + message + '\n'); log.flush()
                raise RuntimeError(message) from error
            raise
        emit(decoder.decode(b'', final=True))
        result = subprocess.CompletedProcess(args, process.returncode, ''.join(chunks))
    log.write(f'\n[Installer] Exitstatus: {result.returncode}\n'); log.flush()
    if check and result.returncode:
        raise RuntimeError('Schritt fehlgeschlagen: ' + shlex.join(map(str, args[:8])))
    return result


def initialize_runtime(log, progress=None):
    try:
        command([*ENTER, 'true'], log, progress=progress)
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        log.write('\n--- Diagnose des Containerstarts ---\n'); log.flush()
        state = {}
        for args in (['podman', 'inspect', '--format', '{{json .State}}', BOX],
                     ['podman', 'logs', '--tail', '200', BOX]):
            try:
                result = command(args, log, check=False, timeout=15)
                if args[1] == 'inspect' and result.returncode == 0:
                    parsed = json.loads(result.stdout)
                    if isinstance(parsed, dict): state = parsed
            except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as diagnostic:
                log.write('Diagnose nicht verfügbar: ' + str(diagnostic) + '\n'); log.flush()
        # A stopped container can retain stale overlay mounts. Only this exact
        # reported failure permits cleanup, and only after rechecking ownership.
        problem = str(state.get('Error', '')).lower()
        if (state.get('Status') in ('exited', 'stopped') and state.get('Running') is False
                and 'creating temporary passwd file' in problem
                and '/merged/etc/passwd: no such file or directory' in problem):
            try:
                owner = command(['podman', 'inspect', '--format', '{{ index .Config.Labels "' + LABEL + '" }}', BOX], log, timeout=15)
                if owner.stdout.strip() != 'runtime-v1': raise RuntimeError('Container gehört nicht zu dieser Anwendung; keine Bereinigung.')
                log.write('\n[Installer] Temporäre Container-Einhängungen bereinigen und einmal neu starten.\n'); log.flush()
                command(['podman', 'container', 'cleanup', BOX], log, timeout=30, progress=progress)
                command([*ENTER, 'true'], log, progress=progress)
                return
            except (OSError, RuntimeError, subprocess.SubprocessError) as retry_error:
                log.write('\nBereinigungsversuch nicht erfolgreich: ' + str(retry_error) + '\n'); log.flush()
                error = retry_error
        raise RuntimeError('Start bzw. Ersteinrichtung der Distrobox fehlgeschlagen. '
                           'Containerstatus und letzte Containerausgaben stehen im Installationslog. '
                           'Es wurden keine Spielstände gelöscht.\n' + str(error)) from error


def install(payload, root, data_home, bin_home, release_id, ui, log):
    if not release_id or any(c not in '0123456789abcdef' for c in release_id): raise ValueError('Ungültige Release-ID.')
    if any(c in str(root) for c in '\n\r:'): raise ValueError('Installationspfad enthält nicht unterstützte Zeichen.')
    ui.stage(1, 'Eigene Laufzeitumgebung vorbereiten …')
    rootless = command(['podman', 'info', '--format', '{{.Host.Security.Rootless}}'], log, progress=ui.activity)
    if rootless.stdout.strip().lower() != 'true': raise RuntimeError('Podman muss ohne Root-Rechte laufen (rootless).')
    exists = command(['podman', 'container', 'exists', BOX], log, check=False)
    if exists.returncode not in (0, 1): raise RuntimeError('Rootless Podman startet nicht. Installationslog prüfen.')
    if exists.returncode == 0:
        owner = command(['podman', 'inspect', '--format', '{{ index .Config.Labels "' + LABEL + '" }}', BOX], log)
        if owner.stdout.strip() != 'runtime-v1': raise RuntimeError('Containername ist bereits fremd belegt: ' + BOX)
    else:
        command(['distrobox', 'create', '--yes', '--no-entry', '--name', BOX, '--image', IMAGE,
                 '--additional-flags', '--label ' + LABEL + '=runtime-v1', '--volume', f'{root}:{root}:rw'], log, progress=ui.activity)
    ui.stage(2, 'Container starten. Beim ersten Start werden Grundpakete geladen; das kann mehrere Minuten dauern …')
    initialize_runtime(log, ui.activity)
    ui.stage(2, 'Benötigte Laufzeitpakete herunterladen und einrichten …')
    command([*ENTER, 'sudo', '-n', 'sh', '-c',
             'if [ -f /etc/apt/sources.list.d/ubuntu.sources ]; then '
             'sed -i "/^Components:/ { /universe/! s/$/ universe/; }" /etc/apt/sources.list.d/ubuntu.sources; fi'], log, progress=ui.activity)
    command([*ENTER, 'sudo', '-n', 'env', 'DEBIAN_FRONTEND=noninteractive', 'apt-get', 'update'], log, progress=ui.activity)
    command([*ENTER, 'sudo', '-n', 'env', 'DEBIAN_FRONTEND=noninteractive', 'apt-get', 'install', '-y', '--no-install-recommends', *PACKAGES], log, progress=ui.activity)
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
    result = command([*ENTER, *CLEAN, str(binary / 'melonDS-coop'), '--coop-capabilities'], log, timeout=30, progress=ui.activity)
    if 'PMCOOP_ABI=2' not in result.stdout: raise RuntimeError('Emulator-Schnittstelle fehlt.')
    if (binary/'gba-worker').is_file():
        gba=command([*ENTER,*CLEAN,str(binary/'gba-worker'),'--coop-capabilities'],log,timeout=30,progress=ui.activity)
        if 'GBA=1' not in gba.stdout or 'STABLE_SAVE=1' not in gba.stdout:raise RuntimeError('GBA-Emulator-Schnittstelle fehlt.')
    with tempfile.TemporaryDirectory(prefix='.install-check-', dir=root) as check:
        command([*ENTER, *CLEAN, 'QT_QPA_PLATFORM=offscreen', 'SDL_AUDIODRIVER=dummy',
                 str(binary / 'platinum-coop'), '--data-dir', str(Path(check) / 'profiles'),
                 '--smoke-test', str(Path(check) / 'launcher.png')], log, timeout=30, progress=ui.activity)
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
        if not args.yes and not ui.message('CouchSplit installieren?\n\n'
                'Die Programme sind enthalten. Laufzeitpakete werden automatisch heruntergeladen.\n'
                'Internet und mindestens 3 GiB freier Speicher werden benötigt.\n'
                'Danach fügst du deine vorbereitete Project-PM-ROM oder deren ZIP hinzu.\n'
                'Spielstände bleiben erhalten. Einrichtung im Desktopmodus.', question=True): return
        with (root / '.installer.lock').open('a') as lock, log_path.open('w') as log:
            try: fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError: raise RuntimeError('Eine Installation läuft bereits.')
            wrapper = install(args.payload.resolve(), root, data, Path.home() / '.local/bin', args.release_id, ui, log)
            ui.stage(5, 'Launcher zu Steam hinzufügen …')
            # Host helper, after the application is installed. Missing/closed Steam
            # must not roll back a working runtime or affect saves.
            steam_script = root / 'software/current/project/scripts/add-to-steam.py'
            try:
                result = command([sys.executable, str(steam_script), '--desktop',
                                  str(data / 'applications/pokemon-platinum-local-coop.desktop'),
                                  '--state', str(root / 'steam-registration.json')], log, check=False, timeout=40,
                                 progress=ui.activity, kill_group=False)
                steam_message = result.stdout.strip()
            except (OSError, RuntimeError, subprocess.SubprocessError) as error:
                steam_message = 'Steam-Anfrage nicht bestätigt: ' + str(error)
        ui.close()
        if not args.yes: ui.message('Installation abgeschlossen.\n\nJetzt deine ROM oder das ROM-ZIP auswählen.\n'
                     'Später findest du das Spiel im Startmenü unter „CouchSplit“.\n\n'
                     + steam_message + '\n'
                     'Fehlt der Eintrag? Im Startmenü Rechtsklick → Erneut zu Steam hinzufügen.\n'
                     'Für diesen Eintrag Steam Input deaktivieren, falls doppelte Eingaben auftreten.')
        if not args.no_launch:
            with log_path.open('a') as log:
                subprocess.Popen([str(wrapper), '--first-run'], stdout=log, stderr=log, start_new_session=True, cwd=Path.home())
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as error:
        with log_path.open('a') as log: log.write('\nFEHLER: ' + str(error) + '\n')
        ui.close(); ui.message(str(error) + '\n\nDetails: ' + str(log_path), error=True)
        raise SystemExit(1)


if __name__ == '__main__': main()
