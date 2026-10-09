// MadTV-MSX build configuration (MSXgl v1.5.0)
ProjName    = "madtv";
ProjModules = [ "src/main", "src/ui", "src/sim", "src/db" ];
ProjSegments = "src/seg/seg";   // segmentos do mapper: src/seg/seg_s<N>_b<B>.c (3,4 = dados no banco 3; 5.. = codigo banked no banco 2)
LibModules  = [ "system", "bios", "vdp", "input", "memory" ];
Machine     = "2";          // MSX2 (V9938) como base
Target       = "ROM_ASCII8"; // mapper: ver docs/PORTING.md
ROMSize      = 128;         // KB total (16 segmentos de 8 KB)
ROMMainSegments = 2;        // segmentos 0-1 = codigo fixo (4000h-7FFFh); banco 2 (8000h) = codigo banked; banco 3 (A000h) = dados
BankedCall      = true;
CheckVersion    = true;
AddROMSignature = true;
AppSignature    = true;
AppCompany      = "MG";
AppID           = "TV";
Optim           = "Size";
DoRun           = false;    // execucao e feita por scripts/run.sh
CompileOpt = (typeof process !== "undefined" && process.env.MADTV_PROF) ? "-DMADTV_PROF" : "";
Verbose         = false;
