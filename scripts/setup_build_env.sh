#!/usr/bin/env bash
# Prepare the build environment for 2.11BSD_X44.
# Installs required packages and cleans the PATH for build.sh.

set -euo pipefail

# Add the "universe" repository when missing.
if ! grep -q '^deb .\+universe' /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null; then
    sudo add-apt-repository -y universe
fi

# Refresh package lists.
sudo apt-get update

# Packages required by the BSD build system.
PKGS=(bmake byacc bison flex)

# Install each package if not already installed.
for pkg in "${PKGS[@]}"; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        sudo apt-get install -y "$pkg"
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
