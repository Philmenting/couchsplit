# Prüfung des bereitgestellten Project-PM-Pakets 0.4.5

14.10.2026. Das vom Nutzer bereitgestellte `Build V0.4.5.zip` enthält einen echten ROM-Binärpatch. **Der Patch ist für Pokémon Platinum USA Revision 1 vorgesehen und auf der bereitgestellten deutschen Original-ROM nicht direkt anwendbar.** Keine gepatchte deutsche ROM oder gemeinsame Sinnoh-Welt wurde erfolgreich erzeugt/getestet.

## Update: US-Basis bereitgestellt und erfolgreich gepatcht

Die passende eigene USA-Rev-1-ROM wurde anschließend vom Nutzer bereitgestellt. SHA-1 stimmt exakt mit der verlangten Basis überein. SHA-256 der Originaldatei: `fbce4c4def0c7797f8dd238a3d7a5e48b4a7e3abd86890ac65f6321cef781bdb`.

Reguläres `xdelta3 -d -s ORIGINAL.nds PATCH.xdelta ERGEBNIS.nds` gelang mit Exitcode 0 und **allen Zielfensterprüfsummen aktiv**. Ein Abschalten war bei diesen Dateien nicht erforderlich. Ergebnis: 134217728 Byte, CPUE Revision 1, SHA-256 `8ef9d4eeee81513f2d6e13c36125cdbfa4c6f498b039ef4b99faa7095d39cfa5`. Originaldatei unverändert. Das neue Patchskript reproduzierte dieses Ergebnis zusätzlich direkt aus dem Build-ZIP mit denselben drei SHA-256-Fingerabdrücken.

Zwei echte gepatchte Emulatorprozesse liefen 35 Sekunden mit dieser ROM: je 2075 Frames, nach Sekunde 5 nochmals 1797/1796 Frames, TCP-Rollen 1/2 verbunden, A-Eingaben nur an Spieler 1, unterschiedliche Intro-/Einführungsbilder, beide SIGTERM-Exitcodes 0. Die Bilder wurden privat visuell geprüft. Die ROM-Wireless-Bridge blieb vor Erhalt des Pokédex inaktiv. **Kein gemeinsames Sinnoh-Gameplay, Ingame-Speichern oder deutscher Port damit verifiziert.**

Das englische Ergebnis wurde dem Nutzer als eigenes ZIP mit Anleitung und Prüfsumme bereitgestellt. ROM, ZIP, Patchbytes und Spielbilder bleiben außerhalb von GitHub. Die folgenden Abschnitte dokumentieren die zuvor durchgeführte deutsche Fehlversuch-Prüfung.

## Auf Bazzite selbst reproduzieren

Die Original-US-ROM zuerst aus ihrem ZIP entpacken. Das Build-ZIP kann direkt übergeben werden; keine Windows-Programme erforderlich. Der bekannte Releasekatalog enthält die verifizierten Basis-, Patch- und Ergebnisprüfsummen. Eine deutsche oder andere falsche Basis wird weiterhin abgelehnt.

```bash
distrobox enter platinum-coop-build -- python3 scripts/prepare-rom.py \
  --rom /pfad/zur/Pokemon-Platinum-USA-Rev1.nds \
  --patch '/pfad/Build V0.4.5.zip' \
  --output /pfad/ausserhalb-des-repos/Pokemon-Platinum-PM-0.4.5-USA.nds
```

Die Ausgabe ist **englisch**. Eine vorhandene Datei wird nicht überschrieben. Unbekannte deutsche Patches benötigen weiterhin unabhängig verifizierte Quelle-/Patch-/Ergebnisprüfsummen; bekannte Releasewerte können nicht durch abweichende Argumente überschrieben werden.

## Geprüfte Dateien

