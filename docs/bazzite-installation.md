# Installation auf Bazzite

Für die komfortable Installation die [Ein-Datei-Anleitung](one-file-installer.md) verwenden. Sie nutzt fertige Programme mit eigener Ubuntu-24.04-Laufzeit und automatischer ROM-/ZIP-Ersteinrichtung. Die folgenden Abschnitte beschreiben weiterhin den optionalen Entwicklerbuild im Fedora-Container.

## Voraussetzungen

Bazzite mit funktionierenden vorhandenen NVIDIA-Treibern, Distrobox/Podman, zwei SDL-kompatible USB-/Bluetooth-Controller. Eine rechtmäßig bereitgestellte Pokémon-Platin-ROM, die bereits mit dem zur Bridge passenden Project-PM-Patch vorbereitet ist. Das Projekt liefert weder ROM noch Nintendo-Dumps und lädt diese nicht herunter.

Ein normaler Platin-Spielstand und ein normaler ROM-Header beweisen keine Multiplayer-Kompatibilität. Die passende Patchquelle und Patchversion müssen beim Project-PM-Projekt geklärt werden.

## Bauen und starten

Quellprojekt an einem dauerhaften Ort im eigenen Benutzerverzeichnis entpacken bzw. klonen; danach:

```bash
cd pokemon-platinum-local-coop
./scripts/bazzite-build.sh
~/.local/bin/pokemon-platinum-local-coop
```

Der Container heißt `platinum-coop-build`, Basis Fedora 44. Das Skript installiert die Entwicklungspakete ausschließlich über `sudo dnf` **innerhalb** des Containers. Auf dem Bazzite-Host werden nur Dateien in `~/.local/bin` und `$XDG_DATA_HOME/applications` angelegt. Kein rpm-ostree, keine Root-Dienste und keine Änderungen an NVIDIA, Gamescope oder bestehenden Steamkonfigurationen.

Der Wrapper führt das gebaute Programm in derselben Distrobox aus. Damit wird keine Qt-/SDL-Kompatibilität auf einem unbekannten Host vorausgesetzt. Zum erneuten Bauen `scripts/bazzite-build.sh` wiederholen. Zum Entfernen Desktop-Eintrag/Wrapper sowie optional Container und Projektordner entfernen; Spielstände liegen getrennt in der Datenwurzel.

Ersteinrichtung im KDE-Desktop:

1. Beide Controller verbinden und „Controller zuordnen“ öffnen. Die Anzeige enthält live SDL-Instanz-IDs.
2. Unter „ROM auswählen“ die vorbereitete ROM angeben und den vorhandenen Project-PM-Patch bestätigen.
3. In „Einstellungen“ das gepatchte `build/bin/melonDS-coop` kontrollieren. Das Buildskript legt es automatisch neben den Launcher.
4. Externe BIOS/Firmwarefelder leer lassen für freien BIOS-Direktstart; eigene DS-Dumps optional vollständig auswählen.
5. „Bildschirmaufteilung“: links/rechts, großer Hauptbildschirm mit kleinerem Touchscreen.
6. Starten und Multiplayer im ROM aktivieren, sofern der Hack dies verlangt. TCP-Status und ROM-Aktivierung werden getrennt angezeigt.

## Steam Gaming Mode

Der Installer öffnet automatisch Bazzites Funktion zum Hinzufügen des Launchers zu Steam. Falls erforderlich, in Desktop-Steam anmelden. Unter „Steam-fremd“ nach **Pokémon Platinum Local Co-op** suchen. Fehlt der Eintrag: im Startmenü Rechtsklick auf den Launcher → „Erneut zu Steam hinzufügen“. Alternativ in Desktop-Steam „Spiel hinzufügen → Steam-fremdes Spiel“ und den erzeugten Startmenüeintrag wählen. Startoptionen müssen nicht global geändert werden. Für diesen einen Eintrag **Steam Input deaktivieren**, wenn Steam physische und virtuelle Controller doppelt präsentiert oder Tastatureingaben emuliert.

Danach in den Gaming Mode wechseln und den Eintrag starten. Ein einziges Qt-Hauptfenster enthält beide Spieleransichten. Keine Fremdfensterpositionierung oder zweite Gamescope-Instanz erforderlich. Ausgabeauflösung in den vorhandenen Steam-/TV-Einstellungen auf 3840 × 2160, 60 Hz wählen. Das Programm respektiert die tatsächlich verfügbare Fenstergröße.

Distrobox-GUI/Wayland/XWayland, Bluetooth und Steam-Gaming-Mode wurden in der Entwicklungsumgebung nicht real auf Bazzite geprüft. Falls der Eintrag nicht startet, zunächst über `distrobox enter platinum-coop-build -- /voller/pfad/build/bin/platinum-coop --fullscreen` im Desktop prüfen. Für den Launcher kann bei Bedarf XWayland ausschließlich per Startumgebung getestet werden: `QT_QPA_PLATFORM=xcb ~/.local/bin/pokemon-platinum-local-coop`; die Worker bleiben offscreen.

## Native Entwicklungsdistribution / CI

Ubuntu 24.04:

```bash
sudo apt-get install -y --no-install-recommends g++ cmake ninja-build pkg-config extra-cmake-modules libsdl2-dev qt6-base-dev qt6-base-private-dev qt6-multimedia-dev qt6-svg-dev libarchive-dev libenet-dev libzstd-dev libcurl4-openssl-dev libfaad-dev libpcap-dev libegl-dev libwayland-dev xdelta3
./scripts/build.sh
./build/bin/platinum-coop
```

Fedora-Abhängigkeiten stehen im geprüften Buildskript. `COOP_BUILD_JOBS=4` begrenzt Parallelität; bei knappem Speicher 2 wählen.

## Dateien und Sicherungen

Standardeinstellungen und Spielerdateien liegen unter `$XDG_DATA_HOME/pokemon-platinum-local-coop` oder `~/.local/share/pokemon-platinum-local-coop`. `--data-dir /pfad` erlaubt eine andere Datenwurzel. Jede ROM-SHA-256 bekommt je Spieler ein eigenes Profil. Backups werden vor Sitzungsbeginn angelegt und nicht automatisch gelöscht. Über „Spielstände“ lassen sich Profilordner öffnen oder geprüfte Backups wiederherstellen, wenn beide Emulatoren beendet sind.

Die Anwendung speichert, wenn das Spiel seine Save-Funktion ausführt. Beenden allein erzeugt keinen neuen Ingame-Spielstand. Vor dem Beenden im Spiel speichern, dann Start + Select zwei Sekunden halten.
