#!/usr/bin/env bash
set -euo pipefail

DSA_DIR="dsa"
DSA_ARCHIVE="dsa.tar.gz"

if [ ! -d "$DSA_DIR" ] || [ ! -f "$DSA_DIR/CMakeLists.txt" ]; then
    echo "[INFO] Unpacking DSA submodule from archive..."
    tar xzf "$DSA_ARCHIVE"
else
    echo "[INFO] DSA already present, skipping extraction."
fi