- `PlatinumMultiplayerV0.4.5.xdelta`: 2519982 Byte, SHA-256 `70c0808f2400e3fb2cbaa0624364b0ac1ae9ace1177043cae8a74a5b616349a0`.
- `SETUP_GUIDE.md`: verlangt in Abschnitt 1 ausdrücklich eine saubere **Pokémon Platinum Rev 1 USA**. SHA-256 der gelesenen Datei: `81d3ecebeeddf88af6748661988b7b3b091c7742905d68484045d146e385498c`.
- Changelog 0.4.5, Datenschutzbeschreibung und Drittlizenzen gelesen. Das übrige Paket enthält ein gebündeltes Windows-Setup mit Python/Tcl/Tk-Laufzeit und Assets. Kein ROM-Mod-Quellcode und kein deutscher Patch gefunden.
- Windows-Programme wurden nicht ausgeführt. Nur Patch und Textdokumente wurden außerhalb dieses Repositories für die Prüfung entpackt. Paket, Patchbytes, Nintendo-ROM und Spielstände werden nicht ins Repository übertragen.

Der passende [No-Intro-Eintrag](https://github.com/libretro/libretro-database/blob/master/metadat/no-intro/Nintendo%20-%20Nintendo%20DS.dat) für USA Revision 1 hat `CPUE`, 134217728 Byte, SHA-1 `0862ec35b24de5c7e2dcb88c9eea0873110d755c`, CRC32 `69D628E8`. Diese Basis lag bei der ersten Prüfung noch nicht vor. Die deutsche Datei hat `CPUD`, Revision 0 und SHA-1 `d40f19ac24d67c721a4ab25e1c013154443d75b4`.

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

Dieser Aufruf wurde mit den echten bereitgestellten Dateien ausgeführt: Patch 0.4.5 erkannt, deutsche Basis erkannt, Kompatibilität abgelehnt. Die Patchanwendung prüft diese bekannte Basisanforderung zusätzlich vor der Decodierung. Sie unterstützt inzwischen den verifizierten US-Build 0.4.5; unbekannte Patches bleiben auf geprüfte CPUD-Ein-/Ausgaben ausgerichtet. Ein automatischer US-zu-Deutsch-Port wird nicht behauptet.

## Multiplayer-Anforderungen aus der Paket-Anleitung

Der ROM-Mod lässt Multiplayer erst nach Erhalt des Pokédex zu. Dann im Beutel unter Key Items **Wireless Play → Start Wireless** auswählen. **CO-OP** für gemeinsame Trainerkämpfe ist standardmäßig ausgeschaltet und muss im Optionsmenü aktiviert werden. Beide Spieler brauchen denselben Build; randomisierte Koop-Einstellungen müssen übereinstimmen. Für zwei Fenster auf demselben PC nennt die Anleitung zwei getrennte Emulatorverzeichnisse und `127.0.0.1`. Diese Aussagen stammen aus der bereitgestellten Anleitung; sie sind noch kein eigener Gameplaynachweis.

Der Launcher zeigt den Pokédex-/Wireless-Play-Schritt jetzt im Verbindungsstatus. Eine TCP-Verbindung vor dieser Aktivierung ist kein Beleg gemeinsamer Spielwelt-Synchronisation.

## Fortsetzung und konkrete Blocker

Für einen belastbaren deutschen Port fehlt weiterhin der ROM-Mod-Quellcode oder ein vom Autor tatsächlich für CPUD erstellter Patch. Ein Binärdelta für CPUE reicht zur geprüften Anpassung aller Spielcode-Hooks und deutschen Textressourcen nicht aus. Es wurde kein vollständiger deutscher Port implementiert.

Die passende USA-Rev-1-Basis wurde inzwischen bereitgestellt, gepatcht und auf Intro-/TCP-/Prozessfunktion getestet. Als Nächstes muss die gemeinsame Spielwelt nach dem Pokédex tatsächlich nachgewiesen werden. Danach ist eine deutsche Lokalisierung auf US-PM-Basis zu untersuchen; ein vollständiger nativer CPUD-Port bleibt ein anderes, noch nicht umgesetztes Vorhaben. Die deutsche Originaldatei wird dabei nicht ersetzt.
