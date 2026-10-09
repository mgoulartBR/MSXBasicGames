#!/bin/bash
# Smoke test 0.4 (openMSX, C-BIOS MSX2): joga por teclas injetadas e confere o estado interno via mapa de simbolos.
# Cobre: titulo -> hub -> comprar filmes -> grade -> contrato -> noticias -> credito -> vender -> salvar/carregar (codigo digitado
# tecla a tecla) -> simular dias em velocidade maxima. Screenshots em screenshots/test/.
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}
MAP=out/madtv.map
OUT=screenshots/test; mkdir -p $OUT; rm -f $OUT/*.png /tmp/madtv_state.txt
sym() { awk -v s="$1" '$2==s{print "0x"$1}' $MAP | head -1; }
DM=$(sym _g_DrawMax); G=$(sym _g_Game); OFFS=$(sym _g_DbgOffsets); CODE=$(sym _g_SaveCode)
[ -n "$G" ] && [ -n "$OFFS" ] && [ -n "$CODE" ] || { echo "FAIL: simbolos ausentes no mapa"; exit 1; }
cat > /tmp/madtv_smoke.tcl <<TCL
set G $G
set OFFS $OFFS
set CODE $CODE
set DM $DM
set SHOTDIR $PWD/$OUT
TCL
cat >> /tmp/madtv_smoke.tcl <<'TCL'
proc press {row mask} { keymatrixdown $row $mask; after time 0.12 "keymatrixup $row $mask" }
proc shot {name} { global SHOTDIR; screenshot -raw $SHOTDIR/$name.png }
proc off {i} { global OFFS; return [peek16 [expr {$OFFS + 2*$i}]] }
proc dump {tag} {
  global G
  set f [open /tmp/madtv_state.txt a]
  set money [expr {[peek16 [expr {$G+[off 2]}]] + 65536*[peek16 [expr {$G+[off 2]+2}]]}]
  if {$money >= 2147483648} { set money [expr {$money - 4294967296}] }
  puts $f "$tag day=[peek16 $G] t=[peek16 [expr {$G+[off 1]}]] money=$money debt=[peek16 [expr {$G+[off 3]}]] image=[peek [expr {$G+[off 4]}]],[peek [expr {$G+[off 4]+1}]] slot0movie=[peek [expr {$G+[off 6]}]] slate0=[peek [expr {$G+[off 7]}]] sub0=[peek [expr {$G+[off 8]}]] contract0=[peek [expr {$G+[off 9]}]] owned1=[peek [expr {$G+[off 5]+1}]] owned2=[peek [expr {$G+[off 5]+2}]]"
  close $f
}
proc readcode {} { global CODE; set s ""; for {set i 0} {$i < 82} {incr i} { append s [format %c [peek [expr {$CODE+$i}]]] }; return $s }
# matriz do teclado MSX: linha 0 = 0-7; 3 = C-J; 4 = K-R; 5 = S-Z; A,B na linha 2 (bits 6,7); 8,9 na linha 1
set ::MATRIX {}
foreach {row chars} {0 01234567 3 CDEFGHIJ 4 KLMNOPQR 5 STUVWXYZ} { set b 0; foreach c [split $chars ""] { dict set ::MATRIX $c [list $row [format 0x%02x [expr {1 << $b}]]]; incr b } }
dict set ::MATRIX A {2 0x40}; dict set ::MATRIX B {2 0x80}; dict set ::MATRIX 8 {1 0x01}; dict set ::MATRIX 9 {1 0x02}
proc typecode {code} {       # digita a partir de agora: 0,16 s por tecla (aperto de 0,08 s) - cada tecla ve uma borda limpa
  set tt 0
  foreach c [split $code ""] {
    set rm [dict get $::MATRIX $c]
    after time $tt "keymatrixdown [lindex $rm 0] [lindex $rm 1]"
    after time [expr {$tt + 0.08}] "keymatrixup [lindex $rm 0] [lindex $rm 1]"
    set tt [expr {$tt + 0.16}]
  }
}
set ::RET "7 0x80"; set ::ESC "7 0x04"; set ::TAB "7 0x08"
set ::UP "8 0x20"; set ::DOWN "8 0x40"; set ::LEFT "8 0x10"; set ::RIGHT "8 0x80"
set t 8
proc at {dt script} { global t; set t [expr {$t + $dt}]; after time $t $script }
proc key {k} { at 0.3 "press {*}\$::$k" }
proc hubto {n} { for {set i 0} {$i < 8} {incr i} { key UP }; for {set i 0} {$i < $n} {incr i} { key DOWN }; key RET; at 0.5 {} }
proc back {} { key ESC; at 0.3 {} }
at 0   { shot 1_title }
key RET
at 1.6 { shot 2_hub; dump start }
# --- Film agency: comprar os filmes 1 e 2 de Lovestory
hubto 1
key DOWN; key RET; key DOWN; key RET
at 0.6 { shot 3_agency; dump bought }
back
# --- Programme grid: colocar o 1o filme possuido no slot 0
hubto 0
key RET; at 0.3 {}; key DOWN; key RET
at 0.6 { shot 4_grid; dump placed }
back
# --- Ad agency: assinar oferta 0
hubto 2
key RET
at 0.6 { shot 5_ads; dump signed }
back
# --- News room: comprar a 1a noticia recebida (linha 4)
hubto 3
for {set i 0} {$i < 4} {incr i} { key DOWN }
key RET
at 0.6 { shot 6_news; dump news }
back
# --- Boss: tomar emprestimo (item 0)
hubto 5
key RET
at 0.6 { shot 7_boss; dump borrowed }
back
# --- Archive: vender o 2o filme da lista (o 1o esta na grade)
hubto 4
key DOWN; key RET
at 0.6 { shot 8_archive; dump sold }
back
# --- Save: mostrar o codigo e guardar
hubto 7
key RET
at 0.8 { shot 9_savecode; set ::SAVED [readcode]; dump saved; set f [open /tmp/madtv_code.txt w]; puts $f $::SAVED; close $f }
back
back
# --- mudar o estado (mais credito), depois carregar o codigo digitando-o tecla a tecla
hubto 5
key RET; key RET
at 0.5 { dump changed }
back
hubto 7
key DOWN; key RET                                   ;# Enter a code
at 0.5 { shot 10_enter_empty }
at 0.3 { typecode $::SAVED }
at 14.0 { shot 11_enter_typed }
key RET                                             ;# confirma
at 1.0 { shot 12_loaded; dump loaded }
# --- simular dias em velocidade maxima
key TAB
at 0.1 { set throttle off }
key TAB
at 95  { shot 13_later; dump later; set f [open /tmp/madtv_draw.txt w]; for {set i 0} {$i < 11} {incr i} { puts -nonewline $f "[peek [expr {$DM+$i}]] " }; close $f; exit }
TCL
timeout 290 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -script /tmp/madtv_smoke.tcl >/tmp/madtv_smoke.log 2>&1
cat /tmp/madtv_state.txt 2>/dev/null
python3 -I - <<'P'
import sys
L=open('/tmp/madtv_state.txt').read().splitlines()
S={l.split()[0]:{k:v for k,v in (kv.split('=') for kv in l.split()[1:])} for l in L}
def need(c,m):
    if not c: print('FAIL:',m); sys.exit(1)
g=lambda tag,k:int(S[tag][k])
need('later' in S,'script nao completou (estados ate: %s)'%list(S))
need(g('start','money')==2500,'dinheiro inicial != 2500')
need(g('bought','money')<g('start','money'),'compra de filme nao debitou')
need(g('placed','slot0movie')!=255,'filme nao entrou na grade')
need(g('signed','contract0')!=255,'contrato nao assinado')
need(g('news','slate0')!=255 and g('news','money')<g('signed','money'),'noticia nao comprada')
need(g('borrowed','debt')==500 and g('borrowed','money')>g('news','money'),'emprestimo nao registrado')
need(g('sold','money')>g('borrowed','money'),'venda de filme nao creditou')
need(g('changed','debt')>g('saved','debt'),'estado nao mudou antes do load')
need(g('loaded','debt')==g('saved','debt') and g('loaded','money')==g('saved','money'),'LOAD por digitacao nao restaurou (debt/money)')
need(g('loaded','day')==g('saved','day'),'LOAD nao restaurou o dia')
need(g('later','day')>=2,'jogo nao avancou para o dia 2')
need(sum(map(int,S['later']['image'].split(',')))<=100,'Image invalido')
d=open('/tmp/madtv_draw.txt').read().split()
names=['title','hub','grid','agency','ads','news','archive','boss','ratings','save','over']
print('desenho completo por tela (jiffies, max):',dict(zip(names,map(int,d))))
need(max(map(int,d))<=90,'alguma tela leva > 90 jiffies para desenhar')
print('PASS')
P
