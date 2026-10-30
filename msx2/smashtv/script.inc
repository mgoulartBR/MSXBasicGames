; ---------------------------------------------------------------------------
; script.inc - room script machine
;
; Each room is a small bytecode program (see rooms.inc).  The machine is modelled on the SMS
; original's: one-byte opcodes with inline operands, a return-address stack for sub-waves, a
; loop counter, tick based waits and "wait until the room is clear" - but with an opcode set of
; its own.  script_step runs once per game tick; it executes opcodes until one blocks.
;
;   00 END                  stop the script
;   01 WAIT n               resume n ticks later
;   02 SPAWN kind           one enemy at a random arena edge (blocks while the pool is full)
;   03 SPAWNAT kind,x,y     one enemy at a fixed position
;   04 WAITCLEAR kind       block until no live enemy of that kind remains ($FF = any kind)
;   05 LOOP n               repeat the body up to the matching ENDLOOP n times (nesting: 4)
;   06 ENDLOOP
;   07 CALL addr16          sub-script, ends with RET (nesting: 4)
;   08 RET
;   09 JUMP addr16
;   0A CALLR n,addr16*n     CALL one of n sub-scripts, picked at random
;   0B EXIT_OPEN            opens the door in the top wall; touching it starts the next room
;   0C DROP item,x,y        puts a floor item in the room (types in items.inc)
;
; A failing opcode (stack overflow/underflow, unknown opcode) ends the script instead of
; running wild, and at most OPS_PER_TICK opcodes run per tick, so a bad script cannot hang the game.
; ---------------------------------------------------------------------------
OP_END          equ 0
OP_WAIT         equ 1
OP_SPAWN        equ 2
OP_SPAWNAT      equ 3
OP_WAITCLEAR    equ 4
OP_LOOP         equ 5
OP_ENDLOOP      equ 6
OP_CALL         equ 7
OP_RET          equ 8
OP_JUMP         equ 9
OP_CALLR        equ 10
OP_EXIT_OPEN    equ 11
OP_DROP         equ 12
OP_COUNT        equ 13
OPS_PER_TICK    equ 32

script_step:
        ld a,(scr_done)
        or a
        ret nz
        ld a,(scr_wait)
        or a
        jr z,.run
        dec a
        ld (scr_wait),a
        ret nz
.run:   ld b,OPS_PER_TICK
.next:  push bc
        call script_op
        pop bc
        or a
        ret nz                  ; the opcode blocked
        djnz .next
        ret

; executes one opcode; A = 0 to go on, nonzero when it blocks (the tick is over)
script_op:
        ld hl,(scr_pc)
        ld a,(hl)
        inc hl
        cp OP_COUNT
        jp nc,op_fail
        add a,a
        ld e,a
        ld d,0
        push hl
        ld hl,op_table
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        pop hl
        ex de,hl                ; HL = handler, DE = operands
        jp (hl)

op_table:
        dw op_end, op_wait, op_spawn, op_spawnat, op_waitclear, op_loop
        dw op_endloop, op_call, op_ret, op_jump, op_callr, op_exit_open, op_drop

; handlers get DE = address of the operands and return A (0 = continue, 1 = block)
op_fail:
op_end: ld a,1
        ld (scr_done),a
        ret

advance:                        ; DE = next opcode
        ld (scr_pc),de
        xor a
        ret

op_wait:
        ld a,(de)
        inc de
        ld (scr_wait),a
        ld (scr_pc),de
        ld a,1
        ret

op_spawn:
        ld a,(de)
        inc de
        ld c,a
        push de
        call find_enemy_slot
        jr nz,.full
        call init_enemy
        call random_edge_pos
        pop de
        jp advance
.full:  pop de                  ; no free slot: stay on this opcode and retry next tick
        ld a,1
        ret

op_spawnat:
        ld a,(de)
        inc de
        ld c,a
        ld a,(de)
        inc de
        ld h,a                  ; x
        ld a,(de)
        inc de
        ld l,a                  ; y
        push de
        push hl
        call find_enemy_slot
        jr nz,.full
        call init_enemy
        pop hl
        ld (ix+E_X),h
        ld (ix+E_Y),l
        pop de
        jp advance
.full:  pop hl
        pop de
        ld a,1
        ret

op_waitclear:
        ld a,(de)
        inc de
        ld c,a
        ld ix,EN_BASE
        ld b,16
.l:     ld a,(ix+E_TYPE)
        cp 1
        jr nz,.n
        ld a,c
        cp 0xFF
        jr z,.blocked
        cp (ix+E_KIND)
        jr z,.blocked
.n:     inc ix
        djnz .l
        jp advance
.blocked:
        ld a,1
        ret

op_loop:
        ld a,(de)
        inc de
        ld c,a                  ; count
        ld a,(lp_depth)
        cp 4
        jp nc,op_fail
        add a,a
        add a,a
        ld l,a
        ld h,0
        push de
        ld de,lp_stack
        add hl,de
        pop de
        ld (hl),e               ; body starts right after the operand
        inc hl
        ld (hl),d
        inc hl
        ld (hl),c
        ld hl,lp_depth
        inc (hl)
        jp advance

op_endloop:
        ld a,(lp_depth)
        or a
        jp z,op_fail
        dec a
        add a,a
        add a,a
        ld l,a
        ld h,0
        push de
        ld de,lp_stack
        add hl,de
        pop de
        inc hl
        inc hl                  ; -> count
        dec (hl)
        jr z,.done
        dec hl
        ld d,(hl)
        dec hl
        ld e,(hl)               ; back to the body
        jp advance
