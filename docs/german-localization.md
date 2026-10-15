# Deutsche PM-0.4.5-Alpha

Stand 15.10.2026. **Implementiert:** deutsche Textlokalisierung der verifizierten US-PM-ROM. **Automatisiert getestet:** zwei identische Builds, Archiv-/Textprüfungen, unveränderte PM-Programmbytes, ROM-Start in zwei echten Emulatorprozessen. **Visuell geprüft:** deutsche Begrüßung mit Umlaut im Emulatorbild. **Nicht verifiziert:** kompletter Spieldurchlauf, sämtliche Menüs, Ingame-Speichern und gemeinsame Sinnoh-Welt; keine manuelle Bestätigung auf Bazzite/TV/Controllerhardware.

## Erzeugen

Python 3.11 oder neuer, keine zusätzlichen Python-Pakete. Die drei ROMs müssen selbst rechtmäßig bereitgestellt werden. ZIPs vorher entpacken. Die PM-ROM zuerst mit `scripts/prepare-rom.py` aus USA Rev 1 und dem echten PM-0.4.5-Patch erzeugen. Dabei keine xdelta-Prüfung abschalten.

```bash
python3 scripts/localize-pm.py \
  --us /pfad/original-usa-rev1.nds \
  --german /pfad/original-platin-deutsch.nds \
  --pm /pfad/platinum-pm-045-us.nds \
  --output /pfad/ausserhalb-des-repos/platinum-pm-045-de-alpha1.nds \
  > /pfad/ausserhalb-des-repos/localization-report.json
```

Das Skript verlangt die exakt geprüften SHA-256-Fingerabdrücke, liest unveränderliche Bytes, verweigert vorhandene Ziele und Ausgaben im Repository. Jede nicht aufgelöste PM-Nachricht oder veränderte Steuersequenz bricht den Build ab. Keine Originaldatei wird verändert. Im Launcher die neue `.nds` auswählen. Sie behält absichtlich `CPUE` und den US-Spielcode; das ist eine deutsche Textfassung des US-Mods, kein Port der deutschen Engine.

| Eingabe/Ergebnis | SHA-256 |
|---|---|
| USA Rev 1, CPUE | `fbce4c4def0c7797f8dd238a3d7a5e48b4a7e3abd86890ac65f6321cef781bdb` |
| Deutsche Originalfassung, CPUD Rev 0 | `9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217` |
| Gepatchte US-PM-0.4.5-ROM | `8ef9d4eeee81513f2d6e13c36125cdbfa4c6f498b039ef4b99faa7095d39cfa5` |
| Deutsche Alpha 1, 141.960.288 Byte | `8b11f36e7bf4a15932eb364358fba183c2067ac8265cfbc97611ca53e3c90949` |

## Technische Umsetzung

`localization/formats.py` liest Nitro-Dateisystem, NARC-Archive und verschlüsselte Nachrichtenbanken. `text.py` behandelt westliche DS-Zeichen, 9-Bit-Namen in 15-Bit-Wörtern und verlustfreie Steuerzeichen. `merge.py` vergleicht US-Original, deutsches Original und PM anhand gleicher IDs beziehungsweise eindeutig zuordenbarer Inhalte. Neue PM-Einträge bleiben erhalten. Eigene Übersetzungen in `de_pm045.json` sind nur über SHA-256 der PM-Ausgangsnachricht zugeordnet; keine Nintendo-Originaltexte oder ROM-Dumps werden mitgeliefert.

