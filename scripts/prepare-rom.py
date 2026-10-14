#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply a supplied xdelta patch to a verified German Platinum base ROM.

Expected SHA-256 values must come from the patch publisher's release metadata.
This tool contains no Project-PM ROM patch and does not port an English patch.
"""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def sha256(path):
    with open(path, 'rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def fingerprint(value):
    if not re.fullmatch('[0-9a-fA-F]{64}', value):
        raise ValueError('Erwartete Prüfsumme muss ein vollständiger SHA-256-Wert sein.')
    return value.lower()


def check_rom(path):
    if not 512 <= path.stat().st_size <= 512 * 1024 * 1024:
        raise ValueError('ROM-Dateigröße ist ungültig.')
    with open(path, 'rb') as source:
        if source.read(16)[12:16] != b'CPUD':
            raise ValueError('Deutsche Platin-ROM mit CPUD-Kennung erforderlich. Eine Kennung allein beweist keine übersetzten Inhalte oder Koop-Kompatibilität.')


def prepare(rom, patch, output, source_sha256, patch_sha256, target_sha256, decoder='xdelta3'):
    expected = [fingerprint(v) for v in (source_sha256, patch_sha256, target_sha256)]
    rom, patch = Path(rom).resolve(strict=True), Path(patch).resolve(strict=True)
    output = Path(output).absolute()
    repository = Path(__file__).resolve().parents[1]
    parent = output.parent.resolve(strict=True)
    if parent == repository or repository in parent.parents:
        raise ValueError('Die erzeugte ROM muss außerhalb des Git-Repositories liegen.')
    if os.path.lexists(output):
        raise FileExistsError('Zieldatei existiert bereits; sie wird nicht überschrieben.')
    check_rom(rom)
    # Immutable private snapshots: checked bytes are the bytes xdelta receives.
    with tempfile.TemporaryDirectory(prefix='.platinum-patch-', dir=parent) as temp:
        temp = Path(temp)
        source, delta, target = temp / 'source.nds', temp / 'patch.xdelta', temp / 'target.nds'
        shutil.copyfile(rom, source); shutil.copyfile(patch, delta)
        if sha256(source) != expected[0]:
            raise ValueError('Basis-ROM-Prüfsumme passt nicht zum Patch. Sprachversion/Revision prüfen; keine Prüfung deaktivieren.')
        if sha256(delta) != expected[1]:
            raise ValueError('Patch-Prüfsumme stimmt nicht mit der erwarteten Veröffentlichung überein.')
        with open(delta, 'rb') as data:
            if data.read(3) != b'\xd6\xc3\xc4':
                raise ValueError('Kein xdelta/VCDIFF-Patch; BPS/IPS werden hier nicht unterstützt.')
        env = dict(os.environ); env.pop('XDELTA', None)
        result = subprocess.run([decoder, '-d', '-s', str(source), str(delta), str(target)],
                                env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True, timeout=180)
        if result.returncode:
            raise ValueError('xdelta konnte den Patch nicht anwenden: ' + result.stdout[-2000:])
        check_rom(target)
        if sha256(target) != expected[2]:
            raise ValueError('Ergebnis-Prüfsumme passt nicht zur erwarteten gepatchten ROM. Kein Ergebnis übernommen.')
        target.chmod(0o600)
        with open(target, 'rb') as data: os.fsync(data.fileno())
        # Atomic create without replacement, also if another process created output meanwhile.
        os.link(target, output)
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True); parser.add_argument('--patch', required=True)
    parser.add_argument('--output', required=True)
    for name in ('source-sha256', 'patch-sha256', 'target-sha256'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--xdelta3', default='xdelta3', help='Pfad zum vorhandenen xdelta3-Decoder')
    args = parser.parse_args()
    try:
        output = prepare(args.rom, args.patch, args.output, args.source_sha256,
                         args.patch_sha256, args.target_sha256, args.xdelta3)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, str(error) + '\n')
    print('Patch angewendet und Prüfsummen bestätigt: ' + str(output))
    print('Deutsche Texte und gemeinsame Spielwelt müssen mit dieser Patchversion im Spiel verifiziert werden.')


if __name__ == '__main__': main()
