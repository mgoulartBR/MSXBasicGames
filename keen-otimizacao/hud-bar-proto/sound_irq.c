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
 ld a,(_bar_page2)
 out (#0x99),a
 ld a,#0x82
 out (#0x99),a
 ld a,(_bar_row)
 out (#0x99),a
 ld a,#0x97
 out (#0x99),a
 xor a
 out (#0x99),a
 ld a,#0x9a
 out (#0x99),a
 xor a
 out (#0x99),a
 ld a,#0x9b
 out (#0x99),a
sound_irq_pop:
 pop hl
 pop de
 pop bc
 pop af
 jp _sound_old_hook
 __endasm;
}

// H.KEYI hook: line interrupt at the end of the fixed HUD bar. Applies the game's page/scroll.
void LineISR(void) __naked
{
 __asm
 push af
 ld a,(_bar_on)
 or a
 jr z,line_ret
 ld a,#1
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
 in a,(#0x99)
 push af
 xor a
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
 pop af
 rrca
 jr nc,line_ret
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
line_ret:
 pop af
 ret
 __endasm;
}
