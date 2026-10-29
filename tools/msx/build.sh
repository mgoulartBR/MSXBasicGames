#!/usr/bin/env bash
# Builds msx2/smashtv/smashtv.rom (graphics -> assemble -> tests on the simulator).
# Needs: pip install z80 z80dis pillow
set -euo pipefail
cd "$(dirname "$0")/../.."
python3 tools/msx/gen_gfx.py msx2/smashtv/gfx.inc
mkdir -p msx2/smashtv/build
python3 tools/msx/z80asm.py msx2/smashtv/main.asm msx2/smashtv/build/smashtv.rom --sym msx2/smashtv/build/smashtv.sym
cp msx2/smashtv/build/smashtv.rom msx2/smashtv/smashtv.rom
if [ "${1:-}" != "--no-test" ]; then
  python3 tools/msx/test_smashtv.py msx2/smashtv/smashtv.rom msx2/smashtv/build/smashtv.sym msx2/smashtv/build/shots
fi
