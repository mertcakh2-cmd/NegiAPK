#!/data/data/com.termux/files/usr/bin/bash

set -e

echo "================================"
echo "       VIMMY INSTALLER"
echo "================================"

pkg update -y
pkg install clang ncurses -y

echo "[+] Compiling Vimmy..."

clang -std=c11 -D_DEFAULT_SOURCE \
-Wall -Wextra -O2 \
editor.c -lncurses -o vimmy

echo "[+] Installing..."

mkdir -p "$PREFIX/bin"
cp vimmy "$PREFIX/bin/vimmy"
chmod +x "$PREFIX/bin/vimmy"

echo ""
echo "Vimmy installed successfully!"
echo "Run: vimmy"
echo "Open file: vimmy filename.txt"
