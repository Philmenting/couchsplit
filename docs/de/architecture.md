# Architektur

[English](../architecture.md) · **Deutsch** · [Anleitungen](README.md)

Der Launcher ist ein C++17/Qt6-Programm. Er verbindet vorhandene Emulatorfunktionen mit einer kleinen Linux-IPC-Schnittstelle. Beide unabhängigen Emulatoren behalten ihre eigene EmuInstance, EmuThread, DS-Hardware, Audio und SaveManager.

````mermaid
flowchart TD
    UI["Qt Launcher / Split-Screen"] --> Input["SDL Controller-Zuordnung"]
    UI --> Manager["Prozesse und Profilsperre"]
    Input --> A["Spieler 1 / Project PM"]
    Input --> B["Spieler 2 / Project PM"]
    Manager --> A
    Manager --> B
    A <-->|"TCP loopback / Wire v2"| B
    A -->|"RAM-Bilddaten / mmap"| UI
    B -->|"RAM-Bilddaten / mmap"| UI
    A --> S1["Eigene Saves"]
    B --> S2["Eigene Saves"]
````

| Modul | Implementierung | Verantwortung |
|---|---|---|
| Emulator Manager | launcher/Session.h | QProcess, Capability-Prüfung, Fehlerüberwachung, SIGTERM, Parent-death-Signal |
| Session Manager | Project-PM-Patch + multiplayer/SocketCompat.h | tatsächliche TCP-Bridge, localhost, POSIX-Portierung, Rollen, Wiederverbindung |
| Controller Manager | controller/Controllers.h | SDL GameController, live instance IDs, neutrale Eingaben bei Trennung, Stick-Touch, Mapping |
| Darstellung | display/Layout.h, display/CoopIPC.h, PlayView | 4:3-Geometrie, mmap-Bilddaten, gemeinsame Qt-Fläche, Cursor |
| Save Manager | saves/SaveStore.h + melonDS SaveManager | Profilsperre, Backup/Restore mit SHA-256, tatsächliches Speichern durch Emulator |
| Launcher UI | launcher/main.cpp | Einrichtung, Sitzungsmenü, TV-Layout, Logs |

## Warum keine Fensterpositionierung?

CouchPlay steuert KDE über temporäre KWin-Skripte, startet verschachteltes Gamescope und isoliert Geräte über einen privilegierten Helfer. KWin steht im Steam Gaming Mode nicht generell zur Verfügung, Wayland erlaubt keine allgemeine Fremdfensterpositionierung. Eine X11-Einbettung wäre ebenfalls keine native Wayland-Lösung. Der vorhandene melonDS-SoftRenderer liefert bereits beide Framebuffer. Diese direkt in einer gemeinsamen Qt-Oberfläche zu zeichnen vermeidet Fremdfenstersteuerung und einen eigenen Compositor.

Der Baseline-Renderer bleibt **softwarebasiert** (DS-Auflösung 256 × 192 pro Bildschirm), Qt übernimmt die hochauflösende Ausgabe. Hardwarebeschleunigte DS-3D-Upscaling-Texturen werden nicht transportiert. Bildqualität und Leistung auf dem Ziel-PC sind noch zu prüfen.

## IPC und Eingaben

Pro Sitzung existiert ein zufälliger temporärer Ordner mit Modus 0700. Die beiden Mapping-Dateien haben Modus 0600, O_EXCL/O_NOFOLLOW und überprüfte Größe/ABI. Lockfreie 32-Bit-Atomics übertragen Tasten, Touch und Sitzungsstatus. Ein Try-Lock schützt die Bildkopie; belegte Frames werden übersprungen, damit das Spiel weiterläuft. Stürzt ein Prozess mitten in der Bildkopie ab, beendet die Prozessüberwachung die Sitzung und entfernt die Mapping-Dateien.

SDL liest ausschließlich im Launcher. Die Worker setzen beim IPC-Betrieb ihren DS-KeyMask direkt aus der jeweils eigenen Mapping-Datei und ignorieren ihre regulären Tastatur-/Controller-Eingaben. Eine getrennte Bluetooth-Verbindung liefert neutrale Eingaben; eine neue SDL-Instanz muss ausdrücklich erneut zugeordnet werden. Ein GUID ist keine eindeutige physische Kennung, insbesondere bei zwei baugleichen Geräten ohne Seriennummer.

## Netzwerk

Der Launcher wählt einen freien lokalen TCP-Port und übergibt ihn beiden Workern. Project PM hostet mit PMCOOP_ROLE nur auf 127.0.0.1; der Client verbindet dieselbe Adresse. Discovery-Broadcast und Online-Relay sind in diesem Modus deaktiviert. Die kleine Zeitspanne zwischen Portfreigabe und Host-bind bleibt ein mögliches Rennen; Fehler werden über Status/Logs sichtbar. Das ist kein Internet-Netplay.

Wire v2: `u16 LE length` plus Payload; Bridge-Tags 1 (Block/Overworld), 2 (Party), 3 (Paket); Kontrollframes 0xFF (Rolle), 0xFE (Name), 0xFD/0xFC (Ping/Pong). Die ROM muss ihre Discovery-/Kontrollblöcke veröffentlichen und ihren Multiplayer-Modus aktivieren. Ein Socket-Peer und ein aktiviertes ROM-Kontrollbyte beweisen keine Welt-Synchronisation.

## Speicher und Lebenszyklus

Standardwurzel: `$XDG_DATA_HOME/pokemon-platinum-local-coop` oder `~/.local/share/pokemon-platinum-local-coop`. Je Spieler und ROM-SHA-256 gibt es ein eigenes Profil. Das verhindert gleichzeitiges Schreiben beider Spieler und Kollisionen gleichnamiger ROMs. Ein anderer Patch/ROM-Hash erhält ein neues Profil; bestehende Saves müssen bewusst übernommen werden.

Der Launcher sperrt die gesamte Datenwurzel über QLockFile, sichert *.sav vor dem Start und schreibt Metadaten atomar. Wiederherstellung prüft das Manifest vollständig und sichert zuerst den aktuellen Stand. Backups werden nicht automatisch gelöscht. SIGTERM wird in den Qt-Eventloops verarbeitet; EmuInstance/SaveManager führen die Upstream-Aufräumarbeiten aus. Erzwungenes SIGKILL kann ungespeicherten Fortschritt verlieren. Kein automatischer Neustart eines abgestürzten Spiels.
