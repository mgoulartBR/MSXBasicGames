	.module vdpasm
	.include "config.inc"
	.globl _g_lzsrc
	.globl _g_vlo
	.globl _g_vhi
	.globl _g_vpg
	.globl _lz_unpack
	.globl _load_assets
	.globl _bank_copy
	.globl _g_bk_bank
	.globl _g_bk_src
	.globl _g_bk_dst
	.globl _g_bk_len
	.globl _g_group
	.globl _g_palp
	.globl _pal_load2
	.globl _psg_w
	.globl _pal_one
	.globl _vc
	.globl _vdp_cmd15
	.globl _vram_blit
	.globl _ev_space
	.globl _ev_shift
	.globl _install_hook
	.globl _cmd_buf
	.globl _cmd_n
	.globl _g_reg
	.globl _g_val
	.globl _g_stick
	.globl _g_trig
	.globl _g_st0
	.globl _g_st1
	.globl _g_t0
	.globl _g_t1
	.globl _g_t3
	.globl _g_row6
	.globl _g_row7
	.globl _palette
	.globl _g_a
	.globl _g_b
	.globl _g_sh
	.globl _g_sgn
	.globl _g_r
	.globl _g_fx
	.globl _g_fy
	.globl _g_fw
	.globl _g_fh
	.globl _g_fc
	.globl _g_ybase
	.globl _cp
	.globl _rec
	.globl _g_dry
	.globl _g_b8
	.globl _hwy_tab
	.globl _kwy_tab
	.globl _clip_l
	.globl _clip_r
	.globl _t_w
	.globl _t_xr
	.globl _t_xl
	.globl _t_y
	.globl _s_mode
	.globl _s_gc
	.globl _s_kc
	.globl _s_rc
	.globl _s_kw
	.globl _s_sr2
	.globl _s_sl2
	.globl _s_xrf
	.globl _s_xlf
	.globl _s_rem
	.globl _s_yt

	.area _CODE

