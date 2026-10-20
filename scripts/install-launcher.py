#!/usr/bin/env python3
"""Install an owned wrapper/desktop entry. Does not modify Steam shortcuts.vdf."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def atomic_text(path, text, mode=0o644):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=path.parent, delete=False) as f:
        temp = Path(f.name)
        try:
            f.write(text); f.flush(); os.fsync(f.fileno()); temp.chmod(mode)
            os.replace(temp, path)
        finally:
            temp.unlink(missing_ok=True)


def desktop_escape(value):
    return value.replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$').replace('%', '%%')


def install(project, data_home, bin_home, box=None):
    binary = project / 'build/bin/platinum-coop'
    if not binary.is_file():
        raise SystemExit('Build fehlt: scripts/build.sh oder scripts/bazzite-build.sh ausführen.')
    bin_home.mkdir(parents=True, exist_ok=True)
    # Stable desktop/wrapper IDs preserve updates and Steam registration.
    wrapper = bin_home / 'pokemon-platinum-local-coop'
    desktop = data_home / 'applications/pokemon-platinum-local-coop.desktop'
    steam = steam_command(project, data_home)
    steam_action = ('if [[ "${1:-}" == --add-to-steam ]]; then\n'
                    '  exec ' + shlex.join([*steam, '--retry', '--dialog']) + '\nfi\n')
    args = ['env', 'DBX_CONTAINER_MANAGER=podman', 'distrobox', 'enter', '--name', box, '--no-tty', '--no-workdir', '--',
            'env', '-u', 'LD_PRELOAD', '-u', 'LD_LIBRARY_PATH', '-u', 'QT_PLUGIN_PATH', '-u', 'QML2_IMPORT_PATH', str(binary)] if box else [str(binary)]
    environment = ''
    if box:
        environment = ('if [[ -z "${QT_QPA_PLATFORM:-}" ]]; then\n'
                       '  if [[ -n "${DISPLAY:-}" ]]; then export QT_QPA_PLATFORM=xcb; else export QT_QPA_PLATFORM=wayland; fi\nfi\n'
                       'export QT_QUICK_BACKEND=software QT_XCB_GL_INTEGRATION=none\n')
    options = ' --fullscreen'
    if box: options += ' --data-dir ' + shlex.quote(str(data_home / 'pokemon-platinum-local-coop'))
    atomic_text(wrapper, '#!/usr/bin/env bash\nset -euo pipefail\n' + steam_action + environment + 'exec ' + shlex.join(args) + options + ' "$@"\n', 0o755)
    desktop.parent.mkdir(parents=True, exist_ok=True)
    atomic_text(desktop, '[Desktop Entry]\nType=Application\nName=CouchSplit\n'
                + 'Comment=Split-screen couch co-op with Project PM\nComment[de]=Couch-Koop und Splitscreen mit Project PM\n'
                + 'Exec="' + desktop_escape(str(wrapper)) + '"\nTerminal=false\nIcon=applications-games\nCategories=Game;Emulator;\n'
                + 'Actions=AddToSteam;\n\n[Desktop Action AddToSteam]\nName=Erneut zu Steam hinzufügen\n'
                + 'Exec="' + desktop_escape(str(wrapper)) + '" --add-to-steam\n')
    return wrapper, desktop


def steam_command(project, data_home):
    return ['python3', str(project / 'scripts/add-to-steam.py'),
            '--desktop', str(data_home / 'applications/pokemon-platinum-local-coop.desktop'),
            '--state', str(data_home / 'pokemon-platinum-local-coop/steam-registration.json')]


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--distrobox')
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    data = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share'))
    wrapper, desktop = install(project, data, Path.home() / '.local/bin', args.distrobox)
    print(f'Launcher: {wrapper}\nDesktop-Eintrag: {desktop}', flush=True)
    subprocess.run(steam_command(project, data))
