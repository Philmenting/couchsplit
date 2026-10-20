# Native Plattformausgaben

[English](../platforms.md) · **Deutsch** · [Anleitungen](README.md)

## Bazzite

.run im Desktop ausführen. Vorhandene Distrobox/Podman erforderlich. Die eigene Ubuntu-Laufzeit wird eingerichtet. Steam-Anfrage und Spielstände entsprechen der bisherigen Anleitung.

## ZorinOS 18 / Ubuntu 24.04, x86_64

Die Datei CouchSplit-ZorinOS18-amd64.deb herunterladen und über die Softwareverwaltung installieren. Alternativ:

    sudo apt install ./CouchSplit-ZorinOS18-amd64.deb

Der Launcher erscheint im Anwendungsmenü. Beim ersten Start Deutsch oder English wählen, danach die eigene vorbereitete ROM oder ihr ZIP auswählen. Kein Distrobox-Container. Laufzeitpakete werden durch APT aufgelöst. ZorinOS 17 mit Ubuntu-22.04-Basis wird von diesem Paket nicht unterstützt.

Programme/Quellen: /opt/pokemon-platinum-local-coop. Persönliche Daten: $XDG_DATA_HOME/pokemon-platinum-local-coop, standardmäßig ~/.local/share/pokemon-platinum-local-coop. Paketentfernung löscht diese persönlichen Daten nicht.

## Windows 10/11 x64

CouchSplit-Windows-Setup.exe ausführen. Standard: Installation im eigenen lokalen Programmeordner, ohne Administratorrechte. Startmenü-/Desktop-Verknüpfungen werden angelegt. Die portable ZIP ist eine Alternative: vollständig entpacken und Start.bat öffnen. Die ZIP nicht direkt im Archiv starten.

Beide echten Emulatoren und Qt/SDL-Laufzeitbibliotheken sind enthalten. Ein eingebetteter CPython-Interpreter führt ausschließlich das geprüfte ROM-/ZIP-Importskript aus; keine separate Python-Installation erforderlich. Daten liegen im von Qt ermittelten persönlichen Datenordner unter AppData/Local/pokemon-platinum-local-coop. Die Deinstallation entfernt keine dortigen ROMs/Saves.

Die Anwendung schreibt keine gemeinsamen Saves. Normales Beenden sendet eine IPC-Abschaltanforderung und wartet auf beide Worker. Eine Windows-Job-Gruppe beendet verbleibende eigene Worker bei einem Launcherabsturz. Nach erzwungenem Prozessabbruch lässt sich ungespeicherter Fortschritt nicht wiederherstellen.

## Update auf 0.2.0 Alpha 2

Launcher und Emulator gemeinsam aktualisieren. Der Launcher verlangt neben IPC ABI 2 die Emulatorfähigkeit `STABLE_SAVE=1`. Koop-Spielstände heißen innerhalb des bisherigen Spieler-/ROM-Hash-Profils jetzt `game.sav`, damit ein geänderter ROM-Dateiname den Fortschritt nicht mehr versteckt. Vor der ersten Übernahme wird ein Backup angelegt, die alte Datei bleibt erhalten. Bei mehreren älteren Saves zuerst die ursprünglich verwendete ROM auswählen; der Launcher rät keine Zuordnung. Ältere Backups werden beim Wiederherstellen auf den festen Speichernamen übernommen. Unterschiedliche ROM-Hashes werden weiterhin nicht automatisch zusammengeführt.

## Oberfläche in 0.2.0 Alpha 5

Die Programme erhalten den gemeinsam implementierten Handheld-/Retro-Stil mit Pixel-Überschriften, lesbarer Systemschrift, klarer Startaktion und sichtbarem Controller-/Tastaturfokus. Die Schrift ist samt OFL-Lizenz enthalten. ROMs, Profile, Spielstände und Controllerzuordnungen verwenden weiter ihre bisherigen Verzeichnisse. Die bisherige vorbereitete ROM bleibt verwendbar; ein separater ROM-Zielfix wird nicht automatisch installiert.

