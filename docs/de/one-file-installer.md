# Eine Datei für Bazzite

[English](../one-file-installer.md) · **Deutsch** · [Anleitungen](README.md)

Der Installer `CouchSplit-Bazzite.run` enthält beide fertigen Linux-x86_64-Programme und ihren vollständigen zugehörigen GPL-Quellcode. Eine ROM ist nicht enthalten. Ein GitHub-Konto, manuelle Paketinstallation und Kompilieren sind für die Nutzung nicht erforderlich.

## Benutzen

1. Im Bazzite-Desktop herunterladen. In Dolphin: Rechtsklick → Eigenschaften → Berechtigungen → **Als Programm ausführbar**. Browser übernehmen dieses Linux-Dateirecht beim Download üblicherweise nicht.
2. Doppelklicken und die Installation starten. Internet, vorhandene Bazzite-Distrobox/Podman und mindestens 3 GiB freier Speicher werden benötigt. Die Einrichtung kann beim ersten Mal mehrere Minuten dauern. Ein animierter Balken, der aktuelle Schritt und eine laufende Zeitangabe zeigen die Aktivität auch während langer Downloads.
3. Beim ersten interaktiven Start zunächst English oder Deutsch wählen. Anschließend öffnet sich die ROM-Auswahl. Die vorbereitete Project-PM-ROM als `.nds` oder das bereitgestellte ROM-ZIP auswählen. Das ZIP wird geprüft und genau eine enthaltene DS-ROM importiert. Die verifizierten PM-0.4.5-US-/Deutsch-Alpha-Fingerabdrücke werden automatisch erkannt. Andere Mods benötigen weiterhin eine ausdrückliche Bestätigung.
4. Beide Controller verbinden und „Koop-Spiel starten“ wählen. Bei zwei Controllern erfolgt die erste Zuordnung automatisch; über „Controller zuordnen“ lässt sie sich ändern.

Die ROM wird mit privaten Dateirechten in den eigenen Datenordner kopiert. Der Download kann danach entfernt werden. Eine normale ungepatchte Platin-ROM wird weiterhin abgelehnt; der Installer beschafft keine Spiele und patcht diese nicht automatisch.

Später im Startmenü **CouchSplit** öffnen. **Launcher beenden** schließt die Anwendung und beendet zuvor beide Emulatorprozesse. Start + Select im Spiel beendet weiterhin nur die Sitzung und kehrt zum Menü zurück. Der Installer fordert automatisch einen Nicht-Steam-Eintrag für den Gaming Mode an. Falls nötig, in Steam anmelden; unter **Steam-fremd** nach dem Launcher suchen. Steam Input für diesen Eintrag deaktivieren, falls doppelte oder emulierte Tastatureingaben auftreten. BIOS/Firmware ist für den voreingestellten freien Direktstart nicht erforderlich.

## Automatisch zu Steam hinzufügen

Der Installer ruft **auf dem Host** Bazzites vorhandenes `steamos-add-to-steam` mit unserem `.desktop`-Eintrag auf. Steam verarbeitet die Aufnahme für das dort angemeldete Konto. Der stabile Wrapper bleibt bei Updates gleich; die neue Programmversion wird dadurch über den vorhandenen Steam-Eintrag gestartet. Steamdateien, Controlleroptionen und andere Spiele werden von unserem Code nicht direkt bearbeitet.

Eine Sperre und eine eigene Datei `steam-registration.json` verhindern eine erneute automatische Anfrage bei Neuinstallation. Die Datei bestätigt nur die **Anfrage**, nicht die tatsächliche Aufnahme durch Steam. Auch bei einem Timeout kann Steam den Eintrag bereits erhalten haben; deshalb wird dann nicht still erneut angefordert. Fehlt die Bazzite-Funktion, bleibt die Anwendung installiert und die Abschlussmeldung erklärt den Rückfall.

Falls der Eintrag fehlt, im Desktop-Steam anmelden und zuerst die Bibliothek prüfen. Danach bei Bedarf im Startmenü Rechtsklick auf den Launcher → **Erneut zu Steam hinzufügen**. Alternativ im Terminal:

```bash
~/.local/bin/pokemon-platinum-local-coop --add-to-steam
```

Dies wiederholt die Anfrage ausdrücklich und kann bei bereits vorhandenem Eintrag einen Doppelgänger erzeugen. Auch nach einem Steam-Kontowechsel oder bewusstem Entfernen des Eintrags diese Aktion verwenden. Manuell geht weiterhin „Spiel hinzufügen → Steam-fremdes Spiel“. Für die automatische Aufnahme wird Bazzites native Steam-Integration vorausgesetzt; eine separat installierte Flatpak-Version wird hier nicht automatisch konfiguriert.

