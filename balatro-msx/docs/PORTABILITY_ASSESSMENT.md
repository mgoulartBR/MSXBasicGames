# Portability assessment

| Aspect | Original (LÖVE/Lua, 1920×1080+, GPU shaders) | MSX2 target | Verdict |
|---|---|---|---|
| Rules/logic | ~5k lines Lua, tables of 150 jokers | pure integer C, 88 jokers | feasible (partial) |
| Graphics | shaders, 2x/1x atlases, 71×95 cards | 256×212×16, 128 KB VRAM | redrawn/converted: cards 28×38 composed from glyphs, jokers cropped to 20×28 |
| Colour | RGBA | 16 of 512, 3-bit/channel | k-means palette (9 anchors + 7 clustered) |
| CPU | modern | Z80 3.58 MHz | integer maths, VDP commands; transitions up to ~0.5 s |
| Memory | GBs | 16 KB RAM (+ mapper ROM 256 KB) | ~1.3 KB static RAM; bank-switched code/data |
| Input | mouse/touch/pad | mouse, joystick, keys | unified widget/focus layer |
| Audio | OGG | PSG 3 channels | original chiptune loop + SFX (the real OST is not used) |

Assets "to be printed on screen": yes, all were adjusted — resized, palette-reduced, tiled in VRAM pages 1–3, text
re-rendered in a 6×~9 font strip, UI redrawn with flat rects. Details in `docs/PORTING.md`.
Risk: proprietary content (see LICENSES.md); balance and visual fidelity are approximations.
