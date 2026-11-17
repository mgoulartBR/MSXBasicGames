// Fixed-bank IRQ: all code and operands stay accessible during bank changes.
void SoundISR(void) __naked
{
 __asm
 push af
 push bc
 push de
 push hl
 ld a,(_sound_duration)
 or a
 jr z,sound_irq_done
 dec a
 ld (_sound_duration),a
 jr nz,sound_irq_running
 xor a
 ld (_sound_priority),a
 ld e,a
 jr sound_irq_volume
sound_irq_running:
 ld b,a
 ld a,(_sound_volume)
 ld e,a
 ld a,b
 cp #8
 jr nc,sound_irq_tone
 ld e,a
sound_irq_tone:
 ld hl,(_sound_period)
 ld de,(_sound_step)
 add hl,de
 ld (_sound_period),hl
 ld a,#0
 out (#0xa0),a
 ld a,l
 out (#0xa1),a
 ld a,#1
 out (#0xa0),a
 ld a,h
 and #15
 out (#0xa1),a
 ld a,(_sound_duration)
 cp #8
 jr c,sound_irq_fade
 ld a,(_sound_volume)
sound_irq_fade:
 ld e,a
sound_irq_volume:
 ld a,#8
 out (#0xa0),a
 ld a,e
 out (#0xa1),a
sound_irq_done:
 ld a,(_flip_flag)
 or a
 jp z,sound_irq_end
 ; --- vblank flip requested by DrawScene: sprite tables, page, scroll (VDP writes are atomic here)
 ld a,(_flip_r5)
 out (#0x99),a
 ld a,#0x85
 out (#0x99),a
 ld a,(_flip_r6)
 out (#0x99),a
 ld a,#0x86
 out (#0x99),a
 ld a,(_flip_r11)
 out (#0x99),a
 ld a,#0x8b
 out (#0x99),a
 ld a,(_flip_r8)
 ld (_g_VDP_REGSAV+8),a
 ld (_cur_r8),a
 out (#0x99),a
 ld a,#0x88
 out (#0x99),a
 ld a,(_flip_flag)
 bit 1,a
 jr z,sound_flip_done
 ld a,(_flip_r2)
 ld (_g_VDP_REGSAV+2),a
 ld (_cur_r2),a
 out (#0x99),a
 ld a,#0x82
 out (#0x99),a
 ld a,(_flip_flag)
 bit 2,a
 jp z,sound_flip_done
 ld a,(_drawn_camera_py)
 ld (_cur_r23),a
 out (#0x99),a
 ld a,#0x97
 out (#0x99),a
 ld hl,(_drawn_camera_px)
 ld de,#7
 add hl,de
 srl h
 rr l
 srl h
 rr l
 srl h
 rr l
 ld a,l
 and #31
 ld (_cur_r26),a
 out (#0x99),a
 ld a,#0x9a
 out (#0x99),a
 ld hl,(_drawn_camera_px)
 ld a,#8
 sub l
 and #7
 ld (_cur_r27),a
 out (#0x99),a
 ld a,#0x9b
 out (#0x99),a
sound_flip_done:
 xor a
 ld (_flip_flag),a
sound_irq_end:
 ld a,(_bar_on)
 or a
 jr z,sound_irq_pop
 ; fixed HUD bar at the bottom: the bar routine changed page/scroll/sprites during lines 184-191;
 ; restore the game's values and arm the line interrupt 8 lines ahead of the bar (line 175)
 ld a,(_cur_r2)
 out (#0x99),a
 ld a,#0x82
 out (#0x99),a
 ld a,(_cur_r23)
 out (#0x99),a
 ld a,#0x97
 out (#0x99),a
 ld a,(_cur_r26)
 out (#0x99),a
 ld a,#0x9a
 out (#0x99),a
 ld a,(_cur_r27)
 out (#0x99),a
 ld a,#0x9b
 out (#0x99),a
 ld a,(_cur_r8)
 out (#0x99),a
 ld a,#0x88
 out (#0x99),a
 ld a,(_cur_r23)
 add a,#175
 out (#0x99),a
 ld a,#0x93
 out (#0x99),a
sound_irq_pop:
 pop hl
 pop de
 pop bc
 pop af
 jp _sound_old_hook
 __endasm;
}

// H.KEYI hook (fixed HUD bar on lines 184-191). The line interrupt armed by the vblank handler
// fires around line 175 (+ BIOS latency of up to a few lines). The handler then
//   1. re-arms R#19 for line 183 and polls S#1.FH (no interrupt latency involved) to sync exactly,
//   2. follows S#2.HR (horizontal retrace) to switch the bar's row segments at line boundaries:
//        end of 183: HUD page, scroll 0, sprites off, R23 -> rows 70,71 on lines 184-185
//        end of 185: R23 -> row 75 (line 186)       end of 186: R23 -> rows 77-79 (lines 187-189)
//        end of 189: R23 -> rows 86,87 (lines 190-191)
//   The vblank handler restores the game registers. Row at line L = (R#23 + L) & 255.
void LineISR(void) __naked
{
 __asm
 push af
 push bc
 push de
 push hl
 exx
 push bc
 push de
 push hl
 exx
 ld a,(_bar_on)
 or a
 jp z,line_ret
 ld a,#1
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
 in a,(#0x99)
 bit 0,a
 jp z,line_done
 ld a,(_cur_r23)
 add a,#182
 out (#0x99),a
 ld a,#0x93
 out (#0x99),a
 ld b,#0
line_sync:
 in a,(#0x99)
 bit 0,a
 jr nz,line_synced
 djnz line_sync
 jp line_abort
line_synced:
 ld a,#2
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
 ld a,(_cur_r8)
 or #2
 exx
 ld h,a
 ld c,#0x99
 ld d,#0x9a
 ld e,#0x9b
 ld l,#0x88
 exx
 ld c,#0x99
 ld d,#142
 ld e,#0x97
 ld h,#0x5f
 ld l,#0x82
 xor a
 exx
 out (c),h
 out (c),l
 exx
w1_1:
 in a,(c)
 and #0x20
 jr nz,w1_1
w2_1:
 in a,(c)
 and #0x20
 jr z,w2_1
 xor a
 exx
 out (c),a
 out (c),d
 out (c),a
 out (c),e
 exx
 out (c),d
 out (c),e
 out (c),h
 out (c),l
 ld h,#145
 ld l,#146
w1_x:
 in a,(c)
 and #0x20
 jr nz,w1_x
w2_x:
 in a,(c)
 and #0x20
 jr z,w2_x
w1_2:
 in a,(c)
 and #0x20
 jr nz,w1_2
w2_2:
 in a,(c)
 and #0x20
 jr z,w2_2
 out (c),h
 out (c),e
w1_3:
 in a,(c)
 and #0x20
 jr nz,w1_3
w2_3:
 in a,(c)
 and #0x20
 jr z,w2_3
 out (c),l
 out (c),e
w1_4:
 in a,(c)
 and #0x20
 jr nz,w1_4
w2_4:
 in a,(c)
 and #0x20
 jr z,w2_4
w1_5:
 in a,(c)
 and #0x20
 jr nz,w1_5
w2_5:
 in a,(c)
 and #0x20
 jr z,w2_5
w1_6:
 in a,(c)
 and #0x20
 jr nz,w1_6
w2_6:
 in a,(c)
 and #0x20
 jr z,w2_6
 ld a,#152
 out (c),a
 out (c),e
 jp line_done
line_abort:
line_done:
 xor a
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
line_ret:
 exx
 pop hl
 pop de
 pop bc
 exx
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
