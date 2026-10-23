#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Privately import Project-PM Platinum or header-valid GBA ROMs. No downloads."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import tempfile
import zipfile

LIMIT = 512 * 1024 * 1024
GBA_LIMIT = 32 * 1024 * 1024


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
                entries = [x for x in archive.infolist() if not x.is_dir() and Path(x.filename).suffix.lower() in ('.nds', '.srl', '.gba')]
                if len(entries) != 1 or not 512 <= entries[0].file_size <= LIMIT:
                    raise ValueError('ZIP muss genau eine DS- oder GBA-ROM bis 512 MiB enthalten.')
                stream = archive.open(entries[0]); name = Path(entries[0].filename).name
            else:
                stream = source.open('rb')
            digest = hashlib.sha256(); size = 0
            with stream, snapshot.open('xb') as output:
                snapshot.chmod(0o600)
                header = stream.read(512)
                gba = len(header) == 512 and header[12:15] != b'CPU' and header[0xB2] == 0x96
                if len(header) != 512 or (header[12:15] != b'CPU' and not gba):
                    raise ValueError('Unterstützt werden Project-PM-Platin und GBA-ROMs mit gültigem Header.')
                if gba and (header[0xB2] != 0x96 or (sum(header[0xA0:0xBE]) + 0x19) & 255):
                    raise ValueError('GBA-ROM hat eine ungültige Header-Prüfsumme.')
                extended = gba and header[0xB5:0xBC] == b'CSXROM1'
                output.write(header); digest.update(header); size += len(header)
                while block := stream.read(1024 * 1024):
                    size += len(block)
                    if size > (GBA_LIMIT if gba and not extended else LIMIT):
                        raise ValueError('GBA-ROM überschreitet 32 MiB ohne CouchSplit-XROM-Kennung.' if gba and not extended else 'ROM überschreitet 512 MiB.')
                    output.write(block); digest.update(block)
                if extended and (size <= GBA_LIMIT or size % 4):
                    raise ValueError('CouchSplit-XROM muss größer als 32 MiB und auf vier Byte ausgerichtet sein.')
                output.flush(); os.fsync(output.fileno())
            safe = re.sub(r'[^A-Za-z0-9_-]+', '-', Path(name).stem).strip('-')[:70] or 'Game'
            target = directory / (safe + '-' + digest.hexdigest()[:12] + ('.gba' if gba else '.nds'))
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
