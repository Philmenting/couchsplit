# Prüfung des bereitgestellten Project-PM-Pakets 0.4.5

14.10.2026. Das vom Nutzer bereitgestellte `Build V0.4.5.zip` enthält einen echten ROM-Binärpatch. **Der Patch ist für Pokémon Platinum USA Revision 1 vorgesehen und auf der bereitgestellten deutschen Original-ROM nicht direkt anwendbar.** Keine gepatchte deutsche ROM oder gemeinsame Sinnoh-Welt wurde erfolgreich erzeugt/getestet.

## Geprüfte Dateien

- `PlatinumMultiplayerV0.4.5.xdelta`: 2519982 Byte, SHA-256 `70c0808f2400e3fb2cbaa0624364b0ac1ae9ace1177043cae8a74a5b616349a0`.
- `SETUP_GUIDE.md`: verlangt in Abschnitt 1 ausdrücklich eine saubere **Pokémon Platinum Rev 1 USA**. SHA-256 der gelesenen Datei: `81d3ecebeeddf88af6748661988b7b3b091c7742905d68484045d146e385498c`.
- Changelog 0.4.5, Datenschutzbeschreibung und Drittlizenzen gelesen. Das übrige Paket enthält ein gebündeltes Windows-Setup mit Python/Tcl/Tk-Laufzeit und Assets. Kein ROM-Mod-Quellcode und kein deutscher Patch gefunden.
- Windows-Programme wurden nicht ausgeführt. Nur Patch und Textdokumente wurden außerhalb dieses Repositories für die Prüfung entpackt. Paket, Patchbytes, Nintendo-ROM und Spielstände werden nicht ins Repository übertragen.

Der passende [No-Intro-Eintrag](https://github.com/libretro/libretro-database/blob/master/metadat/no-intro/Nintendo%20-%20Nintendo%20DS.dat) für USA Revision 1 hat `CPUE`, 134217728 Byte, SHA-1 `0862ec35b24de5c7e2dcb88c9eea0873110d755c`, CRC32 `69D628E8`. Diese Basis liegt hier nicht vor. Die deutsche Datei hat `CPUD`, Revision 0 und SHA-1 `d40f19ac24d67c721a4ab25e1c013154443d75b4`.

## Tatsächlicher Patchversuch

Echter xdelta3 3.0.11, reguläre Decodierung mit unveränderten Prüfungen:

```bash
xdelta3 -d -s /pfad/zur/eigenen-deutschen.nds \
  /pfad/PlatinumMultiplayerV0.4.5.xdelta /privater-testordner/versuch.nds
```

Ergebnis: Exitcode 1, `target window checksum mismatch: XD3_INVALID_INPUT`. Kein Datenblock erfolgreich veröffentlicht, Ausgabedatei 0 Byte, anschließend entfernt. Die SHA-256-Prüfung der deutschen Original-ROM blieb `9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217`. Das war ein realer Kompatibilitätstest mit den bereitgestellten Dateien, keine Simulation.

`xdelta3 printhdr` bestätigt VCDIFF v0, LZMA-Sekundärkompression, kopierende Quelldatenfenster und Adler32-Zielfensterprüfungen. Bereits Fenster 0 referenziert Quelldaten aus einem Bereich von 66763370 Byte. Dieser Patch ist ein Delta gegen seine konkreten Basisbytes; er ersetzt nicht einfach die ROM durch ein sprachunabhängiges Multiplayerprogramm.

Die Anleitung nennt für manuelles Patchen deaktivierte xdelta-Prüfsummen. Diese Empfehlung gehört zu ihrer ausdrücklich verlangten US-Basis. Sie macht die deutsche ROM nicht kompatibel. In dieser Prüfung wurden die Checks nicht abgeschaltet und kein fehlerhaft rekonstruiertes Ergebnis als Port ausgegeben. Ob auch die korrekte US-Basis besondere Decoderoptionen benötigt, wurde mangels dieser Datei nicht getestet.

## Code ergänzt

`config/project-pm-patches.json` ordnet genau diesen Patch-Fingerabdruck den dokumentierten Basisanforderungen zu. `scripts/prepare-rom.py --inspect` kann ein Build-ZIP ohne vollständiges Entpacken und ohne Ausführen seines Installers prüfen. Es liest nur den einzelnen xdelta-Eintrag und berichtet Basiskennung, Revision und Prüfsumme. Ein bekanntes Paket mit falscher Basis endet mit Exitcode 1; unbekannte Patches werden ausdrücklich als unbestätigt gemeldet (Exitcode 2). Keine Datei wird verändert.

```bash
python3 scripts/prepare-rom.py --inspect \
  --rom /pfad/zur/eigenen-deutschen.nds --patch '/pfad/Build V0.4.5.zip'
```

Dieser Aufruf wurde mit den echten bereitgestellten Dateien ausgeführt: Patch 0.4.5 erkannt, deutsche Basis erkannt, Kompatibilität abgelehnt. Die Patchanwendung prüft diese bekannte Basisanforderung zusätzlich vor der Decodierung. Die bisherige Patchanwendung bleibt auf geprüfte CPUD-Ein-/Ausgaben ausgerichtet; ein automatischer US-zu-Deutsch-Port wird nicht behauptet.

## Multiplayer-Anforderungen aus der Paket-Anleitung

Der ROM-Mod lässt Multiplayer erst nach Erhalt des Pokédex zu. Dann im Beutel unter Key Items **Wireless Play → Start Wireless** auswählen. **CO-OP** für gemeinsame Trainerkämpfe ist standardmäßig ausgeschaltet und muss im Optionsmenü aktiviert werden. Beide Spieler brauchen denselben Build; randomisierte Koop-Einstellungen müssen übereinstimmen. Für zwei Fenster auf demselben PC nennt die Anleitung zwei getrennte Emulatorverzeichnisse und `127.0.0.1`. Diese Aussagen stammen aus der bereitgestellten Anleitung; sie sind noch kein eigener Gameplaynachweis.

Der Launcher zeigt den Pokédex-/Wireless-Play-Schritt jetzt im Verbindungsstatus. Eine TCP-Verbindung vor dieser Aktivierung ist kein Beleg gemeinsamer Spielwelt-Synchronisation.

## Fortsetzung und konkrete Blocker

Für einen belastbaren deutschen Port fehlt weiterhin der ROM-Mod-Quellcode oder ein vom Autor tatsächlich für CPUD erstellter Patch. Ein Binärdelta für CPUE reicht zur geprüften Anpassung aller Spielcode-Hooks und deutschen Textressourcen nicht aus. Es wurde kein vollständiger deutscher Port implementiert.

Für einen zunächst englischen Gameplay-/Bridge-Test wäre eine eigene, rechtmäßig bereitgestellte USA-Rev-1-Basis mit dem oben genannten SHA-1 erforderlich. Die deutsche Datei wird dabei nicht ersetzt. Anschließend kann ein deutscher Ressourcen-/Code-Port entwickelt und getrennt auf deutsche Texte, zusätzlichen Multiplayertext, Speichern/Laden und gemeinsame Weltinteraktionen geprüft werden. Eine US-Basis allein ist noch keine deutsche Version.
