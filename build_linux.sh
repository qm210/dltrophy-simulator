#!/usr/bin/env bash
set -euo pipefail

target="./build"
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

unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH CMAKE_PREFIX_PATH CMAKE_INCLUDE_PATH

cmake -S . -B "$target" \
    -DCMAKE_C_COMPILER="/usr/bin/gcc" \
    -DCMAKE_CXX_COMPILER="/usr/bin/g++" \
    -DGLFW_BUILD_WAYLAND="$wayland" \
    -DGLFW_BUILD_X11="$x11"

cmake --build "$target" --parallel "$jobs"
