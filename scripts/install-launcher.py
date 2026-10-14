#!/usr/bin/env python3
"""Install an owned wrapper/desktop entry. Does not modify Steam shortcuts.vdf."""
import argparse
import os
from pathlib import Path
import shlex


def desktop_escape(value):
    return value.replace('\\', '\\\\').replace('"', '\\"').replace('`', '\\`').replace('$', '\\$').replace('%', '%%')


def install(project, data_home, bin_home, box=None):
    binary = project / 'build/bin/platinum-coop'
    if not binary.is_file():
        raise SystemExit('Build fehlt: scripts/build.sh oder scripts/bazzite-build.sh ausführen.')
    bin_home.mkdir(parents=True, exist_ok=True)
    wrapper = bin_home / 'pokemon-platinum-local-coop'
    args = ['distrobox', 'enter', box, '--', str(binary)] if box else [str(binary)]
    wrapper.write_text('#!/usr/bin/env bash\nset -euo pipefail\nexec ' + shlex.join(args) + ' --fullscreen "$@"\n')
    wrapper.chmod(0o755)
    desktop = data_home / 'applications/pokemon-platinum-local-coop.desktop'
    desktop.parent.mkdir(parents=True, exist_ok=True)
    desktop.write_text('[Desktop Entry]\nType=Application\nName=Pokémon Platinum Local Co-op\n'
                       + 'Exec="' + desktop_escape(str(wrapper)) + '"\nTerminal=false\nCategories=Game;Emulator;\n')
    return wrapper, desktop


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--distrobox')
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    data = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share'))
    wrapper, desktop = install(project, data, Path.home() / '.local/bin', args.distrobox)
    print(f'Launcher: {wrapper}\nDesktop-Eintrag: {desktop}\nIn Steam als Nicht-Steam-Spiel hinzufügen. Steam Input für dieses Spiel deaktivieren.')
