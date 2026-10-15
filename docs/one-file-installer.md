# Eine Datei für Bazzite

Der Installer `Pokemon-Platinum-Coop-Bazzite.run` enthält beide fertigen Linux-x86_64-Programme und ihren vollständigen zugehörigen GPL-Quellcode. Eine ROM ist nicht enthalten. Ein GitHub-Konto, manuelle Paketinstallation und Kompilieren sind für die Nutzung nicht erforderlich.

## Benutzen

1. Im Bazzite-Desktop herunterladen. In Dolphin: Rechtsklick → Eigenschaften → Berechtigungen → **Als Programm ausführbar**. Browser übernehmen dieses Linux-Dateirecht beim Download üblicherweise nicht.
2. Doppelklicken und die Installation starten. Internet, vorhandene Bazzite-Distrobox/Podman und mindestens 3 GiB freier Speicher werden benötigt. Die Einrichtung kann beim ersten Mal mehrere Minuten dauern.
3. Anschließend öffnet sich die ROM-Auswahl. Die vorbereitete Project-PM-ROM als `.nds` oder das bereitgestellte ROM-ZIP auswählen. Das ZIP wird geprüft und genau eine enthaltene DS-ROM importiert. Die verifizierten PM-0.4.5-US-/Deutsch-Alpha-Fingerabdrücke werden automatisch erkannt. Andere Mods benötigen weiterhin eine ausdrückliche Bestätigung.
4. Beide Controller verbinden und „Koop-Spiel starten“ wählen. Bei zwei Controllern erfolgt die erste Zuordnung automatisch; über „Controller zuordnen“ lässt sie sich ändern.

Die ROM wird mit privaten Dateirechten in den eigenen Datenordner kopiert. Der Download kann danach entfernt werden. Eine normale ungepatchte Platin-ROM wird weiterhin abgelehnt; der Installer beschafft keine Spiele und patcht diese nicht automatisch.

Später im Startmenü **Pokémon Platinum Local Co-op** öffnen. Für Gaming Mode einmal in Desktop-Steam „Spiel hinzufügen → Steam-fremdes Spiel“ und diesen Eintrag auswählen. Bestehende Steamdateien werden vom Installer nicht umgeschrieben. Steam Input für diesen Eintrag deaktivieren, falls doppelte oder emulierte Tastatureingaben auftreten. BIOS/Firmware ist für den voreingestellten freien Direktstart nicht erforderlich.

## Was eingerichtet wird

- Eigene rootless Distrobox `platinum-coop-runtime-v1` mit Ubuntu 24.04, passend zur Buildumgebung. Ubuntu ist nur die Laufzeit im Container; das Bazzite-System bleibt bestehen.
- Laufzeitbibliotheken über signierte Ubuntu-Paketquellen innerhalb dieses Containers. `sudo` kommt nur dort zum Einsatz. Der Installer lehnt rootful Podman und fremd belegte Containernamen ab. Kein rpm-ostree, kein Root-Dienst, keine Treiber- oder NVIDIA-Konfigurationsänderung.
- Programme und Quellcode unter `$XDG_DATA_HOME/pokemon-platinum-local-coop/software/releases/…`, aktive Version über `software/current`.
- Eigener Wrapper `~/.local/bin/pokemon-platinum-local-coop` und eigener Startmenüeintrag unter `$XDG_DATA_HOME/applications/`. Standard für XDG_DATA_HOME: `~/.local/share`.
- Spielstände und Einstellungen bleiben in der bisherigen Datenwurzel. Die Installation löscht sie nicht. ROM-Importe liegen unter `roms`, Installationslogs unter `installer-logs`.

Der Wrapper verwendet die Containerbibliotheken und setzt den Datenordner ausdrücklich. Er entfernt störende Steam-/Host-Bibliothekspfade nur für den gestarteten Prozess. XWayland wird bei vorhandenem DISPLAY verwendet, sonst Qt-Wayland. Softwaredarstellung benötigt keine NVIDIA-Treiberintegration in den Container.

