#!/usr/bin/env bash
# Set up the build environment for PSD_21144.
# Installs required packages and cleans PATH.

set -euo pipefail
IFS=$'\n\t'

# Determine repository root directory
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Install packages when apt-get is available
if command -v apt-get >/dev/null 2>&1; then
	# Ensure universe repository is enabled
	if ! grep -q '^deb .\+universe' /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null; then
		sudo add-apt-repository -y universe
	fi

	# Refresh package lists; ignore failure
	if ! sudo apt-get update; then
		echo "apt-get update failed; continuing" >&2
	fi

	# List of packages needed for building
	apt_packages=(
		bmake
		byacc
		bison
		flex
		build-essential
		clang
		clang-format
		clang-tidy
		cmake
		gcc
		g++
		gdb
		valgrind
		lcov
		shellcheck
		python3-pip
		gcc-multilib
		g++-multilib
		libncurses-dev
		libssl-dev
		nasm
		doxygen
		python3-sphinx
		python3-sphinx-rtd-theme
		cloc
		qemu-system-x86
		qemu-utils
		qemu-nox
		tmux
	)

	# Install missing packages
	for pkg in "${apt_packages[@]}"; do
		if ! dpkg -s "$pkg" >/dev/null 2>&1; then
			if ! sudo apt-get install -y "$pkg"; then
				echo "Failed to install $pkg" >&2
			fi
		fi
	done
fi

# Install Python requirements if any
if command -v pip3 >/dev/null 2>&1 && [ -f "$ROOT_DIR/requirements.txt" ]; then
	pip3 install --user -r "$ROOT_DIR/requirements.txt"
fi

# Remove relative PATH entries
CLEAN_PATH=""
IFS=':'
for dir in $PATH; do
	case "$dir" in
	'' | '.' | [!/]*)
		continue
		;;
	*)
		CLEAN_PATH="${CLEAN_PATH:+${CLEAN_PATH}:}${dir}"
		;;
	esac
done
unset IFS
export PATH="$CLEAN_PATH"

echo "Environment configured. PATH set to: $PATH"

# Quick sanity build of bmake
if command -v bmake >/dev/null 2>&1; then
	cd "$ROOT_DIR/usr.bin/make"
	bmake
else
	echo "bmake is not installed; build skipped" >&2
fi
