#!/bin/bash
# Remove intermediate files (keeps dist/ and screenshots/)
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
rm -rf out emul build/*.log build/preview src/data/*.c src/data/gfx_data.h
echo "cleaned (generated data is re-created by scripts/build.sh)"
