#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Privately import one user-supplied Platinum ROM or ROM ZIP. No downloads."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import tempfile
import zipfile

LIMIT = 512 * 1024 * 1024


def import_rom(source, directory):
    source, directory = Path(source), Path(directory)
    repository = Path(__file__).resolve().parents[1]
    resolved = directory.resolve()
    if resolved == repository or repository in resolved.parents:
        raise ValueError('ROM-Import muss außerhalb des Quellcodeverzeichnisses liegen.')
    if not source.is_file() or source.stat().st_size > LIMIT:
        raise ValueError('ROM-Datei fehlt oder ist größer als 512 MiB.')
    directory.mkdir(parents=True, exist_ok=True, mode=0o700)
    with tempfile.TemporaryDirectory(prefix='.import-', dir=directory) as temp:
        snapshot = Path(temp) / 'rom.nds'
        archive = zipfile.ZipFile(source) if zipfile.is_zipfile(source) else None
        try:
            name = source.name
            if archive:
                entries = [x for x in archive.infolist() if not x.is_dir() and Path(x.filename).suffix.lower() in ('.nds', '.srl')]
                if len(entries) != 1 or not 512 <= entries[0].file_size <= LIMIT:
                    raise ValueError('ZIP muss genau eine DS-ROM bis 512 MiB enthalten.')
                stream = archive.open(entries[0]); name = Path(entries[0].filename).name
            else:
                stream = source.open('rb')
            digest = hashlib.sha256(); size = 0
            with stream, snapshot.open('xb') as output:
                snapshot.chmod(0o600)
                header = stream.read(512)
                if len(header) != 512 or header[12:15] != b'CPU':
                    raise ValueError('Keine Pokémon-Platin-ROM mit CPU*-Kennung.')
                output.write(header); digest.update(header); size += len(header)
                while block := stream.read(1024 * 1024):
                    size += len(block)
                    if size > LIMIT: raise ValueError('ROM überschreitet 512 MiB.')
                    output.write(block); digest.update(block)
                output.flush(); os.fsync(output.fileno())
            safe = re.sub(r'[^A-Za-z0-9_-]+', '-', Path(name).stem).strip('-')[:70] or 'Platinum'
            target = directory / (safe + '-' + digest.hexdigest()[:12] + '.nds')
            try:
                os.link(snapshot, target)
            except FileExistsError:
                if target.is_symlink() or not target.is_file(): raise ValueError('Unsicheres vorhandenes ROM-Ziel.')
                with target.open('rb') as existing:
                    if hashlib.file_digest(existing, 'sha256').hexdigest() != digest.hexdigest():
                        raise ValueError('Vorhandene importierte ROM ist beschädigt. Datei zuerst prüfen.')
            return target.resolve()
        finally:
            if archive: archive.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True); parser.add_argument('--into', required=True)
    args = parser.parse_args()
    try:
        print(import_rom(args.rom, args.into))
    except (OSError, ValueError, zipfile.BadZipFile, RuntimeError) as error:
        parser.exit(1, str(error) + '\n')
