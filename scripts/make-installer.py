#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build one .run containing Linux binaries plus complete corresponding source.

Network-free packaging. Runtime packages are fetched only when the user installs.
"""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN = {'.nds', '.srl', '.sav', '.dsv', '.rom', '.bios', '.bin', '.exe', '.dll'}


def forbidden(name):
    # This is the GPL free-BIOS source Makefile, not a firmware dump.
    return Path(name).suffix.lower() in FORBIDDEN and name != 'corresponding-source/freebios/Makefile.bios'


def git(*args, cwd): return subprocess.check_output(['git', *args], cwd=cwd)


def snapshot(repository, revision):
    result = {}
    with tarfile.open(fileobj=io.BytesIO(git('archive', revision, cwd=repository))) as archive:
        for member in archive.getmembers():
            if member.isdir(): continue
            if not member.isfile(): raise ValueError('Quellpaket enthält unerwartete Verknüpfung: ' + member.name)
            result[member.name] = (archive.extractfile(member).read(), 0o755 if member.mode & 0o111 else 0o644)
    return result


def source_files(project, upstream, windows=False):
    if git('status', '--porcelain', cwd=project).strip(): raise ValueError('Projekt vor dem Paketbau committen.')
    commit = git('rev-parse', 'HEAD', cwd=project).decode().strip()
    files = {'project/' + name: value for name, value in snapshot(project, 'HEAD').items()}
    lock = json.loads((project / 'config/upstream.lock.json').read_text())
    # Reconstruct matching source independently, then check against the source
    # tree actually used for the local build. Do not package arbitrary local edits.
    with tempfile.TemporaryDirectory(prefix='pm-source-') as temp:
        temp = Path(temp)
        expected = snapshot(upstream, lock['commit'])
        for name, (data, mode) in expected.items():
            path = temp / name; path.parent.mkdir(parents=True, exist_ok=True); path.write_bytes(data); path.chmod(mode)
        subprocess.run(['git', 'apply', str(project / 'patches/project-pm-linux-coop.patch')], cwd=temp, check=True)
        subprocess.run(['python3', str(project / 'scripts/prepare-platform.py'), str(temp)], check=True)
        for own, target in [('multiplayer/SocketCompat.h', 'src/frontend/qt_sdl/SocketCompat.h'),
                            ('display/CoopIPC.h', 'src/frontend/qt_sdl/CoopIPC.h')]:
            expected[target] = ((project / own).read_bytes(), 0o644); (temp / target).write_bytes(expected[target][0])
        for name, (_, mode) in expected.items():
            data = (temp / name).read_bytes()
            if (upstream / name).read_bytes() != data: raise ValueError('Gebauter Upstream weicht vom gepinnten Patch ab: ' + name)
            files['corresponding-source/' + name] = (data, mode)
    files['corresponding-source/UPSTREAM_COMMIT'] = ((lock['commit'] + '\nwith project/patches/project-pm-linux-coop.patch applied\n').encode(), 0o644)
    files['BUILDING.txt'] = (b'''Complete corresponding source is in project/ and corresponding-source/.
Install Ubuntu 24.04 development dependencies from project/docs/bazzite-installation.md.
From this release directory, without fetching any source:
cmake -S corresponding-source -B rebuild/emulator -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO_RELEASE=OFF
cmake --build rebuild/emulator --parallel 4
cmake -S project -B rebuild/launcher -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build rebuild/launcher --parallel 4
ctest --test-dir rebuild/launcher --output-on-failure
mkdir -p project/build/bin
cp rebuild/emulator/melonDS project/build/bin/melonDS-coop
cp rebuild/launcher/platinum-coop project/build/bin/platinum-coop
Run project/build/bin/platinum-coop in the same runtime.
See project/LICENSE and the dependency licenses in corresponding-source/.
''', 0o644)
    suffix = '.exe' if windows else ''
    binaries = tuple(name + suffix for name in ('platinum-coop', 'melonDS-coop'))
    for binary in binaries:
        data = (project / 'build/bin' / binary).read_bytes()
        valid = data[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H', data, 18)[0] == 62
        if windows:
            pe = struct.unpack_from('<I', data, 0x3c)[0] if len(data) >= 64 else len(data)
            valid = data[:2] == b'MZ' and data[pe:pe+4] == b'PE\0\0' and data[pe+4:pe+6] == b'\x64\x86'
        if not valid:
            raise ValueError('Linux-x86_64-Binary fehlt: ' + binary)
        files['project/build/bin/' + binary] = (data, 0o755)
    for name in files:
        if forbidden(name) and name not in {'project/build/bin/' + binary for binary in binaries}: raise ValueError('Geschützte/unerwartete Datei im Paket: ' + name)
    return files, dict(project_commit=commit, upstream_commit=lock['commit'], architecture='x86_64', runtime='windows-ucrt64' if windows else 'ubuntu:24.04')


def make_installer(files, metadata, output, bootstrap):
    manifest = dict(metadata, files={name: hashlib.sha256(data).hexdigest() for name, (data, _) in sorted(files.items())})
    files = dict(files, **{'release.json': (json.dumps(manifest, ensure_ascii=False, sort_keys=True, indent=2).encode(), 0o644)})
    buffer = io.BytesIO()
    with gzip.GzipFile(fileobj=buffer, mode='wb', mtime=0, compresslevel=9) as compressed:
        with tarfile.open(fileobj=compressed, mode='w') as archive:
            for name, (data, mode) in sorted(files.items()):
                info = tarfile.TarInfo(name); info.size = len(data); info.mode = mode; info.mtime = 0
                archive.addfile(info, io.BytesIO(data))
    payload = buffer.getvalue(); digest = hashlib.sha256(payload).hexdigest()
    script = bootstrap.replace('@PAYLOAD_OFFSET@', '0' * 20).replace('@PAYLOAD_SIZE@', str(len(payload))).replace('@PAYLOAD_SHA256@', digest)
    header = ('#!/usr/bin/env bash\nset -euo pipefail\n'
              'if ! command -v python3 >/dev/null; then\n'
              '  echo "Python 3 fehlt. Diesen Installer im Bazzite-Desktop ausführen." >&2\n'
              '  command -v kdialog >/dev/null && kdialog --error "Python 3 fehlt."\n  exit 1\nfi\n'
              'exec python3 - "$0" "$@" <<\'PM_COOP_BOOTSTRAP\'\n' + script + '\nPM_COOP_BOOTSTRAP\n').encode()
    header = header.replace(b'0' * 20, f'{len(header):020d}'.encode(), 1)
    with Path(output).open('xb') as target: target.write(header); target.write(payload)
    Path(output).chmod(0o755)
    return digest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream', type=Path, required=True); parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        files, metadata = source_files(ROOT, args.upstream.resolve())
        make_installer(files, metadata, args.output, (ROOT / 'scripts/installer-bootstrap.py').read_text())
    except (OSError, ValueError, subprocess.SubprocessError) as error: parser.exit(1, str(error) + '\n')
    print('Installer mit Programmen und GPL-Quellcode: ' + str(args.output.resolve()))
