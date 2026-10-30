#!/bin/sh
# Builds the 5-room demo (kapiroto_demo.rom).
# Run it from a kapiroto folder placed in MSXgl/projects: it creates a sibling folder
# MSXgl/projects/kapiroto_demo with KAPIROTO_DEMO defined and builds it with the MSXgl build tool.
set -eu
cd "$(dirname "$0")/.."
D=../kapiroto_demo
rm -rf "$D"
mkdir "$D"
cp build.sh build.bat project_config.js msxgl_config.h kapiroto.c ld_logic.h screens.h "$D"/
printf '#define KAPIROTO_DEMO\n' > "$D/build_config.h"
sed -i 's/^ProjName = "kapiroto";/ProjName = "kapiroto_demo";/' "$D/project_config.js"
cd "$D"
sh build.sh
