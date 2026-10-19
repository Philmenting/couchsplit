#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package only the authored, fingerprint-locked ROM tools and their tests."""
import argparse
import hashlib
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FILES = (
    'LICENSE', 'romfixes/__init__.py', 'romfixes/pm045_target.py',
    'romfixes/pm045_items.py', 'scripts/fix-pm-target-cursor.py',
    'scripts/fix-pm-coop-items.py', 'patches/pm045-target-cursor.s',
    'patches/pm045-item-hp.s', 'tests/test_rom_cursor.py',
    'tests/test_rom_items.py', 'tests/requirements-romfix.txt',
    'docs/rom-target-cursor-fix.md', 'docs/rom-coop-item-fix.md',
    'docs/save-transfer.md',
)
README = '''Project PM ROM correction tools / ROM-Korrekturwerkzeuge

Python 3; no extra runtime packages required. GPL-3.0-or-later (see LICENSE).
Only the exact documented German PM 0.4.5 fingerprints are accepted.
No ROM, game data, firmware, emulator binaries or saved games are included.
The tools download nothing and refuse unknown input or existing output files.

First, apply the target correction to the verified German Alpha-2 ROM:
python scripts/fix-pm-target-cursor.py --rom INPUT.nds --output TARGET-FIX.nds
Then apply the co-op item correction to the target-corrected result:
python scripts/fix-pm-coop-items.py --rom TARGET-FIX.nds --output COOP-FIX.nds

If your ROM already has the target correction, start with the second command.
Instructions, required hashes and test scope are in docs/.
Changing the ROM fingerprint creates separate player/save directories.
Back up both saves first; see docs/save-transfer.md for an explicit copy.

Optional ARM tests (extra dependency needed for the tests only):
python -m pip install -r tests/requirements-romfix.txt
python -m unittest discover -s tests -p "test_rom_*.py" -v

Deutsch: Akzeptiert nur die dokumentierten exakten ROM-Versionen. Werkzeuge
erzeugen neue Dateien und überschreiben keine Originale. Keine automatische
Spielstandübernahme. Anleitungen und Prüfgrenzen stehen in docs/.
'''


def package(root, output):
    # Read the complete allowlist before creating anything. A missing file
    # must not leave a downloadable but incomplete tool archive behind.
    files = {name: (root / name).read_bytes() for name in FILES}
    files['README.txt'] = README.encode('utf-8')
    files['SHA256SUMS'] = ''.join(
        hashlib.sha256(data).hexdigest() + '  ' + name + '\n'
        for name, data in sorted(files.items())).encode('utf-8')
    with output.open('xb') as stream:
        try:
            with zipfile.ZipFile(stream, 'w', zipfile.ZIP_DEFLATED) as archive:
                for name, data in sorted(files.items()):
                    info = zipfile.ZipInfo(name, date_time=(2026, 10, 19, 0, 0, 0))
                    info.compress_type = zipfile.ZIP_DEFLATED
                    info.external_attr = 0o100644 << 16
                    archive.writestr(info, data)
        except BaseException:
            stream.close()
            output.unlink()
            raise


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    package(ROOT, args.output)
    print(args.output)
