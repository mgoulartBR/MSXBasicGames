; ROM ASCII16. Banco 0 (0x4000): cabecalho "AB" + boot. Banco 1 (janela 0x8000-0xBFFF): codigo.
; Bancos 2..: dados, lidos pela janela 0x4000 (registrador do mapper em 0x6000).
 .module crt0
 .globl _main
 .globl init
 .globl l__INITIALIZER
 .globl s__INITIALIZER
 .globl s__INITIALIZED
 .area _HEADER (ABS)
 .org 0x4000
 .db 0x41,0x42
 .dw boot
 .dw 0,0,0
 .db 0,0,0,0,0,0
 .ascii "ROM_AS16"          ; assinatura de tipo de ROM (MSXgl) em 0x4010: o openMSX identifica o mapper ASCII16 sozinho
 .ascii "ASCII16 MegaROM, mapper ASCII16 (16 KB banks, registers 6000h and 7000h)"
 .db 0
 ; etiqueta de mapper (como geo3d/tools/mapper_tag_ascii16.asm): nunca executada, o INIT pula por cima
 jr boot
 ld (0x6000),a
 ld (0x6000),a
 ld (0x6000),a
 ld (0x6000),a
 ld (0x7000),a
 ld (0x7000),a
 ld (0x7000),a
 ld (0x7000),a
 ld (0x77FF),a
 ld (0x77FF),a
 ld (0x77FF),a
 ld (0x77FF),a
boot:
 di
 ld sp,#0xF300
 ld a,#1
 ld (0x7000),a          ; ASCII16: janela 0x8000 = banco 1
 call 0x0138            ; RSLREG
 rrca
 rrca
 and #3                 ; slot primario da pagina 1 (este cartucho)
 ld c,a
 ld b,#0
 ld hl,#0xFCC1          ; EXPTBL
 add hl,bc
 ld a,(hl)
 and #0x80
 or c
 ld c,a
 inc hl
 inc hl
 inc hl
 inc hl                 ; SLTTBL
 ld a,(hl)
 and #12                ; slot secundario da pagina 1
 or c
 ld h,#0x80
 call 0x0024            ; ENASLT: pagina 2 = slot do cartucho
 di
 jp init
 .area _CODE
init:
 di
 ld sp,#0xF300
 in a,(0xFF)            ; copiado de todas as demos kanon-ai (V9968+Geo3D); finalidade nao documentada
 xor #1
 out (0xFE),a
 ld a,(0x002D)          ; MSXVER >= 3: turbo R -> modo R800 ROM
 cp #3
 jr c,noturbo
 ld a,#0x81
 call 0x0180            ; CHGCPU
noturbo:
 di
 ld hl,#0xC000
 ld de,#0xC001
 ld bc,#0x1FFF
 ld (hl),#0
 ldir
 ld bc,#l__INITIALIZER   ; copia os valores iniciais das globais inicializadas (ROM -> RAM)
 ld a,b
 or c
 jr z,noinit
 ld de,#s__INITIALIZED
 ld hl,#s__INITIALIZER
 ldir
noinit:
 call _main
halt_loop:
 jr halt_loop
 .area _HOME
 .area _INITIALIZER
 .area _GSINIT
 .area _GSFINAL
 .area _DATA
 .area _INITIALIZED
 .area _BSEG
 .area _BSS
 .area _HEAP
