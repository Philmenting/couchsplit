# GBA-Spiele unterstützen

[English](../firered.md) · **Deutsch** · [Dokumentation](README.md)

Alpha 10 und der aktuelle Quellcode können GBA-Spiele und ROM-Hacks in der bestehenden CouchSplit-Oberfläche starten. Eine `.gba`-Datei oder ZIP mit genau einem Spiel auswählen. Geprüft werden das feste GBA-Headerbyte, die Header-Prüfsumme und eine Größe zwischen 512 Byte und 32 MiB bei normalen Modulen. Ausdrücklich markierte CouchSplit-XROM-v1-Dateien dürfen bis zu 512 MiB groß sein. Ein gültiger Header erkennt das Format; er bestätigt nicht die Funktion jedes Spiels oder Hacks. Zu große oder beschädigte Dateien werden abgewiesen. Project PM / Platin und der DS-Worker bleiben erhalten; andere DS-Spiele werden durch diese Änderung nicht freigeschaltet.

## Feuerrot-Koop — Alpha 10

Alpha 10 enthält einen **experimentellen Feuerrot-Paarworker**:
zwei echte mGBA-Kerne mit getrennten Saves, eine Partneransicht auf derselben
oder direkt verbundenen Außenkarte und gemeinsame native Kämpfe, in denen jeder Mensch seine eigenen Aktionen wählt.
Der vollständige Welt-Koop ist weiterhin nicht abgeschlossen. Die älteren
Alpha-8-Pakete bleiben unverändert. Der Testmodus unterstützt ausschließlich die exakte
unveränderte deutsche Original-ROM; andere GBA-Spiele, Sprachen und Hacks erhalten
dadurch keinen Koop.

Alpha 10 installieren und den Modus unter **Einstellungen → Feuerrot-Koop
(experimentell)** einschalten. Für einen Quellcode-Build Launcher und beide
Worker gemeinsam bauen; das Skript legt sie in `build/bin` ab:

```sh
COOP_BUILD_JOBS=1 scripts/build.sh
build/bin/platinum-coop
```

Die Auswahl wird gespeichert und gilt ab der nächsten Sitzung. Die Einstellungen
zeigen, ob die ausgewählte ROM die unterstützte deutsche Original-ROM ist.
Andere GBA-Spiele behalten ihre getrennten Sitzungen. Die bisherige Umgebungsvariable
`COUCHSPLIT_FIRERED_EXPERIMENT=1` dient nur als Vorgabe, wenn noch keine Moduswahl
gespeichert wurde; eine ausdrücklich gespeicherte Auswahl hat Vorrang.

Der gewählte Worker muss `FIRERED_PAIR_EXPERIMENT=1` und `FIRERED_RECOVERY=1` melden. Ein Prozess verwaltet
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

Eine gehaltene Enigmabeere in einem der Teams lässt **beide Spiele vor der
Teamumwandlung pausieren**. Laufender Fortschritt, eigene Teams und Flash-Puffer
bleiben erhalten. **Einzelkämpfe fortsetzen** bestätigt ihr per Maus, Tastatur
oder Controller-A von Spieler 1. Die ursprünglichen Begegnungen und Szenen laufen
danach mit eigenen Teams und nativen Belohnungen weiter. Sobald beide in einer
ruhenden Spielwelt sind, geht Koop weiter. Die Enigmabeere danach ablegen, um
wieder gemeinsam zu kämpfen. Es gibt keinen automatischen Einzelkampfwechsel.

EP-Teiler lösen die bisherige Sechs-Plätze-Pause nicht mehr aus. Alpha 9 verwaltet
Teilnahme und Gegenstandsträger für alle zwölf eigenen Pokémon, lädt Empfänger
gezielt in das native Arbeitsteam und lässt die originale Engine EP/EV vergeben,
Attacken lernen und Entwicklungen ausführen. Eigene Enigmabeereneffekte bleiben
im gemeinsamen Kampf unvollständig. Die Fortsetzung erzeugt keinen Ingame-Save:
vor dem Beenden normal speichern. Unerwartete Workerfehler stoppen weiterhin die
Sitzung. Die Statuszeile zeigt die zuständige Person, Warten auf den Partner und
Einzelkämpfe an. [Aktueller Prüfumfang](../verification/firered-alpha9-2026-11-08.json).

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

