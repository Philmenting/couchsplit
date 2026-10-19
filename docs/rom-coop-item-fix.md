# PM 0.4.5: Koop-KP-Abgleich nach Tränken

Untersuchung vom 19.10.2026 mit dem ausgelieferten Alpha-5-Launcher und der bereitgestellten deutschen PM-ROM mit Zielfix 1. **Koop-Kampffix 1** ist eine separate ROM-Korrektur, kein neuer offizieller Project-PM-Release und kein neues Launcher-Paket.

## Reproduzierter Fehler

Zwei echte Emulatorprozesse konnten nach einem gemeinsamen Trainerkampfzug verschiedene KP zeigen. Der gezielte Gegencheck mit der bisherigen Zielfix-1-ROM bestätigte dies auch ohne den ersten experimentellen Korrekturversuch: Nach Absorber und einem Trank von Spieler 2 hatte Spieler 1 auf seinem eigenen System 47 KP, auf dem anderen System 48 KP. Beide Kampfcontroller standen bereits im Eingabemenü derselben Runde. Die TCP-Verbindung und die übertragenen Daten waren weiterhin vorhanden.

Der genaue Mitschnitt zeigte die Ursache im ROM-Ablauf: Der Besitzer wendet einen Trank schon im lokalen Beutelmenü an. Der Partner führt stattdessen das zusätzliche Item-Unterprogramm 305 aus. Dessen Aufruf des KP-Unterprogramms erzeugt einen **zusätzlichen Netzwerk-KP-Abgleich nur auf der Partnerseite**. Im reproduzierten Fall sprang deren KP-Sequenz von 5 auf 6 für die Heilung und anschließend auf 7 für den nächsten Treffer; die andere Seite blieb bei 5. Die weiteren Treffer- und KP-Abgleiche passten dadurch nicht mehr zusammen. Ein pauschales Vorabkopieren bereits geheilter Partner-KP wäre ebenfalls falsch, weil die native Item-Verarbeitung erneut heilt.

Die konkreten PM-Programmstellen und Unterprogramme wurden an der hashgeprüften Nutzer-ROM untersucht. Die unveränderten PRET-Skripte allein beschreiben diese zusätzlichen PM-Abläufe nicht; vollständige Quellen des ROM-Mods liegen weiterhin nicht vor.

## Korrektur

`romfixes/pm045_items.py` schreibt ausschließlich selbst verfassten Thumb-1-Code in zwei Bereiche des unkomprimierten Overlays 122:

- `0x02294af8..0x02294b67`: Die ursprüngliche KP-Importprüfung wird platzsparend neu geschrieben. Ihre Regeln einschließlich Item-Ausschluss, positiver KP, Speziesprüfung, Resync-Vorrang und Begrenzung auf maximale KP bleiben erhalten. Der gewonnene Platz enthält den neuen Filter.
- `0x02295f60..0x02295f63`: Der bestehende Aufruf der KP-Sync-Freigabe führt zunächst durch diesen Filter.

Nur der direkte KP-Unteraufruf des Partner-Itemskripts wird ausgefiltert: Script-Stacktiefe 1, übergeordnetes Script-NARC 1, übergeordnetes Script 305. Damit entfällt genau der einseitige Zusatzabgleich. Für andere Aufrufe wird die bestehende Freigabefunktion mit ihren Prüfungen weiter aufgerufen. Die native Heilung, der Beutelverbrauch und die Animation bleiben im Spiel selbst. Die Änderung schreibt keine Geldbeträge oder Spielstanddaten.

Die eigene Assembly steht in `patches/pm045-item-hp.s`. Overlaygröße, BSS, Header, FAT, vorhandene Literalwerte und alle anderen ROM-Dateien bleiben erhalten. Auch der frühere Zielauswahl-Fix bleibt enthalten. Beide Spieler müssen dieselbe korrigierte ROM verwenden.

## Anwenden

Python 3, ohne zusätzliche Laufzeitpakete:

```text
python scripts/fix-pm-coop-items.py --rom Pokemon-Platinum-PM-DE-Zielfix1.nds --output Pokemon-Platinum-PM-DE-Koopfix1.nds
```

Akzeptiert wird ausschließlich die unten genannte Zielfix-1-Eingabe. Vollständige ROM-, Overlay- und Ergebnisprüfsummen sind verpflichtend. Unbekannte oder bereits geänderte Eingaben, derselbe Quell-/Zielpfad und vorhandene Ausgabedateien werden abgewiesen. Das Werkzeug lädt keine Spieldaten herunter und ersetzt keine vorhandenen Dateien.

