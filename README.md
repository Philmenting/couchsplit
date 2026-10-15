# Pokémon Platinum Local Co-op

Ein Linux-Launcher für zwei **getrennte Project-PM/melonDS-Prozesse**, zwei Controller und ein gemeinsames Split-Screen-Fenster. Zielplattform: Bazzite, KDE Plasma und Steam Gaming Mode, 3840 × 2160 bei 60 Hz.

**Entwicklungsstand:** implementierter Prototyp mit reproduzierbaren ROM-freien Tests. Ein passender Project-PM-Multiplayer-Patch und eine eigene Platin-ROM werden für das gewünschte Spielerlebnis benötigt. Gemeinsames Sinnoh-Gameplay wurde hier nicht geprüft. Ein normales Pokémon Platin erhält durch diesen Launcher keinen freien Welt-Koop.

Eine bereitgestellte deutsche Original-ROM wurde inzwischen in zwei echten Emulatorprozessen gestartet und geprüft: [ROM-Startprüfung](docs/rom-boot-check.md). Sie ist ungepatcht; ihre Multiplayer-Bridge bleibt inaktiv. Die App erklärt bei diesem bekannten Original-Fingerabdruck ausdrücklich den fehlenden Patch.

[Deutsche Lokalisierung](docs/german-localization.md): Eine **US-basierte deutsche PM-0.4.5-Alpha** kann jetzt reproduzierbar aus den drei eigenen ROM-Dateien erzeugt werden. Deutsche Originaltexte, zusätzliche PM-Übersetzungen und ab Alpha 2 die originalen JA/NEIN-Touchgrafiken werden übernommen, die PM-Programmbytes bleiben erhalten. Die deutsche Einführung wurde im Emulator visuell geprüft. Ein vollständiger deutscher Spieldurchlauf und die gemeinsame Sinnoh-Welt sind offen. Das ist kein nativer CPUD-Port.

Die bereitgestellte **US-Rev-1-ROM wurde erfolgreich mit PM 0.4.5 gepatcht** und in zwei echten Emulatoren gestartet. Für diesen bekannten Build genügt `python3 scripts/prepare-rom.py --rom ORIGINAL.nds --patch BUILD.zip --output GEPATCHT.nds`; Prüfsummen werden automatisch geprüft. Dieses erste Ergebnis ist englisch. Anschließend erzeugt `scripts/localize-pm.py` daraus zusammen mit beiden Original-ROMs die deutsche Alpha. [Basisanforderungen und Paketprüfung](docs/project-pm-045.md).

## Veröffentlichungen und Plattformen

