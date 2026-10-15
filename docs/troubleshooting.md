# Fehlerbehebung

| Symptom | Prüfung / Handlung |
|---|---|
| Emulator nicht gefunden / falsche Schnittstelle | scripts/build.sh wiederholen; Einstellungen müssen auf melonDS-coop zeigen, nicht den unveränderten Fork oder Upstream. `--coop-capabilities` muss PMCOOP_ABI=1 ausgeben. |
| ROM wird abgelehnt | Eigene DS-ROM mit CPU*-Platin-Kennung und geeigneter Dateigröße wählen; komprimierte Archive zunächst selbst entpacken. |
| Unveränderte deutsche Platin-ROM erkannt | Passenden Project-PM-Patch samt Basis-ROM-Vorgabe beschaffen. Unterstützung der deutschen Sprachversion ist nicht bestätigt. [ROM-Prüfung und Patch-Abhängigkeit](rom-boot-check.md). |
| TCP verbunden, keine zweite Figur | ROM-Patch/Bridge-Version und Aktivierung im Spiel prüfen. Eine unveränderte Platin-ROM reicht nicht. |
| Keine Verbindung | Beide Logs öffnen. Host-bind/Socketfehler oder anderer Prozess auf dem gewählten Port möglich; sauber beenden und neu starten. Keine Portweiterleitung nötig. |
| 15 Sekunden kein Bild | ROM-/Firmwarefehler, blockierender Upstream-Dialog oder Emulator-Hänger; Logs prüfen, gegebenenfalls vollständige eigene Dumps testen. |
| Controller doppelt / Spieler gleichzeitig bewegt | Steam Input für diesen Eintrag deaktivieren; sichtbare SDL-Geräte prüfen. Kein physisches Gerät und dessen Steam-Virtual-Gerät gleichzeitig zuweisen. |
| Controller nach Bluetooth-Trennung ohne Eingaben | Gewollter Schutz vor falscher Zuordnung. Escape ins Menü, „Controller zuordnen“, neu verbundene Instanz ausdrücklich auswählen, „Koop-Spiel starten“ zur Rückkehr. |
| Touchscreen nicht sichtbar | Einzelansicht mit Select + R wechseln; rechter Stick bewegt Cursor, Stick-Klick berührt. |
| Spielstände scheinen verschwunden | ROM-Hash/Spielerprofil prüfen. Andere Patchversion bekommt ein neues Profil. Profilordner über Spielstände öffnen; nicht auf dieselbe Save-Datei für beide Spieler verweisen. |
| Profilsperre aktiv | Andere Launchersitzung beenden. Sperrdateien nicht entfernen, solange Worker laufen. Nach Prozessabsturz kann QLockFile eine nachweislich verwaiste Sperre bereinigen. |
| Erzwungenes Beenden | Backups prüfen. SIGKILL garantiert keinen Flush und verlorener ungespeicherter Fortschritt kann nicht rekonstruiert werden. |
| Distrobox-Start nur im Desktop möglich | Manuellen Containeraufruf testen, Wayland-/Audio-Sockets und Gerätezugriff kontrollieren, dann Gaming-Mode-Abnahme nach docs/acceptance-tests.md durchführen. |
| Unscharfe 4K-Ausgabe | DS-Auflösung bleibt 256 × 192. Der Prototyp skaliert ohne Stretching, transportiert aber noch keine hochauflösenden OpenGL-Framebuffer. |

Sitzungslogs: Datenwurzel `/logs/<Sitzungs-ID>/player1.log` und `player2.log`. Keine ROM-Inhalte werden hochgeladen. Die App enthält keinen Telemetrie- oder Online-Relay-Aufruf im lokalen Modus.

## Installation zeigt lange keine Änderung

Der erste Einstieg in eine neue Ubuntu-Distrobox installiert selbst Grundpakete. Das kann mehrere Minuten dauern. Frühere Installer hielten zusätzlich die komplette Befehlsausgabe bis zum Prozessende zurück; ein währenddessen kopiertes Log endete deshalb direkt nach `distrobox enter`, obwohl die Installation weiterlief. Das allein ist kein Nachweis für einen Netzwerkfehler oder Absturz.

Den aktuellen Installer verwenden. Er zeigt einen animierten Balken mit Schritt und Zeitangabe und schreibt die Ausgabe laufend ins Installationslog. Bei einem fehlgeschlagenen Erststart werden Status und letzte Containerlogzeilen ergänzt. Eine vorhandene eigene Runtime wird wiederverwendet; Container oder Datenordner müssen für ein Update nicht gelöscht werden. Keine zweite Installation starten, solange noch eine läuft.

## Beenden führt nur ins Menü zurück

In älteren Builds stoppte „Spiel beenden“ nur die Sitzung; im bereits sichtbaren Menü schien der Knopf wirkungslos. Im korrigierten Build heißt er **Launcher beenden** und schließt die Anwendung einschließlich laufender Emulatorprozesse. Das gilt für Mausklick und Controller-Bestätigung. Start + Select im Spiel beendet weiterhin nur die Sitzung. Für ein Update die bisherige Launcher-Version zuerst über das Fenster-Schließen bzw. Alt + F4 beenden und dann den neuen Installer ausführen.

## Container-Neustart: „creating temporary passwd file … merged/etc/passwd: no such file or directory“

Der Fehler tritt vor dem Programmstart in Podman auf. Ein Bericht mit Podman 5.8.4 zeigte ihn bei einem zuvor erfolgreich initialisierten, inzwischen beendeten Container. Die Meldung belegt einen fehlenden Pfad im eingebundenen Containerdateisystem; ob eine veraltete Einhängung oder ein anderer Speicherfehler vorliegt, ist damit noch nicht bewiesen.

Der aktuelle Installer versucht nur bei diesem genauen Fehler, Zustand `exited`/`stopped`, ausdrücklich `Running=false` und erneut verifiziertem Eigentumslabel einmal `podman container cleanup platinum-coop-runtime-v1`, danach erneut den Einstieg. Laut [Podman-Dokumentation](https://docs.podman.io/en/stable/markdown/podman-container-cleanup.1.html) bereinigt dieser Befehl Einhängungen/Netzwerkzustand beendeter Container und entfernt ohne `--rm` keinen Container. Kein `--all`, `prune`, `system reset`, erzwungenes Entfernen oder Löschen des Datenordners. Das ist ein begrenzter Reparaturversuch, kein auf dem betroffenen PC bestätigter Fix. Bei erneutem Fehler das aktuelle Log bereitstellen; die vorhandene Installation und Daten bleiben erhalten.
