# Entwicklungsstatus — 14.10.2026

**Ergebnis:** vollständiger Linux-Build von Project PM + eigenem Launcher, bestandene ROM-freie Tests und reale Starts zweier Emulatorprozesse mit selbst geschriebenem ARM-Homebrew sowie einer rechtmäßig bereitgestellten deutschen Original-Platin-ROM. Die Original-ROM ist ungepatcht. Noch keine erfolgreiche Pokémon-Sinnoh-Welt-Synchronisation und keine Bazzite-Hardwareabnahme. [Konkreter ROM-Test](rom-boot-check.md).

## Implementiert

- POSIX-Portierung des Project-PM-Bridgecodes: nichtblockierende Sockets, select fd+1, EINPROGRESS, SO_ERROR, MSG_NOSIGNAL.
- Eigener lokaler Host/Client-Modus ohne Autopilot-Tastendrücke, ohne Online-Relay und ohne Broadcast. Konfigurierbarer lokaler TCP-Port.
- Getrennte Prozessprofile und Save-Verzeichnisse je Spieler/ROM-Hash; vorhandener melonDS-SaveManager bleibt für Ingame-Speicherung zuständig.
- mmap-Framebuffer- und Inputschnittstelle; beide echten DS-Prozesse in einem Qt-Fenster.
- Links/rechts und oben/unten; 4:3-Layouts, kleiner Touchscreen, nebeneinander, Einzelansicht-Umschaltung.
- SDL-Instanz-ID-Zuordnung, automatische Erstzuordnung bei genau zwei Geräten / eindeutigen gespeicherten Kennungen, explizite Auswahl bei Mehrdeutigkeit, konfigurierbare Tasten, Stick-Touch und Trennungsschutz.
- TV-Oberfläche mit größeren Elementen auf einer 4K-Fläche; Controller-Menünavigation, Logs, Einstellungen, Backup/Restore.
- Sitzungssperre, SHA-256-Backupmanifeste, atomare Metadaten, gemeinsame Beendigung, Prozessfehlerbehandlung, Parent-death-Signal.
- Distrobox-Build/Run-Wrapper, Desktop-Eintrag und manuelle Steam-Verknüpfung ohne Umschreiben vorhandener Konfigurationen.
- GPL-Lizenz, AGENTS.md, gepinnter Upstream, CI-Workflow, dokumentierte Installation und Abnahme.
- Lokale xdelta-Patchvorbereitung mit CPUD-Kennung, drei SHA-256-Prüfungen, unveränderter Originaldatei und atomarer Ausgabe. Enthält keinen Project-PM-ROM-Patch; [deutsche Kompatibilität und Beschaffungsblocker](german-compatibility.md).

## Automatisiert getestet

Umgebung: Ubuntu 24.04 x86_64, GCC 13.3, CMake 3.28.3, Qt 6.4.2, SDL 2.30.0. Entwicklungsbibliotheken wurden lokal aus Ubuntu-Paketen bereitgestellt; keine Bazzite-Umgebung. Qt-GUI-Tests offscreen, Audio im Smoke-Test dummy. Tests liefen hier. Der GitHub-Actions-Workflow nach der Veröffentlichung am 14.10.2026 bestand ebenfalls: vollständiger Neubau, native Komponentensuite, Produktions-TCP-Bridge, Python-Tests, Qt-Launcher-Start und zwei echte Homebrew-Emulatorprozesse. Erfolgreicher Lauf: https://github.com/Philmenting/pokemon-platinum-local-coop/actions/runs/38607299933 (4 Minuten 43 Sekunden).

