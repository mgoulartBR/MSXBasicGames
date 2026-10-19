	.module crt0
	.globl _main
	.globl _cur_bank
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
	ld hl,(0xFC4A)           ; pilha logo abaixo de HIMEM, mas nunca acima de 0xF300: 0xF37D e o gancho do BDOS do Disk BASIC (sem disco, HIMEM = 0xF380 e a pilha cairia em cima dele)
	ld a,h
	cp a,#0xF3
	jr c,sp_ok
	ld hl,#0xF300
sp_ok:	ld sp,hl
	call map_page2           ; mapeia o slot do cartucho tambem na pagina 2
	xor a,a
	ld (0x6000),a            ; ASCII16: banco 0 em 0x4000-0x7FFF (codigo fixo)
	inc a
	ld (0x7000),a            ; banco 1 em 0x8000-0xBFFF (resto do codigo); bancos 2+ = assets, trocados so no carregador
	call gsinit
	ld a,#1
	ld (_cur_bank),a         ; banco de codigo corrente em 0x8000 (gsinit zerou a RAM)
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
