#!/bin/bash
# Smoke test: boota a ROM, captura tela inicial, injeta teclas (Right, Down, Down) e captura de novo.
# Falha se as duas capturas forem identicas (entrada nao respondeu) ou se alguma estiver vazia/preta.
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}
OUT=screenshots/test; mkdir -p $OUT
cat > /tmp/madtv_smoke.tcl <<T
proc press {row mask t} { keymatrixdown \$row \$mask; after time 0.15 "keymatrixup \$row \$mask" }
after time 6  { screenshot -raw $PWD/$OUT/a_boot.png }
after time 7  { press 8 0x80 0 }            ;# Right -> categoria Action
after time 8  { press 8 0x40 0 }            ;# Down
after time 8.6 { press 8 0x40 0 }            ;# Down
after time 10 { screenshot -raw $PWD/$OUT/b_after_input.png }
after time 11 { exit }
T
timeout 40 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -script /tmp/madtv_smoke.tcl >/tmp/madtv_smoke.log 2>&1
for f in a_boot b_after_input; do [ -s $OUT/$f.png ] || { echo "FAIL: $f.png ausente"; exit 1; }; done
if cmp -s $OUT/a_boot.png $OUT/b_after_input.png; then echo "FAIL: tela nao mudou apos input"; exit 1; fi
python3 -I - <<P
import zlib,struct,sys
def px(p):
    d=open(p,'rb').read(); i=8; idat=b''
    while i<len(d):
        n,t=struct.unpack('>I4s',d[i:i+8]); 
        if t==b'IDAT': idat+=d[i+8:i+8+n]
        i+=12+n
    return len(set(zlib.decompress(idat)))
for f in ('a_boot','b_after_input'):
    print(f,'bytes distintos:',px('$OUT/%s.png'%f))
    if px('$OUT/%s.png'%f)<8: sys.exit('FAIL: tela quase vazia')
P
echo "PASS"