Die neue ROM im Koop-Starter hinzufügen und als vorbereitet bestätigen. **Ein neuer ROM-Hash verwendet eigene Spielstandverzeichnisse.** Bestehende Spielstände werden nicht automatisch übertragen. Die Nachtests kopieren ausdrücklich eigene Checkpoints in isolierte Testprofile; daraus folgt keine automatische Migration von Benutzerprofilen.

| Datei | SHA-256 |
|---|---|
| Eingabe: Deutsch Alpha 2 + Zielfix 1 | `dfe675837c5b935a684eb2e18c446dbc7865ba45bcb2343d2158892f6b720520` |
| Ausgabe: zusätzlich Koop-Kampffix 1 | `e90a53f4dd7c24d45e74c1854cfd5cb17fb4dc636179350fea6df8b9bfbd4a7f` |

Beide Dateien haben 141.962.768 Byte. Es werden nur die beiden beschriebenen Codebereiche mit insgesamt 116 Byte Länge beschrieben.

## Prüfung und Grenzen

Lokal bestehen alle 55 Python-Tests mit echtem xdelta-Decoder und ARM-Engine ohne übersprungene Tests. Die neue ROM-Prüfung führt die tatsächlichen eigenen Instruktionen in mindestens 560 systematisch variierten Zuständen aus: 320 KP-Importfälle und 240 Filterfälle für beide Spieler-Slots. Sie prüft außerdem den tatsächlichen BL-Aufruf samt Rückkehr, Register-/Stackerhaltung, ausschließlich erlaubte Schreibzugriffe, unveränderte native Item-Heilungszuständigkeit und die Folge-Abgleichsequenz. Die bestehenden 128 ARM-Zustände der Zielauswahl bleiben geprüft. Eine separate Windows-/Linux-CI führt alle ROM-Tests mit verpflichtender ARM-Abhängigkeit aus; dafür werden keine Nintendo-Daten benötigt.

Ein unabhängiger privater Assembler und das veröffentlichbare Standardbibliothek-Werkzeug erzeugen dieselbe ROM. Auch die gespeicherte Assembly ergibt exakt die eingebetteten Codebytes. Der aktive erste Nachtest mit zwei echten Linux-Emulatoren bestätigt Absorber sowie Tränke beider Spieler nacheinander mit anschließend übereinstimmenden KP, Statuswerten, PP und Kampfwertänderungen. Die identifizierten Filterkontexte wurden auf beiden Seiten im echten Ablauf beobachtet; die KP-Sequenzen blieben gleich. Ein zusätzlicher Test mit gleichzeitigen Tränken bestätigt die nicht durch maximale KP begrenzte Heilung 42 → 62 und 40 → 60. Der folgende gegnerische Treffer führte beiderseits zu 62/59 KP; beide KP-Sequenzen standen auf 1. Die übrigen Kampfwerte stimmten ebenfalls überein. [Maschinenlesbarer Prüfstand](verification/coop-item-sync-2026-10-19.json).

Die Gameplay-Tests verwenden für gezielte, überlebbare Trainerkampfszenarien getrennte Spielstandkopien mit vorbereitetem Level-25-Team. Bei dieser Vorbereitung wurden Geld, Beutel, Geschichte, Trainer-Siegmarkierungen und Positionen nicht bearbeitet. Das ist ein kontrollierter Funktionstest, kein regulär erspielter Level-25-Durchlauf. Die Geldprüfung mit der bisherigen ROM bestätigte im echten gemeinsamen Sieg über Sven +96 ₽ für jeden Spieler, unveränderte Kontostandsdifferenz und Erhalt beider Beträge/Siegmarkierungen nach Speichern und Neuladen. Beide eigenen Begleiter waren unmittelbar nach dem Sieg vor einer Bewegung sichtbar; dafür wurde kein Code geändert.

Die ROM-Korrektur gilt für dieselbe ROM auf allen Plattformen. Tatsächliche Pokémon-Spielnachweise dieser Prüfung stammen von Linux mit virtuellen SDL-Controllern. Physische Controller, natives Windows-/Bazzite-Gameplay und ein vollständiger Durchlauf bleiben getrennte Abnahmen. Die unveränderten Alpha-5-Launcher benötigen für diese ROM-Änderung keinen Neubau.
