#!/usr/bin/env bash
set -euo pipefail

# Visio — C++20 YouTube library
# Quick install script

PREFIX="${PREFIX:-$HOME/.local}"
BUILD_DIR="$(dirname "$0")/build"

echo "=== Visio Install ==="
echo "Prefix: $PREFIX"

# Check dependencies
echo ""
echo "Checking dependencies..."
MISSING=()

command -v cmake   >/dev/null 2>&1 || MISSING+=("cmake")
command -v g++     >/dev/null 2>&1 || MISSING+=("g++")
command -v curl    >/dev/null 2>&1 || MISSING+=("curl")
command -v yt-dlp  >/dev/null 2>&1 || MISSING+=("yt-dlp")
command -v mpv     >/dev/null 2>&1 || MISSING+=("mpv")

if [ ${#MISSING[@]} -gt 0 ]; then
    echo ""
    echo "Missing required tools: ${MISSING[*]}"
    echo ""
    echo "Install them:"
    echo "  Debian/Ubuntu: sudo apt install build-essential cmake curl yt-dlp mpv"
    echo "  Fedora:        sudo dnf install gcc-c++ cmake curl yt-dlp mpv"
    echo "  Arch:          sudo pacman -S base-devel cmake curl yt-dlp mpv"
    echo "  macOS:         brew install cmake curl yt-dlp mpv"
    echo ""
    exit 1
fi

echo "All runtime dependencies found."
echo ""

# Configure
echo "Configuring..."
cmake -S "$(dirname "$0")" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
    -DVISIO_BUILD_TESTS=OFF -DVISIO_BUILD_EXAMPLES=OFF

# Build
echo "Building..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

# Install
echo "Installing to $PREFIX..."
cmake --install "$BUILD_DIR" --prefix "$PREFIX" 2>/dev/null || {
    # Manual install if cmake --install fails (no export set)
    mkdir -p "$PREFIX/include" "$PREFIX/lib"
    cp -r "$(dirname "$0")/include/visio" "$PREFIX/include/"
    cp "$BUILD_DIR/libvisio.a" "$PREFIX/lib/"
    echo "Installed manually."
}

echo ""
echo "=== Done ==="
echo "Library:  $PREFIX/lib/libvisio.a"
echo "Headers:  $PREFIX/include/visio/"
echo ""
echo "Use in CMake:"
echo "  add_subdirectory(path/to/Visio)"
echo "  target_link_libraries(your_app PRIVATE visio)"
