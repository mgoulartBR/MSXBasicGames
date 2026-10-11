#!/bin/bash
# Compila a ROM com MSXgl e copia para dist/.  Uso: scripts/build.sh [clean]
set -e
cd "$(dirname "$0")/.."
[ -f src/data/art_scenes.h ] || echo "#define ART_SCENES 0" > src/data/art_scenes.h   # sem arte local: telas de texto
[ -f src/data/art_hub.h ] || echo "#define ART_HUB 0" > src/data/art_hub.h         # sem arte local: predio procedural
[ -f src/data/art_icons.h ] || echo "#define ART_ICONS 0" > src/data/art_icons.h   # sem arte local: HUD com simbolos de texto
[ -d external/msxgl ] || scripts/setup.sh
[ "$1" = clean ] && rm -rf out emul lib dist/*.rom
node external/msxgl/engine/script/js/build.js target=ROM_ASCII8 2>&1 | tail -n 25
mkdir -p dist
VER=$(cat VERSION)
cp emul/rom/madtv.rom dist/madtv-msx-$VER.rom
ls -l dist/*.rom
sha256sum dist/*.rom
