#!/usr/bin/env bash

set -euo pipefail

echo "=================================="
echo "       NEGI VIMMY BUILDER"
echo "=================================="

if [ -n "${PREFIX:-}" ]; then
    pkg update -y
    pkg install clang ncurses -y
fi

echo "[1/3] Checking source files..."

for file in editor.c decor.c; do
    if [ ! -f "$file" ]; then
        echo "ERROR: Missing $file"
        exit 1
    fi
done

echo "[2/3] Compiling Vimmy..."

clang \
    -std=c11 \
    -D_DEFAULT_SOURCE \
    -Wall -Wextra \
    -O2 \
    editor.c decor.c \
    -lncurses \
    -o vimmy

echo "[3/3] Build complete!"

if [ -n "${PREFIX:-}" ]; then
    install -Dm755 vimmy "$PREFIX/bin/vimmy"
    echo "Installed to Termux!"
fi

echo "Run: ./vimmy"
