# Pokémon Platinum Local Co-op

Ein Linux-Launcher für zwei **getrennte Project-PM/melonDS-Prozesse**, zwei Controller und ein gemeinsames Split-Screen-Fenster. Zielplattform: Bazzite, KDE Plasma und Steam Gaming Mode, 3840 × 2160 bei 60 Hz.

**Entwicklungsstand:** implementierter Prototyp mit reproduzierbaren ROM-freien Tests. Ein passender Project-PM-Multiplayer-Patch und eine eigene Platin-ROM werden für das gewünschte Spielerlebnis benötigt. Gemeinsames Sinnoh-Gameplay wurde hier nicht geprüft. Ein normales Pokémon Platin erhält durch diesen Launcher keinen freien Welt-Koop.

## Start auf Bazzite

```bash
./scripts/bazzite-build.sh
~/.local/bin/pokemon-platinum-local-coop
```

Das Skript legt einen Fedora-44-Distrobox-Container an, installiert die Abhängigkeiten **im Container**, baut beide Programme und erzeugt einen eigenen Launcher/Desktop-Eintrag. Bestehende Steam-, Gamescope- und NVIDIA-Einstellungen werden nicht verändert.

Ersteinrichtung im Desktopmodus: kompatible gepatchte ROM auswählen, Emulatorpfad kontrollieren, Controller zuordnen. Den Desktop-Eintrag anschließend in Steam als Nicht-Steam-Spiel hinzufügen. **Steam Input für dieses Spiel deaktivieren**, damit die SDL-Eingaben eindeutig bleiben. [Vollständige Anleitung](docs/bazzite-installation.md).

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
