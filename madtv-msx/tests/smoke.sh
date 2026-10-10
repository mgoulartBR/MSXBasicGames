#!/bin/bash
# Uso: tests/smoke.sh [rom]   (SHOTS=1 tests/smoke.sh para screenshots fieis, ~2,5 min; sem SHOTS roda em ~3 s mas os screenshots podem sair defasados)
# Smoke test 0.5 (openMSX, C-BIOS MSX2): joga por teclas injetadas e confere o estado interno via mapa de simbolos.
# Cobre: titulo -> hub -> comprar filmes -> grade -> contrato -> noticias -> credito -> vender -> salvar/carregar (codigo digitado
# tecla a tecla) -> simular dias em velocidade maxima. Screenshots em screenshots/test/.
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}
MAP=out/madtv.map
OUT=screenshots/test; mkdir -p $OUT; rm -f $OUT/*.png /tmp/madtv_state.txt
sym() { awk -v s="$1" '$2==s{print "0x"$1}' $MAP | head -1; }
DM=$(sym _g_DrawMax); SCR=$(sym _g_Screen); PX=$(sym _g_PtrX); PY=$(sym _g_PtrY); INJ=$(sym _g_PtrInject); GSEL=$(sym _g_Sel); SPD=$(sym _g_Speed); SEL=$(sym _g_BldSel); G=$(sym _g_Game); OFFS=$(sym _g_DbgOffsets); CODE=$(sym _g_SaveCode); SFXC=$(sym _g_SfxCount); MUSP=$(sym _g_MusicPos); PSPD=$(sym _g_PtrSpeed); SNDS=$(sym _g_SndSfx)
[ -n "$G" ] && [ -n "$OFFS" ] && [ -n "$CODE" ] || { echo "FAIL: simbolos ausentes no mapa"; exit 1; }
cat > /tmp/madtv_smoke.tcl <<TCL
set G $G
set OFFS $OFFS
set CODE $CODE
set SFXC $SFXC
set MUSP $MUSP
set PSPD $PSPD
set SNDS $SNDS
set DM $DM
set SCR $SCR
set PX $PX
set PY $PY
set INJ $INJ
set GSEL $GSEL
set SPD $SPD
set SEL $SEL
set SHOTDIR $PWD/$OUT
set ::env(SHOTS) ${SHOTS:-0}
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
  puts $f "$tag gsel=[peek $::GSEL] spd=[peek $::SPD] scr=[peek $::SCR] sel=[peek $::SEL] day=[peek16 $G] t=[peek16 [expr {$G+[off 1]}]] money=$money debt=[peek16 [expr {$G+[off 3]}]] image=[peek [expr {$G+[off 4]}]],[peek [expr {$G+[off 4]+1}]] slot0movie=[peek [expr {$G+[off 6]}]] slate0=[peek [expr {$G+[off 7]}]] sub0=[peek [expr {$G+[off 8]}]] contract0=[peek [expr {$G+[off 9]}]] owned1=[peek [expr {$G+[off 5]+1}]] owned2=[peek [expr {$G+[off 5]+2}]] sym0=[peek [expr {$G+[off 10]}]] g0=[peek [expr {$G+[off 12]}]] g1=[peek [expr {$G+[off 12]+1}]] g9=[peek [expr {$G+[off 12]+9}]] won=[peek [expr {$G+[off 14]}]] tower=[peek [expr {$G+[off 15]}]] pstate=[peek [expr {$G+[off 16]}]] sfx=[peek $::SFXC] mus=[peek $::MUSP] pspd=[peek $::PSPD] snds=[peek $::SNDS] psg7=[debug read {PSG regs} 7]"
  close $f
}
proc readcode {} { global CODE; set s ""; for {set i 0} {$i < 140} {incr i} { append s [format %c [peek [expr {$CODE+$i}]]] }; return $s }
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
if {!$::env(SHOTS)} { set throttle off }     ;# modo rapido (so asserts); SHOTS=1 = tempo real, screenshots confiaveis
proc at {dt script} { global t; set t [expr {$t + $dt}]; after time $t $script }
proc key {k} { at 0.3 "press {*}\$::$k" }
proc room {i} { at 0.2 "poke $::SEL $i"; key RET; at 4.5 {} }              ;# escolhe a sala (indice) e viaja ate a porta (ate ~4 s)
proc mouse {x y btn} { at 0.1 "poke $::PX $x; poke $::PY $y"; at 0.2 "poke $::INJ $btn"; at 0.3 {} }   ;# btn: 1 = so mover, 2 = clique esquerdo, 4 = direito
proc back {} { key ESC; at 0.3 {} }
at 0   { shot 1_title }
key RIGHT; at 0.3 { shot 1b_title_hard }
key LEFT; at 0.3 {}
key RET
at 1.6 { shot 2_building; dump start }
# --- navegacao do cursor no predio (teclas): UP -> BOSS(12), DOWN -> OFFC(9), RIGHT -> FUN(10), LEFT -> OFFC(9)
key UP;    at 0.3 { dump navup }
key DOWN;  at 0.3 { dump navdown }
key RIGHT; at 0.3 { dump navright }
key LEFT;  at 0.3 { dump navleft; shot 2b_cursor }
# --- MOUSE (cursor injetado: o caminho de hit-test e o mesmo do mouse real)
mouse 84 140 1                                      ;# passa por cima da ADS (sala 4): realca, sem viajar
at 0.3 { dump mhover }
mouse 220 5 2                                       ;# clique na velocidade do cabecalho: muda a velocidade
at 0.3 { dump mspeed }
mouse 220 5 2                                       ;# 2a vez: velocidade 3
mouse 220 5 2                                       ;# 3a vez: volta a velocidade 1
mouse 40 140 2                                      ;# clique na FILM (sala 3): viaja e entra na Agencia
at 5.0 { dump mclick }
mouse 100 70 1                                      ;# passa por cima da 4a linha da lista
at 0.3 { dump mlist }
mouse 100 70 2                                      ;# clique: compra o filme (Agencia: clique = comprar)
at 0.5 { dump mbuy; shot 3m_mouse_buy }
mouse 100 100 4                                     ;# botao direito = voltar ao predio
at 0.5 { dump mback }
# --- Film agency (sala 3): comprar os filmes 1 e 2 de Lovestory
room 3
key DOWN; key RET; key DOWN; key RET
at 0.6 { shot 3_agency; dump bought }
back
# --- Escritorio (sala 9) -> Programme grid: colocar o 1o filme possuido no slot 0
room 9
at 0.6 { shot 3b_office; dump office }
key RET
at 0.5 {}; key RET; at 0.3 {}; key DOWN; key RET
at 0.6 { shot 4_grid; dump placed }
back
back
# --- Ad agency (sala 4): assinar oferta 0
room 4
key RET
at 0.6 { shot 5_ads; dump signed }
back
# --- News room (sala 5): comprar a 1a noticia recebida (linha 4)
room 5
for {set i 0} {$i < 4} {incr i} { key DOWN }
key RET
at 0.6 { shot 6_news; dump news }
back
# --- Boss (sala 12): tomar emprestimo (item 0)
room 12
key RET
at 0.6 { shot 7_boss; dump borrowed }
back
# --- Archive (sala 1): vender o 2o filme da lista (o 1o esta na grade)
room 1
key DOWN; key RET
at 0.6 { shot 8_archive; dump sold }
back
# --- Supermercado (sala 2): comprar Chocolates e Flowers
room 2
key RET; at 0.4 {}; key DOWN; at 0.3 {}; key RET
at 0.6 { shot 8b_shop; dump shop }
back
# --- Betty (sala 13): dar os Chocolates; pedir em casamento sem condicoes (deve recusar)
room 13
key RET
at 0.5 { shot 8c_betty; dump gift }
for {set i 0} {$i < 10} {incr i} { key DOWN }
key RET
at 0.5 { dump refused }
back
# --- Porteiro (sala 0) e escritorios dos rivais (salas 10 e 11)
room 0
at 0.5 { shot 8d_porter; dump porter }
back
room 10
at 0.5 { shot 8e_fun; dump fun }
back
room 11
at 0.5 { dump sun }
back
# --- Opcoes (Escritorio, 4o item): desliga os efeitos e aumenta a velocidade do mouse
room 9
key DOWN; key DOWN; key DOWN; key RET
at 0.5 { dump opt0 }
key RET
at 0.3 { dump opt1 }
key DOWN; key DOWN; key RIGHT
at 0.3 { dump opt2 }
key LEFT
at 0.3 { dump opt3 }
back
back
# --- Corretor (sala 8): construir a 1a torre
room 8
at 0.5 { dump realtor0 }
key RET
at 0.5 { shot 8f_realtor; dump realtor }
back
# --- Roteiros (sala 6) e Estudios (sala 7): comprar roteiro e iniciar a producao (caixa reposto por poke)
at 0.2 { poke [expr {$G+[off 2]}] 0x88; poke [expr {$G+[off 2]+1}] 0x13; poke [expr {$G+[off 2]+2}] 0; poke [expr {$G+[off 2]+3}] 0 }
room 6
key RET
at 0.5 { shot 8g_scripts; dump script }
back
room 7
key DOWN; key RET
at 0.5 { shot 8h_studio; dump studio }
back
# --- Escritorio -> Save/Load: mostrar o codigo e guardar
room 9
key DOWN; key DOWN; key RET
at 0.5 {}; key RET
at 0.8 { shot 9_savecode; set ::SAVED [readcode]; dump saved; set f [open /tmp/madtv_code.txt w]; puts $f $::SAVED; close $f }
back
back
back
# --- mudar o estado (mais credito), depois carregar o codigo digitando-o tecla a tecla
room 12
key RET; key RET
at 0.5 { dump changed }
back
room 9
key DOWN; key DOWN; key RET
at 0.5 {}; key DOWN; key RET                        ;# Enter a code
at 0.5 { shot 10_enter_empty }
at 0.3 { typecode $::SAVED }
at 29.0 { shot 11_enter_typed }
key RET                                             ;# confirma
at 1.0 { shot 12_loaded; dump loaded }
# --- Final feliz: forca simpatia 100, rivais falidos e uma Dream trip; pedir em casamento
back
back
back
room 13
at 0.2 { poke [expr {$G+[off 4]}] 100; poke [expr {$G+[off 4]+1}] 0; poke [expr {$G+[off 4]+2}] 0; poke [expr {$G+[off 10]}] 100; poke [expr {$G+[off 11]+1}] 0; poke [expr {$G+[off 11]+2}] 0; poke [expr {$G+[off 12]+9}] 1 }
for {set i 0} {$i < 10} {incr i} { key DOWN }
key RET
at 0.5 { shot 12b_win; dump win }
key RET
at 0.5 {}
# --- simular dias em velocidade maxima
at 0.2 { poke $::SPD 3 }
at 0.1 { set throttle off }
at 95  { shot 13_later; dump later; set f [open /tmp/madtv_draw.txt w]; for {set i 0} {$i < 22} {incr i} { puts -nonewline $f "[peek [expr {$DM+$i}]] " }; close $f; exit }
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
need(g('start','scr')==1 and g('start','sel')==9,'inicio nao esta no predio com cursor no escritorio')
need((g('navup','sel'),g('navdown','sel'),g('navright','sel'),g('navleft','sel'))==(12,9,10,9),'navegacao do cursor no predio (cima/baixo/direita/esquerda)')
need(g('mhover','sel')==4 and g('mhover','scr')==1,'mouse: passar por cima da porta nao realcou/ nao deveria viajar')
need(g('mspeed','spd')!=g('start','spd'),'mouse: clique na velocidade do cabecalho nao mudou a velocidade')
need(g('mclick','scr')==3 and g('mclick','sel')==3,'mouse: clique na porta nao levou a Agencia')
need(g('mlist','gsel')==3,'mouse: hover na lista nao moveu o cursor')
need(g('mbuy','money')<g('mclick','money'),'mouse: clique na linha da Agencia nao comprou')
need(g('mback','scr')==1,'mouse: botao direito nao voltou ao predio')
need(g('bought','scr')==3 and g('office','scr')==10 and g('placed','scr')==2 and g('signed','scr')==4 and g('news','scr')==5 and g('borrowed','scr')==7 and g('sold','scr')==6 and g('saved','scr')==9,'viagem ate as salas nao abriu a tela esperada')
need(g('shop','g0')==1 and g('shop','g1')==1 and g('shop','money')<g('sold','money'),'supermercado nao vendeu os presentes')
need(g('gift','scr')==12 and g('gift','g0')==0 and g('gift','sym0')>0,'Betty: presente nao subiu a simpatia')
need(g('porter','scr')==13 and g('fun','scr')==14 and g('sun','scr')==15,'Porteiro/escritorios dos rivais nao abriram')
need(g('realtor0','scr')==16 and g('realtor0','tower')==0 and g('realtor','tower')==1 and g('realtor','money')<g('realtor0','money'),'Corretor nao vendeu a torre')
need(g('script','scr')==17 and g('script','pstate')==1 and g('studio','scr')==18 and g('studio','pstate')==2,'Roteiros/Estudios: compra do roteiro ou inicio da producao falhou')
need(g('opt0','scr')==19 and g('opt0','snds')==1 and g('opt1','snds')==0 and g('opt0','pspd')==1 and g('opt2','pspd')==2 and g('opt3','pspd')==1,'Opcoes: som/velocidade do mouse nao mudaram')
need(g('navup','sfx')>g('start','sfx') and g('opt0','sfx')>g('navup','sfx'),'efeitos sonoros nao dispararam (nav/OK)')
need(len(set(g(t,'mus') for t in ['start','navup','navdown','navright','navleft','mhover']))>=3 and all(g(t,'psg7')==0xB8 for t in ['start','navup']),'musica nao avanca / mixer do PSG != B8h')
need(g('refused','won')==0 and g('refused','scr')==12,'Betty: pedido sem condicoes foi aceito')
need(g('win','won')==1 and g('win','scr')==20,'Betty: pedido com todas as condicoes nao levou ao final feliz')
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
names=['title','building','grid','agency','ads','news','archive','boss','ratings','save','office','shop','betty','porter','fun','sun','realtor','scripts','studio','options','over']
print('desenho completo por tela (jiffies, max):',dict(zip(names,map(int,d))))
need(max(map(int,d))<=90,'alguma tela leva > 90 jiffies para desenhar')
print('PASS')
P
