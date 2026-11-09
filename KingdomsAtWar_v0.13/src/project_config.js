// Kingdoms at War v0.13: V9990 in P1 mode (tiles and sprites). The bitmap-mode and V9968 builds are frozen at v0.13-final.
ProjName = "KAW9990P1";
ProjModules = ["main"];
LibModules = ["system", "bios", "vdp", "input", "memory", "v9990"];
CompileOpt = "--reserve-regs-iy"; // keep IY out of allocation across ROM-bound expressions and BIOS callbacks
Machine = "2"; Target = "ROM_ASCII8"; ROMSize = 256; ROMMainSegments = 4; BankedCall = false;
ProjSegments = "banks/art"; DoRun = false; DoDeploy = false;
// Incremental build (KAW_INCR=1) skips objects newer than their source; use only when no .h changed.
CompileSkipOld = !!process.env.KAW_INCR;
// KAW_FAST=1 uses the compiler's default complexity (about 3 minutes instead of 9, slower code).
CompileComplexity = process.env.KAW_FAST ? "DEFAULT" : "OPTIMIZED";
