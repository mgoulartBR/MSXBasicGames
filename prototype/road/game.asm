; Protótipo "game" (v6) para MSX2+ (V9958), SCREEN 5.
; Tela em 3 regiões, cada uma com PÁGINA de VRAM e PALETA próprias, trocadas por interrupção de linha:
;   topo (céu + skyline com scroll próprio) | pista (84 linhas, zonas R#26/R#27/R#23) | cockpit/HUD
; Pista real (track.inc, derivado da ROM do usuário: gerado localmente, não versionar) + carros em sprites.
        include "tconfig.inc"           ; SPEEDF, PALDIR, ILV, HSHIFT, STARTPOS
        include "gamedefs.inc"          ; constantes de geometria (somente EQU)

CMDP    equ 0x99
cb_sx   equ 0xC000
cb_sy   equ 0xC002
cb_dx   equ 0xC004
cb_dy   equ 0xC006
cb_nx   equ 0xC008
cb_ny   equ 0xC00A
cb_clr  equ 0xC00C
cb_arg  equ 0xC00D
cb_cmd  equ 0xC00E
fx      equ 0xC010
fy      equ 0xC012
fw      equ 0xC014
fh      equ 0xC016
fc      equ 0xC018
fcmd    equ 0xC019
curveabs equ 0xC020
curvesgn equ 0xC021
pos     equ 0xC022
pfrac   equ 0xC027
hcur    equ 0xC028
zidx    equ 0xC029
zready  equ 0xC030
zwr     equ 0xC032
zptr    equ 0xC034
palptr  equ 0xC036
tr      equ 0xC038
hwv     equ 0xC039
sbv     equ 0xC03A
skyacc  equ 0xC040
skys    equ 0xC042
vbc     equ 0xC100
idlecnt equ 0xC102
zbuf0   equ 0xC200
zbuf1   equ 0xC280
zsarr   equ 0xC300                      ; deslocamento X (s) por zona de pista
zr23    equ zsarr+32                    ; R#23 por zona de pista
satbuf  equ 0xC340                      ; tabela de atributos de sprites (28 bytes)
carstate equ 0xC360                     ; 3 carros x 6 bytes: pos16, frac, vel, faixa, pad
carpat  equ 0xC380
caryb   equ 0xC381
cars_s  equ 0xC382
carx    equ 0xC383
palsel  equ 0xC3A0                      ; ponteiros das paletas: [0]=topo [1]=pista(fase) [2]=cockpit
hwk     equ 0xC050                      ; meia largura da pista da zona atual
xpabs   equ 0xC051                      ; |posição lateral do jogador| (16 bits)
xpneg   equ 0xC053                      ; 1 se o jogador está à esquerda do centro
xp      equ 0xC054                      ; posição lateral do jogador (256 = borda da pista)
spd16   equ 0xC056                      ; velocidade (8.8): 0..25600 = 0..300 km/h
inp_thr equ 0xC058                      ; acelerar
inp_brk equ 0xC059                      ; frear
inp_str equ 0xC05A                      ; direção (-1, 0, +1)
lap     equ 0xC05B
rank    equ 0xC05C
hit     equ 0xC05D                      ; 1 = colidiu neste quadro
carsz   equ 0xC05E
hu_n    equ 0xC05F
hu_left equ 0xC060
hudrr   equ 0xC061
cardq   equ 0xC062
hudwant equ 0xC400                      ; 18 células do HUD: valor desejado
hudshow equ 0xC420                      ; valor exibido
IM2TAB  equ 0xE000
IM2STUB equ 0xE1E1
PAGE0   equ 0x1F                        ; R#2: página 0 (textura da pista)
PAGE1   equ 0x3F                        ; R#2: página 1 (topo e cockpit)

        org 0x4000
        defb "AB"
        defw init
        defw 0,0,0
        defs 6

init:   di
        ld sp,0xF300
        call 0x0138                     ; RSLREG: ROM de 32 KB => habilita o slot do cartucho também na página 2
        rrca
        rrca
        and 3
        ld c,a
        ld b,0
        ld hl,0xFCC1                    ; EXPTBL
        add hl,bc
        or (hl)
        ld c,a
        inc hl
        inc hl
        inc hl
        inc hl                          ; SLTTBL
        ld a,(hl)
        and 0x0C
        or c
        ld h,0x80
        call 0x0024                     ; ENASLT
        di
        ld a,5
        call 0x005F                     ; CHGMOD: SCREEN 5
        di
        ld a,0x28                       ; sprites ligados + TP (cor 0 usa a paleta)
        ld b,8
        call vdp_reg
        ld a,0x63                       ; display + IE0 + sprites 16x16 AMPLIADOS (32x32)
        ld b,1
        call vdp_reg
        ld a,0x80                       ; 212 linhas
        ld b,9
        call vdp_reg
        ld a,0xF2                       ; cor de fundo = grama (a máscara do scroll fino usa R#7)
        ld b,7
        call vdp_reg
        xor a
        ld b,25
        call vdp_reg
        xor a
        ld b,26
        call vdp_reg
        xor a
        ld b,27
        call vdp_reg
        xor a
        ld b,23
        call vdp_reg
        ld a,PAGE1
        ld b,2
        call vdp_reg
        xor a
        ld b,16
        call vdp_reg
        ld hl,palbase
        ld bc,0x209A
        otir

        ; --- IM2: tabela de 257 bytes iguais, stub JP isr em RAM
        ld hl,IM2TAB
        ld de,IM2TAB+1
        ld bc,256
        ld (hl),0xE1
        ldir
        ld a,0xC3
        ld (IM2STUB),a
        ld hl,isr
        ld (IM2STUB+1),hl
        ld a,IM2TAB>>8
        ld i,a
        im 2

        ; --- variáveis
        ld hl,STARTPOS
        ld (pos),hl
        xor a
        ld (pfrac),a
        ld hl,0
        ld (vbc),hl
        ld (idlecnt),hl
        ld (skyacc),hl
        ld hl,zbuf0
        ld (zwr),hl
        ld (zready),hl
        ld hl,0
        ld (spd16),hl
        ld (xp),hl
        ld (xpabs),hl
        xor a
        ld (xpneg),a
        ld (hudrr),a
        ld a,1
        ld (lap),a
        ld (rank),a
        ld hl,hudshow
        ld de,hudshow+1
        ld bc,17
        ld (hl),0xFF
        ldir
        ld hl,hudwant
        ld de,hudwant+1
        ld bc,17
        ld (hl),0
        ldir
        ld hl,pal_top
        ld (palsel),hl
        ld hl,pal_bot
        ld (palsel+4),hl
        ld a,2
        ld b,15
        call vdp_reg                    ; R#15=2: lê S#2 (CE) durante o desenho

        ; --- página 0: grama + textura da pista (linhas de VRAM ROADY..ROADY+TEXROWS-1)
        ld a,0xC0
        ld (fcmd),a
        ld hl,0
        ld (fx),hl
        ld hl,256
        ld (fw),hl
        ld hl,ROADY
        ld (fy),hl
        ld hl,TEXROWS
        ld (fh),hl
        ld a,0x22
        ld (fc),a
        call fill
        ld a,0x80
        ld (fcmd),a
        ld hl,1
        ld (fh),hl
        xor a
        ld (tr),a
trl:    ld a,(tr)
        add a,ROADY
        ld l,a
        ld h,0
        ld (fy),hl
        ld a,(tr)
        ld e,a
        ld d,0
        ld hl,hwtab
        add hl,de
        ld a,(hl)
        ld (hwv),a
        ld hl,sbt
        add hl,de
        ld a,(hl)
        ld (sbv),a
        ; zebra
        ld a,(hwv)
        ld e,a
        ld d,0
        ld hl,128
        or a
        sbc hl,de
        ld (fx),hl
        ld h,d
        ld l,e
        add hl,hl
        ld (fw),hl
        ld a,(sbv)
        add a,8
        ld (fc),a
        call fill
        ; asfalto
        ld a,(hwv)
        ld b,a
        srl a
        srl a
        srl a
        inc a
        ld c,a
        ld a,b
        sub c
        ld e,a
        ld d,0
        ld hl,128
        or a
        sbc hl,de
        ld (fx),hl
        ld h,d
        ld l,e
        add hl,hl
        ld (fw),hl
        ld a,(sbv)
        add a,4
        ld (fc),a
        call fill
        ; faixa central
        ld a,(hwv)
        srl a
        srl a
        srl a
        srl a
        srl a
        inc a
        ld e,a
        ld d,0
        ld hl,128
        or a
        sbc hl,de
        ld (fx),hl
        ld h,d
        ld l,e
        add hl,hl
        ld (fw),hl
        ld a,15
        ld (fc),a
        call fill
        ld a,(tr)
        inc a
        ld (tr),a
        cp TEXROWS
        jp nz,trl

        ; --- página 1: céu/skyline e cockpit (listas de retângulos; y + 256)
        ld a,0x80
        ld (fcmd),a
        ld hl,arttop
        ld de,256
        call drawart
        ld hl,artcock
        ld de,256
        call drawart
        call waitce

        ; --- sprites: estado dos carros, cores por linha (0x7400) e padrões (0x7800)
        ld hl,carinit
        ld de,carstate
        ld bc,18
        ldir
        ld a,1
        out (CMDP),a
        ld a,14+0x80
        out (CMDP),a
        xor a
        out (CMDP),a
        ld a,0x74
        out (CMDP),a
        ld hl,sprcol
        ld bc,0xC098
        otir
        xor a
        out (CMDP),a
        ld a,0x78
        out (CMDP),a
        ld hl,sprpat
        ld bc,0x0098
        otir
        ld bc,0x8098
        otir

        ; --- rótulos estáticos do HUD (glifos)
        ld hl,labtab
lab1:   ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld a,d
        or e
        jr z,lab2
        ld a,(hl)
        inc hl
        push hl
        ld l,a
        ld h,0
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl
        ld bc,fontdata
        add hl,bc
        call putglyph
        pop hl
        jr lab1
lab2:
        ld a,1
        ld b,15
        call vdp_reg                    ; R#15=1: a ISR lê S#1 (flag de linha)
        call frame                      ; pré-calcula o 1º buffer de zonas
        call hudcalc
        ei

; ------------------------------------------------------------ laço principal
main:   ld a,(vbc)
        ld b,a
wait:   ld hl,(idlecnt)                 ; mede a folga de CPU (67 T por volta)
        inc hl
        ld (idlecnt),hl
        ld a,(vbc)
        cp b
        jr z,wait
        call frame
        call hudcalc
        jp main

; ------------------------------------------------------------ cálculo por quadro
; Buffer de zonas (6 bytes cada): R#26, R#27, R#23, R#2 (página), paleta (0=nenhuma), linha da próxima interrupção
;   E0 = skyline | E1..E<ZN> = pista | E<ZN+1> = cockpit
frame:  call readinput
        call physics
        ld a,(spd16+1)                  ; velocidade (0..100) = incremento de pfrac por quadro
        ld e,a
        ld a,(pfrac)
        add a,e
        ld (pfrac),a
        jr nc,nopinc
        ld hl,(pos)
        inc hl
        ld de,TRKLEN
        or a
        sbc hl,de
        add hl,de
        jr c,pok                        ; pos < TRKLEN
        ld hl,0                         ; fim da volta: recomeça
        ld a,(lap)
        inc a
        ld (lap),a
pok:    ld (pos),hl
nopinc: ; fase da paleta = (pfrac >> 6) & 3  ->  palsel[1] = paltab + 32*fase
        ld a,(pfrac)
        rlca
        rlca
        IF PALDIR
        neg
        ENDIF
        and 3
        rrca
        rrca
        rrca
        ld e,a
        ld d,0
        ld hl,paltab
        add hl,de
        ld (palsel+2),hl
        ; altura na posição atual
        ld hl,(pos)
        ld de,trkh
        add hl,de
        ld a,(hl)
        ld (hcur),a
        ; scroll do skyline: acumula curvatura * velocidade
        ld hl,(pos)
        ld de,trkc
        add hl,de
        ld a,(hl)
        ld c,a
        or a
        jp p,sk1
        neg
sk1:    ld b,a
        ld a,(spd16+1)
        ld e,a
        ld d,0
        ld a,b
        call mulq                       ; HL = |c| * vel
        ld a,h
        srl a                           ; delta = HL >> 9
        ld b,a
        ld a,c
        or a
        ld a,b
        ld d,0
        jp p,sk2
        neg
        ld d,0xFF
sk2:    ld e,a
        ld hl,(skyacc)
        add hl,de
        ld (skyacc),hl
        ld a,4
sk3:    srl h
        rr l
        dec a
        jr nz,sk3
        ld a,l
        ld (skys),a                     ; scroll do skyline em pixels (int8, dá a volta em 256)
        ; --- E0: skyline
        ld ix,(zwr)
        ld b,a
        and 7
        ld (ix+1),a                     ; R#27 = s & 7
        ld a,b
        neg
        add a,7
        sra a
        sra a
        sra a
        and 31
        ld (ix+0),a                     ; R#26 = ((7 - s) >> 3) & 31
        xor a
        ld (ix+2),a
        ld (ix+4),a
        ld a,PAGE1
        ld (ix+3),a
        ld a,ILROAD0
        ld (ix+5),a
        ld de,6
        add ix,de
        ld iy,zsarr
        ld hl,zt3
        ld b,ZN
zl:     ld e,(hl)                       ; q (0..255)
        inc hl
        ld c,(hl)                       ; distância adiante (posições)
        inc hl
        ld a,(hl)
        inc hl
        ld (ix+5),a                     ; linha-base da próxima interrupção
        ld a,(hl)
        inc hl
        ld (hwk),a                      ; meia largura da pista nessa zona
        push hl
        push bc
        ld b,0
        ld hl,(pos)
        add hl,bc                       ; idx = pos + adiante
        ld de,TRKLEN                    ; (E=q é recarregado abaixo)
        or a
        sbc hl,de
        jr nc,wr1
        add hl,de
wr1:    ld (zidx),hl
        pop bc
        pop hl
        push hl
        push bc
        dec hl
        dec hl
        dec hl
        dec hl
        ld e,(hl)                       ; E = q de novo
        ld d,0
        ; --- curva: c = trkc[idx]
        ld hl,(zidx)
        ld bc,trkc
        add hl,bc
        ld a,(hl)
        ld c,a                          ; guarda o sinal
        or a
        jp p,cp1
        neg
cp1:    call mulq                       ; HL = |c| * q
        ld a,h
        srl a                           ; mag = |c|*q >> 9  (0..63)
        ld b,a
        ld a,c
        or a
        ld a,b
        jp p,spos
        neg                             ; s = -mag
spos:   ld c,a                          ; s da curva
        ld de,(xpabs)
        ld a,(hwk)
        call mulq                       ; HL = |xp| * meia largura
        ld a,h                          ; câmera lateral: deslocamento = HL >> 8 (0..69 px)
        ld b,a
        ld a,(xpneg)
        or a
        ld a,c
        jr nz,cam1
        sub b                           ; jogador à direita do centro: o mundo desloca para a esquerda
        jr cam2
cam1:   add a,b
cam2:   ld (iy+0),a
        ld b,a                          ; s (deslocamento à direita, com sinal)
        and 7
        ld (ix+1),a                     ; R#27 = s & 7
        ld a,b
        neg
        add a,7
        sra a
        sra a
        sra a
        and 31
        ld (ix+0),a                     ; R#26 = ((7 - s) >> 3) & 31
        ; --- relevo: dv = (h[idx] - h[pos]) >> HSHIFT, limitado a -16..+16
        ld hl,(zidx)
        ld bc,trkh
        add hl,bc
        ld a,(hl)
        ld hl,hcur
        sub (hl)
        REPT HSHIFT
        sra a
        ENDM
        cp 16
        jp m,cl1
        ld a,16
cl1:    cp -16
        jp p,cl2
        ld a,-16
cl2:    add a,16                        ; R#23 = 16 + dv
        ld (ix+2),a
        ld (iy+32),a
        IF ILV
        ld c,a
        ld a,(ix+5)
        or a
        jr z,nol
        add a,c                         ; R#19 compara com (linha + R#23)
        ld (ix+5),a
nol:
        ENDIF
        ld a,PAGE0
        ld (ix+3),a
        xor a
        ld (ix+4),a
        ld de,6
        add ix,de
        inc iy
        pop bc
        pop hl
        dec b
        jp nz,zl
        ; --- E<ZN+1>: cockpit (página 1, sem scroll, paleta do cockpit); fim da lista
        xor a
        ld (ix+0),a
        ld (ix+1),a
        ld (ix+2),a
        ld (ix+5),a
        ld a,PAGE1
        ld (ix+3),a
        ld a,3
        ld (ix+4),a
        ld ix,(zwr)
        ld a,2
        ld (ix+10),a                    ; 1ª zona de pista: troca para a paleta da pista (fase atual)
        call cars
        ; publica o buffer pronto e alterna o de escrita
        ld hl,(zwr)
        di
        ld (zready),hl
        ei
        ld de,zbuf0
        or a
        sbc hl,de
        ld hl,zbuf0
        jr nz,tz
        ld hl,zbuf1
tz:     ld (zwr),hl
        ret

; ------------------------------------------------------------ interrupção (IM2)
isr:    push af
        push hl
        push bc
        push de
        in a,(CMDP)                     ; S#1 (R#15=1): bit0 = flag de linha
        rrca
        jp nc,isr_vb
        ; --- início de região/zona: R#26, R#27, R#23, R#2; agenda a próxima; troca a paleta
        ld hl,(zptr)
        ld a,(hl)
        inc hl
        out (CMDP),a
        ld a,26+0x80
        out (CMDP),a
        ld a,(hl)
        inc hl
        out (CMDP),a
        ld a,27+0x80
        out (CMDP),a
        ld a,(hl)
        inc hl
        out (CMDP),a
        ld a,23+0x80
        out (CMDP),a
        ld a,(hl)
        inc hl
        out (CMDP),a
        ld a,2+0x80
        out (CMDP),a
        ld e,(hl)                       ; id da paleta (0 = nenhuma)
        inc hl
        ld a,(hl)                       ; linha da próxima interrupção (0 = fim)
        inc hl
        ld (zptr),hl
        or a
        jr z,isr_last
        out (CMDP),a
        ld a,19+0x80
        out (CMDP),a
        jr isr_pal
isr_last:
        ld a,0x06                       ; fim: desliga IE1
        out (CMDP),a
        ld a,0x80
        out (CMDP),a
isr_pal:ld a,e
        or a
        jp z,isr_end
        dec a
        add a,a
        ld e,a
        ld d,0
        ld hl,palsel
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        ex de,hl
        ld a,4
        out (CMDP),a
        ld a,16+0x80
        out (CMDP),a
        ld bc,0x109A
        otir
        jp isr_end
isr_vb: ld a,0                          ; --- VBlank: reconhece S#0
        out (CMDP),a
        ld a,15+0x80
        out (CMDP),a
        in a,(CMDP)
        ld a,1
        out (CMDP),a
        ld a,15+0x80
        out (CMDP),a
        ld a,PAGE1                      ; topo: página 1, sem scroll
        out (CMDP),a
        ld a,2+0x80
        out (CMDP),a
        xor a
        out (CMDP),a
        ld a,26+0x80
        out (CMDP),a
        xor a
        out (CMDP),a
        ld a,27+0x80
        out (CMDP),a
        xor a
        out (CMDP),a
        ld a,23+0x80
        out (CMDP),a
        ld a,4                          ; paleta do topo (entradas 4..11)
        out (CMDP),a
        ld a,16+0x80
        out (CMDP),a
        ld hl,pal_top
        ld bc,0x109A
        otir
        ld a,1                          ; sprites: atributos em VRAM 0x7600
        out (CMDP),a
        ld a,14+0x80
        out (CMDP),a
        xor a
        out (CMDP),a
        ld a,0x76
        out (CMDP),a
        ld hl,satbuf
        ld bc,0x3498
        otir
        call hudupd
        ld a,ILSKY
        out (CMDP),a
        ld a,19+0x80
        out (CMDP),a
        ld a,0x16                       ; liga IE1 (interrupção de linha)
        out (CMDP),a
        ld a,0x80
        out (CMDP),a
        ld hl,(zready)
        ld (zptr),hl
        ld hl,(vbc)
        inc hl
        ld (vbc),hl
isr_end:pop de
        pop bc
        pop hl
        pop af
        ei
        reti

; ------------------------------------------------------------ arte (listas de retângulos)
; registro: x, y, w (0 = 256), h (0 = fim), cor. HL = lista, DE = deslocamento de y (0 ou 256)
drawart:ld a,(hl)
        inc hl
        ld (fx),a
        xor a
        ld (fx+1),a
        ld a,(hl)
        inc hl
        push hl
        ld l,a
        ld h,0
        add hl,de
        ld (fy),hl
        pop hl
        ld a,(hl)
        inc hl
        ld (fw),a
        or a
        ld a,0
        jr nz,da1
        inc a
da1:    ld (fw+1),a
        ld a,(hl)
        inc hl
        or a
        ret z
        ld (fh),a
        xor a
        ld (fh+1),a
        ld a,(hl)
        inc hl
        ld (fc),a
        push hl
        push de
        call fill
        pop de
        pop hl
        jr drawart

; ------------------------------------------------------------ carros adversários
; 3 carros, 4 planos de sprite cada (escuro/corpo, esquerda/direita). Sprites ampliados (32x32);
; o tamanho XL usa 2 sprites lado a lado (64x32). Distância (1/4 de posição) -> ytab/sizetab/zlinetab.
cars:   xor a
        ld (hit),a
        inc a
        ld (rank),a
        ld ix,carstate
        ld iy,satbuf
        ld b,3
carlp:  push bc
        ld a,(ix+2)                     ; avança o carro: frac += vel; carry -> pos++
        add a,(ix+3)
        ld (ix+2),a
        jr nc,cnoinc
        ld l,(ix+0)
        ld h,(ix+1)
        inc hl
        ld de,TRKLEN
        or a
        sbc hl,de
        add hl,de
        jr c,cok
        ld hl,0
cok:    ld (ix+0),l
        ld (ix+1),h
cnoinc: ld l,(ix+0)                     ; distância à frente do jogador (posições)
        ld h,(ix+1)
        ld de,(pos)
        or a
        sbc hl,de
        jr nc,cd1
        ld de,TRKLEN
        add hl,de
cd1:    push hl                         ; ranking: está à frente se dist < TRKLEN/2
        ld de,TRKLEN/2
        or a
        sbc hl,de
        pop hl
        jr nc,cr1
        ld a,h
        or l
        jr z,cr1
        ld a,(rank)
        inc a
        ld (rank),a
cr1:    ld a,h
        or a
        jp nz,chide
        ld a,l
        cp 63
        jp nc,chide
        ld h,l
        ld l,0                          ; HL = dist << 8
        ld a,(pfrac)
        ld c,a
        ld a,(ix+2)
        sub c                           ; fração do carro - fração do jogador
        ld de,0
        jr nc,cf1
        dec de
cf1:    ld e,a
        add hl,de
        bit 7,h
        jp nz,chide
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l
        srl h
        rr l                            ; L = dq (distância em 1/4 de posição)
        ld a,l
        ld (cardq),a
        ld e,l
        ld d,0
        ld hl,sizetab
        add hl,de
        ld a,(hl)
        ld (carsz),a
        ld hl,ytab
        add hl,de
        ld a,(hl)
        ld (caryb),a
        sub ROADY
        ld c,a                          ; c = linha dentro da pista
        ld e,a
        ld d,0
        ld hl,zlinetab
        add hl,de
        ld e,(hl)                       ; zona dessa linha
        ld d,0
        ld hl,zsarr
        add hl,de
        ld a,(hl)
        ld (cars_s),a
        ld hl,zr23
        add hl,de
        ld a,(hl)
        add a,c                         ; linha da textura = linha + R#23
        cp TEXROWS
        jr c,ci1
        ld a,TEXROWS-1
ci1:    ld e,a
        ld d,0
        ld hl,hwtab
        add hl,de
        ld e,(hl)                       ; E = meia largura da pista nessa linha
        ld d,0
        ld a,(ix+4)                     ; faixa (-64..64 = -1..+1 da meia largura)
        ld c,a
        or a
        jp p,cl3
        neg
cl3:    call mulq                       ; HL = |faixa| * largura
        add hl,hl
        add hl,hl                       ; H = HL >> 6
        ld b,h
        ld a,c
        or a
        ld a,b
        jp p,cl4
        neg
cl4:    ld c,a                          ; deslocamento lateral com sinal
        ld hl,128
        ld a,(cars_s)
        ld e,a
        add a,a
        sbc a,a
        ld d,a
        add hl,de                       ; + deslocamento (curva + câmera) da zona
        ld a,c
        ld e,a
        add a,a
        sbc a,a
        ld d,a
        add hl,de                       ; HL = x do centro do carro
        ld a,h
        or a
        jp nz,chide
        ld a,l
        cp 16
        jp c,chide
        cp 241
        jp nc,chide
        ld (carx),a
        ld a,(cardq)                    ; colisão: muito perto e sobrepondo a faixa do jogador
        cp 5
        jr nc,cnc
        ld a,(ix+4)
        ld l,a
        add a,a
        sbc a,a
        ld h,a
        add hl,hl
        add hl,hl                       ; faixa * 4 (mesma escala de xp)
        ld de,(xp)
        or a
        sbc hl,de
        bit 7,h
        jr z,cab
        ex de,hl
        ld hl,0
        or a
        sbc hl,de
cab:    ld de,90
        or a
        sbc hl,de
        jr nc,cnc
        ld a,1
        ld (hit),a
cnc:    ld a,(carsz)
        ld e,a
        ld d,0
        ld hl,szpat
        add hl,de
        ld a,(caryb)
        sub 32
        ld c,a                          ; C = Y
        ld a,(carx)
        ld b,a                          ; B = x do centro
        ld a,(carsz)
        or a
        jr nz,cnorm
        ld a,b                          ; XL: metade esquerda em x-32, direita em x
        sub 32
        ld d,a
        ld e,b
        ld a,(hl)
        inc hl
        ld (iy+0),c
        ld (iy+1),d
        ld (iy+2),a
        ld (iy+3),0
        ld a,(hl)
        inc hl
        ld (iy+4),c
        ld (iy+5),d
        ld (iy+6),a
        ld (iy+7),0
        ld a,(hl)
        inc hl
        ld (iy+8),c
        ld (iy+9),e
        ld (iy+10),a
        ld (iy+11),0
        ld a,(hl)
        ld (iy+12),c
        ld (iy+13),e
        ld (iy+14),a
        ld (iy+15),0
        jr cnext
cnorm:  ld a,b
        sub 16
        ld d,a
        ld a,(hl)
        inc hl
        ld (iy+0),c
        ld (iy+1),d
        ld (iy+2),a
        ld (iy+3),0
        ld a,(hl)
        ld (iy+4),c
        ld (iy+5),d
        ld (iy+6),a
        ld (iy+7),0
        ld (iy+8),212
        ld (iy+12),212
        jr cnext
chide:  ld (iy+0),212
        ld (iy+4),212
        ld (iy+8),212
        ld (iy+12),212
cnext:  ld de,16
        add iy,de
        ld de,6
        add ix,de
        pop bc
        dec b
        jp nz,carlp
        ld (iy+0),216                   ; fim da lista de sprites
        ld a,(hit)
        or a
        ret z
        ld hl,(spd16)                   ; batida: perde 1/8 da velocidade por quadro
        ld d,h
        ld e,l
        srl d
        rr e
        srl d
        rr e
        srl d
        rr e
        or a
        sbc hl,de
        ld (spd16),hl
        ret

; estado inicial: pos(w), frac, vel, faixa, pad. Velocidades em 1/256 de posição por quadro.
carinit:defw STARTPOS+14
        defb 0,30,-24,0
        defw STARTPOS+30
        defb 0,36,24,0
        defw STARTPOS+46
        defb 0,44,0,0

; ------------------------------------------------------------ entrada (BIOS: GTSTCK/GTTRIG)
; setas ou joystick 1 = direção; para cima/botão/espaço = acelera; para baixo = freia.
readinput:
        push bc
        push de
        push hl
        push ix
        push iy
        xor a
        ld (inp_thr),a
        ld (inp_brk),a
        ld (inp_str),a
        xor a
        call 0x00D5                     ; GTSTCK(0): cursores
        ld b,a
        ld a,1
        call 0x00D5                     ; GTSTCK(1): joystick 1
        or a
        jr nz,ri1
        ld a,b
ri1:    ld e,a
        ld d,0
        ld hl,dirthr
        add hl,de
        ld a,(hl)
        ld (inp_thr),a
        ld hl,dirbrk
        add hl,de
        ld a,(hl)
        ld (inp_brk),a
        ld hl,dirstr
        add hl,de
        ld a,(hl)
        ld (inp_str),a
        xor a
        call 0x00D8                     ; GTTRIG(0): barra de espaço
        or a
        jr nz,ri2
        ld a,1
        call 0x00D8                     ; GTTRIG(1): botão do joystick 1
        or a
        jr z,ri3
ri2:    ld a,1
        ld (inp_thr),a
ri3:    pop iy
        pop ix
        pop hl
        pop de
        pop bc
        ret
dirthr: defb 0,1,1,0,0,0,0,0,1          ; direções 0..8 (1=cima, 3=direita, 5=baixo, 7=esquerda)
dirbrk: defb 0,0,0,0,1,1,1,0,0
dirstr: defb 0,0,1,1,1,0,-1,-1,-1

; ------------------------------------------------------------ física
physics:ld hl,(spd16)
        ld a,(inp_brk)
        or a
        jr nz,ph_br
        ld a,(inp_thr)
        or a
        jr nz,ph_ac
        ld a,h                          ; sem acelerar: atrito 6 + (v >> 2)
        srl a
        srl a
        add a,6
        jr ph_dc
ph_br:  ld a,180
ph_dc:  ld e,a
        ld d,0
        or a
        sbc hl,de
        jr nc,ph_f
        ld hl,0
        jr ph_f
ph_ac:  ld a,h                          ; aceleração 64 - (v >> 1), mínimo 8
        srl a
        ld e,a
        ld a,64
        sub e
        jr c,pa0
        cp 8
        jr nc,pa1
pa0:    ld a,8
pa1:    ld e,a
        ld d,0
        add hl,de
        ld de,25600                     ; 300 km/h
        or a
        sbc hl,de
        jr c,pa2
        ld hl,0
pa2:    add hl,de
ph_f:   ld a,(xpabs+1)                  ; fora da pista (|xp| > 256): limita a ~75 km/h
        cp 2
        jr nc,ph_of
        cp 1
        jr nz,ph_ok
        ld a,(xpabs)
        or a
        jr z,ph_ok
ph_of:  ld de,6400
        push hl
        or a
        sbc hl,de
        pop hl
        jr c,ph_ok
        ld de,120                       ; atrito extra fora da pista
        or a
        sbc hl,de
ph_ok:  ld (spd16),hl
        ld a,(spd16+1)                  ; direção: |dxp| = v >> 3
        srl a
        srl a
        srl a
        ld e,a
        ld d,0
        ld a,(inp_str)
        or a
        jr z,ph_cf
        ld hl,(xp)
        jp m,ph_lf
        add hl,de
        jr ph_sv
ph_lf:  or a
        sbc hl,de
ph_sv:  ld (xp),hl
ph_cf:  ld hl,(pos)                     ; força centrífuga: xp -= c * v >> 10
        ld de,trkc
        add hl,de
        ld a,(hl)
        ld c,a
        or a
        jp p,pc1
        neg
pc1:    ld e,a
        ld d,0
        ld a,(spd16+1)
        call mulq
        ld a,h
        srl a
        srl a
        ld e,a
        ld d,0
        ld hl,(xp)
        ld a,c
        or a
        jp m,pc2
        or a
        sbc hl,de
        jr pc3
pc2:    add hl,de
pc3:    ld (xp),hl
        bit 7,h                         ; limita xp a +-290
        jr nz,cn1
        ld de,291
        or a
        sbc hl,de
        jr c,cn3
        ld hl,290
        jr cn2
cn1:    ld de,290
        add hl,de
        bit 7,h
        jr z,cn3
        ld hl,-290
cn2:    ld (xp),hl
cn3:    ld hl,(xp)
        xor a
        ld (xpneg),a
        bit 7,h
        jr z,cn4
        ld a,1
        ld (xpneg),a
        ex de,hl
        ld hl,0
        or a
        sbc hl,de
cn4:    ld (xpabs),hl
        ret

; ------------------------------------------------------------ HUD: valores desejados
; células: 0-2 velocidade (centena, dezena, unidade), 3 marcha, 4 posição, 5 volta, 6-17 LEDs do conta-giros
hudcalc:ld a,(spd16+1)
        ld b,a                          ; B = v (0..100)
        ld l,a
        ld h,0
        ld e,a
        ld d,0
        add hl,hl
        add hl,de                       ; HL = km/h = 3 * v
        ld c,0
hc1:    ld de,100
        or a
        sbc hl,de
        jr c,hc2
        inc c
        jr hc1
hc2:    add hl,de                       ; resto (0..99)
        ld a,l
        ld d,0
hc3:    cp 10
        jr c,hc4
        sub 10
        inc d
        jr hc3
hc4:    ld e,a                          ; E = unidades, D = dezenas, C = centenas
        ld a,c
        or a
        jr nz,hc5
        ld a,10                         ; centena em branco
hc5:    ld (hudwant),a
        ld a,c
        or d
        jr nz,hc6
        ld a,10
        jr hc7
hc6:    ld a,d
hc7:    ld (hudwant+1),a
        ld a,e
        ld (hudwant+2),a
        ld a,b                          ; marcha = 1 + v / 17 (máx. 6); N parado
        ld c,1
hg1:    cp 17
        jr c,hg2
        sub 17
        inc c
        jr hg1
hg2:    ld d,a                          ; D = resto (0..16)
        ld a,c
        cp 7
        jr c,hg3
        ld a,6
hg3:    ld e,a
        ld a,b
        or a
        ld a,e
        jr nz,hg4
        ld a,11                         ; N
hg4:    ld (hudwant+3),a
        ld a,(rank)
        ld (hudwant+4),a
        ld a,(lap)
        cp 4
        jr c,hg5
        ld a,3
hg5:    ld (hudwant+5),a
        ld e,d                          ; conta-giros: LEDs acesos = revtab[resto] (+1 acelerando)
        ld d,0
        ld hl,revtab
        add hl,de
        ld c,(hl)
        ld a,(inp_thr)
        or a
        jr z,hg6
        ld a,c
        cp 12
        jr nc,hg6
        inc c
hg6:    ld hl,hudwant+6
        ld b,12
        ld d,0
hg7:    ld a,d
        cp c
        ld a,0
        jr nc,hg8
        inc a
hg8:    ld (hl),a
        inc hl
        inc d
        djnz hg7
        ret

; ------------------------------------------------------------ HUD: atualização na VRAM (chamada no VBlank)
hudupd: ld a,(hudrr)
        ld (hu_n),a
        ld a,3                          ; até 3 células por quadro
        ld (hu_left),a
        ld b,18
hu1:    push bc
        ld a,(hu_n)
        ld e,a
        ld d,0
        ld hl,hudwant
        add hl,de
        ld a,(hl)
        ld hl,hudshow
        add hl,de
        cp (hl)
        jr z,hu2
        ld (hl),a
        call hudcell
        ld hl,hu_left
        dec (hl)
        jr z,hu4
hu2:    ld a,(hu_n)
        inc a
        cp 18
        jr c,hu3
        xor a
hu3:    ld (hu_n),a
        pop bc
        djnz hu1
        ret
hu4:    pop bc
        ld a,(hu_n)
        inc a
        cp 18
        jr c,hu5
        xor a
hu5:    ld (hudrr),a
        ret

; desenha a célula E (índice) com o valor A
hudcell:ld d,0
        ld c,a                          ; valor
        ld a,e
        cp 6
        jr nc,hc_led
        ld hl,cellpos
        add hl,de
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)                       ; DE = endereço de VRAM (14 bits) do canto do glifo
        ld l,c
        ld h,0
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl                       ; valor * 32
        ld bc,fontdata
        add hl,bc
        jp putglyph
hc_led: sub 6
        ld b,a                          ; B = índice do LED
        add a,a
        add a,a
        ld l,a
        ld h,0
        ld de,167*128+6-0x4000          ; (12 + 8*i)/2 = 6 + 4*i
        add hl,de
        ex de,hl                        ; DE = endereço
        ld a,c
        or a
        ld a,5                          ; apagado
        jr z,hl1
        ld a,b
        cp 8
        ld a,9                          ; verde
        jr c,hl1
        ld a,b
        cp 11
        ld a,10                         ; âmbar
        jr c,hl1
        ld a,12                         ; vermelho
hl1:    ld c,a
        rlca
        rlca
        rlca
        rlca
        or c                            ; byte = 2 pixels da mesma cor
        ld c,a
        ld a,3
        out (CMDP),a
        ld a,14+0x80
        out (CMDP),a                    ; R#14 = 3 (VRAM 0xC000..0xFFFF, página 1)
        ld b,6
hl2:    ld a,e
        out (CMDP),a
        ld a,d
        or 0x40
        out (CMDP),a
        ld a,c
        out (0x98),a
        out (0x98),a
        out (0x98),a
        ld a,e
        add a,0x80
        ld e,a
        jr nc,hl3
        inc d
hl3:    djnz hl2
        ret

; HL = glifo (32 bytes), DE = endereço de VRAM (14 bits) da 1ª linha
putglyph:
        ld a,3
        out (CMDP),a
        ld a,14+0x80
        out (CMDP),a
        ld b,8
pg1:    ld a,e
        out (CMDP),a
        ld a,d
        or 0x40
        out (CMDP),a
        ld a,(hl)
        inc hl
        out (0x98),a
        ld a,(hl)
        inc hl
        out (0x98),a
        ld a,(hl)
        inc hl
        out (0x98),a
        ld a,(hl)
        inc hl
        out (0x98),a
        ld a,e
        add a,0x80
        ld e,a
        jr nc,pg2
        inc d
pg2:    djnz pg1
        ret

cellpos:defw 168*128+170/2-0x4000, 168*128+180/2-0x4000, 168*128+190/2-0x4000
        defw 168*128+124/2-0x4000, 182*128+22/2-0x4000, 182*128+190/2-0x4000
; rótulos fixos: endereço, glifo (12=K 13=M 14=H 15=P 16=L 17=/ 4 3)
labtab: defw 168*128+202/2-0x4000
        defb 12
        defw 168*128+212/2-0x4000
        defb 13
        defw 168*128+222/2-0x4000
        defb 14
        defw 182*128+12/2-0x4000
        defb 15
        defw 182*128+32/2-0x4000
        defb 17
        defw 182*128+42/2-0x4000
        defb 4
        defw 182*128+180/2-0x4000
        defb 16
        defw 182*128+200/2-0x4000
        defb 17
        defw 182*128+210/2-0x4000
        defb 3
        defw 0

; ------------------------------------------------------------ sub-rotinas
mulq:   ld hl,0                         ; HL = A * E  (D=0), A destruído
        ld b,8
mq1:    add hl,hl
        rlca
        jr nc,mq2
        add hl,de
mq2:    djnz mq1
        ret

vdp_reg:out (CMDP),a
        ld a,b
        or 0x80
        out (CMDP),a
        ret

waitce: in a,(CMDP)
        rra
        jr c,waitce
        ret

send:   call waitce
        ld a,0x20
        out (CMDP),a
        ld a,0x91
        out (CMDP),a
        ld hl,cb_sx
        ld bc,0x0F9B
        otir
        ret

fill:   ld hl,0
        ld (cb_sx),hl
        ld (cb_sy),hl
        ld hl,(fx)
        ld (cb_dx),hl
        ld hl,(fy)
        ld (cb_dy),hl
        ld hl,(fw)
        ld (cb_nx),hl
        ld hl,(fh)
        ld (cb_ny),hl
        ld a,(fc)
        ld (cb_clr),a
        xor a
        ld (cb_arg),a
        ld a,(fcmd)
        ld (cb_cmd),a
        jp send

        include "gamedata.inc"          ; tabelas, paletas, arte e sprites (sintéticos / originais)
        include "track.inc"
