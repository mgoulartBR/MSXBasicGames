# Balatro-MSX (fan port, unofficial, non-commercial)

A playable port of the core loop of *Balatro* (poker roguelike) to **MSX2** (V9938, Screen 5, 256×212×16)
using [MSXgl](https://github.com/aoineko-fr/MSXgl) v1.5.0. 256 KB ROM (ASCII-8 mapper), PSG music/SFX, and
**keyboard + joystick + mouse** input (mouse = MSX mouse protocol on port A, hardware sprite cursor).

> **Legal:** Balatro and its art/text are © LocalThunk / Playstack. This repo contains only the port's code and
> conversion tools. The proprietary source/assets are **not** included: put them in `original/` yourself (see
> `docs/PORTING.md`). Generated ROM/asset segments are derived from them and are therefore git-ignored.
> See `LICENSES.md`. Do not redistribute the ROM.

> **Testing status:** the automated tests run **only in the openMSX 19.1 emulator with the C-BIOS MSX2 ROM**. The project owner reports that an earlier build runs on a real MSX; this repo's automated tests run only in openMSX with the C-BIOS MSX2 ROM, and later builds were not tried on hardware.

## Quick start
```
bash scripts/setup.sh          # fetch/pin MSXgl, check tools (see docs/TOOLCHAIN.md)
bash scripts/build.sh          # -> dist/balatro-msx-0.1.rom (+ SHA256, memory report, map in build/)
bash scripts/run.sh            # open in openMSX (mouse on port A; ROM type ASCII8SRAM2 so the run is autosaved)
bash scripts/test.sh           # host unit/sim tests + Z80 self-test + openMSX smoke + screenshots
```

## Controls
| Action | Keyboard | Joystick | Mouse |
|---|---|---|---|
| Move focus | arrows | d-pad | hover |
| Select / confirm | Space / Enter | button A | left click |
| Back / cancel / sort | ESC, Z/X shortcuts shown on screen | button B | right click |
| Mute music | M | – | – |

## What is implemented
Full run structure: title, blind select (Small/Big/Boss, **Skip + 17 tags**), rounds (8 cards, 5 played, hands/
discards, poker evaluation of all 12 hand types incl. Five of a Kind/Flush House/Flush Five, planet levels),
cash-out with interest, shop (jokers, planets, tarots, **11 vouchers**, Booster packs: Arcana/Celestial/Buffoon/Standard/Spectral),
18 Spectral cards (incl. The Soul and Ectoplasm), 15 decks and 8 stakes (New Run screen; Eternal / Perishable / Rental Jokers),
card enhancements, editions (incl. Negative) and seals (22 tarots),
**150 jokers**, all 28 boss blinds, 8 antes with win/game over, optional endless mode to Ante 12, **autosave + Continue** (needs a cartridge with SRAM, see docs/PORTING.md), animated scoring, music + SFX.

## Not implemented / simplified (see TODO.md)
Shop Jokers never roll editions, no Negative / Foil / Holographic / Polychrome / Voucher / Standard / Ethereal tags, no unlocks / challenges,
endless mode beyond Ante 12, controller rumble etc. Visuals are down-converted to 16 colours at 1:1 pixel scale.

Docs: `docs/PORTABILITY_ASSESSMENT.md`, `docs/PORTING.md`, `docs/TOOLCHAIN.md`, `TODO.md`, `LICENSES.md`.
