KAPIROTO - MSX1
A cruel little platformer in 20 rooms. Inspired by Level Devil (Unept).

STATUS
20 playable rooms. Every room was verified solvable by a native solver and then
completed inside openMSX (C-BIOS MSX1) by replaying the solver's solution.
The compiled ROM is kapiroto.rom (32 KB, MSX1, no mapper).

HOW TO BUILD THE ROM
1. Get https://github.com/aoineko-fr/MSXgl
   Reference commit: 6fd8858dce9a5a674529b78ec13d64262810eb34
2. Copy this folder (kapiroto) into MSXgl/projects/.
3. Open a terminal in MSXgl/projects/kapiroto.
   Windows: build.bat
   Linux/macOS: sh build.sh
4. MSXgl prints the path of kapiroto.rom in the deploy folder.
   Target: ROM_32K, MSX1, no mapper.
5. Test it in openMSX / WebMSX with an MSX1 machine.

REQUIREMENTS
Windows/Linux: the toolchain ships with the MSXgl repository.
macOS: Node.js and a compatible SDCC; adjust Compiler/Assembler/Linker/MakeLib.
The script uses Node from PATH or tools/build/Node/node inside MSXgl.
See https://aoineko.org/msxgl/index.php?title=Build_tool

CONTROLS
Left / Right arrows or A / D: walk.
Space: jump (the only jump key).
Joystick port 1: left/right to walk, button A or B to jump.
P or ESC: pause. R: restart the room. M: sound on/off.
Enter or jump: start / continue. After room 20 the game loops back to room 1.

HAZARDS
- Holes in the floor.
- Floor that crumbles when you step on it.
- Spikes that pop out of the floor as you get close.
- A block that drops from the ceiling as you get close.
- An exit door that runs away from you.
- Saw blades patrolling the floor or the air.
- Ceiling spikes that fall as you walk by. Once landed they become solid blocks
  you can stand on.
- A block that slides along the floor and shoves you (into a hole or a wall).
- A platform that moves every time you jump.

FILES
kapiroto.c         Input, drawing (SCREEN 1 tiles + hardware sprites), text and sound.
ld_logic.h         Game rules and the 20 rooms (g_Rooms table). No hardware access.
tools/solver.c     Native room solver: gcc -O2 -o solver tools/solver.c
                   "solver K [room]" searches a solution for each room, where K is
                   how many frames each key press is held (1 = exact, 4 or 8 =
                   human-like timing). Add a third argument to print the key sequence.
msxgl_config.h     MSXgl configuration (text mode, PSG, ...).
project_config.js  Build configuration (ROM_32K target, MSX1).

EDITING ROOMS
Each room is one line of g_Rooms in ld_logic.h with named fields (.gapX, .crackX,
.spikeX, .blockX, .platX/.platY/.platDX, .doorX/.doorMove, .saw, .drop, .push).
After changing a room, run the solver to make sure it is still possible.

SCOPE
SCREEN 1 (TMS9918), hardware sprites, keyboard/joystick and PSG.
Progress is kept only for the current session.

CREDITS
Inspired by Level Devil by Unept, https://unept.com/
MSXgl: Guillaume Aoineko Blanchard, https://github.com/aoineko-fr/MSXgl
Art and implementation: 8bit/lab.
Code and assets of this kit: CC BY-SA 4.0. See LICENSE.txt.
