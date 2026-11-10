# Englische Feuerrot-Koop-Profile

[English](../firered-english-profiles.md) · **Deutsch** · [GBA-Unterstützung](firered.md)

Der Entwicklungsstand ergänzt experimentellen Welt-Koop für die unveränderten
englischen Originalversionen 1.0 und 1.1. Partneranzeige, gemeinsame native
Kämpfe mit getrennten Eingaben sowie persönliche Teams, Taschen und Saves
verwenden dieselben Koordinatoren wie die deutsche Ausgabe. Die veröffentlichten
Alpha-11-Pakete unterstützen weiterhin nur die deutsche ROM. Englisch benötigt
einen zusammen gebauten neuen Launcher und Worker. Vor neuen Binärpaketen
stehen native Plattform- und Paketprüfungen an.

## Unterstützte Originaldateien

| Ausgabe | Header | SHA-256 |
|---|---|---|
| Deutsches Original | BPRD, Revision 0 | `eed4fb0242bcbb6e0b0b80b197b47e3aec99ef4c5562cc45bae50ae86b970507` |
| Englisch 1.0 | BPRE, Revision 0 | `3d0c79f1627022e18765766f6cb5ea067f6b5bf7dca115552189ad65a5c3a8ac` |
| Englisch 1.1 | BPRE, Revision 1 | `729041b940afe031302d630fdbe57c0c145f3f7b6d9b8eca5e98678d0ca4d059` |

Der vollständige Fingerabdruck wählt das Profil, bevor die beiden Kerne
entstehen. Sprache im Header, Dateiname oder ein gültiger GBA-Header allein
schalten keinen Koop frei. Veränderte Dateien und andere Revisionen behalten
den getrennten GBA-Modus. Jeder Spieler und vollständige ROM-Hash haben einen
eigenen Save-Ordner; die englische Auswahl beginnt daher getrennt von einer
vorhandenen deutschen Sitzung.

Launcher und Worker gemeinsam bauen, die englische ROM auswählen und
**Einstellungen → Feuerrot-Koop (experimentell)** aktivieren. Die Einstellungen
erkennen unterstützte englische Originale. Launcher- und Spielsprache bleiben
getrennte Einstellungen. Der neue Worker meldet `FIRERED_ENGLISH=1`; bei einem
älteren Worker wird eine englische Koop-Sitzung vor der Save-Sperre abgewiesen.

## Prüfung der Adressen

Die bisherigen deutschen Adressen bleiben die internen Kennungen.
`RomProfileScope` bindet jeden geprüften Kern für die Lebensdauer seiner
Koordinatoren an das passende ROM-Profil. Nur feste ABI-Stellen werden gezielt
übersetzt; dynamische Spielzeiger und verschlüsselte Pokémon-Daten bleiben
unverändert. Aufrufprüfungen normalisieren ausschließlich den nativen LR-Wert.
Unbekannte Aufrufer laufen nativ weiter. Beide Kerne und ihre Profilbindungen
werden auf demselben Worker-Thread ausgeführt.

Jedes englische Profil enthält eigene Callback-, Daten- und IWRAM-Adressen sowie
alle 38 Kampf-Hooks mit je vier geprüften ursprünglichen Halbwörtern. Vor dem
ersten Eingriff werden die passenden Prüfwerte kontrolliert. Entfernen und
Rollback stellen die ursprünglichen Befehle im Arbeitsspeicher wieder her;
die ROM-Datei bleibt schreibgeschützt und bytegleich.

Kampfbegrüßungen und Spielernamen haben innerhalb derselben Funktion andere
Positionen. Auch der Wunschtraum-Platzhalter liegt an anderer Stelle im Text.
Diese Fälle sind ausdrücklich zugeordnet. Ruhmeshallen-Skriptzeiger und deren
Prüfwerte folgen ebenfalls dem Profil. Der Hinweis bei reservierten
Teamplätzen erscheint in der Sprache der ROM.

Die [englische Fassung](../firered-english-profiles.md#address-and-instruction-review)
verlinkt die festgelegten Quellstände. Die deutsche Rekonstruktion stimmt mit
dem bisherigen Original-Fingerabdruck überein. Alle Hooks, 23 Aufrufstellen,
42 feste EWRAM-Anker sowie ursprüngliche Skript- und Textstellen wurden in den
Originaldateien abgeglichen. ROMs, Saves und Checkpoints gehören zu keinem
Quell-, Test-, Installations- oder Werbepaket.

## Prüfstand und Grenzen

Alle 24 normalen Komponenten-/Oberflächentests bestehen, einschließlich
Profiltrennung, unbekannter Adressen/Aufrufer, falscher Prüfwerte und Entfernen
der englischen Hooks. Erste echte ROM-Tests bestehen auf Englisch 1.0 und 1.1:
gemeinsamer Wildkampf mit beiden Eingaben und persönliche native Saves mit
anschließendem frischem Continue aus der Batterie. Die größere Szenariomatrix
und Aufnahmen im Produktionslauncher werden gesondert abgeschlossen.

Diese ersten Ergebnisse ersetzen keinen vollständigen Kampagnentest. Die
bisherigen Grenzen bei Partnerkollision/-überdeckung, Sonderkampfmodi,
physischen Controllern und tatsächlichem Windows-/Bazzite-Gameplay gelten
weiter. Native Build-Prüfungen bestätigen diese Spieleigenschaften nicht.

Die [englische Anleitung](../firered-english-profiles.md#verification-and-limits)
enthält reproduzierbare Befehle für private Eingaben. Die Werkzeuge sind vom
normalen Build und der Installation ausgeschlossen. Sie verwenden vorbereitete
Testteams und Begegnungen in isolierten Kernen und prüfen auch Rollback,
getrenntes natives Speichern und Neustart.
