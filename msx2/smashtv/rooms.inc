; ---------------------------------------------------------------------------
; rooms.inc - room scripts (bytecode, see script.inc for the opcodes).
; Everything here is original content for the skeleton, not data from the SMS game.
; ---------------------------------------------------------------------------
ROOM_COUNT      equ 4

room_table:
        dw room1, room2, room3, room4

; room 1: a trickle of grunts
room1:  db OP_DROP, ITEM_SPREAD, 200, 150
        db OP_WAIT, 10
        db OP_LOOP, 4
        db OP_SPAWN, KIND_GRUNT
        db OP_WAIT, 6
        db OP_ENDLOOP
        db OP_WAITCLEAR, 0xFF
        db OP_EXIT_OPEN
        db OP_END

; room 2: three pairs of grunts, each closed by a runner, using a sub-wave
room2:  db OP_DROP, ITEM_RAPID, 40, 150
        db OP_WAIT, 8
        db OP_LOOP, 3
        db OP_CALL
        dw wave_pair
        db OP_WAIT, 14
        db OP_ENDLOOP
        db OP_WAITCLEAR, 0xFF
        db OP_EXIT_OPEN
        db OP_END

wave_pair:
        db OP_LOOP, 2
        db OP_SPAWN, KIND_GRUNT
        db OP_WAIT, 3
        db OP_ENDLOOP
        db OP_SPAWN, KIND_RUNNER
        db OP_RET

; room 3: five waves, each picked at random from three sub-waves
room3:  db OP_DROP, ITEM_PIERCE, 200, 60
        db OP_DROP, ITEM_SHIELD, 40, 60
        db OP_WAIT, 8
        db OP_LOOP, 5
        db OP_CALLR, 3
        dw wave_grunts, wave_runners, wave_tank
        db OP_WAIT, 16
        db OP_ENDLOOP
        db OP_WAITCLEAR, 0xFF
        db OP_EXIT_OPEN
        db OP_END

wave_grunts:
        db OP_LOOP, 3
        db OP_SPAWN, KIND_GRUNT
        db OP_WAIT, 2
        db OP_ENDLOOP
        db OP_RET

wave_runners:
        db OP_SPAWN, KIND_RUNNER
        db OP_WAIT, 4
        db OP_SPAWN, KIND_RUNNER
        db OP_RET

wave_tank:
        db OP_SPAWN, KIND_TANK
        db OP_RET

; room 4: tanks in the corners, then runners until everything is dead
room4:  db OP_DROP, ITEM_BOMB, 120, 150
        db OP_DROP, ITEM_LIFE, 60, 100
        db OP_WAIT, 6
        db OP_SPAWNAT, KIND_TANK, 20, 28
        db OP_SPAWNAT, KIND_TANK, 220, 28
        db OP_WAIT, 20
        db OP_LOOP, 3
        db OP_SPAWNAT, KIND_RUNNER, 20, 160
        db OP_SPAWNAT, KIND_RUNNER, 220, 160
        db OP_WAIT, 12
        db OP_ENDLOOP
        db OP_WAITCLEAR, 0xFF
        db OP_EXIT_OPEN
        db OP_END
