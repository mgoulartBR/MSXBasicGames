# Relatório v0.13 — desempenho, reino dos mortos e IA

> **Decisão (novembro de 2026):** o desenvolvimento segue somente em `KAW9990P1.ROM`. Os renderizadores bitmap V9990 e V9968 ficam congelados como binários no commit `3b6414d02c2941c67166ca010190f48b8dd5da9a` (tag `v0.13-final` não pôde ser enviada; criar com `git tag v0.13-final 3b6414d && git push origin v0.13-final`). A árvore de código atual foi simplificada para P1 apenas; `KAW9990.ROM` e `KAW9968.ROM` permanecem como binários de referência e já não são reproduzíveis a partir da árvore atual (use o commit congelado).

BigFive Studios · novembro de 2026 · ROMs reproduzidas com MSXgl 6fd8858 e SDCC 4.6.0.

## 1. Resumo

O pedido foi um ganho real de quadros por segundo no Z80 comum de 3,58 MHz, sem mudar regras, economia, mouse, seleção, Fog nem assets. A meta era 30 quadros por segundo. **Na P1 ela foi praticamente atingida**: 29,6 quadros/s com 4 unidades andando, medido no fim da fase da IA (ver a seção 10). Nos cenários pesados fica em 23 a 27 e o pior caso medido (os primeiros 6 s de contato de uma onda inimiga com 8 unidades grandes) em 13,5. Os números abaixo são os do início da v0.13, antes das otimizações seguintes, e ficam como referência histórica; o estado final está no fim desta seção e na seção 10:

| ROM | 4 unidades andando | 1 unidade andando | v0.12 (4 / 1 unidade) |
|---|---:|---:|---:|
| `KAW9990P1.ROM` (V9990, modo P1, início da v0.13) | 22,0 quadros/s | 24,2 quadros/s | — |
| `KAW9990.ROM` (V9990, modo bitmap) | 9,6 | 15,0 | 3,5 / 6,0 |
| `KAW9968.ROM` (V9968, via script no openMSX) | 4,2 | 8,3 | — / — |

Medição: openMSX 19.1, Z80 a 3,58 MHz, `tests/perf012.tcl`, janela de cerca de 3 s emulados com unidades se movendo e câmera parada. Quadros contados são os apresentados. A oscilação entre execuções vizinhas é de 2 a 3 quadros; diferenças menores que isso não são confiáveis. Todos os números deste documento são de emulação; **nada foi medido em hardware real**.

**Estado final da P1** (mesma metodologia: openMSX 19.1, 3,58 MHz, `tests/fps_active.py` e `tests/fps_heavy.py`; ROM de antes da IA → ROM atual com a IA desligada):

| Cenário | Antes da IA | Atual |
|---|---:|---:|
| 4 unidades andando | 29,2 quadros/s | 29,6 |
| Misto (4 andando, 4 paradas) | 27,4 | 26,6 |
| 8 unidades grandes andando | 24,3 | 23,5 |
| Luta em grupo | 24,4 | 23,2 |

Com a IA ligada, o custo está na seção 10.5. A ROM que mais se aproxima da meta é a P1. O V9968 continua bem abaixo: o caminho nativo tem de repintar atores e fundo por cópias de VRAM, e o V9958 emulado não tem o V9968 acelerado.

## 2. Entregáveis

| Arquivo | Destino | SHA-256 |
|---|---|---|
| `KAW9990P1.ROM` | MSX2 + V9990/GFX9000, modo P1 | `9cd2b27700fc5d84cd82a3f1434598e89e28341ba365e959837eb5c20f4d6aa1` (a ROM de antes da IA era `34611c87afed6fd0b7f8bde55a225af97545d5790530c8b157bfb3914ae9b415`) |
| `KAW9990.ROM` | MSX2 + V9990/GFX9000, modo bitmap | `7fd205da8b3b187db8d09f9d965d8cdd900e32704d03b2c6a31e488fa8c4fb4f` |
| `KAW9968.ROM` | MSX2+ com V9968 (e openMSX com o script do kit) | `9e7374f8b8efab6eb2aeaf8d6309ecbe6263c538088e84625b6295707a93a5c4` |

As três têm 256 KiB (ASCII8) e estão em `KingdomsAtWar_v0.13/` e em `KAW_V9968_TestKit/` (launchers para openMSX, `LEIA-ME.md`, `SHA256SUMS.txt`). A prévia `KAW68PV.ROM` (V9958) deixou de existir. Compilar o commit congelado reproduz estes três arquivos byte a byte; a árvore atual compila só a P1.

## 3. O modo P1 do V9990

O modo bitmap do V9990 obriga a restaurar fundo, redesenhar atores e inverter páginas a cada quadro. O modo P1 (tiles e sprites) elimina quase tudo isso.

- **Mundo:** mapa de tiles na camada B (64×64 nomes), com scroll por registrador. O Fog é a escolha do padrão de cada célula (desconhecido, visível, lembrado com pontilhado). Uma célula que muda custa dois nomes de 4 bytes.
- **HUD:** camada A, linhas 20 a 26, com fonte em padrões.
- **Atores:** sprites 16×16 em blocos (2×2 para poses de 32 px, 3×3 para 48 px). A pose é copiada de um banco em VRAM por **uma** cópia do blitter (os comandos do V9990 enxergam a memória de padrões como um bitmap de 256 px de largura). Cada ator tem dois blocos: a troca só acontece depois da cópia terminar.
- **Barras de vida, setas, anéis, ponteiro, clarão de dano:** sprites.
- **Oclusão por prédios:** os tiles de prédio (sem o fundo de gramado) viram sprites na frente da unidade que está atrás deles, com 14 vagas de padrão reaproveitadas por menor uso recente. A regra é a do modo bitmap: só prédios (nunca árvores), só em células visíveis.
- **Caixa de arrastar, barras e bandeira de construção, fantasma de construção:** retângulos pintados pelo blitter (LMMV) na camada A, nas linhas de nomes 0 a 19, que mostram uma memória de padrões de 256×160 px 1:1 com a tela.
- **Vertical blank:** a interrupção aplica juntos o scroll, a lista de sprites e a lista de retângulos, e atualiza o ponteiro do mouse. Isso eliminou o tremor. As gravações de nomes e de HUD feitas pelo programa principal rodam com interrupções desligadas.
- Mapa de VRAM completo no comentário de `src/video_p1.h`.

