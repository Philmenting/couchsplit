# Technische Machbarkeit und Quellcodeprüfung

Prüfdatum: 14.10.2026. Alle Aussagen beziehen sich auf die fixierten Stände; die HEADs der Referenzprojekte können sich ändern.

| Projekt | Geprüfter Commit | Lizenz / tatsächlicher Aufbau |
|---|---|---|
| ComicartOlie/melonDS-Project-PM, platinum-mp | 4d9785468bbefc8d08002f16df4d85a12f9611f7 | GPL-3.0-or-later, melonDS 1.1, C++17, Qt5/6 + SDL2 |
| Spuds0588/mgba-splitscreen | b66511a35873bda9c961872813e58b35543bcfc5 | MPL-2.0 für mGBA; C-Kern, Rust/Tauri, HTML/Canvas, WebSocket-Bilder |
| hikaps/couchplay | b8c8e47502bb0eac3471acb896dc69b8290de8c8 | Quelldateien tragen GPL-3.0-or-later SPDX; Qt/KDE, Gamescope und privilegierter Helfer |
| melonDS-emu/melonDS | 906e9ebb27da8c6a715cd7abab4abfe8a8d29427 | GPL-3.0-or-later; CMake, Qt/SDL, getrennte Instanz-Konfiguration im bestehenden Frontend |

## Project PM

Branch/Repository waren erreichbar. Der fixierte Commit vom 01.10.2026 entfernt Windows-DLL-Ballast. Die Branchliste ist in upstream-branches.txt gespeichert. Eine GitHub-Abfrage offener Issues lieferte am Prüfdatum keine offenen Issues; daraus folgt keine Fehlerfreiheit.

Im Quellcode `src/frontend/qt_sdl/EmuThread.cpp` liegen sowohl **Scripted test autopilot** als auch die Bridge. Sie sind unterschiedliche Komponenten. `MELONDS_AP=host|join` startet nicht nur einen Transport, sondern kann die ROM automatisch mit Tasteneingaben durchlaufen. Der Launcher verwendet deshalb das neue `PMCOOP_ROLE` und setzt niemals `MELONDS_AP`. Alle geerbten Autopilotvariablen werden vor dem Workerstart entfernt.

Die Bridge besitzt pro Prozess globale Net-/BridgeSt-Objekte. Zwei unabhängige DS-Prozesse passen zu diesem Aufbau; zwei EmuInstances im selben Prozess würden ohne weitere Änderung diese globals teilen. Deshalb bleibt das Projekt bei zwei Prozessen.

### Linux-Blocker und behobene Ursachen

Der Fork beschreibt Linux-Builds, aber sein zusätzlicher Bridgecode benutzt `SOCKET`, WSA-Funktionen, `DWORD`, `GetTickCount` und `GetComputerNameA` außerhalb von Windows-Guards. Auf Linux fehlten entsprechende Definitionen. Außerdem ignoriert POSIX `select(0, ...)` alle Deskriptoren, während Winsock den ersten Parameter ignoriert. Nichtblockierendes POSIX-connect liefert EINPROGRESS statt Winsock-EWOULDBLOCK. Ein geschlossenes Ziel kann bei POSIX-send SIGPIPE auslösen.

SocketCompat.h adaptiert die API, nutzt fd+1, normalisiert EINPROGRESS und unterdrückt SIGPIPE mit MSG_NOSIGNAL. SO_ERROR prüft, ob ein schreibbereiter connect tatsächlich erfolgreich war. Der eigentliche Frame- und Mailboxcode bleibt erhalten. Unser Patch bindet lokalen Hostbetrieb an loopback, erlaubt einen pro Sitzung ausgewählten Port, deaktiviert lokale Discovery und liefert IPC-Status/Bilder/Eingaben.

### Verbindungen

