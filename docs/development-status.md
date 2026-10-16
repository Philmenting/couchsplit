# Entwicklungsstatus — 16.10.2026

**Aktueller Nachtrag:** Alpha 4 wurde mit der bereitgestellten deutschen PM-0.4.5-ROM tatsächlich bis zum Pokédex gespielt. Zwei Figuren und ihre Begleiter bewegen sich synchron; ein gemeinsamer Wildkampf mit getrennten Befehlen, identischen Gegnerzuständen, Sieg/EP/Levelaufstieg und anschließendem Ingame-Speichern/-Laden wurde unter Linux geprüft. Native Windows-/Bazzite-Spielabnahme, Trainer-Koop, Tausch und mehrstündiger Dauertest bleiben offen. Die älteren Abschnitte unten beschreiben den damaligen Prüfstand.

Am 16.10.2026 wurde die veraltete Zielauswahl nach besiegtem Gegner erneut im tatsächlichen Koop-Kampf reproduziert und eine separate, exakt hashgebundene ROM-Korrektur erstellt. Der vermutete dauerhafte Begleiterverlust bestätigte sich im Nachtest nicht. [Korrektur und Prüfgrenzen](rom-target-cursor-fix.md).

## Implementiert

- POSIX-Portierung des Project-PM-Bridgecodes: nichtblockierende Sockets, select fd+1, EINPROGRESS, SO_ERROR, MSG_NOSIGNAL.
- Eigener lokaler Host/Client-Modus ohne Autopilot-Tastendrücke, ohne Online-Relay und ohne Broadcast. Konfigurierbarer lokaler TCP-Port.
- Getrennte Prozessprofile und Save-Verzeichnisse je Spieler/ROM-Hash; vorhandener melonDS-SaveManager bleibt für Ingame-Speicherung zuständig.
- mmap-Framebuffer- und Inputschnittstelle; beide echten DS-Prozesse in einem Qt-Fenster.
- Links/rechts und oben/unten; 4:3-Layouts, kleiner Touchscreen, nebeneinander, Einzelansicht-Umschaltung.
- SDL-Instanz-ID-Zuordnung, automatische Erstzuordnung bei genau zwei Geräten / eindeutigen gespeicherten Kennungen, explizite Auswahl bei Mehrdeutigkeit, konfigurierbare Tasten, Stick-Touch und Trennungsschutz.
- TV-Oberfläche mit größeren Elementen auf einer 4K-Fläche; Controller-Menünavigation, Logs, Einstellungen, Backup/Restore.
- Sitzungssperre, SHA-256-Backupmanifeste, atomare Metadaten, gemeinsame Beendigung, Prozessfehlerbehandlung, Parent-death-Signal.
- Distrobox-Build/Run-Wrapper, Desktop-Eintrag und automatische Steam-Anfrage über Bazzites Host-Helfer ohne direkte Bearbeitung vorhandener Konfigurationen.
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
| US-PM-ROM 0.4.5 nach weiterer Bereitstellung | Reguläres xdelta mit aktiven Prüfungen erfolgreich, Ergebnis-SHA-256 bestätigt und aus ZIP reproduziert. Zwei echte Prozesse 35 Sekunden, je 2075 Frames, TCP-Rollen 1/2, beide Exitcode 0. Einführung getestet; Wireless-Aktivierung und gemeinsame Welt offen. |
| GUI-Erzeugung | Qt offscreen gestartet, Screenshots bei 1280 × 1000 und 3840 × 2160 erzeugt |
| Buildpatch | Gegen den gepinnten Quellstand reversibel anwendbar geprüft |
| GitHub Actions | Vollständiger Neubau und alle ROM-freien Tests erfolgreich (Run 38607299933) |

Auch der Folgecommit zur Original-ROM-Erkennung bestand den vollständigen GitHub-Neubau und alle damaligen ROM-freien Tests: https://github.com/Philmenting/pokemon-platinum-local-coop/actions/runs/38610385288. Die anschließend ergänzte Patchvorbereitung wurde lokal mit xdelta3 3.0.11 getestet: sieben Python-Testfälle (einschließlich der zwei Installertests), alle erfolgreich. Dabei wurde kein Project-PM-ROM-Patch verwendet; dessen Beschaffung und deutscher Port bleiben blockiert.

