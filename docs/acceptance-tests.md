# Verbleibende manuelle Abnahme auf dem Ziel-PC

Für jeden Test Datum, Buildcommit, Bazzite-Version, Displaybackend, Controllergeräte, ROM-SHA-256 und Ergebnis festhalten. Keine ROMs oder Firmware an GitHub anhängen.

1. **Gemeinsame Sinnoh-Welt (entscheidend):** passende Project-PM-ROM auf beiden Instanzen starten, zwei verschiedene Namen/Charaktere, ROM-Multiplayer aktivieren, gleiche Karte besuchen. Beide Figuren sehen einander, bewegen sich unabhängig und wechseln Karten. NPC-/Objektinteraktionen und vom Hack unterstützte Kämpfe/Tausch getrennt prüfen. Screenshot allein beweist keine Synchronisation; Änderungen beobachten.
2. **Saves:** beide Spieler unterschiedlich fortschreiten lassen, jeweils Ingame speichern, gemeinsam sauber beenden, neu starten. Keine gegenseitige Überschreibung. Backup wiederherstellen und nur den gewählten Spieler kontrollieren.
3. **Controller:** alle DS-Tasten getrennt testen. Controller 1 bewegen, Spieler 2 bleibt unbeeinflusst. Touch für beide testen, Zuordnung tauschen, Bluetoothgerät trennen und neu verbinden. Keine automatische Übernahme des zweiten Controllers. Steam Input aus/an vergleichen und geeignete Einstellung dokumentieren.
4. **Ansichten:** 3840 × 2160 / 60 Hz, links/rechts (je 1920 × 2160 Fläche), oben/unten und alle DS-Layouts. 4:3-Seitenverhältnis, Cursor, Schriften und Select+R prüfen. Kein Stretching.
5. **KDE Wayland / Steam Gamescope:** erst Desktop, dann Gaming Mode, zum Desktop wechseln und zurück. Vollbild, Fokus, Audio, 60-Hz-Ausgabe und Controllerzugriff prüfen.
6. **Abbruch:** Worker kontrolliert beenden, TCP-Verbindung verlieren/neu aufbauen, Launcher via Steam beenden; keine übrig gebliebenen Prozesse. Nach erzwungenem Abbruch Saves und Sperre prüfen.
7. **Dauerbetrieb:** zunächst 30 Minuten, dann mindestens 2 Stunden mit Bewegung/Kämpfen/Speichern. CPU, Speicher, thermisches Verhalten, Ton, Verbindungsabbrüche und Framerate beobachten. Keine NVIDIA-Treiber- oder PCIe-Einstellungen durch das Projekt ändern.
8. **Fehlende Dateien:** Emulatorpfad, ROM und jeweils eine externe BIOS-/Firmwaredatei entfernen/umbenennen. Verständliche Fehler, kein verlorener bestehender Spielstand, danach erfolgreicher erneuter Start.
9. **Wiederholte Starts:** zehn Sitzungen starten/speichern/beenden; Backupzuordnung, Profiltrennung und Prozessbereinigung prüfen.

Die ROM-freien Tests prüfen diese Komponenten nur in den explizit in development-status.md genannten Teilbereichen. Sie ersetzen keine Zielplattformabnahme.
