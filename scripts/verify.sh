#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ctest --test-dir "$project_dir/build/launcher" --output-on-failure
python3 -m unittest discover -s "$project_dir/tests" -p 'test_*.py'
python3 "$project_dir/tests/test_transport.py" --source "$project_dir/.cache/project-pm"
python3 "$project_dir/tests/test_save_paths.py" --source "$project_dir/.cache/project-pm"
python3 "$project_dir/tests/test_config_threads.py" --source "$project_dir/.cache/project-pm"
QT_QPA_PLATFORM=offscreen SDL_AUDIODRIVER=dummy "$project_dir/build/bin/platinum-coop" --data-dir "${TMPDIR:-/tmp}/platinum-coop-ui-check" --smoke-test "${TMPDIR:-/tmp}/platinum-coop-ui-check.png" --smoke-size 3840x2160
python3 "$project_dir/tests/smoke_emulator.py" "$project_dir/build/bin/melonDS-coop"
