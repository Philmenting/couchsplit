# SPDX-License-Identifier: GPL-3.0-or-later
"""Fingerprint-locked item HP barrier correction for German PM + Zielfix 1.

Only newly authored instructions are bundled. Native item healing stays intact.
The target-selection correction in the input ROM is retained unchanged.
"""
from pathlib import Path
import struct

from .pm045_target import digest

SOURCE_SHA256 = 'dfe675837c5b935a684eb2e18c446dbc7865ba45bcb2343d2158892f6b720520'
TARGET_SHA256 = 'e90a53f4dd7c24d45e74c1854cfd5cb17fb4dc636179350fea6df8b9bfbd4a7f'
ROM_SIZE = 141962768
OVERLAY_SHA256 = 'ff9b7ee30093a6c92ed3bb95b6e139b4ec8a2144bcfb8c944f926759fcd2880c'
OVERLAY_ENTRY = (122, 0x02283ea0, 85152, 59744, 0x022988a0, 0x022988a4, 122, 0)
CODE_START = 0x02294af8
CODE_END = 0x02294b68
FILTER_START = 0x02294b3c
HOOK_START = 0x02295f60
ORIGINAL_SYNC = 0x02295c54
CODE = bytes.fromhex('b84f387f0e2833d080373e89002e2fddad48406900282bd1f8f716fdc02141436918af480a58002a22dd4c380b5afa899a421dd150300a58964200dd160004380e5015e008b5b823cb58012b0cd1bc23cb58012b08d1cc23cb584c2292000132934201d1002008bd01f078f808bdc046')
HOOK = bytes.fromhex('fef7ecfd')
PATCHES = ((CODE_START, CODE), (HOOK_START, HOOK))
LITERALS = ((0x02294ddc, 0x0229d540), (0x02294dc0, 0x0229ce40),
            (0x02294dd8, 0x00002dcc))


def patched_rom(data):
    if len(data) != ROM_SIZE or digest(data) != SOURCE_SHA256:
        raise ValueError('Es wird die geprüfte deutsche PM-0.4.5-ROM mit Zielfix 1 benötigt. Prüfsumme stimmt nicht; keine Änderung vorgenommen.')
    table, table_size = struct.unpack_from('<II', data, 0x50)
    if table_size < 123 * 32 or table + table_size > len(data):
        raise ValueError('Ungültige Overlaytabelle')
    entry = struct.unpack_from('<8I', data, table + 122 * 32)
    if entry != OVERLAY_ENTRY:
        raise ValueError('Unerwartetes Koop-Overlay')
    fat, fat_size = struct.unpack_from('<II', data, 0x48)
    if fat_size < 123 * 8 or fat + fat_size > len(data):
        raise ValueError('Ungültige Dateitabelle')
    start, end = struct.unpack_from('<II', data, fat + 122 * 8)
    if (end - start != entry[2] or end > len(data)
            or digest(data[start:end]) != OVERLAY_SHA256):
        raise ValueError('Koop-Overlay stimmt nicht mit der geprüften Version überein')
    if len(CODE) != CODE_END - CODE_START:
        raise ValueError('Interne Codegrößenprüfung fehlgeschlagen')
    for address, expected in LITERALS:
        if struct.unpack_from('<I', data, start + address - entry[1])[0] != expected:
            raise ValueError('Unerwartete Koop-Konstanten')
    if len(HOOK) != 4 or FILTER_START != CODE_START + 68:
        raise ValueError('Interne Hook-Prüfung fehlgeschlagen')
    result = bytearray(data)
    for address, code in PATCHES:
        offset = start + address - entry[1]
        result[offset:offset + len(code)] = code
    if digest(result) != TARGET_SHA256:
        raise ValueError('Interne Ergebnisprüfung fehlgeschlagen; Ausgabe verworfen')
    return bytes(result)


def write_new_rom(source, target):
    source, target = Path(source), Path(target)
    if source.resolve() == target.resolve():
        raise ValueError('Bitte eine neue Ausgabedatei wählen. Die ursprüngliche ROM bleibt erhalten.')
    result = patched_rom(source.read_bytes())
    with target.open('xb') as output:
        try:
            output.write(result)
            output.flush()
        except BaseException:
            output.close()
            target.unlink()
            raise
    return TARGET_SHA256