### Diferenças conhecidas em relação ao modo bitmap

- Barras de vida das unidades em passos de 2 px (para liberar padrões de sprite).
- O recorte da unidade na entrada e na saída da mina foi reproduzido: as linhas de cima da pose são apagadas (4 px por passo) na cópia da pose para o sprite (`tests/mine_crop_p1.tcl`, `reference/recorte_mina.png`).
- O Fog é recalculado no máximo a cada 20 ticks de vídeo (cerca de 0,33 s), então o Fog revela com até esse atraso e a visibilidade de inimigos em combate acompanha. Só vale para a P1.
- O carregamento inicial leva cerca de 36 s a 3,58 MHz (antes cerca de 25 s no modo bitmap): são mais padrões e a decodificação roda no Z80 uma vez.
- Cerca de 16 sprites por linha de varredura é o limite do chip (observado no protótipo; não medi o número exato). Grupos grandes de unidades lado a lado perto desse limite podem perder sprites.

## 4. Otimizações comuns às três ROMs

- **Fog:** `fog_merge` em Z80 com lista de bytes de trabalho, sem zerar 256 bytes a cada chamada; `fog_at` e `fog_art` em Z80; carimbo de visão em Z80.
- **Rotas:** mapa de busca guardado entre rotas, com restauração em Z80 (`nav_restore`) e a expansão da busca em Z80.
- **Combate e simulação:** caminho rápido em Z80 para o passo de movimento dentro de uma célula (`sim_fast`), busca de candidatos a alvo em Z80 (`cand_scan`), verificador (`sim_flags`) que pula economia, produção e torres quando não há trabalho, e pré-teste de alcance antes do teste caro.
- **Entrada:** uma varredura do teclado por quadro de vídeo.
- **V9990 bitmap:** apresentação pela interrupção de vertical blank, scroll deslocando a própria página, Fog gravado no mundo residente, comandos LMMM em Z80, detecção de atores alterados e descarte fora da tela em Z80.
- **V9968:** poses sem compressão, envio de poses em Z80, scroll incremental e cópia de uma linha de peças por vez.

## 5. O que foi tentado e não funcionou

Cada item foi medido e **revertido**:

| Tentativa | Resultado |
|---|---|
| Simulação a 15 Hz (modo bitmap) | 4 unidades 8,9 contra 8,1 quadros/s, 1 unidade 12,1 contra 13,6: pior com uma unidade |
| Simulação a 15 Hz (P1) | 4 unidades 12,9 contra 19,5, 1 unidade 13 contra 23,2: o desenho passa de 15 quadros/s e fica preso ao passo da simulação |
| Limitar o Fog e esconder barras de inimigos sem dano (modo bitmap) | sem efeito mensurável |
| Atualizar no máximo 2 atores em movimento por quadro (modo bitmap) | 30 contra 31 quadros: o desenho dos atores não era o gargalo |
| `damage_mask` em Z80 (modo bitmap) | 28 contra 28 quadros |
| Mouse a meia taxa | cerca de 1% de ganho, quebrava os testes de mouse |
| Não esperar o blitter ao copiar poses (P1) | sem mudança |
| Atualizar só os sprites que mudaram (P1) | não implementada: a cópia de atributos custa cerca de 2,5% do tempo |

## 6. Riscos e lições

- **Margem de pilha.** Na P1 a RAM livre entre as variáveis e a pilha chegou a cerca de 235 bytes e os testes passaram a travar (a pilha invadia dados). No fim da fase da IA `RAM_end` é EF9A, a cerca de 1000 bytes do limite de F380; a IA, as chaves de teste e o fim de jogo somaram menos de 50 bytes de RAM. A janela de 256 bytes do descompressor (só usada no carregamento) e o buffer de atributos de sprite compartilham a mesma RAM. Qualquer variável nova na P1 precisa checar `RAM_end` em `tests/check_memory092.py`.
- **Medição.** O openMSX grava `z80_freq` nas configurações e herda valores de execuções anteriores (por exemplo 10,7 MHz). `tests/helpers.tcl` fixa 3,58 MHz; os launchers do kit usam um arquivo de configuração próprio. Medidas antigas que mostravam 22 a 25 quadros/s eram desse artefato.
- **Perfil.** O tempo que o perfil por linha atribui a `video_sprites` inclui a interrupção de vertical blank, que roda no meio de qualquer linha.
- **Estado do V9968.** O openMSX não tem V9968; o kit roda a ROM inalterada no núcleo V9958 e emula só a leitura do ID por script. Execução no cartucho real continua pendente.
- **Limites conhecidos de teste.** Nada foi testado em hardware real ou com mouse físico. O autor relatou ter testado a P1 no WebMSX ("excelente", sem roteiro preenchido) e o recorte da mina ("tudo correto"); a IA, a defesa automática, as torres inimigas, o fim de jogo e o reinício só foram executados no openMSX.
- **Falhas silenciosas de compilação.** Duas vezes o teste rodou numa ROM antiga porque a compilação falhou (`build.sh` não imprime o erro no terminal; ver `build-V9990P1.log`). Conferir a linha `PASS V9990P1 fixed_end=… RAM_end=…` antes de confiar num resultado.