| Test | Tatsächliches Ergebnis |
|---|---|
| Vollbuild Project PM | 223 Buildschritte einschließlich Qt/SDL-Frontend und finalem Link erfolgreich |
| Vollbuild Launcher + Tests | Erfolgreich; eigener Code nach Warnungskorrekturen ohne die zuvor gemeldeten Einrückungswarnungen |
| Produktions-TCP-Bridge | Rollen 1/2, Lobby-Namen, beide Richtungen, fragmentierte Frames, ungültige Länge, Wiederverbindung, refused connect und SIGPIPE erfolgreich |
| Native Komponentensuite | IPC in getrennten Prozessen, ABI/Dateigrenzen, 4:3-Geometrie in mehreren Auflösungen, Profilsperre, Backup/Restore, Korruptionsschutz erfolgreich |
| Virtuelle SDL-Controller | Unabhängige DS-Tasten, Touch, Toggle-Flanke, Trennung und keine falsche Übernahme nach Indexverschiebung erfolgreich |
| Prozessmanager | Drei Wiederholungsstarts, graceful stop, Workerfehler und Freigabe der Sperre erfolgreich; bewusst TEST-ONLY-Prozessfixture |
| Zwei echte Emulatorprozesse | Eigenes ARM-Homebrew auf beiden DS-Kernen ausgeführt, RAM-Framebuffer ausgegeben, TCP verbunden, Rollen vergeben, SIGTERM-Aufräumen mit Exitcode 0 |
| Bereitgestellte Original-Platin-ROM | 35 Sekunden in zwei realen Prozessen, >2000 Frames je Spieler, TCP-Rollen 1/2, Eingaben nur an Spieler 1, beide Exitcode 0; ROM-Bridge blieb inaktiv. Opt-in-Test außerhalb CI. |
| Installer | Zwei Python-Tests: Pfade mit Leerzeichen, Argumentquoting, fehlendes Binary; keine Steamkonfigurationsänderung |
| Patchvorbereitung | Echte xdelta3-Testpatches auf eigenen synthetischen Daten: Rekonstruktion, Originalerhalt, Zielschutz, Prüfsummenfehler, englische Kennungen, kaputter Patch und fehlender Decoder |
| Bereitgestellter PM-Patch 0.4.5 | Anleitung verlangt USA Rev 1; echter Decoder verweigert deutsche Original-ROM mit Prüfsummenfehler. ZIP-Inspektion erkennt Patch/ROM und lehnt die unpassende Basis ab. Kein gepatchtes Gameplay gestartet. |
| GUI-Erzeugung | Qt offscreen gestartet, Screenshots bei 1280 × 1000 und 3840 × 2160 erzeugt |
| Buildpatch | Gegen den gepinnten Quellstand reversibel anwendbar geprüft |
| GitHub Actions | Vollständiger Neubau und alle ROM-freien Tests erfolgreich (Run 38607299933) |

Auch der Folgecommit zur Original-ROM-Erkennung bestand den vollständigen GitHub-Neubau und alle damaligen ROM-freien Tests: https://github.com/Philmenting/pokemon-platinum-local-coop/actions/runs/38610385288. Die anschließend ergänzte Patchvorbereitung wurde lokal mit xdelta3 3.0.11 getestet: sieben Python-Testfälle (einschließlich der zwei Installertests), alle erfolgreich. Dabei wurde kein Project-PM-ROM-Patch verwendet; dessen Beschaffung und deutscher Port bleiben blockiert.

