#!/usr/bin/env bash
# Setup script used to provision the build environment for 2.11BSD_X44.
# It installs all needed tools using apt and pip then performs a basic
# build of usr.bin/make when bmake is available.

set -euo pipefail

# Absolute path to the repository root so relative operations work no
# matter where the script is executed from.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Only attempt package installation when apt-get is available.  Some
# systems or restricted environments may not provide it.
if command -v apt-get >/dev/null 2>&1; then
    # Enable the "universe" repository if it has not already been added.
    if ! grep -q '^deb .\+universe' /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null; then
        sudo add-apt-repository -y universe
    fi

    # Refresh package information.  Failure is not fatal in order to
    # accommodate environments without network access.
    if ! sudo apt-get update; then
        echo "apt-get update failed; using existing package lists" >&2
    fi

    # Packages required for the BSD build system.
    apt_packages=(
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

    # Install each package if not present already.  Installation errors
    # are reported but do not stop the remainder of the script.
    for pkg in "${apt_packages[@]}"; do
        if ! dpkg -s "$pkg" >/dev/null 2>&1; then
            if ! sudo apt-get install -y "$pkg"; then
                echo "Failed to install $pkg" >&2
            fi
        fi
    done
fi

# Install Python dependencies when pip is available.
if command -v pip3 >/dev/null 2>&1; then
    if [ -f "$ROOT_DIR/requirements.txt" ]; then
        # Use --user so system packages are untouched.
        pip3 install --user -r "$ROOT_DIR/requirements.txt"
    fi
fi

# Remove relative entries from PATH so build.sh behaves correctly.
CLEAN_PATH=""
IFS=':'
for dir in $PATH; do
    case "$dir" in
        ''|'.')
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

# Display the final PATH so callers know the configuration that will be
# used for subsequent build steps.
echo "Environment configured. PATH set to: $PATH"

# Attempt a test build of usr.bin/make when bmake is installed.  This
# provides a quick sanity check that the toolchain works.
if command -v bmake >/dev/null 2>&1; then
    cd "$ROOT_DIR/usr.bin/make"
    bmake
else
    echo "bmake is not installed; build skipped" >&2
fi

