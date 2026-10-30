; ---------------------------------------------------------------------------
; defs.inc - BIOS entries, VDP ports, VRAM layout, RAM map
; ---------------------------------------------------------------------------

; ---- MSX BIOS (main ROM, page 0) ----
CHGMOD          equ 0x005F      ; A = screen mode
WRTVDP          equ 0x0047      ; B = data, C = register
GTSTCK          equ 0x00D5      ; A = 0 cursors, 1/2 joystick -> A = 0..8 (clockwise from up)
GTTRIG          equ 0x00D8      ; A = 0 space, 1/2 joy A, 3/4 joy B -> A = 0xFF when pressed
SNSMAT          equ 0x0141      ; A = keyboard matrix row -> A (0 = pressed)
CHPUT           equ 0x00A2      ; A = character
MSXVER          equ 0x002D      ; 0 = MSX1, 1 = MSX2, 2 = MSX2+, 3 = turboR
JIFFY           equ 0xFC9E      ; incremented by the BIOS at every VBlank

; ---- VDP ports ----
VDP_DATA        equ 0x98
VDP_CTRL        equ 0x99
VDP_PAL         equ 0x9A
VDP_INDIR       equ 0x9B

; ---- VRAM layout (Screen 5: 128 bytes/line, 256 lines/page) ----
SPR_COL_VRAM    equ 0x7400      ; sprite colour table (16 bytes/plane)
SPR_ATT_VRAM    equ 0x7600      ; sprite attribute table (4 bytes/plane)
SPR_PAT_VRAM    equ 0x7800      ; sprite patterns (32 bytes per 16x16)
; Pages (256 lines each): 0 and 1 are the two display buffers (the sprite tables live in the
; unused lines of page 0 and do not move), 2 holds a clean copy of the arena, 3 the graphics sheet.
CLEAN_Y         equ 512         ; page 2: clean arena, used to erase blitted objects
SHEET_Y         equ 768         ; page 3: font + enemy frames
SPR_HIDDEN      equ 217         ; sprite Y that is never visible (not 216: that terminates the list)

; ---- game constants (pixels) ----
ARENA_X0        equ 16
ARENA_X1        equ 240
ARENA_Y0        equ 24
ARENA_Y1        equ 184
TICK_VBLANKS    equ 3           ; game logic at VBlank/3 (20 Hz NTSC), like the SMS original
PLAYER_SPEED    equ 3
BULLET_SPEED    equ 8
DOOR_X          equ 112         ; exit gap in the top wall, 32 px wide, lines 16..23
KIND_GRUNT      equ 0
KIND_RUNNER     equ 1
KIND_TANK       equ 2
STACK_TOP       equ 0xF300