| Pfad | Protokoll | Bedeutung |
|---|---|---|
| Native melonDS LAN | UDP 7064 / ENet | ursprüngliche LAN-Sitzung, unabhängig vom freien Welt-Koop |
| Project PM Bridge | TCP 7820 standardmäßig | plattformübergreifende ROM-Mailbox-Synchronisation |
| Bridge Discovery | UDP 7821 | PLATMP-Discovery, lokaler Launcher deaktiviert Broadcast |
| Online Relay | TCP 7833 standardmäßig | PMRELAY1; für dieses Projekt nicht verwendet |
| Lokaler Launcher | zufälliger freier TCP-Port auf 127.0.0.1 | gleiche Bridgeframes, direkt host/client ohne native LAN-Lobby |

`startFromEnv()` und `BridgePump()` unterstützen TCP-Start unabhängig von der nativen LAN-Lobby. Der ROM-Discoveryblock wird erst später gesucht. Daher kann der Sockettransport ohne ROM geprüft werden. Der Test tests/test_transport.py extrahiert den **tatsächlichen gepatchten namespace mpnet** in einen C++-Harness und prüft Rollen, Namen, bidirektionale Frames, Fragmentierung, ungültige Länge, Wiederverbindung, verweigerte Verbindung und SIGPIPE. Es wird kein eigener Ersatztransport getestet.

### ROM-Abhängigkeit

Die untersuchte Emulatorquelle enthält keinen geeigneten ROM-Patch. Die GitHub-Suche im Konto ComicartOlie fand die beiden Emulatorports, aber kein eindeutig passendes öffentliches Patchrepository. Die Emulator-README bezeichnet den Hack als Project PM; eine aktuelle, passende Patchquelle und eine konkrete veröffentlichte Patchversion sind damit **nicht verifiziert**.

Die Mailbox-Bridge nutzt Wire v2 (Kommentar: 8 Spieler, ROM-Protokoll v18), Discovery-Magics `0xCAFE1234 / 0x5678CAFE` und Kontrollblock-Magic `BRG1`. Die vom Nutzer bereitgestellte ROM muss diese Schnittstelle tatsächlich veröffentlichen. Der Launcher prüft nur Platin-Headerkennung CPU*, Dateigröße und SHA-256 sowie eine ausdrückliche Nutzerbestätigung. Er kann die Patchkompatibilität nicht beweisen.

**Hauptakzeptanzkriterium offen:** Beide unterschiedlichen Spielfiguren sehen einander in derselben Sinnoh-Welt, bewegen sich unabhängig und speichern getrennt. Kein Transporttest ersetzt diesen Test.

## Architekturvorbilder

mGBA SplitScreen demonstriert Frameausgabe aus mehreren Kernen in einer Oberfläche, Link-Synchronisation und Gamepad-Zuordnung. Der GBA-Kern kann keine Nintendo-DS-ROM ausführen. Der Browser/Tauri-Stack wird hier nicht eingebaut: melonDS besitzt bereits ein brauchbares Qt-Frontend und RAM-Framebuffer. Quellcode wurde nicht übernommen.

CouchPlay isoliert komplexe fremde Spiele durch separate Benutzer, Gerätesteuerung und Gamescope-Prozesse. Die Quelldateien GamescopeInstance.cpp, WindowManager.cpp und SessionRunner.cpp zeigen verschachtelte Gamescope-Ausführung und KWin/D-Bus-Fensterpositionierung; die Installation benötigt einen privilegierten Helfer. Für zwei selbst kontrollierte Emulatorprozesse reichen getrennte Profile und direkte SDL-Zuordnung. Deshalb werden weder Root-Helfer noch temporäre Systemkonten eingeführt. Quellcode wurde nicht übernommen.

Upstream melonDS unterstützt CMake, Qt6 und SDL2, sowie `portable`-Konfigurationsordner neben dem Binary und XDG-Konfigurationspfade. Unser explizites `PMCOOP_PROFILE` erhält Vorrang vor diesen Defaults, damit zwei Worker zuverlässig verschiedene Verzeichnisse benutzen. Die vorhandenen SaveManager- und Framebufferfunktionen werden weiterverwendet.
