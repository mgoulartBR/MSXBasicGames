#!/bin/bash
# Teste do caminho REAL do mouse: openMSX com mouse emulado na porta 1 + movimento/clique do host via xdotool (Xvfb).
# Requer: xvfb, xdotool. Verifica: M liga o mouse, o cursor se move, passar sobre uma porta a realca e o clique viaja ate la e entra.
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}; MAP=out/madtv.map
sym() { awk -v s="$1" '$2==s{print "0x"$1}' $MAP | head -1; }
PX=$(sym _g_PtrX); PY=$(sym _g_PtrY); PM=$(sym _g_PtrMode); SC=$(sym _g_Screen); BS=$(sym _g_BldSel)
command -v xdotool >/dev/null || { echo "SKIP: xdotool ausente"; exit 0; }
mkdir -p screenshots/test; rm -f /tmp/mouse_real.txt
cat > /tmp/mouse_real.tcl <<T
plug joyporta mouse
proc press {row mask} { keymatrixdown \$row \$mask; after time 0.12 "keymatrixup \$row \$mask" }
proc rec {tag} { set f [open /tmp/mouse_real.txt a]; puts \$f "\$tag x=[peek $PX] y=[peek $PY] mode=[peek $PM] scr=[peek $SC] sel=[peek $BS]"; close \$f }
after time 8 { press 7 0x80 }                 ;# inicia o jogo
after time 9.5 { press 4 0x04 }               ;# tecla M: mouse na porta 1
after time 10.5 { rec before }
after time 14 { rec moved; screenshot -raw $PWD/screenshots/test/mouse_cursor.png }
after time 23 { rec entered }
after time 24 { exit }
T
Xvfb :80 -screen 0 800x600x24 >/dev/null 2>&1 & XP=$!
sleep 1
DISPLAY=:80 timeout 60 openmsx -machine C-BIOS_MSX2 -cart "$ROM" -script /tmp/mouse_real.tcl >/tmp/mouse_real.log 2>&1 & OP=$!
sleep 12
DISPLAY=:80 xdotool mousemove 400 300; sleep 0.5
for i in 1 2 3 4 5 6 7 8; do DISPLAY=:80 xdotool mousemove_relative -- 12 8; sleep 0.15; done
sleep 1.5
DISPLAY=:80 xdotool mousedown 1; sleep 0.25; DISPLAY=:80 xdotool mouseup 1     # clique >= 1 quadro (como um clique humano)
wait $OP; kill $XP 2>/dev/null
cat /tmp/mouse_real.txt
python3 -I - <<'P'
import sys
S={l.split()[0]:{k:int(v) for k,v in (kv.split('=') for kv in l.split()[1:])} for l in open('/tmp/mouse_real.txt').read().splitlines()}
def need(c,m):
    if not c: print('FAIL:',m); sys.exit(1)
need('entered' in S,'script nao completou')
need(S['before']['mode']==1,'tecla M nao ligou o mouse na porta 1')
need((S['moved']['x'],S['moved']['y'])!=(S['before']['x'],S['before']['y']),'o cursor nao se moveu com o mouse')
need(S['moved']['sel']==5 and S['moved']['scr']==1,'passar sobre a porta NEWS nao a realcou')
need(S['entered']['scr']==5,'clique do mouse nao levou a News room')
print('PASS')
P
