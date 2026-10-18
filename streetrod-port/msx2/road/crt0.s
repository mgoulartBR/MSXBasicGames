	.module crt0
	.globl _main
	.globl s__INITIALIZER
	.globl s__INITIALIZED
	.globl l__INITIALIZER
	.globl s__DATA
	.globl l__DATA

	.area _HEADER (ABS)
	.org 0x4000
	.db 0x41
	.db 0x42
	.dw init
	.dw 0
	.dw 0
	.dw 0
	.dw 0
	.dw 0
	.dw 0

	.area _HOME
	.area _CODE
	.area _INITIALIZER
	.area _GSINIT
	.area _GSFINAL
	.area _DATA
	.area _INITIALIZED
	.area _BSEG
	.area _BSS
	.area _HEAP

	.area _CODE
init:
	ld sp,(0xFC4A)
	call map_page2           ; ROM de 32 KB: os assets ficam em 0x8000
	call gsinit
	call _main
1$:	jr 1$

	.area _GSINIT
gsinit:
	ld hl,#s__DATA
	ld (hl),#0
	ld de,#s__DATA+1
	ld bc,#l__DATA-1
	ldir
	ld bc,#l__INITIALIZER
	ld a,b
	or a,c
	jr z,gsinit_next
	ld de,#s__INITIALIZED
	ld hl,#s__INITIALIZER
	ldir
gsinit_next:
	.area _GSFINAL
	ret

	.area _CODE
; mapeia o slot do cartucho (o mesmo da pagina 1) tambem na pagina 2
map_page2:
	call 0x0138              ; RSLREG
	rrca
	rrca
	and a,#3
	ld c,a
	ld hl,#0xFCC1            ; EXPTBL
	add a,l
	ld l,a
	ld a,(hl)
	and a,#0x80
	or a,c
	ld c,a
	inc l
	inc l
	inc l
	inc l                    ; SLTTBL
	ld a,(hl)
	and a,#0x0C
	or a,c
	ld h,#0x80
	call 0x0024              ; ENASLT
	ei
	ret
