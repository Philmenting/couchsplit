#!/usr/bin/env python3
"""Build and stress the real emulator Config.cpp against local file boundaries."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def test(source):
    project = Path(__file__).resolve().parents[1]
    flags = shlex.split(subprocess.check_output([
        'pkg-config', '--cflags', '--libs', 'Qt6Widgets', 'Qt6Network',
        'Qt6OpenGLWidgets', 'Qt6Multimedia', 'sdl2'], text=True))
    # This is a console test using Qt's normal entry point, not SDL2main.
    flags = [flag for flag in flags if flag not in ('-Dmain=SDL_main', '-lSDL2main', '-mwindows')]
    if os.name == 'nt':
        flags += ['-DWIN32_LEAN_AND_MEAN', '-DNOMINMAX', '-DSDL_MAIN_HANDLED']
    with tempfile.TemporaryDirectory(prefix='coop-config-') as temporary:
        binary = Path(temporary) / ('config.exe' if os.name == 'nt' else 'config')
        includes = [source / 'src', source / 'src/net', source / 'src/frontend', source / 'src/frontend/qt_sdl', project / 'build/emulator/src']
        subprocess.run(['g++', '-std=c++17', '-O1', '-UNDEBUG', '-fPIC', '-pthread',
            *['-I' + str(path) for path in includes],
            str(project / 'tests/test_config_threads.cpp'),
            str(source / 'src/frontend/qt_sdl/Config.cpp'), *flags, '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    test(parser.parse_args().source.resolve())
