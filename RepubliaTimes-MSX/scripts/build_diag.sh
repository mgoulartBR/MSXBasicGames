#!/bin/bash
# Real-hardware diagnostic ROMs (ASCII8 + Konami): checks ROM window checksums, bank-switch latency,
# RAM, VRAM read-back and the VBlank hook, and prints a report to photograph.
# Expected checksums of segments 4-6 and of the fixed font are computed from the built ROM image and
# fed back as -D defines; iterated until stable (segment 6 holds code with absolute addresses).
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
sums() { # tag -> -D defines from dist/...-diag-<tag>.rom + out/republia.map
python3 - "$1" "$VERSION" <<'PY'
import re,sys
tag,ver=sys.argv[1],sys.argv[2]
rom=open(f'dist/republia-msx-{ver}-diag-{tag}.rom','rb').read()
def seg(n): return sum(rom[n*0x2000:(n+1)*0x2000]) & 0xFFFF
m=open('out/republia.map').read()
a=int(re.search(r'\n\s+([0-9A-F]{8})\s+_g_Font_Silk\s',m).group(1),16)&0xFFFF
font=sum(rom[a-0x4000:a-0x4000+95*9])&0xFFFF
print(f'-DRT_DIAG -DEXP4={seg(4)} -DEXP5={seg(5)} -DEXP6={seg(6)} -DEXPF={font}')
PY
}
for m in ascii8:ROM_ASCII8 konami:ROM_KONAMI; do
  tag=${m%%:*}; mp=${m##*:}
  DEFS="-DRT_DIAG"
  for pass in 1 2 3 4; do
    RT_DIAG_MODULES=1 RT_MAPPER=$mp RT_TAG=diag-$tag RT_DEFINES="$DEFS" ./scripts/build.sh >/dev/null 2>&1 || { echo "build failed"; exit 1; }
    NEW=$(sums $tag)
    [ "$NEW" = "$DEFS" ] && break
    DEFS="$NEW"
  done
  [ "$NEW" = "$DEFS" ] || { echo "checksums did not converge"; exit 1; }
  echo "$tag: $DEFS (pass $pass)"
done
ls -l dist/*diag*.rom
