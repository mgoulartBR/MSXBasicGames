#!/bin/bash
# Prepara o ambiente: MSXgl (versao fixa), SDCC (embutido no MSXgl), openMSX + C-BIOS.
set -e
cd "$(dirname "$0")/.."
MSXGL_TAG=v1.5.0
if [ ! -d external/msxgl ]; then
	mkdir -p external
	git clone --depth 1 --branch "$MSXGL_TAG" https://github.com/aoineko-fr/MSXgl.git external/msxgl
fi
external/msxgl/tools/sdcc/bin/sdcc --version | head -1
command -v node >/dev/null || { echo "node necessario"; exit 1; }
command -v openmsx >/dev/null || { apt-get install -y openmsx xvfb; }
echo "setup OK"
