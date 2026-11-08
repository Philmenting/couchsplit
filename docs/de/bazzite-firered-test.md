# Bazzite: Feuerrot-Entwicklungsversion testen

[English](../bazzite-firered-test.md) · **Deutsch** · [Dokumentation](README.md)

Alpha 11 enthält den experimentellen Feuerrot-Worker. Für die normale Einrichtung
[das Bazzite-Paket](one-file-installer.md) installieren, die eigene unveränderte
deutsche Feuerrot-ROM wählen und **Einstellungen → Feuerrot-Koop (experimentell)**
aktivieren. [Feuerrot-Anleitung und Grenzen](firered.md). Tatsächliche Bazzite-Spiel-
und Controller-Abnahme steht weiterhin aus.

Die folgende optionale Quellcode-Anleitung nutzt einen eigenen Container und
eigene Profile. Im Desktopmodus ein Terminal öffnen; der neue Quellordner darf
noch nicht existieren. Die Paketinstallation verwendet `sudo` nur innerhalb des
Containers und benötigt Internetzugang.

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
