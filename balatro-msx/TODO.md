# TODO
## Critical
- [TODO] Test on real MSX2 hardware (mouse, timing) — nothing tested outside openMSX/C-BIOS.
- [TODO] Decide the repo licence; confirm legal status with rights holders before any ROM distribution.
## Gameplay
- [DONE] Endless mode after the Ante 8 win, up to Ante 12 (chips are u32: Ante 13+ would overflow).
- [DONE] All 28 bosses (face-down bosses reveal on play; Amber Acorn shuffles but does not flip jokers).
- [DONE] Card enhancements (8), editions (Foil/Holo/Polychrome; Negative not implemented), seals (Gold/Red/Blue/Purple), 22 tarots, Standard packs, Wheel of Fortune (jokers' editions).
- [TODO] Spectral cards/packs (the VRAM atlas has no room left for their art), Negative edition, Black Hole/other missing planets, other decks/stakes; save.
- [PARTIAL] Jokers: 88 of 150; balance only checked by bot simulation.
## Graphics
- [PARTIAL] 16-colour conversion; some joker art cropped to 20×28.
## Audio
- [PARTIAL] Original PSG tune + SFX; no SCC/FM.
## Performance
- [PARTIAL] Screen transitions ≈0.5 s in emulator (measured in openMSX frames); steady-state within frame budget.
## Testing
- [DONE] Host + Z80 self-test (37/37), smoke tests, mouse test with xdotool (emulated).