Das native Wechselmenü zeigt jeder Person die eigenen sechs Pokémon. Eine
ungeladene Reserve darf einen ungebundenen eigenen Bankplatz ersetzen, auch wenn
dessen Pokémon noch lebt. Das Journal erhält Besitz, Fortschritt und Teilnahme.
Kampfteam/Indizes/Reihenfolge werden vor dem nativen Wiederaufbau wiederhergestellt,
auch bei Abbruch und Rücksetzung. Aktive Pokémon, vorgemerkte Wechsel und
Wunschtraum-Ziele bleiben geschützt. Alpha 10 weist eine vorerst gesperrte Auswahl
vor dem Wechsel ab, zeigt einen deutschen Hinweis im Spiel und lässt die Person
im Menü eine geladene Alternative wählen oder, soweit nativ erlaubt, abbrechen.
Bei beschädigten Zuständen sowie vollständig gebundenen Zwangsersatz-/EP-Plätzen
kann weiterhin eine vollständige Kampfrücksetzung nötig sein.

Alpha 9 verteilt native EP/EV über alle zwölf ursprünglichen Teamplätze.
Vorbereitete Zwei-Kern-Tests decken zwölf EP-Teiler-Träger, zwölf ausstehende
Entwicklungen, beide Initiatoren, Attackenlernen bei störenden Eingaben der
anderen Person und Wechsel durch beide vollständigen Teams ab. Frühere
Teilnahme geht beim Auslagern nicht verloren. Entwicklung verwendet Eingaben
und Nationaldex-Freischaltung des tatsächlichen Besitzers, auch wenn diese vom
Initiator abweichen. Native Saves und frisches Weiter erhalten die Ergebnisse.
[Genauer Prüfumfang](../verification/firered-alpha9-2026-11-08.json).

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

Die Kartenaktualisierung erhält eigenes Eiskaskadenhöhlen-Eis und Loreleis Puppen.
Seeschaum UG3/UG4 können ihre nativen Wasserströmungs-Varianten ohne Kartenaustritt
anwenden, nach Prüfung von Raster, Abmessungen und Tilesets. Durch Spieler/Objekte
besetzte geänderte Felder warten sicher; persönliche Rätsel-Flags bleiben lokal.
Linkclub-/Wunderkarten-Abfragen erhalten eigene Daten, Tanoby nutzt seine originale
Beleuchtungsänderung. 32 gezielte Kartenfälle umfassen alle 19 oberen Stockwerke
der Pokémon-Center sowie natives Speichern und frisches Weiter.

**Vor vollständiger Koop-Freigabe noch nötig:** ein durchgehender Zwei-Spieler-Lauf
durch Hauptspiel und Nachspiel, verbleibende Trainerturm-/e-Reader-Kartenoperationen,
Partnerkollision/Geländeverdeckung, Spezialmodi, mehrstündige Sitzungen, echte
Controller, hörbarer Ton und Pokémon-Spielabnahme unter Windows/Bazzite.
Nincadas native Entwicklung ergänzt Ninjatom jetzt nur im freien Teamplatz des
Besitzers, mit dessen Pokédex-Eintrag. Bei vollem Team oder abgebrochener Entwicklung
entsteht keines. Ninjask und Ninjatom behalten ihre originale gemeinsame PID-/OT-
Kennung und bleiben in Folgekämpfen und Wechselmenüs getrennt zugeordnet.

