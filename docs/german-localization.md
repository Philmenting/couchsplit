# Deutsche PM-0.4.5-Alpha

Stand 15.10.2026, **Alpha 2**. **Implementiert:** deutsche Textlokalisierung der verifizierten US-PM-ROM und originale deutsche JA/NEIN-Grafiken einschließlich aller acht Animationszustände. **Automatisiert getestet:** zwei identische Builds, Archiv-/Textprüfungen, unveränderte PM-Programmbytes, ROM-Start in zwei echten Emulatorprozessen. **Visuell geprüft:** deutsche Begrüßung mit Umlaut sowie JA/NEIN im Emulatorbild; Antippen von JA führt zur nächsten Einführungsszene. **Nicht verifiziert:** kompletter Spieldurchlauf, sämtliche Menüs, Ingame-Speichern und gemeinsame Sinnoh-Welt; keine manuelle Bestätigung auf Bazzite/TV/Controllerhardware.

## Erzeugen

Python 3.11 oder neuer, keine zusätzlichen Python-Pakete. Die drei ROMs müssen selbst rechtmäßig bereitgestellt werden. ZIPs vorher entpacken. Die PM-ROM zuerst mit `scripts/prepare-rom.py` aus USA Rev 1 und dem echten PM-0.4.5-Patch erzeugen. Dabei keine xdelta-Prüfung abschalten.

```bash
python3 scripts/localize-pm.py \
  --us /pfad/original-usa-rev1.nds \
  --german /pfad/original-platin-deutsch.nds \
  --pm /pfad/platinum-pm-045-us.nds \
  --output /pfad/ausserhalb-des-repos/platinum-pm-045-de-alpha2.nds \
  > /pfad/ausserhalb-des-repos/localization-report.json
```

Das Skript verlangt die exakt geprüften SHA-256-Fingerabdrücke, liest unveränderliche Bytes, verweigert vorhandene Ziele und Ausgaben im Repository. Jede nicht aufgelöste PM-Nachricht oder veränderte Steuersequenz bricht den Build ab. Keine Originaldatei wird verändert. Im Launcher die neue `.nds` auswählen. Sie behält absichtlich `CPUE` und den US-Spielcode; das ist eine deutsche Textfassung des US-Mods, kein Port der deutschen Engine.

| Eingabe/Ergebnis | SHA-256 |
|---|---|
| USA Rev 1, CPUE | `fbce4c4def0c7797f8dd238a3d7a5e48b4a7e3abd86890ac65f6321cef781bdb` |
| Deutsche Originalfassung, CPUD Rev 0 | `9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217` |
| Gepatchte US-PM-0.4.5-ROM | `8ef9d4eeee81513f2d6e13c36125cdbfa4c6f498b039ef4b99faa7095d39cfa5` |
| Deutsche Alpha 1 (vorherige Fassung), 141.960.288 Byte | `8b11f36e7bf4a15932eb364358fba183c2067ac8265cfbc97611ca53e3c90949` |
| Deutsche Alpha 2, 141.962.768 Byte | `800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c` |

## Technische Umsetzung

`localization/formats.py` liest Nitro-Dateisystem, NARC-Archive und verschlüsselte Nachrichtenbanken. `text.py` behandelt westliche DS-Zeichen, 9-Bit-Namen in 15-Bit-Wörtern und verlustfreie Steuerzeichen. `merge.py` vergleicht US-Original, deutsches Original und PM anhand gleicher IDs beziehungsweise eindeutig zuordenbarer Inhalte. Neue PM-Einträge bleiben erhalten. Eigene Übersetzungen in `de_pm045.json` sind nur über SHA-256 der PM-Ausgangsnachricht zugeordnet; keine Nintendo-Originaltexte oder ROM-Dumps werden mitgeliefert.

