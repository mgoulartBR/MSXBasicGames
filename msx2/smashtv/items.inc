; ---------------------------------------------------------------------------
; items.inc - weapons, enemy kills, drops and floor items
;
; Weapons (p_weapon), picked up from items and lasting p_wunits seconds (BCD) before the
; pistol comes back, like the power-up timer of the SMS original:
;   0 pistol   one shot every 2 ticks
;   1 spread   three shots in a fan, one every 3 ticks
;   2 rapid    a shot every tick
;   3 piercing damage 2, goes on through the enemies it kills, one every 3 ticks
; Items (sprite planes 17..22): 1-3 weapons, 4 extra life, 5 shield, 6 bomb, 7 prize (+100).
; ---------------------------------------------------------------------------

; cooldown (ticks between shots - 1), damage, piercing, spread
weapon_tab:
        db 1, 1, 0, 0
        db 2, 1, 0, 1
        db 0, 1, 0, 0
        db 2, 2, 1, 0

ring_dir:                       ; the 8 directions clockwise from up
        db DIR_UP, DIR_UP|DIR_RIGHT, DIR_RIGHT, DIR_DOWN|DIR_RIGHT
        db DIR_DOWN, DIR_DOWN|DIR_LEFT, DIR_LEFT, DIR_UP|DIR_LEFT
ring_idx:                       ; direction bits (0..10) -> index in ring_dir, $FF = not a direction
        db 0xFF, 0, 4, 0xFF, 6, 7, 5, 0xFF, 2, 1, 3

; chance (out of 256) that a killed enemy drops something, per kind; and what it can drop
drop_chance:
        db 30, 45, 255
drop_table:
        db 1, 2, 3, 4, 5, 6, 7, 7, 7, 1, 2, 3, 5, 7, 6, 4

