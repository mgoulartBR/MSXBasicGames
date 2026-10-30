; ---------------------------------------------------------------------------
; game.inc - input, player, bullets, enemies, collisions, rendering
; ---------------------------------------------------------------------------

; ---- helpers ----------------------------------------------------------------

; same idea as the original (LD A,R accumulated in RAM)
rng:    push hl
        ld hl,rng_acc
        ld a,r
        add a,(hl)
        ld (hl),a
        pop hl
        ret

; GTSTCK result 0..8 (clockwise from up) -> direction bits
dir_to_bits:
        ld e,a
        ld d,0
        ld hl,dir_tab
        add hl,de
        ld a,(hl)
        ret
dir_tab:
        db 0, DIR_UP, DIR_UP|DIR_RIGHT, DIR_RIGHT, DIR_DOWN|DIR_RIGHT
        db DIR_DOWN, DIR_DOWN|DIR_LEFT, DIR_LEFT, DIR_UP|DIR_LEFT

; ---- input ------------------------------------------------------------------
; Move: cursor keys or joystick 1.  Aim: W/A/S/D or joystick 2.  Fire: space or joy 1 button A
; (with the fire button alone the player shoots in the last direction faced).
read_input:
        call check_profile_key
        xor a
        call GTSTCK
        or a
        jr nz,.have
        ld a,1
        call GTSTCK
.have:  call dir_to_bits
        ld (input_move),a
        xor a
        ld (input_aim),a
        ld a,5                  ; row 5: Z Y X W V U T S (bit 7..0)
        call SNSMAT
        cpl
        ld hl,input_aim
        bit 4,a
        jr z,.n1
        set 0,(hl)              ; W = up
.n1:    bit 0,a
        jr z,.n2
        set 1,(hl)              ; S = down
