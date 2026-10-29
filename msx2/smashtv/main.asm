; ---------------------------------------------------------------------------
; Smash T.V. (MSX2) - technical skeleton
; 16 KB ROM at $4000.  Screen 5, hardware sprites for the player and bullets,
; VDP-command blits for the enemies (double buffered), 20 Hz game tick (VBlank / 3) like the SMS original.
; Build: tools/msx/build.sh -> msx2/smashtv/build/smashtv.rom
; ---------------------------------------------------------------------------
        include "defs.inc"

        org 0x4000
        db "AB"
        dw init                 ; INIT
        dw 0, 0, 0              ; STATEMENT, DEVICE, TEXT
        ds 6, 0

init:   di
        ld sp,STACK_TOP
        ld a,(MSXVER)
        or a
        jp z,not_msx2
        ld a,5
        call CHGMOD             ; SCREEN 5
        call vdp_init
        call load_palette
        call upload_gfx
        call draw_arena
        call copy_clean
        call game_init
        ei

main_loop:
.wait:  halt
        ld a,(JIFFY)
        ld hl,last_jiffy
        sub (hl)
        cp TICK_VBLANKS
        jr c,.wait
        ld a,(JIFFY)
        ld (last_jiffy),a
        ld a,2                  ; backdrop colours show where the frame time goes
        call set_border
        call read_input
        call update_player
        call update_bullets
        call update_enemies
        call collide_bullets
        call collide_player
        call script_step
        call check_exit
        ld a,6
        call set_border
        call render
        call flip               ; waits for the VBlank, shows the finished page
        jp main_loop

not_msx2:
        ld hl,msg_msx2
.p:     ld a,(hl)
        or a
        jr z,.stop
        call CHPUT
        inc hl
        jr .p
.stop:  jr .stop
msg_msx2:
        db "MSX2 required", 0

        include "vdp.inc"
        include "game.inc"
        include "script.inc"
        include "rooms.inc"
        include "gfx.inc"

        ds 0x8000-$, 0xFF       ; pad to 16 KB (an error here means the ROM outgrew its page)
