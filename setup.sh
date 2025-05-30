#!/usr/bin/env bash
# Root setup script for 2.11BSD_X44.
# It configures the environment and attempts a basic build of bmake.

set -euo pipefail

# Determine the directory of this script.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Configure the build environment.
# Add the "universe" repository when missing because several tools
# used during the build are provided from it.
if ! grep -q '^deb .\+universe' /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null; then
    sudo add-apt-repository -y universe
fi

# Refresh package lists. If the environment has no network connectivity
# the build may still proceed using whatever packages are already
# available in the cache.
if ! sudo apt-get update; then
    echo "apt-get update failed; using existing package lists" >&2
fi

# Verify that packages expected from the universe repository are
# actually provided by it. The build may still succeed if this check
# fails, but the user is notified about potential configuration issues.
check_repo() {
    local pkg=$1
    local repo=$2
    if ! apt-cache policy "$pkg" | grep -q "$repo"; then
        echo "Warning: $pkg not found in the $repo repository" >&2
    fi
}

check_repo bmake universe
check_repo byacc universe
check_repo flex universe
if apt-cache policy bison | grep -q "universe"; then
    echo "Warning: bison should come from the main repository" >&2
fi

# Packages required by the BSD build system and additional utilities
# used during compilation.
PKGS=(
    bmake
    byacc
    bison
    flex
    cmake
    clang
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

