# TODO

Status legend: DONE / PARTIAL / TODO / BLOCKED

## Critical
* TODO — verify on **real MSX hardware** (ASCII8 flash cart, H.TIMI hook, TMS9918 timing).
* TODO — get the original author's permission before any public release (see LICENSES.md).

## Gameplay
* DONE — day generation, readership/loyalty maths, goals, endings, restart as Democria.
* DONE — placement/pick-up/discard, paper & feed modes, End Day, day-over popup.
* TODO — MSX mouse support (original control scheme).
* TODO — playtest full 10-day run on both paths (only debug-start jumps were verified).

## Graphics
* DONE — Screen 2 UI, converted logos/dial/press band, procedural clock and needle.
* PARTIAL — "Loyalty" label clipped by 1 px on the dial.
* TODO — optional MSX2 Screen 5 variant with more colours (not needed).

## Audio
* PARTIAL — music is an automatic transcription of the MP3 loops; needs review by ear.
* DONE — PSG SFX, mute key (M).
* TODO — optional SCC/MSX-MUSIC arrangement.

## Performance
* DONE — asm glyph blitter, incremental redraw, ISR-driven input/audio.
* TODO — measure frame time on a real 3.58 MHz machine.

## Polish
* TODO — save/high-score (original has none).
* TODO — title-screen animation.

## Testing
* DONE — smoke (`tests/smoke.tcl`), gameplay (`tests/play.tcl`), story branches
  (`scripts/test_morning.sh`), audio register sampling (`tests/audio.tcl`), day length (`tests/daylen.tcl`).
* TODO — joystick test (openMSX joystick plug), MSX2 / turbo R machines.
