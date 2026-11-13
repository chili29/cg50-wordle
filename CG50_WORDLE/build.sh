#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
command -v fxsdk >/dev/null 2>&1 || { echo "fxSDK is not installed or not on PATH." >&2; exit 1; }
fxsdk build-cg
echo "Build finished. Output: $(pwd)/WORDLE.g3a"
