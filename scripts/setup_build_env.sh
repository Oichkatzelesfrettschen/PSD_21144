#!/usr/bin/env bash
# Prepare the build environment for 2.11BSD_X44.
# Installs required packages and sanitizes the PATH to satisfy build.sh.
set -e

# Ensure the "universe" repository is available.
sudo add-apt-repository -y universe
sudo apt-get update

# Install the tools used by the BSD build system.
sudo apt-get install -y bmake byacc bison flex

# Remove non-absolute components from PATH to appease build.sh.
CLEAN_PATH=""
IFS=':'
for dir in $PATH; do
    case "$dir" in
        ""|".")
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

echo "Environment configured. PATH set to: $PATH"
