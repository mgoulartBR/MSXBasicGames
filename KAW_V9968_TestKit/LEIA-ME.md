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

## V9990 (GFX9000) e modo turbo
- `play-v9990.bat` / `.sh`: V9990 em Z80 de 3,58 MHz (MSX2 comum).
- `play-v9990-turbo.bat` / `.sh` e `play-v9968-emu-turbo.bat` / `.sh`: Z80 a 10,7 MHz (turbo, como MSXBook). O VDP não acelera.
- Medido no openMSX (quadros/s com 1 / 4 unidades / scroll): V9990 3,58 MHz 9,6 / 6,9 / 14; 7,2 MHz 23 / 14 / 22; 10,7 MHz 25 / 21 / 27. V9968 via script: 3,58 MHz 5 / 3 / 6; 10,7 MHz 20 / 10 / 16.

**Atenção às configurações do openMSX:** `z80_freq` fica gravado nas configurações do openMSX. Os launchers deste kit usam `-setting kaw_openmsx_settings.xml` (arquivo próprio, criado na pasta do kit), então o modo turbo não vaza para o seu openMSX normal. Se você rodou `set z80_freq` à mão, restaure com `set z80_freq 3579545`.

## KAW9990P1.ROM (experimental, modo P1 do V9990)

Rode `play-v9990-p1.sh` (Linux/macOS) ou `play-v9990-p1.bat` (Windows). Mesma arquitetura do V9990, mas o mundo é de tiles com scroll por hardware e as unidades são sprites.

- Cerca de duas vezes mais quadros por segundo que o KAW9990.ROM com unidades andando.
- O carregamento inicial é mais longo (cerca de 36 s a 3,58 MHz); a barra de progresso aparece no meio da tela.
- Ainda faltam o clarão de dano, o fantasma de construção e a oclusão por prédios e árvores.