Primärquellen: Bazzite [installiert den KDE-Helfer](https://github.com/ublue-os/bazzite/blob/main/spec_files/steamdeck-kde-presets/steamdeck-kde-presets-desktop.spec); das [GNOME-Gegenstück](https://github.com/ublue-os/bazzite/blob/main/spec_files/steamdeck-gnome-presets/steamos-add-to-steam) zeigt das verwendete Steam-Protokoll und die Unterstützung von Desktopdateien. Der vorhandene Helfer wird aufgerufen, kein fremder GPLv2-Skriptcode in dieses GPLv3-Projekt kopiert. Quellstand am 15.10.2026 geprüft. Der tatsächliche Steam-Client ist in dieser Entwicklungsumgebung nicht verfügbar.

## Was eingerichtet wird

- Eigene rootless Distrobox `platinum-coop-runtime-v1` mit Ubuntu 24.04, passend zur Buildumgebung. Ubuntu ist nur die Laufzeit im Container; das Bazzite-System bleibt bestehen.
- Laufzeitbibliotheken über signierte Ubuntu-Paketquellen innerhalb dieses Containers. `sudo` kommt nur dort zum Einsatz. Der Installer lehnt rootful Podman und fremd belegte Containernamen ab. Kein rpm-ostree, kein Root-Dienst, keine Treiber- oder NVIDIA-Konfigurationsänderung.
- Programme und Quellcode unter `$XDG_DATA_HOME/pokemon-platinum-local-coop/software/releases/…`, aktive Version über `software/current`.
- Eigener Wrapper `~/.local/bin/pokemon-platinum-local-coop` und eigener Startmenüeintrag unter `$XDG_DATA_HOME/applications/`. Standard für XDG_DATA_HOME: `~/.local/share`.
- Spielstände und Einstellungen bleiben in der bisherigen Datenwurzel. Die Installation löscht sie nicht. ROM-Importe liegen unter `roms`, Installationslogs unter `installer-logs`. Befehlsausgaben werden schon während der Ausführung geschrieben; bei Wartezeiten kommt alle 15 Sekunden eine Aktivitätsmeldung hinzu.

Der Wrapper verwendet die Containerbibliotheken und setzt den Datenordner ausdrücklich. Er entfernt störende Steam-/Host-Bibliothekspfade nur für den gestarteten Prozess. XWayland wird bei vorhandenem DISPLAY verwendet, sonst Qt-Wayland. Softwaredarstellung benötigt keine NVIDIA-Treiberintegration in den Container.

Vor Aktivierung einer Version prüft der Installer Emulator-ABI und Qt-Launcher mit einem temporären Testprofil. Erst bei Erfolg wechselt die aktive Version. Der erste Containerstart wird ausdrücklich als Ersteinrichtung ausgeführt. Schlägt er fehl oder überschreitet sein Zeitlimit, sammelt der Installer zusätzlich den Containerstatus und die letzten 200 Containerlogzeilen. Er entfernt den Container nicht automatisch. Nur beim genau erkannten Podman-Neustartfehler mit fehlendem `merged/etc/passwd` versucht er für einen nachweislich beendeten eigenen Container eine Bereinigung temporärer Einhängungen und genau einen erneuten Einstieg. Andere Fehler, laufende oder fremde Container lösen dies nicht aus. Bei Fehlern bleiben bisherige aktive Version und Spielstände erhalten; ein erneuter Start kann die Einrichtung fortsetzen. Frühere Softwareversionen bleiben zum Wiederherstellen erhalten.

Zum Entfernen zuerst beide Spiele schließen. Den eigenen Startmenüeintrag und Wrapper entfernen; den eigenen Runtime-Container bei Bedarf über Distrobox entfernen. Den Datenordner mit Spielständen separat sichern und nur bei ausdrücklichem Wunsch löschen. Der Installer installiert keine automatische Deinstallationsroutine, die versehentlich Saves entfernen könnte.

## Paket prüfen / selbst bauen

```bash
bash CouchSplit-Bazzite.run --check
```

Das prüft das vollständige eingebettete Paket und alle Dateifingerabdrücke ohne Installation. Es ist ein Integritätscheck, keine digitale Herausgebersignatur. Archivpfade, Links, Dateigrößen und Dateiliste werden vor Ausführung des Installationsprogramms geprüft. Es werden keine heruntergeladenen Shellskripte ausgeführt.

Für Entwickler nach sauberem Ubuntu-24.04-Build:

```bash
scripts/build.sh
python3 -m unittest discover -s tests -p 'test_*.py'
python3 scripts/make-installer.py --upstream .cache/project-pm --output /tmp/CouchSplit-Bazzite.run
bash /tmp/CouchSplit-Bazzite.run --check
```

Vor Paketbau Änderungen committen. Der Paketbauer rekonstruiert den gepinnten Upstream samt eigenem Patch und vergleicht ihn mit dem tatsächlich verwendeten Quellbaum. Beide Binärdateien müssen nach dem letzten C++-Änderungsstand neu gebaut sein. `.git`, Zugangsdaten, ROMs, Firmware und Spielstände werden nicht mitgeliefert. Das Paket enthält die eigenen Projektdateien, die gepatchte vollständige Upstreamquelle und die Buildanleitung; keine Laufzeitbibliotheken vom Host.

Primärquellen: [Distrobox auf Bazzite](https://docs.bazzite.gg/Installing_and_Managing_Software/Distrobox/), [Distrobox Create](https://distrobox.it/usage/distrobox-create/), [Distrobox Enter](https://distrobox.it/usage/distrobox-enter/), [KDE-Dialoge](https://develop.kde.org/docs/administration/kdialog/), [Steam: Nicht-Steam-Spiele hinzufügen](https://help.steampowered.com/en/faqs/view/4B8B-9697-2338-40EC).

## Prüfstatus

Implementiert: Paketbauer, eingebetteter Installer, grafischer Ablauf mit KDialog/Zenity beziehungsweise Konsole als Rückfall, Runtime-Einrichtung, Startprüfungen, versionsweises Aktivieren, ROM-/ZIP-Import, automatische Ersteinrichtung und Steam-Helferaufruf mit Wiederholschutz/erneuter Anfrage.

Automatisiert geprüft: 38 Python-Tests, darunter Selbstextraktion/Manipulationsablehnung, Archivpfade, Erst-/Neuinstallation, Fehler vor Aktivierung, Schutz fremder Container und Importvalidierung. Sechs zusätzliche Steam-Tests prüfen Argumente mit Sonderzeichen, Wiederholschutz, explizite Wiederholung, fehlenden Helfer, Fehler/Timeout, beschädigte Metadaten, Root-Ablehnung und Dispatch vor dem Containerstart. Steam-Aufrufe und Containerbefehle werden in diesen Unit-Tests ausdrücklich durch Testdoubles abgegrenzt; das ist kein tatsächlicher Distrobox-Installationstest. Fünf zusätzliche Regressionstests prüfen laufende Ausgabe vor Prozessende, unvollständige UTF-8-Bytes, Zeitüberschreitungen mit erhaltener Ausgabe/Prozessgruppenbeendigung, nichtinteraktive Befehle, Containerdiagnose und die animierte KDE-Ansteuerung. Native Komponentensuite und neuer Launcher-Build bestehen. Ein zusätzlicher nativer UI-Test betätigt den echten Beenden-Knopf über Mausereignisse und virtuelle SDL-Controller, prüft außerdem das geordnete Beenden zweier Testprozesse und das Freigeben der Spielstandsperre. Die Testprozesse emulieren kein Spiel. Drei zusätzliche Tests begrenzen die Podman-Bereinigung auf den erkannten eigenen/beendeten Container und genau einen Wiederholungsversuch. Dieser externe Reparaturaufruf ist nur mit Testdoubles geprüft. Die echte deutsche Alpha-2-ROM wurde aus ihrem ZIP importiert und anhand ihres vollständigen SHA-256 bestätigt. Der Launcher wurde offscreen gestartet.

Benutzerrückmeldung vom 15.10.2026: Installation lief auf Bazzite durch und der Launcher startete; Fortschritt war nicht erkennbar, Beenden schloss die App nicht. Diese beiden Fehler sind nun im Code korrigiert. Nicht auf dem Zielsystem nachgeprüft: die neuen Fortschrittsdialoge und der Beenden-Fix, physische Controller, tatsächliche Aufnahme durch einen laufenden Steam-Client, Steam Gaming Mode und 4K-TV. In dieser Entwicklungsumgebung sind Podman und Distrobox nicht installiert. Gemeinsame Sinnoh-Spielwelt und vollständiger Spieldurchlauf bleiben weiterhin offen. Diese Grenzen werden durch einen komfortableren Installer nicht aufgehoben.
