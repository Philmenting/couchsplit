# Project PM 0.4.5: Zielauswahl nach besiegtem Gegner

Untersuchung vom 16.10.2026 mit der vom Nutzer bereitgestellten deutschen Alpha-2-ROM. Die Launcher-Ausgabe bleibt Alpha 4. Die neue ROM-Kopie heißt **Deutsch Alpha 2 + Zielauswahl-Fix 1**; es handelt sich nicht um einen neuen offiziellen Project-PM-Release.

## Bestätigter Fehler und Ursache

In zwei echten gemeinsamen Wildkämpfen auf Route 201 blieb die vorherige Auswahl nach dem Besiegen des linken Gegners auf dessen leerem Feld. A bestätigte keinen Befehl; Rechts zum lebenden Gegner und A funktionierten. Damit liegt kein allgemeiner Eingabeausfall oder eingefrorener Netzwerktransport vor.

Die ursprüngliche Logik ist im [PRET-Quellcode](https://github.com/pret/pokeplatinum/blob/1dc13dcea40c04ab53f1f607de46023dc409ef20/src/battle/battle_subscreen.c#L3534) nachvollziehbar: `BattleSystem_Cursor_Battler` übernimmt bei gleichem Ziellayout die gespeicherte Cursorposition ohne Anwesenheitsprüfung. Die A-Verarbeitung prüft die Anwesenheit dagegen und verwirft die leere Auswahl. Das gleiche Muster wurde im unveränderten Kampf-Overlay 16 der konkreten Nutzer-ROM abgeglichen. Die vollständigen Project-PM-ROM-Mod-Quellen liegen weiterhin nicht vor; die öffentlichen Emulatorquellen sind kein Ersatz dafür.

## Korrektur

`romfixes/pm045_target.py` ersetzt ausschließlich zwei Codebereiche innerhalb dieses verifizierten, unkomprimierten Overlays. Die selbst geschriebene Thumb-1-Implementierung steht in `patches/pm045-target-cursor.s`.

- Bei passendem Layout und verfügbarem Ziel bleibt die vorige Auswahl erhalten.
- Andernfalls gilt die ursprüngliche Priorität der verfügbaren Zielfelder: links oben, rechts oben, links unten, rechts unten (Battler-Slots 3, 1, 0, 2).
- Flächen-/Selbstzielmenüs, manuelle Navigation, Attacken, Gegner, Netzwerkdaten und Spielstände werden nicht verändert.
- Der vorhandene Literalpool, Overlaygröße/-adresse, BSS, Dateitabelle, Header, Texte und alle übrigen ROM-Dateien bleiben bytegleich.
- Beide Spieler müssen dieselbe korrigierte ROM benutzen. Die Änderung gilt auf allen Plattformen, die diese ROM emulieren; tatsächlicher Spieltest hier unter Linux. Kein Windows-/Bazzite-Spieltest behauptet.

Die frühere Vorgabe, bei **reiner Lokalisierung** alle Programmbytes zu erhalten, gilt weiterhin für `localize-pm.py`. Dieses separate, ausdrücklich beauftragte Fehlerkorrekturverfahren ändert erstmals bewusst die beschriebenen Programmstellen. Es überschreibt weder die Alpha-2-ROM noch bestehende Ausgaben.

## Verwendung

Python 3, ohne Zusatzpakete:

```text
python scripts/fix-pm-target-cursor.py --rom Pokemon-Platinum-PM-0.4.5-DE-Alpha2.nds --output Pokemon-Platinum-PM-0.4.5-DE-Zielfix1.nds
```

Nur der exakte Alpha-2-Fingerabdruck wird akzeptiert. Eine unbekannte, veränderte, englische oder bereits korrigierte ROM wird abgewiesen. Vorhandene Zieldateien und derselbe Quell-/Zielpfad werden abgewiesen. Keine Prüfsummenübersteuerung. Original und alle Saves bleiben erhalten.

Die neue Datei im Koop-Starter hinzufügen und bestätigen, dass sie vorbereitet ist. **Der neue ROM-Hash verwendet ein eigenes Spielstandverzeichnis.** Bestehende Spielstände werden nicht automatisch übertragen. Für die private Testwiederholung wurden die beiden eigenen Checkpoint-Saves ausdrücklich in isolierte neue Testprofile kopiert; dies ist keine automatische Migration von Benutzerprofilen.

| Datei | SHA-256 |
|---|---|
| Eingabe: deutsche Alpha 2, 141.962.768 Byte | `800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c` |
| Kampf-Overlay 16, 235.360 Byte | `c6acaf7747783d49d2a1bc549efaf009ddb0f9f584a1569d41ed76b90a34d716` |
| Ausgabe: Deutsch Alpha 2 + Zielfix 1, gleiche Größe | `dfe675837c5b935a684eb2e18c446dbc7865ba45bcb2343d2158892f6b720520` |

Die beiden ersetzten Bereiche beginnen im ROM bei `0x2300ec` (64 Byte) und `0x230140` (12 Byte), zur Laufzeit bei `0x0227f3ec` und `0x0227f440`. Keine Nintendo-Programmbytes, ROM, Saves, Screenshots oder Originaltexte werden mit dem Quellcode veröffentlicht.

## Regressionen und tatsächlicher Prüfstand

ROM-freie Tests führen die neuen ARM-Instruktionen mit Unicorn aus: alle 16 Anwesenheitsmasken × vier vorherige Cursorpositionen × gleiches/geändertes Layout = 128 Kombinationen. Geprüft werden gültige Zielauswahl, Erhaltung gültiger voriger Ziele, unveränderter Stack und erhaltene Register sowie ausschließlich zwei Cursor-Byte-Schreibzugriffe. Gesonderte Tests prüfen Datei-/Hashgrenzen, geschützte Originale/Zieldateien und den Ergebnisfingerabdruck. Der ARM-Test ist in einer eigenen Linux-/Windows-CI verpflichtend; nur die Testumgebung benötigt `tests/requirements-romfix.txt`.

Der ursprüngliche Fehler wurde zusätzlich mit den tatsächlichen ursprünglichen ARM-Instruktionen lokal reproduziert; derselbe Zustand wählt nach der Korrektur den lebenden rechten Gegner. Originalcode bleibt dabei privat.

**Aktiver Nachtest mit korrigierter ROM bestanden:** Zwei echte Alpha-4-Emulatorprozesse, virtuelle SDL-Controller, gleiche ROM bei beiden Spielern, gemeinsamer Wildkampf gegen zwei Staralili (Lv. 2). Nach dem Besiegen des linken Staralili stand der Cursor auf beiden DS-Schirmen beim lebenden rechten Staralili. Einmal A bei Spieler 1 wurde angenommen („Warte auf den anderen Spieler …“), während Spieler 2 weiterhin seine eigene Auswahl sah. Nach dessen A lief die Runde weiter; beide gewannen, erhielten EP und erreichten Lv. 7. Rückkehr ins Feld und alle eigenen/entfernten Begleiter sichtbar. Keine zusätzliche Richtungseingabe zur Reparatur des Ziels nötig.

Ingame-Speichern/-Laden der korrigierten ROM ebenfalls bestanden: beide Figuren auf eine andere Position auf Route 201 bewegt, regulär gespeichert, Abschluss abgewartet, beide Prozesse beendet und neu gestartet. Beide luden die neue Position. Beide Dateien sind 524.288 Byte groß und nach dem Laden unverändert. Ein vorheriger Testlauf wurde vom Testtreiber während der Speicheranzeige beendet und lud deshalb den älteren Checkpoint (Lv. 6); das war ein Ablaufversäumnis des Tests, kein nachgewiesener Produktfehler. Die reguläre Wiederholung wurde aus diesem Checkpoint durchgeführt, nicht aus dem ungespeicherten Lv.-7-Zustand. Original-/Checkpoint-Saves blieben erhalten. Alle Sitzungen endeten geordnet ohne verbleibende Worker oder Sitzungssperre.

## Zweite Auffälligkeit: Begleiter

Der vermutete dauerhafte Begleiterverlust bestätigte sich bei der Nachuntersuchung nicht. Nach dem Laden des unveränderten alten Checkpoints wurden beide Chelast beim Weitergehen wieder sichtbar, ohne Kartenwechsel, neue Einstellung oder Reparatur. Beide folgten anschließend auch im Gras. Nach einem weiteren vollständigen gemeinsamen Wildkampf mit der **unveränderten** ROM waren sowohl eigene als auch entfernte Begleiter sichtbar.

Damit ist kein Begleiter-Codefix begründet. Der genaue Grund der zunächst fehlenden Darstellung (etwa Überdeckung/Position oder vorübergehender Zustand) ist noch nicht sicher belegt. Die frühere Vermutung eines bleibenden Verlusts ist zurückgenommen; keine allgemeine Fehlerfreiheit der Begleiterfunktion behauptet.
