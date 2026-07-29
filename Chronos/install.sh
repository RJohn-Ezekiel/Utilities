#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-$HOME/.local/bin}"
mkdir -p "$BIN"

echo "==> Building Chronos..."
cmake -S "$(dirname "$0")" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

echo "==> Installing to $BIN ..."
cp build/src/chronos "$BIN/chronos.bin"

cat > "$BIN/chronos" << 'SCRIPT'
#!/bin/bash
export QT_LOGGING_RULES="kf.*.warning=false"
export QT_QPA_PLATFORMTHEME=""
exec "$0.bin" "$@"
SCRIPT
chmod +x "$BIN/chronos"

echo "Done. Run: chronos"
