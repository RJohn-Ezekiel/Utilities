#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-$HOME/.local/bin}"
mkdir -p "$BIN"

echo "==> Building Codex..."
cmake -S "$(dirname "$0")" -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

echo "==> Installing to $BIN ..."
cp build/bin/codex "$BIN/codex.bin"

cat > "$BIN/codex" << 'SCRIPT'
#!/bin/bash
export QT_LOGGING_RULES="kf.*.warning=false"
export QT_QPA_PLATFORMTHEME=""
exec "$0.bin" "$@"
SCRIPT
chmod +x "$BIN/codex"

echo "Done. Run: codex"
