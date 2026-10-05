// Balatro MSX port - MSXgl build configuration
// Built with MSXgl's own Node.js build tool (engine/script/js/build.js).
// Machine: MSX2 baseline (V9938, 128 KB VRAM). Mapper: ASCII-8.

ProjName    = "balatro";
ProjModules = [ "src/main" ];
ProjSegments = "src/seg/gfx";

LibModules = [ "system", "bios", "vdp", "input", "memory", "psg" ];

Machine = "2";
Target  = "ROM_ASCII8";
ROMSize = 256;           // KB, ASCII-8 mapper
ROMMainSegments = 3;     // 24 KB of fixed code/rodata (banks 0-2); bank 3 is the data window
BankedCall = true;

CheckVersion = true;
AppSignature = true;
AppCompany = "MG";
AppID = "BL";

Optim = "Speed";
Verbose = false;

// Emulator: openMSX with C-BIOS MSX2 (headless-friendly); mouse in port A for testing
EmulMachine = true;
EmulPortA = "Mouse";
