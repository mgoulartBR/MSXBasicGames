#!/bin/bash
# Release bundle: ROM + map + SHA-256 + docs  ->  dist/republia-msx-<version>.zip
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
./scripts/build.sh >/dev/null
RT_MAPPER=ROM_KONAMI RT_TAG=konami ./scripts/build.sh >/dev/null
./scripts/build.sh >/dev/null   # leave the ASCII8 build as the last/default one
(cd dist && sha256sum "republia-msx-$VERSION.rom" "republia-msx-$VERSION-konami.rom" > SHA256SUMS)
rm -f "dist/republia-msx-$VERSION.zip"
zip -j -q "dist/republia-msx-$VERSION.zip" "dist/republia-msx-$VERSION.rom" "dist/republia-msx-$VERSION-konami.rom" "dist/republia-msx-$VERSION.map" dist/SHA256SUMS README.md LICENSES.md
ls -l "dist/republia-msx-$VERSION.zip"; cat dist/SHA256SUMS
