#!/usr/bin/env bash
set -euo pipefail

backend="${1:-any}"
jobs="${JOBS:-$(nproc)}"

case "$backend" in
    x11)
        wayland=OFF
        x11=ON
        ;;
    wayland)
        wayland=ON
        x11=OFF
        ;;
    *)
        wayland=ON
        x11=ON
        ;;
esac

cmake -S . -B ./build \
    -DGLFW_BUILD_WAYLAND="$wayland" \
    -DGLFW_BUILD_X11="$x11"

cmake --build ./build --parallel "$jobs"