.done:  ld hl,lp_depth
        dec (hl)
        jp advance

op_call:
        ld a,(de)
        inc de
        ld c,a
        ld a,(de)
        inc de
        ld b,a                  ; BC = target, DE = return address
do_call:
        ld a,(scr_depth)
        cp 4
        jp nc,op_fail
        add a,a
        ld l,a
        ld h,0
        push de
        ld de,scr_stack
        add hl,de
        pop de
        ld (hl),e
        inc hl
        ld (hl),d
        ld hl,scr_depth
        inc (hl)
        ld (scr_pc),bc
        xor a
        ret

op_ret:
        ld a,(scr_depth)
        or a
        jp z,op_fail
        dec a
        ld (scr_depth),a
        add a,a
        ld l,a
        ld h,0
        ld de,scr_stack
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        jp advance

op_jump:
        ld a,(de)
        inc de
        ld c,a
        ld a,(de)
        ld b,a
        ld (scr_pc),bc
        xor a
        ret

op_callr:
        ld a,(de)
        inc de
        or a
        jp z,op_fail
        ld c,a                  ; C = n
        ld l,a
        ld h,0
        add hl,hl
        add hl,de               ; HL = return address, just after the n table entries
        push hl
        push de                 ; table start
        call rng
.mod:   cp c
        jr c,.ok
        sub c
        jr .mod
.ok:    add a,a
        ld l,a
        ld h,0
        pop de
        add hl,de
        ld c,(hl)
        inc hl
        ld b,(hl)               ; BC = chosen sub-script
        pop de                  ; DE = return address
        jr do_call

op_drop:
        ld a,(de)
        inc de
        ld c,a                  ; item type
        ld a,(de)
        inc de
        ld h,a                  ; x
        ld a,(de)
        inc de
        ld l,a                  ; y
        push de
        ld d,h
        ld e,l
        ld b,255                ; stays 12 seconds
        call item_spawn
        pop de
        jp advance

op_exit_open:
        ld a,1
        ld (exit_open),a
        push de
        ld a,7                  ; yellow gap in the top wall
        call set_door
        pop de
        jp advance

; ---- rooms ------------------------------------------------------------------------
; the door gap (A = colour) changes in the clean page now and in each display page when it is
; hidden (draw_door, called by the renderer), so the page on screen never changes under the beam
set_door:
        ld (door_color),a
        ld a,2
        ld (door_dirty),a
        ld hl,16+CLEAN_Y
        ld (cmd_dy),hl
        jr door_rect

draw_door:
        ld a,(door_dirty)
        or a
        ret z
        dec a
        ld (door_dirty),a
        ld hl,(back_y)
        ld de,16
        add hl,de
        ld (cmd_dy),hl
door_rect:
        ld a,(door_color)
        ld b,a
        add a,a
        add a,a
        add a,a
        add a,a
        or b
        ld (cmd_clr),a
        ld hl,DOOR_X
        ld (cmd_dx),hl
        ld hl,32
        ld (cmd_nx),hl
        ld hl,8
        ld (cmd_ny),hl
        xor a
        ld (cmd_arg),a
        ld a,0xC0               ; HMMV
        ld (cmd_cmd),a
        jp vdp_cmd

; starts the room in room_idx / room_bcd: live enemies are retired (they are erased from both
; pages by the renderer), bullets cleared, door closed, script rewound, player back at the start
room_start:
        ld ix,EN_BASE
        ld b,16
.e:     ld a,(ix+E_TYPE)
        cp 1
        jr nz,.n
        ld (ix+E_TYPE),2
.n:     inc ix
        djnz .e
        ld hl,BUL_BASE
        ld de,BUL_BASE+1
        ld bc,0x4F
        ld (hl),0
        ldir
        ld hl,IT_BASE           ; floor items stay behind too
        ld de,IT_BASE+1
        ld bc,0x3F
        ld (hl),0
        ldir
        ld a,1
        ld (item_col_dirty),a
        xor a
        ld (exit_open),a
        ld (scr_done),a
        ld (scr_wait),a
        ld (scr_depth),a
        ld (lp_depth),a
        ld a,3                  ; wall colour
        call set_door
        ld a,(room_idx)
        add a,a
        ld e,a
        ld d,0
        ld hl,room_table
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        ld (scr_pc),de
        ld a,120
        ld (p_x),a
        ld a,140
        ld (p_y),a
        ld a,20
        ld (p_inv),a            ; a short grace period
        ld a,2
        ld (score_dirty),a
        ret

room_next:
        ld a,(room_idx)
        inc a
        cp ROOM_COUNT
        jr c,.nowrap
        xor a                   ; after the last room, start over
        ld (room_idx),a
        ld a,1
        ld (room_bcd),a
        jp room_start
.nowrap:
        ld (room_idx),a
        ld a,(room_bcd)
        add a,1
        daa
        ld (room_bcd),a
        jp room_start

; the exit is the gap in the top wall: the player (16 px wide) has to be inside it
check_exit:
        ld a,(exit_open)
        or a
        ret z
        ld a,(p_y)
        cp ARENA_Y0+1
        ret nc
        ld a,(p_x)
        sub DOOR_X
        cp 32-16+1
        ret nc
        jr room_next
