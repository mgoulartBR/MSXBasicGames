; Pista pseudo-3D para MSX2+ (V9958), SCREEN 5 - versão v3: curvas (R#26/R#27) + RELEVO (R#23).
; - Textura da pista desenhada UMA vez; nenhum HMMM por quadro.
; - Listras animadas por ciclo de paleta (4 fases).
; - Curva por zonas: interrupção de linha (IM2) escreve R#26/R#27 em cada zona.
; - Relevo: R#23 (scroll vertical) por zona = linhas diferentes da textura (dv = -16..+16).
; Dados 100% sintéticos (hills.inc). Montar com pasmo.
        include "hconfig.inc"           ; SPEED, PALDIR, FORCECURVE, FORCEHILL, ILV, HILLPH

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
hillabs equ 0xC024
hillsgn equ 0xC025
zready  equ 0xC030
zwr     equ 0xC032
zptr    equ 0xC034
palptr  equ 0xC036
tr      equ 0xC038
hwv     equ 0xC039
sbv     equ 0xC03A
vbc     equ 0xC100
idlecnt equ 0xC102
zbuf0   equ 0xC200
zbuf1   equ 0xC260
IM2TAB  equ 0xE000
IM2STUB equ 0xE1E1

        org 0x4000
        defb "AB"
        defw init
        defw 0,0,0
        defs 6

init:   di
        ld sp,0xF300
        ld a,5
        call 0x005F                     ; CHGMOD: SCREEN 5
        di
        ld a,0x0A                       ; sprites off
        ld b,8
        call vdp_reg
        ld a,0x80                       ; 212 linhas
        ld b,9
        call vdp_reg
        ld a,0xF3                       ; cor de fundo = grama (a máscara do scroll fino usa R#7)
        ld b,7
        call vdp_reg
        xor a
        ld b,25                         ; V9958: sem máscara/2 páginas
        call vdp_reg
        xor a
        ld b,26
        call vdp_reg
        xor a
        ld b,27
        call vdp_reg
        ld a,0x1F                       ; exibe página 0
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
        ld hl,0
        ld (pos),hl
        ld (vbc),hl
        ld (idlecnt),hl
        ld hl,zbuf0
        ld (zwr),hl
        ld a,2
        ld b,15
        call vdp_reg                    ; R#15=2: lê S#2 (CE) durante o desenho

        ; --- céu e grama (HMMV, 2 pixels por byte)
        ld a,0xC0
        ld (fcmd),a
        ld hl,0
        ld (fx),hl
        ld hl,256
        ld (fw),hl
        ld hl,0
        ld (fy),hl
        ld hl,70
        ld (fh),hl
        ld a,0x11
        ld (fc),a
        call fill
        ld hl,70
        ld (fy),hl
        ld hl,30
        ld (fh),hl
        ld a,0x22
        ld (fc),a
        call fill
        ld hl,100
        ld (fy),hl
        ld hl,156
        ld (fh),hl
        ld a,0x33
        ld (fc),a
        call fill

        ; --- linhas da pista (LMMV 1 linha): zebra, asfalto, faixa central
        ld a,0x80
        ld (fcmd),a
        ld hl,1
        ld (fh),hl
        xor a
        ld (tr),a
trl:    ld a,(tr)
        add a,100
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
        ld a,(sbv)
        add a,12
        ld (fc),a
        call fill
        ld a,(tr)
        inc a
        ld (tr),a
        cp TEXROWS
        jp nz,trl
        call waitce

        ld a,1
        ld b,15
        call vdp_reg                    ; R#15=1: ISR lê S#1 (flag de linha)
        call frame                      ; pré-calcula o 1º buffer de zonas
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
        jp main

; ------------------------------------------------------------ cálculo por quadro
frame:  ld hl,(pos)
        ld de,SPEED
        add hl,de
        ld (pos),hl
        ld a,h
        rrca
        rrca
        and 63
        ld e,a
        ld d,0
        ld hl,curvetab
        add hl,de
        ld a,(hl)
        IF FORCECURVE
        ld a,FORCECURVE
        ENDIF
        ld (curvesgn),a
        or a
        jp p,pc1
        neg
pc1:    ld (curveabs),a
        ; fase da paleta = (pos>>8) & 3  ->  palptr = paltab + 32*fase
        ld a,(pos+1)
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
        ld (palptr),hl
        ; relevo: índice = ((pos>>10) + HILLPH) & 63
        ld a,(pos+1)
        rrca
        rrca
        and 63
        add a,HILLPH
        and 63
        ld e,a
        ld d,0
        ld hl,hilltab
        add hl,de
        ld a,(hl)
        IF FORCEHILL
        ld a,FORCEHILL
        ENDIF
        ld (hillsgn),a
        or a
        jp p,ph1
        neg
ph1:    ld (hillabs),a
        ; zonas: R#26, R#27, R#23 e linha da próxima interrupção (4 bytes por zona)
        ld ix,(zwr)
        ld hl,zt2
        ld b,ZN
zl:     ld e,(hl)                       ; q (0..255)
        inc hl
        ld a,(hl)
        inc hl
        ld (ix+3),a                     ; linha-base da próxima interrupção
        push hl
        push bc
        ld d,0
        ld a,(curveabs)
        call mulq
        ld a,h
        srl a                           ; mag = |curva|*q >> 9  (0..63)
        ld c,a
        ld a,(curvesgn)
        or a
        ld a,c
        jp p,spos
        neg                             ; s = -mag
spos:   ld b,a                          ; s (deslocamento à direita, com sinal)
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
        ld a,(hillabs)
        call mulq
        ld a,h                          ; dv = |relevo|*q >> 8  (0..16)
        ld c,a
        ld a,(hillsgn)
        or a
        ld a,c
        jp p,hpos
        neg
hpos:   add a,16                        ; R#23 = 16 + dv
        ld (ix+2),a
        IF ILV
        ld c,a
        ld a,(ix+3)
        or a
        jr z,nol
        add a,c                         ; R#19 compara com (linha + R#23)
        ld (ix+3),a
nol:
        ENDIF
        inc ix
        inc ix
        inc ix
        inc ix
        pop bc
        pop hl
        djnz zl
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
        jr nc,isr_vb
        ; --- início de zona: escreve R#26, R#27 e R#23; agenda a próxima
        ld hl,(zptr)
        ld a,(hl)
        inc hl
        ld b,(hl)
        inc hl
        ld c,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld (zptr),hl
        out (CMDP),a
        ld a,26+0x80
        out (CMDP),a
        ld a,b
        out (CMDP),a
        ld a,27+0x80
        out (CMDP),a
        ld a,c
        out (CMDP),a
        ld a,23+0x80
        out (CMDP),a
        ld a,d
        or a
        jr z,isr_last
        out (CMDP),a
        ld a,19+0x80
        out (CMDP),a
        jr isr_end
isr_last:
        ld a,0x06                       ; fim da pista: desliga IE1
        out (CMDP),a
        ld a,0x80
        out (CMDP),a
        jr isr_end
isr_vb: ld a,0                          ; --- VBlank: reconhece S#0
        out (CMDP),a
        ld a,15+0x80
        out (CMDP),a
        in a,(CMDP)
        ld a,1
        out (CMDP),a
        ld a,15+0x80
        out (CMDP),a
        ld a,4                          ; paleta: entradas 4..15 (fase atual)
        out (CMDP),a
        ld a,16+0x80
        out (CMDP),a
        ld hl,(palptr)
        ld bc,0x189A
        otir
        xor a                           ; scroll zero (céu)
        out (CMDP),a
        ld a,26+0x80
        out (CMDP),a
        xor a
        out (CMDP),a
        ld a,27+0x80
        out (CMDP),a
        xor a
        out (CMDP),a
        ld a,23+0x80                    ; sem deslocamento vertical no céu
        out (CMDP),a
        ld a,IL0
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

        include "hills.inc"
        defs 0x8000-$
