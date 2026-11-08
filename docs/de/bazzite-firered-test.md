# Bazzite: Feuerrot-Entwicklungsversion testen

[English](../bazzite-firered-test.md) · **Deutsch** · [Dokumentation](README.md)

Dies ist ein Test aus dem Quellcode, kein Alpha-8-Installer: Die veröffentlichte
Alpha 8 enthält den experimentellen Feuerrot-Worker noch nicht. Verwende den
Desktopmodus und deine eigene unveränderte deutsche Feuerrot-ROM. Umfang und
verbleibende Grenzen stehen in der [Feuerrot-Anleitung](firered.md). Diese
Anleitung konnte hier noch nicht auf einem Bazzite-Gerät ausgeführt werden.

Führe die folgenden Befehle in einem Terminal aus. Der neue Projektordner darf
noch nicht existieren. Die Befehle erstellen eine eigene Distrobox, laden den
aktuellen GitHub-Quellcode und verwenden einen separaten Datenordner. Die
Installation der Abhängigkeiten benötigt Internet und fragt **innerhalb des
Containers** nach Administratorrechten.

```bash
distrobox create --name couchsplit-firered-dev --image registry.fedoraproject.org/fedora:44 --yes
distrobox enter couchsplit-firered-dev -- sudo dnf install -y gcc-c++ cmake ninja-build git pkgconf-pkg-config extra-cmake-modules SDL2-devel libarchive-devel enet-devel libzstd-devel faad2-devel qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtmultimedia-devel qt6-qtsvg-devel wayland-devel libcurl-devel libpcap-devel python3 xdelta
distrobox enter couchsplit-firered-dev -- git clone --branch main https://github.com/Philmenting/couchsplit.git "$HOME/CouchSplit-FireRed"
cd "$HOME/CouchSplit-FireRed"
distrobox enter couchsplit-firered-dev -- bash "$PWD/scripts/build.sh"
distrobox enter couchsplit-firered-dev -- "$PWD/build/bin/platinum-coop" --data-dir "$HOME/.local/share/couchsplit-firered-dev"
```

Danach startet der letzte Befehl den Launcher; für spätere Starts genügt dieser
Befehl erneut im Projektordner. Wähle die ROM aus, aktiviere **Einstellungen →
Feuerrot-Koop (experimentell)** und weise, falls verfügbar,
zwei Controller zu. Prüfe Start, beide Ansichten, Eingaben, Speichern und
Neustart im Desktopmodus. Das Repository enthält weder ROM noch Spielstände. Bei
Problemen helfen Programm- und Spiellogs; ROMs oder Spielstände musst du dafür
nicht weitergeben. Der regulär installierte Launcher verwendet einen anderen
Datenordner.

Unter Einstellungen lässt sich die Tonwahl für den Testmodus prüfen. Für ein
Paar-Backup speichern beide im Spiel, beenden die Sitzung und starten sie erneut;
erst beim nächsten Start wird das Paar angelegt. Vor **Spielstände → Beide
Spieler gemeinsam wiederherstellen** die Sitzung wieder beenden. Das Backup
enthält die letzten Ingame-Spielstände, keinen gleichzeitigen Savestate.
