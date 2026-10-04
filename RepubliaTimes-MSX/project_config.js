// Republia Times MSX - MSXgl build configuration (BigFive Studios port)
ProjName    = "republia";
ProjModules = [ "src/main", "src/platform/msx_gfx", "src/platform/msx_input", "src/platform/msx_audio", "src/platform/msx_isr", "src/game/logic", "src/game/widgets", "src/game/ui", "src/game/scr_title", "src/game/scr_morning", "src/game/scr_night", "src/game/scr_play", "src/data/font_data", "src/data/music_data" ];
LibModules  = [ "system", "bios", "vdp", "input", "memory", "psg", "mouse" ];
Machine     = "1";          // MSX1 (Screen 2) - see docs/PORTING.md
Target      = "ROM_ASCII8"; // ASCII8 mapper, code in segs 0-2, data in segs 4-5 (docs/PORTING.md)
ROMSize     = 64;
ProjSegments = "src/data/seg";
Verbose     = true;
DoRun       = false;
AppSignature = false;
Optim = "SIZE";

// Optional debug defines, e.g. RT_DEFINES="-DDBG_START_DAY=8 -DDBG_FAST=5" scripts/build.sh
// Workaround for an MSXgl v1.5.0 quirk (no patch to the library): engine/src/mouse.c uses INPUT_DI /
// INPUT_EI inside __asm blocks but never includes input.h, so the macros stay unexpanded and the
// assembler fails. Defining them on the command line (same values as input.h with
// INPUT_USE_ISR_PROTECTION=TRUE) fixes it. See docs/TOOLCHAIN.md.
CompileOpt = "-DINPUT_DI=di -DINPUT_EI=ei " + (process.env.RT_DEFINES || "");
