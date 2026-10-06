; Cartucho ROM 16 KiB em 0x4000 ("AB"), sem mapper. Dados em RAM a partir de 0xC000.
 .module crt0
 .globl _main
 .area _HEADER (ABS)
 .org 0x4000
 .db 0x41,0x42
 .dw init
 .dw 0,0,0
 .db 0,0,0,0,0,0
 .area _CODE
init:
 di
 ld sp,#0xF300
 ; turbo R? (MSXVER em 0x002D >= 3) -> modo R800 ROM
 ld a,(0x002D)
 cp #3
 jr c,noturbo
 ld a,#0x81
 call 0x0180
noturbo:
 di
 ld hl,#0xC000
 ld de,#0xC001
 ld bc,#0x0FFF
 ld (hl),#0
 ldir
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