Korrektur zur früheren Alpha-9-Einschränkung: Zufällige Pokérus-Ansteckung,
Verbreitung und Tagesablauf sind im originalen Feuerrot **deaktiviert**. Der Worker
erhält den aktiven/abgeklungenen Status jedes Pokémon. Der originale Spielcode
berechnet dessen EV-Bonus auch bei ausgelagerten EP-Teiler-Trägern. Die Mechanik
aus Rubin/Saphir/Smaragd wird nicht hinzugefügt. Siehe den festgelegten
[Originalcode](https://github.com/pret/pokefirered/blob/ce6406d8c2eaf46d543c8a2606de0c3cb1951982/src/pokemon.c#L5607)
und die [Alpha-10-Prüfungen](../verification/firered-alpha10-2026-11-08.json). Das Arbeitsteam lädt meist bis
zu drei einsatzfähige Pokémon je Person; das Journal ist keine unbegrenzte
Virtualisierung aller nativen Kampfmechaniken. Geschützte Referenzen können
Rücksetzung verlangen. Mehrdeutige Duplikate beider Profile bleiben abgewiesen; nur das native
Ninjask-/Ninjatom-Geschwisterpaar bildet eine gezielte Ausnahme. Der Modus bleibt experimentell.

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
Standardmäßig laufen Build, Komponenten und native Szenarien mit einem Auftrag.
Mit `--jobs 2` (höchstens acht) steigen Parallelität und CPU-Last.
Für gezielte Wiederholungen wählen `--case pair/capture --case gyms/misty` nur
die genannten nativen Szenarien sowie Komponenten- und Python-Tests aus.

```sh
python3 scripts/test-firered-matrix.py --profile full --rom ORIGINAL_DE.gba \
  --checkpoint PRIVATER_STARTER_ZUSTAND --save-early SAVE_0_ORDEN.sav \
  --save-late SAVE_8_ORDEN.sav --build-dir build/launcher --report ergebnis.json
```

Die Voraussetzungen vieler Szenarien werden vorbereitet. Ein erfolgreicher Lauf
belegt weder eine natürlich durchgespielte Kampagne noch echte Controller. Die
Überlauf-Szenarien prüfen jetzt gemeinsame EP-Vergabe bis zu zwölf Empfängern.
Enigmabeeren-Fälle prüfen die ausdrückliche Fortsetzung mit Einzelkämpfen.
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

## Pausenfunktion prüfen

Das native Szenario `exp-share-enigma` und seine Varianten `-leader2` und `-both`
prüfen unveränderte laufende Zustände/Flash, ausdrücklichen Einzelkampf, beide
nativen Saves/frisches Weiter und späteren gemeinsamen Kampfbeginn ohne Beere.
`exp-share-overflow` prüft in Alpha 9 stattdessen erfolgreichen gemeinsamen Kampf.

Für einen separaten Test mit echter Session/PlayView und Worker werden private
Route-1-Saves angelegt und ein Kampf durch virtuelle Controller-Eingaben ausgelöst:

```sh
cmake --build build/launcher --target verify-firered-pair verify-gba gba-worker
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUER_PRIVATER_ENIGMA_FIXTURE exp-share-enigma-launcher
python3 tests/smoke_firered_recovery.py build/launcher/verify-gba build/launcher/gba/gba-worker ORIGINAL_DE.gba NEUER_PRIVATER_ENIGMA_FIXTURE NEUES_PRIVATES_ENIGMA_ERGEBNIS 1
```

Das letzte Argument wählt Spieler 1 (`0`) oder Spieler 2 (`1`). Erzeugte Saves,
Screenshots und Spielprotokolle außerhalb des Repositorys aufbewahren. Der Test
hält die Pause 16 Sekunden, prüft laufende Bildmeldungen und erhaltene Saves/
Profilsperre, klickt die echte UI-Aktion und beendet den nativen Kampf bis zur
Rückkehr zu Koop. Ausgangsteams, Gegenstände und Ort sind angelegte Testdaten;
dies ist kein durchgehender Kampagnentest.
[Prüfbelege und Grenzen](../verification/firered-recovery-ui-2026-11-08.json).

## Dauertest in einer Produktionssitzung

Der private Test wiederholt natürliche Grasbegegnungen mit wechselndem Initiator,
Übergänge zwischen Alabastia und Route 1 und Ingame-Speichern in einer laufenden
Session/PlayView/Worker-Sitzung. Er prüft beide Eingabebesitzer, vollständige native
Save-Prüfsummen/Generationen, eigene Teams, PC, Geld und Kampfzähler. Erst am Ende
folgen Neustart und frisches Weiter. Vorbereitete starke Level-100-Teams machen
die Strecke wiederholbar; dies ist kein natürlicher Kampagnen- oder Hardwaretest.

```sh
cmake --build build/launcher --parallel 1 --target verify-firered-pair verify-gba gba-worker
build/launcher/verify-firered-pair ORIGINAL_DE.gba PRIVATER_STARTER_ZUSTAND NEUE_PRIVATE_DAUERTEST_SAVES soak-launcher
python3 tests/smoke_firered_soak.py build/launcher/verify-gba build/launcher/gba/gba-worker ORIGINAL_DE.gba NEUE_PRIVATE_DAUERTEST_SAVES NEUES_PRIVATES_DAUERTEST_ERGEBNIS --cycles 10 --minimum-minutes 10
```

Erzeugte Saves, Bilder und Protokolle außerhalb von Git halten; der Ausgabeordner
muss neu sein. Die Matrix baut und testet standardmäßig mit einem Auftrag zur
Zeit; dies lässt sich mit `--jobs 1` ausdrücklich wählen. Unter Linux mit
systemd-Benutzersitzung begrenzt `systemd-run --user --scope -p CPUQuota=70% nice -n 15 …` die gesamte Prozessgruppe auf 70 % eines CPU-Kerns. Dies ändert
nur die Ausführungsplanung, weder Lüftersteuerung noch Spielstand.

## Getrennter GBA-Splitscreen-Modus

Außerhalb des Feuerrot-Testmodus nach dem Zuordnen zweier Controller **Splitscreen starten** wählen. Jeder Spieler erhält einen Bildschirm mit 240×160 Pixeln, eigene Eingaben/Audio und einen eigenen Spielstand. Die Menüaufteilung bleibt erhalten, Texte und Bedienhinweise passen sich dem Spiel an. Es gibt kein zusätzliches Reisemenü und kein zusätzliches Emulatorfeld in den Einstellungen. Dieser getrennte Modus ergänzt keine gemeinsame Spielwelt, GBA-Linktausch/-kämpfe, gemeinsame Pokémon-Bank oder Regionsreisen.

Im Spiel speichern und das Ende der Speicheranzeige abwarten. Start + Select für zwei Sekunden beendet die Sitzung. Der Worker sichert geänderte Speicherdaten alle 60 Frames und beim geordneten Beenden atomar. Unterstützt sind EEPROM mit 512 Byte / 8 KiB, SRAM mit 32 KiB und Flash/SRAM mit 64 KiB / Flash mit 128 KiB. Spiele ohne Batteriespeicher erzeugen keinen leeren Spielstand. Ungültige Größen und erkannte Größenkonflikte werden ohne Überschreiben des vorhandenen Stands abgewiesen. Die Dateisicherung ersetzt nicht das Speichern im Spiel. Spielstände und geprüfte Backups bleiben nach Spieler und exaktem ROM-SHA-256 getrennt. Andere Builds oder Sprachen übernehmen keine Spielstände automatisch.

Der Launcher wählt den mitgelieferten `gba-worker` automatisch. Das vorhandene DS-Emulatorfeld und die BIOS-Einstellungen bleiben unverändert und betreffen GBA nicht. mGBA nutzt seine Software-BIOS-Implementierung; ein externes GBA-BIOS ist nicht erforderlich. Select + R wird als normale GBA-Eingabe weitergegeben, weil GBA keinen zweiten Bildschirm hat. Controller-Zuordnung, Trennungsverhalten und Beenden-Tastenkombination bleiben erhalten.

Build: `scripts/build.sh`; mGBA 0.10.5 ist in `config/mgba.lock.json` festgelegt. Quellpakete enthalten die festgelegte mGBA-Quelle und MPL-2.0-Lizenzen unter `gba-corresponding-source` sowie die nachvollziehbare Erweiterung `patches/mgba-xrom.patch`. CMake wendet diese auf eine separate Buildkopie an; die festgelegte Originalquelle bleibt unverändert. Offline ist `-DCOUCHSPLIT_MGBA_SOURCE=/pfad/zu/gba-corresponding-source` möglich. IPC-ABI bleibt 2. Alpha 10 enthält diese GBA-Erweiterung. Native Paketprüfungen sind von tatsächlicher Pokémon-Spielabnahme unter Windows/Bazzite zu unterscheiden; Letztere bleibt offen.

Produktions-Worker ohne kommerzielle Spielinhalte prüfen:

```sh
python3 tests/smoke_gba.py build/bin/gba-worker
```

Das private Werkzeug `build/launcher/verify-gba` startet eine separat bereitgestellte GBA-ROM mit echtem Session/PlayView und virtuellen SDL-Controllern. Eigene ARM-Testprogramme prüfen Speicherchips und Wiederladen; echtes deutsches Feuerrot wird getrennt geprüft. Dies belegt weder physische Controller noch einen vollständigen Durchlauf jedes Spiels.

## Große Reise-ROMs

[CouchSplit XROM v1](extended-rom.md) erhält eine einzige `.gba`-Datei und dieselbe Launcher-Oberfläche. Unveränderliche Daten hinter den nativen 32 MiB sind über einen zusätzlichen Emulator-Adressbereich erreichbar. Das eigene Format benötigt den passenden Worker; beliebige GBA-Emulatoren oder echte Module unterstützen es nicht automatisch. Die sieben reservierten Headerbytes ab 0xB5 enthalten `CSXROM1`, die Header-Prüfsumme stimmt, die Datei ist auf vier Byte ausgerichtet und größer als 32 MiB. Unbekannte übergroße ROMs werden weiter abgewiesen. Ein größerer Container ergänzt keine fehlenden Handlungskapitel.
