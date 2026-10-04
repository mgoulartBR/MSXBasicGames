# Porting notes — The Republia Times → MSX

# PORTABILITY ASSESSMENT

## Jogo original
*The Republia Times* by **Lucas Pope** (@dukope): a one-screen "editor-in-chief" game. You pick
news items from a feed, drag them onto a 4×5 newspaper grid in three sizes, and the loyalty /
readership of the public (and the fate of your family) depends on what you print. Played over
3 + 2 + 4 "days" with a twist ending that swaps the government (Republia ⇄ Democria).

## Arquitetura original
ActionScript 3 + **Flixel** (Flash), 540×320 logical pixels shown at 2×, 60 fps. 19 `.as` files,
2096 lines. States: `MorningState` → `PlayState` → `NightState` → `MorningState`…
Pure game logic is isolated in `GameStatus`, `Readership`, `Goal`, `Day`, `PaperSummary`,
`NewsItem`; UI in `Feed`, `Paper`, `Clock`, `StatMeters`, `CenterPopup`, `Util`.

## Licença
No license file in the supplied material → see `LICENSES.md` (fan port, non-commercial, ask the
author before redistributing).

## Dependências
Flixel only (mouse, sprites, text, sound). Nothing else.

## Subsistemas
Mouse drag & drop, 60 s real-time day clock, random day generation, readership maths, story
text builder, analog clock + dial gauge drawn with `drawLine`, mp3 music/SFX, embedded fonts.

## Assets encontrados
* **71 news items** (`NewsItem.as`: day range, loyalty ±1/0, interesting flag, blurb, headline).
  Parsed *from the original source* by `tools/gen_assets.py` (the .as file stays the source of truth).
* 22 PNG (all 1-bit, 1 colour + transparent), 4 TTF pixel fonts, 11 MP3 (2 music loops
  12.8 s / 8.7 s, 9 short SFX incl. a silence file).
* Goals: 3 (`first-state` d3, `second-state` d5, `last-rebel` d10). 11 briefing days.

## Plataforma MSX recomendada
**MSX1** (TMS9918, Z80 @ 3.58 MHz, 16 KB VRAM, ≥ 16 KB RAM). Runs unchanged on MSX2/2+/turbo R
(Screen 2 is a subset of their VDP) — not separately tested beyond the emulator.

## Motivo
The art is monochrome pixel art and the game is text/menu driven: no scrolling, no sprites
needed, almost static screen. Nothing requires V9938 features, so the least demanding machine
wins (brief §6).

## Screen mode recomendado
**Screen 2 (Graphic 2), 256×192**, used as an *unique-tile bitmap*: the name table is the
identity (768 unique tiles), so every 8×8 cell has its own pattern/colour bytes and text can be
drawn pixel-accurately with proportional fonts. Colour is per 8×1 pixel row: black ink on white
paper ("newspaper"), inverse for selection, red for rebel messages, light green/red for the
placement footprint.

## Estratégia gráfica
Everything is composed in small RAM "canvases" laid out like VRAM pattern bytes (tile-major) and
blitted with one VRAM write per tile row (`src/platform/msx_gfx.c`). No frame buffer (RAM is
13 KB). Only changed regions are redrawn: single feed entries, single paper cells, colour table
only for selection/footprint (cheap).
Original 540×320 layout was redone for 256×192; images are converted at build time:
logo (native 250×41), masthead 140×23→128×21, press band 335×79→248×59, dial 52×52→40×40,
ministry building native. Clock and dial needle are drawn procedurally exactly like the original.

## Estratégia de sprites
None. The cursor/footprint is shown with colour attributes. (Hardware sprites would only add
the 4-sprite-per-line limit with no benefit here.)

## Estratégia de scroll
None needed (feed scrolls by whole entries, redrawn only when an entry arrives).

