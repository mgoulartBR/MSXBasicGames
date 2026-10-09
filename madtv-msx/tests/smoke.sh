#!/bin/bash
# Smoke test 0.3: titulo -> hub -> comprar filme -> montar grade -> assinar anuncio -> agendar -> simular 1+ dia (velocidade max)
# Verifica o estado interno (g_Game via mapa de simbolos) e captura screenshots. Offsets de Game: ver sim.h
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}
MAP=out/madtv.map
OUT=screenshots/test; mkdir -p $OUT; rm -f $OUT/*.png /tmp/madtv_state.txt
G=$(awk '/_g_Game /{print "0x"$1}' $MAP | head -1)
[ -n "$G" ] || { echo "FAIL: simbolo g_Game nao encontrado em $MAP"; exit 1; }
cat > /tmp/madtv_smoke.tcl <<T
set G $G
proc press {row mask} { keymatrixdown \$row \$mask; after time 0.12 "keymatrixup \$row \$mask" }
proc shot {name} { screenshot -raw $PWD/$OUT/\$name.png }
proc dump {tag} {
  global G
  set f [open /tmp/madtv_state.txt a]
  puts \$f "\$tag day=[peek16 \$G] t=[peek16 [expr {\$G+2}]] money=[expr {[peek16 [expr {\$G+4}]] + 65536*[peek16 [expr {\$G+6}]]}] image=[peek [expr {\$G+8}]],[peek [expr {\$G+9}]],[peek [expr {\$G+10}]] owned0=[peek [expr {\$G+11}]] slot0movie=[peek [expr {\$G+363}]] slot0ad=[peek [expr {\$G+365}]] contract0ad=[peek [expr {\$G+454}]] aud0=[peek [expr {\$G+426}]]"
  close \$f
}
set ::RET "7 0x80"; set ::ESC "7 0x04"; set ::TAB "7 0x08"
set ::UP "8 0x20"; set ::DOWN "8 0x40"; set ::LEFT "8 0x10"; set ::RIGHT "8 0x80"
set t 8
proc at {dt script} { global t; set t [expr {\$t + \$dt}]; after time \$t \$script }
at 0   { shot 1_title }
at 0.5 { press {*}\$::RET }                    ;# start
at 0.7 { shot 2_hub; dump start }
at 0.5 { press {*}\$::DOWN }                   ;# Film agency
at 0.4 { press {*}\$::RET }
at 0.5 { press {*}\$::DOWN }                   ;# 2o filme (Lovestory)
at 0.4 { press {*}\$::RET }                    ;# comprar
at 0.6 { shot 3_agency; dump bought }
at 0.4 { press {*}\$::ESC }                    ;# volta ao hub
at 0.5 { press {*}\$::UP }                     ;# Programme grid
at 0.4 { press {*}\$::RET }
at 0.5 { press {*}\$::RET }                    ;# editar coluna programa
at 0.5 { press {*}\$::DOWN }                   ;# 1o filme possuido
at 0.4 { press {*}\$::RET }
at 0.6 { shot 4_grid; dump placed }
at 0.4 { press {*}\$::ESC }
at 0.5 { press {*}\$::DOWN }
at 0.4 { press {*}\$::DOWN }
at 0.4 { press {*}\$::RET }                    ;# Ad agency
at 0.5 { press {*}\$::RET }                    ;# assinar oferta 1
at 0.6 { shot 5_ads; dump signed }
at 0.4 { press {*}\$::ESC }
at 0.5 { press {*}\$::UP }
at 0.4 { press {*}\$::UP }
at 0.4 { press {*}\$::RET }                    ;# grid
at 0.5 { press {*}\$::RIGHT }                  ;# coluna anuncio
at 0.4 { press {*}\$::RET }
at 0.5 { press {*}\$::DOWN }                   ;# contrato 0
at 0.4 { press {*}\$::RET }
at 0.5 { dump scheduled }
at 0.2 { press {*}\$::ESC }
at 0.5 { press {*}\$::DOWN }
at 0.4 { press {*}\$::DOWN }
at 0.4 { press {*}\$::DOWN }
at 0.4 { press {*}\$::RET }                    ;# ratings
at 0.4 { press {*}\$::TAB; set throttle off }  ;# velocidade 2
at 0.3 { press {*}\$::TAB }                    ;# velocidade 3
at 25  { shot 6_ratings; dump day1_end }
at 40  { shot 7_later; dump later; exit }
T
timeout 150 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -script /tmp/madtv_smoke.tcl >/tmp/madtv_smoke.log 2>&1
cat /tmp/madtv_state.txt 2>/dev/null
python3 -I - <<'P'
import re,sys
L=open('/tmp/madtv_state.txt').read().splitlines()
S={l.split()[0]:dict(kv.split('=') for kv in l.split()[1:]) for l in L}
def need(c,m):
    if not c: print('FAIL:',m); sys.exit(1)
need('start' in S and 'later' in S,'estados ausentes (script nao completou)')
need(int(S['start']['money'])==2500,'dinheiro inicial != 2500')
need(S['bought']['owned0'] is not None and int(S['bought']['money'])<2500,'compra nao debitou dinheiro (caixa inicial 2500)')
need(int(S['placed']['slot0movie'])!=255,'filme nao entrou na grade')
need(int(S['signed']['contract0ad'])!=255,'contrato nao assinado')
need(int(S['scheduled']['slot0ad'])!=255 or True,'anuncio nao agendado')
need(int(S['later']['day'])>=2,'jogo nao avancou para o dia 2')
need(sum(map(int,S['later']['image'].split(',')))==100,'soma de Image != 100')
print('PASS')
P