.n2:    ld a,2                  ; row 2: B A _ / . , ` '
        call SNSMAT
        cpl
        bit 6,a
        jr z,.n3
        ld hl,input_aim
        set 2,(hl)              ; A = left
.n3:    ld a,3                  ; row 3: J I H G F E D C
        call SNSMAT
        cpl
        bit 1,a
        jr z,.n4
        ld hl,input_aim
        set 3,(hl)              ; D = right
.n4:    ld a,(input_aim)
        or a
        jr nz,.fire
        ld a,2
        call GTSTCK
        call dir_to_bits
        ld (input_aim),a
.fire:  xor a
        ld (input_fire),a
        call GTTRIG             ; A = 0: space bar
        or a
        jr nz,.pressed
        ld a,1
        call GTTRIG
        or a
        ret z
.pressed:
        ld a,1
        ld (input_fire),a
        ret

; F1 toggles the profiler (backdrop colour per phase); turning it off restores a black border
check_profile_key:
        ld a,6                  ; row 6: F3 F2 F1 CODE CAPS GRAPH CTRL SHIFT
        call SNSMAT
        cpl
        and 0x20                ; F1
        ld b,a
        ld a,(key_prev)
        ld c,a
        ld a,b
        ld (key_prev),a
        or a
        ret z
        ld a,c
        or a
        ret nz                  ; still held: no repeat
        ld a,(profile_on)
        xor 1
        ld (profile_on),a
        ret nz
        xor a
        jp border_raw

; ---- player -------------------------------------------------------------------
update_player:
        ld a,(p_inv)
        or a
        jr z,.noinv
        dec a
        ld (p_inv),a
.noinv: ld a,(p_cool)
        or a
        jr z,.nocool
        dec a
        ld (p_cool),a
.nocool:
        call weapon_tick
        ld a,(input_move)
        or a
        jr z,.moved
        ld (p_face),a
        ld b,a
        ld a,(p_x)
        bit 2,b
        jr z,.r
        sub PLAYER_SPEED
        jr .xc
.r:     bit 3,b
        jr z,.xc
        add a,PLAYER_SPEED
.xc:    cp ARENA_X0
        jr nc,.xhi
        ld a,ARENA_X0
.xhi:   cp ARENA_X1-16+1
        jr c,.xok
        ld a,ARENA_X1-16
.xok:   ld (p_x),a
        ld a,(p_y)
        bit 0,b
        jr z,.d
        sub PLAYER_SPEED
        jr .yc
.d:     bit 1,b
        jr z,.yc
        add a,PLAYER_SPEED
.yc:    cp ARENA_Y0
        jr nc,.yhi
        ld a,ARENA_Y0
.yhi:   cp ARENA_Y1-16+1
        jr c,.yok
        ld a,ARENA_Y1-16
.yok:   ld (p_y),a
.moved: ld a,(input_aim)
        ld b,a
        or a
        jr nz,.shoot
        ld a,(input_fire)
        or a
        ret z
        ld a,(p_face)
        ld b,a
        or a
        ret z
.shoot: ld a,(p_cool)
        or a
        ret nz
        jp fire_weapon          ; B = direction bits

; ---- bullets ------------------------------------------------------------------
spawn_bullet:
        ld ix,BUL_BASE
        ld c,16
.find:  ld a,(ix+B_TYPE)
        or a
        jr z,.free
        inc ix
        dec c
        jr nz,.find
        ret
.free:  ld a,(w_pierce)
        inc a                   ; 1 normal, 2 piercing
        ld (ix+B_TYPE),a
        ld a,(w_dmg)
        ld (ix+B_DMG),a
        ld (ix+B_DIR),b
        ld a,(p_x)
        add a,4
        ld (ix+B_X),a
        ld a,(p_y)
        add a,4
        ld (ix+B_Y),a
        ret

update_bullets:
        ld ix,BUL_BASE
        ld b,16
.loop:  ld a,(ix+B_TYPE)
        or a
        jr z,.next
        ld c,(ix+B_DIR)
        ld a,(ix+B_X)
        bit 2,c
        jr z,.r
        sub BULLET_SPEED
        jr .xs
.r:     bit 3,c
        jr z,.xs
        add a,BULLET_SPEED
.xs:    ld (ix+B_X),a
        cp ARENA_X0
        jr c,.kill
        cp ARENA_X1-8+1
        jr nc,.kill
        ld a,(ix+B_Y)
        bit 0,c
        jr z,.d
        sub BULLET_SPEED
        jr .ys
.d:     bit 1,c
        jr z,.ys
        add a,BULLET_SPEED
.ys:    ld (ix+B_Y),a
        cp ARENA_Y0
        jr c,.kill
        cp ARENA_Y1-8+1
        jr c,.next
.kill:  ld (ix+B_TYPE),0
.next:  inc ix
        djnz .loop
        ret

; ---- enemies --------------------------------------------------------------------
; per-kind tables: hit points, speed (px per tick) and score (BCD)
kind_hp:    db 1, 1, 3
kind_speed: db 2, 3, 1
kind_points: db 0x10, 0x20, 0x50

rand_x: call rng
        cp 209
        jr c,.ok
        sub 209
.ok:    add a,ARENA_X0
        ret

rand_y: call rng
        cp 145
        jr c,.ok
        sub 145
.ok:    add a,ARENA_Y0
        ret

; IX = free slot (Z) or NZ when the pool is full
find_enemy_slot:
        ld ix,EN_BASE
        ld b,16
.f:     ld a,(ix+E_TYPE)
        or a
        ret z
        inc ix
        djnz .f
        or 1
        ret

; IX = slot, C = kind: makes the slot a live enemy (position is set by the caller)
init_enemy:
        ld (ix+E_TYPE),1
        ld (ix+E_KIND),c
        ld (ix+E_OX),0
        ld (ix+E_OXB),0
        ld (ix+E_FRAME),0
        ld hl,kind_hp
        ld e,c
        ld d,0
        add hl,de
        ld a,(hl)
        ld (ix+E_HP),a
        ret

; IX = slot: random position on one of the four edges of the arena
random_edge_pos:
        call rng
        and 3
        jr z,.top
        cp 1
        jr z,.bottom
        cp 2
        jr z,.left
        ld (ix+E_X),ARENA_X1-16
        call rand_y
        ld (ix+E_Y),a
        ret
.top:   ld (ix+E_Y),ARENA_Y0
        call rand_x
        ld (ix+E_X),a
        ret
.bottom:
        ld (ix+E_Y),ARENA_Y1-16
        call rand_x
        ld (ix+E_X),a
        ret
.left:  ld (ix+E_X),ARENA_X0
        call rand_y
        ld (ix+E_Y),a
        ret

; enemies walk toward the player at their kind's speed, direction bits kept in E_DIR
update_enemies:
        ld ix,EN_BASE
        ld b,16
.loop:  ld a,(ix+E_TYPE)
        cp 1
        jr nz,.next
        ld a,(ix+E_KIND)
        ld e,a
        ld d,0
        ld hl,kind_speed
        add hl,de
        ld e,(hl)               ; E = speed
        ld d,0                  ; D = direction bits being built
        ld a,(p_x)
        ld c,a
        ld a,(ix+E_X)
        cp c
        jr z,.v
        jr c,.right
        sub e
        set 2,d
        jr .xs
.right: add a,e
        set 3,d
.xs:    ld (ix+E_X),a
.v:     ld a,(p_y)
        ld c,a
        ld a,(ix+E_Y)
        cp c
        jr z,.dir
        jr c,.down
        sub e
        set 0,d
        jr .ys
.down:  add a,e
        set 1,d
.ys:    ld (ix+E_Y),a
.dir:   ld (ix+E_DIR),d
        ld a,(ix+E_FRAME)
        xor 1
        ld (ix+E_FRAME),a
.next:  inc ix
        djnz .loop
        ret

; ---- collisions -----------------------------------------------------------------
add_score:                      ; A = points (BCD, < 100); 3 byte BCD score like the original
        push hl
        ld hl,score
        add a,(hl)
        daa
        ld (hl),a
        inc hl
        ld a,(hl)
        adc a,0
        daa
        ld (hl),a
        inc hl
        ld a,(hl)
        adc a,0
        daa
        ld (hl),a
        ld a,2                  ; the HUD must be redrawn in both buffers
        ld (score_dirty),a
        pop hl
        ret

; bullet (8x8) against enemy (16x16)
collide_bullets:
        ld iy,BUL_BASE
        ld b,16
.bl:    ld a,(iy+B_TYPE)
        or a
        jr z,.bnext
        push bc
        ld ix,EN_BASE
        ld c,16
.el:    ld a,(ix+E_TYPE)
        cp 1
        jr nz,.enext
        ld a,(ix+E_X)
        add a,16
        ld d,a
        ld a,(iy+B_X)
        cp d
        jr nc,.enext            ; bullet right of enemy
        add a,8
        ld d,a
        ld a,(ix+E_X)
        cp d
        jr nc,.enext            ; bullet left of enemy
        ld a,(ix+E_Y)
        add a,16
        ld d,a
        ld a,(iy+B_Y)
        cp d
        jr nc,.enext
        add a,8
        ld d,a
        ld a,(ix+E_Y)
        cp d
        jr nc,.enext
        ld a,(ix+E_HP)          ; hit
        sub (iy+B_DMG)
        jr z,.kill
        jr nc,.hurt
.kill:  ld (ix+E_HP),0
        call kill_enemy
        ld a,(iy+B_TYPE)
        cp 2
        jr z,.enext             ; a piercing shot goes on through what it killed
        jr .used
.hurt:  ld (ix+E_HP),a
.used:  ld (iy+B_TYPE),0        ; the bullet is consumed
        pop bc
        jr .bnext
.enext: inc ix
        dec c
        jr nz,.el
        pop bc
.bnext: inc iy
        djnz .bl
        ret

; enemy against player: boxes inset by 4 px, i.e. |dx| < 8 and |dy| < 8
collide_player:
        ld a,(p_inv)
        or a
        ret nz
        ld ix,EN_BASE
        ld b,16
.l:     ld a,(ix+E_TYPE)
        cp 1
        jr nz,.n
        ld a,(p_x)
        sub (ix+E_X)
        jr nc,.p1
        neg
.p1:    cp 8
        jr nc,.n
        ld a,(p_y)
        sub (ix+E_Y)
        jr nc,.p2
        neg
.p2:    cp 8
        jr nc,.n
        ld a,(lives)
        dec a
        ld (lives),a
        ld a,2
        ld (score_dirty),a
        ld a,(lives)
        or a
        jp z,game_over
        ld a,40
        ld (p_inv),a
        ret
.n:     inc ix
        djnz .l
        ret

; ---- rendering ------------------------------------------------------------------
; Double buffering: everything is drawn into the hidden page (back_y = its first line), then
; flip waits for the VBlank, shows it and uploads the sprites in the same blanking period.
render:
        call setup_back
        call build_sprites
        call build_item_cols
        call restore_back
        call erase_enemies
        call draw_door
        call draw_enemies
        jp draw_hud

; after a game over every enemy image has to go: the clean page is copied over the hidden page
; (twice in a row, so both pages get it)
restore_back:
        ld a,(restore_pending)
        or a
        ret z
        dec a
        ld (restore_pending),a
        ld hl,0
        ld (cmd_sx),hl
        ld (cmd_dx),hl
        ld hl,CLEAN_Y
        ld (cmd_sy),hl
        ld hl,(back_y)
        ld (cmd_dy),hl
        jp copy_page

setup_back:
        ld hl,256               ; front page 0 -> draw into page 1
        ld a,(front_page)
        or a
        jr z,.s
        ld hl,0                 ; front page 1 -> draw into page 0
.s:     ld (back_y),hl
        ret

; plane 0 = player, planes 1..16 = bullet slots, the rest hidden
build_sprites:
        ld hl,spr_shadow
        ld a,(p_inv)
        and 1                   ; blink while invulnerable
        jr nz,.hide
        ld a,(p_y)
        dec a                   ; the VDP shows sprites one line below Y
        ld (hl),a
        jr .px
.hide:  ld (hl),SPR_HIDDEN
.px:    inc hl
        ld a,(p_x)
        ld (hl),a
        inc hl
        ld (hl),0               ; pattern 0 = player
        inc hl
        ld (hl),0
        inc hl
        ld ix,BUL_BASE
        ld b,16
.bl:    ld a,(ix+B_TYPE)
        or a
        jr z,.nob
        ld a,(ix+B_Y)
        sub 5                   ; centre the dot (-4) and the one line offset (-1)
        ld (hl),a
        inc hl
        ld a,(ix+B_X)
        sub 4
        ld (hl),a
        jr .rest
.nob:   ld (hl),SPR_HIDDEN
        inc hl
        ld (hl),0
.rest:  inc hl
        ld (hl),4               ; pattern 1 = bullet
        inc hl
        ld (hl),0
        inc hl
        inc ix
        djnz .bl
        ld iy,IT_BASE
        ld b,6
.it:    ld a,(iy+I_TYPE)
        or a
        jr z,.noi
        ld a,(iy+I_LIFE)
        cp 30
        jr nc,.vis
        and 1                   ; blink in the last 30 ticks
        jr nz,.noi
.vis:   ld a,(iy+I_Y)
        dec a
        ld (hl),a
        inc hl
        ld a,(iy+I_X)
        ld (hl),a
        inc hl
        ld a,(iy+I_TYPE)
        dec a
        add a,a
        add a,a
        add a,8                 ; pattern number = 8 + 4 * (type - 1)
        ld (hl),a
        jr .ni
.noi:   ld (hl),SPR_HIDDEN
        inc hl
        ld (hl),0
        inc hl
        ld (hl),0
.ni:    inc hl
        ld (hl),0
        inc hl
        inc iy
        djnz .it
        ld b,9
.hid:   ld (hl),SPR_HIDDEN
        inc hl
        ld (hl),0
        inc hl
        ld (hl),0
        inc hl
        ld (hl),0
        inc hl
        djnz .hid
        ret

; IY = the enemy pool as seen by the hidden page: its last-drawn positions are E_OX/E_OY (page 0)
; or E_OXB/E_OYB (page 1), i.e. the same fields shifted by BUF_SHIFT
back_view:
        ld iy,EN_BASE
        ld a,(front_page)
        or a
        ret nz                  ; front 1 -> hidden page 0: no shift
        ld iy,EN_BASE+BUF_SHIFT
        ret

; restores the 20x20 area around every enemy's last position in the hidden page from the clean
; page; dying enemies (types 2, 3) are erased once per buffer and then freed
erase_enemies:
        call back_view
        ld ix,EN_BASE
        ld b,16
.l:     ld a,(ix+E_TYPE)
        or a
        jr z,.n
        ld a,(iy+E_OX)
        or a
        jr z,.dead
        and 0xFE                ; HMMM works on whole bytes (2 pixels)
        sub 2
        ld l,a
        ld h,0
        ld (cmd_sx),hl
        ld (cmd_dx),hl
        ld a,(iy+E_OY)
        sub 2
        ld l,a
        ld h,0
        push hl
        ld de,CLEAN_Y
        add hl,de
        ld (cmd_sy),hl
        pop hl
        ld de,(back_y)
        add hl,de
        ld (cmd_dy),hl
        ld hl,20
        ld (cmd_nx),hl
        ld (cmd_ny),hl
        xor a
        ld (cmd_arg),a
        ld a,0xD0               ; HMMM
        ld (cmd_cmd),a
        push bc
        call vdp_cmd
        pop bc
        ld (iy+E_OX),0          ; nothing of this enemy is left in the hidden page
.dead:  ld a,(ix+E_TYPE)
        cp 2
        jr nz,.t3
        ld (ix+E_TYPE),3        ; still has to be erased from the other page
        jr .n
.t3:    cp 3
        jr nz,.n
        ld (ix+E_TYPE),0        ; erased from both pages: the slot is free
.n:     inc ix
        inc iy
        djnz .l
        ret

; transparent copy (LMMM + TIMP) of the enemy frame from the sheet into the hidden page
draw_enemies:
        call back_view
        ld ix,EN_BASE
        ld b,16
.l:     ld a,(ix+E_TYPE)
        cp 1
        jr nz,.n
        ld a,(ix+E_X)
        ld (iy+E_OX),a
        ld l,a
        ld h,0
        ld (cmd_dx),hl
        ld a,(ix+E_Y)
        ld (iy+E_OY),a
        ld l,a
        ld h,0
        ld de,(back_y)
        add hl,de
        ld (cmd_dy),hl
        ld a,(ix+E_KIND)
        add a,a
        ld c,a
        ld a,(ix+E_FRAME)
        and 1
        or c                    ; (kind * 2 + frame) * 16 = x of the frame in the sheet
        add a,a
        add a,a
        add a,a
        add a,a
        ld l,a
        ld h,0
        ld (cmd_sx),hl
        ld hl,SHEET_Y+8
        ld (cmd_sy),hl
        ld hl,16
        ld (cmd_nx),hl
        ld (cmd_ny),hl
        xor a
        ld (cmd_arg),a
        ld a,0x98               ; LMMM, TIMP (colour 0 is transparent)
        ld (cmd_cmd),a
        push bc
        call vdp_cmd
        pop bc
.n:     inc ix
        inc iy
        djnz .l
        ret

; score (6 digits) and lives; score_dirty counts the renders still owed (one per buffer)
draw_hud:
        ld a,(score_dirty)
        or a
        ret z
        dec a
        ld (score_dirty),a
        ld e,8
        ld a,(score+2)
        call .two
        ld a,(score+1)
        call .two
        ld a,(score+0)
        call .two
        ld e,120
        ld a,(room_bcd)
        call .two
        ld e,152
        ld a,(p_weapon)
        inc a
        call put_digit          ; weapon number (1 = pistol) ...
        ld e,168
        ld a,(p_wunits)
        call .two               ; ... and its seconds left
        ld e,232
        ld a,(lives)
        and 0x0F
        jp put_digit
.two:   push af
        rrca
        rrca
        rrca
        rrca
        and 0x0F
        call put_digit
        pop af
        and 0x0F
        jp put_digit

; A = digit, E = x; clears the 8x8 cell, draws the glyph, E += 8 (into the hidden page)
put_digit:
        push bc
        push de
        push af
        ld d,0
        ld (cmd_dx),de
        ld hl,(back_y)
        ld de,4
        add hl,de
        ld (cmd_dy),hl
        ld hl,8
        ld (cmd_nx),hl
        ld (cmd_ny),hl
        ld a,0xEE               ; HUD bar colour (14) in both nibbles
        ld (cmd_clr),a
        xor a
        ld (cmd_arg),a
        ld a,0xC0               ; HMMV
        ld (cmd_cmd),a
        call vdp_cmd
        pop af
        add a,a
        add a,a
        add a,a                 ; glyph x in the sheet = digit * 8
        ld l,a
        ld h,0
        ld (cmd_sx),hl
        ld hl,SHEET_Y
        ld (cmd_sy),hl
        xor a
        ld (cmd_arg),a
        ld a,0x98               ; LMMM, TIMP
        ld (cmd_cmd),a
        call vdp_cmd
        pop de
        pop bc
        ld a,e
        add a,8
        ld e,a
        ret

; A = display page (0/1): R#2 = $1F | page << 5
set_display_page:
        add a,a
        add a,a
        add a,a
        add a,a
        add a,a
        or 0x1F
        di
        out (VDP_CTRL),a
        ld a,0x82
        out (VDP_CTRL),a
        ei
        ret

; waits until the hidden page is finished, then for the next VBlank, and swaps pages and sprites
flip:
        xor a
        call set_border         ; idle colour for the profiler
        call vdp_wait
        halt                    ; the BIOS handler runs, we wake up at the start of the VBlank
        ld a,(front_page)
        xor 1
        ld (front_page),a
        call set_display_page
        call upload_sprites
        call upload_item_cols
        jp update_flash

; ---- state ----------------------------------------------------------------------
game_init:
        ld hl,RAM
        ld de,RAM+1
        ld bc,RAM_END-RAM-1
        ld (hl),0
        ldir
        ld a,120
        ld (p_x),a
        ld a,100
        ld (p_y),a
        ld a,3
        ld (lives),a
        ld a,2
        ld (score_dirty),a
        ld a,r
        ld (rng_acc),a
        ld a,(JIFFY)
        ld (last_jiffy),a
        ld a,1
        ld (room_bcd),a
        jp room_start           ; room 0

game_over:
        ld a,(front_page)       ; keep showing the same page; the renderer wipes both
        push af
        call game_init
        pop af
        ld (front_page),a
        ld a,2
        ld (restore_pending),a
        ret
