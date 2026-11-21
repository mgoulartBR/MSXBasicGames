# Keen Dreams MSX2 — 0.9.103

Continuação da 0.9.99. Mesma campanha, mesma arte; mais desempenho nas fases 6 e 7 (City e Apple) e em várias outras. **Requer MSX2+** (em MSX2 a ROM para com uma mensagem). ROM ASCII16, 2 MiB.

**Arquivos:** `KEEN-0.9.103.ROM`, `KEEN-0.9.103-fontes.zip` (fontes completas; compilar dá a mesma ROM, SHA-256 idêntico), `SHA256SUMS.txt`, `graficos/` (barras 0.9.99 × 0.9.103, variação por fase e 0.9.75 × 0.9.103).

## Português

### O que mudou (0.9.99 → 0.9.103)
- **Soldados com baioneta, Aspar/Mouth e Brocco em passe único (LMMM + TIMP)**, em vez de AND + OR: metade dos bytes de textura e dos comandos de VDP por desenho. (Mesmo método do chefe de Boobus, 0.9.96.)
- **City (fase 6) e Apple (fase 7)**: a coleta dos objetos visíveis (`CityCollect`, `AppleCollect`, `AppleStatic`) e a seleção de atores (`CitySelect`) foram reescritas em Z80.
- **Carrinhos (fases 2 e 7)**: `CartContact` em Z80.
- Rotinas novas verificadas por oráculos no emulador (a regra original em C é recalculada a cada chamada e comparada; também com entradas sorteadas): **0 divergências** em todas.

### Desempenho (openMSX, C-BIOS MSX2+, Z80 a 5,37 MHz; renders por segundo)
Roteiro de posição controlada (o Keen é levado a pontos fixos); gráficos em `graficos/`.

| Fase | 0.9.99 | 0.9.103 | Variação |
|---|---|---|---|
| 1 Hill | 14,8 | 14,4 | −2,9% * |
| 2 Mines | 8,8 | 9,2 | +5,0% |
| 3 Bridge | 6,3 | 6,9 | +8,5% |
| 4 Rapids | 8,9 | 9,2 | +2,8% |
| 5 Pass | 16,1 | 16,5 | +2,6% |
| 6 City | 6,6 | 8,6 | **+29,8%** |
| 7 Apple | 12,4 | 15,0 | **+21,1%** |
| 8 Grove | 17,5 | 17,8 | +1,9% |
| 9 Bay | 11,8 | 13,0 | +9,7% |
| 10 Swamp | 11,3 | 11,2 | −1,0% * |
| 11 Boobus | 7,2 | 6,6 | −8,4% * |
| 12 Castle | 10,9 | 10,9 | 0,0% |

\* Quedas dentro do ruído do método: o roteiro refaz a trajetória do Keen a cada ROM (±5–10% por fase). Com o Keen parado no mesmo ponto, Hill e Boobus dão resultados iguais nas duas versões (148/148 e 124/122 renders), e essas fases não tiveram mudança de código relevante.

Medições feitas **apenas no openMSX**; nada foi testado em MSX real.

### Diferenças e limites (herdados)
- A física do Keen avança dois ticks de 70 Hz por passada (35 Hz); o "feeling" pode diferir de leve do original.
- Em MSX2 (V9938) a ROM não roda (rolagem por software trava ao rolar para a esquerda): requisito MSX2+.
- Em modo R800 a barra de HUD sai corrompida (a detecção só vê o clock do Z80); abaixo de ~5 MHz a barra é desligada automaticamente.
- Em capturas isoladas a barra de HUD pode mostrar um quadro anômalo por um instante (transição de morte / barra desligada).
- Cobertura do TIMP ainda parcial (City, Apple, Castle e carrinhos ainda usam AND/OR); nessas fases o gargalo é CPU.

### Compilar
```
MSXGL_ROOT=/caminho/MSXgl bash scripts/build.sh
```
MSXgl 1.5.2 + SDCC 4.6.0 (o do próprio MSXgl). Saída: `build/keen_msx.rom`.

## English

Continuation of 0.9.99. Same campaign and art; faster stages 6 and 7 (City and Apple) and several others. **MSX2+ required** (on MSX2 the ROM stops with a message). ASCII16 mapper, 2 MiB.

**What changed (0.9.99 → 0.9.103)**
- **Bayonet soldiers, Aspar/Mouth and Brocco drawn in one pass (LMMM + TIMP)** instead of AND + OR: half the texture bytes and half the VDP commands per draw (same method as the Boobus boss, 0.9.96).
- **City (stage 6) and Apple (stage 7)**: object collection (`CityCollect`, `AppleCollect`, `AppleStatic`) and actor selection (`CitySelect`) rewritten in Z80.
- **Carts (stages 2 and 7)**: `CartContact` in Z80.
- Each new routine was checked by an emulator oracle (the original C rule recomputed on every call, also with randomized inputs): **0 mismatches**.

**Performance** — openMSX only (C-BIOS MSX2+, Z80 at 5.37 MHz), controlled-position script, renders/s, 0.9.99 → 0.9.103: Hill 14.8 → 14.4*, Mines 8.8 → 9.2, Bridge 6.3 → 6.9, Rapids 8.9 → 9.2, Pass 16.1 → 16.5, **City 6.6 → 8.6 (+30%)**, **Apple 12.4 → 15.0 (+21%)**, Grove 17.5 → 17.8, Bay 11.8 → 13.0, Swamp 11.3 → 11.2*, Boobus 7.2 → 6.6*, Castle 10.9 → 10.9. (*within the method's noise: the script replays the Keen's trajectory differently for each ROM; standing still at the same spot, Hill and Boobus are equal.) **Not tested on real hardware.**

**Known limits**: MSX2+ only; HUD bar garbled in R800 mode; Keen's physics at 35 Hz; occasional one-frame HUD-bar glitch in isolated captures; TIMP coverage is partial.
