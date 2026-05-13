#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$ROOT/build"

echo "=== Building KvadraAccelerometer (Level 1) ==="
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

echo ""
echo "Binaries:"
echo "  $BUILD_DIR/server/server"
echo "  $BUILD_DIR/node_a/node_a"
echo "  $BUILD_DIR/node_b/node_b"
echo ""
echo "Quick start (three terminals):"
echo "  Terminal 1: $BUILD_DIR/server/server  config/server.conf"
echo "  Terminal 2: $BUILD_DIR/node_b/node_b  config/node_b.conf"
echo "  Terminal 3: $BUILD_DIR/node_a/node_a  config/node_a.conf"
