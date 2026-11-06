# Relatório v0.13 — desempenho (parcial)

Base: v0.12 (ROMs reproduzidas bit a bit com MSXgl 6fd8858 / SDCC 4.6.0). Medição: openMSX 19.1, Z80 3,58 MHz, cenário `tests/perf012.tcl` (1 e 4 unidades), quadros apresentados por segundo emulado.

| Alvo | cenário | v0.12 | v0.13 | ganho |
|---|---|---:|---:|---:|
| V9990 | 1 unidade | 5,99 | 8,22 | +37% |
| V9990 | 4 unidades | 3,45 | 3,67 | +6% |
| V9968 (via script no openMSX) | 1 / 4 unidades | — | 3,62 / 2,08 | — |

**Continua lento.** Nenhum alvo chega perto de fluidez; o ganho é real mas modesto.

## Mudanças
1. Poses nativas guardadas sem compressão (`tools/prepare013.py`, bancos 16–21): o decodificador LZ em C custava ~360 ciclos/byte.
2. Upload de linhas de pose sem aritmética de 32 bits (`vwrite_row`): cache_frame caiu de ~35 ms para ~13 ms por chamada (4 unidades).
3. `fog_at`/`fog_art` em Z80; `damage_mask` com bordas pré-calculadas e só atores ativos.
4. SDCC `--max-allocs-per-node 50000`: código fixo menor (V9968 ganhou ~600 bytes de folga: termina em 9E04).

## Onde está o tempo (V9990, 4 unidades, por quadro ≈ 290 ms)
simulação (~8,6 ticks × 11,6 ms) ≈ 34%; `video_sprites` ≈ 47% (fog_apply/fog_cover/damage_mask/draw_actor); fog_update ≈ 9%.
Próximos alvos medidos: BFS (`nav_prepare` 31 ms/rota, `navigation`), fog_apply/fog_cover/fog_damage, clip_copy/paint_world nativos, structure_bars, apresentação assíncrona.

## Validação
`suite012.sh` V9990 e PREVIEW: 0 falhas (inclui combate, fog, trabalhadores, câmera, overlays, mouse). V9968 via shim: perf, fog011, combat011, worker011, camera011, overlay011 com 0 falhas. Não rodados: kernel/damage/native012 host scripts, kingdom011 em V9968, hardware real, WebMSX.

## Ferramentas novas
`tests/prof13.py` (perfil de PC, `GATE=função`), `tests/incl13.py` (tempo inclusivo por função).

## Atualização: V9990
- Envio de comandos LMMM em Z80 (um OTIR de R#32..45 em vez de ~12 chamadas de biblioteca) e fog_cover com cópia única para células vizinhas de mesmo estado: V9990, 4 unidades, 13 → 16 quadros em 3,5 s (≈4,6 quadros/s). 1 unidade ≈ 7,5 quadros/s.
- Testes V9990: fog011, camera011, overlay011, combat011, worker011, defense011 e 36 verificações de pixels do Fog sem falhas.

## Perfil atual (V9990, 4 unidades, amostragem de PC)
simulação ≈ 34% (simulate, navigation, nav_prepare, combat_tick), fog_update ≈ 10% (fog_merge 47% disso), composição/Fog/desenho ≈ 40%, entrada ≈ 7%.
30 quadros/s exigiria reescrever em Z80 os caminhos quentes (BFS, composição, Fog); micro-otimizações em C não chegam lá.

## Atualização: scroll, Fog e rotas
Medido com `tests/scroll13.tcl` (seta direita por 2,5 s, unidades paradas) e `tests/perf012.tcl`, openMSX, Z80 3,58 MHz.

| Alvo | scroll (quadros/s) | 1 unidade | 4 unidades |
|---|---:|---:|---:|
| V9990 v0.12→v0.13 | ~4 → 11,6 | 6,0 → 7,7 | 3,45 → 4,8 |
| V9968 via script | ~2,8 → 3,2 | — → 3,6 | — → 2,0 |

Mudanças: V9990 rola movendo os pixels da própria página (uma cópia) e pintando só as faixas expostas; o Fog do V9990 fica gravado no mundo residente (restaurar a tela é uma cópia, sem `fog_cover`); `fog_cover`/`fog_page_pending` reescritos por bytes; BFS sem laço de vizinhos e sem deslocamento variável; varredura ociosa de combate com rejeição por distância.
Testes: suíte completa V9990 (156 PASS) + 36 verificações de pixels do Fog; combat011, worker011, fog011, camera011 em V9990 e V9968 sem falhas.
O scroll do V9968 (caminho nativo, sem mundo residente) ainda é o pior caso medido.
