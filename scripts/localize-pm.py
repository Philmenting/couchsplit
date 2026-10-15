#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a German text localization of the exact US-based PM 0.4.5 release.

Requires three legally supplied ROMs. No assets, ROMs or delta patches bundled.
Outputs a trimmed CPUE ROM; this does not port the German executable to PM.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from localization.formats import Rom, crc16
from localization.merge import localize

INPUTS = {
    'us': 'fbce4c4def0c7797f8dd238a3d7a5e48b4a7e3abd86890ac65f6321cef781bdb',
    'german': '9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217',
    'pm': '8ef9d4eeee81513f2d6e13c36125cdbfa4c6f498b039ef4b99faa7095d39cfa5',
}
DICTIONARY = ROOT / 'localization/de_pm045.json'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def audit(original, result, replacements):
    """Byte-level proof: only selected FAT slots and three header fields change
    within the original image. New resources are appended after that image.
    This also covers unnamed overlay FAT entries and executable metadata.
    """
    before, after = original.data, result.data
    if len(after) < len(before): raise ValueError('ROM was truncated')
    expected = bytearray(before)
    for name in replacements:
        offset = original.fat_offset + 8 * original.names[name]
        expected[offset:offset + 8] = after[offset:offset + 8]
    for start, length in ((0x14, 1), (0x80, 4), (0x15E, 2)):
        expected[start:start + length] = after[start:start + length]
    if bytes(expected) != after[:len(before)]: raise ValueError('Unexpected original ROM byte modification')
    if struct.unpack_from('<H', after, 0x15E)[0] != crc16(after[:0x15E]):
        raise ValueError('Header CRC mismatch')
    if struct.unpack_from('<I', after, 0x80)[0] != len(after): raise ValueError('ROM size mismatch')
    if after[12:16] != b'CPUE': raise ValueError('US engine identity changed')


def build(us, german, pm, output):
    output = Path(output).absolute()
    parent = output.parent.resolve(strict=True)
    if parent == ROOT or ROOT in parent.parents:
        raise ValueError('ROM-Ausgabe muss außerhalb des Git-Repositories liegen.')
    if os.path.lexists(output): raise FileExistsError('Zieldatei existiert bereits; kein Überschreiben.')
    roms = {}
    for key, path in (('us', us), ('german', german), ('pm', pm)):
        path = Path(path)
        if path.stat().st_size != 134217728: raise ValueError('Unerwartete ROM-Größe: ' + key)
        data = path.read_bytes()  # Immutable snapshots: checked bytes are processed bytes.
        if digest(data) != INPUTS[key]: raise ValueError('ROM-Prüfsumme falsch: ' + key)
        roms[key] = Rom(data)
    dictionary_bytes = DICTIONARY.read_bytes()
    translations = json.loads(dictionary_bytes)
    replacements, stats, missing = localize(roms['us'], roms['german'], roms['pm'], translations)
    if missing: raise ValueError(f'{len(missing)} PM-Texte ohne Übersetzung. Ausgabe abgebrochen.')
    encoded = roms['pm'].replace(replacements)
    audit(roms['pm'], Rom(encoded), replacements)
    with tempfile.NamedTemporaryFile(prefix='.pm-de-', dir=parent) as temporary:
        os.chmod(temporary.name, 0o600)
        temporary.write(encoded); temporary.flush(); os.fsync(temporary.fileno())
        os.link(temporary.name, output)  # Atomic create, never replace an existing file.
    return dict(version=translations['version'], input_sha256=INPUTS,
                dictionary_sha256=digest(dictionary_bytes), output_sha256=digest(encoded),
                output_size=len(encoded), game_code='CPUE', statistics=stats,
                changed_resources=sorted(replacements), executable_bytes_preserved=True,
                all_other_original_bytes_preserved_except_fat_and_header=True,
                world_synchronization_verified=False, full_playthrough_verified=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('us', 'german', 'pm', 'output'): parser.add_argument('--' + key, required=True)
    args = parser.parse_args()
    try:
        report = build(args.us, args.german, args.pm, args.output)
    except (OSError, ValueError, KeyError, struct.error) as error:
        parser.exit(1, str(error) + '\n')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__': main()
