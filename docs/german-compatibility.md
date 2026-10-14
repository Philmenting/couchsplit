# Deutsche Ausgabe: Patchbeschaffung und Kompatibilität

Stand 14.10.2026. **Keine deutsche Project-PM-Koop-ROM erstellt oder als funktionsfähig bestätigt.** Die deutsche Original-ROM `CPUD`, Revision 0, wurde bereits in zwei realen Emulatoren gestartet; siehe [ROM-Startprüfung](rom-boot-check.md). Die fehlende Komponente ist weiterhin der ROM-seitige Multiplayer-Mod.

**Update nach Dateibereitstellung:** Das Paket `Build V0.4.5.zip` liegt jetzt vor. Seine Anleitung verlangt USA Revision 1; der echte xdelta-Test auf der deutschen Original-ROM scheitert mit einem Prüfsummenfehler. Die direkte Patchbeschaffung ist damit erledigt, aber ein CPUD-Port und die ROM-Mod-Quelle fehlen weiter. [Vollständiger Pakettest](project-pm-045.md). Die folgende öffentliche Quellenrecherche beschreibt den Stand vor diesem Upload.

**Weiteres Update:** Die passende US-Rev-1-Basis wurde ebenfalls bereitgestellt und PM 0.4.5 erfolgreich mit aktiven xdelta-Prüfungen angewendet. Das englische Ergebnis startet in zwei echten Emulatorprozessen. Deutsche Sprachübernahme und tatsächliche Sinnoh-Welt-Synchronisation sind noch nicht umgesetzt beziehungsweise bestätigt.

## Erneut geprüfte Primärquellen

