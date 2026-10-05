// Balatro MSX port - MSXgl build configuration
// Built with MSXgl's own Node.js build tool (engine/script/js/build.js).
// Machine: MSX2 baseline (V9938, 128 KB VRAM). Mapper: ASCII-8.

ProjName    = "balatro";
ProjModules = [ "src/main", "src/platform/pvideo", "src/platform/ctrl", "src/gen/font_gen",
                "src/game/rng", "src/game/poker" ];
ProjSegments = "src/seg/seg";

LibModules = [ "system", "bios", "vdp", "input", "memory", "psg" ];

Machine = "2";
Target  = "ROM_ASCII8";
ROMSize = 256;           // KB, ASCII-8 mapper
ROMMainSegments = 2;     // 16 KB fixed code (banks 0-1); bank 2 = switchable code segments, bank 3 = data window
BankedCall = true;

CheckVersion = true;
AppSignature = true;
AppCompany = "MG";
AppID = "BL";

Optim = "Speed";
CompileOpt = "-Iinclude -Isrc";
Verbose = false;

// Emulator: openMSX with C-BIOS MSX2 (headless-friendly); mouse in port A for testing
EmulMachine = true;
EmulPortA = "Mouse";
