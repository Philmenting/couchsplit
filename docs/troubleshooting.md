# Fehlerbehebung

| Symptom | Prüfung / Handlung |
|---|---|
| Emulator nicht gefunden / falsche Schnittstelle | scripts/build.sh wiederholen; Einstellungen müssen auf melonDS-coop zeigen, nicht den unveränderten Fork oder Upstream. `--coop-capabilities` muss PMCOOP_ABI=1 ausgeben. |
| ROM wird abgelehnt | Eigene DS-ROM mit CPU*-Platin-Kennung und geeigneter Dateigröße wählen; komprimierte Archive zunächst selbst entpacken. |
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
