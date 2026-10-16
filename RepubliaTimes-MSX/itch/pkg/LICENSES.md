# Licenses and third-party notices

## This port
Port code under `src/`, `tools/`, `scripts/`, `tests/`, `docs/` © 2026 **BigFive Studios**.
Unless the original author states otherwise, this port is a **non-commercial fan port**.

## Original game: *The Republia Times* by Lucas Pope (@dukope)
* The material provided for this port (`original/`) contains **no LICENSE / README / notice
  file**: no license grant was found. Copyright in the game, its text, art, fonts, sound and
  music remains with Lucas Pope (and the owners of the embedded fonts).
* This port reuses the original **game rules, news-item texts, goals, briefing texts, pixel art
  (converted/downscaled) and fonts (rasterised)** and an automatic approximation of the music
  loops. Credit to the original author is displayed in the game and must be kept.
* **Legal limitation**: before publishing or redistributing the ROM outside this repository
  (stores, cartridges, contests), get the original author's permission. Until then treat the
  ROM and `src/data/*` as private / evaluation material.
* The original `original/src/assets/*.ttf` fonts (`7x5`, `SG03`, `SILKWONDER`,
  `MotorolaScreentype`) have no license information in the provided material; only SILKWONDER
  is rasterised into the ROM (see `src/data/font_data.c`).
* Items found in `original/` but **not used** on MSX: Flixel glue, mp3 files (replaced by PSG
  data), `Mute.png`, `Cursor.png`, `Button*.png`, `CenterPopup.png`, `Background.png` (the screen
  layout was redone for 256x192).

## MSXgl (engine / library)
* MSXgl © Guillaume "Aoineko" Blanchard, **CC BY-SA 4.0** (`MSXgl/LICENSE.md`, fetched by
  `scripts/setup.sh`). The compiled ROM contains MSXgl code (CRT0, VDP, BIOS, PSG, input,
  memory modules); under CC BY-SA, distribution of adaptations must credit MSXgl and carry the
  same license for the MSXgl-derived parts. The MSXgl tree itself is **not** vendored here.
* SDCC (bundled in MSXgl): GPL with the standard library exception for generated code.

## Tools used only on the host
openMSX (GPL-2.0), C-BIOS (BSD-style, redistributable), Python/Pillow/NumPy, ffmpeg, Node.js.
No proprietary MSX BIOS is used or distributed.
