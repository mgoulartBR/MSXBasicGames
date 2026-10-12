#!/bin/bash
# Testes de SRAM: (A) persistencia entre execucoes (salva no slot 1 numa execucao, carrega via Esc no titulo noutra),
# (B) ROM sem SRAM (mapper ASCII8 comum): g_SramBit = 0 e Escritorio > Save/Load abre direto a tela do codigo.
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}; MAP=out/madtv.map
sym() { awk -v s="$1" '$2==s{print "0x"$1}' $MAP | head -1; }
G=$(sym _g_Game); OFFS=$(sym _g_DbgOffsets); SB=$(sym _g_SramBit); SCR=$(sym _g_Screen); SEL=$(sym _g_BldSel); SS=$(sym _g_SndSfx); PS=$(sym _g_PtrSpeed)
PERS=$HOME/.openMSX/persistent; rm -rf "$PERS"/roms/madtv-* 2>/dev/null
hdr() { cat <<T
set G $G
set OFFS $OFFS
proc off {i} { global OFFS; return [peek16 [expr {\$OFFS + 2*\$i}]] }
proc press {row mask} { keymatrixdown \$row \$mask; after time 0.12 "keymatrixup \$row \$mask" }
set t 8
proc at {dt s} { global t; set t [expr {\$t + \$dt}]; after time \$t \$s }
proc key {row mask} { at 0.3 "press $row \$mask" }
proc money {} { global G; return [expr {[peek16 [expr {\$G+[off 2]}]]}] }
T
}
# --- A1: salva no slot 1 com o dinheiro alterado
{ hdr; cat <<T
at 0 { press 7 0x04 }
at 0.6 { press 7 0x80 }
at 1.6 { poke [expr {\$G+[off 2]}] 0x39; poke [expr {\$G+[off 2]+1}] 0x05 }       ;# dinheiro = 1337
at 0.3 { poke $SEL 9 }
at 0.2 { press 7 0x80 }
at 4.5 {}
at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 7 0x80 }      ;# Escritorio > Save/Load
at 0.4 { press 7 0x80 }; at 0.4 { press 7 0x80 }                                  ;# Save to a slot > slot 1
at 0.5 { set f [open /tmp/sram_a.txt w]; puts \$f "sbit=[peek $SB] money=[money]"; close \$f; exit }
T
} > /tmp/sram_a.tcl
# --- A2: nova execucao: Esc no titulo carrega o slot 1
{ hdr; cat <<T
at 0 { press 7 0x04 }
at 0.6 { press 7 0x04 }                                                             ;# Esc no titulo (o 1o dispensa a abertura)
at 1.2 { press 7 0x80 }                                                             ;# slot 1 (carregar)
at 1.0 { set f [open /tmp/sram_b.txt w]; puts \$f "sbit=[peek $SB] money=[money] scr=[peek $SCR]"; close \$f; exit }
T
} > /tmp/sram_b.tcl
# --- B: sem SRAM
{ hdr; cat <<T
at 0 { press 7 0x04 }
at 0.6 { press 7 0x80 }
at 1.6 { poke $SEL 9 }
at 0.2 { press 7 0x80 }
at 4.5 {}
at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 7 0x80 }
at 0.6 { set f [open /tmp/sram_c.txt w]; puts \$f "sbit=[peek $SB] scr=[peek $SCR]"; close \$f; exit }
T
} > /tmp/sram_c.tcl
# --- A3: apaga o slot 1 (menu: Erase a slot > slot 1 > OK de novo para confirmar)
{ hdr; cat <<T
at 0 { press 7 0x04 }
at 0.6 { press 7 0x80 }
at 1.6 { poke $SEL 9 }
at 0.2 { press 7 0x80 }
at 4.5 {}
at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 7 0x80 }
at 0.4 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 7 0x80 }
at 0.4 { press 7 0x80 }; at 0.4 { press 7 0x80 }
at 0.5 { exit }
T
} > /tmp/sram_d.tcl
# --- A4: Esc no titulo depois de apagar: nao ha slot usado, vai para a tela do codigo (scr=9)
{ hdr; cat <<T
at 0 { press 7 0x04 }
at 0.6 { press 7 0x04 }
at 1.5 { set f [open /tmp/sram_e.txt w]; puts \$f "sbit=[peek $SB] scr=[peek $SCR]"; close \$f; exit }
T
} > /tmp/sram_e.tcl
# --- F1: Options > efeitos sonoros desligados (grava na SRAM); F2: nova execucao ja le g_SndSfx = 0 e a velocidade do mouse
{ hdr; cat <<T
at 0 { press 7 0x04 }
at 0.6 { press 7 0x80 }
at 1.6 { poke $SEL 9 }
at 0.2 { press 7 0x80 }
at 4.5 {}
at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 7 0x80 }
at 0.4 { press 7 0x80 }
at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x40 }; at 0.3 { press 8 0x80 }
at 0.5 { exit }
T
} > /tmp/sram_f.tcl
{ hdr; cat <<T
at 1.5 { set f [open /tmp/sram_g.txt w]; puts \$f "sfx=[peek $SS] pspd=[peek $PS]"; close \$f; exit }
T
} > /tmp/sram_g.tcl
rm -f /tmp/sram_?.txt
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8SRAM8 -script /tmp/sram_a.tcl >/dev/null 2>&1
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8SRAM8 -script /tmp/sram_b.tcl >/dev/null 2>&1
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -script /tmp/sram_c.tcl >/dev/null 2>&1
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8SRAM8 -script /tmp/sram_d.tcl >/dev/null 2>&1
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8SRAM8 -script /tmp/sram_e.tcl >/dev/null 2>&1
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8SRAM8 -script /tmp/sram_f.tcl >/dev/null 2>&1
timeout 60 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8SRAM8 -script /tmp/sram_g.tcl >/dev/null 2>&1
cat /tmp/sram_a.txt /tmp/sram_b.txt /tmp/sram_c.txt /tmp/sram_e.txt /tmp/sram_g.txt
python3 -I - <<'P'
import sys
def rd(f): return dict(kv.split('=') for kv in open(f).read().split())
def need(c,m):
    if not c: print('FAIL:',m); sys.exit(1)
a,b,c,e,g=rd('/tmp/sram_a.txt'),rd('/tmp/sram_b.txt'),rd('/tmp/sram_c.txt'),rd('/tmp/sram_e.txt'),rd('/tmp/sram_g.txt')
need(g['sfx']=='0' and g['pspd']=='2','F: opcoes (sem efeitos, mouse rapido) nao voltaram da SRAM: %s'%g)
need(e['scr']=='9','A3/A4: slot 1 nao foi apagado (Esc no titulo ainda abre os slots)')
need(a['sbit']=='64' and a['money']=='1337','A1: SRAM nao detectada ou dinheiro de teste nao aplicado')
need(b['sbit']=='64' and b['money']=='1337' and b['scr']=='1','A2: o slot salvo noutra execucao nao foi carregado pelo Esc do titulo (%s)'%b)
need(c['sbit']=='0' and c['scr']=='9','B: sem SRAM deveria abrir direto a tela do codigo (%s)'%c)
print('PASS')
P