Vor Aktivierung einer Version prüft der Installer Emulator-ABI und Qt-Launcher mit einem temporären Testprofil. Erst bei Erfolg wechselt die aktive Version. Bei Fehlern bleiben bisherige aktive Version und Spielstände erhalten; ein erneuter Start kann die Einrichtung fortsetzen. Frühere Softwareversionen bleiben zum Wiederherstellen erhalten.

Zum Entfernen zuerst beide Spiele schließen. Den eigenen Startmenüeintrag und Wrapper entfernen; den eigenen Runtime-Container bei Bedarf über Distrobox entfernen. Den Datenordner mit Spielständen separat sichern und nur bei ausdrücklichem Wunsch löschen. Der Installer installiert keine automatische Deinstallationsroutine, die versehentlich Saves entfernen könnte.

## Paket prüfen / selbst bauen

```bash
bash Pokemon-Platinum-Coop-Bazzite.run --check
```

Das prüft das vollständige eingebettete Paket und alle Dateifingerabdrücke ohne Installation. Es ist ein Integritätscheck, keine digitale Herausgebersignatur. Archivpfade, Links, Dateigrößen und Dateiliste werden vor Ausführung des Installationsprogramms geprüft. Es werden keine heruntergeladenen Shellskripte ausgeführt.

Für Entwickler nach sauberem Ubuntu-24.04-Build:

```bash
scripts/build.sh
python3 -m unittest discover -s tests -p 'test_*.py'
python3 scripts/make-installer.py --upstream .cache/project-pm --output /tmp/Pokemon-Platinum-Coop-Bazzite.run
bash /tmp/Pokemon-Platinum-Coop-Bazzite.run --check
```

Vor Paketbau Änderungen committen. Der Paketbauer rekonstruiert den gepinnten Upstream samt eigenem Patch und vergleicht ihn mit dem tatsächlich verwendeten Quellbaum. Beide Binärdateien müssen nach dem letzten C++-Änderungsstand neu gebaut sein. `.git`, Zugangsdaten, ROMs, Firmware und Spielstände werden nicht mitgeliefert. Das Paket enthält die eigenen Projektdateien, die gepatchte vollständige Upstreamquelle und die Buildanleitung; keine Laufzeitbibliotheken vom Host.

Primärquellen: [Distrobox auf Bazzite](https://docs.bazzite.gg/Installing_and_Managing_Software/Distrobox/), [Distrobox Create](https://distrobox.it/usage/distrobox-create/), [Distrobox Enter](https://distrobox.it/usage/distrobox-enter/), [KDE-Dialoge](https://develop.kde.org/docs/administration/kdialog/), [Steam: Nicht-Steam-Spiele hinzufügen](https://help.steampowered.com/en/faqs/view/4B8B-9697-2338-40EC).

## Prüfstatus

Implementiert: Paketbauer, eingebetteter Installer, grafischer Ablauf mit KDialog/Zenity beziehungsweise Konsole als Rückfall, Runtime-Einrichtung, Startprüfungen, versionsweises Aktivieren, ROM-/ZIP-Import und automatische Ersteinrichtung.

Automatisiert geprüft: 24 Python-Tests, darunter Selbstextraktion/Manipulationsablehnung, Archivpfade, Erst-/Neuinstallation, Fehler vor Aktivierung, Schutz fremder Container und Importvalidierung. Containerbefehle werden in diesen Unit-Tests ausdrücklich durch Testdoubles abgegrenzt; das ist kein tatsächlicher Distrobox-Installationstest. Native Komponentensuite und neuer Launcher-Build bestehen. Die echte deutsche Alpha-2-ROM wurde aus ihrem ZIP importiert und anhand ihres vollständigen SHA-256 bestätigt. Der Launcher wurde offscreen gestartet.

Nicht verifiziert: vollständige Einrichtung auf einem realen Bazzite-/Podman-System, grafische Hostdialoge, physische Controller, Steam Gaming Mode und 4K-TV. In dieser Entwicklungsumgebung sind Podman und Distrobox nicht installiert. Gemeinsame Sinnoh-Spielwelt und vollständiger Spieldurchlauf bleiben weiterhin offen. Diese Grenzen werden durch einen komfortableren Installer nicht aufgehoben.
