# Porting notes

## Inputs
Place the extracted Balatro source (`game.lua`, `card.lua`, `resources/…`) in `balatro-msx/original/` (git-ignored).
`tools/lua_data.py` parses `game.lua` tables; `tools/content.py` holds the curated content lists.

## Pipeline (`scripts/build.sh`)
1. `tools/gen_assets.py`: palette → atlas (VRAM-ready, ASCII-8 segments `seg_sN_b3.asm`), font strips, logo, tags.
2. `tools/gen_data.py`: `include/data_gen.h`, text/description segments. `tools/gen_music.py`: PSG tune.
3. MSXgl `node build.js` (SDCC 4.6.0) → ROM; `tools/memreport.py` prints budgets; SHA256 printed; MAP in `build/`.

## Architecture
- Bank 0–2 fixed (~11.6 KB code/rodata of 24 KB), bank 2 swaps `__banked` code segments 24–31; bank 3 swaps data (atlas 4–15, logo 16, tags 17, text 20, descriptions 21). Strings shared across segments live in SEG20 or fixed memory.
- VRAM: page 0 display; rows 212–227 tag icons; pages 1–3 card/joker atlas; BIOS sprite tables at the end.
- Rendering: HMMV/HMMM (even x/width) where possible, LMMV/LMMM otherwise; targeted redraw per widget/slot; display disabled during screen switches.
- Scoring produces an event list (chips u32, mult ×100) replayed as animation. State in a single `Game g`.
- Input: keyboard, joystick, MSX mouse (relative deltas, clamped cursor sprite) unified into widgets with focus; hover = focus, click = confirm.

## Tests (`scripts/test.sh`)
37 shared cases run on host (ASan/UBSan) **and** on Z80 in a self-test ROM (results compared); bot-driven simulations;
openMSX Tcl smoke tests reading a RAM beacon (flow, shop, packs, tags, animation, pointer); `tests/mouse_test.sh`
drives the emulated mouse with xdotool.

## Known toolchain bug
SDCC 4.6.0 miscompiles chained stores of u32 fields into a volatile array; worked around with one store per call (`bset()`).
