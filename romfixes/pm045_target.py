# SPDX-License-Identifier: GPL-3.0-or-later
"""Fingerprint-locked cursor correction for the supplied German PM 0.4.5 ROM.

Only our newly assembled Thumb instructions are bundled, never ROM assets.
See docs/rom-target-cursor-fix.md and patches/pm045-target-cursor.s.
"""
import hashlib
from pathlib import Path
import struct

SOURCE_SHA256 = '800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c'
TARGET_SHA256 = 'dfe675837c5b935a684eb2e18c446dbc7865ba45bcb2343d2158892f6b720520'
ROM_SIZE = 141962768
OVERLAY_SHA256 = 'c6acaf7747783d49d2a1bc549efaf009ddb0f9f584a1569d41ed76b90a34d716'
OVERLAY_ADDRESS = 0x0224A700
CURSOR_START = 0x0227F3EC
CURSOR_END = 0x0227F456
# Position-independent stack/register operations; the one existing PC-relative
# literal and the intervening pool are verified by the complete input hash.
CHANGES = (
    (0x0227F3EC, bytes.fromhex(
        '079881791248305c81420dd107980179427953005b1814a8c35c0a30c35c012b02d1'
        'b9707a7020e014a816ab02330021425c9a5c012a0dd001310329f8db09e0')),
    (0x0227F440, bytes.fromhex('01200840b8704908797004e0')),
)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def patched_rom(data):
    if len(data) != ROM_SIZE or digest(data) != SOURCE_SHA256:
        raise ValueError('Es wird die unveränderte deutsche Project-PM-0.4.5-ROM (Alpha 2) benötigt. Prüfsumme stimmt nicht; keine Änderung vorgenommen.')
    table, size = struct.unpack_from('<II', data, 0x50)
    if size < 17 * 32 or table + size > len(data):
        raise ValueError('Ungültige Overlaytabelle')
    entry = struct.unpack_from('<8I', data, table + 16 * 32)
    if entry != (16, OVERLAY_ADDRESS, 235360, 64, 0x02283D9C, 0x02283DA4, 16, 0):
        raise ValueError('Unerwartetes Kampf-Overlay')
    fat, fat_size = struct.unpack_from('<II', data, 0x48)
    if fat_size < 17 * 8 or fat + fat_size > len(data):
        raise ValueError('Ungültige Dateitabelle')
    start, end = struct.unpack_from('<II', data, fat + 16 * 8)
    if end - start != entry[2] or end > len(data) or digest(data[start:end]) != OVERLAY_SHA256:
        raise ValueError('Kampf-Overlay stimmt nicht mit der geprüften Version überein')
    result = bytearray(data)
    for address, code in CHANGES:
        offset = start + address - OVERLAY_ADDRESS
        result[offset:offset + len(code)] = code
    if digest(result) != TARGET_SHA256:
        raise ValueError('Interne Ergebnisprüfung fehlgeschlagen; Ausgabe verworfen')
    return bytes(result)


def write_new_rom(source, target):
    source, target = Path(source), Path(target)
    if source.resolve() == target.resolve():
        raise ValueError('Bitte einen anderen Ausgabepfad wählen. Die ursprüngliche ROM bleibt erhalten.')
    result = patched_rom(source.read_bytes())
    # Exclusive creation also protects existing files/hard links. The old game
    # and all saves remain untouched. Remove only our own partial new output.
    with target.open('xb') as output:
        try:
            output.write(result)
            output.flush()
        except BaseException:
            output.close()
            target.unlink()
            raise
    return TARGET_SHA256
