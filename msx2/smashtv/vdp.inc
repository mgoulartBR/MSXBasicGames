; ---------------------------------------------------------------------------
; vdp.inc - V9938 helpers: register setup, palette, VRAM access, command engine
; Interrupts are disabled around every sequence of two writes to the control
; port, otherwise the BIOS VBlank handler (which reads the status register)
; could split the pair.
; ---------------------------------------------------------------------------

; (data, register) pairs written through the BIOS WRTVDP, $FF ends the table
vdp_regs:
        db 0x62, 1              ; display on, VBlank IRQ on, 16x16 sprites
        db 0xEF, 5              ; sprite attribute table -> $7600 (colour table $7400)
        db 0x00, 11
        db 0x0F, 6              ; sprite patterns -> $7800
        db 0x28, 8              ; 64K VRAM chips, colour 0 is a real colour (TP), sprites on
        db 0x1F, 2              ; display page 0
        db 0x00, 7              ; backdrop colour 0 (changed at run time as a profiler)
        db 0xFF

vdp_init:
        ld hl,vdp_regs
.next:  ld a,(hl)
        cp 0xFF
        ret z
        ld b,a
        inc hl
        ld c,(hl)
        inc hl
        push hl
        call WRTVDP
        pop hl
        jr .next

; A = backdrop colour 0..15, only while the profiler is on (F1)
set_border:
        ld b,a
        ld a,(profile_on)
        or a
        ret z
        ld a,b
border_raw:
        di
        out (VDP_CTRL),a
        ld a,0x87
        out (VDP_CTRL),a
        ei
        ret

load_palette:
        di
        xor a
        out (VDP_CTRL),a
        ld a,0x90               ; R#16 = 0: first palette entry
        out (VDP_CTRL),a
        ld hl,palette
        ld b,32
        ld c,VDP_PAL
        otir
        ei
        ret

; HL = VRAM address bits 15..0, A = bit 16 (set_vram_wr: bit 16 = 0); sets up a write. Clobbers A, B.
set_vram_wr:
        xor a
set_vram_wr17:
        di
        add a,a
        add a,a                 ; bit 16 -> bit 2 of R#14
        ld b,a
        ld a,h
        rlca
        rlca
        and 3
        or b
        out (VDP_CTRL),a        ; R#14 = address bits 16..14
        ld a,0x8E
        out (VDP_CTRL),a
        ld a,l
        out (VDP_CTRL),a
        ld a,h
        and 0x3F
        or 0x40
        out (VDP_CTRL),a
        ei
        ret

; HL = source, BC = byte count (> 0); VRAM write address already set
vram_out:
        ld a,(hl)
        out (VDP_DATA),a
        inc hl
        dec bc
        ld a,b
        or c
        jr nz,vram_out
        ret

; waits for the command engine (status register 2, bit 0 = CE); preserves BC
vdp_wait:
        push bc
.poll:  di
        ld a,2
        out (VDP_CTRL),a
        ld a,0x8F
        out (VDP_CTRL),a
        in a,(VDP_CTRL)
        ld b,a
        xor a
        out (VDP_CTRL),a        ; back to status register 0 for the BIOS handler
        ld a,0x8F
        out (VDP_CTRL),a
        ei
        bit 0,b
        jr nz,.poll
        pop bc
        ret

; starts the command described by cmd_buf (R#32..R#46); clobbers A, BC, HL
vdp_cmd:
        call vdp_wait
        di
        ld a,32
        out (VDP_CTRL),a
        ld a,0x91               ; R#17 = 32, auto increment
        out (VDP_CTRL),a
        ld hl,cmd_buf
        ld b,15
        ld c,VDP_INDIR
        otir
        ei
        ret

upload_gfx:
        ld hl,SPR_PAT_VRAM
        call set_vram_wr
        ld hl,spr_pat
        ld bc,64
        call vram_out
        ld hl,SPR_COL_VRAM
        call set_vram_wr
        ld hl,spr_col
        ld bc,512
        call vram_out
        ; sheet records: dw vram_addr(15..0), db addr bit 16, db length, bytes...; dw 0 ends
        ld hl,sheet_data
.rec:   ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld a,d
        or e
        ret z
        ld a,(hl)               ; address bit 16
        inc hl
        ld b,(hl)               ; length
        inc hl
        push hl
        ex de,hl                ; HL = address, DE = source
        push bc
        call set_vram_wr17
        pop bc
        pop hl
        ld c,VDP_DATA
        otir
        jr .rec

upload_sprites:
        ld hl,SPR_ATT_VRAM
        call set_vram_wr
        ld hl,spr_shadow
        ld b,128
        ld c,VDP_DATA
        otir
        ret

; rectangles: dw x, y, w, h : db colour ; x = $FFFF ends the table
arena_rects:
        dw 0, 0, 256, 212
        db 0                    ; clear the visible page
        dw 0, 0, 256, 16
        db 14                   ; HUD bar
        dw 8, 16, 240, 176
        db 3                    ; wall
        dw ARENA_X0, ARENA_Y0, ARENA_X1-ARENA_X0, ARENA_Y1-ARENA_Y0
        db 2                    ; floor
        dw 0xFFFF

draw_arena:
        ld hl,arena_rects
.next:  ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld a,e
        and d
        cp 0xFF
        ret z
        ld (cmd_dx),de
        ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld (cmd_dy),de
        ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld (cmd_nx),de
        ld e,(hl)
        inc hl
        ld d,(hl)
        inc hl
        ld (cmd_ny),de
        ld a,(hl)
        inc hl
        ld b,a
        add a,a
        add a,a
        add a,a
        add a,a
        or b
        ld (cmd_clr),a
        xor a
        ld (cmd_arg),a
        ld a,0xC0               ; HMMV
        ld (cmd_cmd),a
        push hl
        call vdp_cmd
        pop hl
        jr .next

; copies the finished arena (page 0) to the clean page (page 2) and to the second buffer (page 1)
copy_clean:
        ld hl,0
        ld (cmd_sx),hl
        ld (cmd_sy),hl
        ld (cmd_dx),hl
        ld hl,CLEAN_Y
        ld (cmd_dy),hl
        call copy_page
        ld hl,256
        ld (cmd_dy),hl
        jr copy_page

; copies the clean page over both display buffers (erases every blitted object)
restore_all:
        ld hl,0
        ld (cmd_sx),hl
        ld (cmd_dx),hl
        ld hl,CLEAN_Y
        ld (cmd_sy),hl
        ld hl,0
        ld (cmd_dy),hl
        call copy_page
        ld hl,256
        ld (cmd_dy),hl
copy_page:
        ld hl,256
        ld (cmd_nx),hl
        ld hl,212
        ld (cmd_ny),hl
        xor a
        ld (cmd_arg),a
        ld a,0xD0               ; HMMM
        ld (cmd_cmd),a
        jp vdp_cmd