; direction bits (same as the original game's +0x60 field)
DIR_UP          equ 1
DIR_DOWN        equ 2
DIR_LEFT        equ 4
DIR_RIGHT       equ 8

; ---- RAM (page 3) ----
RAM             equ 0xC000
last_jiffy      equ RAM+0
input_move      equ RAM+1       ; direction bits
input_aim       equ RAM+2       ; direction bits
input_fire      equ RAM+3
rng_acc         equ RAM+4
score           equ RAM+5       ; 3 bytes BCD, little endian
score_dirty     equ RAM+8
lives           equ RAM+9
p_x             equ RAM+10
p_y             equ RAM+11
p_inv           equ RAM+12      ; invulnerability ticks
p_cool          equ RAM+13      ; shot cooldown
p_face          equ RAM+14      ; last direction, used when only the fire button is held
hud_lives_old   equ RAM+16
profile_on      equ RAM+17      ; 1 = backdrop colour shows where the tick time goes (F1 toggles)
key_prev        equ RAM+18      ; F1 state at the previous tick (edge detection)
front_page      equ RAM+19      ; page being displayed (0/1); the other one is drawn into
back_y          equ RAM+20      ; word: first line of the page being drawn (0 or 256) - 2 bytes
p_weapon        equ RAM+25      ; 0 pistol, 1 spread, 2 rapid, 3 piercing
p_wunits        equ RAM+26      ; seconds of weapon left, BCD
p_wsub          equ RAM+27      ; ticks until the next second
w_dmg           equ RAM+28      ; damage of the shot being fired
w_pierce        equ RAM+29      ; 1 = the shot goes through the enemies it kills
flash_timer     equ RAM+30      ; bomb flash: flips left
item_col_dirty  equ RAM+31      ; item colour shadow must be rebuilt
item_col_upload equ RAM+0x30    ; ... and sent to the VDP at the next flip
restore_pending equ RAM+22      ; pages still to be wiped from the clean copy (game over)
door_color      equ RAM+23      ; colour of the exit gap in the top wall
door_dirty      equ RAM+24      ; pages still to be redrawn with door_color
cmd_buf         equ RAM+0x20    ; VDP command block: SX SY DX DY NX NY (words), CLR ARG CMD
cmd_sx          equ cmd_buf+0
cmd_sy          equ cmd_buf+2
cmd_dx          equ cmd_buf+4
cmd_dy          equ cmd_buf+6
cmd_nx          equ cmd_buf+8
cmd_ny          equ cmd_buf+10
cmd_clr         equ cmd_buf+12
cmd_arg         equ cmd_buf+13
cmd_cmd         equ cmd_buf+14
spr_shadow      equ RAM+0x40    ; 32 planes * 4 bytes (Y, X, pattern, reserved)

; Entity pools: 16 slots, struct of arrays with a $10 stride, as in the SMS original
; (slot address = base + slot, field = (IX + k)).
BUL_BASE        equ RAM+0x100
B_TYPE          equ 0x00        ; 0 = free
B_X             equ 0x10
B_Y             equ 0x20
B_DIR           equ 0x30
B_DMG           equ 0x40        ; damage; B_TYPE 1 = normal shot, 2 = piercing shot
EN_BASE         equ RAM+0x1C0
E_TYPE          equ 0x00        ; 0 free, 1 alive, 2 dying (erase here), 3 dying (erase in the other page), then free
E_HP            equ 0x10
E_X             equ 0x20
E_Y             equ 0x30
E_DIR           equ 0x40
E_OX            equ 0x50        ; position last drawn in page 0 (0 = not drawn yet)
E_OY            equ 0x60
E_FRAME         equ 0x70
BUF_SHIFT       equ -0x70       ; the same fields for page 1 live at E_OX+BUF_SHIFT, E_OY+BUF_SHIFT
                                ; (negative offsets: IX+d only reaches +127)
E_OXB           equ E_OX+BUF_SHIFT
E_OYB           equ E_OY+BUF_SHIFT
E_KIND          equ -0x30       ; 0 grunt, 1 runner, 2 tank

; items on the floor: 6 slots (sprite planes 17..22), struct of arrays
ITEM_SPREAD     equ 1           ; item types 1..3 are weapons: the type is the weapon number
ITEM_RAPID      equ 2
ITEM_PIERCE     equ 3
ITEM_LIFE       equ 4
ITEM_SHIELD     equ 5
ITEM_BOMB       equ 6
ITEM_PRIZE      equ 7
IT_BASE         equ RAM+0x250
I_TYPE          equ 0x00
I_X             equ 0x10
I_Y             equ 0x20
I_LIFE          equ 0x30        ; ticks left on the floor
item_cols_ram   equ RAM+0x2A0   ; 6 planes * 16 colour bytes, sent to the sprite colour table
RAM_END         equ RAM+0x300

; room script machine state (see script.inc)
scr_pc          equ RAM+0xC0    ; word: next opcode
scr_wait        equ RAM+0xC2    ; ticks left to wait
scr_done        equ RAM+0xC3    ; 1 = script finished (or failed)
scr_depth       equ RAM+0xC4    ; call stack depth (return addresses)
lp_depth        equ RAM+0xC5    ; loop stack depth
room_idx        equ RAM+0xC6    ; 0-based index into room_table
room_bcd        equ RAM+0xC7    ; number shown in the HUD
exit_open       equ RAM+0xC8
scr_stack       equ RAM+0xD0    ; 4 return addresses
lp_stack        equ RAM+0xD8    ; 4 entries of 4 bytes: body pointer (word), count, unused
