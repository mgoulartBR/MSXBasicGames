# MSX DOOM — mapeamento para V9968 + Geo3D

Fontes consultadas (commits clonados em 2026-10-06): hra1129/V9968_Cartridge, kanon-ai/V9968_Geo3D_SampleDemo (docs/TECHNICAL.md), kanon-ai/NEON_REVENANT (v9968-geo3d).

| Hardware | Protótipo web |
|---|---|
| Tela 256×212, SCREEN 5/8 | canvas interno 256×212, ampliado com `image-rendering: pixelated` |
| SCREEN 8, 256 cores GRB 3-3-2 | pós-processamento `quantize()` por quadro |
| ≤255 vértices e ≤255 faces por RUN do Geo3D | paredes só com faces expostas; F3 mostra faces visíveis e nº de RUNs |
| Sombreamento por face | `flatShading: true` |
| Sem Z-buffer, ordenação por RUN (painter) | NÃO emulado: WebGL usa Z-buffer |
| Textura afim, sem correção de perspectiva | NÃO emulado: WebGL usa perspectiva correta |
| R800 = lógica/IA/input; Geo3D = transformação; V9968 = framebuffer/sprites | `game.js` (lógica) / Three.js (transformação) / canvas 2D (arma e HUD) |

## Status de validação
- Protótipo web: testado só em Chromium headless (captura de tela, sem erros de console). Jogabilidade completa e áudio: UNTESTED.
- ROM MSX (turbo R + V9968 + Geo3D, C/SDCC, ASCII16): **não existe ainda — UNTESTED**. Não há SDCC nem openMSX com Geo3D neste ambiente.
- Sprites e tiles: pixel art procedural (`sprites.js`); Higgsfield não foi usado.
