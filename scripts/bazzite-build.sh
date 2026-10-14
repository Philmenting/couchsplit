#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
box="platinum-coop-build"
command -v distrobox >/dev/null || { echo 'Distrobox wird benötigt.' >&2; exit 1; }
if ! distrobox list --no-color | awk -F'|' '{gsub(/ /,"",$2);print $2}' | grep -qx "$box"; then
  distrobox create --name "$box" --image registry.fedoraproject.org/fedora:44 --yes
fi
# sudo runs inside the disposable container, never on the Bazzite host.
distrobox enter "$box" -- sudo dnf install -y gcc-c++ cmake ninja-build git pkgconf-pkg-config extra-cmake-modules SDL2-devel libarchive-devel enet-devel libzstd-devel faad2-devel qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtmultimedia-devel qt6-qtsvg-devel wayland-devel libcurl-devel libpcap-devel python3 xdelta
distrobox enter "$box" -- bash "$project_dir/scripts/build.sh"
python3 "$project_dir/scripts/install-launcher.py" --distrobox "$box"
