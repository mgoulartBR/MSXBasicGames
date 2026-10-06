// MadTV-MSX build configuration (MSXgl v1.5.0)
ProjName    = "madtv";
ProjModules = [ "src/madtv" ];
LibModules  = [ "system", "bios", "vdp", "print", "input", "memory" ];
Machine     = "2";          // MSX2 (V9938) como base
Target      = "ROM_32K";    // milestone 0.1; migrar para ROM_ASCII16 quando os dados crescerem
CheckVersion    = true;
AddROMSignature = true;
AppSignature    = true;
AppCompany      = "MG";
AppID           = "TV";
Optim           = "Speed";
DoRun           = false;    // execucao e feita por scripts/run.sh
Verbose         = false;
