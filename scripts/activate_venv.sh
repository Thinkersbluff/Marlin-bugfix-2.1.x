#!/usr/bin/env bash
set -euo pipefail

# Resolve paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/.."
cd "$REPO_ROOT"

# Activate repository venv
if [ -f ".venv/bin/activate" ]; then
  # shellcheck disable=SC1091
  source .venv/bin/activate
  echo "Activated .venv in $REPO_ROOT"
else
  cat <<'EOS'
No .venv found in the repository.
To create it, run:

  python3 -m venv .venv
  source .venv/bin/activate
  pip install --upgrade pip setuptools wheel
  pip install -U platformio

After that re-run this script.
EOS
  exit 1
fi