## Steam und Tests

Bazzite fordert automatisch die Aufnahme an. Auf ZorinOS und Windows in Steam „Spiel hinzufügen → Steam-fremdes Spiel“ wählen und den Launcher hinzufügen. Bei doppelten Controller-Eingaben Steam Input nur für diesen Eintrag deaktivieren.

Die CI baut beide Betriebssystemfamilien und führt native Tests sowie ROM-freie Emulator-Smoke-Tests aus. Automatisierte Windows-/Ubuntu-Installations-, Start- und Deinstallationsprüfungen sind Teil der CI; physische Hardware und interaktive Zielsystem-Abnahme bleiben separat. Tatsächliche Welt-Koop-/Wild-/Trainerkampfprüfungen stammen von Linux mit virtuellen SDL-Controllern. Ein kompletter deutscher Spieldurchlauf und Dauerbetrieb sind nicht bestätigt.

Quellen: https://help.zorin.com/docs/apps-games/install-apps/ (ZorinOS-18-Basis Ubuntu 24.04), https://www.msys2.org/docs/ci/ , https://doc.qt.io/qt-6/windows-deployment.html .


## Öffentliche Ausgabe 0.2.0 Alpha 6

Die [Release-Seite](https://github.com/Philmenting/couchsplit/releases/tag/v0.2.0-alpha.6) bündelt die zuvor lokal getesteten Alpha2–5-Verbesserungen und die eigenen ROM-Korrekturwerkzeuge. Pakete und Werkzeuge stammen vom selben Release-Commit; Veröffentlichung erfolgt erst nach nativen Linux-/Windows- und verpflichtenden ARM-Prüfungen. Kein ROM-Download enthalten. [Spielstände bei geändertem ROM-Hash übernehmen](save-transfer.md).


## Sprachauswahl in 0.2.0 Alpha 7

Beim ersten interaktiven Start erscheint „Language / Sprache“ vor der ROM-Einrichtung — auch nach einem Update, wenn das bisherige Profil noch keine Sprache enthält. Deutsch und English stehen zur Wahl. Die Auswahl wird im vorhandenen persönlichen `settings.json` als `language: de` oder `language: en` gespeichert. Abbrechen beendet die Ersteinrichtung ohne eine Auswahl zu speichern. Beim nächsten Start wird eine gültige gespeicherte Auswahl direkt verwendet.

Unter **Einstellungen → Sprache / Settings → Language** lässt sich die Oberfläche später sofort umschalten. Eine laufende Sitzung und ihre getrennten Spielstände bleiben erhalten. Die Auswahl übersetzt den Launcher, seine Hilfe und eigenen Fehlermeldungen; die Sprache der ROM und rohe Betriebssystem-/Emulator-Logs ändern sich dadurch nicht. Native Dateiauswahldialoge können weiterhin der Systemsprache folgen. Beschädigte Einstellungen werden mit einem Hinweis abgelehnt und nicht überschrieben.


## CouchSplit — Alpha 8

Das Projekt heißt jetzt CouchSplit. Die neuen Downloads verwenden `CouchSplit-…`; ältere Release-Dateien behalten ihre bisherigen Namen. Bestehende Installationen lassen sich darüber aktualisieren. Zur Kompatibilität bleiben die bisherigen technischen Kennungen (`platinum-coop`, `pokemon-platinum-local-coop`, Windows-AppId/Installationsordner und Bazzite-Laufzeit) erhalten. Vorhandene Einstellungen, ROMs und Spielstände werden weiter am bisherigen Ort verwendet; keine automatische Migration oder Zusammenführung. Die sichtbaren Launcher- und Startmenünamen sind CouchSplit. Bestehende Steam-Verknüpfungen können noch den alten Namen zeigen; ihre Konfiguration wird nicht direkt bearbeitet.
