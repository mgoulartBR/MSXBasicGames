# Keen Dreams MSX2 — 0.9.98

Versão de desempenho sobre a 0.9.62. Mesma campanha, mesma arte; o jogo roda mais rápido e a barra de HUD agora é fixa no rodapé. ROM ASCII16, 2 MiB.

**Arquivos:** `KEEN-0.9.98.ROM`, `KEEN-0.9.98-fontes.zip` (fontes completas; compilam para a mesma ROM, SHA-256 idêntico), `SHA256SUMS.txt`.

## Português

### O que mudou
- **Desenho dos atores em um único passe (LMMM + TIMP)**, em vez de duas passagens AND/OR: o herói, os atlas de Hill, Melon e Bridge, vários quadros das demais fases e o **chefe de Boobus** (fase 11). Corta o tempo de VDP dos atores quase pela metade onde eles dominam.
- **Muito código reescrito em Z80**: rolagem do cenário (`Columns`/`Rows`), listas de atores e itens, chaves, bombas, carrinhos, varreduras de bônus e picapes, passos dos inimigos de várias fases.
- **Troca de página/rolagem na interrupção de vblank**: o desenho não espera mais o VDP terminar; o flip só acontece com o VDP livre.
- **Barra de HUD fixa no rodapé** (interrupção de linha), em MSX2+ a partir de ~5 MHz. Abaixo disso a barra é desligada automaticamente e o HUD volta a ser desenhado no anel.
- **Correção de congelamento**: nas versões 0.9.76 a 0.9.87 a barra podia travar o jogo (a fase 5 foi onde apareceu).

### Desempenho (openMSX, C-BIOS MSX2+; renders por segundo)
Medidas no **openMSX**, com o Z80 forçado a cada velocidade. **Nada foi testado em MSX real.**

Roteiro de posição controlada a 5,37 MHz (o Keen é levado a pontos fixos e anda/pula; 0.9.75 → 0.9.98):

| Fase | 0.9.75 | 0.9.98 |
|---|---|---|
| 1 Hill | 9,1 | 14,7 |
| 2 Mines | 6,8 | 8,8 |
| 3 Bridge | 5,1 | 6,3 |
| 4 Rapids | 6,8 | 8,9 |
| 5 Pass | 10,1 | 14,4 |
| 6 City | 4,6 | 6,6 |
| 7 Apple | 6,7 | 12,4 |
| 8 Grove | 3,7 | 17,4 |
| 9 Bay | 8,7 | 11,9 |
| 10 Swamp | 7,0 | 11,2 |
| 11 Boobus | 4,6 | 7,2 |
| 12 Castle | 6,3 | 10,9 |

(O teletransporte refaz o anel a cada ponto, então os valores absolutos são menores do que andando de verdade; vale a comparação entre versões. As trajetórias variam ±10% de uma ROM para outra.)

Andando 40 s, 0.9.98: a 5,37 MHz — Hill 39,6; Apple 18,6; Grove 35,9; Castle 22,2; Bridge 15,8; City 14,3; Rapids 13,6; Bay 12,1; Mines 12,1; Pass 11,4; Boobus 6,1. A 7,16 MHz — Hill 44,1; Grove 41,9; Apple 29,8; Bridge 23,7; Mines 20,8; City 20,8. A 3,58 MHz o jogo roda mas lento (Hill 21,6; as demais entre 3,5 e 17), e a barra de HUD fica desligada.

### Diferença de comportamento
- A física do Keen avança dois ticks de 70 Hz por passada (35 Hz). Gravidade e aceleração horizontal são aplicadas como no original (nos ticks ímpares), mas o passo é maior; o jogo continua jogável e as fases terminam, porém o "feeling" pode diferir de leve do original.

### Limites conhecidos
- **Só openMSX.** O desempenho em hardware real (esperas de E/S do VDP, MSX2 contra MSX2+/turbo R) não foi medido.
- Em capturas isoladas, a barra de HUD pode mostrar um quadro anômalo por um instante (observado em algumas fases, também em versões anteriores); causa não encontrada.
- A cobertura do TIMP é parcial: Rapids, Pass, Grove, Bay, Swamp e Castle ainda usam AND/OR em vários quadros (a análise mostra que nessas fases o gargalo é CPU, não VDP, então o ganho restante seria pequeno).
- A fase 10 (Swamp) não foi medida andando (o roteiro de entrada não inicia a fase); só no roteiro de posição controlada.

### Compilar
```
MSXGL_ROOT=/caminho/MSXgl bash scripts/build.sh
```
MSXgl 1.5.2 + SDCC 4.6.0 (o do próprio MSXgl). Saída: `build/keen_msx.rom`.

## English

Performance release on top of 0.9.62. Same campaign and art; the game runs faster and the HUD is now a fixed bar at the bottom of the screen. ASCII16 mapper, 2 MiB.

**What changed**
- **One-pass actor drawing (LMMM + TIMP)** instead of two AND/OR passes: the hero, the Hill/Melon/Bridge atlases, many frames of the other stages and the **Boobus boss** (stage 11).
- **A lot of Z80 rewrites**: scroll ring (`Columns`/`Rows`), actor/item lists, keys, bombs, carts, bonus/pickup scans, enemy steps in several stages.
- **Page flip/scroll moved into the vblank interrupt**; drawing no longer waits for the VDP, the flip only happens when the VDP is idle.
- **Fixed HUD bar at the bottom** (line interrupt), on MSX2+ from ~5 MHz up; below that it is disabled automatically.
- **Freeze fix** for the HUD bar present in 0.9.76–0.9.87.

**Performance** — measured in **openMSX only** (C-BIOS MSX2+, Z80 clock forced to 3.58/5.37/7.16 MHz); **not tested on real hardware**. Controlled-position script at 5.37 MHz, 0.9.75 → 0.9.98: Hill 9.1 → 14.7, Mines 6.8 → 8.8, Bridge 5.1 → 6.3, Rapids 6.8 → 8.9, Pass 10.1 → 14.4, City 4.6 → 6.6, Apple 6.7 → 12.4, Grove 3.7 → 17.4, Bay 8.7 → 11.9, Swamp 7.0 → 11.2, Boobus 4.6 → 7.2, Castle 6.3 → 10.9 renders/s. Walking, 0.9.98: Hill 39.6 fps at 5.37 MHz and 44.1 at 7.16 MHz.

**Behavior difference**: Keen's physics steps two 70 Hz ticks per pass (35 Hz).

**Known limits**: openMSX only; an occasional one-frame HUD-bar glitch in isolated captures (cause unknown); TIMP coverage is partial (the remaining AND/OR stages are CPU-bound, so the possible gain is small).
