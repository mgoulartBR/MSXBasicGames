#!/bin/bash
# Compila a ROM com MSXgl e copia para dist/.  Uso: scripts/build.sh [clean]
set -e
cd "$(dirname "$0")/.."
[ -d external/msxgl ] || scripts/setup.sh
[ "$1" = clean ] && rm -rf out emul lib dist/*.rom
node external/msxgl/engine/script/js/build.js target=ROM_32K 2>&1 | tail -n 25
mkdir -p dist
cp emul/rom/madtv.rom dist/madtv-msx-0.1.rom
ls -l dist/*.rom
sha256sum dist/*.rom
