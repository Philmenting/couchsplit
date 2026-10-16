#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source_dir="$project_dir/.cache/project-pm"
build_dir="$project_dir/build"
commit=4d9785468bbefc8d08002f16df4d85a12f9611f7
jobs="${COOP_BUILD_JOBS:-4}"
mkdir -p "$project_dir/.cache" "$build_dir"
patch_file="$build_dir/project-pm-normalized.patch"
python3 -c 'from pathlib import Path;import sys;Path(sys.argv[2]).write_bytes(Path(sys.argv[1]).read_bytes().replace(bytes([13,10]),bytes([10])))' "$project_dir/patches/project-pm-linux-coop.patch" "$patch_file"
if [[ ! -d "$source_dir/.git" ]]; then
  git -c core.autocrlf=false clone --no-checkout https://github.com/ComicartOlie/melonDS-Project-PM.git "$source_dir"
fi
git -C "$source_dir" config core.autocrlf false
if [[ "$(git -C "$source_dir" rev-parse HEAD 2>/dev/null || true)" != "$commit" ]]; then
  git -C "$source_dir" fetch origin "$commit"
  git -C "$source_dir" checkout --detach "$commit"
fi
if git -C "$source_dir" apply --check "$patch_file"; then
  git -C "$source_dir" apply "$patch_file"
elif ! git -C "$source_dir" apply --reverse --check "$patch_file"; then
  # Upgrade a checkout already prepared by an older project release.
  python3 "$project_dir/scripts/prepare-platform.py" "$source_dir"
  python3 "$project_dir/scripts/prepare-platform.py" --check "$source_dir"
fi
python3 "$project_dir/scripts/prepare-platform.py" "$source_dir"
cp "$project_dir/multiplayer/SocketCompat.h" "$source_dir/src/frontend/qt_sdl/"
cp "$project_dir/display/CoopIPC.h" "$source_dir/src/frontend/qt_sdl/"
cmake -S "$source_dir" -B "$build_dir/emulator" -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO_RELEASE=OFF
cmake --build "$build_dir/emulator" --parallel "$jobs"
cmake -S "$project_dir" -B "$build_dir/launcher" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir/launcher" --parallel "$jobs"
mkdir -p "$build_dir/bin"
suffix=""
if [[ "${OS:-}" == Windows_NT ]]; then suffix=".exe"; fi
cp "$build_dir/emulator/melonDS$suffix" "$build_dir/bin/melonDS-coop$suffix"
cp "$build_dir/launcher/platinum-coop$suffix" "$build_dir/bin/"
ctest --test-dir "$build_dir/launcher" --output-on-failure
python3 "$project_dir/tests/test_transport.py" --source "$source_dir"
python3 "$project_dir/tests/test_save_paths.py" --source "$source_dir"
python3 "$project_dir/tests/test_config_threads.py" --source "$source_dir"
"$build_dir/bin/melonDS-coop$suffix" --coop-capabilities