Die Formatfakten wurden anhand der Primärquellen [Nachrichtenleser](https://github.com/pret/pokeplatinum/blob/main/src/message.c), [Zeichentabelle](https://github.com/pret/pokeplatinum/blob/main/charmap.txt) und [String-Implementierung](https://github.com/pret/pokeplatinum/blob/main/src/string_gf.c) geprüft. Kein Code oder Asset daraus wurde übernommen.

Es ändern sich ausschließlich `msgdata/msg.narc`, `msgdata/pl_msg.narc`, `graphic/font.narc` und `graphic/pl_font.narc`. Neue Ressourcen werden angehängt; die zugehörigen FAT-Einträge, ROM-Größe, Kapazität und Header-CRC werden aktualisiert. Ein Bytevergleich bestätigt: alle ursprünglichen Bytes außerhalb dieser FAT-/Header-Felder bleiben identisch. Zusätzlich werden alle FAT-Ressourcen einschließlich unbenannter Overlaydateien überprüft. ARM9, ARM7, Overlays, Skripte und PM-Mailboxlogik bleiben unverändert.

| Kategorie | Einträge |
|---|---:|
| Deutsche Originaltexte bei gleicher ID | 72.586 |
| Eindeutig zugeordnete, verschobene Originaltexte | 2.701 |
| Eigene PM-Übersetzungen einschließlich gleicher Textvorkommen | 1.208 |
| Absichtlich neutrale Einträge, Leertexte, Eigennamen und einzelne japanische Diagnoselabels | 421 |
| Unbenutzter Diamond/Pearl-Abspann unverändert erhalten | 216 |
| Ungeklärte PM-Einträge | 0 |
| Gesamt in beiden Nachrichtenarchiven | 77.132 |

857 Übersetzungsfingerabdrücke und 40 explizit beibehaltene Fingerabdrücke. Ein mehrfach verwendeter Text braucht nur eine Übersetzung. Die Abspannbank hat 216 US- gegenüber 237 deutschen Einträgen mit verschobenen IDs. Sie wird vollständig erhalten, statt falsche Personen-/Leerzeilen zu übernehmen. `msgdata/scenario/scr_msg.narc` ist in allen drei Eingaben bytegleich und hat ein anderes Format; es wird nicht umgeschrieben. Die beiden deutschen Fontarchive werden nur übernommen, weil PM sie gegenüber dem US-Original nicht verändert hat.

## Grenzen und Abnahme

Die Zählung ist ein Nachweis für die bearbeiteten Nachrichtenarchive, keine Behauptung, dass jedes im Spiel sichtbare Pixel deutsch ist. Grafisch eingebettete englische Schrift (im Einführungstest konkret die „YES/NO“-Schaltflächen), außerhalb dieser Archive gespeicherte Texte, Entwickler-Eigennamen, Abspann und sprachabhängige US-Grammatikroutinen können bestehen bleiben. Schriftbreiten, lange Menüs und sämtliche Formatierungsvarianten müssen beim Spielen geprüft werden. Kein Grafik-/Titelbild-Port implementiert.

Neue ROM bedeutet im Launcher einen neuen Spiel-Hash und damit neue Save-Verzeichnisse. Bestehende Spielstände zuerst sichern; ihre Migration und Kompatibilität sind nicht bestätigt. Beide Spieler müssen dieselbe Alpha verwenden. Vor einem Versionswechsel wieder sichern.

Prüfen: Einführung, Pokémon-/Trainer-/Ortsnamen, Attacken/Items, PC-Labor, Pokémon-Zusammenfassung, Platzhalter und mehrseitige Dialoge. Nach Erhalt des Pokédex: Beutel → Basis-Items → Funkspiel → Funkspiel starten. Tatsächliche Teambeitritte, Bewegungen, Kämpfe, Tausch, getrennte Spielstände und Wiederverbindung nach [Abnahmeprotokoll](acceptance-tests.md) testen. Ein TCP-Link allein bestätigt diese Funktionen nicht.

ROM-freie Tests: `python3 -m unittest discover -s tests -p 'test_*.py'` (für xdelta-Tests `xdelta3` im PATH). Der opt-in-Test `tests/verify_user_rom.py` prüft mit der eigenen ROM Start, Frames, getrennte Eingaben, TCP und Prozessende; er schaltet Funkspiel nicht automatisch frei.
