# GBA-Spiele unterstützen

[English](../firered.md) · **Deutsch** · [Dokumentation](README.md)

Der aktuelle Entwicklungsquellcode kann GBA-Spiele und ROM-Hacks in der bestehenden CouchSplit-Oberfläche starten. Eine `.gba`-Datei oder ZIP mit genau einem Spiel auswählen. Geprüft werden das feste GBA-Headerbyte, die Header-Prüfsumme und eine Größe zwischen 512 Byte und 32 MiB bei normalen Modulen. Ausdrücklich markierte CouchSplit-XROM-v1-Dateien dürfen bis zu 512 MiB groß sein. Ein gültiger Header erkennt das Format; er bestätigt nicht die Funktion jedes Spiels oder Hacks. Zu große oder beschädigte Dateien werden abgewiesen. Project PM / Platin und der DS-Worker bleiben erhalten; andere DS-Spiele werden durch diese Änderung nicht freigeschaltet.

## Entwicklungsziel Feuerrot-Koop — 28. Oktober 2026

Der Entwicklungsquellstand enthält einen **experimentellen Feuerrot-Paarworker**:
zwei echte mGBA-Kerne mit getrennten Saves, eine Partneransicht auf derselben
oder direkt verbundenen Außenkarte und gemeinsame native Kämpfe, in denen jeder Mensch seine eigenen Aktionen wählt.
Der geforderte vollständige Welt-Koop ist weiterhin unvollständig. Das öffentliche
Alpha 8 bleibt unverändert. Der Testmodus unterstützt ausschließlich die exakte
unveränderte deutsche Original-ROM; andere GBA-Spiele, Sprachen und Hacks erhalten
dadurch keinen Koop.

Launcher und beide Worker zusammen bauen und den Testmodus ausdrücklich
einschalten. Das Build-Skript legt die passenden Programme gemeinsam in
`build/bin` ab:

```sh
scripts/build.sh
COUCHSPLIT_FIRERED_EXPERIMENT=1 build/bin/platinum-coop
```

Der gewählte Worker muss `FIRERED_PAIR_EXPERIMENT=1` melden. Ein Prozess verwaltet
beide Emulatorkerne und beide bestehenden IPC-Flächen; Controller werden weiterhin
nur im Launcher abgefragt. Normale GBA-Sitzungen verwenden zwei unabhängige Worker.
Im Testmodus zeigen beide Ansichten denselben nativen Kampf, während der andere
Kern pausiert. Außerhalb von Kämpfen steuert jeder Kern seinen eigenen Avatar;
der Partner wird derzeit ins Bild gezeichnet, ohne native Kollision oder Verdeckung
durch Gelände.

