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

## Medição com Z80 acelerado (openMSX, `z80_freq`)
| Alvo | Clock | 1 unidade | 4 unidades | scroll |
|---|---|---:|---:|---:|
| V9990 | 3,58 MHz | 9,6 | 6,9 | 14 |
| V9990 | 7,16 MHz | 22,9 | 14,4 | 21,6 |
| V9990 | 10,7 MHz | 25 | 21,4 | 26,8 |
| V9968 (via script) | 3,58 MHz | 5,2 | 2,9 | 6 |
| V9968 (via script) | 10,7 MHz | 20 | 10 | 15,6 |
Em Z80 acelerado o jogo se aproxima de 25–27 quadros/s no V9990; a 3,58 MHz o custo de CPU domina. O VDP emulado não é acelerado nessas medidas.

## Correção de medição e estado a 3,58 MHz (Z80 padrão)
Descoberta: `set z80_freq` **persiste** nas configurações do openMSX. Depois dos testes em clock acelerado, vários resultados intermediários (e falhas de pose por escrita rápida demais na VRAM) vinham do Z80 a 10,7 MHz. Os testes agora fixam o clock sempre (`tests/helpers.tcl`); os kits usam arquivo de configuração próprio.
Medido de novo a 3,58 MHz com o laço principal sem espera ociosa (a inversão de página fica por conta da interrupção):
| Alvo | 1 unidade | 4 unidades | scroll |
|---|---:|---:|---:|
| V9990 | 9,6 | 6,9 | 14,4 |
| V9968 (via script) | 6,9 | 3,2 | 6,4 |
O laço livre não mexeu no V9990 (a CPU já estava ocupada) e deu ~30% no V9968 com 1 unidade. Cursor de teclado agora anda por quadro de vídeo (não por volta do laço). Testes: suíte V9990 (156), V9968 (worker/combat/fog/camera/overlay/kingdom/native012) sem falhas a 3,58 MHz.

## Atualização: desenho por ator (V9990)
- `draw_actor` sem divisões (`key/10`, `key%10` por `(key*205)>>11` e deslocamentos), largura da barra de vida em cache por ator, laço de prédios sem chamadas a `intersect`, barras de vida em uma cópia (tira [preenchimento|fundo]). 6,9 → 4,9 ms por ator.
- V9990 a 3,58 MHz: 1 unidade 10,3 quadros/s; 4 unidades 7,0; scroll 14,4.
- Pixels idênticos à versão anterior (capturas estáticas), scroll incremental idêntico, suíte V9990 156 PASS.
- `combat011`: as esperas por morte agora também esperam `combat_kills` (mesma corrida de leitura entre `unit_hp=0` e o contador em `die()`).

## Atualização: fog_update
- `fog_merge` (Z80): salta bytes com `next==visible` e sem trilha; bytes só com trilha já vista só têm a trilha zerada. `fog_stamp`: as linhas de máscara agora são combinadas por rotina Z80 (`stamp_rows`), sem `poll_input` no meio. Dois `poll_input` em volta do merge removidos.
- `fog_update`: ~45 → 22 ms por chamada (V9990, 4 unidades). 1 unidade 11,3 quadros/s; 4 unidades 7,5; scroll 14,4. V9968 (via script): 8,0 / 3,6 / 6,4.
- `kernel012.tcl` passou a gerar `seen ⊇ visible` (invariante real do jogo, que o salto rápido usa); 257 casos Z80 passam. Suíte V9990 156 PASS, 36 pixels do Fog, scroll incremental idêntico; V9968 (via script): fog/worker/combat/camera/overlay/kingdom sem falhas.

## Atualização: cola do video_sprites
- Retângulo de cada ator calculado com aritmética de bytes relativa à origem do desenho (sem somas de 16 bits repetidas): o código fixo encolheu ~0,8 KB e o laço de preparação caiu de ~18 para ~13 ms por quadro.
- `clip_copy` do V9990 em Z80 (`v9_clip_run`): recorte à janela e envio do comando LMMM sem argumentos na pilha.
- Tentativa descartada: marcação de dano (`damage_mask`) em Z80 não deu ganho medível (o custo está espalhado em pedaços de ~2 ms); revertida.
- V9990 a 3,58 MHz: 1 unidade 13,0 quadros/s; 4 unidades 8,1; scroll 15,2. V9968 (via script): 7,4 / 3,6 / 6,4.
- `refresh13.tcl` agora move uma unidade e rola a câmera antes de comparar scroll incremental com repintura total (idênticos nos dois alvos). Capturas estáticas idênticas; suíte V9990 156 PASS; V9968: fog/worker/combat/camera/overlay/kingdom/native012 sem falhas.

## Adendo: busca de rotas, damage_mask e Mouse_Read

- Busca de rotas (BFS) reescrita em Z80 (`nav_bfs_run`).
- `damage_mask` reescrito em Z80 (`dm_build`, `dm_mark`, `dm_queries`, `dm_closure`). A caixa do Fog no V9990 entra como consulta (`fog_query`).
- Resultado V9990 com 4 unidades andando e scroll: 28 quadros em 3,44 s (cerca de 8 fps), igual ao anterior. O `damage_mask` em Z80 não deu ganho mensurável.
- `Mouse_Read` em meia taxa: ganho de cerca de 1%, quebrou os testes de mouse; revertido.
- V9968: 4 unidades andando, 13 quadros em 3,6 s.
- Testes: suíte completa V9990 e V9968 sem falhas. Também corrigi kernel012.tcl e run_mouse.py, que procuravam o mapa do PREVIEW no V9968.
- Não aplicados, aguardam aprovação: simulação a 15 Hz, Fog menos frequente, barras de vida só ao selecionar ou sofrer dano.

## Adendo 2: simulação a 15 Hz, Fog, barras e preparo dos atores em Z80

- 15 Hz: testado e descartado. 4 unidades: 8,9 fps contra 8,1; 1 unidade: 12,1 fps contra 13,6.
- Limite de frequência do Fog e barras de inimigos só ao selecionar: sem efeito mensurável, revertidos.
- V9990: detecção de atores alterados (`sp_changed`) e descarte fora da tela (`sp_cull`) em Z80. 4 unidades andando com scroll: 31 quadros em 3,47 s (8,9 fps), contra 28 em 3,44 s (8,1 fps). Suíte completa e comparações de pixels sem falhas.
- Redesenho sem restauração por ator: não implementado. Apagar o ator antigo exige uma cópia por retângulo de qualquer forma, então não reduz o número de cópias.

## Adendo 3: laço de movimento do simulate em Z80

- `sim_fast` (Z80): passo de movimento dentro de uma célula. Sai antes de alterar qualquer estado quando a célula se completa, e o código C original assume.
- V9990: 4 unidades 31 quadros em 3,47 s (igual), 1 unidade 43 quadros em 3,0 s (antes 42). Ganho pequeno: o custo restante está nas rotas, no combate, na travessia de células e nas unidades paradas.
- V9968: 4 unidades 14 quadros em 3,6 s, 1 unidade 23 quadros em 2,97 s.
- Suítes completas V9990 e V9968 sem falhas.
