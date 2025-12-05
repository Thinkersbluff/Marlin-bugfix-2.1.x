#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/.."
cd "$REPO_ROOT"

# Activate venv (fallback to using activate_venv.sh)
if [ -f ".venv/bin/activate" ]; then
  # shellcheck disable=SC1091
  source .venv/bin/activate
else
  echo "No .venv found. Run: python3 -m venv .venv && source .venv/bin/activate && pip install -U platformio"
  exit 1
fi

# Verify pio available
if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO (pio) not found in activated environment. Install with: pip install -U platformio"
  exit 1
fi

LOG="pio_test_queue.log"
echo "Running native test: test_queue -> $LOG"
pio test -e linux_native_test -f test_queue -vvv | tee "$LOG"

# Copy log into test/ for easy reference (non-fatal)
cp -v "$LOG" test/ 2>/dev/null || true