Die Patchvorbereitung bestand anschließend auch GitHub CI (https://github.com/Philmenting/pokemon-platinum-local-coop/actions/runs/38612252475). Nach Bereitstellung von Build 0.4.5 wurden Paket-/Basisprüfung und Pokédex-Hinweis ergänzt: lokaler Neubau, native Komponentensuite und nun neun Python-Tests einschließlich ZIP-Inspektion/Mehrdeutigkeitsablehnung erfolgreich. Der reale Patchversuch auf der deutschen ROM wurde erwartungsgemäß abgelehnt; keine gemeinsame Spielwelt getestet. Der zuvor fehlende Patch liegt jetzt vor, aber für die verlangte US-Basis beziehungsweise einen deutschen Port fehlen weiter die passenden Dateien.

Der Homebrew-Test enthält keine Nintendo-Spiel-/Logo-/BIOS-/Firmwarebytes. Er prüft **nicht** ROM-Mailbox-Synchronisation oder Pokémon-Gameplay. Die separate TCP-Prüfung extrahiert den echten Bridgecode; das ist kein Ersatztransport.

## Manuell bestätigt

Die hier erzeugten Qt-Menübilder wurden auf Lesbarkeit und abgeschnittene Inhalte visuell kontrolliert. Private DS-Bilder der deutschen Original-ROM zeigen unterschiedliche Intro-/Einführungszustände nach Eingaben nur an Spieler 1. **Keine Funktion wurde auf dem tatsächlichen Bazzite-PC mit Fernseher und physischen Controllern manuell bestätigt.**

## Nicht verifiziert / offen

1. Patch 0.4.5 wurde bereitgestellt; er verlangt USA Revision 1. Die passende US-Basis liegt nicht vor; ein deutscher CPUD-Patch und ROM-Mod-Quellcode fehlen. [Pakettest](project-pm-045.md).
2. Wichtigstes Akzeptanzkriterium: zwei Spielfiguren interagieren synchron in derselben Sinnoh-Welt. Die bereitgestellte deutsche Original-ROM startet, aber der vorliegende US-Patch passt nicht darauf. Ein deutscher Port wurde nicht erstellt.
3. Tatsächliche Ingame-Save-Write/Load-Vorgänge von Pokémon; die Sicherungs-/Wiederherstellungsmechanik ist geprüft, aber nicht ein kompletter Pokémon-Spielzyklus.
4. Physische USB-/Bluetooth-Controller, Steam-Input-Konflikte, Gerätetrennung und Neuverbindung auf dem Zielsystem.
5. KDE Plasma Wayland, Steam Gamescope, Distrobox-GUI/Audio/Gerätezugriff auf Bazzite, Desktop/Gaming-Mode-Wechsel.
6. 4K/60-Hz-Ausgabe und Performance/Thermik auf RTX 4060; Bildgeometrie und 4K-Menürendering sind geprüft, tatsächliche TV-Ausgabe nicht.
7. Mehrstündiges Spielen, Kämpfe/Tausch und Verbindungsunterbrechungen während des Pokémon-Gameplays.
8. Hochauflösendes DS-3D-Rendering, Rumble und umfangreiche Spielebibliothek: nicht implementiert. Direkter Start einer vorbereiteten ROM und geprüfte lokale xdelta-Patchvorbereitung sind vorhanden; ein deutscher Project-PM-Patch und automatische ROM-Mod-Portierung sind nicht vorhanden.

## Priorisierte nächste Aufgaben

- [x] Project-PM-Linux-Build und tatsächlichen TCP-Transport ermöglichen.
- [x] Profile, Prozessverwaltung, Eingaben, Darstellung und Launcher implementieren.
- [x] Verfügbare ROM-freie Tests und echte DS-Homebrew-Starts durchführen.
- [x] Repository auf GitHub anlegen und lokalen Commitstand veröffentlichen.
- [ ] Passende, rechtmäßig bereitgestellte ROM/Project-PM-Patchversion testen.
- [x] Bereitgestellten Patch 0.4.5 auf Basisanforderung prüfen und echten deutschen Patchversuch durchführen: inkompatible Basis bestätigt.
- [ ] Gemeinsame Sinnoh-Welt nach docs/acceptance-tests.md nachweisen; erst dann spielbare Koop-Version behaupten.
- [ ] Bazzite-/Controller-/TV-Abnahme und Dauerbetrieb.

## GitHub-Veröffentlichung

GitHub-Repository am 14.10.2026 nach Zustimmung über die Weboberfläche privat erstellt: https://github.com/Philmenting/pokemon-platinum-local-coop. Quellcode und Testnachweise sind vollständig übertragen; die Git-Trees der drei Entwicklungsstände stimmen exakt mit den lokal geprüften Ständen überein.

Das private Repository wurde über die GitHub-Weboberfläche erstellt. Die unterstützten Datei-/Git-Operationen wurden anschließend über die verbundene GitHub-App ausgeführt. Kein fremdes Upstream-Repository wurde verändert. Die ursprünglichen drei lokalen Commits bleiben zusätzlich im lokalen Branch local-implementation erhalten; main folgt der veröffentlichten GitHub-Historie.

Das folgende Skript dient ausschließlich der erstmaligen Veröffentlichung einer unabhängigen Kopie ohne bestehendes origin; für dieses bereits veröffentlichte Repository ist es nicht mehr erforderlich:

```bash
./scripts/publish-github.sh            # privat
./scripts/publish-github.sh --public   # öffentlich, falls gewünscht
```

Das Skript verweigert uncommittierte Änderungen und ein bereits bestehendes origin. Es fragt kein Token ab und legt keines im Projekt ab. Die Datei AGENTS.md unterstützt die Fortsetzung mit Codex.