## 7. Validação

`bash tests/suite012.sh <variante>` com `V9990`, `V9968` ou `V9990P1` verifica a memória (código fixo abaixo de A000h, RAM com margem) e roda: `kernel012` (rotina Z80 do Fog contra a referência, 257 casos, 230272 bytes), `perf012`, `fog011`, `combat011`, `worker011`, `camera011`, `overlay011`, `perf091`, `defense011` (ou `kingdom011` no V9968) e `box011` (seleção por área com eventos de mouse injetados). Na P1 entram também `occl_p1` (oclusão) e `flash_p1` (clarão de dano). Resultado do início da v0.13: V9990 19 verificações sem falhas, V9968 19 sem falhas, V9990P1 23 sem falhas. **Estado atual (só P1):** `suite012.sh V9990P1` com 206 verificações e nenhuma falha, incluindo `flashseq_p1`, `mine_crop_p1`, `enemy_kinds`, `defend011`, `enemy_tower`, `endgame` e `enemy_visual`; `ai_enemy`, `ai_waves` e `wave_perf` rodam à parte.

Os testes de lógica leem RAM e VRAM sem escrever saldos nem posições. Exceções: `kernel012` (dados sintéticos), `occl_p1` e `flash_p1` (posicionam unidades e forçam vida e contadores de dano para gerar as capturas).

## 8. Como reproduzir

```sh
MSXGL_ROOT=/caminho/MSXgl bash KingdomsAtWar_v0.13/build.sh V9990 V9968 V9990P1
cd KingdomsAtWar_v0.13 && bash tests/suite012.sh V9990P1
```

`KAW_FAST=1` compila em cerca de 3 minutos (otimização padrão do SDCC) em vez de 9, com código um pouco maior e mais lento. As ROMs entregues são `OPTIMIZED`.

## 9. Próximos passos

- Jogar partidas reais e ajustar o equilíbrio da IA (a calibração atual é por modelo).
- Reduzir o custo da luta com muitas unidades grandes: o pior caso medido (13,5 quadros/s por cerca de 6 s) é espalhado entre desenho de sprites (~35%), combate (~15%), simulação (~8%) e HUD (~3%), sem ponto único.
- Canteiro de obras próprio dos mortos (arte nova) e mais de dois soldados inimigos ao mesmo tempo (exige rever o layout de memória de sprites).
- Testar a P1 em BIOS real, GFX9000 físico e outros emuladores, em especial a varredura de teclado desligada (`SCNCNT`), o reinício por `jp 0` e a cópia de tiles da linha 3072 da VRAM.

---

## 10. Fase seguinte: reino dos mortos, IA e fim de jogo

Depois do desempenho, o jogo ganhou um adversário completo. Tudo roda só na `KAW9990P1.ROM`.

### 10.1 Arte e integração

- **Unidades:** seis tipos de múmia (camponês, espadachim, lanceiro, arqueiro, cavaleiro de camelo, aríete-sarcófago), em estilo realista como os humanos (decisão do autor após comparar com uma versão cartoon). Folhas geradas pelo autor, convertidas por `tools/convert_units.py` (quantização com quotas de matiz) com contorno só nas bordas de baixo e da direita e curva de contraste (zero custo em tempo de execução), integradas por `tools/integrate_units.py`. Paleta própria no grupo 1 dos sprites.
- **Prédios:** oito tipos, convertidos por rampa de luminosidade (`tools/convert_ramp.py`: 5 tons de arenito, rampa cinza para metal, cores próprias para vermelho e dourado), porque os tiles da camada B compartilham a paleta do terreno. As tentativas anteriores (mapeamento por cor mais próxima, agrupamento por regiões) saíram enlameadas. Entram como **92 tiles únicos** (banco 22, 7,5 KB LZ) nos patterns 1568 em diante, com a versão tracejada do Fog lembrado (`ENEMY_DITH`). O V9990 aceita números de pattern bem acima de 2047 (sonda `tests/patlimit_probe.tcl`: 3000 funciona).
- **Oclusão:** os tiles inimigos ficam crus na VRAM a partir da linha 3072 (`EOV_Y`) e alimentam os sprites oclusores. Antes da correção, uma unidade ao lado de um prédio inimigo recebia por cima o tile do prédio humano de mesmo índice.
- A oficina de cerco é o prédio de pior legibilidade; o autor aceitou assim.

### 10.2 IA (`src/banks/art_s23_b3.c`)

Economia abstrata (sem camponeses simulados), obras em ordem fixa (15 s cada, um prédio por vez), treino dos dois atores inimigos (slots 8 e 9, o limite `RN=10` da memória de sprites), ondas com tabela de composições e movimento próprio (passo de uma célula, 75 px/s, parado entre as células para o `combat_tick` escolher alvos, ponto de passagem no corredor da base). A base deixa um corredor livre porque um passo guloso oscilava diante de uma fileira de prédios (achado no primeiro teste). Parâmetros de equilíbrio como constantes (primeira onda 150 s, intervalo 40 s, treino 15/25 s, ouro +2 a cada 3 s); ver `tools/ai_timeline.py`.

### 10.3 Defesa automática

Soldado parado sem HOLD persegue inimigos visíveis a até 5 células (`AGGRO`), guarda o posto e abandona a perseguição a mais de 8 células (`LEASH`), voltando ao posto. Ordens do jogador apagam o posto.

### 10.4 Vitória, derrota e reinício

`end_check` (uma vez por segundo) decide o fim; `game_over` congela `simulate`; o HUD mostra a mensagem; **Enter** reinicia por `jp 0` depois de silenciar a interrupção do V9990.

### 10.5 Desempenho com a IA

