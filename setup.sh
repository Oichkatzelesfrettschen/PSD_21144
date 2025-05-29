#!/usr/bin/env bash
# Root setup script for 2.11BSD_X44.
# It configures the environment and attempts a basic build of bmake.

set -euo pipefail

# Determine the directory of this script.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Load the environment setup routine.
source "$ROOT_DIR/scripts/setup_build_env.sh"

# Try building usr.bin/make if bmake is available.
if command -v bmake >/dev/null 2>&1; then
    cd "$ROOT_DIR/usr.bin/make"
    bmake
else
    echo "bmake is not installed; build skipped" >&2
fi

