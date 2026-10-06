# MSX DOOM — ROM (turbo R + V9968 + Geo3D)

Esqueleto jogável em C/SDCC: ROM de 16 KiB, sem mapper, cabeçalho `AB` em 0x4000.
O protótipo web equivalente está em [`../msxdoom`](../msxdoom).

```sh
./build.sh                       # SDCC >= 4.2, makebin, Python 3 -> out/msxdoom.rom
python3 tools/test_game.py       # testes de lógica no Z80 emulado (pip install z80 pillow)
python3 tools/z80_harness.py 4   # renderiza quadros em out/harness_N.png
```

![Cena inicial](out/preview_start.png) ![Inimigo na mira](out/preview_enemy.png)

## Controles (MSX)
Cursor ←/→ giram, ↑/↓ andam, ESPAÇO atira (reinicia após o fim). Mouse: não implementado.

## Status de validação

| Nível | Resultado |
|---|---|
| BUILD OK | Sim. SDCC 4.2.0, ~10 KiB de 16 KiB usados. |
| BOOT OK | **UNTESTED** em openMSX, blueMSX+, FPGA e hardware. |
| SIMULATED | Sim: ROM executada num Z80 emulado (pip `z80`) com um modelo próprio e simplificado de V9968 (SCREEN 5, LMMV, paleta, páginas) e Geo3D (registradores de `geo3d_engine.v`, faces preenchidas). 7 testes de lógica passam (andar, girar, colisão, tiro que mata, fim de jogo, reinício). **Isso não é um emulador de MSX.** |
| EMULATOR TESTED (openMSX/blueMSX+ com Geo3D) | **UNTESTED** |
| FPGA TESTED / HARDWARE TESTED | **UNTESTED** |

```text
MSXgl: não usado
SDCC: 4.2.0 #13081
V9968 specification/revision: hra1129/V9968_Cartridge @ 55966fb
Geo3D specification/revision: geo3d_engine.v (alexmoncks, v1.0.0 conforme geo3d/readme.md), cópia em V9968_Cartridge
openMSX build / commit / Machine XML: não testado
V9968 configuration: I/O base 0x98 (Geo3D em 0x9D/0x9F, como NEON_REVENANT)
Mapper: nenhum (ROM 16 KiB em 0x4000)   ROM size: 16384
Target machine: MSX turbo R + V9968 + Geo3D   CPU mode: R800 ROM (CHGCPU 0x81) se MSXVER>=3
Test status: BUILD OK + SIMULATED; todo o resto UNTESTED
```
Fontes lidas em 2026-10-06: V9968_Cartridge `55966fb` (hra1129); V9968_Geo3D_SampleDemo `3fe8a01` e NEON_REVENANT `030555e` (kanon-ai).

## Arquitetura
R800/Z80: input, IA, colisão, hitscan, câmera. Geo3D: transformação, projeção, culling, sombreado e ordem do pintor
(4 chunks do mapa + 1 RUN por inimigo, cada um <= 255 vértices/faces). V9968: teto/chão, arma, HUD, mira e a página exibida.
`tools/gen_level.py` converte o mapa em streams VDATA/FDATA prontos para `OTIR`.

## Suposições NÃO verificadas em hardware (revisar primeiro se algo falhar)
1. Matriz enviada linha a linha (M00,M01,M02,M10...) e `p' = M·v + T` (T somado depois da rotação, como em NEON_REVENANT).
2. Convenção da câmera: +Z para frente, +X direita, +Y cima; `F=170, CX=128, CY=106, ZNEAR=4`.
3. Desbloqueio `OUT (0x9C),0` e `R#20=1, R#21=0`, copiados da prática do NEON_REVENANT/TECHNICAL.md. Não confirmei que SCREEN 5 + LMMV exige isso.
4. Troca de página via `R#2 = 0x1F | page<<5` e `YPAGE = page<<8` (como nas demos); espera de VBLANK por polling de S#0 bit 7.
5. Teclado: linha 8 da matriz (setas e espaço), como em NEON_REVENANT. PSG: R7 = 0xB7 (ruído no canal A).
6. Paleta de 16 cores (SCREEN 5). Rampa 1-7 para paredes, 8-14 para inimigos (`BASE + nível de sombra`), 15 chão.

## Achados e limitações
- **SDCC 4.2.0 gerou `(s32)a*b` errado** com multiplicador negativo (ex.: `(-16384*151)>>14` deu +453). Todo o jogo usa `mulq14()` (soma e deslocamento), validado contra Python em 600 pares.
- Sem Z-buffer entre RUNs: um inimigo seria desenhado através da parede. Mitigação: só desenha inimigo com linha de visão (grade). Chunks são ordenados do mais distante ao mais próximo; paredes de chunks diferentes podem se sobrepor incorretamente em casos extremos.
- Os modelos de nível são reenviados ao Geo3D a cada quadro (4 chunks); o custo real é desconhecido (UNTESTED). Se for lento, enviar só os chunks próximos.
- Sem texturas (o Geo3D suporta LRMM; exigiria atlas na VRAM e `CTRL bit2`). Inimigo é uma pirâmide provisória, não o imp do protótipo web.
- Sem mouse, sem portas que abrem, sem texto (HUD só com barras), sem tela de título.