Dois defeitos foram achados na medição e corrigidos: `init_world` não zerava `training`/`time` dos prédios (com a IA, `building_count` sobe até 12 e `sim_flags` lia lixo, ligando `production()` em todo tick: 6% da CPU) e `sim_flags` contava canteiros inimigos como trabalho. Depois disso, numa onda de dois soldados (cavaleiro e arqueiro) contra 8 unidades humanas grandes com defesa automática: marcha até o contato por volta de 26 quadros/s; primeiros 6 s de contato 13,5 quadros/s (mediana 69 ms, p90 103 ms); depois cerca de 22,5. Perfil do contato: sprites ~35%, combate ~15%, simulação ~8%, HUD ~3%.

### 10.6 Testes

Chaves em RAM (`ai_enabled`, `auto_defend`, `end_enabled`) permitem manter os testes antigos, que pressupõem a base inimiga vazia: `tests/helpers.tcl` as desliga salvo `KAW_AI`, `KAW_DEF` ou `KAW_END`. Cenários novos: `ai_enemy` (8 prédios em ≤130 s, onda marcha até a base humana), `ai_waves` (baixas, treino, retorno com a composição seguinte, três ondas), `defend011`, `enemy_tower`, `endgame`, `enemy_visual`, `wave_perf`/`fps_wave.py`.

### 10.7 Limites e riscos

- A IA só tem dois soldados por vez: a pressão vem do ritmo e da qualidade. Um jogador competente com 8 unidades e torres deve vencer sempre; um jogador lento ou passivo toma uma onda a cada cerca de 1:20 com cavaleiros a partir dos 5 minutos. O equilíbrio é um chute informado por simulação, não por partidas.
- Soldados humanos sem HOLD agora se defendem sozinhos, mas operários não; a IA ataca a unidade ou prédio mais próximo, sem preferir operários.
- O reinício por `jp 0`, a varredura de teclado desligada e a cópia de tiles da linha 3072 só foram testados no openMSX com C-BIOS.
- O canteiro de obras inimigo é o dos humanos.
- O pior caso de desempenho (contato com muitas unidades grandes) continua em 13,5 quadros/s por cerca de 6 s.

---

# Anexo A — Histórico detalhado

Notas cronológicas do trabalho, preservadas como foram escritas. Os números de quadros por segundo desta parte são de etapas intermediárias e foram superados pela tabela da seção 1.

## A.1 Etapas iniciais e V9990/V9968 em modo bitmap (histórico)

Base: v0.12 (ROMs reproduzidas bit a bit com MSXgl 6fd8858 / SDCC 4.6.0). Medição: openMSX 19.1, Z80 3,58 MHz, cenário `tests/perf012.tcl` (1 e 4 unidades), quadros apresentados por segundo emulado.

| Alvo | cenário | v0.12 | v0.13 | ganho |
|---|---|---:|---:|---:|
| V9990 | 1 unidade | 5,99 | 8,22 | +37% |
| V9990 | 4 unidades | 3,45 | 3,67 | +6% |
| V9968 (via script no openMSX) | 1 / 4 unidades | — | 3,62 / 2,08 | — |

**Continua lento.** Nenhum alvo chega perto de fluidez; o ganho é real mas modesto.

### Mudanças
1. Poses nativas guardadas sem compressão (`tools/prepare013.py`, bancos 16–21): o decodificador LZ em C custava ~360 ciclos/byte.
2. Upload de linhas de pose sem aritmética de 32 bits (`vwrite_row`): cache_frame caiu de ~35 ms para ~13 ms por chamada (4 unidades).
3. `fog_at`/`fog_art` em Z80; `damage_mask` com bordas pré-calculadas e só atores ativos.
4. SDCC `--max-allocs-per-node 50000`: código fixo menor (V9968 ganhou ~600 bytes de folga: termina em 9E04).

### Onde está o tempo (V9990, 4 unidades, por quadro ≈ 290 ms)
simulação (~8,6 ticks × 11,6 ms) ≈ 34%; `video_sprites` ≈ 47% (fog_apply/fog_cover/damage_mask/draw_actor); fog_update ≈ 9%.
Próximos alvos medidos: BFS (`nav_prepare` 31 ms/rota, `navigation`), fog_apply/fog_cover/fog_damage, clip_copy/paint_world nativos, structure_bars, apresentação assíncrona.

### Validação
`suite012.sh` V9990 e PREVIEW: 0 falhas (inclui combate, fog, trabalhadores, câmera, overlays, mouse). V9968 via shim: perf, fog011, combat011, worker011, camera011, overlay011 com 0 falhas. Não rodados: kernel/damage/native012 host scripts, kingdom011 em V9968, hardware real, WebMSX.

### Ferramentas novas
`tests/prof13.py` (perfil de PC, `GATE=função`), `tests/incl13.py` (tempo inclusivo por função).

