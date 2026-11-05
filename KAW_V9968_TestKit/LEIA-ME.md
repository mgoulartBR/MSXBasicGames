# Kingdoms at War — ambiente de teste V9968 (v0.13)

**Não existe emulador de V9968.** Este kit roda a `KAW9968.ROM` **sem alterações** no openMSX (núcleo V9958)
e emula apenas a leitura do ID do chip (V9968 = ID 3) por script (`v9968_shim.tcl`).
Tudo o mais (comandos de VDP, VRAM, raster, mouse, lógica) é executado de verdade.

## Como usar
1. Instale o openMSX (19.x ou mais novo) com C-BIOS (vem junto).
2. Windows: dê duplo clique em `play-v9968-emu.bat`. Linux/macOS: `bash play-v9968-emu.sh`.
3. O mouse MSX é ligado na porta 1. Tecla M redetecta o mouse.

## Limites (leia)
- Não é teste no cartucho V9968 real. O teste positivo no hardware continua pendente.
- O jogo usa a fórmula de scroll R#26/R#27 do V9968; no V9958 emulado o scroll fino horizontal pode aparecer até 7 px deslocado.
- Velocidade: reflete Z80 de 3,58 MHz e o VDP V9958 do openMSX, não o FPGA do V9968.
