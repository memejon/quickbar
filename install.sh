#!/usr/bin/env bash
set -euo pipefail

prefix="${1:-/usr}"
build_dir="${BUILD_DIR:-build}"

cmake -B "$build_dir" -DCMAKE_INSTALL_PREFIX="$prefix"
cmake --build "$build_dir"
if [[ "$prefix" == /usr ]]; then
    sudo cmake --install "$build_dir"
else
    cmake --install "$build_dir"
fi

echo "Installed QuickBar to $prefix. Restart plasmashell and add the QuickBar widget."