| Quelle | Tatsächlich gefunden |
|---|---|
| [ComicartOlie, GitHub](https://github.com/ComicartOlie?tab=repositories) | Zwei öffentliche Emulatorrepositories; kein öffentliches ROM-Mod-Repository. Project-PM-Branch weiterhin auf dem gepinnten Commit. |
| [Emulatorrelease v1.4.1-beta](https://github.com/ComicartOlie/melonDS-Project-PM/releases/tag/v1.4.1-beta) | Windows-Emulatorarchiv und README; keine xdelta/BPS-ROM-Patchdatei. |
| [Offizieller Setup Guide von ComicartOlie](https://www.youtube.com/watch?v=LgCSqOyCN3A) | Beschreibung nennt Entwickler-Discord, xdelta-Webpatcher und vier Emulatorlinks. Kein direkter ROM-Patchdownload und keine Aussage zur deutschen Basis-ROM. Beschreibung direkt aus den YouTube-Videometadaten gelesen. |
| [Neuerer Entwicklerbeitrag zur 8-Spieler-Erweiterung](https://www.reddit.com/r/PokemonROMhacks/comments/1wduy8y/project_platinum_multiplayer_8_player_expansion/) | Testteilnahme über https://discord.com/invite/bTZfjHqbVd. Keine Patchdatei oder deutsche Kompatibilitätszusage im gelesenen Beitrag. |
| [pret/pokeplatinum](https://github.com/pret/pokeplatinum) | README nennt ausschließlich US-Builds, Revision 0 und 1. Das beweist nicht, welche Sprachversionen der nicht vorliegende Project-PM-Mod unterstützt. |
| [Vom Tutorial verlinkter Linuxport](https://github.com/fdelavega02/melonDS-Project-PM/releases/tag/project-pm-linux-v1.4.1-beta) | x86_64-AppImage des Emulators; weder ROM-Patch noch ROM-Mod-Quelle in den geprüften Release-/Tree-Daten. Kein Ersatz für die ROM-Abhängigkeit. |

Öffentliche Repositorysuche nach `pokeplatinum multiplayer` fand keinen passenden Mod-Quellcode. Diese Recherche belegt, was öffentlich geprüft wurde; sie beweist nicht, dass nirgendwo eine weitere Quelle existiert. Der öffentliche Discord-Einladungsendpunkt war aus dieser Ausführungsumgebung nicht lesbar (HTTP 403); Serverkanäle/Anhänge wurden nicht gelesen. Keine Nachricht an Entwickler gesendet und keine Nintendo-ROM aus dem Internet beschafft.

## Was für einen deutschen Port fehlt

Erforderlich ist entweder ein für `CPUD` veröffentlichter Patch samt exakter Basis-/Ergebnis-Prüfsumme oder der ROM-Mod-Quellcode mit Nutzungsbedingungen und Buildanleitung. Ein US-xdelta-Patch ist nicht automatisch ein deutscher Port. Die Emulator-Bridge findet ihre Mailbox dynamisch über Discovery-Magics; im geprüften Bridgecode gibt es dafür keine CPUE-Sprachvoraussetzung. Der ROM-Mod muss diese Mailbox jedoch selbst bereitstellen und mit seinem Spielcode verbinden.

Mit der ROM-Mod-Quelle können die deutschen Spielressourcen, ROM-Hooks/Overlays und zusätzlichen Multiplayertexte angepasst und getestet werden. Ohne diese Quelle lassen sich die fehlenden Funktionen nicht aus dem Emulatorrelease rekonstruieren. Nur die Headerkennung zu ändern oder xdelta-Prüfungen abzuschalten erzeugt keine belastbare Sprachunterstützung.

## Implementierte lokale Patchvorbereitung

`scripts/prepare-rom.py` verwendet den vorhandenen xdelta3-Decoder. Es prüft ausdrücklich:

- deutsche `CPUD`-Kennung bei unbekannten deutschen Patches; für den verifizierten US-PM-Build 0.4.5 die katalogisierten `CPUE`-Kennungen;
- erwartete SHA-256-Werte von Basis-ROM, Patch und Ergebnis;
- vorhandene Zieldateien werden nicht überschrieben;
- Original-ROM bleibt erhalten, Ausgabe mit privaten Dateirechten außerhalb des Repositories;
- fehlerhafte oder unpassende Patches veröffentlichen keine teilweise erzeugte Datei.

Die erwarteten Prüfsummen müssen aus vertrauenswürdigen Release-Metadaten oder einem reproduzierbar geprüften Build stammen. Fehlende Werte sind ein Blocker; sie werden nicht durch frei gewählte Werte ersetzt. CPUD ist nur eine Kennung, kein Beweis deutscher Texte oder funktionierender Multiplayerlogik. Dieses Hilfsprogramm enthält **keinen Project-PM-ROM-Patch** und portiert keine englische Version automatisch.

Bazzite: Das Buildskript installiert das Fedora-Paket `xdelta` ausschließlich in Distrobox; dieses liefert `xdelta3`. Auf Ubuntu heißt das Paket `xdelta3`. Sobald ein tatsächlich passender deutscher Patch vorliegt:

```bash
distrobox enter platinum-coop-build -- python3 scripts/prepare-rom.py \
  --rom /pfad/zur/eigenen-deutschen.nds \
  --patch /pfad/zum/geprueften-deutschen.xdelta \
  --output /pfad/ausserhalb-des-repos/platin-project-pm-de.nds \
  --source-sha256 9ffc28ff2b2d8fab60454548f92e60dd5d44f0dcc456e6e00e756424b3259217 \
  --patch-sha256 "$PATCH_SHA256" --target-sha256 "$TARGET_SHA256"
```

Der Beispielaufruf setzt einen Patch voraus, dessen dokumentierte Basis exakt diese deutsche Original-ROM ist. Die beiden Variablen müssen die echten verifizierten Releasewerte enthalten. Anschließend mit `tests/verify_user_rom.py` starten und die [Spielweltabnahme](acceptance-tests.md) durchführen: deutsche Einführung/Namen/Dialoge, beide Spielfiguren, Bewegungen/Interaktionen, Speichern/Laden und Wiederverbinden. Erst nach diesen Tests darf deutsche Koop-Unterstützung behauptet werden.

## Automatisierte Tests und verbleibender Blocker

Die Patchvorbereitung wird mit echten xdelta3-Encoding-/Decoding-Vorgängen auf selbst erstellten synthetischen Daten geprüft. Keine Nintendo-Spielbytes in Tests. Prüfungen umfassen erfolgreiche Rekonstruktion, Pfade mit Leerzeichen, Originalerhalt, Schreibrechte, Verweigerung vorhandener Ziele, falsche Basis-/Patch-/Ergebnis-Prüfsummen, englische Kennungen, kaputte Patches und fehlenden Decoder. Das ist ein Test der Patchvorbereitung; **kein Test eines deutschen Project-PM-Patches**.

Der nun bereitgestellte Patch 0.4.5 hat als Basis CPUE USA Revision 1. Er passt nicht auf die deutsche Datei. Die ebenfalls bereitgestellte passende US-Basis wurde erfolgreich gepatcht und auf Start-/TCP-Funktion geprüft. Für Deutsch ist weiterhin eine Lokalisierung des US-PM-Ergebnisses oder ein nativer CPUD-Port zu entwickeln. Ein SHA-1-/Kennungsabgleich ist mit `scripts/prepare-rom.py --inspect` möglich.
