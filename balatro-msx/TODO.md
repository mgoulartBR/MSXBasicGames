# TODO
## Critical
- [DONE] Autosave to cartridge SRAM (ASCII8 + SRAM mapper, e.g. openMSX `-romtype ASCII8SRAM2`): saved when the run changes on the Blind / shop / round-input screens; **Continue** on the title screen. Plain ROM carts have no SRAM: saving is off. Not tested on real hardware or on 8 KB SRAM variants.
- [DONE] Real hardware: the project owner reports the game runs on a real MSX (details such as the machine, cartridge and mouse are not recorded here; later builds, with Negative / the new decks, were only run in openMSX).
- [DONE] Licence / legal: the owner considers this settled (many fan versions of Balatro exist on itch.io). This is the owner's decision, not legal advice: no licence file was added and the ROM is not distributed in this repository.
## Gameplay
- [DONE] Endless mode after the Ante 8 win, up to Ante 12 (chips are u32: Ante 13+ would overflow).
- [DONE] All 28 bosses (face-down bosses reveal on play; Amber Acorn shuffles but does not flip jokers).
- [DONE] Card enhancements (8), editions (Foil/Holo/Polychrome/Negative), seals (Gold/Red/Blue/Purple), 22 tarots, Standard packs, Wheel of Fortune (jokers' editions).
- [DONE] Joker art streaming from ROM: VRAM is no longer the limit (room for 18 Spectral cells and ~60 more jokers' art; the new jokers' rules are still TODO).
- [DONE] 18 Spectral cards incl. The Soul and Ectoplasm, and Spectral packs.
- [DONE] Negative edition on Jokers and consumables: +1 Joker / consumable slot (up to 8 Joker and 4 consumable slots; the Joker row overlaps like the hand when it no longer fits). Negative comes from Ectoplasm and from Perkeo's copies; shop Jokers never roll editions in this port (no Foil/Holo/Poly/Negative in the shop) and there is no Negative Tag.
- [DONE] Jokers: all 150 (Diet Cola creates a Double Tag when sold); balance only checked by bot simulation.
- [DONE] 11 base decks (Red, Blue, Yellow, Green, Ghost, Abandoned, Checkered, Zodiac, Painted, Plasma, Erratic) and the 8 stakes with Eternal / Perishable / Rental stickers; choose them on the New Run screen. No unlock progression (all available).
- [DONE] Black, Magic, Nebula and Anaglyph decks, with the Crystal Ball and Telescope vouchers and the Double Tag (so 11 vouchers, 17 skip tags, 15 decks).
## Graphics
- [PARTIAL] 16-colour conversion; some joker art cropped to 20×28.
## Audio
- [PARTIAL] Original PSG tune + SFX; no SCC/FM.
## Performance
- [PARTIAL] Screen transitions ≈0.5 s in emulator (measured in openMSX frames); steady-state within frame budget.
## Testing
- [DONE] Host + Z80 self-test (37/37), smoke tests, mouse test with xdotool (emulated).
