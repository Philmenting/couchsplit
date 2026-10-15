# SPDX-License-Identifier: GPL-3.0-or-later
"""Embedded by make-installer.py. Verify and unpack before running installation."""
import hashlib
from contextlib import ExitStack
import io
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tarfile
import tempfile


def unpack(payload, destination):
    with tarfile.open(fileobj=io.BytesIO(payload), mode='r:gz') as archive:
        members = archive.getmembers(); names = set(); total = 0
        if len(members) > 10000: raise ValueError('Zu viele Paketdateien.')
        for member in members:
            path = PurePosixPath(member.name)
            if path.is_absolute() or '..' in path.parts or not member.isfile() or member.name in names:
                raise ValueError('Unsicherer Paketpfad: ' + member.name)
            total += member.size
            if total > 256 * 1024**2 or member.size > 64 * 1024**2: raise ValueError('Installationspaket zu groß.')
            names.add(member.name)
        for member in members:
            target = destination / member.name; target.parent.mkdir(parents=True, exist_ok=True)
            with archive.extractfile(member) as source, target.open('xb') as output:
                while block := source.read(1024 * 1024): output.write(block)
            target.chmod(0o755 if member.mode & 0o111 else 0o644)
    manifest = json.loads((destination / 'release.json').read_text())
    if names != set(manifest['files']) | {'release.json'}: raise ValueError('Paketdateiliste stimmt nicht.')
    for name, expected in manifest['files'].items():
        with (destination / name).open('rb') as data:
            if hashlib.file_digest(data, 'sha256').hexdigest() != expected: raise ValueError('Beschädigte Paketdatei: ' + name)
    return manifest


def main():
    # Fixed-width offset and immutable payload fingerprints are filled at packaging.
    offset = int('@PAYLOAD_OFFSET@')
    size = int('@PAYLOAD_SIZE@')
    expected = '@PAYLOAD_SHA256@'
    if size > 100 * 1024**2: raise ValueError('Ungültige Paketgröße.')
    with Path(sys.argv[1]).open('rb') as source:
        source.seek(offset); payload = source.read(size + 1)
    if len(payload) != size or hashlib.sha256(payload).hexdigest() != expected:
        raise ValueError('Download beschädigt oder unvollständig. Installer erneut herunterladen.')
    args = sys.argv[2:]
    if any(arg not in ('--check', '--yes', '--no-launch') for arg in args): raise ValueError('Unbekannte Installer-Option.')
    with tempfile.TemporaryDirectory(prefix='platinum-coop-install-') as temp:
        directory = Path(temp); manifest = unpack(payload, directory)
        if '--check' in args:
            print('Paket vollständig geprüft: ' + str(len(manifest['files'])) + ' Dateien, keine Installation ausgeführt.')
            return
        with ExitStack() as stack:
            try: input_stream = stack.enter_context(open('/dev/tty', 'rb'))
            except OSError: input_stream = subprocess.DEVNULL
            result = subprocess.run([sys.executable, str(directory / 'project/scripts/install-bazzite.py'),
                                     '--payload', str(directory), '--release-id', expected[:20], *args], stdin=input_stream)
        raise SystemExit(result.returncode)


if __name__ == '__main__':
    try: main()
    except (OSError, ValueError, KeyError, tarfile.TarError, json.JSONDecodeError) as error:
        message = str(error); print(message, file=sys.stderr)
        try: subprocess.run(['kdialog', '--error', message], timeout=30)
        except (OSError, subprocess.SubprocessError): pass
        raise SystemExit(1)