; B = direction bits of the shot (the player's aim)
fire_weapon:
        ld a,(p_weapon)
        add a,a
        add a,a
        ld e,a
        ld d,0
        ld hl,weapon_tab
        add hl,de
        ld a,(hl)
        inc a
        ld (p_cool),a           ; the tick decrements it before the next check
        inc hl
        ld a,(hl)
        ld (w_dmg),a
        inc hl
        ld a,(hl)
        ld (w_pierce),a
        inc hl
        ld a,(hl)
        push af                 ; spread?
        call spawn_bullet
        pop af
        or a
        ret z
        ld a,b
        cp 11
        ret nc
        ld hl,ring_idx
        add a,l
        ld l,a
        jr nc,.i
        inc h
.i:     ld a,(hl)
        cp 0xFF
        ret z                   ; not one of the 8 directions: straight shot only
        ld d,a                  ; D = ring index of the aim
        dec a
        and 7
        call dir_of_idx
        ld b,a
        call spawn_bullet
        ld a,d
        inc a
        and 7
        call dir_of_idx
        ld b,a
        jp spawn_bullet

dir_of_idx:                     ; A = ring index -> A = direction bits (keeps D, E)
        ld hl,ring_dir
        add a,l
        ld l,a
        jr nc,.n
        inc h
.n:     ld a,(hl)
        ret

; weapon countdown, once per tick
weapon_tick:
        ld a,(p_weapon)
        or a
        ret z
        ld hl,p_wsub
        dec (hl)
        ret nz
        ld (hl),20
        ld a,(p_wunits)
        sub 1
        daa
        ld (p_wunits),a
        ld a,2
        ld (score_dirty),a
        ld a,(p_wunits)
        or a
        ret nz
        ld (p_weapon),a         ; time is up: back to the pistol
        ret

; ---- kills and drops ----------------------------------------------------------------
; IX = enemy that has just been killed: it starts dying, scores, and may drop an item
kill_enemy:
        push bc
        ld (ix+E_TYPE),2
        call score_kill
        call maybe_drop
        pop bc
        ret

score_kill:                     ; adds the kind's points
        ld a,(ix+E_KIND)
        ld e,a
        ld d,0
        ld hl,kind_points
        add hl,de
        ld a,(hl)
        jp add_score

maybe_drop:
        ld a,(ix+E_KIND)
        ld e,a
        ld d,0
        ld hl,drop_chance
        add hl,de
        ld c,(hl)
        call rng
        cp c
        ret nc
        call rng
        and 15
        ld e,a
        ld d,0
        ld hl,drop_table
        add hl,de
        ld c,(hl)               ; item type
        ld d,(ix+E_X)
        ld e,(ix+E_Y)
        ld b,120                ; ticks on the floor
        ; fall through

; C = type, D = x, E = y, B = ticks to live. Silently does nothing when the 6 slots are taken.
item_spawn:
        push iy
        push bc
        ld iy,IT_BASE
        ld b,6
.f:     ld a,(iy+I_TYPE)
        or a
        jr z,.free
        inc iy
        djnz .f
        pop bc
        pop iy
        ret
.free:  pop bc
        ld (iy+I_TYPE),c
        ld (iy+I_X),d
        ld (iy+I_Y),e
        ld (iy+I_LIFE),b
        ld a,1
        ld (item_col_dirty),a
        pop iy
        ret

; ---- floor items ----------------------------------------------------------------------
update_items:
        ld iy,IT_BASE
        ld b,6
.l:     ld a,(iy+I_TYPE)
        or a
        jr z,.n
        dec (iy+I_LIFE)
        jr nz,.alive
        ld (iy+I_TYPE),0        ; nobody took it
        jr .dirty
.alive: ld a,(p_x)
        sub (iy+I_X)
        jr nc,.a1
        neg
.a1:    cp 12
        jr nc,.n
        ld a,(p_y)
        sub (iy+I_Y)
        jr nc,.a2
        neg
.a2:    cp 12
        jr nc,.n
        ld a,(iy+I_TYPE)
        ld (iy+I_TYPE),0
        push bc
        call pickup_effect
        pop bc
.dirty: ld a,1
        ld (item_col_dirty),a
.n:     inc iy
        djnz .l
        ret

pickup_effect:                  ; A = item type
        cp ITEM_LIFE
        jr c,.weapon
        jr z,.life
        cp ITEM_SHIELD
        jr z,.shield
        cp ITEM_BOMB
        jp z,smash
        jp add_score100         ; prize
.weapon:
        ld (p_weapon),a         ; weapon number = item type
        ld a,0x15               ; 15 seconds
        ld (p_wunits),a
        ld a,20
        ld (p_wsub),a
        jr .hud
.life:  ld a,(lives)
        cp 9
        jr nc,.hud
        inc a
        ld (lives),a
        jr .hud
.shield:
        ld a,100
        ld (p_inv),a
.hud:   ld a,2
        ld (score_dirty),a
        ret

; the bomb: every live enemy dies (and scores, but drops nothing), the floor flashes
smash:
        ld ix,EN_BASE
        ld b,16
.l:     ld a,(ix+E_TYPE)
        cp 1
        jr nz,.n
        ld (ix+E_TYPE),2
        push bc
        call score_kill
        pop bc
.n:     inc ix
        djnz .l
        ld a,2
        ld (flash_timer),a
        ret

add_score100:
        ld hl,score+1
        ld a,(hl)
        add a,1
        daa
        ld (hl),a
        inc hl
        ld a,(hl)
        adc a,0
        daa
        ld (hl),a
        ld a,2
        ld (score_dirty),a
        ret

; ---- video side ---------------------------------------------------------------------------
; rebuilds the colour table of the item planes when an item appeared or went away
build_item_cols:
        ld a,(item_col_dirty)
        or a
        ret z
        xor a
        ld (item_col_dirty),a
        inc a
        ld (item_col_upload),a
        ld iy,IT_BASE
        ld de,item_cols_ram
        ld b,6
.l:     ld a,(iy+I_TYPE)
        or a
        jr z,.blank
        dec a
        ld l,a
        ld h,0
        add hl,hl
        add hl,hl
        add hl,hl
        add hl,hl               ; (type - 1) * 16
        push bc
        ld bc,item_cols
        add hl,bc
        ld bc,16
        ldir
        pop bc
        jr .n
.blank: ld c,16
        xor a
.z:     ld (de),a
        inc de
        dec c
        jr nz,.z
.n:     inc iy
        djnz .l
        ret

; during the VBlank (called by flip)
upload_item_cols:
        ld a,(item_col_upload)
        or a
        ret z
        xor a
        ld (item_col_upload),a
        ld hl,SPR_COL_VRAM+17*16
        call set_vram_wr
        ld hl,item_cols_ram
        ld b,96
        ld c,VDP_DATA
        otir
        ret

; bomb flash: palette entry 2 (the floor) is white for two flips
update_flash:
        ld a,(flash_timer)
        or a
        ret z
        dec a
        ld (flash_timer),a
        ld hl,pal_flash
        jr nz,.w
        ld hl,pal_floor
.w:     di
        ld a,2
        out (VDP_CTRL),a
        ld a,0x90               ; R#16 = 2
        out (VDP_CTRL),a
        ld b,2
        ld c,VDP_PAL
        otir
        ei
        ret
pal_flash:
        db 0x77, 0x07
pal_floor:
        db 0x13, 0x01           ; same as palette entry 2 in gfx.inc
