#!/usr/bin/env bash
set -euo pipefail
KAW_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
KAW_GL="${MSXGL_ROOT:-$KAW_DIR/.toolchain/MSXgl}"
KAW_REV=6fd8858dce9a5a674529b78ec13d64262810eb34
command -v node >/dev/null
command -v git >/dev/null
command -v python3 >/dev/null
if [[ ! -f "$KAW_GL/engine/script/js/build.js" ]]; then
 mkdir -p "$KAW_GL"
 git -C "$KAW_GL" init -q
 git -C "$KAW_GL" remote add origin https://github.com/aoineko-fr/MSXgl.git
 git -C "$KAW_GL" fetch --depth 1 origin "$KAW_REV"
 git -C "$KAW_GL" checkout --detach FETCH_HEAD
fi
[[ "$(git -C "$KAW_GL" rev-parse HEAD)" == "$KAW_REV" ]] || { echo "Revisao MSXgl incorreta";exit 1; }
chmod +x "$KAW_GL"/tools/sdcc/bin/* "$KAW_GL/tools/MSXtk/bin/MSXhex"
cd "$KAW_DIR/src"
KAW_VARIANTS=("$@")
if [[ ${#KAW_VARIANTS[@]} -eq 0 ]]; then KAW_VARIANTS=(V9990 V9968 V9990P1); fi
for KAW_VARIANT in "${KAW_VARIANTS[@]}"; do
 KAW_TARGET="$KAW_VARIANT" node "$KAW_GL/engine/script/js/build.js" > "$KAW_DIR/build-$KAW_VARIANT.log" 2>&1
 case "$KAW_VARIANT" in V9990) KAW_NAME=KAW9990;; V9990P1) KAW_NAME=KAW9990P1;; V9968) KAW_NAME=KAW9968;; PREVIEW) KAW_NAME=KAW68PV;; esac
 mkdir -p "$KAW_DIR/build/$KAW_VARIANT"
 cp "out/$KAW_NAME.rom" "$KAW_DIR/$KAW_NAME.ROM"
 cp out/*_b3.sym "$KAW_DIR/build/$KAW_VARIANT/"
 cp "out/main.sym" "out/$KAW_NAME.map" "out/$KAW_NAME.ihx" "$KAW_DIR/build/$KAW_VARIANT/"
 python3 "$KAW_DIR/tests/check_memory092.py" "$KAW_VARIANT"
done

python3 - "$KAW_DIR" <<'PY2'
from pathlib import Path
import hashlib,sys
for p in sorted(Path(sys.argv[1]).glob('*.ROM')):
 b=p.read_bytes();assert len(b)==262144 and b[:2]==b'AB'
 print(p.name,len(b),hashlib.sha256(b).hexdigest())
PY2