Die Formatfakten wurden anhand der Primärquellen [Nachrichtenleser](https://github.com/pret/pokeplatinum/blob/main/src/message.c), [Zeichentabelle](https://github.com/pret/pokeplatinum/blob/main/charmap.txt) und [String-Implementierung](https://github.com/pret/pokeplatinum/blob/main/src/string_gf.c) geprüft. Kein Code oder Asset daraus wurde übernommen.

Es ändern sich ausschließlich `msgdata/msg.narc`, `msgdata/pl_msg.narc`, `graphic/font.narc`, `graphic/pl_font.narc` und `graphic/touch_subwindow.narc`. Neue Ressourcen werden angehängt; die zugehörigen FAT-Einträge, ROM-Größe, Kapazität und Header-CRC werden aktualisiert. Ein Bytevergleich bestätigt: alle ursprünglichen Bytes außerhalb dieser FAT-/Header-Felder bleiben identisch. Zusätzlich werden alle FAT-Ressourcen einschließlich unbenannter Overlaydateien überprüft. ARM9, ARM7, Overlays, Skripte und PM-Mailboxlogik bleiben unverändert.

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

## Originale JA/NEIN-Grafik ab Alpha 2

Die deutsche Edition speichert das entsprechende Archiv unter `resource/ger/touch_subwindow/touch_subwindow.narc`, während die US-Engine `graphic/touch_subwindow.narc` lädt. Der Dateiname allein reicht daher nicht für einen Sprachvergleich. `localization/graphics.py` importiert die originalen deutschen Archivbytes unter dem US-Pfad. Es liefert keine Nintendo-Grafikbytes mit.

Geprüft: PM-Archiv ist bytegleich zum US-Original. Beide Archive enthalten zehn Ressourcen; ausschließlich die komprimierte NCGR-Textur (Mitglied 1) unterscheidet sich. Palette und alle acht Animationskarten sind bytegleich. Die entpackte Textur bleibt 3.776 Byte groß: keine Vergrößerung der VRAM-Belegung, keine Änderung der Schaltflächengeometrie, Touchbereiche oder Programmroutinen. Das neue Skript verweigert PM-Modifikationen und abweichende Palette, Animation oder Speichergröße.

Primärquellen für Zuordnung und Verwendung: [Lader](https://github.com/pret/pokeplatinum/blob/1dc13dcea40c04ab53f1f607de46023dc409ef20/src/yes_no_touch_menu.c) und [Archiv-Reihenfolge](https://github.com/pret/pokeplatinum/blob/1dc13dcea40c04ab53f1f607de46023dc409ef20/res/graphics/yes_no_touch_menu/yes_no_touch_menu.order). Die tatsächlich verwendeten Grafiken stammen ausschließlich aus der eigenen deutschen ROM.

Zwei zusätzliche ROM-freie Tests prüfen Grafikübernahme und Schutz vor inkompatiblen Ressourcen. Die vollständige Python-Suite umfasst jetzt 17 erfolgreiche Tests. Zwei Builds erzeugten denselben Alpha-2-Fingerabdruck. Ein 110-Sekunden-Test mit zwei echten Prozessen ergab 6566/6571 Frames, TCP-Rollen 1/2 und beide Exitcodes 0. Eingaben: A-Impulse nur für Spieler 1, Sekunden 10–65, dann neutral; einmal Touch bei (128, 83) in Sekunde 90. Das Bild vor dem Touch zeigte JA/NEIN; danach erschien Professor Eibe mit dem nächsten Dialog. Dies bestätigt Anzeige und JA-Aktion im Einführungstest, keine gemeinsame Spielwelt. Alpha 1 bleibt als historischer Stand erhalten.

## Grenzen und Abnahme

Die Zählung ist ein Nachweis für die bearbeiteten Nachrichtenarchive, keine Behauptung, dass jedes im Spiel sichtbare Pixel deutsch ist. Die zuvor englischen YES/NO-Touch-Schaltflächen sind ab Alpha 2 deutsch. Weitere grafisch eingebettete Schrift, außerhalb dieser Archive gespeicherte Texte, Entwickler-Eigennamen, Abspann und sprachabhängige US-Grammatikroutinen können bestehen bleiben. Schriftbreiten, lange Menüs und sämtliche Formatierungsvarianten müssen beim Spielen geprüft werden. Weitere Grafikarchive und das Titelbild sind nicht lokalisiert.

Neue ROM bedeutet im Launcher einen neuen Spiel-Hash und damit neue Save-Verzeichnisse. Bestehende Spielstände zuerst sichern; ihre Migration und Kompatibilität sind nicht bestätigt. Beide Spieler müssen dieselbe Alpha verwenden. Vor einem Versionswechsel wieder sichern.

Prüfen: Einführung, Pokémon-/Trainer-/Ortsnamen, Attacken/Items, PC-Labor, Pokémon-Zusammenfassung, Platzhalter und mehrseitige Dialoge. Nach Erhalt des Pokédex: Beutel → Basis-Items → Funkspiel → Funkspiel starten. Tatsächliche Teambeitritte, Bewegungen, Kämpfe, Tausch, getrennte Spielstände und Wiederverbindung nach [Abnahmeprotokoll](acceptance-tests.md) testen. Ein TCP-Link allein bestätigt diese Funktionen nicht.

ROM-freie Tests: `python3 -m unittest discover -s tests -p 'test_*.py'` (für xdelta-Tests `xdelta3` im PATH). Der opt-in-Test `tests/verify_user_rom.py` prüft mit der eigenen ROM Start, Frames, getrennte Eingaben, TCP und Prozessende; er schaltet Funkspiel nicht automatisch frei.