; Executa _cmd_n comandos de 11 bytes (R#36..R#46) de _cmd_buf, esperando CE=0 entre eles.
; Interrupcoes so ficam desabilitadas durante pares de escritas no VDP.
_exec_cmds::
	ld hl,#_cmd_buf
	ld bc,(_cmd_n)
	ld a,b
	or a,c
	ret z
ex_next:
	call ex_wait
	di
	ld a,#36
	out (#0x99),a
	ld a,#0x91
	out (#0x99),a
	push bc
	ld bc,#0x0B9B
	otir
	pop bc
	ei
	dec bc
	ld a,b
	or a,c
	jr nz,ex_next
	call ex_wait
	ret

ex_wait:
	di
	ld a,#2
	out (#0x99),a
	ld a,#0x8F
	out (#0x99),a
	in a,(#0x99)
	ld d,a
	xor a,a
	out (#0x99),a
	ld a,#0x8F
	out (#0x99),a
	ei
	bit 0,d
	jr nz,ex_wait
	ret

_vdp_reg_g::
	di
	ld a,(_g_val)
	out (#0x99),a
	ld a,(_g_reg)
	or a,#0x80
	out (#0x99),a
	ei
	ret

_pal_load::
	di
	xor a,a
	out (#0x99),a
	ld a,#0x90
	out (#0x99),a
	ld hl,#_palette
	ld bc,#0x209A
	otir
	ei
	ret

_set_screen5::
	push ix
	push iy
	ld a,#5
	call 0x005F
	pop iy
	pop ix
	ret

; _read_input: le teclado/joystick pela BIOS. Resultados vao para a memoria (a BIOS pode destruir qualquer registrador, inclusive IX/IY).
; g_st0/g_st1 = GTSTCK cursores/joystick 1 (0..8); g_t0/g_t1/g_t3 = GTTRIG espaco/joy A/joy B; g_row6 = SNSMAT linha 6 (bit0 = SHIFT, 0 = apertado)
_read_stick::
_read_input::
	push ix
	push iy
	xor a,a
	call 0x00D5
	ld (_g_st0),a
	ld a,#1
	call 0x00D5
	ld (_g_st1),a
	xor a,a
	call 0x00D8
	ld (_g_t0),a
	ld a,#1
	call 0x00D8
	ld (_g_t1),a
	ld a,#3
	call 0x00D8
	ld (_g_t3),a
	ld a,#6
	call 0x0141
	ld (_g_row6),a
	ld a,#7
	call 0x0141
	ld (_g_row7),a          ; linha 7 (F4 F5 ESC TAB STOP BS SELECT RET): bit2 = ESC, bit4 = STOP, bit5 = BS (0 = apertado)
	pop iy
	pop ix
	ret

; ---------------------------------------------------------------------
; _mul12: g_r = ((i16)g_a * (u16)g_b) >> 12
_mul12::
	ld bc,(_g_a)
	ld de,(_g_b)
	xor a,a
	bit 7,b
	jr z,1$
	ld a,#1
	push af
	xor a,a                  ; BC = -BC
	sub a,c
	ld c,a
	sbc a,a
	sub a,b
	ld b,a
	jr 2$
1$:	push af
2$:	ld hl,#0
	ld a,#16
3$:	or a,a
	bit 0,c
	jr z,4$
	add hl,de
4$:	rr h
	rr l
	rr b
	rr c
	dec a
	jr nz,3$
	add hl,hl                ; resultado = (HL<<4) | (B>>4)
	add hl,hl
	add hl,hl
	add hl,hl
	ld a,b
	rrca
	rrca
	rrca
	rrca
	and a,#0x0F
	or a,l
	ld l,a
	pop af
	or a,a
	jr z,5$
	xor a,a
	sub a,l
	ld l,a
	sbc a,a
	sub a,h
	ld h,a
5$:	ld (_g_r),hl
	ret

; _fill_g: entrada para C. Usa g_fx,g_fy,g_fw,g_fh,g_fc, g_ybase.
_fill_g::
	ld hl,(_g_fh)
	bit 7,h
	ret nz
	ld a,h
	or a,l
	ret z
	ld (_rec+6),hl
	ld hl,(_g_fy)
	ld bc,(_g_ybase)
	add hl,bc
	ld (_rec+2),hl
	ld de,(_g_fx)
	ld hl,(_g_fw)
	ld a,(_g_fc)
	; continua em fill_regs

; fill_regs: DE = x, HL = largura (i16), A = cor; _rec+2 (y+ybase) e _rec+6 (h) ja preenchidos.
; Clipping horizontal [0,256), pixels pares, emite HMMV (espera o anterior terminar).
fill_regs:
	ld c,a
	bit 7,h
	ret nz
	ld a,h
	or a,l
	ret z
	add hl,de                ; HL = x2
	bit 7,d
	jr z,1$
	ld de,#0
1$:	push hl                  ; --- clip esquerdo/direito: x em [clip_l, clip_r)
	ld hl,(_clip_l)
	or a,a
	sbc hl,de                ; clip_l - x
	pop hl
	jr c,9$                  ; x > clip_l -> ok
	jr z,9$
	ld de,(_clip_l)          ; x = clip_l
9$:	push de
	ld de,(_clip_r)
	or a,a
	sbc hl,de                ; x2 - clip_r
	jr c,10$                 ; x2 < clip_r -> ok
	ld hl,(_clip_r)
	jr 11$
10$:	add hl,de
11$:	pop de
	inc hl
	ld a,l
	and a,#0xFE
	ld l,a
	ld a,e
	and a,#0xFE
	ld e,a
	or a,a
	sbc hl,de
	jr z,6$
	bit 7,h
	jr nz,6$
	ld (_rec+0),de
	ld (_rec+4),hl
	ld a,c
	rlca
	rlca
	rlca
	rlca
	or a,c
	ld (_rec+8),a
	xor a,a
	ld (_rec+9),a
	ld a,#0xC0
	ld (_rec+10),a
	ld a,(_g_dry)
	or a,a
	jr nz,8$
	call ex_wait
	di
	ld a,#36
	out (#0x99),a
	ld a,#0x91
	out (#0x99),a
	ld hl,#_rec
	ld bc,#0x0B9B
	otir
	ei
8$:	ld hl,(_cmd_n)
	inc hl
	ld (_cmd_n),hl
6$:	ret

; _strip_run: percorre as scanlines de uma faixa gerando os retangulos (zebra+pista e opcionalmente gramado).
; Entradas: s_yt, s_rem (u8), s_xlf (= centro xc em 4 bits fracionarios), s_sl2 (incremento por grupo),
; s_rc, s_kc, s_gc (cores), s_mode (bit0 = zebras separadas, bit1 = gramado por scanline).
; A largura da pista e da zebra vem de hwy_tab/kwy_tab indexadas pela scanline.
_strip_run::
	ld a,(_s_yt)
	ld (_t_y),a
grp:
	ld a,(_s_rem)
	or a,a
	ret z
	ld b,#STEP
	cp a,b
	jr nc,1$
	ld b,a
1$:	sub a,b
	ld (_s_rem),a
	ld a,b
	ld (_rec+6),a
	xor a,a
	ld (_rec+7),a
	ld a,(_t_y)
	ld l,a
	ld h,#0
	ld de,(_g_ybase)
	add hl,de
	ld (_rec+2),hl
	; hw = hwy_tab[y]; kw = kwy_tab[y]
	ld a,(_t_y)
	ld e,a
	ld d,#0
	ld hl,#_kwy_tab
	add hl,de
	ld c,(hl)
	ld b,#0
	ld (_s_kw),bc
	ld hl,#_hwy_tab
	add hl,de
	add hl,de
	ld e,(hl)
	inc hl
	ld d,(hl)                ; DE = hw
	ld a,(_t_y)              ; y += h
	ld hl,#_rec+6
	add a,(hl)
	ld (_t_y),a
	; xc = s_xlf >> 4
	ld hl,(_s_xlf)
	sra h
	rr l
	sra h
	rr l
	sra h
	rr l
	sra h
	rr l
	ld a,d
	or a,e
	jp z,noroad
	push hl                  ; xc
	or a,a
	sbc hl,de                ; xl = xc - hw
	ld (_t_xl),hl
	ex de,hl                 ; HL = hw, DE = xl
	add hl,hl                ; w = 2*hw
	ld (_t_w),hl
	add hl,de                ; xr = xl + w
	ld (_t_xr),hl
	pop hl
	ld a,(_s_mode)
	and a,#2
	jr z,2$
	ld hl,(_t_xl)            ; gramado esquerdo: x=0, w = xl - kw
	ld de,(_s_kw)
	or a,a
	sbc hl,de
	ld de,#0
	ld a,(_s_gc)
	call fill_regs
2$:	ld a,(_s_mode)
	and a,#1
	jr nz,4$
	ld hl,(_t_xl)            ; zebra unica sob a pista: x = xl-kw, w = w + 2kw
	ld de,(_s_kw)
	or a,a
	sbc hl,de
	ex de,hl                 ; DE = xl-kw, HL = kw
	add hl,hl
	ld bc,(_t_w)
	add hl,bc
	ld a,(_s_kc)
	call fill_regs
	jr 5$
4$:	ld hl,(_t_xl)            ; zebra esquerda
	ld de,(_s_kw)
	or a,a
	sbc hl,de
	ex de,hl
	ld hl,(_s_kw)
	ld a,(_s_kc)
	call fill_regs
	ld de,(_t_xr)            ; zebra direita
	ld hl,(_s_kw)
	ld a,(_s_kc)
	call fill_regs
5$:	ld de,(_t_xl)            ; pista
	ld hl,(_t_w)
	ld a,(_s_rc)
	call fill_regs
	ld a,(_s_mode)
	and a,#2
	jp z,next
	ld hl,(_t_xr)            ; gramado direito: x = xr+kw, w = 256 - x
	ld de,(_s_kw)
	add hl,de
	ex de,hl
	ld hl,#256
	or a,a
	sbc hl,de
	ld a,(_s_gc)
	call fill_regs
	jp next
noroad:
	ld a,(_s_mode)
	and a,#2
	jp z,next
	ld de,#0
	ld hl,#256
	ld a,(_s_gc)
	call fill_regs
next:
	ld hl,(_s_xlf)
	ld de,(_s_sl2)
	add hl,de
	ld (_s_xlf),hl
	jp grp

; _mul16x8: g_r = ((i16)g_a * (u8)g_b8) >> 9      (g_b8 = byte)
_mul16x8::
	ld bc,(_g_a)
	ld a,b
	rla
	sbc a,a
	ld (_g_sgn),a            ; 0xFF se negativo
	or a,a
	jr z,1$
	xor a,a
	sub a,c
	ld c,a
	sbc a,a
	sub a,b
	ld b,a
1$:	ld a,(_g_b8)
	ld hl,#0
	ld e,#8
2$:	or a,a
	bit 0,a
	jr z,3$
	add hl,bc
3$:	rr h
	rr l
	rra
	dec e
	jr nz,2$
	sra h                    ; >> 9 total: HL tem os 16 bits altos (>>8); falta 1
	rr l
	ld a,(_g_sgn)
	or a,a
	jr z,4$
	xor a,a
	sub a,l
	ld l,a
	sbc a,a
	sub a,h
	ld h,a
4$:	ld (_g_r),hl
	ret


; ---------------------------------------------------------------------
; _draw_road_asm: percorre as NROWS-1 faixas (da mais distante para a mais proxima).
	.area _DATA
d_r:	.ds 1
d_yf:	.ds 2
d_yn:	.ds 2
d_yt:	.ds 2
d_yb:	.ds 2
d_sl:	.ds 2
	.area _CODE
_draw_road_asm::
	ld a,#NROWS-1
	ld (d_r),a
dr_loop:
	ld a,(d_r)
	add a,a
	ld e,a
	ld d,#0
	ld hl,#_Y
	add hl,de
	ld e,(hl)
	inc hl
	ld d,(hl)                ; DE = Y[r]  (yf)
	ld (d_yf),de
	ld a,(d_r)
	dec a
	add a,a
	ld e,a
	ld d,#0
	ld hl,#_Y
	add hl,de
	ld e,(hl)
	inc hl
	ld d,(hl)                ; DE = Y[r-1] (yn)
	ld (d_yn),de
	; if yn <= yf: proxima
	ld hl,(d_yf)
	ex de,hl                 ; DE = yf, HL = yn
	or a,a
	sbc hl,de                ; yn - yf
	jp m,dr_next
	jp z,dr_next
	; dy = yn - yf (em HL) -> guardar em BC
	ld b,h
	ld c,l
	; yt = max(yf,0)
	ld hl,(d_yf)
	bit 7,h
	jr z,1$
	ld hl,#0
1$:	ld (d_yt),hl
	; yb = min(yn, VIEW_BOT)
	ld hl,(d_yn)
	ld de,#VIEW_BOT
	or a,a
	sbc hl,de
	bit 7,h
	jr z,9$
	add hl,de                ; yn < VIEW_BOT: mantem yn
	jr 2$
9$:	ld hl,#VIEW_BOT
2$:	ld (d_yb),hl
	ld de,(d_yt)
	or a,a
	sbc hl,de                ; yb - yt
	jp m,dr_next
	jp z,dr_next
	ld a,l
	ld (_s_rem),a
	ld a,(d_yt)
	ld (_s_yt),a
	; cores pela paridade de PAR[r-1]
	ld a,(d_r)
	dec a
	ld e,a
	ld d,#0
	ld hl,#_PAR
	add hl,de
	ld a,(hl)
	or a,a
	jr z,3$
	ld a,#5
	ld (_s_rc),a
	ld a,#6
	ld (_s_kc),a
	ld a,#3
	ld (_s_gc),a
	jr 4$
3$:	ld a,#4
	ld (_s_rc),a
	ld a,#7
	ld (_s_kc),a
	ld a,#2
	ld (_s_gc),a
4$:	ld a,#DRAW_MODE
	ld (_s_mode),a
	and a,#6                 ; bit1 = gramado por scanline, bit2 = sem gramado (fundo estatico)
	jr nz,5$
	; modo 0: gramado da faixa inteira em um unico retangulo
	ld hl,#0
	ld (_g_fx),hl
	ld hl,(d_yt)
	ld (_g_fy),hl
	ld hl,#256
	ld (_g_fw),hl
	ld hl,(d_yb)
	ld de,(d_yt)
	or a,a
	sbc hl,de
	ld (_g_fh),hl
	ld a,(_s_gc)
	ld (_g_fc),a
	push bc
	call _fill_g
	pop bc
5$:	; slope = ((XC[r-1]-XC[r]) * rdy_tab[dy]) >> 12  ; BC = dy
	ld h,b
	ld l,c
	add hl,hl
	ld de,#_rdy_tab
	add hl,de
	ld e,(hl)
	inc hl
	ld d,(hl)
	ld (_g_b),de
	ld a,(d_r)
	add a,a
	ld e,a
	ld d,#0
	ld hl,#_XC
	add hl,de
	ld e,(hl)
	inc hl
	ld d,(hl)                ; DE = XC[r]
	push de
	dec hl
	dec hl
	dec hl                   ; HL -> XC[r-1] (low)
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a                   ; HL = XC[r-1]
	pop de
	push de                  ; guarda XC[r]
	or a,a
	sbc hl,de                ; XC[r-1] - XC[r]
	ld (_g_a),hl
	ld a,#12
	ld (_g_sh),a
	call _mul12
	ld hl,(_g_r)
	ld (d_sl),hl
	pop hl                   ; HL = XC[r]
	add hl,hl
	add hl,hl
	add hl,hl
	add hl,hl                ; xc << 4
	; se yt != yf: xlf += sl * (yt - yf)   (yf < 0: laco de somas)
	ld de,(d_yf)
	bit 7,d
	jr z,7$
	ld bc,(d_sl)
6$:	add hl,bc
	inc de
	bit 7,d
	jr nz,6$
7$:	ld (_s_xlf),hl
	ld hl,(d_sl)
	ld b,#STEP_SH
8$:	add hl,hl
	djnz 8$
	ld (_s_sl2),hl
	call _strip_run
dr_next:
	ld a,(d_r)
	dec a
	ld (d_r),a
	jp nz,dr_loop
	ret

_vdp_wait::
	jp ex_wait

_cpu_bench::
	ld bc,#0
1$:	dec bc
	ld a,b
	or a,c
	jr nz,1$
	ret

; ---------------------------------------------------------------------
; Gancho H.TIMI (0xFD9F): le o teclado (linhas 8 e 6 da matriz) a cada interrupcao de vblank e trava as bordas de
; subida de ESPACO e SHIFT em _ev_space/_ev_shift (o jogo roda a ~10 fps e perderia toques curtos).
	.area _DATA
_ev_space::	.ds 1
_ev_shift::	.ds 1
_k_prev::	.ds 1
_old_timi::	.ds 5
	.area _CODE
_install_hook::
	di
	ld hl,#0xFD9F
	ld de,#_old_timi
	ld bc,#5
	ldir
	xor a,a
	ld (_ev_space),a
	ld (_ev_shift),a
	ld (_k_prev),a
	ld a,#0xC3
	ld (0xFD9F),a
	ld hl,#_key_isr
	ld (0xFDA0),hl
	ei
	ret

_key_isr:
	push af
	push bc
	in a,(#0xAA)
	ld c,a                   ; guarda porta C do PPI
	and a,#0xF0
	or a,#8
	out (#0xAA),a
	in a,(#0xA9)             ; linha 8: bit0 = espaco (0 = apertado)
	cpl
	and a,#1
	ld b,a                   ; b bit0 = espaco agora
	ld a,c
	and a,#0xF0
	or a,#6
	out (#0xAA),a
	in a,(#0xA9)             ; linha 6: bit0 = shift
	cpl
	and a,#1
	add a,a                  ; bit1 = shift
	or a,b                   ; a = estado agora
	ld b,a
	ld a,(_k_prev)
	cpl
	and a,b                  ; bordas de subida
	ld (_k_edge),a
	rra
	jr nc,1$
	ld a,#1
	ld (_ev_space),a
1$:	ld a,(_k_edge)
	and a,#2
	jr z,2$
	ld (_ev_shift),a
2$:	ld a,b
	ld (_k_prev),a
	ld a,c
	out (#0xAA),a            ; restaura porta C
	pop bc
	pop af
	jp _old_timi
	.area _DATA
_k_edge:	.ds 1
	.area _CODE

; ---------------------------------------------------------------------
; _lz_unpack: descomprime (formato LZ4-like, ver gen_assets.py) de (g_lzsrc) para _cmd_buf (reaproveitado como buffer na carga)
	.area _DATA
lz_tok:	.ds 1
	.area _CODE
_lz_unpack::
	ld hl,(_g_lzsrc)
	ld de,#_cmd_buf
lz_next:
	ld a,(hl)
	inc hl
	ld (lz_tok),a
	rrca
	rrca
	rrca
	rrca
	and a,#15
	ld c,a
	ld b,#0
	cp a,#15
	call z,lz_ext
	ld a,b
	or a,c
	jr z,lz_nolit
	ldir
lz_nolit:
	ld c,(hl)
	inc hl
	ld b,(hl)
	inc hl
	ld a,b
	or a,c
	ret z                    ; offset 0 = fim
	push hl
	ld h,d
	ld l,e
	or a,a
	sbc hl,bc
	ex (sp),hl               ; hl = ponteiro de origem, (sp) = inicio da copia
	ld a,(lz_tok)
	and a,#15
	ld c,a
	ld b,#0
	cp a,#15
	call z,lz_ext
	inc bc
	inc bc
	inc bc
	inc bc
	ex (sp),hl
	ldir
	pop hl
	jr lz_next
lz_ext:
1$:	ld a,(hl)
	inc hl
	push af
	add a,c
	ld c,a
	jr nc,2$
	inc b
2$:	pop af
	inc a
	jr z,1$
	ret

; _vram_blit: copia g_vpg paginas de 256 bytes de _cmd_buf para a VRAM em (g_vhi:g_vlo)
_vram_blit::
	di
	ld a,(_g_vhi)
	add a,a
	add a,a
	ld b,a
	ld a,(_g_vlo+1)
	rlca
	rlca
	and a,#3
	or a,b
	out (#0x99),a
	ld a,#0x8E
	out (#0x99),a
	ld a,(_g_vlo)
	out (#0x99),a
	ld a,(_g_vlo+1)
	and a,#0x3F
	or a,#0x40
	out (#0x99),a
	ld a,(_g_vpg)
	ld d,a
	ld hl,#_cmd_buf
	ld c,#0x98
1$:	ld b,#0
	otir
	dec d
	jr nz,1$
	xor a,a
	out (#0x99),a
	ld a,#0x8E
	out (#0x99),a
	ei
	ret

; _vdp_cmd15: espera CE=0 e envia 15 bytes de _vc para R#32..R#46 (HMMM/LINE)
_vdp_cmd15::
	call ex_wait
	di
	ld a,#32
	out (#0x99),a
	ld a,#0x91
	out (#0x99),a
	ld hl,#_vc
	ld bc,#0x0F9B
	otir
	ei
	ret

; _psg_w: escreve g_val no registrador g_reg do PSG (AY-3-8910), portas 0xA0/0xA1
_psg_w::
	ld a,(_g_reg)
	di
	out (#0xA0),a
	ld a,(_g_val)
	out (#0xA1),a
	ei
	ret

; _pal_one: define a cor g_reg (0..15) da paleta como (g_val = R<<4|B, g_sh = G)
_pal_one::
	ld a,(_g_reg)
	di
	out (#0x99),a
	ld a,#0x90
	out (#0x99),a
	ld a,(_g_val)
	out (#0x9A),a
	ld a,(_g_sh)
	out (#0x9A),a
	ei
	ret

; _load_assets: para cada faixa da tabela (assets.inc): seleciona o banco ASCII16 em 0x8000, descomprime e copia para a VRAM
; (copias = 1: so no destino; 3: paginas 0, 1 e 2). Tem de ficar na pagina 1 (nao trocada), como lz_unpack e vram_blit.
	.include "assets.inc"
_load_assets::
	ld a,#ASSET_N
	or a,a
	ret z
	ld hl,#asset_tab
	ld b,#ASSET_N
la_next:
	push bc
	ld a,(_g_group)
	ld c,a
	ld a,(hl)
	inc hl
	ld b,a                    ; b = banco
	ld a,(hl)
	inc hl
	cp a,c
	jr z,la_go
	ld de,#7
	add hl,de                 ; faixa de outro grupo: pula src(2) vlo(2) vhi pg cp
	jr la_done
la_go:
	ld a,b
	ld (0x7000),a
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ld (_g_lzsrc),de
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ld (_g_vlo),de
	ld a,(hl)
	inc hl
	ld (_g_vhi),a
	ld a,(hl)
	inc hl
	ld (_g_vpg),a
	ld a,(hl)
	inc hl
	push af
	push hl
	call _lz_unpack
	call _vram_blit
	pop hl
	pop af
	cp a,#1
	jr z,la_done
	push hl
	ld hl,(_g_vlo)
	ld a,h
	xor a,#0x80
	ld h,a
	ld (_g_vlo),hl            ; segunda copia: pagina 1 (vlo + 0x8000)
	call _vram_blit
	ld hl,(_g_vlo)
	ld a,h
	xor a,#0x80
	ld h,a
	ld (_g_vlo),hl
	ld a,#1
	ld (_g_vhi),a             ; terceira copia: pagina 2 (A16 = 1)
	call _vram_blit
	pop hl
la_done:
	pop bc
	djnz la_next
	ld a,(_cur_bank)
	ld (0x7000),a             ; volta o banco de codigo corrente em 0x8000
	ret

; _pal_load2: carrega a paleta de 32 bytes apontada por g_palp
_pal_load2::
	ld hl,(_g_palp)
	di
	xor a,a
	out (#0x99),a
	ld a,#0x90
	out (#0x99),a
	ld bc,#0x209A
	otir
	ei
	ret

; _bank_copy: copia g_bk_len bytes de (g_bk_src em 0x8000+, banco g_bk_bank) para g_bk_dst; volta o banco 1 (tem de ficar na pagina 1)
_bank_copy::
	ld a,(_g_bk_bank)
	ld (0x7000),a
	ld hl,(_g_bk_src)
	ld de,(_g_bk_dst)
	ld a,(_g_bk_len)
	ld c,a
	ld b,#0
	ldir
	ld a,(_cur_bank)
	ld (0x7000),a
	ret

; ---- troca de bancos de codigo (SDCC __banked: ___sdcc_bcall_ehl usa get_bank/set_bank, que preservam os demais registradores)
	.area _DATA
_cur_bank::
	.ds 1
	.area _CODE
get_bank::
	ld a,(_cur_bank)
	ret
set_bank::
	ld (_cur_bank),a
	ld (0x7000),a
	ret
