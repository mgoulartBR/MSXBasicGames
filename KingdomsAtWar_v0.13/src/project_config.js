const variant = process.env.KAW_TARGET || "V9990";
ProjName = variant === "V9990" ? "KAW9990" : variant === "PREVIEW" ? "KAW68PV" : "KAW9968";
ProjModules=["main"];
LibModules=["system","bios","vdp","input","memory"];
if(variant === "V9990") LibModules.push("v9990");
CompileOpt=variant === "V9990" ? "-DKAW_V9990" : variant === "PREVIEW" ? "-DKAW_PREVIEW" : "";
Machine="2";Target="ROM_ASCII8";ROMSize=256;ROMMainSegments=4;BankedCall=false;
ProjSegments="banks/art";DoRun=false;DoDeploy=false;

// Avoid IY allocation across ROM-bound expressions and BIOS callbacks.
CompileOpt += " --reserve-regs-iy";

// Compilacao incremental opcional (KAW_INCR=1): pula objetos mais novos que o fonte.
// Use apenas quando nenhum .h mudou; buildp.sh decide isso.
CompileSkipOld = !!process.env.KAW_INCR;
CompileComplexity = "OPTIMIZED";
