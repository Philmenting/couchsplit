#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Create a new ROM copy with the cooperative healing correction."""
import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from romfixes.pm045_items import write_new_rom


def main():
    parser = argparse.ArgumentParser(description='Korrigiert die einseitige Koop-KP-Synchronisierung bei Tränken der deutschen PM-0.4.5-ROM mit Zielfix 1. Original und Spielstände bleiben erhalten.')
    parser.add_argument('--rom', required=True, type=Path, help='Entpackte, geprüfte Zielfix-1-ROM (.nds)')
    parser.add_argument('--output', required=True, type=Path, help='Neue Datei; vorhandene Dateien werden nicht überschrieben')
    args = parser.parse_args()
    try:
        sha = write_new_rom(args.rom, args.output)
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + '\n')
    print('Neue ROM erstellt:', args.output)
    print('SHA-256:', sha)
    print('Im Koop-Starter als neue ROM hinzufügen. Der neue Fingerabdruck verwendet eigene Spielstände; vorhandene Spielstände werden nicht automatisch übertragen.')


if __name__ == '__main__':
    main()