Unter **Einstellungen** lässt sich für den Testmodus der Ton von Spieler 1,
Spieler 2, beiden oder Stumm wählen. Die Auswahl gilt ab dem nächsten
Sitzungsstart. Im gemeinsamen Kampf wird der native Kampfton einmal abgespielt,
unabhängig davon, wer den Kampf beginnt (außer bei Stumm). Die Auswahl ist von
[SplitEmu](https://github.com/JosipFX/splitemu) inspiriert und eigenständig umgesetzt.

Beim Start einer Paar-Sitzung legt der Launcher ein geprüftes **Paar-Backup**
an, sobald beide Feuerrot-Speicherdateien vorhanden sind. Beide sollten zuerst
im Spiel speichern, die Sitzung beenden und erneut starten, um ein neues Paar
anzulegen. Gesichert werden die letzten einzelnen Ingame-Spielstände, kein
gleichzeitiger Emulator-Savestate: Sie können daher unterschiedliche Spielmomente
enthalten. Unter **Spielstände** wählt ihr **Beide Spieler gemeinsam
wiederherstellen** und den gewünschten Paar-Backupordner. Zuvor werden beide
vorhandenen Spielstände gesichert. Nach einem Programmabbruch während der
Wiederherstellung versucht die nächste Sitzung zuerst, das Paar anhand eines
Journals zurückzusetzen. Sind die Wiederherstellungsdaten nicht lesbar, wird
die Sitzung blockiert. Beschädigte gewählte Backups werden vor dem Austausch
der Dateien abgelehnt. Stromausfall und Hardwarefehler wurden nicht getestet.

Passen aktive Pokémon und lebende EP-Teiler-Träger nicht in die sechs nativen
Kampfplätze, stoppt die Sitzung vor der Teamumwandlung mit einem Hinweis, wie
man mit weniger EP-Teilern erneut starten kann. Nicht gespeicherter Fortschritt
seit dem letzten Ingame-Speichern kann verloren sein. Andere Workerfehler
behalten allgemeine Fehlermeldungen und Logs.

Eine gehaltene Enigmabeere wird vor der Teamumwandlung ebenfalls abgelehnt: Ihre
Wirkung stammt aus dem jeweiligen Spielstand; allein die Daten des Initiators
könnten sonst beim Partner zu falschen Effekten oder EP führen. Der Launcher
erklärt diesen Fall gesondert. Bis eigene Beereneffekte pro Spieler unterstützt
werden, die letzten Spielstände laden und gehaltene Enigmabeeren vor dem
gemeinsamen Kampf ablegen. Beim Sitzungsstopp kann ungespeicherter Fortschritt
verloren gehen.

Die Kampftransaktion sichert beide Maschinen und Flash-Puffer vollständig. Aktionen,
Wechsel und Attackenlernen gehen an den zuständigen Menschen. Traineridentität,
Ordeneffekte, native KP/AP/Status/EP, Gegenstände, Fänge, PC und Pokédex werden den
eigenen Spielern zugeordnet. Native Geldanzeigen zeigen den Anteil des Initiators.
Trainerpreisgeld und Zahltag werden gleich geteilt;
eine ungerade Einheit erhält der Initiator. Geldobergrenzen gelten getrennt. Bei
Niederlagen gilt die eigene native Verlustberechnung. Ein besiegter Spieler kann
sich erholen, während sein Partner den Kampf beendet. Kombinierte Teams werden
nicht in Batteriespielstände geschrieben. Beenden während eines Kampfes stellt
die vollständige Ausgangstransaktion vor dem geordneten Stopp wieder her.

Private Tests mit der echten ROM prüfen normale wilde und Trainerkämpfe, Zahltag,
unterschiedliche Traineridentitäten, Spieler 2 als Initiator, Erschöpfung,
Niederlage, Fänge bei vollen PCs, native Kapazitätsablehnung, zwei ausgewählte
Bälle sowie je eigenes Ingame-Speichern mit frischem Laden allein aus dem
Batteriespeicher. Dabei werden eigene Testteams/-begegnungen und virtuelle Eingaben
verwendet; das bestätigt weder eine Kampagne noch echte Controller. Getrennte
Tests des Produktions-Session-/Workerpfads prüfen Start, beide Bilder, Eingaben,
Neustart, Prozessausfall sowie Sperren und Erhalt vorhandener Saves.

## Kampagnensynchronisierung — 29. Oktober 2026

Der ausdrücklich aktivierte Entwicklungs-Worker teilt jetzt geprüfte
Kampagnen-Meilensteine über Labor/Paket/Pokédex/Rocko hinaus: die weiteren sieben
Arenen mit eigenen Orden/TMs, Bill und das Schiffsticket, wichtige VMs und
Schlüsselgegenstände, Turm/Pokéflöte/Relaxo, die Silph-Befreiung sowie die
Sevii-Rubin-/Saphir-Aufgabe. Starter, Geschenk-Pokémon, Fossilwahl, gewöhnliche
Fundstücke, Pokédex-Inhalte, Team, PC, Geld und Einkäufe bleiben persönlich.
Ein voller Beutel verschiebt die eigene Belohnung, ohne einen Empfang vorzutäuschen.

Übergaben warten, bis native Skripte, Menüs und Bewegung das Feld freigeben.
Vor Änderungen werden beide Maschinen und Flash-Puffer gesichert. Bereits geladene
NPCs, Türen und Kollisionen werden über die originalen Kartenskripte und die native
Skript-Engine aktualisiert. Nicht unterstützte ausgeführte Kartenoperationen
verschieben die gesamte zugehörige Änderung. Temporäre Rätsel und Liga-Durchläufe
werden nicht als dauerhafte Story-Schalter vereinigt. Ein gemeinsamer Top-Vier-Sieg
öffnet den eigenen aktuellen Ausgang des Partners; die native Niederlagenbehandlung
setzt den Liga-Durchlauf weiterhin regulär zurück.

Eine wartende Begegnung lässt dem Partner Zeit für Menü oder Zwischenszene.
Gleichzeitige Zufallsbegegnungen im Gras werden ein gemeinsamer Kampf; verschiedene
Skriptobjekte behalten ihre eigenen Fortsetzungen und werden nacheinander behandelt.
Auch der erste Laborrivale wartet, bis beide teilnehmen können. Skript-Wildkämpfe,
das enthüllte Knogga und originale legendäre Begegnungen behalten ihre nativen
Kampfregeln und Rückkehrskripte. Safari-, Link- und Einrichtungskämpfe bleiben eigene
native Modi.

Beide Champion-Szenen können sich am nativen Kampfstart treffen, auch bei
unterschiedlichen Startern. Anschließend laufen die eigenen Abschluss-Szenen weiter;
jeder erhält eigene native Ruhmeshallen-Sektoren, Bänder und Erstabschlussdaten.
Ein untätiger Partner bekommt den originalen Ruhmeshallen-Einstieg. Nationaldex und
Saphir-Netzwerkabschluss verwenden die nativen Engine-Funktionen. Ein bloßer
Spielabschluss-Schalter, Pokédex-Inhalte oder fremde Wander-Pokémon werden nicht
kopiert. Jeder Starter initialisiert sein eigenes natives Wander-Pokémon genau einmal.

Private Tests mit der exakten ROM prüfen alle sieben weiteren originalen
Arenaleiter-Dialoge, Trainerteams, Belohnungsskripte und getrenntes natives Speichern/
frisches Weiter. Major Bobs Strahlen und Pyros Türen umfassen echte Kollisionstests.
Hinzu kommen Zerschneider/Surfer/Stärke, das Knogga-Koordinatenskript, beide
Champion-Fortsetzungen, Ruhmeshalle, Nationaldex und Netzwerkabschluss für alle drei
Starter. Karten, Voraussetzungen und starke Teams sind ausdrücklich vorbereitete
Testdaten. Das ist **kein durchgehender natürlicher Spieldurchlauf und kein
Kampagnennachweis über die Produktionsoberfläche**.
[Prüfumfang](../verification/firered-campaign-2026-10-29.json).
Der frühere [27-Szenarien-Bericht](../verification/firered-paired-2026-10-29.json)
behält seinen datierten Umfang. Die öffentliche Alpha 8 bleibt unverändert.

### Weitere Launcher-Laufzeitarbeiten — 29. Oktober 2026

Ein früherer Launcher-Test zeigte ein einzelnes wildes Pokémon als zwei aktive
Gegner; die Ballwurfmeldung nannte außerdem den Kampfinitiator statt des
werfenden Gastes. Bei der Nachprüfung trat die Verdopplung auch bei einem
Trainer mit nur einem Pokémon auf. Der experimentelle Worker unterdrückt nun
den zusätzlichen Gegner beim Kampfbeginn und nach Menüwechseln und nennt beim
Ballwurf den tatsächlich handelnden Spieler. Gezielt geprüft wurden unter Linux
beide Kampfinitiatoren, mehrere Runden, Beutel und eigene Teammenüs, Fänge sowie
Trainer- und Wildkämpfe. Ein normal ausgelöster Launcher-Kampf mit bestehenden
Teams umfasste außerdem einen Fang durch Spieler 2, natives Speichern und
Neustart. Der [Nachprüfungsbericht](../verification/firered-single-opponent-fix-2026-10-29.json)
nennt die genauen Nachweise und Grenzen. Der frühere
[Testbericht](../verification/firered-launcher-journey-2026-10-29.json) hält die
damals noch offenen Fehler historisch fest.

Ein weiterer Fehler verhinderte die normale Flucht aus einem
umgewandelten Wildkampf. Der Worker lässt nun für beide Spieler die originale
Geschwindigkeits- und Zufallsberechnung zu; ein Fluchtversuch kann regulär
weiterhin scheitern. Native Zwei-Kern-Tests decken beide Kampfinitiatoren, einen
alleinigen Fluchtversuch von Spieler 2, getrennte Beutel und Speicherstände ab;
zusätzlich wurde der Launcher mit echten Spieleingaben und virtuellen Controllern
geprüft.

Beide Spieler erreichten und gewannen außerdem den frühen Rivalenkampf auf
Route 22 auf normalem Weg. Die 144 Pokédollar wurden mit je 72 aufgeteilt. Da
der zweite Spieler zunächst die Ausgangsszene behielt, teilt der Worker diesen
Abschluss nun erst nach dem nativen Siegbeleg und der abgeschlossenen Szene.
Dabei erhält jeder seine zum eigenen Starter passende Rivalenvariante. Echte
Spielstände nach dem Kampf und frisches Weiter bestätigen den Fortschritt sowie
unveränderte persönliche Gegenstände und Geldbeträge. Nach einem Neustart lief
Spieler 2 über das ursprüngliche Auslösefeld, ohne die Rivalenszene erneut zu
starten.

Der Linux-Launcher wurde mit zwei virtuellen Controllern und getrennten
Testprofilen aus öffentlichen englischen/japanischen Spielständen auf der exakten
deutschen ROM geprüft. Beobachtet wurden der Übergang zwischen Route 1 und
Vertania, gegenseitige Sichtbarkeit in einem Gebäude, eigene Teammenüs, ein
gemeinsamer Wildkampfsieg, ein Fang durch Spieler 2 sowie getrenntes Speichern
und Launcher-Neustarts. In einem weiteren Profilpaar blieben acht synchronisierte
Orden nach beidseitigem Speichern und Neustart erhalten. Die importierten
fremdsprachigen Teststände belegen keine allgemeine Sprachkompatibilität.
[Prüfbericht](../verification/firered-launcher-journey-2026-10-29.json).
Echte Controller und native Tests auf Windows/Bazzite stehen noch aus.
[Bazzite-Testanleitung aus dem Quellcode](bazzite-firered-test.md).

[Tests mit öffentlichen Spielständen und Rückblick-Korrektur](development-status.md#feuerrot-tests-mit-öffentlichen-spielständen-und-rückblick-korrektur--29-oktober-2026)
ergänzen gezielte Prüfungen mit vorhandenen Teams und unterschiedlichen Spielständen.
Die ursprünglichen Sprachen und vorbereiteten Kartenpositionen sind dort angegeben.

[Datierter Prüfumfang](../verification/firered-launcher-runtime-2026-10-29.json):
20 native Tests, 60 Python-Tests ohne Auslassungen und 50 gezielte ROM-Fälle
bestehen. Die Voraussetzungen sind vorbereitete Testdaten; das ist kein
durchgehender natürlicher Kampagnendurchlauf.

Das native Wechselmenü zeigt jetzt jeder Person die eigenen sechs Pokémon.
Eine ungeladene Reserve darf einen ungebundenen eigenen Bankplatz ersetzen,
auch wenn dessen Pokémon noch lebt. Dessen Daten bleiben im Eigentumsjournal.
Kampfteam/Indizes/Reihenfolge werden vor dem nativen Wiederaufbau wiederhergestellt,
auch bei Abbruch und Rücksetzung. Lebende Teilnehmer mit ausstehenden EP,
geladene EP-Teiler-Träger, Wunschtraum-Ziele, vorgemerkte Wechsel und ausstehende
Entwicklungen bleiben geschützt. Vor einem gemeinsamen Kampf berücksichtigt der
Worker jetzt lebende EP-Teiler-Träger aus den vollständigen ursprünglichen Teams,
auch auf späteren Plätzen. Native EP/EV, Attackenlernen und Entwicklung wurden
mit vorbereiteten Zwei-Kern-Fällen für beide Kampfinitiatoren geprüft. Passen die
beiden aktiven Pokémon und alle berechtigten Träger nicht in sechs native Plätze,
stoppt die experimentelle Sitzung vor der Teamumwandlung; die zuletzt geprüften
Speicherdateien bleiben erhalten. Vollständig gebundene Arbeitsplätze können
weiterhin eine Rücksetzung verlangen; nicht alle Lern- und Entwicklungspfade der
zwölf Pokémon sind abgedeckt. [Genauer Prüfumfang](../verification/firered-exp-share-2026-10-29.json).

Die ROM-Datei bleibt unverändert. Der Paar-Worker zeigt Partner jetzt auch über
beidseitige native Verbindungen angrenzender Außenkarten an, einschließlich
versetzter Koordinaten. Die Darstellung liest nur Spieldaten; gemeinsame Kämpfe
benötigen weiterhin dieselbe Karte. Partnerkollision und Geländeverdeckung bleiben
offen. ROM-Prüfungen decken beide Ansichten an Alabastia/Route 1 und Route 1/Vertania,
einen normalen Übergang zu Fuß sowie getrenntes Speichern/frisches Weiter ab.

Geprüfte Rocket-Fortsetzungskämpfe teilen jetzt die Türwächter vor Giovanni im
UG4 und die Rüpel im 7. Stock des Turms nach deren originalem Verschwinden.
Lostelles Rettung wartet bei vollem Beerenbeutel nicht mehr auf das optionale
Geschenk; die originale Voll-Meldung erlaubt den Story-Fortschritt. Später wird
keine Ersatzbeere erfunden. Echte Kämpfe/Fortsetzungsszenen prüfen die geladenen
Türen/NPCs und gespeicherten Fortschritte beider Personen.

Die Kartenaktualisierung erhält eigenes Eiskaskadenhöhlen-Eis, die bestehende
Seeschaum-Kartenvariante und Loreleis eigene Puppensammlung. Die Tanoby-Kammern
verwenden ihre originale Beleuchtungsänderung. Elf native Kartenfälle prüfen
echtes Speichern und frisches Weiter. Neue Seeschaum-Varianten warten weiterhin
auf normalen Karteneintritt; optionale Link-/Spezialmodus-Helfer sind nicht abgedeckt.

**Vor vollständiger Koop-Freigabe noch nötig:** ein durchgehender Zwei-Spieler-Lauf
vom Anfang durch Hauptspiel und Nachspiel, verbleibende nicht unterstützte
Kartenoperationen, vollständiger EP-Teiler und alle Lern-/
Entwicklungspfade, Partnerkollision/Geländeverdeckung, Spezialmodi, lange Sitzungen,
echte Controller, hörbarer Ton und native Windows-/Bazzite-Abnahme. Das native
Kampfteam lädt normalerweise bis zu drei einsatzfähige Pokémon je Person; für
EP-Teiler-Träger können mehr Plätze an eine Person gehen. Sicheres
Nachladen und das eigene Wechselmenü können das ganze eigene Team unter den
oben genannten Referenzgrenzen verwenden. Geschützte native Referenzen können weiterhin
Rücksetzung verlangen. Identische PID-/OT-Identitäten beider Profile werden
abgewiesen. Der Modus bleibt experimentell.

ROM-abhängige Werkzeuge sind von normalem Build, Installation und CTest ausgeschlossen.
ROM, Saves und rohe Zustände werden nicht verteilt. Benötigt werden die eigene
exakte ROM, ein privater initialisierter Oberweltzustand und ein neuer Ausgabeordner:

Die folgende Testreihe bündelt die Prüfungen. `quick` führt eine kleinere
Auswahl aus; `full` prüft unter anderem beide Initiatoren, Fänge, Wechsel, EP,
Arenen, Kampagnenstellen, Komponenten und Launcher. Die optionalen Saves müssen
die unten im Prüfbericht bezeichneten Dateien mit null beziehungsweise acht
Orden sein: Einzelne Szenarien setzen deren konkrete Teams und Fortschritte
voraus. Ohne beide Saves werden ihre Szenarien ausdrücklich übersprungen. Der
Prozessabbruchtest des Produktions-Launchers läuft nur unter Linux. Das Skript
baut die Testprogramme, vergleicht die Eingabedateien vor und nach dem Lauf,
verwendet einen privaten temporären Ordner und erstellt den Bericht nur unter
einem neuen Dateinamen. Fehlerdiagnosen können private Spieldetails enthalten;
den Bericht vor dem Teilen prüfen.
Mit `--jobs 2` (höchstens acht) laufen unabhängige native Szenarien parallel.
Für gezielte Wiederholungen wählen `--case pair/capture --case gyms/misty` nur
die genannten nativen Szenarien sowie Komponenten- und Python-Tests aus.

```sh
python3 scripts/test-firered-matrix.py --profile full --rom ORIGINAL_DE.gba \
  --checkpoint PRIVATER_STARTER_ZUSTAND --save-early SAVE_0_ORDEN.sav \
  --save-late SAVE_8_ORDEN.sav --build-dir build/launcher --report ergebnis.json
```

Die Voraussetzungen vieler Szenarien werden vorbereitet. Ein erfolgreicher Lauf
belegt weder eine natürlich durchgespielte Kampagne noch echte Controller. Bei
Kapazitätsüberschreitung und Enigmabeere wird die sichere Ablehnung geprüft,
nicht deren volle Unterstützung.
Der [Matrixbericht vom 29. Oktober](../verification/firered-matrix-2026-10-29.json)
hält 94 bestandene Prüfgruppen unter Linux fest, darunter Komponenten- und
Python-Tests, zwei unabhängige öffentliche Saves und ein aus dem frühen Save
erzeugter Ausgangszustand. Die Szenarien bereiten Teile des Spielzustands vor;
ein natürlicher Gesamtdurchlauf, echte Controller und native Windows-/Bazzite-
Abnahmen sind nicht belegt.

Ein privater Ausgangszustand lässt sich aus einem geeigneten 128-KiB-Save in
einem isolierten Emulator erzeugen. ROM und Quell-Save bleiben unverändert;
der erzeugte Zustand enthält private Spieldaten und gehört nicht ins Repository:

```sh
cmake --build build/launcher --target create-firered-checkpoint
build/launcher/create-firered-checkpoint ORIGINAL_DE.gba SAVE_0_ORDEN.sav PRIVATER_STARTER_ZUSTAND
```

Der Save muss eine ruhige Oberwelt mit einem Team erreichen. Andere Saves
können andere Situationen ergeben, sodass einzelne Prüfungen fehlschlagen können.

Einzelprüfungen können weiterhin direkt gestartet werden:

```sh
cmake --build build/launcher --target verify-firered-pair
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATER_OBERWELTZUSTAND NEUER_AUSGABEORDNER wild
cmake --build build/launcher --target verify-firered-campaign verify-firered-gyms
build/launcher/verify-firered-campaign ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUER_KAMPAGNEN_ORDNER shared-champion-rendezvous
build/launcher/verify-firered-gyms ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUER_ARENEN_ORDNER surge
cmake --build build/launcher --target verify-firered-connections verify-firered-progression verify-firered-map-setup
build/launcher/verify-firered-connections ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUER_VERBINDUNGS_ORDNER
build/launcher/verify-firered-progression ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUER_STORYTOR_ORDNER lostelle-full-berry
build/launcher/verify-firered-map-setup ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUER_KARTEN_ORDNER
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUER_MENUE_ORDNER party-menu-cancel
cmake --build build/launcher --target verify-firered-story
build/launcher/verify-firered-story ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND WEITERER_NEUER_ORDNER brock-full
python3 tests/smoke_firered_pair_worker.py build/launcher/gba/gba-worker ORIGINAL_DE.gba
python3 tests/smoke_firered_pair_session.py build/launcher/verify-gba build/launcher/gba/gba-worker ORIGINAL_DE.gba
```

Die Spielstandsprüfungen verwenden separat bereitgestellte 128-KiB-Saves und
ändern diese nicht. Diese Beispiele nutzen die zwei im
[Prüfbericht](../verification/firered-public-saves-2026-10-29.json) verlinkten
Dateien: Für `questlog` steht der Spielstand mit acht Orden zuerst, für `brock`
der Spielstand ohne Orden. Rocko nutzt die zweite Attacke des originalen Pokémon.

```sh
cmake --build build/launcher --target verify-firered-public-saves verify-firered-public-pair
build/launcher/verify-firered-public-saves ORIGINAL_DE.gba SAVE_1.sav SAVE_2.sav
build/launcher/verify-firered-public-pair ORIGINAL_DE.gba SAVE_8_ORDEN.sav SAVE_0_ORDEN.sav questlog
build/launcher/verify-firered-public-pair ORIGINAL_DE.gba SAVE_0_ORDEN.sav SAVE_8_ORDEN.sav brock
```

Das frühere [Einzelkern-Eingabeexperiment](../verification/firered-team-input-2026-10-28.json)
belegt seinen historischen Umfang. Es ist kein Testbericht der Paarimplementierung.

## Aktueller Splitscreen-Modus

Nach dem Zuordnen zweier Controller **Splitscreen starten** wählen. Jeder Spieler erhält einen Bildschirm mit 240×160 Pixeln, eigene Eingaben/Audio und einen eigenen Spielstand. Die Menüaufteilung bleibt erhalten, Texte und Bedienhinweise passen sich dem Spiel an. Es gibt kein zusätzliches Reisemenü und kein zusätzliches Emulatorfeld in den Einstellungen. Gemeinsame Spielwelt, GBA-Linktausch/-kämpfe, gemeinsame Pokémon-Bank und Regionsreisen werden durch diesen Worker nicht umgesetzt.

Im Spiel speichern und das Ende der Speicheranzeige abwarten. Start + Select für zwei Sekunden beendet die Sitzung. Der Worker sichert geänderte Speicherdaten alle 60 Frames und beim geordneten Beenden atomar. Unterstützt sind EEPROM mit 512 Byte / 8 KiB, SRAM mit 32 KiB und Flash/SRAM mit 64 KiB / Flash mit 128 KiB. Spiele ohne Batteriespeicher erzeugen keinen leeren Spielstand. Ungültige Größen und erkannte Größenkonflikte werden ohne Überschreiben des vorhandenen Stands abgewiesen. Die Dateisicherung ersetzt nicht das Speichern im Spiel. Spielstände und geprüfte Backups bleiben nach Spieler und exaktem ROM-SHA-256 getrennt. Andere Builds oder Sprachen übernehmen keine Spielstände automatisch.

Der Launcher wählt den mitgelieferten `gba-worker` automatisch. Das vorhandene DS-Emulatorfeld und die BIOS-Einstellungen bleiben unverändert und betreffen GBA nicht. mGBA nutzt seine Software-BIOS-Implementierung; ein externes GBA-BIOS ist nicht erforderlich. Select + R wird als normale GBA-Eingabe weitergegeben, weil GBA keinen zweiten Bildschirm hat. Controller-Zuordnung, Trennungsverhalten und Beenden-Tastenkombination bleiben erhalten.

Build: `scripts/build.sh`; mGBA 0.10.5 ist in `config/mgba.lock.json` festgelegt. Quellpakete enthalten die festgelegte mGBA-Quelle und MPL-2.0-Lizenzen unter `gba-corresponding-source` sowie die nachvollziehbare Erweiterung `patches/mgba-xrom.patch`. CMake wendet diese auf eine separate Buildkopie an; die festgelegte Originalquelle bleibt unverändert. Offline ist `-DCOUCHSPLIT_MGBA_SOURCE=/pfad/zu/gba-corresponding-source` möglich. IPC-ABI bleibt 2. Die veröffentlichten Alpha-8-Pakete und die frühere reine Feuerrot-Vorschau enthalten diese allgemeine Erweiterung nicht. Native Windows-/Bazzite-Ausführung dieser Änderung ist noch nicht geprüft.

Produktions-Worker ohne kommerzielle Spielinhalte prüfen:

```sh
python3 tests/smoke_gba.py build/bin/gba-worker
```

Das private Werkzeug `build/launcher/verify-gba` startet eine separat bereitgestellte GBA-ROM mit echtem Session/PlayView und virtuellen SDL-Controllern. Eigene ARM-Testprogramme prüfen Speicherchips und Wiederladen; echtes deutsches Feuerrot wird getrennt geprüft. Dies belegt weder physische Controller noch einen vollständigen Durchlauf jedes Spiels.

## Große Reise-ROMs

[CouchSplit XROM v1](extended-rom.md) erhält eine einzige `.gba`-Datei und dieselbe Launcher-Oberfläche. Unveränderliche Daten hinter den nativen 32 MiB sind über einen zusätzlichen Emulator-Adressbereich erreichbar. Das eigene Format benötigt den passenden Worker; beliebige GBA-Emulatoren oder echte Module unterstützen es nicht automatisch. Die sieben reservierten Headerbytes ab 0xB5 enthalten `CSXROM1`, die Header-Prüfsumme stimmt, die Datei ist auf vier Byte ausgerichtet und größer als 32 MiB. Unbekannte übergroße ROMs werden weiter abgewiesen. Ein größerer Container ergänzt keine fehlenden Handlungskapitel.
