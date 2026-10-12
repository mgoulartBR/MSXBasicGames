; Microbenchmark: taxa de cópia VRAM->VRAM do V9958 em SCREEN 5 (HMMM 256xNY).
; Parâmetros por config: NY (linhas) e NCMD (comandos por quadro).
        include "speedcfg.inc"
        org 0x4000
        defb "AB"
        defw init
        defw 0,0,0
        defs 6
init:   di
        ld sp,0xF300
        ld a,5
        call 0x005F
        di
        ld a,0x0A
        ld b,8
        call vdp_reg
        ld a,0x80
        ld b,9
        call vdp_reg
        ld a,2
        ld b,15
        call vdp_reg
        ld hl,0
        ld (0xC100),hl
loop:   ld d,NCMD
cmd:    call waitce
        ld a,0x20
        out (0x99),a
        ld a,0x91
        out (0x99),a
        ld hl,buf
        ld bc,0x0F9B
        otir
        dec d
        jr nz,cmd
        call waitce
        ld hl,(0xC100)
        inc hl
        ld (0xC100),hl
        jp loop
waitce: in a,(0x99)
        rra
        jr c,waitce
        ret
vdp_reg:out (0x99),a
        ld a,b
        or 0x80
        out (0x99),a
        ret
buf:    defw 0,768, 0,0, 256, NY
        defb CLRV,0,CMDB
        defs 0x8000-$
