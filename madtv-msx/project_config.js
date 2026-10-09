// MadTV-MSX build configuration (MSXgl v1.5.0)
ProjName    = "madtv";
ProjModules = [ "src/madtv", "src/ui", "src/sim" ];
ProjSegments = "src/data/db_data";   // segmentos extra: src/data/db_data_s<N>_b<B>.c
LibModules  = [ "system", "bios", "vdp", "print", "input", "memory" ];
Machine     = "2";          // MSX2 (V9938) como base
Target       = "ROM_ASCII8"; // mapper: ver docs/PORTING.md
ROMSize      = 128;         // KB total (16 segmentos de 8 KB)
ROMMainSegments = 3;        // segmentos 0-2 = codigo fixo (4000h-9FFFh); banco 3 (A000h) = janela de dados
CheckVersion    = true;
AddROMSignature = true;
AppSignature    = true;
AppCompany      = "MG";
AppID           = "TV";
Optim           = "Speed";
DoRun           = false;    // execucao e feita por scripts/run.sh
CompileOpt = (typeof process !== "undefined" && process.env.MADTV_PROF) ? "-DMADTV_PROF" : "";
Verbose         = false;
