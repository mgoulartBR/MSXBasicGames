# Porting notes

## Inputs
Place the extracted Balatro source (`game.lua`, `card.lua`, `resources/…`) in `balatro-msx/original/` (git-ignored).
`tools/lua_data.py` parses `game.lua` tables; `tools/content.py` holds the curated content lists.

## Pipeline (`scripts/build.sh`)
1. `tools/gen_assets.py`: palette → atlas (VRAM-ready, ASCII-8 segments `seg_sN_b3.asm`), font strips, logo, tags.
2. `tools/gen_data.py`: `include/data_gen.h`, text/description segments. `tools/gen_music.py`: PSG tune.
3. MSXgl `node build.js` (SDCC 4.6.0) → ROM; `tools/memreport.py` prints budgets; SHA256 printed; MAP in `build/`.

## Architecture
- Bank 0–2 fixed (~11.6 KB code/rodata of 24 KB), bank 2 swaps `__banked` code segments 24–31; bank 3 swaps data (atlas 4–11, logo 12, tags 13, joker art 14–18 (+19, 23 reserved), text 20, descriptions 21); code segment 22 holds the per-joker scoring phases. Strings shared across segments live in SEG20 or fixed memory.
- VRAM: page 0 display; rows 212–227 tag icons; pages 1–2 atlas (cards, planets, tarots, vouchers, blind icons, font strips; 8 mapper segments). Page 3 is free.
- Joker art is **streamed**: 24x32 cards live in ROM segments (21 per 8 KB segment, 384 bytes each, up to 7 segments = 147 jokers) and are blitted straight to the screen with HMMC by `Vid_Joker()`; nothing of it is kept in VRAM.
- Rendering: HMMV/HMMM (even x/width) where possible, LMMV/LMMM otherwise; targeted redraw per widget/slot; display disabled during screen switches.
- Scoring produces an event list (chips u32, mult ×100) replayed as animation. State in a single `Game g`.
- Input: keyboard, joystick, MSX mouse (relative deltas, clamped cursor sprite) unified into widgets with focus; hover = focus, click = confirm.

## Tests (`scripts/test.sh`)
37 shared cases run on host (ASan/UBSan) **and** on Z80 in a self-test ROM (results compared); bot-driven simulations;
openMSX Tcl smoke tests reading a RAM beacon (flow, shop, packs, tags, animation, pointer); `tests/mouse_test.sh`
drives the emulated mouse with xdotool.

## Known toolchain bug
SDCC 4.6.0 miscompiles chained stores of u32 fields into a volatile array; worked around with one store per call (`bset()`).

## Save
`src/platform/save.c`: the whole run is the single `Game g` struct (~600 bytes), so a save is that struct plus an 8-byte header
(magic, version, screen, length, Fletcher checksum), written header-last. It lives in the cartridge SRAM of an ASCII8 + SRAM mapper
(Koei-style; openMSX `-romtype ASCII8SRAM2`): writing the SRAM-enable segment number (probed from 0x20, 0x40, 0x80, 0x10) to the bank-3
register maps the SRAM at 0xA000. On a plain ROM the probe fails and saving is simply off.
`ui_update` checksums `g` every 32 frames and writes when it changed and the screen is idle (blind select, shop, or round waiting for input;
not during scoring animations or packs). Game over / win erases the save. The title screen offers Continue / New Run.
Tested in openMSX only (`tests/tcl/smoke_save.tcl` resets the emulated machine and continues); the SRAM persists in openMSX's persistent dir.
