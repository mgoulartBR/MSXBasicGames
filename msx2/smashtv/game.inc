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
        call spawn_bullet       ; B = direction bits
        ld a,1
        ld (p_cool),a
        ret

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
.free:  ld (ix+B_TYPE),1
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

spawn_enemy:
        ld a,(spawn_timer)
        or a
        jr z,.try
        dec a
        ld (spawn_timer),a
        ret
.try:   ld ix,EN_BASE
        ld b,MAX_ENEMIES
.find:  ld a,(ix+E_TYPE)
        or a
        jr z,.free
        inc ix
        djnz .find
        ret
.free:  ld (ix+E_TYPE),1
        ld (ix+E_HP),1
        ld (ix+E_OX),0
        ld (ix+E_FRAME),0
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
        jr .set
.top:   ld (ix+E_Y),ARENA_Y0
        call rand_x
        ld (ix+E_X),a
        jr .set
.bottom:
        ld (ix+E_Y),ARENA_Y1-16
        call rand_x
        ld (ix+E_X),a
        jr .set
.left:  ld (ix+E_X),ARENA_X0
        call rand_y
        ld (ix+E_Y),a
.set:   ld a,15
        ld (spawn_timer),a
        ret

; grunts walk toward the player, 2 px per tick, direction bits kept in E_DIR
update_enemies:
        ld ix,EN_BASE
        ld b,16
.loop:  ld a,(ix+E_TYPE)
        cp 1
        jr nz,.next
        ld d,0
        ld a,(p_x)
        ld c,a
        ld a,(ix+E_X)
        cp c
        jr z,.v
        jr c,.right
        sub ENEMY_SPEED
        set 2,d
        jr .xs
.right: add a,ENEMY_SPEED
        set 3,d
.xs:    ld (ix+E_X),a
.v:     ld a,(p_y)
        ld c,a
        ld a,(ix+E_Y)
        cp c
        jr z,.dir
        jr c,.down
        sub ENEMY_SPEED
        set 0,d
        jr .ys
.down:  add a,ENEMY_SPEED
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
add_score:                      ; +10 points, 3 byte BCD like the original
        push af
        push hl
        ld hl,score
        ld a,(hl)
        add a,0x10
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
        ld a,1
        ld (score_dirty),a
        pop hl
        pop af
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
        ld (iy+B_TYPE),0        ; hit: the bullet is consumed
        dec (ix+E_HP)
        jr nz,.done
        ld (ix+E_TYPE),2        ; dying: erased and freed by the renderer
        call add_score
.done:  pop bc
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
        ld a,1
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
render:
        call build_sprites
        call upload_sprites
        call erase_enemies
        call draw_enemies
        jp draw_hud

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
        ld b,15
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

; restore the 20x20 area around every enemy's last drawn position from the clean page
erase_enemies:
        ld ix,EN_BASE
        ld b,16
.l:     ld a,(ix+E_TYPE)
        or a
        jr z,.n
        ld a,(ix+E_OX)
        or a
        jr z,.n
        and 0xFE                ; HMMM works on whole bytes (2 pixels)
        sub 2
        ld l,a
        ld h,0
        ld (cmd_sx),hl
        ld (cmd_dx),hl
        ld a,(ix+E_OY)
        sub 2
        ld l,a
        ld h,0
        ld (cmd_dy),hl
        ld de,CLEAN_Y
        add hl,de
        ld (cmd_sy),hl
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
.n:     inc ix
        djnz .l
        ret

; transparent copy (LMMM + TIMP) of the enemy frame from the sheet; frees dying enemies
draw_enemies:
        ld ix,EN_BASE
        ld b,16
.l:     ld a,(ix+E_TYPE)
        cp 1
        jr z,.draw
        cp 2
        jr nz,.n
        ld (ix+E_TYPE),0
        ld (ix+E_OX),0
        jr .n
.draw:  ld a,(ix+E_X)
        ld (ix+E_OX),a
        ld l,a
        ld h,0
        ld (cmd_dx),hl
        ld a,(ix+E_Y)
        ld (ix+E_OY),a
        ld l,a
        ld h,0
        ld (cmd_dy),hl
        ld a,(ix+E_FRAME)
        and 1
        add a,a
        add a,a
        add a,a
        add a,a                 ; frame * 16
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
        djnz .l
        ret

; score (6 digits) and lives, only redrawn when something changed
draw_hud:
        ld a,(score_dirty)
        or a
        ret z
        xor a
        ld (score_dirty),a
        ld e,8
        ld a,(score+2)
        call .two
        ld a,(score+1)
        call .two
        ld a,(score+0)
        call .two
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

; A = digit, E = x; clears the 8x8 cell, draws the glyph, E += 8
put_digit:
        push bc
        push de
        push af
        ld d,0
        ld (cmd_dx),de
        ld hl,4
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
        ld a,1
        ld (score_dirty),a
        ld a,r
        ld (rng_acc),a
        ld a,(JIFFY)
        ld (last_jiffy),a
        ret

game_over:
        call restore_all        ; wipe enemies that were blitted onto page 0
        call game_init
        ret