## Estratégia de áudio
PSG only (no expansion required). Music and SFX run **inside the VBlank hook (H.TIMI)**:
music on channels A+B at a 30 Hz sequencer tick (normalised for 50/60 Hz), SFX on channel C
with noise. The main loop only posts requests, so long redraws never stall audio.
* SFX: hand-written PSG step tables imitating the originals (click, drag, drop, feed, alarm, day over).
* Music: `tools/gen_music.py` **automatically transcribes** the two MP3 loops (dominant pitch in a
  melody and a bass band every 1/30 s). It is an approximation, *not* an arrangement; to be reviewed
  by ear (NOT tested on real hardware, only register activity verified in openMSX).

## Formato do executável
**ROM, ASCII8 mapper, 64 KB** (`republia-msx-0.1.rom`).

## Mapper recomendado
ASCII8 (8 KB segments, bank regs 6000h/6800h/7000h/7800h). Reason: code + data are ≈ 40 KB, more
than a 32 KB ROM. Plain 48 KB ROMs need page-0 mapping tricks; Konami mappers waste a bank on a
fixed segment-0 and need SCC-style registers. ASCII8 is supported by flash cartridges, openMSX
and MSXgl's default crt0 (initial segments 0-3 = linear 32 KB, so the code needs no banking):
```
seg 0-2 (4000h-9FFFh)  fixed: all code, font, SFX/music tables, small tables   ~20.1 KB
seg 3   (A000h window)  default window, spare
seg 4   news table + all 71 news strings                                       7.1 KB
seg 5   converted bitmaps                                                      7.0 KB
seg 6   Text_Morning / Text_Night code + story strings                         6.3 KB
seg 7   unused
```
Bank 3 (A000h) is the data window. Rule (`msx_seg.h`): play screen keeps SEG_NEWS mapped,
all other screens map SEG_GFX; text builders map SEG_TEXT only during the call and never return
pointers into it. The ISR touches fixed code/data only.

## Estimativa de ROM
Final: 64 KB image, ~41 KB used (fixed 20.1 KB + 7.1 + 7.0 + 6.3).

## Estimativa de RAM
Measured from the linker map: `.data/.bss` = **6.4 KB** (0C000h–0D8E3h): feed texts (2 KB),
message buffer (1.2 KB), canvases (~1.6 KB: article 864 B, strip 256 B, clock/dial ~600 B), state.
Stack uses the top of RAM (from HIMEM). ≥ 5 KB free.
VRAM: pattern 0000h–17FFh, name 1800h–1AFFh, (sprites unused), colour 2000h–37FFh.