Die erste Bazzite-Ausgabe ist als [v0.1.0-alpha.1](https://github.com/Philmenting/pokemon-platinum-local-coop/releases/tag/v0.1.0-alpha.1) verfügbar. Native ZorinOS-18-/Windows-Pakete und ihr tatsächlicher Prüfstand: [Plattformanleitung](docs/platforms.md). Alle Ausgaben bleiben Alpha; keine ROM enthalten.

## Start auf Bazzite

**Einfachster Weg:** `Pokemon-Platinum-Coop-Bazzite.run` im Desktop herunterladen, in den Dateieigenschaften als ausführbar markieren und doppelklicken. Der Installer richtet eine eigene Laufzeit ein, legt den Startmenüeintrag an, fordert automatisch die Aufnahme in Steam an und öffnet danach die ROM-Auswahl. Die vorbereitete ROM oder ihr ZIP hinzufügen. Beide fertigen Programme sind enthalten; kein Kompilieren und kein GitHub-Login. Internet und 3 GiB freier Speicher erforderlich. [Anleitung und tatsächlicher Prüfstatus](docs/one-file-installer.md).

Für Entwickler bleibt der Quellbuild verfügbar:

```bash
./scripts/bazzite-build.sh
~/.local/bin/pokemon-platinum-local-coop
```

Das Skript legt einen Fedora-44-Distrobox-Container an, installiert die Abhängigkeiten **im Container**, baut beide Programme und erzeugt einen eigenen Launcher/Desktop-Eintrag. Bestehende Steam-, Gamescope- und NVIDIA-Einstellungen werden nicht verändert.

Ersteinrichtung im Desktopmodus: kompatible gepatchte ROM auswählen, Emulatorpfad kontrollieren, Controller zuordnen. Die Installation fordert über Bazzites Funktion automatisch einen Nicht-Steam-Eintrag an. Falls dieser fehlt: Startmenü-Rechtsklick → „Erneut zu Steam hinzufügen“. **Steam Input für dieses Spiel deaktivieren**, damit die SDL-Eingaben eindeutig bleiben. [Vollständige Anleitung](docs/bazzite-installation.md).

## Funktionen im Code

- Zwei überwachte Emulatorprozesse mit getrennten Konfigurationen und Spielständen.
- Automatische lokale TCP-Host-/Client-Bridge; kein öffentlicher Relayserver, keine Portweiterleitung.
- Ein Qt-Fenster für beide Spieler; links/rechts oder oben/unten, unverändertes DS-Seitenverhältnis 4:3.
- Großer oberer Bildschirm mit kleinem Touchscreen, DS-Bildschirme nebeneinander oder umschaltbare Einzelansicht.
- Zentrale SDL-Controller-Zuordnung nach Instanz-ID, sichere Trennung, remappbare DS-Tasten, Stick-Touch.
- Vor jedem Start Save-Backup, Prüfsummen und Wiederherstellung; Sperre gegen parallele Sitzungen.
- TV-Oberfläche, Einstellungen, Spielstandverwaltung, Logs und gemeinsames Beenden.

Steuerung: linker Stick/D-Pad bewegen, rechter Stick Touch-Cursor, rechter Stick-Klick berühren. **Select + R** schaltet die Einzelansicht um; **Start + Select für 2 Sekunden** beendet beide Emulatoren. Escape öffnet das Launchermenü; dort „Koop-Spiel starten“ kehrt zur laufenden Sitzung zurück. Im Menü werden Spieleingaben neutral gesetzt.

## Dateien und Build

ROMs, Nintendo-BIOS und proprietäre Firmware sind nicht enthalten. Der Direktstart nutzt standardmäßig die freie melonDS-BIOS-Implementierung und generierte Firmware. Eigene vollständige DS-Dumps sind optional konfigurierbar; bei Kompatibilitätsproblemen separat testen.

Auf einer Entwicklungsdistribution: [Abhängigkeiten installieren](docs/bazzite-installation.md), danach `scripts/build.sh`. Ausgabe: `build/bin/platinum-coop` und `build/bin/melonDS-coop`. Eine generische, verifizierte AppImage ist noch nicht verfügbar; die Distrobox-Ausführung hält Laufzeitbibliotheken konsistent.

Ein Übergabepaket kann außerdem die hier kompilierten Linux-Binaries und den dazugehörigen GPL-Quellcode enthalten. Diese Binaries sind keine verifizierte portable Bazzite-Version; auf Bazzite den Distrobox-Build verwenden.

[Architektur](docs/architecture.md) · [Machbarkeit und Upstream-Prüfung](docs/technical-feasibility.md) · [Teststatus](docs/development-status.md) · [Fehlerbehebung](docs/troubleshooting.md) · [Manuelle Abnahme](docs/acceptance-tests.md)

## Herkunft und Lizenz

GPL-3.0-or-later. melonDS und der Project-PM-Bridgecode bleiben Werke ihrer jeweiligen Autoren. Dieses Projekt liefert eigene Launcherkomponenten und einen Patch für den fixierten Fork:

- [Project PM / platinum-mp](https://github.com/ComicartOlie/melonDS-Project-PM/tree/platinum-mp): Emulationskern, ROM-Mailbox-Bridge, SaveManager.
- [melonDS Upstream](https://github.com/melonDS-emu/melonDS): ursprünglicher Emulator und Qt/SDL-Frontend.
- [mGBA SplitScreen](https://github.com/Spuds0588/mgba-splitscreen) und [CouchPlay](https://github.com/hikaps/couchplay): untersuchte Architekturvorbilder; kein Code daraus übernommen.

Dieses inoffizielle Projekt ist nicht mit Nintendo, Game Freak oder The Pokémon Company verbunden.
