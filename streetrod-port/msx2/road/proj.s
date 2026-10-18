	.module proj
	.include "config.inc"
	.include "layout.inc"
	.globl _pos, _sub, _P, _xoff_ahead
	.globl _XC, _Y, _PAR
	.globl _rc8_tab, _ytab, _curve_tab, _hill_tab
	.globl _g_a, _g_b8, _g_r, _mul16x8

	.area _DATA
p_dx:	.ds 2
p_xoff:	.ds 2
p_hc:	.ds 2
p_rc:	.ds 1
p_xc:	.ds 2
p_y:	.ds 2
p_hw:	.ds 2
p_r:	.ds 1
p_z:	.ds 1
p_s:	.ds 1

	.area _CODE

; HL = min(HL, DE) (com sinal)
smin:	push hl
	or a,a
	sbc hl,de
	pop hl
	ret m
	ex de,hl
	ret

; HL = max(HL, DE) (com sinal)
smax:	push hl
	or a,a
	sbc hl,de
	pop hl
	ret p
	ex de,hl
	ret

clampx:	ld de,#-500
	call smax
	ld de,#700
	jp smin

; DE = word_tab[z]  (HL = base da tabela)
tabw:	ld a,(p_z)
	add a,a
	ld e,a
	ld d,#0
	add hl,de
	ld e,(hl)
	inc hl
	ld d,(hl)
	ret

; HL = base de array de words + 2*r
arr_r:	ld a,(p_r)
	add a,a
	ld e,a
	ld d,#0
	add hl,de
	ret

_project_asm::
	ld a,(_pos)
	ld c,a
	ld b,#0
	ld hl,#_hill_tab
	add hl,bc
	ld a,(hl)
	ld l,a
	rla
	sbc a,a
	ld h,a
	ld (p_hc),hl             ; hillcam
	ld hl,#0
	ld (p_dx),hl
	ld (p_xoff),hl
	xor a,a
	ld (p_r),a
	ld a,(_sub)
	ld b,a
	ld a,#12
	sub a,b
	ld (p_z),a
pr_row:
	ld a,(p_z)
	ld e,a
	ld d,#0
	ld hl,#_rc8_tab
	add hl,de
	ld a,(hl)
	ld (p_rc),a
	ld a,(_pos)
	ld hl,#p_r
	add a,(hl)
	ld (p_s),a
	; dx += curve[s]; xoff += dx
	ld c,a
	ld b,#0
	ld hl,#_curve_tab
	add hl,bc
	ld a,(hl)
	ld c,a
	rla
	sbc a,a
	ld b,a
	ld hl,(p_dx)
	add hl,bc
	ld (p_dx),hl
	ld bc,(p_xoff)
	add hl,bc
	ld (p_xoff),hl
	ld a,(p_r)
	cp a,#4
	jr nz,1$
	ld (_xoff_ahead),hl
1$:	; xc = CX + ((xoff - P) * rc) >> 12
	ld de,(_P)
	or a,a
	sbc hl,de
	ld (_g_a),hl
	ld a,(p_rc)
	ld (_g_b8),a
	call _mul16x8
	ld hl,(_g_r)
	ld de,#CX
	add hl,de
	call clampx
	ld (p_xc),hl
	; y = HZ + ytab[z] + ((hillcam - hill[s]) * rc) >> 12
	ld a,(p_s)
	ld c,a
	ld b,#0
	ld hl,#_hill_tab
	add hl,bc
	ld a,(hl)
	ld c,a
	rla
	sbc a,a
	ld b,a
	ld hl,(p_hc)
	or a,a
	sbc hl,bc
	ld (_g_a),hl
	ld a,(p_rc)
	ld (_g_b8),a
	call _mul16x8
	ld a,(p_z)
	ld c,a
	ld b,#0
	ld hl,#_ytab
	add hl,bc
	ld c,(hl)
	ld b,#0
	ld hl,(_g_r)
	add hl,bc
	ld bc,#HZ
	add hl,bc
	ld de,#250
	call smin
	ld (p_y),hl
	ld a,(p_r)
	or a,a
	jr z,2$
	dec a                    ; oclusao: y <= Y[r-1]
	add a,a
	ld e,a
	ld d,#0
	ld hl,#_Y
	add hl,de
	ld e,(hl)
	inc hl
	ld d,(hl)
	ld hl,(p_y)
	call smin
	ld (p_y),hl
2$:	ld hl,#_Y
	call arr_r
	ld de,(p_y)
	ld (hl),e
	inc hl
	ld (hl),d
	ld hl,#_XC
	call arr_r
	ld de,(p_xc)
	ld (hl),e
	inc hl
	ld (hl),d
	ld a,(p_s)
	rrca
	and a,#1
	ld c,a
	ld a,(p_r)
	ld e,a
	ld d,#0
	ld hl,#_PAR
	add hl,de
	ld (hl),c
	; proxima linha
	ld a,(p_z)
	add a,#8
	ld (p_z),a
	ld a,(p_r)
	inc a
	ld (p_r),a
	cp a,#NROWS
	jp nz,pr_row
	ret
