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

## Atualização: scroll incremental (V9990 e V9968)
Antes o scroll forçava restaurar e redesenhar todos os atores a cada passo de câmera. Agora só entram os atores alterados e os que tocam as faixas expostas (o buffer circular do V9968 e a página deslocada do V9990 mantêm válidos os pixels dos demais).

| Alvo | scroll (quadros/s) antes → agora |
|---|---:|
| V9990 | 11,6 → 13,2 (v0.12: ~4) |
| V9968 (via script) | 3,2 → 5,2 (v0.12: ~2,8) |

Validação: `tests/refresh13.tcl` + `tests/refresh13_check.py` comparam o campo de jogo após scroll incremental com uma repintura total forçada: idênticos em V9990 e V9968. Suíte completa V9990 (156 PASS) e fog011, camera011, overlay011, combat011, worker011 em V9968 sem falhas.
`unit_art_frame` do V9990 passou a ser bancada (liberou código fixo; fim do código fixo V9990 = 9CE3).

## Atualização: rotas
- O mapa de busca (`parent[]`) agora fica guardado entre rotas. Cada rota restaura só as células que enfileirou e as dos aliados bloqueados; o rebuild completo (`nav_prepare`, ~31 ms) só ocorre quando o terreno muda (construção, ruína, recurso esgotado). O BFS mantém a ordem E,S,O,N, então os caminhos são os mesmos.
- V9990, 4 unidades: `simulate` 11,6 → 8,8 ms por tick; `navigation` 4,4 → 2,7 ms; início de rota (`nav_begin`) ~14 ms contra ~31 ms só da preparação.
- Quadros/s: V9990 1 unidade 7,6; 4 unidades 5,1; scroll 13,2. V9968 (via script): 1 unidade 3,8; 4 unidades 2,2.
- Testes: suíte V9990 (156 PASS); V9968 via script: worker011, combat011, fog011 e kingdom011 (32 PASS).
- Ajuste de teste: em `combat011.tcl` a espera `allied_death` amostrava a morte do camponês no meio de `die()` (HP já zero, contador de mortes ainda não incrementado). Agora espera também `combat_kills>=2`. A corrida existia no teste; o jogo mais rápido só mudou a fase da amostragem.

## Atualização: composição do desenho (renderer nativo)
- Cada linha de peças de uma pose agora sai numa única cópia quando as peças são vizinhas no cache (48×48: 9 → 3 cópias; 32×32: 4 → 2).
- `clip_copy` com caminho direto quando não cruza a borda do buffer circular; `paint_world` com índices de célula de 8 bits; acerto de cache sem varredura (slot anterior do ator).
- V9968 (via script): 1 unidade 4,5 quadros/s; 4 unidades 2,5; scroll 6,0. V9990 inalterado: 7,3 / 5,1 / 13,6.
- Validação: 120 poses da ROM e 4948 pixels do cache (`native012.py`, agora para dados sem compressão); capturas estáticas idênticas à versão anterior; `refresh13` (scroll incremental = repintura total) idêntico; kingdom011, worker011, combat011, fog011, camera011, overlay011 no V9968; suíte V9990 156 PASS.
- Capturas do kingdom011 diferem da versão anterior só por estado de jogo (a versão mais rápida já tem o cavaleiro recrutado).

## Atualização: entrada e upload de poses
- Teclado: uma varredura da matriz por quadro de vídeo (`kbrow[9]`); os ~35 testes de tecla por quadro viraram consultas com máscara constante (antes cada um lia o PPI). `poll_input` chegava a 0,4 ms por chamada, 7% do tempo no cenário de 1 unidade.
- Upload de poses nativas em Z80 direto da janela da ROM, sem buffer intermediário nem aritmética de 32 bits: ~58 ms → bem menos por pose (V9968, 1 unidade: 14 → 16 quadros em 3,1 s). `video_cursor` do V9968 sem aritmética de 32 bits.
- Medido: V9968 (via script) 1 unidade 5,2 quadros/s, 4 unidades 2,8, scroll 6,0; V9990 7,6 / 5,1 / 13,2.
- Validação: `native012.py` (120 poses + 6198 pixels de cache), scroll incremental idêntico à repintura total, kingdom/worker/combat/fog/camera/overlay no V9968 e suíte V9990 completa (156 PASS) sem falhas.
- Observação de teste: o script `v9968_shim.tcl` procura a ROM ao lado dele; no ambiente de testes `tests/KAW9968.ROM` agora é link para `../KAW9968.ROM` (uma cópia velha fazia o jogo travar na detecção).

## Atualização: V9990 (Fog e composição)
- `fog_merge` em Z80 com salto rápido para bytes sem mudança e lista dos bytes de trabalho (`fog_work_idx`): 20 → 11 ms por atualização. As varreduras do Fog (laço de células e gravação no mundo) só percorrem essa lista.
- Gravação do Fog no mundo residente: células que apenas saem da visão recebem só o dither (sem repintar o tile), em corridas horizontais de uma cópia; tiles só para células recém-vistas.
- `fog_damage` usa a lista de atores ativos de `damage_mask` e nem roda sem atores visíveis. Detecção de mudança em `video_sprites` por ponteiros; laço de setas só se houver setas.
- V9990 medido: 1 unidade 8,6 quadros/s; 4 unidades 5,8; scroll 14,0 (v0.12: 6,0 / 3,45 / ~4).
- Testes: suíte V9990 + kernel012 (kernel Z80 do Fog contra a referência) + 36 pixels do Fog + scroll incremental idêntico + box011.
- Ajuste de teste: `defense011` `repair_animates` amostrava `repair_active` no instante em que `repair_tick` o limpa e reativa; agora procura o bit ativo por até 0,66 s.

## Atualização: V9990 com apresentação por interrupção e Fog em retângulo único
- O V9990 inverte as páginas na interrupção de retrace vertical (gancho H.KEYI da BIOS, flag em P#6, R#17/R#18 em Z80). O laço principal não espera mais o retrace: simulação e desenho seguem enquanto a inversão está pendente (`video_sprites` só roda sem inversão pendente).
- Mudanças de Fog no V9990: todas as células alteradas da janela viram UM retângulo (cópia única do mundo residente) em vez de varreduras e cópias por trecho; `fog_damage` testa só esse retângulo.
- V9990: 1 unidade 9,6 quadros/s; 4 unidades 6,9; scroll 14 (v0.12: 6,0 / 3,45 / ~4).
- Validação: suíte V9990 completa (156 PASS), kernel012, 36 pixels do Fog, scroll incremental idêntico à repintura total, box011.
- Ferramenta nova `tests/lineprof.py`: perfil inclusivo por linha de C dentro de uma função.
