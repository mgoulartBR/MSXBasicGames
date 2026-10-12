#!/bin/bash
# Teste de estresse: teclas e mouse aleatorios (semente fixa) por ~150 s de tempo emulado em velocidade maxima, em varias sementes.
# Verifica: o laco principal continua vivo (g_MusicPos avanca quando a musica esta ligada), o numero da tela e a velocidade permanecem validos.
# Nao valida o dinheiro (a sessao aleatoria raramente passa do menu/predio).
# Uso: tests/fuzz.sh [rom] [sementes...]   (padrao: 3 sementes)
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}; shift
SEEDS=${@:-1 2 3}
MAP=out/madtv.map
sym() { awk -v s="$1" '$2==s{print "0x"$1}' $MAP | head -1; }
G=$(sym _g_Game); OFFS=$(sym _g_DbgOffsets); SCR=$(sym _g_Screen); MP=$(sym _g_MusicPos); SM=$(sym _g_SndMus); PX=$(sym _g_PtrX); PY=$(sym _g_PtrY); INJ=$(sym _g_PtrInject); SEL=$(sym _g_BldSel); SPD=$(sym _g_Speed)
rm -rf "$HOME"/.openMSX/persistent/roms/madtv-* 2>/dev/null
for SEED in $SEEDS; do
cat > /tmp/fuzz.tcl <<T
set G $G
set OFFS $OFFS
proc off {i} { global OFFS; return [peek16 [expr {\$OFFS + 2*\$i}]] }
proc press {row mask} { keymatrixdown \$row \$mask; after time 0.1 "keymatrixup \$row \$mask" }
set throttle off
expr {srand($SEED)}
set keys {{7 0x80} {7 0x80} {7 0x04} {8 0x20} {8 0x40} {8 0x10} {8 0x80} {7 0x08} {4 0x20} {8 0x40} {8 0x80} {7 0x80}}
proc act {} {
  global keys
  set r [expr {int(rand()*100)}]
  if {\$r < 70} { set k [lindex \$keys [expr {int(rand()*[llength \$keys])}]]; press [lindex \$k 0] [lindex \$k 1] } elseif {\$r < 80} { set row [expr {int(rand()*6)}]; set bit [expr {1 << int(rand()*8)}]; press \$row [format 0x%02x \$bit] } else { poke $PX [expr {int(rand()*256)}]; poke $PY [expr {int(rand()*212)}]; poke $INJ [lindex {1 2 4 2} [expr {int(rand()*4)}]] }
}
for {set i 0} {\$i < 900} {incr i} { after time [expr {10 + 0.15*\$i}] act }
for {set i 0} {\$i < 48} {incr i} { after time [expr {13 + 3.0*\$i}] { lappend ::fzlog "[peek $SCR] [peek $MP] [peek $SM] [peek16 \$::G] [peek $SPD]" } }
after time 150 {
  set f [open /tmp/fuzz_out.txt w]
  foreach s \$::fzlog { puts \$f \$s }
  set money [expr {[peek16 [expr {\$G+[off 2]}]] + 65536*[peek16 [expr {\$G+[off 2]+2}]]}]
  if {\$money >= 2147483648} { set money [expr {\$money - 4294967296}] }
  puts \$f "MONEY \$money"
  close \$f; exit
}
T
rm -f /tmp/fuzz_out.txt
timeout 300 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8SRAM8 -script /tmp/fuzz.tcl >/dev/null 2>&1
python3 -I - $SEED <<'P'
import sys
seed=sys.argv[1]
try: L=open('/tmp/fuzz_out.txt').read().split('\n')
except Exception: print('FAIL seed',seed,': script nao terminou'); sys.exit(1)
S=[l.split() for l in L if l and not l.startswith('MONEY')]
bad=[s for s in S if int(s[0])>21 or int(s[4])>3]
live=0; tot=0
for a,b in zip(S,S[1:]):
    if a[2]=='1' and b[2]=='1' and a[1]!=b[1] or (a[2]=='1' and b[2]=='1' and a[1]==b[1] and False): live+=1
    if a[2]=='1' and b[2]=='1': tot+=1
print('seed %s: amostras=%d telas invalidas=%d vivo=%d/%d' % (seed,len(S),len(bad),live,tot))
if len(S)<30 or bad or (tot>5 and live*100<tot*50):
    print('FAIL seed',seed); sys.exit(1)
P
[ $? -eq 0 ] || exit 1
done
echo PASS