## Principais riscos
1. Not tested on real hardware (no access): ASCII8 bank writes, H.TIMI hook inter-slot call,
   VDP timing on a real TMS9918 (MSXgl's 30-cycle VRAM loop is used).
2. Text-heavy redraw speed on 3.58 MHz (solved: asm glyph blitter, incremental redraws).
3. Music is an automatic transcription (may sound off).
4. Original licence unclear.

## Sistemas reutilizáveis
Rules, goal logic, day generation, readership maths, summary maths, all texts (ported 1:1 from
the AS3 code into `src/game/logic.c`, `texts.c`), news data (parsed).

## Sistemas que precisam ser reescritos
Rendering (Flixel sprites/text → tile canvases), input (mouse → pad/keyboard focus model),
audio (mp3 → PSG), fonts (TTF → 8-row proportional bitmaps), clock/dial (kept procedural).

## Plano incremental (and status)
| milestone | content | status |
|---|---|---|
| 0.1 | toolchain, build, openMSX headless + screenshots | DONE |
| 0.2 | asset pipeline (news, fonts, gfx, music) | DONE |
| 0.3 | canvas / text / screens (title, morning, night) | DONE |
| 0.4 | play screen: feed, paper, placement, clock, gauge | DONE |
| 0.5 | rules: day generation, readership, goals, endings | DONE (verified by debug-start tests) |
| 0.6 | audio (PSG SFX + music) | PARTIAL (music = automatic approximation) |
| 0.7 | ISR architecture, perf pass | DONE |
| 1.0 | real hardware verification | TODO |

---

# Decisions

## Video / Audio / Input / Memory / ROM mapper
See above. Input: keyboard (cursor keys, SPACE/RETURN = A, ESC/TAB = B, M = mute) **and**
joystick ports 1+2 (direction + two triggers) merged. Both are sampled in the VBlank hook so
short taps are never lost. Mouse is **not implemented** (TODO).

## Features preserved
* All 71 news items with original day ranges, loyalty effect and "interesting" flag.
* `Day.as` generation (critical items, one weather item, 7–9 items, 8–9 on rebel days, shuffled,
  appear times `rand * 0.75*i/n * 60 s`, halved on day 1).
* Readership/loyalty maths (loyalty bonus per 100 readers above 200, integer truncation of
  `int += Number`, ×0.5/0.75/0.9/1.25 reader factors), loyalty clamp ±30, article sizes ×1/×3/×6,
  coverage rule (< 0.75 of 19 cells), "interesting" thresholds (<2, >2).
* Goals (day 3/5/10), pass/fail branches, state ⇄ rebel path, all briefing / night / tutorial
  texts, the `|`-variants of the rebel messages chosen by goal status, ending that swaps
  Republia→Democria (masthead, text) and restarts with the "new wife and child" line.
* Day clock 60 s (day 1 at half speed), End Day = 10× speed, alarm at 75 %, "day over" popup.
* One article per news item, no overlaps, 4×5 grid, sizes S 1×2, M 2×2, B 3×3 cells.
* Bug-compat: the unreachable "decreasing readership" comment of `Readership.as` is kept
  unreachable; `Morning` for the failed second goal prints the *next* goal's loyalty (-30) as in the
  original; `haveWonAtLeastOnce` is set on *any* game over as in the original.

## Features adapted
* **Drag & drop → focus model**: feed selection (↑/↓), size selection (←/→ BIG/MED/SMALL), `A`
  picks the article up, arrows move a footprint on the grid (green = free, red = overlap), `A`
  drops, `B` discards (= "dropped outside the paper"). `B` in the feed switches to *paper mode*
  to pick up a placed article. An invalid drop keeps you in placement mode instead of deleting
  the article (the original removes it).
* Feed shows the **headline** of each item; the full blurb of the selected item is in the bottom
  panel (the 540 px feed does not fit 88 px). Rebel messages (no headline) show their first
  characters in red and are auto-selected when they arrive.
* Messages longer than the screen are paged (`SPACE`).
* Fonts: only SILKWONDER (3×7 px proportional) is used; SG03 renders `e` like `a` at this size,
  Motorola/7x5 are too wide.
* Real-time scaling: time unit 1/600 s so the day is 60 s on both 50 and 60 Hz.

## Features omitted
Mouse control, the mute *button* (replaced by the M key), cursor image, mouse-over sounds,
the animated "dragging" sprite.

## Known limitations
* Emulator-only testing (openMSX 19.1 + C-BIOS). Joystick path implemented, **not tested**.
* 50 Hz and 60 Hz machines were both run in openMSX (day length measured, see `tests/`).
* Music/SFX not listened to on real hardware.
* "Loyalty" label under the dial loses the tail of the `y` (8-row font cell).

## Performance
* Profiling (PC sampling in openMSX, `tests/prof.tcl`): drawing text was 70 % of CPU in the
  first prototype (~9000 cycles/char in C). The glyph blitter was rewritten in Z80 assembly
  (`glyph_blit`, documented in `msx_gfx.c`), ~5× faster. Feed redraw is now incremental.
* Day-start page redraw measured ≈ 0.3 s worst case at 3.58 MHz (not per frame).
* Per-frame cost in play: clock hands (160 B blit) + input handling; ISR ≈ 2–3 k cycles.

## Assembly routines
`glyph_blit` in `src/platform/msx_gfx.c` (no alternate regs / stack tricks, so it is ISR-safe).
Reason: proven hotspot above. Everything else is C.
