#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply a supplied xdelta patch to its verified Platinum base ROM.

Known PM releases use recorded source/patch/target fingerprints automatically.
Unknown German patches require three independently verified SHA-256 values.
This tool contains no Project-PM ROM patch and does not port an English patch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile

CATALOG_PATH = Path(__file__).resolve().parents[1] / 'config/project-pm-patches.json'


def sha256(path):
    with open(path, 'rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def fingerprint(value):
    if not re.fullmatch('[0-9a-fA-F]{64}', value):
        raise ValueError('Erwartete Prüfsumme muss ein vollständiger SHA-256-Wert sein.')
    return value.lower()


def patch_bytes(patch):
    patch = Path(patch)
    if zipfile.is_zipfile(patch):
        with zipfile.ZipFile(patch) as archive:
            entries = [e for e in archive.infolist() if e.filename.lower().endswith('.xdelta') and not e.is_dir()]
            if len(entries) != 1 or entries[0].file_size > 64 * 1024 * 1024:
                raise ValueError('Paket muss genau einen xdelta-Patch bis 64 MiB enthalten.')
            return archive.read(entries[0])
    else:
        if patch.stat().st_size > 64 * 1024 * 1024:
            raise ValueError('Patch darf höchstens 64 MiB groß sein.')
        return patch.read_bytes()


def inspect_patch(rom, patch):
    rom = Path(rom)
    patch_digest = hashlib.sha256(patch_bytes(patch)).hexdigest()
    with open(rom, 'rb') as source:
        header = source.read(512)
        source.seek(0); source_sha1 = hashlib.file_digest(source, 'sha1').hexdigest()
    if len(header) != 512:
        raise ValueError('DS-ROM-Header fehlt.')
    catalog = json.loads(CATALOG_PATH.read_text())
    expected = next((p for p in catalog if p['patch_sha256'] == patch_digest), None)
    code = header[12:16].decode('ascii', errors='replace')
    matches = bool(expected and expected['source_sha1'] == source_sha1
                   and expected['source_game_code'] == code
                   and expected['source_revision'] == header[0x1E]
                   and expected['source_size'] == rom.stat().st_size)
    return dict(patch_sha256=patch_digest, expected_base=expected,
                source_game_code=code, source_revision=header[0x1E],
                source_sha1=source_sha1, source_size=rom.stat().st_size,
                matches_documented_base=matches,
                german_coop_verified=False)


def require_known_base(report):
    expected = report['expected_base']
    if expected and not report['matches_documented_base']:
        raise ValueError('Project PM ' + expected['version'] + ' benötigt ' + expected['source_label']
                         + ' (' + expected['source_game_code'] + '). Die gewählte Basis-ROM passt nicht. '
                         + 'Dieser Patch ist kein deutscher Port; Prüfsummen nicht abschalten.')


def check_rom(path, game_code=b'CPUD'):
    if not 512 <= path.stat().st_size <= 512 * 1024 * 1024:
        raise ValueError('ROM-Dateigröße ist ungültig.')
    with open(path, 'rb') as source:
        if source.read(16)[12:16] != game_code:
            raise ValueError('Platin-ROM mit ' + game_code.decode() + '-Kennung erforderlich. Eine Kennung allein beweist keine übersetzten Inhalte oder Koop-Kompatibilität.')


def prepare(rom, patch, output, source_sha256=None, patch_sha256=None, target_sha256=None, decoder='xdelta3'):
    rom, patch = Path(rom).resolve(strict=True), Path(patch).resolve(strict=True)
    output = Path(output).absolute()
    repository = Path(__file__).resolve().parents[1]
    parent = output.parent.resolve(strict=True)
    if parent == repository or repository in parent.parents:
        raise ValueError('Die erzeugte ROM muss außerhalb des Git-Repositories liegen.')
    if os.path.lexists(output):
        raise FileExistsError('Zieldatei existiert bereits; sie wird nicht überschrieben.')
    report = inspect_patch(rom, patch)
    require_known_base(report)
    release = report['expected_base'] or {}
    expected = []
    for key, provided in zip(('source_sha256', 'patch_sha256', 'target_sha256'),
                             (source_sha256, patch_sha256, target_sha256)):
        recorded = release.get(key)
        if provided and recorded and fingerprint(provided) != recorded:
            raise ValueError('Angegebene Prüfsumme widerspricht dem bekannten Release: ' + key)
        if not (provided or recorded):
            raise ValueError('Unbekannter Patch: alle drei verifizierten SHA-256-Werte sind erforderlich.')
        expected.append(fingerprint(provided or recorded))
    source_code = release.get('source_game_code', 'CPUD').encode('ascii')
    target_code = release.get('target_game_code', 'CPUD').encode('ascii')
    check_rom(rom, source_code)
    # Immutable private snapshots: checked bytes are the bytes xdelta receives.
    with tempfile.TemporaryDirectory(prefix='.platinum-patch-', dir=parent) as temp:
        temp = Path(temp)
        source, delta, target = temp / 'source.nds', temp / 'patch.xdelta', temp / 'target.nds'
        shutil.copyfile(rom, source); delta.write_bytes(patch_bytes(patch))
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
        check_rom(target, target_code)
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
    parser.add_argument('--output')
    parser.add_argument('--inspect', action='store_true', help='Basis-ROM prüfen, ohne Dateien zu verändern; --patch akzeptiert auch das Build-ZIP')
    for name in ('source-sha256', 'patch-sha256', 'target-sha256'):
        parser.add_argument('--' + name)
    parser.add_argument('--xdelta3', default='xdelta3', help='Pfad zum vorhandenen xdelta3-Decoder')
    args = parser.parse_args()
    try:
        if args.inspect:
            report = inspect_patch(args.rom, args.patch)
            print(json.dumps(report, ensure_ascii=False, indent=2))
            require_known_base(report)
            if report['expected_base'] is None:
                parser.exit(2, 'Patch unbekannt: keine dokumentierte Basis-Kompatibilität bestätigt.\n')
            return
        if not args.output:
            parser.error('--output ist für die Patchanwendung erforderlich.')
        output = prepare(args.rom, args.patch, args.output, args.source_sha256,
                         args.patch_sha256, args.target_sha256, args.xdelta3)
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError, zipfile.BadZipFile) as error:
        parser.exit(1, str(error) + '\n')
    print('Patch angewendet und Prüfsummen bestätigt: ' + str(output))
    print('Sprache und gemeinsame Spielwelt müssen mit dieser Patchversion im Spiel verifiziert werden.')


if __name__ == '__main__': main()
