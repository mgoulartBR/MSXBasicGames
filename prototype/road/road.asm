; Protótipo de pista pseudo-3D para MSX2+ (V9958), SCREEN 5, ROM 16 KB em 0x4000.
; Técnica: textura da pista em VRAM (página 3); por quadro, 56 bandas de 2 linhas
; copiadas com HMMM para a página oculta, com deslocamento X da curva.
; Dados 100% sintéticos (tables.inc). Montar com pasmo.
        include "config.inc"            ; define VSYNC (0 ou 1) e SPEED

CMDP    equ 0x99
PALP    equ 0x9A
INDP    equ 0x9B

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
kv      equ 0xC020
curve   equ 0xC021
mag     equ 0xC022
hid     equ 0xC023
tv      equ 0xC024
tr      equ 0xC025
hwv     equ 0xC026
bsy     equ 0xC028
bdy     equ 0xC02A
pos     equ 0xC02C
counter equ 0xC100

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
        IF DISPOFF
        ld a,0x20                       ; tela desligada (só para benchmark)
        ld b,1
        call vdp_reg
        ENDIF
        ld a,0x80                       ; 212 linhas
        ld b,9
        call vdp_reg
        ld a,0x1F                       ; mostra página 0
        ld b,2
        call vdp_reg
        xor a
        ld b,16                         ; paleta a partir da cor 0
        call vdp_reg
        ld hl,palette
        ld bc,0x209A
        otir

        xor a
        ld (counter),a
        ld (counter+1),a
        ld (pos),a
        ld (pos+1),a
        ld a,1
        ld (hid),a                      ; renderiza na página 1; exibe a 0
        ld a,2
        ld b,15
        call vdp_reg                    ; R#15=2 -> lê S#2 (CE)

        ; céu estático nas páginas 0 e 1
        ld hl,1
        ld (fh),hl
        ld hl,0
        ld (fx),hl
        ld hl,256
        ld (fw),hl
        ld hl,skytab
sky:    ld a,(hl)
        cp 0xFF
        jr z,skydone
        ld e,a                          ; y
        inc hl
        ld a,(hl)                       ; altura
        inc hl
        ld b,(hl)                       ; cor
        inc hl
        push hl
        ld d,a
        ld a,b
        ld (fc),a
        ld l,e
        ld h,0
        ld (fy),hl
        ld a,d
        ld (fh),a
        xor a
        ld (fh+1),a
        call fill                       ; página 0
        ld a,(fy+1)
        or 1
        ld (fy+1),a
        call fill                       ; página 1 (y+256)
        pop hl
        jr sky
skydone:

        ; textura: 2 variantes x 112 linhas na página 3 (y=768...)
        ld hl,1
        ld (fh),hl
        xor a
        ld (tv),a
tvl:    xor a
        ld (tr),a
trl:    ld a,(tv)
        or a
        ld hl,768
        jr z,tv0
        ld de,112
        add hl,de
tv0:    ld a,(tr)
        ld e,a
        ld d,0
        add hl,de
        ld (fy),hl
        ld hl,hwtab
        add hl,de
        ld a,(hl)
        ld (hwv),a
        ; grama
        ld hl,0
        ld (fx),hl
        ld hl,256
        ld (fw),hl
        ld a,(tv)
        add a,2
        ld (fc),a
        call fill
        ; zebra (rumble)
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
        ld a,(tv)
        add a,6
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
        ld a,(tv)
        add a,4
        ld (fc),a
        call fill
        ; faixa central (só variante 0)
        ld a,(tv)
        or a
        jr nz,noc
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
noc:    ld a,(tr)
        inc a
        ld (tr),a
        cp 112
        jp nz,trl
        ld a,(tv)
        inc a
        ld (tv),a
        cp 2
        jp nz,tvl

; ------------------------------------------------------------ laço principal
render: ld a,(pos+1)
        rrca
        rrca
        and 63
        ld e,a
        ld d,0
        ld hl,curvetab
        add hl,de
        ld a,(hl)
        ld (curve),a
        xor a
        ld (kv),a
band:   ld a,(kv)
        add a,a
        ld e,a
        ld d,0                          ; de = 2k
        ld hl,zt
        add hl,de
        ld c,(hl)
        inc hl
        ld b,(hl)
        ld hl,(pos)
        add hl,bc
        ld a,h
        and 1
        ld hl,768
        jr z,v0
        ld bc,112
        add hl,bc
v0:     add hl,de
        ld (bsy),hl
        ld a,(kv)
        add a,a
        add a,100
        ld l,a
        ld a,(hid)
        ld h,a
        ld (bdy),hl
        ld a,(kv)
        ld e,a
        ld d,0
        ld hl,qt
        add hl,de
        ld e,(hl)
        ld a,(curve)
        ld c,a
        or a
        jp p,mpos
        neg
mpos:   ld hl,0
        ld b,8
mul:    add hl,hl
        rlca
        jr nc,nm
        add hl,de
nm:     djnz mul
        ld a,h
        srl a
        and 0xFE
        ld (mag),a
        ld hl,(bsy)
        ld (cb_sy),hl
        ld hl,(bdy)
        ld (cb_dy),hl
        ld hl,2
        ld (cb_ny),hl
        xor a
        ld (cb_clr),a
        ld (cb_arg),a
        ld a,0xD0                       ; HMMM
        ld (cb_cmd),a
        ld a,(mag)
        or a
        jr nz,shifted
        ld hl,0
        ld (cb_sx),hl
        ld (cb_dx),hl
        ld hl,256
        ld (cb_nx),hl
        call send
        jr next
shifted:ld e,a
        ld d,0                          ; de = m
        ld a,(curve)
        or a
        jp m,negc
        ld hl,0
        ld (cb_sx),hl
        ld (cb_dx),de
        ld hl,256
        or a
        sbc hl,de
        ld (cb_nx),hl
        call send
        ld hl,0
        ld (cb_dx),hl
        ld (cb_nx),de
        call send
        jr next
negc:   ld (cb_sx),de
        ld hl,0
        ld (cb_dx),hl
        ld hl,256
        or a
        sbc hl,de
        ld (cb_nx),hl
        call send
        ld hl,256
        or a
        sbc hl,de
        ld (cb_sx),hl
        ld (cb_dx),hl
        ld (cb_nx),de
        call send
next:   ld a,(kv)
        inc a
        ld (kv),a
        cp 56
        jp nz,band

        call waitce
        xor a
        ld b,15
        call vdp_reg                    ; S#0
        IF VSYNC
vs:     in a,(CMDP)
        rlca
        jr nc,vs
        ENDIF
        ld a,(hid)
        or a
        ld a,0x1F
        jr z,fl
        ld a,0x3F
fl:     ld b,2
        call vdp_reg                    ; troca a página exibida
        ld a,(hid)
        xor 1
        ld (hid),a
        ld a,2
        ld b,15
        call vdp_reg
        ld hl,(counter)
        inc hl
        ld (counter),hl
        ld hl,(pos)
        ld de,SPEED
        add hl,de
        ld (pos),hl
        jp render

; ------------------------------------------------------------ sub-rotinas
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
        ld a,0x80                       ; LMMV
        ld (cb_cmd),a
        jp send

skytab: defb 0,60,1, 60,25,9, 85,15,10, 0xFF

        include "tables.inc"
        defs 0x8000-$
