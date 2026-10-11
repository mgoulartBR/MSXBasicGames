#!/bin/bash
# Teste do caminho FM (MSX-MUSIC): C-BIOS_MSX2+ tem YM2413 (assinatura APRLOPLL). Confere deteccao (g_MusicType=1),
# instrumentos nos canais 1-3 e que notas (key-on) sao escritas durante a musica. No C-BIOS_MSX2 (sem FM) o tipo deve ser 0 (PSG).
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}; MAP=out/madtv.map
sym() { awk -v s="$1" '$2==s{print "0x"$1}' $MAP | head -1; }
MT=$(sym _g_MusicType); MP=$(sym _g_MusicPos)
cat > /tmp/fm_test.tcl <<T
proc rd {r} { return [debug read {C-BIOS MSX-MUSIC regs} \$r] }
set ::keys 0
foreach t {10 10.3 10.6 10.9 11.2 11.5 11.8 12.1 12.4 12.7} { after time \$t { foreach c {0x20 0x21 0x22} { if {[rd \$c] & 0x10} { incr ::keys } } } }
after time 13 { set f [open /tmp/fm_result.txt w]; puts \$f "type=[peek $MT] pos=[peek $MP] inst=[format %02x [rd 0x30]],[format %02x [rd 0x31]],[format %02x [rd 0x32]] keys=\$::keys"; close \$f; exit }
T
rm -f /tmp/fm_result.txt
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2+ -cart "$ROM" -script /tmp/fm_test.tcl >/dev/null 2>&1
cat /tmp/fm_result.txt
python3 -I - <<'P'
import sys,re
d=dict(kv.split('=') for kv in open('/tmp/fm_result.txt').read().split())
def need(c,m):
    if not c: print('FAIL:',m); sys.exit(1)
need(d['type']=='1','MSX-MUSIC nao detectado no C-BIOS_MSX2+')
need(d['inst']=='43,e2,26','instrumentos FM nao programados')
need(int(d['keys'])>=3,'nenhuma nota FM tocada (key-on)')
print('PASS')
P
