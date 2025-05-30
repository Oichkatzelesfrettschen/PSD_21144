#!/usr/bin/env bash
# Root setup script for 2.11BSD_X44.
# It configures the environment and attempts a basic build of bmake.

set -euo pipefail

# Determine the directory of this script.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Configure the build environment.
# Add the "universe" repository when missing.
if ! grep -q '^deb .\+universe' /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null; then
    sudo add-apt-repository -y universe
fi

# Refresh package lists; continue when the network is unavailable.
if ! sudo apt-get update; then
    echo "apt-get update failed; using existing package lists" >&2
fi

# Packages required by the BSD build system as well as the ack tools.
PKGS=(
    bmake
    byacc
    bison
    flex
    build-essential
    ack
    ack-dev
    ack-clang
)

# Install each package if not already installed.
for pkg in "${PKGS[@]}"; do
    # Install the package when it is not already present. Continue on
    # failures so the rest of the environment can still be prepared.
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        if ! sudo apt-get install -y "$pkg"; then
            echo "Failed to install $pkg" >&2
        fi
    fi
done

# Remove relative entries from PATH to satisfy build.sh.
CLEAN_PATH=""
IFS=':'
for dir in $PATH; do
    case "$dir" in
        ''|'.')
            # Skip empty and current directory entries.
            continue
            ;;
        /*)
            CLEAN_PATH="${CLEAN_PATH:+${CLEAN_PATH}:}${dir}"
            ;;
        *)
            echo "Removed relative path entry: $dir" >&2
            ;;
    esac
done
unset IFS
export PATH="$CLEAN_PATH"

# Display the final PATH for confirmation.
echo "Environment configured. PATH set to: $PATH"

# Try building usr.bin/make if bmake is available.
if command -v bmake >/dev/null 2>&1; then
    cd "$ROOT_DIR/usr.bin/make"
    bmake
else
    echo "bmake is not installed; build skipped" >&2
fi

