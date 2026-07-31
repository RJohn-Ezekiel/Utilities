#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-$HOME/.local/bin}"
mkdir -p "$BIN"

echo "==> Building Phonio..."
cmake -S "$(dirname "$0")" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

echo "==> Installing to $BIN ..."
cp build/phonio "$BIN/phonio.bin"

cat > "$BIN/phonio" << 'SCRIPT'
#!/bin/bash
export QT_LOGGING_RULES="kf.*.warning=false"
export QT_QPA_PLATFORMTHEME=""
exec "$0.bin" "$@"
SCRIPT
chmod +x "$BIN/phonio"

echo "Done. Run: phonio"
