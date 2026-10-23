# GBA-Spiele unterstützen

[English](../firered.md) · **Deutsch** · [Dokumentation](README.md)

Der lokale Entwicklungsbuild kann GBA-Spiele und ROM-Hacks in der bestehenden CouchSplit-Oberfläche starten. Eine `.gba`-Datei oder ZIP mit genau einem Spiel auswählen. Geprüft werden das feste GBA-Headerbyte, die Header-Prüfsumme und eine Größe zwischen 512 Byte und 32 MiB bei normalen Modulen. Ausdrücklich markierte CouchSplit-XROM-v1-Dateien dürfen bis zu 512 MiB groß sein. Ein gültiger Header erkennt das Format; er bestätigt nicht die Funktion jedes Spiels oder Hacks. Zu große oder beschädigte Dateien werden abgewiesen. Project PM / Platin und der DS-Worker bleiben erhalten; andere DS-Spiele werden durch diese Änderung nicht freigeschaltet.

Nach dem Zuordnen zweier Controller **Splitscreen starten** wählen. Jeder Spieler erhält einen Bildschirm mit 240×160 Pixeln, eigene Eingaben/Audio und einen eigenen Spielstand. Die Menüaufteilung bleibt erhalten, Texte und Bedienhinweise passen sich dem Spiel an. Es gibt kein zusätzliches Reisemenü und kein zusätzliches Emulatorfeld in den Einstellungen. Gemeinsame Spielwelt, GBA-Linktausch/-kämpfe, gemeinsame Pokémon-Bank und Regionsreisen werden durch diesen Worker nicht umgesetzt.

Im Spiel speichern und das Ende der Speicheranzeige abwarten. Start + Select für zwei Sekunden beendet die Sitzung. Der Worker sichert geänderte Speicherdaten alle 60 Frames und beim geordneten Beenden atomar. Unterstützt sind EEPROM mit 512 Byte / 8 KiB, SRAM mit 32 KiB und Flash/SRAM mit 64 KiB / Flash mit 128 KiB. Spiele ohne Batteriespeicher erzeugen keinen leeren Spielstand. Ungültige Größen und erkannte Größenkonflikte werden ohne Überschreiben des vorhandenen Stands abgewiesen. Die Dateisicherung ersetzt nicht das Speichern im Spiel. Spielstände und geprüfte Backups bleiben nach Spieler und exaktem ROM-SHA-256 getrennt. Andere Builds oder Sprachen übernehmen keine Spielstände automatisch.

Der Launcher wählt den mitgelieferten `gba-worker` automatisch. Das vorhandene DS-Emulatorfeld und die BIOS-Einstellungen bleiben unverändert und betreffen GBA nicht. mGBA nutzt seine Software-BIOS-Implementierung; ein externes GBA-BIOS ist nicht erforderlich. Select + R wird als normale GBA-Eingabe weitergegeben, weil GBA keinen zweiten Bildschirm hat. Controller-Zuordnung, Trennungsverhalten und Beenden-Tastenkombination bleiben erhalten.

Build: `scripts/build.sh`; mGBA 0.10.5 ist in `config/mgba.lock.json` festgelegt. Quellpakete enthalten die festgelegte mGBA-Quelle und MPL-2.0-Lizenzen unter `gba-corresponding-source` sowie die nachvollziehbare Erweiterung `patches/mgba-xrom.patch`. CMake wendet diese auf eine separate Buildkopie an; die festgelegte Originalquelle bleibt unverändert. Offline ist `-DCOUCHSPLIT_MGBA_SOURCE=/pfad/zu/gba-corresponding-source` möglich. IPC-ABI bleibt 2. Die veröffentlichten Alpha-8-Pakete und die frühere reine Feuerrot-Vorschau enthalten diese allgemeine Erweiterung nicht. Native Windows-/Bazzite-Ausführung dieser Änderung ist noch nicht geprüft.

Produktions-Worker ohne kommerzielle Spielinhalte prüfen:

```sh
python3 tests/smoke_gba.py build/bin/gba-worker
```

Das private Werkzeug `build/launcher/verify-gba` startet eine separat bereitgestellte GBA-ROM mit echtem Session/PlayView und virtuellen SDL-Controllern. Eigene ARM-Testprogramme prüfen Speicherchips und Wiederladen; echtes deutsches Feuerrot wird getrennt geprüft. Dies belegt weder physische Controller noch einen vollständigen Durchlauf jedes Spiels.

## Große Reise-ROMs

[CouchSplit XROM v1](extended-rom.md) erhält eine einzige `.gba`-Datei und dieselbe Launcher-Oberfläche. Unveränderliche Daten hinter den nativen 32 MiB sind über einen zusätzlichen Emulator-Adressbereich erreichbar. Das eigene Format benötigt den passenden Worker; beliebige GBA-Emulatoren oder echte Module unterstützen es nicht automatisch. Die sieben reservierten Headerbytes ab 0xB5 enthalten `CSXROM1`, die Header-Prüfsumme stimmt, die Datei ist auf vier Byte ausgerichtet und größer als 32 MiB. Unbekannte übergroße ROMs werden weiter abgewiesen. Ein größerer Container ergänzt keine fehlenden Handlungskapitel.
