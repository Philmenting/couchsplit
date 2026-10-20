# Bereitgestellte deutsche ROM: Startprüfung, 14.10.2026

[English](../rom-boot-check.md) · **Deutsch** · [Anleitungen](README.md)

Die bereitgestellte Datei wurde ausschließlich außerhalb dieses Git-Repositories entpackt und getestet. ROM, Spielbilder, Firmware, Saves und vollständige Emulatorlogs werden nicht veröffentlicht.

## Identität

- Nintendo-DS-Kennung: `CPUD`, Revision 0, 134217728 Byte.
- SHA-256: `9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217`.
- SHA-1: `d40f19ac24d67c721a4ab25e1c013154443d75b4`.
- CRC32: `37E9D9E5`.

SHA-1, CRC32, Größe und Kennung stimmen mit dem Eintrag **Pokemon - Platin-Edition (Germany)** in den [No-Intro-Metadaten von libretro](https://github.com/libretro/libretro-database/blob/master/metadat/no-intro/Nintendo%20-%20Nintendo%20DS.dat) überein. Das ist die unveränderte deutsche Original-ROM, keine Project-PM-Multiplayer-ROM. Die App erkennt genau diesen Original-Fingerabdruck und erklärt den fehlenden Patch schon bei der Auswahl. Andere Fingerabdrücke sind kein Beleg für Patchkompatibilität; deren ausdrückliche Bestätigung bleibt erforderlich.

## Tatsächlicher Test

Umgebung wie in [Entwicklungsstatus](development-status.md): Ubuntu-Container, Qt offscreen, Dummy-Audio, Software-3D, freie melonDS-BIOS-Implementierung und generierte Firmware. Zwei echte gepatchte Project-PM-Prozesse, getrennte Profile, getrennter Save-Pfad, lokaler TCP-Port. Laufzeit 35 Sekunden. Zwischen Sekunde 10 und 20 nur für Spieler 1 A-Tastendrücke über die produktive IPC-Schnittstelle. Keine physischen Controller.

| Beobachtung | Ergebnis |
|---|---|
| Veröffentlichte Frames | Spieler 1: 2050, Spieler 2: 2053 |
| Framefortschritt nach Sekunde 5 | 1796 bzw. 1797 weitere Frames |
| TCP-Verbindung | Beide verbunden, Rollen 1/2 |
| ROM-Bridge aktiviert | Auf beiden Seiten während des gesamten Tests false |
| Verschiedene Spielzustände | Spieler 1 zeigt Einführungstext, Spieler 2 das Pokémon-Intro; Bilder visuell geprüft |
| Gemeinsames Beenden | SIGTERM, beide Exitcode 0 |

Das bestätigt ROM-Ausführung, fortlaufende Bildausgabe, adressierte IPC-Eingaben, Loopback-Transport und geordnetes Beenden. Es bestätigt weder vollständige Bild-/Spielkompatibilität noch Ingame-Speichern/Laden, Sinnoh-Welt-Synchronisation, 4K-Performance oder Bazzite-Hardwarebetrieb. Ohne Patch kann die Original-ROM keine Project-PM-Weltdaten bereitstellen. Ein erfolgreicher TCP-Link allein ändert das nicht.

## Wiederholen

Opt-in-Test mit eigener Datei; Ausgabeordner muss neu und außerhalb des Repositories sein. Dort entstehen private Logs und DS-Spielbilder. Das Skript kopiert keine ROM und startet keine öffentliche Netzwerkverbindung.

```bash
python3 tests/verify_user_rom.py build/bin/melonDS-coop /pfad/zur/eigenen.nds \
  --output /pfad/ausserhalb-des-repos/privater-boot-test --seconds 35
```

Die automatisierten CI-Tests bleiben ROM-frei. Dieser zusätzliche Test benötigt eine rechtmäßig bereitgestellte Datei. Eingabesequenzen sind eine Startprüfung und kein universeller automatischer Spielablauf.

## Fehlende Abhängigkeit

Benötigt wird der zur Emulator-Bridge passende Project-PM-ROM-Patch samt Patchanleitung und verlangter Basis-ROM-Prüfsumme. **Unterstützung der deutschen Ausgabe ist noch nicht bestätigt.** Nicht blind einen Patch für eine andere Sprachversion anwenden. Der Autor ComicartOlie verweist im [Project-PM-Testaufruf](https://www.reddit.com/r/PokemonROMhacks/comments/1vlgnlw/project_platinum_multiplayer_looking_for_more/) auf den Entwickler-Discord für Testteilnahme: https://discord.gg/W6MRVgcKtQ. Das untersuchte Emulatorrelease liefert keinen ROM-Patch. Eine frei zugängliche, geprüfte Patchdatei wurde bislang nicht gefunden. Mit einer passenden vorbereiteten ROM ist als Nächstes die Spielweltabnahme nach [acceptance-tests.md](acceptance-tests.md) durchzuführen.