### Atualização: V9990
- Envio de comandos LMMM em Z80 (um OTIR de R#32..45 em vez de ~12 chamadas de biblioteca) e fog_cover com cópia única para células vizinhas de mesmo estado: V9990, 4 unidades, 13 → 16 quadros em 3,5 s (≈4,6 quadros/s). 1 unidade ≈ 7,5 quadros/s.
- Testes V9990: fog011, camera011, overlay011, combat011, worker011, defense011 e 36 verificações de pixels do Fog sem falhas.

### Perfil atual (V9990, 4 unidades, amostragem de PC)
simulação ≈ 34% (simulate, navigation, nav_prepare, combat_tick), fog_update ≈ 10% (fog_merge 47% disso), composição/Fog/desenho ≈ 40%, entrada ≈ 7%.
30 quadros/s exigiria reescrever em Z80 os caminhos quentes (BFS, composição, Fog); micro-otimizações em C não chegam lá.

### Atualização: scroll, Fog e rotas
Medido com `tests/scroll13.tcl` (seta direita por 2,5 s, unidades paradas) e `tests/perf012.tcl`, openMSX, Z80 3,58 MHz.

| Alvo | scroll (quadros/s) | 1 unidade | 4 unidades |
|---|---:|---:|---:|
| V9990 v0.12→v0.13 | ~4 → 11,6 | 6,0 → 7,7 | 3,45 → 4,8 |
| V9968 via script | ~2,8 → 3,2 | — → 3,6 | — → 2,0 |

Mudanças: V9990 rola movendo os pixels da própria página (uma cópia) e pintando só as faixas expostas; o Fog do V9990 fica gravado no mundo residente (restaurar a tela é uma cópia, sem `fog_cover`); `fog_cover`/`fog_page_pending` reescritos por bytes; BFS sem laço de vizinhos e sem deslocamento variável; varredura ociosa de combate com rejeição por distância.
Testes: suíte completa V9990 (156 PASS) + 36 verificações de pixels do Fog; combat011, worker011, fog011, camera011 em V9990 e V9968 sem falhas.
O scroll do V9968 (caminho nativo, sem mundo residente) ainda é o pior caso medido.

### Atualização: scroll incremental (V9990 e V9968)
Antes o scroll forçava restaurar e redesenhar todos os atores a cada passo de câmera. Agora só entram os atores alterados e os que tocam as faixas expostas (o buffer circular do V9968 e a página deslocada do V9990 mantêm válidos os pixels dos demais).

| Alvo | scroll (quadros/s) antes → agora |
|---|---:|
| V9990 | 11,6 → 13,2 (v0.12: ~4) |
| V9968 (via script) | 3,2 → 5,2 (v0.12: ~2,8) |

Validação: `tests/refresh13.tcl` + `tests/refresh13_check.py` comparam o campo de jogo após scroll incremental com uma repintura total forçada: idênticos em V9990 e V9968. Suíte completa V9990 (156 PASS) e fog011, camera011, overlay011, combat011, worker011 em V9968 sem falhas.
`unit_art_frame` do V9990 passou a ser bancada (liberou código fixo; fim do código fixo V9990 = 9CE3).

### Atualização: rotas
- O mapa de busca (`parent[]`) agora fica guardado entre rotas. Cada rota restaura só as células que enfileirou e as dos aliados bloqueados; o rebuild completo (`nav_prepare`, ~31 ms) só ocorre quando o terreno muda (construção, ruína, recurso esgotado). O BFS mantém a ordem E,S,O,N, então os caminhos são os mesmos.
- V9990, 4 unidades: `simulate` 11,6 → 8,8 ms por tick; `navigation` 4,4 → 2,7 ms; início de rota (`nav_begin`) ~14 ms contra ~31 ms só da preparação.
- Quadros/s: V9990 1 unidade 7,6; 4 unidades 5,1; scroll 13,2. V9968 (via script): 1 unidade 3,8; 4 unidades 2,2.
- Testes: suíte V9990 (156 PASS); V9968 via script: worker011, combat011, fog011 e kingdom011 (32 PASS).
- Ajuste de teste: em `combat011.tcl` a espera `allied_death` amostrava a morte do camponês no meio de `die()` (HP já zero, contador de mortes ainda não incrementado). Agora espera também `combat_kills>=2`. A corrida existia no teste; o jogo mais rápido só mudou a fase da amostragem.

### Atualização: composição do desenho (renderer nativo)
- Cada linha de peças de uma pose agora sai numa única cópia quando as peças são vizinhas no cache (48×48: 9 → 3 cópias; 32×32: 4 → 2).
- `clip_copy` com caminho direto quando não cruza a borda do buffer circular; `paint_world` com índices de célula de 8 bits; acerto de cache sem varredura (slot anterior do ator).
- V9968 (via script): 1 unidade 4,5 quadros/s; 4 unidades 2,5; scroll 6,0. V9990 inalterado: 7,3 / 5,1 / 13,6.
- Validação: 120 poses da ROM e 4948 pixels do cache (`native012.py`, agora para dados sem compressão); capturas estáticas idênticas à versão anterior; `refresh13` (scroll incremental = repintura total) idêntico; kingdom011, worker011, combat011, fog011, camera011, overlay011 no V9968; suíte V9990 156 PASS.
- Capturas do kingdom011 diferem da versão anterior só por estado de jogo (a versão mais rápida já tem o cavaleiro recrutado).

### Atualização: entrada e upload de poses
- Teclado: uma varredura da matriz por quadro de vídeo (`kbrow[9]`); os ~35 testes de tecla por quadro viraram consultas com máscara constante (antes cada um lia o PPI). `poll_input` chegava a 0,4 ms por chamada, 7% do tempo no cenário de 1 unidade.
- Upload de poses nativas em Z80 direto da janela da ROM, sem buffer intermediário nem aritmética de 32 bits: ~58 ms → bem menos por pose (V9968, 1 unidade: 14 → 16 quadros em 3,1 s). `video_cursor` do V9968 sem aritmética de 32 bits.
- Medido: V9968 (via script) 1 unidade 5,2 quadros/s, 4 unidades 2,8, scroll 6,0; V9990 7,6 / 5,1 / 13,2.
- Validação: `native012.py` (120 poses + 6198 pixels de cache), scroll incremental idêntico à repintura total, kingdom/worker/combat/fog/camera/overlay no V9968 e suíte V9990 completa (156 PASS) sem falhas.
- Observação de teste: o script `v9968_shim.tcl` procura a ROM ao lado dele; no ambiente de testes `tests/KAW9968.ROM` agora é link para `../KAW9968.ROM` (uma cópia velha fazia o jogo travar na detecção).

### Atualização: V9990 (Fog e composição)
- `fog_merge` em Z80 com salto rápido para bytes sem mudança e lista dos bytes de trabalho (`fog_work_idx`): 20 → 11 ms por atualização. As varreduras do Fog (laço de células e gravação no mundo) só percorrem essa lista.
- Gravação do Fog no mundo residente: células que apenas saem da visão recebem só o dither (sem repintar o tile), em corridas horizontais de uma cópia; tiles só para células recém-vistas.
- `fog_damage` usa a lista de atores ativos de `damage_mask` e nem roda sem atores visíveis. Detecção de mudança em `video_sprites` por ponteiros; laço de setas só se houver setas.
- V9990 medido: 1 unidade 8,6 quadros/s; 4 unidades 5,8; scroll 14,0 (v0.12: 6,0 / 3,45 / ~4).
- Testes: suíte V9990 + kernel012 (kernel Z80 do Fog contra a referência) + 36 pixels do Fog + scroll incremental idêntico + box011.
- Ajuste de teste: `defense011` `repair_animates` amostrava `repair_active` no instante em que `repair_tick` o limpa e reativa; agora procura o bit ativo por até 0,66 s.

### Atualização: V9990 com apresentação por interrupção e Fog em retângulo único
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

### Atualização: desenho por ator (V9990)
- `draw_actor` sem divisões (`key/10`, `key%10` por `(key*205)>>11` e deslocamentos), largura da barra de vida em cache por ator, laço de prédios sem chamadas a `intersect`, barras de vida em uma cópia (tira [preenchimento|fundo]). 6,9 → 4,9 ms por ator.
- V9990 a 3,58 MHz: 1 unidade 10,3 quadros/s; 4 unidades 7,0; scroll 14,4.
- Pixels idênticos à versão anterior (capturas estáticas), scroll incremental idêntico, suíte V9990 156 PASS.
- `combat011`: as esperas por morte agora também esperam `combat_kills` (mesma corrida de leitura entre `unit_hp=0` e o contador em `die()`).

### Atualização: fog_update
- `fog_merge` (Z80): salta bytes com `next==visible` e sem trilha; bytes só com trilha já vista só têm a trilha zerada. `fog_stamp`: as linhas de máscara agora são combinadas por rotina Z80 (`stamp_rows`), sem `poll_input` no meio. Dois `poll_input` em volta do merge removidos.
- `fog_update`: ~45 → 22 ms por chamada (V9990, 4 unidades). 1 unidade 11,3 quadros/s; 4 unidades 7,5; scroll 14,4. V9968 (via script): 8,0 / 3,6 / 6,4.
- `kernel012.tcl` passou a gerar `seen ⊇ visible` (invariante real do jogo, que o salto rápido usa); 257 casos Z80 passam. Suíte V9990 156 PASS, 36 pixels do Fog, scroll incremental idêntico; V9968 (via script): fog/worker/combat/camera/overlay/kingdom sem falhas.

### Atualização: cola do video_sprites
- Retângulo de cada ator calculado com aritmética de bytes relativa à origem do desenho (sem somas de 16 bits repetidas): o código fixo encolheu ~0,8 KB e o laço de preparação caiu de ~18 para ~13 ms por quadro.
- `clip_copy` do V9990 em Z80 (`v9_clip_run`): recorte à janela e envio do comando LMMM sem argumentos na pilha.
- Tentativa descartada: marcação de dano (`damage_mask`) em Z80 não deu ganho medível (o custo está espalhado em pedaços de ~2 ms); revertida.
- V9990 a 3,58 MHz: 1 unidade 13,0 quadros/s; 4 unidades 8,1; scroll 15,2. V9968 (via script): 7,4 / 3,6 / 6,4.
- `refresh13.tcl` agora move uma unidade e rola a câmera antes de comparar scroll incremental com repintura total (idênticos nos dois alvos). Capturas estáticas idênticas; suíte V9990 156 PASS; V9968: fog/worker/combat/camera/overlay/kingdom/native012 sem falhas.

### Adendo: busca de rotas, damage_mask e Mouse_Read

- Busca de rotas (BFS) reescrita em Z80 (`nav_bfs_run`).
- `damage_mask` reescrito em Z80 (`dm_build`, `dm_mark`, `dm_queries`, `dm_closure`). A caixa do Fog no V9990 entra como consulta (`fog_query`).
- Resultado V9990 com 4 unidades andando e scroll: 28 quadros em 3,44 s (cerca de 8 fps), igual ao anterior. O `damage_mask` em Z80 não deu ganho mensurável.
- `Mouse_Read` em meia taxa: ganho de cerca de 1%, quebrou os testes de mouse; revertido.
- V9968: 4 unidades andando, 13 quadros em 3,6 s.
- Testes: suíte completa V9990 e V9968 sem falhas. Também corrigi kernel012.tcl e run_mouse.py, que procuravam o mapa do PREVIEW no V9968.
- Não aplicados, aguardam aprovação: simulação a 15 Hz, Fog menos frequente, barras de vida só ao selecionar ou sofrer dano.

### Adendo 2: simulação a 15 Hz, Fog, barras e preparo dos atores em Z80

- 15 Hz: testado e descartado. 4 unidades: 8,9 fps contra 8,1; 1 unidade: 12,1 fps contra 13,6.
- Limite de frequência do Fog e barras de inimigos só ao selecionar: sem efeito mensurável, revertidos.
- V9990: detecção de atores alterados (`sp_changed`) e descarte fora da tela (`sp_cull`) em Z80. 4 unidades andando com scroll: 31 quadros em 3,47 s (8,9 fps), contra 28 em 3,44 s (8,1 fps). Suíte completa e comparações de pixels sem falhas.
- Redesenho sem restauração por ator: não implementado. Apagar o ator antigo exige uma cópia por retângulo de qualquer forma, então não reduz o número de cópias.

### Adendo 3: laço de movimento do simulate em Z80

- `sim_fast` (Z80): passo de movimento dentro de uma célula. Sai antes de alterar qualquer estado quando a célula se completa, e o código C original assume.
- V9990: 4 unidades 31 quadros em 3,47 s (igual), 1 unidade 43 quadros em 3,0 s (antes 42). Ganho pequeno: o custo restante está nas rotas, no combate, na travessia de células e nas unidades paradas.
- V9968: 4 unidades 14 quadros em 3,6 s, 1 unidade 23 quadros em 2,97 s.
- Suítes completas V9990 e V9968 sem falhas.

### Adendo 4: migração do V9990 para o modo P1 (variante experimental KAW9990P1.ROM)

- Mundo em tiles na camada B com scroll por registrador; Fog escolhido por tile (desconhecido, lembrado com dither, visível); HUD em tiles na camada A; unidades, anéis de seleção e ponteiro como sprites; poses copiadas pelo blitter.
- Ainda faltam: barras de vida, caixa de arrastar, setas, fantasma de construção, oclusão por prédios e árvores, recorte da animação de entrada na mina.
- Desempenho (V9990 a 3,58 MHz, 4 unidades andando com scroll): P1 59 quadros em 3,44 s (17,2 fps) contra 31 quadros em 3,47 s (8,9 fps) no modo B1. 1 unidade: 69 quadros em 2,97 s (23,2 fps) contra 43 em 3,0 s (14,3 fps).
- Testes de lógica sem falhas na P1: perf012, fog011, combat011, worker011, camera011, overlay011, perf091, defense011. kernel012 estourou o tempo limite de 180 s e não foi investigado.
- Boot (até o laço principal): cerca de 36 s emulados na P1, contra cerca de 25 s no B1.

### Adendo 5: tremor na P1

- Causas tratadas: scroll e atributos de sprite eram gravados em momentos diferentes do quadro; uma pose nova era copiada sobre a que estava na tela; o ponteiro era gravado fora do vertical blank.
- Agora: scroll e lista de sprites são aplicados juntos na interrupção de vertical blank; cada unidade tem dois blocos de pose e só troca depois que a cópia termina; o ponteiro é atualizado na mesma interrupção; gravações de tiles e HUD rodam com interrupções desligadas.
- Erro encontrado no caminho: células de sprite na última linha da grade (linhas 252 a 255) colidem com a tabela de atributos; ponteiro e anel foram movidos para a linha 14.
- Medido: 4 unidades 51 quadros em 3,41 s (15 fps); 1 unidade 68 quadros em 3,0 s (22,7 fps). Testes de lógica sem falhas.

### Adendo 6: Fog em P1

- Causa das "treliças": os padrões de tile em P1 são numerados `8 + 4*phy` (visível) e `768 + 4*phy` (lembrado, com pontilhado). O laço de carga percorre 192 tiles, então os padrões 190 e 191 caíam em 768 a 775 e apagavam o pontilhado dos tiles 0 e 1 (os mais comuns do gramado). Células lembradas apareciam sem pontilhado, em faixas em escada.
- Correção: o bloco de padrões pontilhados passou a começar em 800.
- Verificado: o mapa de nomes na VRAM coincide com o estado do Fog (0 divergências em 1024 células), e a captura da cena `fog011` mostra a área lembrada uniforme, como no modo B1.

### Adendo 7: caixa de arrastar em P1

- A camada A cobre a área do mundo (linhas de nomes 0 a 19) e mostra a memória de padrões como um bitmap 1:1 de 256x160 pixels. As quatro linhas de 1 pixel da caixa são pintadas com LMMV (cor 15) e apagadas com LMMV (cor 0), sem usar sprites e sem entrar no limite de sprites por linha.
- Os comandos são emitidos na interrupção de vertical blank, depois do ponteiro e dos sprites, só se o blitter estiver livre; senão ficam para o próximo quadro. A caixa nunca aparece pela metade.
- Banco de poses reorganizado: poses de 32 px em linhas de 32 e de 48 px em linhas de 48 (1104 linhas, antes 1632); padrões do HUD movidos para o índice 640 em diante.
- Verificado: `box011` (19 verificações) sem falhas; capturas com a caixa durante o arrasto e depois de soltar (sem resíduo). 4 unidades: 50 quadros em 3,44 s; 1 unidade: 67 quadros em 3,0 s.

### Adendo 8: barras de vida e setas em P1

- Barras de vida das unidades: sprites de 16x2 px com a largura já no padrão (17 larguras, 2 cores), mostrados para inimigos, feridos ou em combate, como no modo B1. Largura em cache por (vida, tipo) para evitar divisão a cada quadro.
- Setas: sprite de 16x16 com 4 orientações, no mesmo critério de visibilidade do modo B1.
- Barras e bandeiras de construção: retângulos pintados com LMMV na camada A, na mesma lista da caixa de arrastar, repintados no vertical blank.
- Não feito: clarão branco de dano (cor da barra e cruz) e fantasma de construção.
- Medido: 4 unidades 41 quadros em 3,44 s (11,9 fps); 1 unidade 53 quadros em 2,97 s (17,8 fps). Antes das barras e setas: 50 e 67. Testes combat011, defense011 e box011 sem falhas.

### Adendo 9: montagem de sprites em Z80, boot e pilha na P1

- Z80: escrita dos atributos de um ator (`emit_block`), descarte fora da tela (`sp_cull`), descompressor LZ do carregamento (`lz_decode`). Ordem de profundidade persistente entre quadros, tabela de células de pose (sem divisões por quadro), largura das barras em cache.
- Boot (até o laço principal): cerca de 36 s emulados (antes cerca de 42 s).
- Erro encontrado: com o crescimento das variáveis, a margem de pilha caiu para cerca de 235 bytes e os testes passaram a travar (a pilha invadia os dados). A janela do descompressor (256 bytes, usada só no carregamento) e o buffer de atributos de sprite agora compartilham a mesma RAM; margem atual de cerca de 490 bytes.
- Medido: 4 unidades 52 quadros em 3,44 s (15 fps); 1 unidade 61 quadros em 3,0 s (20 fps). Antes de barras e setas: 50 e 67; com barras e setas: 41 e 53. Testes de lógica e box011 sem falhas.

### Adendo 10: fantasma de construção em P1

- O contorno do edifício (lado 32, 48 ou 64 px; cor 10 se a posição é válida, cor 7 se não) entra na mesma lista de retângulos da caixa de arrastar e das barras de construção, pintado no vertical blank. Só aparece com o modo de colocação ativo e o mouse acima do HUD.
- A posição do mouse em modo de colocação agora marca o quadro como sujo quando muda (antes só o modo B1 redesenhava por outros caminhos).
- Verificado: `overlay011` (primeiro, movido e cancelado) e capturas dos três estados; demais testes sem falhas. 4 unidades: 52 quadros; 1 unidade: 61 quadros em 3,0 s.

### Adendo 11: oclusão por prédios em P1

- Mesma regra do modo B1: uma unidade cujos pés estão acima da borda inferior do prédio e cujo retângulo cobre células dele fica atrás das partes opacas do prédio. Árvores nunca ocultaram unidades no B1 e continuam assim.
- Implementação: a arte dos tiles de prédio (sem fundo de gramado) fica num banco de bitmap na VRAM; os tiles que cobrem a unidade viram sprites de 16x16 à frente dela (24 slots de padrão, copiados pelo blitter na primeira vez que são necessários e reaproveitados por menos usado recentemente). O prédio só oculta em células visíveis (sem Fog).
- Barras de vida das unidades passaram a ter largura em passos de 2 px (9 padrões por cor em vez de 17) para liberar padrões de sprite.
- Verificado por captura com camponeses ao lado do castelo; testes de lógica e box011 sem falhas. 4 unidades: 49 quadros em 3,44 s; 1 unidade: 55 quadros em 2,97 s (antes da oclusão: 52 e 61).

### Adendo 12: clarão de dano em P1

- Com `hurt_flash > 4` a barra da unidade fica branca (cor 15, fundo 12) e uma cruz branca de 5x5 px aparece abaixo dela, como no modo B1. Nove padrões de barra branca e um de cruz; os slots de oclusão passaram de 24 para 14 para liberar padrões.
- Verificado por captura (vida e `hurt_flash` forçados em duas unidades); testes de lógica, oclusão e box011 sem falhas. 4 unidades: 45 quadros em 3,44 s; 1 unidade: 56 quadros em 2,97 s.

### Adendo 13: fog_update em P1

- `fog_merge`: em vez de zerar `fog_need` e `fog_delta` (256 bytes) a cada chamada, zera só as entradas da lista de trabalho anterior; laço com DJNZ. Saídas idênticas (`kernel012`: 257 casos, 230272 bytes).
- P1: o Fog virou uma passada sobre as células com `fog_delta` (um nome de tile por célula), sem execuções.
- P1: sight recalculada no máximo a cada 20 ticks de vídeo (cerca de 0,33 s), como já aprovado; o Fog revela com até esse atraso.
- Medido (4 unidades): 47 para 52 quadros em 3,44 s; 1 unidade: 58 quadros em 2,97 s.
- A alteração do `fog_merge` vale para os três ROMs; V9990 e V9968 estão sendo recompilados e testados antes de publicar.
- V9990 e V9968 recompilados com o `fog_merge` novo (OPTIMIZED): suítes completas sem falhas nos dois.

### Adendo 14: simulate

- Z80: `nav_restore` (restaura o mapa de pais da busca de rotas), `cand_scan` (candidatos a alvo vizinhos em `combat_tick`), `sim_flags` (pula economia, produção e torres quando não há trabalho; produção ainda roda uma vez para zerar `builder_working`), `occ_scan` (ocupação de células em `free_at`, só na P1). `near_box` descarta prédios fora de alcance antes do teste banked.
- Medido: P1 4 unidades 52 para 57 quadros em 3,4 s; 1 unidade 58 para 63 em 3,0 s. V9990: 33 e 45 quadros; V9968: 15 e 25.
- V9990 e V9968 recompilados (OPTIMIZED): suítes completas sem falhas nos dois; P1: testes de lógica, oclusão, clarão, kernel012 e box011 sem falhas.

### Adendo 15: unit_art_frame e unit_position em P1

- `unit_prep` (banco 27, só P1) substitui as duas chamadas banked por atores: mesma regra de quadro, visibilidade e origem do desenho, com os testes de trabalhador e de máscara de obra avaliados uma vez. O código do modo B1 e do V9968 não mudou (V9990 e V9968 não precisaram ser recompilados).
- Medido: P1 4 unidades 57 para 63 quadros em 3,4 s (18,5 fps); 1 unidade 63 para 68 em 3,0 s (22,7 fps). Todos os testes de lógica, oclusão, clarão e box011 sem falhas.

### Adendo 16: video_sprites em P1

- Visibilidade dos prédios (`fog_target`) em cache por época do Fog; ordenação por profundidade com os valores de `py` copiados para um vetor local. A tentativa de não esperar o blitter ao copiar poses não mudou nada e foi revertida.
- Medido: 4 unidades 63 para 67 quadros em 3,44 s (19,5 fps); 1 unidade 68 para 69 em 2,97 s (23,2 fps). Todos os testes de lógica, oclusão, clarão, kernel012 e box011 sem falhas.
- Observação sobre o perfil: parte do tempo que o perfil por linha atribui a `video_sprites` é da interrupção de vertical blank (cópia de atributos de sprite e retângulos), que roda no meio de qualquer linha.
