# TODO
## Critical
- [TODO] Test on real MSX2 hardware (mouse, timing) — nothing tested outside openMSX/C-BIOS.
- [TODO] Decide the repo licence; confirm legal status with rights holders before any ROM distribution.
## Gameplay
- [DONE] Endless mode after the Ante 8 win, up to Ante 12 (chips are u32: Ante 13+ would overflow).
- [DONE] All 28 bosses (face-down bosses reveal on play; Amber Acorn shuffles but does not flip jokers).
- [DONE] Card enhancements (8), editions (Foil/Holo/Polychrome; Negative not implemented), seals (Gold/Red/Blue/Purple), 22 tarots, Standard packs, Wheel of Fortune (jokers' editions).
- [DONE] Joker art streaming from ROM: VRAM is no longer the limit (room for 18 Spectral cells and ~60 more jokers' art; the new jokers' rules are still TODO).
- [DONE] 16 Spectral cards and Spectral packs (The Soul and Ectoplasm are left out: no legendary Jokers / no Negative edition).
- [TODO] Negative edition (needs a 6th Joker slot in the UI), save.
- [DONE] Jokers: 149 of 150 (Diet Cola needs a Double Tag); balance only checked by bot simulation.
- [DONE] 11 decks (Red, Blue, Yellow, Green, Ghost, Abandoned, Checkered, Zodiac, Painted, Plasma, Erratic) and the 8 stakes with Eternal / Perishable / Rental stickers; choose them on the New Run screen. No unlock progression (all available).
- [TODO] Black, Magic, Nebula and Anaglyph decks (need a 6th Joker slot / Crystal Ball / Telescope vouchers / Double Tag).
## Graphics
- [PARTIAL] 16-colour conversion; some joker art cropped to 20×28.
## Audio
- [PARTIAL] Original PSG tune + SFX; no SCC/FM.
## Performance
- [PARTIAL] Screen transitions ≈0.5 s in emulator (measured in openMSX frames); steady-state within frame budget.
## Testing
- [DONE] Host + Z80 self-test (37/37), smoke tests, mouse test with xdotool (emulated).
