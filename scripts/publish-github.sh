#!/usr/bin/env bash
# Run explicitly with an authenticated GitHub CLI. Defaults to private.
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
command -v gh >/dev/null || { echo 'GitHub CLI (gh) wird benötigt.' >&2; exit 1; }
gh auth status
if [[ -n "$(git -C "$project_dir" status --porcelain)" ]]; then
  echo 'Zuerst lokale Änderungen prüfen und committen.' >&2; exit 1
fi
visibility=--private
if [[ "${1:-}" == --public ]]; then visibility=--public; elif [[ $# -ne 0 ]]; then echo 'Aufruf: publish-github.sh [--public]' >&2; exit 2; fi
if git -C "$project_dir" remote get-url origin >/dev/null 2>&1; then
  echo 'origin ist bereits gesetzt. Ziel vor einem manuellen Push prüfen.' >&2; exit 1
fi
gh repo create pokemon-platinum-local-coop "$visibility" --description 'Two-player Project PM / melonDS couch launcher for Bazzite Linux; shared-world gameplay pending ROM verification' --source "$project_dir" --remote origin --push
