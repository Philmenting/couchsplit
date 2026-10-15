#!/usr/bin/env python3
"""Install an owned wrapper/desktop entry. Does not modify Steam shortcuts.vdf."""
import argparse
import os
from pathlib import Path
import shlex
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
    wrapper = bin_home / 'pokemon-platinum-local-coop'
    args = ['env', 'DBX_CONTAINER_MANAGER=podman', 'distrobox', 'enter', '--name', box, '--no-tty', '--no-workdir', '--',
            'env', '-u', 'LD_PRELOAD', '-u', 'LD_LIBRARY_PATH', '-u', 'QT_PLUGIN_PATH', '-u', 'QML2_IMPORT_PATH', str(binary)] if box else [str(binary)]
    environment = ''
    if box:
        environment = ('if [[ -z "${QT_QPA_PLATFORM:-}" ]]; then\n'
                       '  if [[ -n "${DISPLAY:-}" ]]; then export QT_QPA_PLATFORM=xcb; else export QT_QPA_PLATFORM=wayland; fi\nfi\n'
                       'export QT_QUICK_BACKEND=software QT_XCB_GL_INTEGRATION=none\n')
    options = ' --fullscreen'
    if box: options += ' --data-dir ' + shlex.quote(str(data_home / 'pokemon-platinum-local-coop'))
    atomic_text(wrapper, '#!/usr/bin/env bash\nset -euo pipefail\n' + environment + 'exec ' + shlex.join(args) + options + ' "$@"\n', 0o755)
    desktop = data_home / 'applications/pokemon-platinum-local-coop.desktop'
    desktop.parent.mkdir(parents=True, exist_ok=True)
    atomic_text(desktop, '[Desktop Entry]\nType=Application\nName=Pokémon Platinum Local Co-op\n'
                + 'Comment=Zwei Spieler · Project PM · ROM hinzufügen und starten\n'
                + 'Exec="' + desktop_escape(str(wrapper)) + '"\nTerminal=false\nIcon=applications-games\nCategories=Game;Emulator;\n')
    return wrapper, desktop


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--distrobox')
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    data = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share'))
    wrapper, desktop = install(project, data, Path.home() / '.local/bin', args.distrobox)
    print(f'Launcher: {wrapper}\nDesktop-Eintrag: {desktop}\nIn Steam als Nicht-Steam-Spiel hinzufügen. Steam Input für dieses Spiel deaktivieren.')
