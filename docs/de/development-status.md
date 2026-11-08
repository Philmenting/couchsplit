# Entwicklungsstatus

[English](../development-status.md) · **Deutsch** · [Anleitungen](README.md)

Aktualisiert am 8. November 2026. **[CouchSplit 0.2.0 Alpha 10](https://github.com/Philmenting/couchsplit/releases/tag/v0.2.0-alpha.10)** enthält Project PM / Pokémon Platin, getrennten GBA-Splitscreen und den aktivierbaren deutschen Feuerrot-Testmodus. Ältere Ergebnisse gelten für die jeweils genannte Version und Umgebung, nicht als vollständige Kampagnen- oder Hardwarefreigabe.

## Aktueller Quellcode — erzwungene Wechsel und EP-Vergabe

Der Quellcode nach Alpha 10 beschränkt Brüller/Wirbelwind im Trainerkampf auf die
eigenen sechs Pokémon, einschließlich ungeladener Reserven. Wunschtraum speichert
den ursprünglichen Namen getrennt. Der Platz eines besiegten Anwenders darf dadurch
wiederverwendet werden, ohne den Ersatz oder die spätere Meldung zu beschädigen.
Die EP-Vergabe nutzt nur kurzzeitig inaktive eigene Plätze und stellt die vorherige
Belegung nach jedem Empfänger wieder her. Auch beim Attackenlernen und beim
Verlassen der Übersicht bleiben beide aktiven Pokémon beim richtigen Besitzer.

Der [gezielte Prüfbericht](../verification/firered-reserves-2026-11-08.json)
unterscheidet vorbereitete Platzblockaden von tatsächlich ausgeführten nativen
Attacken, Niederlagen, Wechseln und EP-Vergaben. Er dokumentiert beide Kampfinitiatoren
und Ingame-SAVE mit erneutem Continue. Alle elf gezielten nativen Szenarien,
24 Komponententests und 64 Python-Tests bestanden ohne übersprungene Tests.
Kampagnen- und echte Controller-/Bazzite-Tests
bleiben offen; unbekannte oder beschädigte native Verweise führen weiterhin zum
sicheren Abbruch. Veröffentlichte Alpha-10-Pakete bleiben unverändert. Diese
Korrekturen liegen zunächst im Quellcode und warten auf ein späteres Release.

## Alpha 10 — Wechselmenü und besondere Besitzfälle

Eine gesperrte Reserveauswahl bleibt mit Erklärung im Spielmenü; ein geladenes
Teammitglied kann stattdessen gewählt werden. Nincadas native Entwicklung ergänzt
Ninjatom im eigenen freien Teamplatz, mit korrektem Abbruch und Pokédex-Eintrag.
Die gemeinsame originale Kennung von Ninjask/Ninjatom wird sauber unterschieden,
ohne ROM, PID oder Trainerdaten zu verändern. Das originale Feuerrot hat keine
zufällige Pokérus-Ansteckung/Verbreitung; eigener Status und nativer EV-Bonus
bleiben auch bei zwölf Teammitgliedern erhalten.

[Alpha-10-Prüfbericht](../verification/firered-alpha10-2026-11-08.json): gezielte
**19 Original-ROM-Szenarien, 23 native Prüfgruppen und alle 64 Python-Tests ohne
Überspringen**, ergänzt um Sitzungs- und Wiederaufnahmeprüfungen im Launcher. Der Alpha-9-Dauertest unten bleibt
historischer Nachweis und wurde nicht als Alpha 10 wiederholt. Der Praxistest des
Nutzers unter Bazzite mit zwei echten Controllern steht aus. Vollständig gebundene
Zwangsersatz-/EP-Plätze können weiter eine Rücksetzung erfordern; die übrigen
Experiment-/Kampagnengrenzen gelten weiterhin.
[Release-Hinweise](../releases/v0.2.0-alpha.10.md).

## Alpha 9 — eigene EP, Kartenaktualisierung und Dauertest

Mehr EP-Teiler-Träger führen nicht mehr zur Sechs-Plätze-Ablehnung. Ein Journal
hält alle zwölf ursprünglichen Identitäten, Teilnahme, Fortschritt und ausstehende
Entwicklungen fest, während die originale Engine Belohnungen und Lernen ausführt.
Das native Wechselmenü zeigt jeder Person ihre eigenen sechs Pokémon. Vorbereitete
Original-ROM-Prüfungen decken exakte EP/EV für zwölf Träger, zwölf Entwicklungen,
beide Initiatoren, störende Eingaben beim Lernen, Wechsel durch beide vollständigen
Teams und unterschiedliche Nationaldex-Freischaltungen ab. Eine Regression beim
Fangen mit kleinen Teams wurde behoben und nachgetestet. Aktive Pokémon, vorgemerkte
Wechsel und Wunschtraum-Ziele schützen weiterhin ihre nativen Arbeitsplätze.

Die Kartenaktualisierung behandelt die beiden geprüften Seeschaum-Varianten mit
gestoppter Strömung ohne erneuten Eintritt, wartet bei belegten/abweichenden Feldern
und erhält persönliche Rätselzustände. Linkclub-/Wunderkarten-Abfragen erhalten das
eigene geladene Ergebnis. 32 gezielte native Kartenfälle, darunter alle 19 oberen
Pokémon-Center-Etagen, bestehen Speichern und frisches Weiter. Trainerturm- und
e-Reader-Helfer bleiben offen.

Lokal bestehen **23 native Tests und alle 63 Python-Tests ohne Auslassungen**.
Der Dauertest nutzt echte Session/PlayView und Paar-Worker, virtuelle Controller,
natürliche Begegnungen durch Laufen, native Kartenwechsel und Ingame-Saves. Er
prüft vollständige native Save-Generationen, eigene Teams/PC/Geld und Kampfzähler,
anschließend frisches Weiter. Der [Alpha-9-Prüfbericht](../verification/firered-alpha9-2026-11-08.json)
trennt abgeschlossene Läufe vom zuvor auf Nutzerwunsch unterbrochenen Lauf und
nennt genaue Anzahlen und Dauer. Der abgeschlossene Lauf umfasst **19 Minuten
17 Sekunden, zehn gemeinsame Kämpfe und 20 Kartenwechsel**; je Person wurden
43 native Saves während der Sitzung und ein weiterer nach frischem Weiter geprüft.
Starke Teams und Ausgangspositionen sind
vorbereitet; dies belegt keinen natürlich durchgespielten Kampagnendurchlauf.

Feuerrot bleibt experimentell. Enigmabeeren verwenden die bestätigte Fortsetzung
mit Einzelkämpfen. Ninjatom-Teamergänzungen ergänzt Alpha 10. Die frühere
Pokérus-Verbreitungsgrenze war falsch: Feuerrot deaktiviert diese Routinen.
Partnerkollision/Geländeverdeckung, Spezialmodi, mehrstündiges Spielen,
echte Controller, hörbarer Ton und tatsächliches Pokémon-Spiel unter Windows/Bazzite
bleiben offen. Native Plattform-CI prüft Builds, Tests und Installation getrennt.
[Release-Hinweise](../releases/v0.2.0-alpha.9.md) · [Einrichtung und Wiederholung](firered.md).

## Feuerrot: Moduswahl und behebbare Kampfpausen — 8. November 2026

Historischer Stand vor Alpha 9: Die hier beschriebene Kapazitätsgrenze wird durch
das EP-Journal oben ersetzt; die Enigmabeeren-Fortsetzung gilt weiterhin.

**Einstellungen → Feuerrot-Koop (experimentell)** speichert jetzt die Moduswahl
und zeigt, ob die ausgewählte ROM der unterstützten deutschen Original-ROM
entspricht. Die deutsche und englische Oberfläche erläutern die noch offenen
Kampagnengrenzen. Änderungen gelten ab der nächsten Sitzung; andere GBA-Spiele
und der DS-Pfad behalten ihr bisheriges Verhalten. Warten auf den Partner und
die aktuelle Kampfsteuerung werden direkt in der Spielansicht angezeigt.

Zu viele notwendige EP-Teiler-Träger oder eine gehaltene Enigmabeere pausieren
nun beide Spiele vor der Teamumwandlung und erhalten den laufenden Fortschritt.
Die sichtbare Aktion **Einzelkämpfe fortsetzen** setzt die ursprünglichen nativen
Begegnungen ausdrücklich mit eigenen Teams und Belohnungen fort. Sobald beide
wieder in einer ruhenden Spielwelt sind, geht Koop weiter. Die betreffenden
getragenen Gegenstände danach ablegen, um wieder gemeinsam kämpfen zu können.
Dies ersetzt das Worker-Ende aus den historischen Einträgen vom 29. Oktober
unten. Die fehlenden gemeinsamen Gegenstandseffekte werden dadurch nicht
ergänzt; es entsteht auch kein automatischer Spielstand. Unerwartete Fehler
beenden die Sitzung weiterhin mit einer Diagnose.

Prüfung unter Linux: **22 native Tests und alle 60 Python-Tests bestehen**.
Sechs vorbereitete Zwei-Kern-Fälle mit Original-ROM decken beide Gründe, beide
Initiatoren und gleichzeitige Begegnungen ab: unveränderte Pause, ausdrückliche
native Fortsetzung, eigenes Ingame-Speichern/frisches Weiter und erneuter
gemeinsamer Kampfbeginn nach Ablegen der Gegenstände. Zwei normale gemeinsame
Kämpfe bestehen ebenfalls. Echte Session/PlayView und Worker prüfen beide
Ablehnungen durch einen Lauf ins Gras aus angelegten Test-Saves, eine Pause von
16 Sekunden über die Frame-Überwachung hinaus, UI-Bestätigung, nativen Einzelkampf
und Rückkehr zu Koop. Separate UI/IPC-Fixtures prüfen Deutsch/Englisch,
Speicherfehler der Einstellungen, alte Worker sowie alte oder gehaltene
Bestätigungseingaben. Absturz, Profil-/Save-Erhalt und allgemeine GBA-Speicher-,
Eingabe- und Neustarttests bestehen. Das sind gezielte Testfälle mit virtuellen
Controllern, kein Gesamtdurchlauf oder physischer Windows-/Bazzite-/Controllertest.
[Belege und Grenzen](../verification/firered-recovery-ui-2026-11-08.json) ·
[Einrichtung und Wiederholung](firered.md#pausenfunktion-prüfen).

## Feuerrot: Paar-Backup und Ton — 29. Oktober 2026

Der lokale Testbuild ergänzt die gemeinsame Sicherung und Wiederherstellung der
beiden letzten Ingame-Spielstände. Prüfsummen, Sicherung vor Wiederherstellung
und ein Journal schützen vor einer unterbrochenen Wiederherstellung; vor einer
neuen Sitzung wird das Paar zurückgesetzt. Das sind keine zeitgleichen Emulator-
Savestates. Die EP-Teiler-Kapazitätsgrenze wird auf Deutsch und Englisch erklärt
und die Fehlermeldung bleibt nach dem Worker-Ende sichtbar. Die eigene Tonwahl
für Spieler 1, Spieler 2, beide oder Stumm ist von
[SplitEmu](https://github.com/JosipFX/splitemu) inspiriert; der gemeinsame
Kampfton wird nur einmal abgespielt. Die Idee konkreter Fehlerhinweise stammt
auch aus [GBA-PK Multiplayer](https://github.com/TheHunterManX/GBA-PK-multiplayer/releases/tag/1.0);
dessen anders lizenzierter Code wurde nicht übernommen.

Prüfung unter Zorin OS: 21 native Tests und alle 60 Python-Tests bestanden;
nach der letzten Wiederherstellungsänderung bestanden die betroffenen Kern- und
Launchertests erneut. Mit der exakten deutschen Original-ROM startete der echte
Paar-Worker mit jeder Tonwahl, legte ein geprüftes Paar aus angelegten Test-Saves
an und erhielt diese beim absichtlich beendeten Kindprozess außerhalb eines
Kampfes. Die produktive Oberfläche mit Original-ROM-Auswahl, Test-Saves und
Test-Worker prüfte Paar-Wiederherstellung und Kapazitätshinweis auf Deutsch und
Englisch bei 640×360. Eine vorbereitete Zwei-Kern-Begegnung mit der Original-ROM
prüfte die EP-Teiler-Ablehnung für beide Initiatoren; sie nutzte nicht den ganzen
produktiven Worker-Fehlerpfad. Abbruch, beschädigte/fehlende Dateien und
Rücksetzung beider Dateien wurden in Komponententests simuliert. Hörbarer Ton,
physische Controller, echtes Bazzite/Windows, Stromausfall und eine vollständige
Kampagne wurden nicht getestet.

## Feuerrot: EP-Teiler und Bills Ticket — 29. Oktober 2026

Der experimentelle Paar-Worker berücksichtigt nun berechtigte EP-Teiler-Träger
aus späteren Teamplätzen im sechsköpfigen nativen Arbeitsteam, bevor das Spiel
die EP verteilt. Vorbereitete Zwei-Kern-Fälle mit der Original-ROM prüfen beide
Kampfinitiatoren, eigene EP/EV, vier Träger einer Person, Attackenlernen trotz
entgegengesetzter Eingaben des Partners, Entwicklung, natives Speichern und
frisches Laden. Passen die nötigen aktiven Pokémon und Träger nicht in sechs
Plätze, stoppt die experimentelle Sitzung vor der Teamumwandlung statt EP falsch
zu verteilen. Eine unbegrenzte Virtualisierung aller zwölf Pokémon und sämtliche
Sondergegenstände sind damit noch nicht abgedeckt.

Für beide Ausgangsspieler wurde außerdem Bills originaler Ticket-Dialog mit
vorbereiteter Verwandlung und vollem Schlüsselgegenstand-Beutel des Partners
geprüft. Ohne Platz entsteht kein falscher Empfang; nach dem Freimachen wird
genau ein Ticket zugestellt und nach Speichern und frischem Weiter behalten.
Die originale ROM bleibt unverändert. [Prüfung und Grenzen](../verification/firered-exp-share-2026-10-29.json).

## Feuerrot-Tests mit öffentlichen Spielständen und Rückblick-Korrektur — 29. Oktober 2026

Zwei unabhängig veröffentlichte Spielstände mit null und acht Orden wurden mit
der unveränderten deutschen Original-ROM geladen. Ihre vorhandenen Teams gewannen
gemeinsame wilde Kämpfe, einmal von Spieler 1 und einmal von Spieler 2 gestartet.
Eigenes Team, Beutel,
PC, Geld sowie natives Speichern und erneutes Laden wurden geprüft. Ein separater
originaler Rocko-Kampf teilte 1.400 Pokédollar in je 700 auf. Für diesen Kampf
wurden beide Spieler auf die Arena-Karte gesetzt und die automatische Synchronisierung
aufgeschoben; Teams und Begegnung wurden nicht künstlich geändert.

An den ursprünglichen Speicherorten synchronisierten sich geprüfte Story- und
Kampagnen-Meilensteine. Nach getrenntem Speichern und erneutem Laden wurden keine
Belohnungen doppelt vergeben. Unter den öffentlichen Dateien waren englische und
japanische Spielstände: Die Prüfung bestätigt diese Fälle, nicht allgemein jeden
Sprachwechsel. Der Produktionslauncher und Paar-Worker starteten die zwei Saves
unter Linux auch mit virtuellen Controllern. Fünf öffentliche Saves bestanden das
einzelne native Speichern/Laden. Ein weiterer wurde wegen doppelter Gegenstandsstapel
zurückgewiesen, eine unvollständige Datei bereits vor dem Laden.

Dabei wurde ein Fehler behoben: Der Spielrückblick enthält frühere Felddaten,
die der Worker bisher für den aktuellen Spielstand halten konnte. Er wartet nun
auf beide aktuellen Spielwelten, bevor er Fortschritt synchronisiert oder Kämpfe
zusammenführt, und blendet Rückblick-Avatare aus. Rückblick und normale Aufzeichnung
werden getrennt geprüft. **Alle 20 nativen Tests bestehen.** Das öffentliche Alpha 8
bleibt unverändert; vollständige Kampagne, echte Controller und native Windows-/
Bazzite-Tests sind weiterhin offen. [Quellen, Belege und Grenzen](../verification/firered-public-saves-2026-10-29.json).

## Feuerrot-Launcher-Laufzeitarbeiten — 29. Oktober 2026

Die originale deutsche ROM bleibt unverändert. Der lokale Paar-Worker zeigt im
nativen Wechselmenü jetzt die eigenen sechs Pokémon und lädt eigene Reserven
sicher nach. Vor der Rückkehr wird das interne Kampfteam wiederhergestellt.
Native Prüfungen decken freiwilligen/erzwungenen Wechsel, Abbruch/erneutes Öffnen,
Pokémon-Info und Rücksetzung mitten im Menü ab. Ausstehende EP und gebundene native
Referenzen bleiben geschützt; vollständig belegte Arbeitsplätze können weiter
Rücksetzung verlangen. Die spätere EP-Teiler-Nachprüfung oben ersetzt die damalige
Einschränkung für nicht geladene Träger; alle Lern- und Entwicklungspfade für
zwölf Pokémon sind weiterhin nicht vollständig abgedeckt.

Partner sind jetzt über beidseitige Außenkarten-Verbindungen sichtbar, auch bei
Koordinatenversatz. Ein echter Übergang zu Fuß und unveränderte Maschinenschnappschüsse
bestehen. Rocket-Tür/Turmrüpel und Lostelle bei vollem Beerenbeutel erreichen beide
Spieler. Eiskaskadenhöhle, bestehende Seeschaum-Varianten, eigene Puppen und alle
sieben Tanoby-Beleuchtungen behalten eigenen Zustand und native Saves.

**20 native Tests, alle 60 Python-Tests ohne Auslassungen, 34 Paar-Kampffälle,
elf Kartenfälle, drei Story-Sperren und zwei Kartenverbindungen bestehen.**
Produktions-Worker/Session und allgemeine GBA-Speicher-/Eingabe-/Neustartprüfungen
bestehen ebenfalls; ASan/UBSan/Leckprüfungen für Menü, Topologie, Kampagnenkatalog,
Laufzeit und Hooks auch. Lokale Launcher-/Worker-Dateien sind neu gebaut.
Alpha 8 bleibt unverändert. [Datierte Belege und Grenzen](../verification/firered-launcher-runtime-2026-10-29.json).
Ein natürlicher Gesamtdurchlauf, Partnerkollision/Geländeverdeckung, Spezialmodi,
lange Sitzungen und native Windows-/Bazzite-/Hardwareabnahme bleiben offen.

## Feuerrot-Kampagnensynchronisierung — 29. Oktober 2026

Der lokale Testmodus teilt jetzt geprüfte Kanto-/Sevii-Meilensteine, die sieben
weiteren Arenen mit eigenen TM-Empfängen, Story-Schlüssel/VMs und geladene
NPCs/Türen/Kollisionen. Dazu kommen aktuelle Top-Vier-Ausgänge, eigene native
Ruhmeshallen, Nationaldex und Saphir-Netzwerkabschluss mit eigenem Wander-Pokémon.
Menüs/Zwischenszenen werden abgewartet; passende Begegnungen setzen beide eigenen
nativen Skripte fort. Gleichzeitige Grasbegegnungen werden zusammengeführt,
verschiedene Skriptobjekte bleiben nacheinander an der Reihe.
[Verhalten und offene Grenzen](firered.md#kampagnensynchronisierung--29-oktober-2026).

Gezielte ROM-Abnahmen prüfen alle sieben weiteren Leiter plus vollen TM-Stapel,
originales Knogga, Zerschneider/Surfer/Stärke, zwei Champion-Fortsetzungen, eigene
native Ruhmeshallen, Nationaldex, alle drei Wander-Pokémon und bereits geladene
Netzwerkmaschinen/Wächter. Vorbereitete Voraussetzungen/starke Teams werden von
originaler Spielausführung getrennt ausgewiesen. Eigene native Saves und frisches
Weiter sind geprüft. Durchgehende natürliche Kampagne, nicht unterstützte
Kartenoperationen, vollständiger EP-Teiler, native Partnerkollision und
Zielhardware bleiben offen. Alpha 8 bleibt unverändert.
[Datierte Belege](../verification/firered-campaign-2026-10-29.json).

Frühere Kampagnenerweiterungs-Prüfungen: **18 native Tests, alle 60
Python-Tests ohne Auslassungen und 30 gezielte ROM-Szenarien** bestehen.
Produktions-Worker-/Session-Tests bestätigen Neustart, sauberen Stopp,
Absturz-/Sperrbehandlung und Save-Erhalt; die allgemeine GBA-Regression besteht.
Die ROM-Szenarien verwenden die Produktionslogik in separaten Prüfwerkzeugen.

## Frühere Paar-Kampf-Basis — 28.–29. Oktober 2026

Der lokale Quellstand enthält einen ausdrücklich einzuschaltenden Zwei-Kern-Worker,
eine Partneransicht auf derselben Karte und gemeinsame native Kämpfe mit eigenen
menschlichen Aktionen sowie getrennten Ergebnissen und Saves. Normales GBA und DS
bleiben der Standard. Das ist **kein vollständiger Welt-Koop-Release**.
[Einrichtung, Prüfwerkzeuge und offene Anforderungen](firered.md#entwicklungsziel-feuerrot-koop--28-oktober-2026).

Unter Linux mit der exakten deutschen Original-ROM geprüft: normale wilde und
Trainerkämpfe, eigene Traineridentität/EP, AP, Fang-/Beutel-/PC-/Pokédex-Zuordnung,
Geldobergrenzen und geteilte Trainer-/Zahltagbelohnung, vollständige Maschinen-/
Flash-Rücksetzung, Fortsetzung durch den überlebenden Partner sowie eigene native
Niederlagenbehandlung. Ein voller Initiator-PC blockiert keinen Gast mit Platz;
ein tatsächlich voller Empfänger erhält die native Ablehnung. Bei zwei gewählten
Bällen wird nur der tatsächlich ausgeführte Wurf bezahlt. Jede Abnahme nutzt das
normale Speichermenü und frisches Weiter allein aus dem Batteriespeicher für beide
Trainer, mit eigenen Teams, Beuteln, Geld, PC, Pokédex und Identität.

Der erste Rivalenkampf startet zusätzlich durch echte Bewegung und sein originales
Feldskript, ohne künstlich ausgelöste Begegnung. Seine Tutorialsteuerung wird auf
zwei menschliche Verbündete umgestellt; Originalflags und Fortsetzung kehren zurück.
Sieg und Niederlage beenden die Laborszene, heilen beide Trainer über die native
Engine, teilen diesen geprüften Meilenstein und überstehen getrenntes Speichern/
Laden. Spieler 2 erhält ausdrücklich eine andere Test-Traineridentität; der
Niederlagentest setzt niedrige KP und Heuler. Das ist eine gezielte echte
Spielprüfung und kein vollständiger Kampagnendurchlauf.

Getrennte Prüfungen mit Produktions-Session/PlayView und Paarworker bestätigen
beide Bilder, virtuelle Eingaben, Neustart, geordneten Stopp, Absturzerkennung,
Sperrlebensdauer und Erhalt vorheriger Saves. Alle 60 Python-Tests bestehen mit
installierter ARM-Abhängigkeit. Native Kern-/UI- und Feuerrot-Komponententests
sowie Produktions-Worker-Regressionen für SRAM/Flash/EEPROM und 64/512-MiB-XROM
bestehen. Physische Controller, hörbarer Ton, native Windows-/Bazzite-Ausführung
und eine vollständige Kampagne bleiben ungeprüft.

Die früheren vier geprüften Story-Ereignisse (Laborrivale, Eichs Paket, Pokédex und Rocko) besaßen
einen ausdrücklichen Ereignis-/Belohnungsmechanismus und native NPC-Aktualisierung.
Empfangsflags im eigenen Save erlauben Nachlieferung nach Neustart. Diese ursprüngliche begrenzte
Liste und sicheres Nachladen erschöpfter Reserven ersetzten keine kampagnenweite
Story-/NPC-Synchronisierung, freie Sechserteam-Auswahl, vollständige EP-Teiler-/
Entwicklungsbehandlung, Partnerkollision oder Geländeverdeckung. Der frühere
[Einzelkern-Bericht](../verification/firered-team-input-2026-10-28.json) behält seinen
historischen Umfang. Die öffentlichen Alpha-8-Pakete bleiben unverändert.


Frühere Basis-Abnahme: **14 native Tests, alle 60 Python-Tests ohne
Auslassungen, 21 Paar-Kampfszenarien und 6 native Story-Szenarien** bestehen.
Die 27 ROM-Prüfungen umfassen getrenntes natives Speichern/frisches Batterieladen,
persönliche Fangdaten, Team-AP, Entwicklungs-/Aktionsstatistiken und geprüfte
Quest-/Ruhmesdatei-Empfänge. Auch die nativen Trainer-/Zahltag-Zahlenpuffer passen
zum tatsächlichen geteilten Preisgeld. [Maschinenlesbarer Umfang](../verification/firered-paired-2026-10-29.json).

## Große GBA-ROMs: 23. Oktober 2026

Der lokale Build unterstützt ausdrücklich markierte CouchSplit-XROM-v1-Dateien bis 512 MiB, bei gleicher Oberfläche und erhaltenen DS-/GBA-Pfaden. Normale GBA-ROMs bleiben auf 32 MiB begrenzt. Ein nachvollziehbarer mGBA-Patch ergänzt einen nur lesbaren Datenbereich, CPU-/LDM-Zugriffe, DMA-Quellen und BIOS-Dekomprimierung. Er erzeugt keine Spielinhalte oder zusätzlichen Handlungskapitel. [Format und Grenzen](extended-rom.md).

Lokal geprüft: eigene ARM-Testprogramme mit 64 und 512 MiB lesen tatsächliche Daten am Dateiende; Byte-/Halbwort-/Wort-, unaligned- und LDM-Zugriffe, DMA16/32, BIOS-CpuSet/LZ77 nach RAM/VRAM, Eingabeisolation zweier Worker, kaltes SRAM-Laden und sauberer Stopp. Native Komponenten-/UI-Tests und alle 60 Python-Tests bestehen (7 optionale Abhängigkeits-/Umgebungsprüfungen in diesem Lauf übersprungen); bestehende SRAM-/Flash-/EEPROM-Prüfungen am Produktions-Worker bestehen ebenfalls. Die separate private 64-MiB-Reise-ROM verlagert echte Pokémon-Grafiken hinter 32 MiB; Einstiege Kanto/Johto/Hoenn, gemeinsamer PC und normales Speichern sind kurz geprüft. Vollständige Kampagnen sowie Windows/Bazzite, Hardware und hörbarer Ton bleiben ungeprüft.

## Allgemeine GBA-Erweiterung — 21.10.2026

Die bestehende GitHub-Menüaufteilung bleibt erhalten. Ein geprüfter Header wählt GBA automatisch aus: direkter/ZIP-Import, maximal 32 MiB, ein Bildschirm mit 240×160 Pixeln je Spieler und passende Eingaben/Hinweise. Es gibt kein Reise-/Bankmenü und keine zusätzliche Emulatorzeile in den Einstellungen. Project PM / Platin nutzt weiterhin den bisherigen DS-Worker und IPC-ABI 2. [GBA-Einrichtung und Grenzen](firered.md).

Unter Zorin OS 18.1 mit lokalem Qt 6.4.2, SDL 2.30 und mGBA 0.10.5 geprüft: native Komponenten-/UI-Tests, GBA-Auswahl und deutsche/englische Menübilder, virtuelle Controller einschließlich Trennung und normales Select+R unter GBA bestehen. Alle 59 Python-Tests bestehen mit echtem xdelta3 und aktivierten ARM-Abhängigkeiten. Eigene ARM-Cartridge-Testprogramme laufen auf zwei Produktions-Workern für SRAM mit 32 KiB, Flash mit 64/128 KiB sowie EEPROM mit 512 Byte/8 KiB. Sie schreiben getrennten Fortschritt, prüfen Eingabeisolation, laden nach Neustart identische Speicherbytes und beenden sich sauber. Ein Spiel ohne Batteriespeicher erzeugt keinen Save; ungültige Speichergrößen werden ohne Überschreiben der vorhandenen Bytes abgewiesen.

Die echte deutsche Feuerrot-ROM erreicht getrennt davon das Zimmer des bestehenden Testhelden über Weiter, lädt nach Neustart erneut und behält die vorherigen Save-Prüfsummen. Die ROM bleibt unverändert. Dies ist zusätzlich zum früheren Nachweis der Ingame-Speichererstellung unten geprüft. DS-Produktionstransport, Speicherpfad-/Konfigurationsprüfungen und zwei echte DS-Worker mit eigenem ARM-Programm bestehen ebenfalls. Gemeinsame GBA-Spielwelt/Link-Multiplayer, Regionsreisen, Bank, physische Controller, hörbarer Ton, Windows/Bazzite und vollständige Spieldurchläufe sind nicht nachgewiesen. Die allgemeine Formaterkennung bestätigt nicht jeden ROM-Hack. Öffentliche Releases bleiben unverändert.

## Frühere reine Feuerrot-Erweiterung — 21.10.2026

Die lokale Entwicklungsversion erkennt die deutsche Feuerrot-Originalausgabe anhand ihrer exakten Prüfsumme und startet zwei getrennte Abenteuer über mGBA 0.10.5. Der vorhandene Launcher übernimmt Controllerzuordnung, zwei Bildflächen mit jeweils 240×160 Pixeln sowie getrennte Spielstände, Sperren und Backups pro Spieler und ROM. Eine externe GBA-BIOS-Datei wird nicht benötigt. [Einrichtung und Grenzen](firered.md).

Unter Zorin OS 18.1 mit Qt-Offscreen, Dummy-Audio und virtuellen SDL-Controllern geprüft: echte `.gba`-/ZIP-Importe mit unveränderter Prüfsumme, zwei echte GBA-Worker mit deutschen Spielbildern, ausschließlich an Spieler 1 wirkende Eingaben sowie Ingame-Speichern mit allen 14 Speicherabschnitten und Laden über **Weiter** nach sauberem Beenden. Spieler 2 behält seinen getrennten leeren Spielstand. Das private Prüfprogramm verwendet die tatsächliche Produktions-Sitzung und Bilddarstellung. Native Komponenten-/UI-Tests und alle 58 Python-Tests einschließlich echtem xdelta3 und ARM-Prüfungen bestehen. Der Produktions-DS-Transporttest und der Smoke-Test mit zwei echten DS-Homebrew-Workern bestehen weiterhin.

Ein erster Neustart während der noch sichtbaren Speicheranzeige unterbrach den ersten Ingame-Speichervorgang. Der Wiederholungstest nach abgeschlossenem Speichern bestand. Atomare Dateiausgabe ersetzt nicht das vollständige Speichern innerhalb des Spiels.

Gemeinsame Feuerrot-Spielwelt, Link-Kämpfe/Tausch, Pokémon-Bank und Regionsreisen sind noch nicht umgesetzt. Physische Controller, hörbare Tonausgabe, GBA-Ausführung unter Windows/Bazzite und ein vollständiger Spieldurchlauf sind offen. Der öffentliche Alpha-8-Release enthält diese Erweiterung noch nicht; seine Pakete bleiben unverändert. Die folgenden Abschnitte dokumentieren frühere Entwicklungsstände.

**Aktueller Nachtrag:** Alpha 4 wurde mit der bereitgestellten deutschen PM-0.4.5-ROM tatsächlich bis zum Pokédex gespielt. Zwei Figuren und ihre Begleiter bewegen sich synchron; ein gemeinsamer Wildkampf mit getrennten Befehlen, identischen Gegnerzuständen, Sieg/EP/Levelaufstieg und anschließendem Ingame-Speichern/-Laden wurde unter Linux geprüft. Native Windows-/Bazzite-Spielabnahme, Trainer-Koop, Tausch und mehrstündiger Dauertest bleiben offen. Die älteren Abschnitte unten beschreiben den damaligen Prüfstand.

Am 16.10.2026 wurde die veraltete Zielauswahl nach besiegtem Gegner erneut im tatsächlichen Koop-Kampf reproduziert und eine separate, exakt hashgebundene ROM-Korrektur erstellt. Der vermutete dauerhafte Begleiterverlust bestätigte sich im Nachtest nicht. [Korrektur und Prüfgrenzen](rom-target-cursor-fix.md).

## Implementiert

- POSIX-Portierung des Project-PM-Bridgecodes: nichtblockierende Sockets, select fd+1, EINPROGRESS, SO_ERROR, MSG_NOSIGNAL.
- Eigener lokaler Host/Client-Modus ohne Autopilot-Tastendrücke, ohne Online-Relay und ohne Broadcast. Konfigurierbarer lokaler TCP-Port.
- Getrennte Prozessprofile und Save-Verzeichnisse je Spieler/ROM-Hash; vorhandener melonDS-SaveManager bleibt für Ingame-Speicherung zuständig.
- mmap-Framebuffer- und Inputschnittstelle; beide echten DS-Prozesse in einem Qt-Fenster.
- Links/rechts und oben/unten; 4:3-Layouts, kleiner Touchscreen, nebeneinander, Einzelansicht-Umschaltung.
- SDL-Instanz-ID-Zuordnung, automatische Erstzuordnung bei genau zwei Geräten / eindeutigen gespeicherten Kennungen, explizite Auswahl bei Mehrdeutigkeit, konfigurierbare Tasten, Stick-Touch und Trennungsschutz.
- TV-Oberfläche mit größeren Elementen auf einer 4K-Fläche; Controller-Menünavigation, Logs, Einstellungen, Backup/Restore.
- Sitzungssperre, SHA-256-Backupmanifeste, atomare Metadaten, gemeinsame Beendigung, Prozessfehlerbehandlung, Parent-death-Signal.
- Distrobox-Build/Run-Wrapper, Desktop-Eintrag und automatische Steam-Anfrage über Bazzites Host-Helfer ohne direkte Bearbeitung vorhandener Konfigurationen.
- GPL-Lizenz, AGENTS.md, gepinnter Upstream, CI-Workflow, dokumentierte Installation und Abnahme.
- Lokale xdelta-Patchvorbereitung mit CPUD-Kennung, drei SHA-256-Prüfungen, unveränderter Originaldatei und atomarer Ausgabe. Enthält keinen Project-PM-ROM-Patch; [deutsche Kompatibilität und Beschaffungsblocker](german-compatibility.md).

## Automatisiert getestet

Umgebung: Ubuntu 24.04 x86_64, GCC 13.3, CMake 3.28.3, Qt 6.4.2, SDL 2.30.0. Entwicklungsbibliotheken wurden lokal aus Ubuntu-Paketen bereitgestellt; keine Bazzite-Umgebung. Qt-GUI-Tests offscreen, Audio im Smoke-Test dummy. Tests liefen hier. Der GitHub-Actions-Workflow nach der Veröffentlichung am 14.10.2026 bestand ebenfalls: vollständiger Neubau, native Komponentensuite, Produktions-TCP-Bridge, Python-Tests, Qt-Launcher-Start und zwei echte Homebrew-Emulatorprozesse. Erfolgreicher Lauf: https://github.com/Philmenting/couchsplit/actions/runs/38607299933 (4 Minuten 43 Sekunden).

| Test | Tatsächliches Ergebnis |
|---|---|
| Vollbuild Project PM | 223 Buildschritte einschließlich Qt/SDL-Frontend und finalem Link erfolgreich |
| Vollbuild Launcher + Tests | Erfolgreich; eigener Code nach Warnungskorrekturen ohne die zuvor gemeldeten Einrückungswarnungen |
| Produktions-TCP-Bridge | Rollen 1/2, Lobby-Namen, beide Richtungen, fragmentierte Frames, ungültige Länge, Wiederverbindung, refused connect und SIGPIPE erfolgreich |
| Native Komponentensuite | IPC in getrennten Prozessen, ABI/Dateigrenzen, 4:3-Geometrie in mehreren Auflösungen, Profilsperre, Backup/Restore, Korruptionsschutz erfolgreich |
| Virtuelle SDL-Controller | Unabhängige DS-Tasten, Touch, Toggle-Flanke, Trennung und keine falsche Übernahme nach Indexverschiebung erfolgreich |
| Prozessmanager | Drei Wiederholungsstarts, graceful stop, Workerfehler und Freigabe der Sperre erfolgreich; bewusst TEST-ONLY-Prozessfixture |
| Zwei echte Emulatorprozesse | Eigenes ARM-Homebrew auf beiden DS-Kernen ausgeführt, RAM-Framebuffer ausgegeben, TCP verbunden, Rollen vergeben, SIGTERM-Aufräumen mit Exitcode 0 |
| Bereitgestellte Original-Platin-ROM | 35 Sekunden in zwei realen Prozessen, >2000 Frames je Spieler, TCP-Rollen 1/2, Eingaben nur an Spieler 1, beide Exitcode 0; ROM-Bridge blieb inaktiv. Opt-in-Test außerhalb CI. |
| Installer | Zwei Python-Tests: Pfade mit Leerzeichen, Argumentquoting, fehlendes Binary; keine Steamkonfigurationsänderung |
| Patchvorbereitung | Echte xdelta3-Testpatches auf eigenen synthetischen Daten: Rekonstruktion, Originalerhalt, Zielschutz, Prüfsummenfehler, englische Kennungen, kaputter Patch und fehlender Decoder |
| Bereitgestellter PM-Patch 0.4.5 | Anleitung verlangt USA Rev 1; echter Decoder verweigert deutsche Original-ROM mit Prüfsummenfehler. ZIP-Inspektion erkennt Patch/ROM und lehnt die unpassende Basis ab. Kein gepatchtes Gameplay gestartet. |
| US-PM-ROM 0.4.5 nach weiterer Bereitstellung | Reguläres xdelta mit aktiven Prüfungen erfolgreich, Ergebnis-SHA-256 bestätigt und aus ZIP reproduziert. Zwei echte Prozesse 35 Sekunden, je 2075 Frames, TCP-Rollen 1/2, beide Exitcode 0. Einführung getestet; Wireless-Aktivierung und gemeinsame Welt offen. |
| GUI-Erzeugung | Qt offscreen gestartet, Screenshots bei 1280 × 1000 und 3840 × 2160 erzeugt |
| Buildpatch | Gegen den gepinnten Quellstand reversibel anwendbar geprüft |
| GitHub Actions | Vollständiger Neubau und alle ROM-freien Tests erfolgreich (Run 38607299933) |

Auch der Folgecommit zur Original-ROM-Erkennung bestand den vollständigen GitHub-Neubau und alle damaligen ROM-freien Tests: https://github.com/Philmenting/couchsplit/actions/runs/38610385288. Die anschließend ergänzte Patchvorbereitung wurde lokal mit xdelta3 3.0.11 getestet: sieben Python-Testfälle (einschließlich der zwei Installertests), alle erfolgreich. Dabei wurde kein Project-PM-ROM-Patch verwendet; dessen Beschaffung und deutscher Port bleiben blockiert.

Die Patchvorbereitung bestand anschließend auch GitHub CI (https://github.com/Philmenting/couchsplit/actions/runs/38612252475). Nach Bereitstellung von Build 0.4.5 wurden Paket-/Basisprüfung und Pokédex-Hinweis ergänzt: lokaler Neubau, native Komponentensuite und nun neun Python-Tests einschließlich ZIP-Inspektion/Mehrdeutigkeitsablehnung erfolgreich. Der reale Patchversuch auf der deutschen ROM wurde erwartungsgemäß abgelehnt; keine gemeinsame Spielwelt getestet. Der zuvor fehlende Patch liegt jetzt vor, aber für die verlangte US-Basis beziehungsweise einen deutschen Port fehlen weiter die passenden Dateien.

Nach zusätzlicher Bereitstellung der US-Basis: Patchskript unterstützt den bekannten Release aus ZIP mit automatischen Prüfsummen und verbietet abweichende Hash-Overrides. Das echte Ergebnis wurde ein zweites Mal reproduziert. Lokaler Neubau und native Tests erfolgreich; zehn Python-Tests bestehen, einschließlich automatischer Release-Prüfsummen/ZIP-Decodierung mit eigenem Testdatenkatalog und echtem xdelta. Patched-US-Start: 35 Sekunden, je 2075 Frames, TCP verbunden, beide sauber beendet. Die englische Datei wurde dem Nutzer separat bereitgestellt; keine ROM in GitHub.

Der Homebrew-Test enthält keine Nintendo-Spiel-/Logo-/BIOS-/Firmwarebytes. Er prüft **nicht** ROM-Mailbox-Synchronisation oder Pokémon-Gameplay. Die separate TCP-Prüfung extrahiert den echten Bridgecode; das ist kein Ersatztransport.

## Deutsche Text-Alpha, 15.10.2026

Implementiert: stdlib-Nitro/NARC/Nachrichtenwerkzeuge, Drei-Wege-Abgleich, 857 eigene Übersetzungsfingerabdrücke (1.208 Vorkommen), deutsche Originaltexte und geprüfte Fontübernahme, exakte Eingabehashes, atomare Ausgabe ohne Überschreiben. Alle PM-Programm-/Overlay-/Skriptbytes bleiben unverändert. Die unbenutzte regional unterschiedliche Diamond/Pearl-Abspannbank wird erhalten. [Verfahren, Abdeckung und Grenzen](german-localization.md).

Automatisiert getestet: fünf neue synthetische Format-/Merge-/Kontrollzeichen-/Byteinvarianztests; vollständige Python-Suite mit echtem xdelta3: 15 Tests erfolgreich, keine übersprungen. Zweimaliger echter ROM-Build ergibt identischen SHA-256 `8b11f36e7bf4a15932eb364358fba183c2067ac8265cfbc97611ca53e3c90949`. 40 Sekunden in zwei echten Emulatorprozessen: 2369/2370 Frames, je 2097 zusätzliche Frames nach Sekunde 5, TCP verbunden, Rollen 1/2, beide sauber mit Exitcode 0 beendet. Funkspiel-Bridge blieb vor dem Pokédex inaktiv; keine Welt-Synchronisation bewiesen.

Visuell geprüft: deutsche Begrüßung „Schön, dich kennenzulernen!“ mit Umlaut im echten Emulator-Framebuffer. Nicht verifiziert: vollständige Story, alle PM-Menüs und Textbreiten, sprachabhängige US-Grammatikroutinen, Ingame-Save-Migration, Bazzite-Hardware. Grafisch eingebettete Schrift ist nicht lokalisiert; „YES/NO“ in der Einführung wurde als englisch erkannt. Ein weiterer 105-Sekunden-Lauf mit Eingaben an Spieler 1 bestand mit 6272/6273 Frames, TCP-Verbindung und beiden Exitcodes 0. Originaltext-Anleitungen auf beiden DS-Schirmen wurden deutsch dargestellt.

## JA/NEIN-Grafik, Alpha 2

Implementiert: originale deutsche Touch-Textur aus dem regionalen Pfad unter dem US-Pfad übernommen. Die übrigen neun Archivmitglieder (Palette und acht Animationskarten) sind bytegleich, entpackte Grafikgröße und alle PM-Programmbytes unverändert. Keine Nintendo-Grafikbytes im Repository. Schutz vor PM-Modifikationen und inkompatiblen Ressourcen durch zwei neue Tests; komplette Python-Suite jetzt 17 Tests erfolgreich.

Automatisiert getestet: zwei byteidentische Builds, SHA-256 `800106962a6dd4eba0d090c4acca322d8e45f0eefbe3281aa2a843459a08629c`. 110 Sekunden in zwei echten Emulatorprozessen, 6566/6571 Frames, TCP verbunden, Rollen 1/2, beide Exitcodes 0. Visuell geprüft: JA/NEIN im Framebuffer vor dem Touch; einmaliges Antippen von JA führt zu Professor Eibe und dem nächsten Dialog. Kein Bazzite-/Hardwaretest und kein Welt-Koop-Nachweis. Andere Grafiken wurden dadurch nicht automatisch lokalisiert. [Details](german-localization.md).

## Ein-Datei-Installer

Implementiert: siehe [Installationsablauf und abgegrenzter Prüfstatus](one-file-installer.md). Automatisiert getestet: 38 Python-Tests sowie nativer Neubau/Komponententest und Offscreen-Launcher-Start bestehen. Echter ROM-ZIP-Import ergibt den unveränderten Alpha-2-SHA-256. Sechs zusätzliche Steam-Tests prüfen den Host-Helferaufruf, Wiederholschutz, explizite Wiederholung und Fehlergrenzen mit Testdoubles. Weitere fünf Installer-Regressionstests prüfen Live-Ausgabe, UTF-8, Timeout/Prozessgruppen, Containerdiagnose und Fortschrittsansteuerung. Ein nativer UI-Test bestätigt den Beenden-Knopf mit Mausereignissen und virtueller SDL-Navigation einschließlich geordnetem Stoppen zweier Testprozesse und Freigeben der Savesperre. Auf Bazzite bleiben Fortschritts-/Beenden-Korrektur und Steam-Aufnahme nachzuprüfen.

## Manuell bestätigt

Die hier erzeugten Qt-Menübilder wurden auf Lesbarkeit und abgeschnittene Inhalte visuell kontrolliert. Private DS-Bilder der deutschen Original-ROM zeigen unterschiedliche Intro-/Einführungszustände nach Eingaben nur an Spieler 1. **Benutzerrückmeldung vom 15.10.2026:** Der Installer lief auf Bazzite durch und der Launcher startete. Dabei fehlte sichtbarer Fortschritt und der bisherige Beenden-Knopf schloss die App nicht. Die Korrekturen wurden automatisiert geprüft; ihre Bestätigung auf dem tatsächlichen PC und Tests mit physischen Controllern stehen aus.

## Nicht verifiziert / offen

1. Patch 0.4.5 und passende US-Rev-1-Basis wurden bereitgestellt und erfolgreich gepatcht. Die US-basierte deutsche Text-Alpha ist inzwischen implementiert und gestartet; ein nativer CPUD-Patch und vollständige Spielprüfung fehlen. [Pakettest](project-pm-045.md).
2. Die gemeinsame Welt, Funkspiel-Aktivierung und gemeinsame Wildkämpfe sind inzwischen unter Linux mit der Nutzer-ROM geprüft. Trainer-Koop, Tausch und Zielhardware bleiben offen.
3. Tatsächliches Ingame-Speichern und erneutes Laden beider Pokémon-Spielstände sind unter Linux geprüft; kein vollständiger Spieldurchlauf.
4. Physische USB-/Bluetooth-Controller, Steam-Input-Konflikte, Gerätetrennung und Neuverbindung auf dem Zielsystem.
5. KDE Plasma Wayland, Steam Gamescope, Distrobox-GUI/Audio/Gerätezugriff auf Bazzite, Desktop/Gaming-Mode-Wechsel.
6. 4K/60-Hz-Ausgabe und Performance/Thermik auf RTX 4060; Bildgeometrie und 4K-Menürendering sind geprüft, tatsächliche TV-Ausgabe nicht.
7. Mehrstündiges Spielen, Kämpfe/Tausch und Verbindungsunterbrechungen während des Pokémon-Gameplays.
8. Hochauflösendes DS-3D-Rendering, Rumble und umfangreiche Spielebibliothek: nicht implementiert. Direkter Start einer vorbereiteten ROM und geprüfte lokale xdelta-Patchvorbereitung sind vorhanden; ein deutscher Project-PM-Patch und automatische ROM-Mod-Portierung sind nicht vorhanden.

## Priorisierte nächste Aufgaben

- [x] Project-PM-Linux-Build und tatsächlichen TCP-Transport ermöglichen.
- [x] Profile, Prozessverwaltung, Eingaben, Darstellung und Launcher implementieren.
- [x] Verfügbare ROM-freie Tests und echte DS-Homebrew-Starts durchführen.
- [x] Repository auf GitHub anlegen und lokalen Commitstand veröffentlichen.
- [x] Passende, rechtmäßig bereitgestellte ROM/Project-PM-Patchversion auf Start testen.
- [x] US-basierte deutsche Text-Alpha bauen und Einführung prüfen.
- [x] Passende USA-Rev-1-Basis mit PM 0.4.5 patchen und in zwei echten Emulatoren auf Intro-/TCP-/Prozessfunktion testen.
- [x] Bereitgestellten Patch 0.4.5 auf Basisanforderung prüfen und echten deutschen Patchversuch durchführen: inkompatible Basis bestätigt.
- [ ] Gemeinsame Sinnoh-Welt nach docs/acceptance-tests.md nachweisen; erst dann spielbare Koop-Version behaupten.
- [ ] Bazzite-/Controller-/TV-Abnahme und Dauerbetrieb.

## GitHub-Veröffentlichung

GitHub-Repository am 14.10.2026 nach Zustimmung über die Weboberfläche privat erstellt: https://github.com/Philmenting/couchsplit. Quellcode und Testnachweise sind vollständig übertragen; die Git-Trees der drei Entwicklungsstände stimmen exakt mit den lokal geprüften Ständen überein.

Das private Repository wurde über die GitHub-Weboberfläche erstellt. Die unterstützten Datei-/Git-Operationen wurden anschließend über die verbundene GitHub-App ausgeführt. Kein fremdes Upstream-Repository wurde verändert. Die ursprünglichen drei lokalen Commits bleiben zusätzlich im lokalen Branch local-implementation erhalten; main folgt der veröffentlichten GitHub-Historie.

Das folgende Skript dient ausschließlich der erstmaligen Veröffentlichung einer unabhängigen Kopie ohne bestehendes origin; für dieses bereits veröffentlichte Repository ist es nicht mehr erforderlich:

```bash
./scripts/publish-github.sh            # privat
./scripts/publish-github.sh --public   # öffentlich, falls gewünscht
```

Das Skript verweigert uncommittierte Änderungen und ein bereits bestehendes origin. Es fragt kein Token ab und legt keines im Projekt ab. Die Datei AGENTS.md unterstützt die Fortsetzung mit Codex.

## Podman-Neustartfehler nach Zielsystem-Rückmeldung

Implementiert: begrenzter Bereinigungsversuch ohne Containerlöschung für den exakten Fehler „creating temporary passwd file … merged/etc/passwd: no such file or directory“, nur nach Stop-/Eigentumsprüfung. Automatisiert getestet: drei weitere Tests prüfen Erfolgspfad, maximal einen Wiederholungsversuch und Ablehnung laufender/fremder/anders fehlerhafter Container. Nicht verifiziert: dass `podman container cleanup` den konkreten Fehler auf dem Bazzite-PC behebt. Der Bericht mit Podman 5.8.4 zeigt erfolgreiche frühere Initialisierung und einen fehlgeschlagenen späteren Start; die tieferliegende Speicherursache ist offen. Keine ROM-/Save-Löschung und keine Eingriffe in andere Container.


## Native Plattformpakete 0.2.0 Alpha 1

Bazzite 0.1.0-alpha.1 ist als unveränderter GitHub-Prerelease veröffentlicht. Der neue Code ergänzt native Windows-Dateimappings, kooperative Worker-Abschaltung (IPC ABI 2), Unicode-ROM-Import und Windows-Job-Gruppen. Paketbauer erzeugen ein ZorinOS-18-/Ubuntu-24.04-DEB sowie eine Windows-Setup-EXE und portable ZIP mit passenden eigenen und Emulatorquellen.

Die Freigabe ist in `.github/workflows/platforms.yml` an erfolgreiche native Linux-/Windows-Builds, Komponenten- und ROM-freie Emulatorprüfungen gebunden. Zusätzlich werden DEB und Windows-Setup in der CI installiert, der installierte Launcher gestartet und anschließend deinstalliert; persönliche Daten müssen erhalten bleiben. Details und tatsächliche Ergebnisse stehen beim jeweiligen Release-Commit in GitHub Actions. Das ersetzt keine manuelle Abnahme auf ZorinOS/Windows, mit Bluetooth-Controllern oder einer synchronen Sinnoh-Spielwelt.

Windows-Nativtests bestätigen die Komponenten einschließlich kontrollierter Worker-Abschaltung. Die Transportprüfung deckte außerdem den Windows-Firewall-Helfer im PM-Upstream auf: Eigene lokale Sitzungen überspringen ihn jetzt ausdrücklich; es werden keine Firewall-Regeln oder Administratorprompts für Loopback eingerichtet.


## Plattformübergreifende Fehlerkorrekturen 0.2.0 Alpha 2

Implementiert am 15.10.2026: fester Koop-Speichername `game.sav` innerhalb des unveränderten Spieler-/ROM-Hash-Profils. Der Launcher sichert vor der Übernahme, kopiert einen eindeutig zugeordneten alten Save und erhält die Quelldatei. Mehrdeutige Altbestände verlangen die ursprüngliche ROM-Auswahl. Das Wiederherstellen älterer Backups aktualisiert ebenfalls den festen Save. Die neue Emulatorfähigkeit `STABLE_SAVE=1` verhindert das Mischen mit alten Emulator-Binaries; IPC bleibt ABI 2. Der eigene Emulatorpfad verändert nur Koop-DS-Saves, keine Savestate-, Cheat-, GBA- oder regulären melonDS-Dateinamen.

Das Hauptmenü besitzt einen Scrollbereich und folgt dem Fokus bei Tastatur-/Controllerbedienung. Lange ROM-/Controllernamen erzwingen keine überbreiten Fenster. Minimieren neutralisiert Tasten und Touch sofort und bei jedem weiteren Timerlauf; verborgene Zeit zählt nicht zur Beenden-Kombination.

Lokal auf Zorin OS 18.1 bestanden: 38 Python-Tests mit xdelta3, beide nativen CTest-Ziele, produktiver Transport und die aus dem echten Emulatorquellcode kompilierte Speicherpfadprüfung. Neue native Regressionen decken Save-Übernahme, umbenannte ROMs, Altbackups, unveränderte aktuelle Saves bei Mehrdeutigkeit, getrennte Spieler/ROM-Hashes sowie 360p/720p/768p/1080p/4K-Menüs, Scrollen per virtuellem SDL-Controller und Eingaben im minimierten Fenster ab. Auch der neu gebaute Launcher bei 1280 × 720 und zwei neu gebaute echte Emulatoren mit selbst erzeugtem ARM-Homebrew bestehen den Bild-/TCP-/geordneten Abschalttest. Windows- und Bazzite-Zielsystembedienung sowie echte Pokémon-Spielstände/Sinnoh-Synchronisation werden dadurch nicht behauptet. Native Plattform-Builds und Paketprüfungen bleiben vor Veröffentlichung erforderlich.


Zusätzlicher Integrationsbefund vom 16.10.2026: Ein echter wiederholter Session-Start mit umbenanntem ARM-Homebrew reproduzierte einen TOML-Serialisierungsabsturz. Die Konfigurations-Getter legen fehlende Werte an, während ein anderer Emulator-Thread den Baum speichert. Ein separater Test gegen den echten `Config.cpp` reproduziert denselben Fehler vor der Korrektur. Die öffentlichen Tabellen-/Arrayzugriffe sowie Laden/Speichern teilen jetzt einen rekursiven Mutex; nach der Korrektur besteht der gleichzeitige Getter/Setter/Array-/Serialisierungstest. Die Dateioperationen des Testprogramms sind ausdrücklich lokale Platform-Testgrenzen. Nach dem Neubau bestehen auch zwei echte vollständige Session-Starts mit identischen, unterschiedlich benannten Test-ROMs und getrennten, aktualisierten Save-Markern für beide Spieler; beide Sitzungen beenden ihre Worker geordnet und geben die Sperre frei.

## Aktive Fehlerfälle und Korrekturen 0.2.0 Alpha 3

Am 16.10.2026 mit virtuellen SDL-Controllern und zwei echten Linux-Emulatoren reproduziert: Das minimierte Hauptmenü führte noch Controller-Aktionen aus; eine neue ROM-Auswahl änderte die Anzeige trotz weiterlaufender alter Sitzung; frei belegte Tasten der Ansichts-/Beenden-Kombination gelangten zusätzlich als DS-Eingabe zum Spiel.

Korrigiert im gemeinsamen Plattformcode: Das minimierte/verborgene Menü nimmt keine Aktion an und verlangt nach der Rückkehr zunächst das Loslassen gehaltener Tasten. Die ROM-Auswahl wird vor dem Dateidialog mit einem Hinweis auf das reguläre Sitzungsende blockiert. Tastenkombinationen verbrauchen ihre physischen Tasten vor der frei gewählten DS-Zuordnung; andere Tasten und Spieler bleiben unabhängig.

Lokal bestätigt: beide nativen CTest-Ziele und alle 38 Python-Tests erfolgreich. Neue native Regressionen prüfen minimiertes/verborgenes Menü, gehaltene Taste beim Wiederherstellen, weiterhin funktionierende neue Tastendrücke, blockierte ROM-Auswahl mit unveränderten Einstellungen und fortsetzbarer Sitzung, beide Kombinationen mit allen zwölf DS-Belegungen sowie unbeteiligte DS-Start-/R-Eingaben. Die Tests verwenden produktive Widgets und virtuelle Controller; die nativen Sitzungstests verwenden ausdrücklich Test-Worker.

Zusätzlich besteht der isolierte Aktivtest mit unveränderten produktiven Widget-/Session-Funktionen und zwei echten Emulatorprozessen: alle drei ursprünglichen Fehlerfälle, zulässiger Import und tatsächlicher Start von ROM B nach Ende der Sitzung A, Trennung ohne Geräteübernahme, Minimieren im Spiel, Prozessausfall sowie zwölf vollständige Starts und Beendigungen. Unterschiedliche vorbereitete Test-Spielstände beider Spieler bleiben dabei bytegenau erhalten; die Homebrew-ROM schreibt keine Pokémon-Saves. Die drei Korrekturen wurden mit frischen Profilen unter Qt offscreen und auf einem virtuellen X11-Bildschirm erneut erfolgreich geprüft. 720p-Menüaufnahme sowie der separate Produktions-Emulator-Test mit Bildausgabe, TCP-Rollen und geordneter Abschaltung bestehen ebenfalls.

Pakete werden als 0.2.0-alpha.3 gekennzeichnet. Native Windows-/Linux-Neubauten und Paketinstallationsprüfungen sind vor Auslieferung abzuwarten. Physische Controller, Bazzite/Wayland/Gamescope und tatsächliche Pokémon-Spielwelt bleiben davon getrennte offene Abnahmen.

## Controller-Menü und Spielhilfe 0.2.0 Alpha 4

Der aktive Nachtest von Alpha 3 reproduzierte zwei weitere Fehler: Nach ausdrücklicher Zuordnung steuerte weiterhin das erste SDL-Gerät das Menü, und die einzeilige Bedienhilfe wurde bei 1280 × 720 abgeschnitten. Beide Korrekturen gelten im gemeinsamen Code für Linux und Windows.

Die Menüabfrage gehört nach der ersten erfolgreichen Live-Zuordnung ausschließlich Spieler 1. Ein abgezogener Spieler-1-Controller bleibt ohne Eingabe; andere Geräte und neue Ersatzgeräte übernehmen nicht automatisch. Auch eine fehlgeschlagene Neuzuordnung schaltet den anfänglichen Geräte-Fallback nicht wieder frei. Vor der ersten Live-Zuordnung bleibt die ursprüngliche Navigation zur Einrichtung erhalten. Nach ausdrücklicher Neuzuordnung funktioniert die Menübedienung wieder.

Die Bedienhilfe verwendet zwei echte Qt-Textlabels mit Wortumbruch. Jeder Spielerbereich reserviert die anhand seiner Breite benötigte Höhe und hält die DS-Bilder oberhalb. Lokale native Regressionen bestätigen vollständigen Text bei 360p/720p/768p/1080p/4K in beiden Spieleraufteilungen, verfügbaren Bildbereich, fremde/Spieler-2-Eingaben ohne Aktion, Menü-Navigation durch Spieler 1, Trennung mit verschobenen Geräteindizes, keine Ersatzübernahme und erfolgreiche ausdrückliche Neuzuordnung.

Lokal bestehen beide CTest-Ziele und alle 38 Python-Tests. Der isolierte Aktivtest mit produktiven Widgets, virtuellen SDL-Controllern und zwei echten Emulatoren bestätigt beide neuen Korrekturen einschließlich fortlaufender Sitzung, Eingabe nach Neuzuordnung und Menü-Beenden mit Freigabe der Sperre. Die nativen Tests verwenden für Sitzungen ausdrücklich Test-Worker; der zusätzliche Aktivtest verwendet selbst erzeugte ARM-ROMs und die echten Emulator-Binaries des letzten nativen Linux-Pakets. Native Windows-/Linux-Neubauten und Installationsprüfungen sind vor Auslieferung erforderlich. Physische Controller, Bazzite/Wayland/Gamescope, Pokémon-Spielstände und die gemeinsame Sinnoh-Welt bleiben getrennte offene Abnahmen.


## Nachuntersuchung der ROM-Auffälligkeiten, 16.10.2026

Die unveränderte Nutzer-ROM reproduzierte die leere Zielauswahl in einem zweiten gemeinsamen Wildkampf. Die passende Cursorinitialisierung im Kampf-Overlay wurde mit dem ursprünglichen PRET-Code abgeglichen. Ein separates Python-Werkzeug erzeugt eine neue, hashgebundene ROM-Kopie mit Prüfung auf weiterhin verfügbare gespeicherte Ziele. Original-ROM und Saves bleiben erhalten; neue ROM-Hashes erzeugen eigene Profile. Die Lokalisierung selbst behält weiterhin ihre bisherigen Programmbyte-Invarianten.

Alle 46 Python-Tests einschließlich Ausführung der tatsächlichen neuen Thumb-Instruktionen in 128 Zustandskombinationen bestehen lokal; beide vorhandenen nativen CTest-Ziele bestehen ebenfalls. Der zusätzliche ARM-Test prüft Register-/Stackerhaltung und ausschließlich die beabsichtigten Cursor-Schreibzugriffe; eine eigene Linux-/Windows-CI macht ihn verpflichtend. Die Begleiter wurden ohne Codeänderung beim Weitergehen wieder sichtbar und blieben nach einem weiteren gemeinsamen Kampf sichtbar. Ein dauerhafter Verlust wird nicht mehr als bestätigter Fehler geführt.

Der aktive Korrektur-Nachtest mit zwei echten Alpha-4-Emulatorprozessen bestand: nach dem ersten besiegten Staralili sprang der Cursor beider Spieler auf das verbleibende Staralili; A wurde ohne zusätzliche Richtungseingabe angenommen. Gemeinsame Runde, Sieg, EP und beiderseitiger Aufstieg auf Lv. 7 sowie Rückkehr mit sichtbaren Begleitern wurden geprüft. Der Codefix gilt in der ROM für alle Plattformen; dieser Spielnachweis stammt ausschließlich von Linux. Ingame-Speichern/-Laden der korrigierten ROM ist ebenfalls mit einer regulären Wiederholung und geänderter Position beider Figuren bestätigt. Ein zuvor zu früh beendeter Speichervorgang im Testtreiber lud den alten Checkpoint; daraus wird kein Produktfehler abgeleitet. Alle Sitzungen endeten geordnet ohne verbleibende Worker oder Sperre. Details und Fingerabdrücke in der verlinkten Korrekturdokumentation.


## Launcher im Handheld-/Retro-Stil, 0.2.0 Alpha 5

Auf ausdrücklichen Wunsch des Nutzers: modernere native Qt-Oberfläche mit stärkerem Pokémon-/Retro-Bezug. Eingebettete Silkscreen-Überschriften (SIL OFL 1.1), gut lesbare Systemschrift für Bedienelemente, dunkles Gehäuse, helle Spiel-/Controllerinformationen, roter Start und goldene Fokusmarkierung. Eigene geometrische Symbole, keine Nintendo-Grafiken oder -Schriften. Alle Funktionen sind bei 1280 × 720 ohne Scrollen erreichbar. Schmale Fenster stapeln die Bereiche und behalten Fokus-Scrollen; 4K nutzt größere Schrift und Bedienelemente. Dialoge folgen derselben Gestaltung; Abbrechen/Schließen werden deutsch beschriftet.

Produkt- und Designkontext stehen in PRODUCT.md, DESIGN.md und .impeccable/design.json. Der Quellcode der Gestaltung ist gemeinsam für Windows und Linux; bestehende Controller-/Spielstand-/Sitzungslogik bleibt erhalten. Die Bildschirmanzeige trägt Koralle/Cyan für Spieler 1/2 und erhält den vollständigen umgebrochenen Hilfetext.

Lokal erfolgreich: beide nativen CTest-Ziele (inklusive Controllerbesitz, Trennung, Fokus, Minimieren, blockiertem ROM-Wechsel und Session-Abschaltung) sowie alle 46 Python-Tests mit echtem xdelta-Decoder und ARM-Regressionsengine. Zusätzliche native Prüfung bestätigt Font-Einbettung, passende Beschriftungsbreiten und Aktivierung per Leertaste entsprechend dem angezeigten Tastaturhinweis. Gebündelte X11-Aufnahmen prüfen 360p, 720p, 1080p, 4K, eingerichteten/leeren Zustand und einen tatsächlichen Dialog. Zwei echte Worker mit selbst geschriebenem ARM-Homebrew liefern die Frames für beide Spieleraufteilungen; beide enden geordnet und geben die Sperre frei. Diese roten/grünen Testbilder sind ausdrücklich kein Pokémon-Spielnachweis.

Die unabhängige Designkontrolle gibt das geprüfte Hauptmenü frei. Native Windows-/Linux-Neubauten, Installationsprüfungen und Paketausgabe werden vor Auslieferung separat abgewartet. Physische Controller, tatsächliche Bazzite-/Wayland-/Gamescope-Bedienung und Pokémon-Dauertests bleiben offen. Der separate Zielfix der ROM gehört nicht in die Installer und wird nicht durch ein Launcher-Update automatisch angewendet.

## Koop-KP-Synchronisierung und Geldtest, 19.10.2026

Der aktive Trainerkampf-Nachtest mit der bisherigen deutschen Zielfix-1-ROM reproduziert unterschiedliche KP trotz weiterhin verbundener Worker und beiderseits beendeter Runde. Der Partner verarbeitet einen Trank über ein zusätzliches ROM-Unterprogramm; dessen einseitiger KP-Netzwerkabgleich verschiebt die folgenden KP-Sequenzen. Der neue, separat hashgebundene [Koop-Kampffix 1](rom-coop-item-fix.md) filtert genau diesen direkten Partner-Item-KP-Unteraufruf. Die native Heilung bleibt zuständig; der ursprüngliche Item-Ausschluss beim allgemeinen KP-Import und alle anderen Freigabeprüfungen bleiben erhalten. Geschrieben werden zwei selbst verfasste Codebereiche von insgesamt 116 Byte Länge im Overlay 122; ROM, Saves und Nintendo-Code sind nicht Bestandteil des Git-Pakets.

Lokal bestehen alle 55 Python-Tests mit echtem xdelta-Decoder und ARM-Engine ohne Skips. 320 neue ARM-KP-Importfälle, 240 Filter-/ABI-Fälle und die bestehenden 128 Cursorfälle sind enthalten. Unabhängiger Assembler, gespeicherte eigene Assembly und Standardbibliothek-Patchwerkzeug erzeugen exakt denselben Code und ROM-Fingerabdruck. Die dedizierte ROM-CI verlangt diese Tests unter Windows und Linux.

Zwei tatsächliche Alpha-5-Linux-Emulatorprozesse bestätigen Absorber, Tränke beider Spieler nacheinander und gleichzeitige Tränke mit übereinstimmenden KP, Statuswerten, PP und Kampfwertänderungen. Im gleichzeitigen Fall heilen die Pokémon vor einem Treffer genau 42 → 62 und 40 → 60 KP; nach dem gegnerischen Treffer stehen auf beiden Seiten 62/59 KP und dieselbe KP-Sequenz 1. Die gezielten Szenarien nutzen ausdrücklich getrennte Spielstandkopien mit vorbereitetem Level-25-Team; Geld, Beutel, Geschichte, Siegmarkierungen und Positionen wurden bei dieser Vorbereitung nicht bearbeitet.

Der separate Geldtest mit der bisherigen ROM bestätigt einen echten gemeinsamen Trainersieg über Sven: beide Spieler erhalten 96 ₽, die Kontostandsdifferenz bleibt erhalten und beide Geldstände/Siegmarkierungen überstehen reguläres Speichern und Neuladen. Ein früherer regulärer Level-6-Test bestätigt bei einer Niederlage getrennte Abzüge von je 48 ₽. Beide eigenen Begleiter waren im Siegszenario unmittelbar nach der Rückkehr vor einer Bewegung sichtbar; dafür wurde kein Code geändert. Original-ROM und Originalspielstände sind unverändert. Tatsächliches Pokémon-Gameplay dieser Prüfung stammt von Linux mit virtuellen SDL-Controllern; native Windows-/Bazzite-Spielabnahme, physische Controller und ein vollständiger Durchlauf bleiben offen. Die Alpha-5-Launcherprogramme wurden für diese ROM-Korrektur nicht geändert.


## Release-Vorbereitung 0.2.0 Alpha 6,19.10.2026

Die geprüften Launcher-/Plattform-/ROM-Korrekturen aus PR1 sind nach main übernommen. Die englische GitHub-Startseite mit deutscher Kurzfassung verwendet echte, mit Herkunft und Testgrenzen dokumentierte Aufnahmen. Die Alpha 6-Paketversionen und Release-Notizen sind abgestimmt. Der zusammengeführte Veröffentlichungsjob verlangt native Linux-/Windows-Jobs und eine verpflichtende ROM-ARM-Matrix; der Linux-Haupttest installiert die ARM-Abhängigkeit ausdrücklich. Veröffentlichung nur bei markiertem main-Commit, vollständigem Upload und übereinstimmenden Assetnamen/-größen. Bereits veröffentlichte Releases werden nicht überschrieben; passende unvollständige Entwürfe können fehlende Assets ergänzen. ROM-Korrekturwerkzeuge werden über eine feste eigene Quell-Allowlist ohne Spieldaten gepackt. Die zusätzliche Paketprüfung deckt fremde Dateien, Prüfsummen, Reproduzierbarkeit und unvollständige Quellen ab. Tatsächliche Veröffentlichungs-/CI-Nachweise werden nach Abschluss separat festgehalten.


## Launcher-Sprachauswahl — 19.10.2026, Alpha 7

Deutsch und Englisch im nativen Qt-Launcher: zweisprachige Erstauswahl vor ROM-Einrichtung, gespeicherte Wahl in bestehenden Profilen und sofortiger Wechsel unter Einstellungen. Die ROM-Sprache, Firmware-Benutzernamen, Spieler-/ROM-Verzeichnisse und Worker bleiben unabhängig davon. Kompakte Einstellungen scrollen, statt mehrzeilige Hinweise abzuschneiden.

Lokal bestanden: native Komponenten- und Produktions-UI-Tests mit virtuellen SDL-Controllern; DE/EN-Menüs und Spielhilfe bei 640×360, 720p, 768p, 1080p und 4K in beiden Teilungen; Erstauswahl, Abbrechen, Neustart, ungültige Sprache, beschädigtes JSON ohne Überschreiben, fehlgeschlagene atomare Speicherung und Erhalt unbekannter/älterer Felder. Umschalten bei laufenden TEST-ONLY-Workern erhält Sitzungssperre und denselben Laufzeitordner; anschließendes sauberes Beenden gibt die Sperre frei. Alle 57 Python-Tests bestanden ohne Skip mit echtem xdelta/Unicorn. Zwei echte Emulatorprozesse führten eigenes ARM-Homebrew aus, verbanden sich und beendeten kooperativ. Der Import des vorhandenen echten korrigierten ROM-ZIPs erhielt dessen SHA-256 unverändert.

Native UI-Aufnahmen prüfen Auswahl, beide Menüs, Einstellungen und Hilfe; dies ist kein neuer Pokémon-Spielablauf und keine physische Controller-/Bazzite-/Windows-Spielabnahme. Veröffentlichte Plattformpakete setzen die nativen CI-Neubauten und Installationsprüfungen voraus. System-Dateidialoge und rohe Fremd-Logs können der Betriebssystem-/Originalsprache folgen.


## CouchSplit — 19.10.2026, Alpha 8

Auf Nutzerwunsch heißt das Projekt CouchSplit; der Name betont Couch-Koop und Splitscreen. Aktuelle Spielunterstützung bleibt Project PM / Pokémon Platin. Sichtbare Launcher-, Dialog-, Installer- und Startmenünamen sowie künftige Download-Dateien werden umgestellt. Profile, ROM-Hashes, Speicherpfade, Controllerzuordnung, Laufzeitbesitz und bestehende Installationskennungen bleiben erhalten. Die Windows-Installation ersetzt ausschließlich ihre bekannten alten Programmverknüpfungen.

Lokal geprüft: native Komponenten-/UI-/Controller-/Sitzungstests in Deutsch/Englisch; 57 Python-Tests ohne Skip mit echtem xdelta/Unicorn; zwei echte Homebrew-Emulatorprozesse mit sauberem IPC-Ende. Neue native Bildschirmaufnahmen zeigen CouchSplit bei 640×360, 720p, 768p, 1080p und 4K; kein neuer Pokémon-Spielablauf. Der Import des tatsächlichen korrigierten ROM-ZIPs behält seinen SHA-256. Die CI ergänzt echte Alpha-7-zu-8-Upgrades unter Windows und Debian/Ubuntu mit bekannten alten Pakethashes, erhaltenen eigenen Testprofil-/Spielstandbytes und Deinstallation ohne Datenverlust. Plattformpakete werden erst nach diesen Prüfungen veröffentlicht. Physische Controller, Bazzite-Zielhardware und neue Pokémon-Spielabnahmen sind dadurch nicht nachgewiesen.

## Veröffentlichter Stand — 20.10.2026

[CouchSplit Alpha 8](https://github.com/Philmenting/couchsplit/releases/tag/v0.2.0-alpha.8) ist veröffentlicht, Commit `07025b313eab58f7afdb1fb9f903219e5961c01d`. Native Windows-/Linux-Prüfungen, echte Alpha-7-zu-8-Upgrades mit erhaltenen eigenen Testeinstellungen/-spielständen, ROM-freie Builds und ARM-Regressionen bestanden. Alle sechs öffentlichen Downloads wurden geladen und gegen API-Prüfsummen/SHA256SUMS geprüft; alle 126 versionierten Projektdateien stimmen in Windows-, Bazzite- und Debian-Quellpaketen mit diesem Release überein. Bazzite --check prüfte 911 Dateien ohne Hostinstallation.

[Native Plattformen und Veröffentlichung](https://github.com/Philmenting/couchsplit/actions/runs/39480215694) · [Build/ROM-freie Tests](https://github.com/Philmenting/couchsplit/actions/runs/39480215786) · [ARM unter Windows/Linux](https://github.com/Philmenting/couchsplit/actions/runs/39480215738). Die älteren Abschnitte dokumentieren ihren damaligen Stand. Trainer-Koop und der Geldtest wurden am 19.10.2026 nachgeprüft; frühere offene Aufgaben dazu sind historisch. Physische Zielhardware, Tausch und vollständiger Spieldurchlauf bleiben offen. Die Umbenennung enthält keinen neuen Pokémon-Spieltest.
