# MSX DOOM — ROM texturizada (turbo R + V9968 + Geo3D)

ROM **ASCII16** de 256 KiB em C/SDCC: SCREEN 8 com paleta **EPAL de 256 cores**, paredes **texturizadas pelo Geo3D**
(LRMM por linha), inimigos como billboards texturizados com transparência, arma/mira/HUD por blits de fonte.
O protótipo web com a mesma arte está em [`../msxdoom`](../msxdoom).

![Cena](out/preview_start.png) ![Inimigo](out/preview_enemy.png)

```sh
./build.sh                         # SDCC >= 4.2, Python 3, numpy, pillow  ->  out/msxdoom.rom
python3 tools/test_boot.py         # boot no Z80 emulado: VRAM, paleta e quadro conferidos
python3 tools/test_game.py         # 7 testes de lógica (andar, girar, colisão, tiro, fim de jogo, reinício)
python3 tools/z80_harness.py 4     # renderiza quadros em out/harness_N.png
node tools/export_art.js           # (opcional) reexporta a arte de ../msxdoom/sprites.js para assets/*.png
```
Rodar (**o tipo de ROM precisa ser ASCII16 explicitamente**):

```text
# turbo R / R800 (BIOS própria):
openmsx -machine Panasonic_FS-A1ST_V9968 -ext geo3d -cart out/msxdoom.rom -romtype ASCII16
# MSX2+ / Z80, sem turbo R e sem BIOS dumpada (C-BIOS):
openmsx -machine C-BIOS_V9968_JP -ext geo3d -cart out/msxdoom.rom -romtype ASCII16
```
`C-BIOS_V9968_JP` não vem no openMSX oficial: é o `config/machines/C-BIOS_V9968_JP.xml` do repositório
[renatus-xxxx/openmsx-v9968-windows-setup](https://github.com/renatus-xxxx/openmsx-v9968-windows-setup) (o `setup-cbios-v9968.bat` monta o ambiente),
usado com o openMSX com Geo3D de [alexmoncks/openMSX](https://github.com/alexmoncks/openMSX) (ramo `geo3d`), conforme o `README.geo3d.md` dele.
Nesse perfil `MSXVER=2`, então a ROM **não** chama `CHGCPU` e roda em Z80. **Z80 não é mais rápido que o R800**: use-o só se o turbo R der problema. Controles: cursor ←/→ giram, ↑/↓ andam, ESPAÇO atira/reinicia.

## Status de validação

| Nível | Resultado |
|---|---|
| BUILD OK | Sim. SDCC 4.2.0. Código: 8,7 KB do banco 1; 10 bancos de dados; ROM 256 KiB. |
| BOOT OK | **UNTESTED** (openMSX, blueMSX+, FPGA, hardware) |
| SIMULATED | Sim: a ROM roda num Z80 emulado (pip `z80`) com modelo próprio e simplificado de ASCII16, V9968 (SCREEN 8, EPAL, LMMV, LMMM/TIMP) e Geo3D (faces texturizadas conforme `geo3d_engine.v`). `test_boot.py` e `test_game.py` passam. **Não é um emulador de MSX.** |
| EMULATOR TESTED / FPGA TESTED / HARDWARE TESTED | **UNTESTED** |

```text
MSXgl: não usado                    SDCC: 4.2.0 #13081
V9968 spec/revision: hra1129/V9968_Cartridge 55966fb (RTL: vdp_cpu_interface.v, vdp_command.v)
Geo3D spec/revision: geo3d_engine.v (alexmoncks, v1.0.0)  | referências: kanon-ai/V9968_Geo3D_SampleDemo 3fe8a01, NEON_REVENANT 030555e
openMSX build / commit / Machine XML: não testado
V9968 configuration: I/O base 0x98 (Geo3D em 0x9D/0x9F)    Mapper: ASCII16   ROM size: 262144
Target machine: MSX turbo R + V9968 + Geo3D    CPU mode: R800 ROM (CHGCPU 0x81) se MSXVER>=3
Test status: BUILD OK + SIMULATED; todo o resto UNTESTED
```

## Como funciona
- **Mapper:** banco 0 = cabeçalho `AB` + boot; o boot seleciona o banco 1 na janela 0x8000 (`0x7000`) e faz `ENASLT` da página 2 para o slot do cartucho; o **código roda do banco 1**. Os dados ficam nos bancos 2+, lidos pela janela 0x4000 (registrador em `0x6000`).
- **Vídeo:** `R#0=14` (SCREEN 8), `R#20=0x11` (EPAL + HS) após `OUT (0x9C),0`, `R#21=0`, `R#16`+porta `0x9A` com 256×(R,G,B de 5 bits) (RTL: `vdp_cpu_interface.v`). `R#51..58 = 0,0,0,0,255,0,255,3` (janela de origem do LRMM), como o NEON_REVENANT.
- **VRAM (256 KiB = 1024 linhas de 256 B):** páginas de desenho em 0..511 (`YPAGE = página<<8`); texturas em 512..1023 (carregadas no boot a partir dos bancos 2..9):

| Linhas | Conteúdo |
|---|---|
| 512–575 | atlas de paredes 256×64 (4 tiles 64×64), cópia escura (nível 0) |
| 576–639 | sprites 256×64 (2 tipos × 6 poses, 24×32), sem sombra |
| 640–703 | atlas de paredes, cópia média (nível 2) |
| 768–831 | atlas de paredes, cópia clara (nível 4) |
| 832–895 | pistola (parada e disparando) e clarão |
| 896–934 | fontes 5×7 (cinza, laranja, vermelho) e dígitos grandes |

- **Por que só 3 cópias de sombra:** o Geo3D escolhe a linha-fonte por `TEXY + nível*TSTRIDE`. A luz é **fixa no mundo** (recalculada por quadro no espaço da câmera), então paredes alinhadas aos eixos só recebem os níveis 0, 2 e 4; os intervalos entre as cópias guardam sprites. Com `TSTRIDE=0` os sprites usam uma cópia só.
- **Paleta (256):** 63 cores-base escolhidas por median-cut sobre as texturas, em 3 brilhos (índices 1–63 claro, 65–127 médio, 129–191 escuro; 0 = preto/transparente) + 14 rampas de teto/chão + cores do HUD.
- **Geometria:** cada parede exposta vira 4 tiras de 16 unidades (limita a distorção afim e o buraco do plano próximo); 544 faces em 16 chunks de 4×4 células (≤255 faces/vértices por RUN). Chunks fora do campo de visão são descartados na CPU e os visíveis são desenhados do mais longe ao mais perto. Jogador com raio de colisão 14 e `ZNEAR=2`.
- **Inimigos:** 1 RUN por inimigo, quad de câmera (matriz identidade), textura do atlas de sprites com `LOP=8` (TIMP). Só são desenhados se houver linha de visão (não há Z-buffer entre RUNs).
- **Arma e HUD:** LMMM com TIMP a partir da VRAM; o painel (y ≥ 178) é desenhado nas duas páginas, e os valores só são redesenhados quando mudam.

## Suposições NÃO verificadas em emulador/hardware (revisar primeiro se algo falhar)
1. **TIMP no LRMM:** `LOP=8` (registrador Geo3D 0x45) faz o LRMM tratar o texel 0 como transparente. O RTL repassa o `lop` ao comando, mas não testei o comportamento. Se estiver errado, os inimigos aparecem com fundo preto.
2. Matriz enviada linha a linha e `p' = M·v + T`; câmera +Z frente/+X direita/+Y cima; `F=170, CX=128, CY=89, ZNEAR=2, W=256, H=178`.
3. Coordenadas U/V são relativas a `TEXX/TEXY` (e o nível soma `nível*TSTRIDE` às linhas), como descreve o cabeçalho de `geo3d_engine.v`.
4. `IN A,(0xFF)` / `XOR 1` / `OUT (0xFE),A` no boot foi **copiado de todas as demos kanon-ai**; a finalidade não está documentada (provavelmente configuração do cartucho/máquina V9968).
5. Teclado: linha 8 da matriz (setas e espaço), como no NEON_REVENANT; PSG: `R7=0xB7`. `R#2 = 0x1F | página<<5` para trocar de página; polling de VBLANK em S#0 bit 7.
6. A rampa de brilho e o atlas foram validados só pelo meu simulador (rasterização afim por linha, como descrito no RTL).

## Achados e limitações
- **Bug do SDCC 4.2.0:** `(s32)a*b` com multiplicador negativo deu resultado errado; o jogo usa `mulq14()` (soma e deslocamento), validado contra Python em 600 pares.
- **Desempenho (medido só em simulação; UNTESTED no openMSX):** `tools/profile.py` conta T-states de CPU por quadro. A 1ª versão gastava ~1,21 M (3 fps em Z80); a atual gasta **~0,56 M (~6,4 fps de teto de CPU em Z80 de 3,58 MHz)**. Ganhos: `mulq14` em assembly (`src/math.s`), visibilidade dos chunks por incrementos na grade 4×4 (4 multiplicações em vez de ~64), `los()` sem divisão, LOD das paredes (2 tiras longe / 4 perto), HMMV nos preenchimentos, inimigos fora do campo de visão filtrados antes de ordenar/desenhar e inimigos distantes parados. Hoje ~38 % do tempo é `OTIR` para o Geo3D (~10 KB por quadro). **Não medi** o tempo de VDP/Geo3D (LRMM, HMMV) do emulador, que pode dominar; no R800 a parte de CPU cai bastante.
- Sem Z-buffer entre RUNs: paredes de chunks diferentes podem se sobrepor incorretamente em casos extremos. Sem sombreado por distância.
- Sem mouse, sem portas que abrem, sem tela de título, sem texto de vitória/derrota (só uma faixa colorida).
- **Se a imagem sair preta ou sem texturas**, verifique nesta ordem: ROM tipo ASCII16; extensão `geo3d` no openMSX; EPAL/`R#20`; janela do LRMM (`R#51–58`).
