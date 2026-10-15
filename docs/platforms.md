# Native Plattformausgaben

## Bazzite

.run im Desktop ausführen. Vorhandene Distrobox/Podman erforderlich. Die eigene Ubuntu-Laufzeit wird eingerichtet. Steam-Anfrage und Spielstände entsprechen der bisherigen Anleitung.

## ZorinOS 18 / Ubuntu 24.04, x86_64

Die Datei Pokemon-Platinum-Coop-ZorinOS18-amd64.deb herunterladen und über die Softwareverwaltung installieren. Alternativ:

    sudo apt install ./Pokemon-Platinum-Coop-ZorinOS18-amd64.deb

Der Launcher erscheint im Anwendungsmenü. Beim ersten Start die eigene vorbereitete ROM oder ihr ZIP auswählen. Kein Distrobox-Container. Laufzeitpakete werden durch APT aufgelöst. ZorinOS 17 mit Ubuntu-22.04-Basis wird von diesem Paket nicht unterstützt.

Programme/Quellen: /opt/pokemon-platinum-local-coop. Persönliche Daten: $XDG_DATA_HOME/pokemon-platinum-local-coop, standardmäßig ~/.local/share/pokemon-platinum-local-coop. Paketentfernung löscht diese persönlichen Daten nicht.

## Windows 10/11 x64

Pokemon-Platinum-Coop-Windows-Setup.exe ausführen. Standard: Installation im eigenen lokalen Programmeordner, ohne Administratorrechte. Startmenü-/Desktop-Verknüpfungen werden angelegt. Die portable ZIP ist eine Alternative: vollständig entpacken und Start.bat öffnen. Die ZIP nicht direkt im Archiv starten.

Beide echten Emulatoren und Qt/SDL-Laufzeitbibliotheken sind enthalten. Ein eingebetteter CPython-Interpreter führt ausschließlich das geprüfte ROM-/ZIP-Importskript aus; keine separate Python-Installation erforderlich. Daten liegen im von Qt ermittelten persönlichen Datenordner unter AppData/Local/pokemon-platinum-local-coop. Die Deinstallation entfernt keine dortigen ROMs/Saves.

Die Anwendung schreibt keine gemeinsamen Saves. Normales Beenden sendet eine IPC-Abschaltanforderung und wartet auf beide Worker. Eine Windows-Job-Gruppe beendet verbleibende eigene Worker bei einem Launcherabsturz. Nach erzwungenem Prozessabbruch lässt sich ungespeicherter Fortschritt nicht wiederherstellen.

## Steam und Tests

Bazzite fordert automatisch die Aufnahme an. Auf ZorinOS und Windows in Steam „Spiel hinzufügen → Steam-fremdes Spiel“ wählen und den Launcher hinzufügen. Bei doppelten Controller-Eingaben Steam Input nur für diesen Eintrag deaktivieren.

Die CI baut beide Betriebssystemfamilien und führt native Tests sowie ROM-freie Emulator-Smoke-Tests aus. Die grafische Installation und physische Hardware auf ZorinOS/Windows bleiben manuell zu prüfen. Gemeinsame Sinnoh-Welt, kompletter deutscher Spieldurchlauf und Dauerbetrieb sind gesonderte Abnahmepunkte.

Quellen: https://help.zorin.com/docs/apps-games/install-apps/ (ZorinOS-18-Basis Ubuntu 24.04), https://www.msys2.org/docs/ci/ , https://doc.qt.io/qt-6/windows-deployment.html .