Die Patchvorbereitung bestand anschließend auch GitHub CI (https://github.com/Philmenting/pokemon-platinum-local-coop/actions/runs/38612252475). Nach Bereitstellung von Build 0.4.5 wurden Paket-/Basisprüfung und Pokédex-Hinweis ergänzt: lokaler Neubau, native Komponentensuite und nun neun Python-Tests einschließlich ZIP-Inspektion/Mehrdeutigkeitsablehnung erfolgreich. Der reale Patchversuch auf der deutschen ROM wurde erwartungsgemäß abgelehnt; keine gemeinsame Spielwelt getestet. Der zuvor fehlende Patch liegt jetzt vor, aber für die verlangte US-Basis beziehungsweise einen deutschen Port fehlen weiter die passenden Dateien.

Nach zusätzlicher Bereitstellung der US-Basis: Patchskript unterstützt den bekannten Release aus ZIP mit automatischen Prüfsummen und verbietet abweichende Hash-Overrides. Das echte Ergebnis wurde ein zweites Mal reproduziert. Lokaler Neubau und native Tests erfolgreich; zehn Python-Tests bestehen, einschließlich automatischer Release-Prüfsummen/ZIP-Decodierung mit eigenem Testdatenkatalog und echtem xdelta. Patched-US-Start: 35 Sekunden, je 2075 Frames, TCP verbunden, beide sauber beendet. Die englische Datei wurde dem Nutzer separat bereitgestellt; keine ROM in GitHub.

Der Homebrew-Test enthält keine Nintendo-Spiel-/Logo-/BIOS-/Firmwarebytes. Er prüft **nicht** ROM-Mailbox-Synchronisation oder Pokémon-Gameplay. Die separate TCP-Prüfung extrahiert den echten Bridgecode; das ist kein Ersatztransport.

## Deutsche Text-Alpha, 15.10.2026

Implementiert: stdlib-Nitro/NARC/Nachrichtenwerkzeuge, Drei-Wege-Abgleich, 857 eigene Übersetzungsfingerabdrücke (1.208 Vorkommen), deutsche Originaltexte und geprüfte Fontübernahme, exakte Eingabehashes, atomare Ausgabe ohne Überschreiben. Alle PM-Programm-/Overlay-/Skriptbytes bleiben unverändert. Die unbenutzte regional unterschiedliche Diamond/Pearl-Abspannbank wird erhalten. [Verfahren, Abdeckung und Grenzen](german-localization.md).

Automatisiert getestet: fünf neue synthetische Format-/Merge-/Kontrollzeichen-/Byteinvarianztests; vollständige Python-Suite mit echtem xdelta3: 15 Tests erfolgreich, keine übersprungen. Zweimaliger echter ROM-Build ergibt identischen SHA-256 `8b11f36e7bf4a15932eb364358fba183c2067ac8265cfbc97611ca53e3c90949`. 40 Sekunden in zwei echten Emulatorprozessen: 2369/2370 Frames, je 2097 zusätzliche Frames nach Sekunde 5, TCP verbunden, Rollen 1/2, beide sauber mit Exitcode 0 beendet. Funkspiel-Bridge blieb vor dem Pokédex inaktiv; keine Welt-Synchronisation bewiesen.

Visuell geprüft: deutsche Begrüßung „Schön, dich kennenzulernen!“ mit Umlaut im echten Emulator-Framebuffer. Nicht verifiziert: vollständige Story, alle PM-Menüs und Textbreiten, sprachabhängige US-Grammatikroutinen, Ingame-Save-Migration, Bazzite-Hardware. Grafisch eingebettete Schrift ist nicht lokalisiert; „YES/NO“ in der Einführung wurde als englisch erkannt. Ein weiterer 105-Sekunden-Lauf mit Eingaben an Spieler 1 bestand mit 6272/6273 Frames, TCP-Verbindung und beiden Exitcodes 0. Originaltext-Anleitungen auf beiden DS-Schirmen wurden deutsch dargestellt.

## JA/NEIN-Grafik, Alpha 2

Implementiert: originale deutsche Touch-Textur aus dem regionalen Pfad unter dem US-Pfad übernommen. Die übrigen neun Archivmitglieder (Palette und acht Animationskarten) sind bytegleich, entpackte Grafikgröße und alle PM-Programmbytes unverändert. Keine Nintendo-Grafikbytes im Repository. Schutz vor PM-Modifikationen und inkompatiblen Ressourcen durch zwei neue Tests; komplette Python-Suite jetzt 17 Tests erfolgreich.

Automatisiert getestet: zwei byteidentische Builds, SHA-256 `800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c`. 110 Sekunden in zwei echten Emulatorprozessen, 6566/6571 Frames, TCP verbunden, Rollen 1/2, beide Exitcodes 0. Visuell geprüft: JA/NEIN im Framebuffer vor dem Touch; einmaliges Antippen von JA führt zu Professor Eibe und dem nächsten Dialog. Kein Bazzite-/Hardwaretest und kein Welt-Koop-Nachweis. Andere Grafiken wurden dadurch nicht automatisch lokalisiert. [Details](german-localization.md).

## Ein-Datei-Installer

Implementiert: siehe [Installationsablauf und abgegrenzter Prüfstatus](one-file-installer.md). Automatisiert getestet: 38 Python-Tests sowie nativer Neubau/Komponententest und Offscreen-Launcher-Start bestehen. Echter ROM-ZIP-Import ergibt den unveränderten Alpha-2-SHA-256. Sechs zusätzliche Steam-Tests prüfen den Host-Helferaufruf, Wiederholschutz, explizite Wiederholung und Fehlergrenzen mit Testdoubles. Weitere fünf Installer-Regressionstests prüfen Live-Ausgabe, UTF-8, Timeout/Prozessgruppen, Containerdiagnose und Fortschrittsansteuerung. Ein nativer UI-Test bestätigt den Beenden-Knopf mit Mausereignissen und virtueller SDL-Navigation einschließlich geordnetem Stoppen zweier Testprozesse und Freigeben der Savesperre. Auf Bazzite bleiben Fortschritts-/Beenden-Korrektur und Steam-Aufnahme nachzuprüfen.

## Manuell bestätigt

Die hier erzeugten Qt-Menübilder wurden auf Lesbarkeit und abgeschnittene Inhalte visuell kontrolliert. Private DS-Bilder der deutschen Original-ROM zeigen unterschiedliche Intro-/Einführungszustände nach Eingaben nur an Spieler 1. **Benutzerrückmeldung vom 15.10.2026:** Der Installer lief auf Bazzite durch und der Launcher startete. Dabei fehlte sichtbarer Fortschritt und der bisherige Beenden-Knopf schloss die App nicht. Die Korrekturen wurden automatisiert geprüft; ihre Bestätigung auf dem tatsächlichen PC und Tests mit physischen Controllern stehen aus.

## Nicht verifiziert / offen

1. Patch 0.4.5 und passende US-Rev-1-Basis wurden bereitgestellt und erfolgreich gepatcht. Die US-basierte deutsche Text-Alpha ist inzwischen implementiert und gestartet; ein nativer CPUD-Patch und vollständige Spielprüfung fehlen. [Pakettest](project-pm-045.md).
2. Die gemeinsame Welt, Funkspiel-Aktivierung und gemeinsame Wildkämpfe sind inzwischen unter Linux mit der Nutzer-ROM geprüft. Trainer-Koop, Tausch und Zielhardware bleiben offen.
3. Tatsächliches Ingame-Speichern und erneutes Laden beider Pokémon-Spielstände sind unter Linux geprüft; kein vollständiger Spieldurchlauf.
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
- [x] Passende, rechtmäßig bereitgestellte ROM/Project-PM-Patchversion auf Start testen.
- [x] US-basierte deutsche Text-Alpha bauen und Einführung prüfen.
- [x] Passende USA-Rev-1-Basis mit PM 0.4.5 patchen und in zwei echten Emulatoren auf Intro-/TCP-/Prozessfunktion testen.
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

## Podman-Neustartfehler nach Zielsystem-Rückmeldung

Implementiert: begrenzter Bereinigungsversuch ohne Containerlöschung für den exakten Fehler „creating temporary passwd file … merged/etc/passwd: no such file or directory“, nur nach Stop-/Eigentumsprüfung. Automatisiert getestet: drei weitere Tests prüfen Erfolgspfad, maximal einen Wiederholungsversuch und Ablehnung laufender/fremder/anders fehlerhafter Container. Nicht verifiziert: dass `podman container cleanup` den konkreten Fehler auf dem Bazzite-PC behebt. Der Bericht mit Podman 5.8.4 zeigt erfolgreiche frühere Initialisierung und einen fehlgeschlagenen späteren Start; die tieferliegende Speicherursache ist offen. Keine ROM-/Save-Löschung und keine Eingriffe in andere Container.


## Native Plattformpakete 0.2.0 Alpha 1

Bazzite 0.1.0-alpha.1 ist als unveränderter GitHub-Prerelease veröffentlicht. Der neue Code ergänzt native Windows-Dateimappings, kooperative Worker-Abschaltung (IPC ABI 2), Unicode-ROM-Import und Windows-Job-Gruppen. Paketbauer erzeugen ein ZorinOS-18-/Ubuntu-24.04-DEB sowie eine Windows-Setup-EXE und portable ZIP mit passenden eigenen und Emulatorquellen.

Die Freigabe ist in `.github/workflows/platforms.yml` an erfolgreiche native Linux-/Windows-Builds, Komponenten- und ROM-freie Emulatorprüfungen gebunden. Zusätzlich werden DEB und Windows-Setup in der CI installiert, der installierte Launcher gestartet und anschließend deinstalliert; persönliche Daten müssen erhalten bleiben. Details und tatsächliche Ergebnisse stehen beim jeweiligen Release-Commit in GitHub Actions. Das ersetzt keine manuelle Abnahme auf ZorinOS/Windows, mit Bluetooth-Controllern oder einer synchronen Sinnoh-Spielwelt.

Windows-Nativtests bestätigen die Komponenten einschließlich kontrollierter Worker-Abschaltung. Die Transportprüfung deckte außerdem den Windows-Firewall-Helfer im PM-Upstream auf: Eigene lokale Sitzungen überspringen ihn jetzt ausdrücklich; es werden keine Firewall-Regeln oder Administratorprompts für Loopback eingerichtet.


## Plattformübergreifende Fehlerkorrekturen 0.2.0 Alpha 2

Implementiert am 15.10.2026: fester Koop-Speichername `game.sav` innerhalb des unveränderten Spieler-/ROM-Hash-Profils. Der Launcher sichert vor der Übernahme, kopiert einen eindeutig zugeordneten alten Save und erhält die Quelldatei. Mehrdeutige Altbestände verlangen die ursprüngliche ROM-Auswahl. Das Wiederherstellen älterer Backups aktualisiert ebenfalls den festen Save. Die neue Emulatorfähigkeit `STABLE_SAVE=1` verhindert das Mischen mit alten Emulator-Binaries; IPC bleibt ABI 2. Der eigene Emulatorpfad verändert nur Koop-DS-Saves, keine Savestate-, Cheat-, GBA- oder regulären melonDS-Dateinamen.

Das Hauptmenü besitzt einen Scrollbereich und folgt dem Fokus bei Tastatur-/Controllerbedienung. Lange ROM-/Controllernamen erzwingen keine überbreiten Fenster. Minimieren neutralisiert Tasten und Touch sofort und bei jedem weiteren Timerlauf; verborgene Zeit zählt nicht zur Beenden-Kombination.

Lokal auf Zorin OS 18.1 bestanden: 38 Python-Tests mit xdelta3, beide nativen CTest-Ziele, produktiver Transport und die aus dem echten Emulatorquellcode kompilierte Speicherpfadprüfung. Neue native Regressionen decken Save-Übernahme, umbenannte ROMs, Altbackups, unveränderte aktuelle Saves bei Mehrdeutigkeit, getrennte Spieler/ROM-Hashes sowie 360p/720p/768p/1080p/4K-Menüs, Scrollen per virtuellem SDL-Controller und Eingaben im minimierten Fenster ab. Auch der neu gebaute Launcher bei 1280 × 720 und zwei neu gebaute echte Emulatoren mit selbst erzeugtem ARM-Homebrew bestehen den Bild-/TCP-/geordneten Abschalttest. Windows- und Bazzite-Zielsystembedienung sowie echte Pokémon-Spielstände/Sinnoh-Synchronisation werden dadurch nicht behauptet. Native Plattform-Builds und Paketprüfungen bleiben vor Veröffentlichung erforderlich.


Zusätzlicher Integrationsbefund vom 16.10.2026: Ein echter wiederholter Session-Start mit umbenanntem ARM-Homebrew reproduzierte einen TOML-Serialisierungsabsturz. Die Konfigurations-Getter legen fehlende Werte an, während ein anderer Emulator-Thread den Baum speichert. Ein separater Test gegen den echten `Config.cpp` reproduziert denselben Fehler vor der Korrektur. Die öffentlichen Tabellen-/Arrayzugriffe sowie Laden/Speichern teilen jetzt einen rekursiven Mutex; nach der Korrektur besteht der gleichzeitige Getter/Setter/Array-/Serialisierungstest. Die Dateioperationen des Testprogramms sind ausdrücklich lokale Platform-Testgrenzen. Nach dem Neubau bestehen auch zwei echte vollständige Session-Starts mit identischen, unterschiedlich benannten Test-ROMs und getrennten, aktualisierten Save-Markern für beide Spieler; beide Sitzungen beenden ihre Worker geordnet und geben die Sperre frei.

## Aktive Fehlerfälle und Korrekturen 0.2.0 Alpha 3

Am 16.10.2026 mit virtuellen SDL-Controllern und zwei echten Linux-Emulatoren reproduziert: Das minimierte Hauptmenü führte noch Controller-Aktionen aus; eine neue ROM-Auswahl änderte die Anzeige trotz weiterlaufender alter Sitzung; frei belegte Tasten der Ansichts-/Beenden-Kombination gelangten zusätzlich als DS-Eingabe zum Spiel.

Korrigiert im gemeinsamen Plattformcode: Das minimierte/verborgene Menü nimmt keine Aktion an und verlangt nach der Rückkehr zunächst das Loslassen gehaltener Tasten. Die ROM-Auswahl wird vor dem Dateidialog mit einem Hinweis auf das reguläre Sitzungsende blockiert. Tastenkombinationen verbrauchen ihre physischen Tasten vor der frei gewählten DS-Zuordnung; andere Tasten und Spieler bleiben unabhängig.

Lokal bestätigt: beide nativen CTest-Ziele und alle 38 Python-Tests erfolgreich. Neue native Regressionen prüfen minimiertes/verborgenes Menü, gehaltene Taste beim Wiederherstellen, weiterhin funktionierende neue Tastendrücke, blockierte ROM-Auswahl mit unveränderten Einstellungen und fortsetzbarer Sitzung, beide Kombinationen mit allen zwölf DS-Belegungen sowie unbeteiligte DS-Start-/R-Eingaben. Die Tests verwenden produktive Widgets und virtuelle Controller; die nativen Sitzungstests verwenden ausdrücklich Test-Worker.

Zusätzlich besteht der isolierte Aktivtest mit unveränderten produktiven Widget-/Session-Funktionen und zwei echten Emulatorprozessen: alle drei ursprünglichen Fehlerfälle, zulässiger Import und tatsächlicher Start von ROM B nach Ende der Sitzung A, Trennung ohne Geräteübernahme, Minimieren im Spiel, Prozessausfall sowie zwölf vollständige Starts und Beendigungen. Unterschiedliche vorbereitete Test-Spielstände beider Spieler bleiben dabei bytegenau erhalten; die Homebrew-ROM schreibt keine Pokémon-Saves. Die drei Korrekturen wurden mit frischen Profilen unter Qt offscreen und auf einem virtuellen X11-Bildschirm erneut erfolgreich geprüft. 720p-Menüaufnahme sowie der separate Produktions-Emulator-Test mit Bildausgabe, TCP-Rollen und geordneter Abschaltung bestehen ebenfalls.

Pakete werden als 0.2.0-alpha.3 gekennzeichnet. Native Windows-/Linux-Neubauten und Paketinstallationsprüfungen sind vor Auslieferung abzuwarten. Physische Controller, Bazzite/Wayland/Gamescope und tatsächliche Pokémon-Spielwelt bleiben davon getrennte offene Abnahmen.

## Controller-Menü und Spielhilfe 0.2.0 Alpha 4

Der aktive Nachtest von Alpha 3 reproduzierte zwei weitere Fehler: Nach ausdrücklicher Zuordnung steuerte weiterhin das erste SDL-Gerät das Menü, und die einzeilige Bedienhilfe wurde bei 1280 × 720 abgeschnitten. Beide Korrekturen gelten im gemeinsamen Code für Linux und Windows.

Die Menüabfrage gehört nach der ersten erfolgreichen Live-Zuordnung ausschließlich Spieler 1. Ein abgezogener Spieler-1-Controller bleibt ohne Eingabe; andere Geräte und neue Ersatzgeräte übernehmen nicht automatisch. Auch eine fehlgeschlagene Neuzuordnung schaltet den anfänglichen Geräte-Fallback nicht wieder frei. Vor der ersten Live-Zuordnung bleibt die ursprüngliche Navigation zur Einrichtung erhalten. Nach ausdrücklicher Neuzuordnung funktioniert die Menübedienung wieder.

Die Bedienhilfe verwendet zwei echte Qt-Textlabels mit Wortumbruch. Jeder Spielerbereich reserviert die anhand seiner Breite benötigte Höhe und hält die DS-Bilder oberhalb. Lokale native Regressionen bestätigen vollständigen Text bei 360p/720p/768p/1080p/4K in beiden Spieleraufteilungen, verfügbaren Bildbereich, fremde/Spieler-2-Eingaben ohne Aktion, Menü-Navigation durch Spieler 1, Trennung mit verschobenen Geräteindizes, keine Ersatzübernahme und erfolgreiche ausdrückliche Neuzuordnung.

Lokal bestehen beide CTest-Ziele und alle 38 Python-Tests. Der isolierte Aktivtest mit produktiven Widgets, virtuellen SDL-Controllern und zwei echten Emulatoren bestätigt beide neuen Korrekturen einschließlich fortlaufender Sitzung, Eingabe nach Neuzuordnung und Menü-Beenden mit Freigabe der Sperre. Die nativen Tests verwenden für Sitzungen ausdrücklich Test-Worker; der zusätzliche Aktivtest verwendet selbst erzeugte ARM-ROMs und die echten Emulator-Binaries des letzten nativen Linux-Pakets. Native Windows-/Linux-Neubauten und Installationsprüfungen sind vor Auslieferung erforderlich. Physische Controller, Bazzite/Wayland/Gamescope, Pokémon-Spielstände und die gemeinsame Sinnoh-Welt bleiben getrennte offene Abnahmen.


## Nachuntersuchung der ROM-Auffälligkeiten, 16.10.2026

Die unveränderte Nutzer-ROM reproduzierte die leere Zielauswahl in einem zweiten gemeinsamen Wildkampf. Die passende Cursorinitialisierung im Kampf-Overlay wurde mit dem ursprünglichen PRET-Code abgeglichen. Ein separates Python-Werkzeug erzeugt eine neue, hashgebundene ROM-Kopie mit Prüfung auf weiterhin verfügbare gespeicherte Ziele. Original-ROM und Saves bleiben erhalten; neue ROM-Hashes erzeugen eigene Profile. Die Lokalisierung selbst behält weiterhin ihre bisherigen Programmbyte-Invarianten.

Alle 46 Python-Tests einschließlich Ausführung der tatsächlichen neuen Thumb-Instruktionen in 128 Zustandskombinationen bestehen lokal; beide vorhandenen nativen CTest-Ziele bestehen ebenfalls. Der zusätzliche ARM-Test prüft Register-/Stackerhaltung und ausschließlich die beabsichtigten Cursor-Schreibzugriffe; eine eigene Linux-/Windows-CI macht ihn verpflichtend. Die Begleiter wurden ohne Codeänderung beim Weitergehen wieder sichtbar und blieben nach einem weiteren gemeinsamen Kampf sichtbar. Ein dauerhafter Verlust wird nicht mehr als bestätigter Fehler geführt.

Der aktive Korrektur-Nachtest mit zwei echten Alpha-4-Emulatorprozessen bestand: nach dem ersten besiegten Staralili sprang der Cursor beider Spieler auf das verbleibende Staralili; A wurde ohne zusätzliche Richtungseingabe angenommen. Gemeinsame Runde, Sieg, EP und beiderseitiger Aufstieg auf Lv. 7 sowie Rückkehr mit sichtbaren Begleitern wurden geprüft. Der Codefix gilt in der ROM für alle Plattformen; dieser Spielnachweis stammt ausschließlich von Linux. Ingame-Speichern/-Laden der korrigierten ROM ist ebenfalls mit einer regulären Wiederholung und geänderter Position beider Figuren bestätigt. Ein zuvor zu früh beendeter Speichervorgang im Testtreiber lud den alten Checkpoint; daraus wird kein Produktfehler abgeleitet. Alle Sitzungen endeten geordnet ohne verbleibende Worker oder Sperre. Details und Fingerabdrücke in der verlinkten Korrekturdokumentation.


## Launcher im Handheld-/Retro-Stil, 0.2.0 Alpha 5

Auf ausdrücklichen Wunsch des Nutzers: modernere native Qt-Oberfläche mit stärkerem Pokémon-/Retro-Bezug. Eingebettete Silkscreen-Überschriften (SIL OFL 1.1), gut lesbare Systemschrift für Bedienelemente, dunkles Gehäuse, helle Spiel-/Controllerinformationen, roter Start und goldene Fokusmarkierung. Eigene geometrische Symbole, keine Nintendo-Grafiken oder -Schriften. Alle Funktionen sind bei 1280 × 720 ohne Scrollen erreichbar. Schmale Fenster stapeln die Bereiche und behalten Fokus-Scrollen; 4K nutzt größere Schrift und Bedienelemente. Dialoge folgen derselben Gestaltung; Abbrechen/Schließen werden deutsch beschriftet.

Produkt- und Designkontext stehen in PRODUCT.md, DESIGN.md und .impeccable/design.json. Der Quellcode der Gestaltung ist gemeinsam für Windows und Linux; bestehende Controller-/Spielstand-/Sitzungslogik bleibt erhalten. Die Bildschirmanzeige trägt Koralle/Cyan für Spieler 1/2 und erhält den vollständigen umgebrochenen Hilfetext.

Lokal erfolgreich: beide nativen CTest-Ziele (inklusive Controllerbesitz, Trennung, Fokus, Minimieren, blockiertem ROM-Wechsel und Session-Abschaltung) sowie alle 46 Python-Tests mit echtem xdelta-Decoder und ARM-Regressionsengine. Zusätzliche native Prüfung bestätigt Font-Einbettung, passende Beschriftungsbreiten und Aktivierung per Leertaste entsprechend dem angezeigten Tastaturhinweis. Gebündelte X11-Aufnahmen prüfen 360p, 720p, 1080p, 4K, eingerichteten/leeren Zustand und einen tatsächlichen Dialog. Zwei echte Worker mit selbst geschriebenem ARM-Homebrew liefern die Frames für beide Spieleraufteilungen; beide enden geordnet und geben die Sperre frei. Diese roten/grünen Testbilder sind ausdrücklich kein Pokémon-Spielnachweis.

Die unabhängige Designkontrolle gibt das geprüfte Hauptmenü frei. Native Windows-/Linux-Neubauten, Installationsprüfungen und Paketausgabe werden vor Auslieferung separat abgewartet. Physische Controller, tatsächliche Bazzite-/Wayland-/Gamescope-Bedienung und Pokémon-Dauertests bleiben offen. Der separate Zielfix der ROM gehört nicht in die Installer und wird nicht durch ein Launcher-Update automatisch angewendet.
