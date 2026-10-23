# Große GBA-ROMs mit CouchSplit

[English](../extended-rom.md) · **Deutsch** · [GBA-Anleitung](firered.md)

CouchSplit XROM v1 unterstützt eine einzelne Datei bis 512 MiB bei gleicher Launcher-Oberfläche. Dies ist eine Emulator-Erweiterung für eigens gebaute Reise-ROMs. Normale GBA-Module bleiben auf 32 MiB begrenzt. Das Aneinanderhängen kommerzieller ROMs vereint ihre Engines und Handlungen nicht.

Die ersten 32 MiB behalten die normale GBA-Abbildung mit Code, Spiegelbereichen, GPIO und Speicherbussen. Dateidaten ab `0x02000000` erscheinen nur lesbar ab Gastadresse `0x10000000`: bis 480 MiB, Ende vor `0x2E000000`. Ausführbarer Code bleibt im nativen Bereich. CPU-Byte-/Halbwort-/Wort- und LDM-Zugriffe, DMA-Quellen und BIOS-Dekomprimierung lesen dieselben Zusatzdaten. Nicht ausgerichtete Zugriffe behalten die native Rotation; hinter dem tatsächlichen Datenende wird FF gelesen. Schreiben verändert die Datei nicht. Der Host hält die schreibgeschützte Abbildung bis zum Ende des Kerns, je Worker getrennt. Veränderliche Bankzustände werden nicht gespeichert.

Das normale Headerbyte und die Prüfsumme müssen stimmen; reservierte Bytes `0xB5..0xBB` enthalten `CSXROM1`. Die Datei ist größer als 32 MiB, maximal 512 MiB und auf vier Byte ausgerichtet. Rohdatei und ZIP mit einer ROM verwenden dieselben Grenzen. Worker ohne `GBA_XROM=1` werden vor dem Sperren des Profils abgewiesen. Speicherchipgrößen und Trennung nach Spieler und vollständigem ROM-SHA-256 bleiben erhalten. Keine automatische Übernahme fremder Builds.

mGBA 0.10.5 bleibt festgelegt. `scripts/prepare-mgba-xrom.py` prüft die fünf betroffenen Ursprungsdateien, erstellt eine eigene Quellkopie und prüft alle geänderten Ergebnisse. CMake verwendet den Patch-Hash für den Ausgabeordner; Upgrades überschreiben keine früheren generierten Quellen. Quellpakete enthalten Ursprungsquelle, Patch, Vorbereitungstool und Lizenzen für einen passenden Offline-Neubau.

Ein Linker kann Daten bei VMA `0x10000000`, LMA `0x0A000000` neben Code ab `0x08000000` ablegen; objcopy erzeugt so echte Zusatzdaten ab Dateioffset 32 MiB. Der native Bereich wird reserviert, anschließend Kennung und Header-Prüfsumme setzen. Die Kennung allein konvertiert kein Spiel: Verlagerte Daten benötigen die richtigen Gastadressen. Beliebige GBA-Emulatoren und echte GBA-Hardware sind nicht automatisch kompatibel.

`python3 tests/smoke_xrom.py build/bin/gba-worker` prüft eigene ARM-Programme ohne Spielassets: CPU, DMA, BIOS, Größenlimit, Eingabe und Speichern. `tests/smoke_gba.py` prüft weiter normale Module. Das ersetzt weder private Spielkompatibilitätsprüfungen noch vollständige Durchläufe. Physische Controller, hörbarer Ton und native Windows-/Bazzite-Ausführung sind für diese Erweiterung ungeprüft.
