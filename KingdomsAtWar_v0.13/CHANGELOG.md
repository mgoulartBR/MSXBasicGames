# v0.13 — Desempenho (novembro de 2026)

- **KAW9990P1.ROM** (novo): V9990 em modo P1, com mapa de tiles e scroll por registrador, Fog como escolha de padrão, HUD em camada própria, unidades, barras de vida, setas, anéis, ponteiro e oclusão por prédios como sprites, e caixa de arrastar, barras de construção e fantasma de construção pintados pelo blitter no vertical blank. 4 unidades andando: cerca de 19 quadros/s (v0.12: cerca de 3,5).
- V9990 (modo bitmap): apresentação por interrupção, scroll incremental, Fog gravado no mundo residente, comandos do blitter em Z80. 4 unidades: cerca de 9,6 quadros/s.
- V9968: poses guardadas sem compressão, envio de poses em Z80, scroll incremental, composição por linha de peças. 4 unidades: cerca de 4,2 quadros/s.
- Comum: rotas com mapa de busca persistente e restauração em Z80, busca de alvos e subsistemas ociosos puláveis, Fog (`fog_merge`) sem varreduras redundantes, entrada por uma varredura de teclado por quadro.
- Removido: `KAW68PV.ROM` (PREVIEW V9958).
- **Desempenho P1:** ordenação dos atores e o laço de emissão de sprites (envio de pose, oclusão, barra de vida, bloco, anel) em Z80. 4 unidades andando: ≈29 quadros/s medidos sem as esperas ociosas do teste (`tests/fps_active.py`); o `perf012` bruto inclui essas esperas e dá ≈22.
- **Cenários pesados (`tests/heavy012.tcl`, `tests/fps_heavy.py`):** 8 unidades grandes (cavaleiros/aríetes de 48 px) andando e luta em grupo; o teste escreve ouro/unidades/tipos pelo depurador só para medir. Antes: 17,8 e 16,6 quadros/s; agora: 20,8 e 19,0. Mudanças: barra de vida, anel, preparo de unidades não-trabalhadoras e corte de visibilidade em Z80; largura das barras de prédios em cache; **a interrupção deixa de acionar a varredura de teclado da BIOS** (`SCNCNT`=255 a cada vertical blank; o jogo lê a matriz direto), que custava ≈4% do tempo. Cenário de 4 unidades: ≈30 quadros/s.
- **Vitória e derrota.** Uma vez por segundo `end_check` (em `art_s23_b3.c`): se o castelo humano (prédio 0) cai, derrota; se o castelo inimigo (prédio 1) cai, vitória (a derrota vence o empate). `game_over` (1 vitória, 2 derrota) congela a simulação (`simulate` sai no início; tempo, unidades, IA e ondas param), zera os clarões de ataque e mostra na faixa do HUD "VICTORY: ENEMY KEEP DESTROYED" ou "DEFEAT: YOUR KEEP DESTROYED" e, na linha de ações, "PRESS ENTER TO PLAY AGAIN". Seleção, câmera e mouse continuam funcionando. Chave `end_enabled` (ligada em `combat_init`); `tests/helpers.tcl` a desliga nos testes antigos (o `combat011` destrói o castelo inimigo e segue testando) salvo `KAW_END=1`. Teste `tests/endgame.tcl` (vitória, congelamento, prioridade da derrota, chave desligada). RAM +2 B (EF9A).
- **Equilíbrio da IA (ajuste por modelo, sem partidas reais).** Constantes no topo de `art_s23_b3.c`: primeira onda 200→150 s (`AI_FIRST_WAVE`), intervalo entre lançamentos 60→40 s (`AI_WAVE_GAP`), treino 20/30→15/25 s (`AI_TRAIN_LIGHT/HEAVY`), ouro +1 a cada 2 s → +2 a cada 3 s, tabela de ondas um degrau mais forte (guardas esqueleto, lanceiro+arqueiro, cavaleiro+espadachim, cavaleiro+arqueiro, dois cavaleiros, aríete+cavaleiro, repetindo dois cavaleiros). `tools/ai_timeline.py` simula a linha do tempo: com um jogador que vence cada onda em 35 s, os cavaleiros passam de 6:26 para 4:22 e há uma onda por minuto (antes, uma a cada ~1:06 após a primeira, aos 3:20); contra um jogador que demora 60 s por onda, cavaleiros aos 5:12 (antes 7:33). Limite estrutural: a IA só tem 2 soldados ao mesmo tempo, então a pressão vem do ritmo e da qualidade, não da quantidade; um jogador competente com 8 unidades e torres ainda deve vencer sempre. Ajustar depois de partidas reais. `tests/ai_waves.tcl` atualizado para a nova tabela.
- **Reinício dentro do jogo.** Depois da vitória ou derrota, Enter (`poll_input` → `game_restart` em `video.h`) desliga a interrupção do V9990 e salta para o vetor de reset da BIOS (`jp 0`): o cartucho sobe de novo do zero, como ao ligar o MSX (carga de ~45 s no openMSX). Mais seguro que reinicializar à mão mapa, fog, IA e vídeo. `tests/endgame.tcl` cobre derrota → Enter → novo jogo com relógio e castelos no estado inicial. Código fixo +29 B (971B), RAM igual.
- **Desempenho com a IA ativa (`tests/wave_perf.tcl`, `tests/fps_wave.py`).** Dois defeitos achados e corrigidos: (1) `init_world` não zerava `training`/`time` dos prédios; com a IA, `building_count` sobe até 12 e `sim_flags` lia lixo nos ids 2..10, ligando `production()` em todo tick (6% da CPU); (2) `sim_flags` não ignorava canteiros de obra inimigos (a IA cronometra as obras sozinha). Medidas (openMSX, 3,58 MHz, 8 unidades humanas grandes): código novo com a IA desligada = ROM anterior ±1 quadro/s (4 unidades 29,2→29,6; misto 27,4→26,6; 8 andando 24,3→23,5; luta em grupo 24,4→23,2). Com a IA e a defesa ligadas: marcha da onda ~26 quadros/s, primeiros 6 s de contato (8 unidades grandes + cavaleiro e arqueiro inimigos) 13,5 quadros/s (mediana 69 ms, p90 103 ms), depois ~22,5 quadros/s. O custo no contato se divide entre desenho de sprites (~35%), combate (~15%), simulação (~8%) e HUD (~3%), sem ponto único dominante.
- **Limite de perseguição.** O soldado em defesa automática guarda o posto (`auto_hx/auto_hy`, 255 = sem posto) no momento em que escolhe um alvo sozinho. Se o alvo o levar a mais de 8 células (`LEASH`, em qualquer eixo) do posto, a perseguição é abandonada (`work_stop`) e ele volta ao posto; depois da luta (alvo morto, fora de vista ou abandonado) também volta, parando quando chega a 1 célula do posto ou após 6 tentativas bloqueadas (`auto_try`). Novos inimigos dentro de 5 células ainda são enfrentados antes de voltar, mas só se o soldado estiver a até 8 células do posto. Ordens do jogador (mover, atacar, parar, segurar) apagam o posto. RAM +24 B (EF98). `tests/defend011.tcl` cobre a volta ao posto e o inimigo que foge (o soldado vai a 9 células, desiste e volta).
- **Torre inimiga testada.** `tests/enemy_tower.tcl`: torre pronta de dono 1 atira em soldado humano a 2 células (8 flechas, 6 de dano cada, o mata), não toca o soldado a 9 células nem unidades inimigas e para de atirar sem alvos. Nenhuma mudança de código: `tower_tick` já era independente do dono.
- **Defesa automática.** Soldado humano parado (sem ordem, fora de "segurar posição") escolhe sozinho um inimigo visível a até 5 células (`AGGRO` em `art_s26_b3.c`, via o mesmo `cand_scan` do combate) e o persegue pela navegação já existente; aríete nunca persegue unidades. "Segurar posição" continua só revidando no alcance. Chave `auto_defend` (RAM, ligada em `combat_init`); `tests/helpers.tcl` a desliga nos testes antigos (salvo `KAW_DEF=1`). Teste `tests/defend011.tcl` (parado ataca, em "segurar" não persegue). RAM +1 B, código fixo igual. O limite de perseguição vem no item seguinte.
- **IA inimiga: expansão e ondas (`src/banks/art_s23_b3.c`).** Economia abstrata (sem camponeses simulados: +1 madeira/s, +1 ouro a cada 2 s, +1 madeira/s com serraria), ordem de construção fixa dentro da base (quartel, serraria, casa, torre, estábulo, oficina, casa, torre), um prédio por vez com 15 s de obra (canteiro como o dos humanos), custos iguais aos dos humanos. Os dois soldados inimigos (slots 8 e 9) são treinados de novo quando morrem (20 s, 30 s para cavaleiro/aríete, com custo) e saem em onda quando os dois estão vivos e `game_seconds >= ai_launch` (1ª onda aos 200 s: os guardas esqueleto; depois espadachins, lanceiro+arqueiro, cavaleiro+espadachim, cavaleiro+arqueiro, dois cavaleiros, aríete+cavaleiro, repetindo a última). Movimento próprio (passo de 1 célula, 75 px/s, parado entre as células para o `combat_tick` escolher alvos), alvo = unidade ou prédio humano mais próximo, ponto de passagem (5,21) para atravessar a base. A base deixa livre o corredor x=4..7; as árvores de x=0 e x=10 (y=15..19) são removidas. RAM +24 B (`ai_*` em `main.c`), código fixo igual. Testes: `tests/ai_enemy.tcl` (expansão em ≤130 s, onda marcha até a base humana) e `tests/ai_waves.tcl` (baixas, treino, retorno com a composição seguinte, 3 ondas). `ai_enabled` desliga a IA; `tests/helpers.tcl` a desliga nos testes antigos (que supõem a base inimiga vazia) salvo `KAW_AI=1`. Limites conhecidos: só 2 soldados inimigos ao mesmo tempo (limite de atores `RN=10`); soldados humanos parados não revidam sozinhos (regra anterior do jogo); torres inimigas ainda não testadas contra unidades humanas.
- **Facção inimiga: prédios (primeira etapa).** Os 8 prédios dos Mortos (castelo, casa, quartel, serraria, forja, estábulo, torre, oficina de cerco) viram tiles extras da camada B: 92 tiles únicos (banco 22, LZ, 7,5 KB), padrões a partir de 1568 (depois do bloco dither), tabela `enemy_tile_map[tipo*16+deslocamento]`; `p1_cell` escolhe o tile inimigo quando a célula pertence a um prédio com `building_owner`. Conversão pela rampa de luminosidade (`tools/convert_ramp.py`, 5 tons de arenito + rampa cinza para metal) na paleta do terreno, sem custo em tempo de execução. Teste `tests/enemy_buildings.tcl` (capturas). Ainda sem: versão dither (Fog lembrado) e oclusão por sprite para prédios inimigos; canteiro de obras continua o mesmo.
- **Legibilidade da facção inimiga:** contorno de 1 px só nas bordas de baixo e da direita (sombra) no segundo tom mais escuro da paleta + curva de contraste; o contorno completo foi descartado por ficar forte demais frente aos humanos (`tools/style_variants.py contorno suave sombra contraste`; variantes testadas: médio, sombra, tracejado em `reference/mumia/final/contorno_variantes.png`). Antes: contorno externo completo + curva de contraste na paleta (`tools/style_variants.py contorno suave contraste`), aplicados às seis unidades (`reference/mumia/final/`). Sem custo em jogo (suíte e desempenho iguais; dados nos bancos 16–21).
- **Facção inimiga no estilo realista dos humanos.** As seis unidades dos mortos foram refeitas (folhas geradas com a referência humana em anexo; prompts em `PROMPTS_FACCAO_MORTOS.md`) e convertidas com `convert_units.py --realista` (moda por bloco, sem contorno preferido, cotas de matiz para vermelho, verde-azulado, aço e ouro; paleta única de 15 cores). Poses integradas por `integrate_units.py`; arte e conversões em `reference/mumia/realista/`. As versões cartoon ficam em `reference/mumia/unidades/` só como registro.
- **Facção inimiga: unidades completas.** Camponês-múmia (24 poses, com trabalho), arqueiro, lanceiro-chacal, cavaleiro de camelo e aríete-sarcófago, além do guerreiro-múmia, todos com UMA paleta de 15 cores do inimigo (`tools/convert_units.py`, `tools/integrate_units.py`). As poses passaram a ter chave contígua (`pose_base`, 208 chaves; `key_slot` posiciona cada pose nos bancos de 32 e 48 px da VRAM, que foram reorganizados: 32 px 192..639, 48 px 640..1599, arte dos prédios 1600..) e unidades inimigas (a partir do slot 8) usam `skin_map` (tipo lógico → tipo de pose) sem mudar regras nem tamanhos. Dados novos nos bancos 16–20. Carregamento inicial: +10 s no openMSX (37,5 → 47,5 s); desempenho em jogo inalterado. Os testes agora esperam 55 s no início.
- **Facção inimiga: mortos (múmias do deserto).** O soldado inimigo (tipo 6) agora é o guerreiro-múmia, convertido de folha gerada por IA (`tools/convert_sheet.py` → 32×32, 15 cores; `tools/integrate_enemy.py` reempacota as 16 poses no fim dos bancos 9–10 e atualiza limites, células e a paleta própria do inimigo, grupo 1 dos sprites). Arte e conversões em `reference/mumia/`. Desempenho inalterado.
- **Paleta própria do inimigo (infraestrutura):** sprites de unidades inimigas usam o grupo de paleta 1 do V9990 (bits 6–7 do byte de informação do sprite), sem custo por quadro; `team_opt` (0–4, padrão 0 = visual original) troca as cores de pano 6–8 do grupo. Opções de teste em `reference/identidade_inimigo.png`. A identidade final depende da arte que o autor definir.
- **Recorte na entrada/saída da mina:** a pose do camponês é copiada inteira para o sprite e as linhas de cima (4 px × passo da mina) são apagadas por um preenchimento do blitter (`p1_fill0`); a troca de pose considera pose e recorte (`actor_crop`). Teste `mine_crop_p1.tcl` captura um quadro por passo (`reference/recorte_mina.png`). Custo medido: nenhum nos cenários de 4 e 8 unidades.
- **Validação em outras máquinas** (`VALIDACAO.md`, `tests/machines_smoke.sh`, `KAW_MACHINE=`): C-BIOS MSX2/JP/BR/MSX2+ e as duas PAL (EU) passam; a ROM agora avisa na tela quando não há V9990 em vez de ficar em branco.
- **Combate a 15 Hz:** `combat_tick` roda em ticks pares; os contadores de golpe, dano e recarga descem de 2 em 2 (mesma duração em tempo real). Equivalência medida em `tests/combat_time.tcl` (duelos de 4,0 s nas duas versões, 51 golpes, 3 mortes, resultados iguais; golpe até 0,05 s mais tarde) e em capturas (`reference/combate_15hz.png`, `tests/duel_seq.tcl`). Luta: 24,0 → 24,9 fps; ocupado 42,2 → 40,5 ms/quadro.
- **Simulação e combate (terceira rodada):** `navigation` sai logo quando não há ordem pendente (`req_any`); `ct_scan` pula unidades ociosas sem alvo fora dos ticks de varredura (a cada 8); a caixa do alvo é calculada uma vez por unidade e tick (`cp_core`); `sim_scan` usa ponteiro de unidade em IX. Luta: tempo ocupado 44,6 → 42,2 ms/quadro (22,9 → 24,0 fps); corpo a corpo parado: CPU 56% → 54%; misto 27,1 → 27,2 fps. Tentativa descartada: reescrever a leitura das teclas em Z80 (sem ganho; o custo de entrada está no protocolo do mouse e no C ao redor).
- **Golpes só redesenham nas transições:** o gesto de ataque e o clarão de dano só marcam a tela como suja quando o contador cruza 4 (muda a pose) ou chega a 0, em vez de a cada tick; a imagem é idêntica pixel a pixel (`tests/flashseq_p1.tcl`, `reference/atacantes_transicoes.png`). Corpo a corpo parado sintético (`tests/melee012.tcl`, `melee_report.py`): 12,2 → 6,6 quadros/s desenhados e CPU 67% → 55%.
- **Barras e unidades paradas:** a barra de vida de unidade só aparece para selecionada, alvo inimigo selecionado ou ferida (antes: todo inimigo e todo alvo); unidades paradas não são reavaliadas (pose, posição, chave) enquanto continuam paradas, sem Fog novo e sem atacar, com renovação a cada 16 quadros; largura da barra limitada a 8 passos. Cenário misto (4 andando, 4 parados): 26,4 → 27,1 fps; luta: 22,6 → 23,4 fps (`reference/barras_selecionado_ferido.png`).
- **Reduções de desenho avaliadas (`reference/reducoes_desenho.png`):** com `draw_opt` (padrão 0, só diagnóstico; `KAW_DRAW=n tests/fps_heavy.py`) desliga-se: 1 barras de vida das unidades (−3,3 ms/quadro), 2 anéis (−1,4), 4 barras/bandeira dos prédios (−4,9), 8 barras só de unidade ferida (−1,2); todas juntas: −9,4 ms (20,7 → 25,2 fps na luta). A aparência foi mantida: as barras dos prédios agora ficam em cache (refeitas só se câmera, Fog, vida ou obras mudam), o que cortou o custo delas de 4,9 para 2,0 ms. Luta: 22,6 fps; 8 unidades: 24,3; 4 unidades: 29,7–30.
- **Luta em grupo (segunda rodada):** varredura de unidades da simulação (`sim_scan`), do combate (`ct_scan`), preparo de todos os atores (`units_prep`), ordenação com verificação prévia e laço de emissão em Z80; sprites só nas células 16×16 que têm pixels (`pose_cells`, derivada de `pose_bounds`, sem perda: as imagens de teste são idênticas). Tempo ocupado por quadro na luta: 57,2 → 47,6 ms. 8 unidades: 24,6 fps; luta: 21,8 fps (antes 20,8 e 19,0); 4 unidades: 30 fps. `tests/busy_heavy.py` mede o tempo ocupado sem a quantização do vertical blank.
- **Fog em duas fases:** `fog_update` só recalcula e funde a visibilidade; `fog_apply`, no quadro seguinte, atualiza memória, nomes de tiles e época (varredura de bits em Z80). Pior quadro do cenário de 4 unidades: 74 → 59 ms. A média fica em ≈29 fps, no limite da simulação a 30 Hz.
- **Simplificação:** código bitmap/V9968 removido da árvore (congelado no commit `3b6414d`); `video_p1.h` virou `video.h`; ≈1 KB de RAM liberado (RAM_end EE67); testes B1/V9968 removidos; `verify_fog_p1.py` novo.
- Testes: `suite012.sh` roda `V9990P1`; novos `occl_p1.tcl` e `flash_p1.tcl`; `perf012.tcl` mede 1 e 4 unidades; `tests/prof13.py`, `incl13.py` e `lineprof.py` para perfil.

# v0.12 — Desempenho

Fog estático em cache, stencils completos em ROM, kernel Z80, faixas de dano, sistemas ociosos suspensos, poses nativas recortadas sem perda e transferências em bloco. Resultados e limitações em test_report.md.

# v0.11 — Defesa, reparação e Fog of War (02/11/2026)

- Torres com aquisição automática, linha de tiro, flechas e recarga.
- Pesquisa da ferraria aplicada aos ataques e à proteção dos militares; atributos efetivos no HUD.
- Reparação por camponeses, com custo em madeira, animação, navegação e cancelamento.
- Defensores reagem a construções adjacentes quando não há unidade adjacente para enfrentar.
- Fog of War com desconhecido, memória do terreno e visão atual; seleção, ataques e posicionamento respeitam a visibilidade.
- Composição do Fog junto à restauração regional, preservando os buffers e o HUD.
- Arte aprovada e escalas mantidas; ROMs V9968, V9990 e PREVIEW.

# v0.10 — Funções militares (02/11/2026)

- Ataques para piqueiro, arqueiro, cavaleiro e aríete, mantendo o espadachim.
- Alcances e intervalos por classe; bônus do piqueiro contra cavalaria.
- Flechas visíveis com impacto após o voo e teste de obstrução.
- Aproximações de ataque respeitam alcance, obstáculos e reservas.
- STOP/HOLD, HP e contexto por classe, incluindo seleção em grupo.
- Aríete ataca somente construções; trabalhadores preservam tarefas nas ordens militares.
- Assets aprovados preservados. Torre, pesquisa e IA ampliada permanecem pendentes.

# v0.9.5 — Simulação, colisões e poses

- Reúne a atualização visual da mina e o movimento em uma passagem por unidade viva, reutilizando tipo e ponteiro.
- Percorre colisões por ponteiro, preservando posições, reservas e critérios de unidade viva.
- Consome referências LZ em blocos, preservando sobreposições e estado entre chamadas.
- Compara 360 mil transições de simulação, 720 mil consultas de colisão e mais de 8 milhões de bytes decodificados.
- Mede a carga de poses ausentes no cache da PREVIEW e a cadência nos cenários de 1/4/8 unidades.
- Arte, renderizador, velocidade e regras preservados. Há resultados mistos e picos restantes; V9968 físico pendente.

# v0.9.4 — Preparação de caminhos e lógica

- Perfil estatístico de PC identifica os custos em cenas com 1, 4 e 8 unidades.
- Prepara as 1.024 células de navegação com núcleo Z80, mantendo consultas de entrada por linha.
- Evita cálculos de coleta para camponeses sem tarefa e metadados de produção sem trabalho ativo.
- Preserva renderizador, arte, simulação, acumulador e regras.
- Confere o mapa de busca em 16 execuções reais e compara funções C antigas/novas em 144 mil transições.
- Compara cadência e chegada à v0.9.3. Picos e custos do cache ainda existem; V9968 físico pendente.

# v0.9.3 — Regiões recompostas e cadência

- Separa os atores cujo fundo precisa ser recomposto daqueles que precisam ser redesenhados por sobreposição.
- Restringe a invalidação de terreno às células alteradas e aos atores afetados.
- Considera as bordas da seleção/prévia; deixa de apagar o interior da prévia e recompõe bordas ao mover/cancelar.
- Dispensa a limpeza anterior dos atores quando o V9990 vai substituir o campo inteiro por mudança de câmera.
- Mede intervalos entre quadros e chamadas de recomposição, atores e caracteres, comparando com a v0.9.2.
- Preserva arte e regras. Execução positiva em V9968 físico pendente.

# v0.9.2 — Desempenho

- Envio dos registradores de comando SCREEN5 por sequência indireta de 15 bytes.
- Consulta de entrada durante espera do VDP limitada à mudança do relógio, preservando a primeira leitura.
- Tabelas para máscaras de repintura, coordenadas de blocos dos sprites e fases da velocidade.
- Avanço em lote dentro da célula com verificação de colisão e transições nas bordas preservadas.
- Teste diferencial de 240 mil transições; benchmark comparável de 1/4/8 unidades e regressão nas ROMs finais.
- Escolha de poses no banco 27 no destino nativo para preservar o limite do código fixo.
- Arte, controles e escopo do combate preservados. Execução positiva em V9968 físico pendente.

# v0.9.1 — Desempenho e seleção por área

- Seleção por arrasto com destaque durante o gesto, confirmação ao soltar, substituição e adição com Shift.
- Ordens coletivas de madeira/ouro para camponeses e de ataque para espadachins.
- Quantidade e tipo do grupo no HUD; clique individual preservado.
- Busca de alvos ociosos espaçada, atualização integral apenas durante combate/efeitos ativos.
- Remoção de repinturas ociosas periódicas; atores distantes/slots vazios descartados cedo.
- V9990 com restauração de regiões danificadas e cópia do HUD apenas quando mudou.
- Barras de aliados saudáveis fora de combate ficam concentradas no HUD.
- Composição do mapa e câmera movidas ao banco 27 para preservar o limite do código fixo.

# v0.9 — Primeiro combate

- Arena ao sul com dois espadachins defensores e castelo com estandarte ocre.
- Ataque contextual por clique direito para espadachins, com aproximação cardinal alcançável e reservada.
- Vida, dano, intervalo entre golpes, retaliação dos defensores, parada e manutenção de posição.
- Indicadores de vida, contato e movimento breve de ataque usando as poses existentes.
- Morte libera colisão/seleção/população; recrutamento reutiliza vagas.
- Destruição do castelo inimigo libera sua área.
- Entrada dos atalhos S/H capturada também durante esperas do VDP.
- Oito aliados e dois defensores em slots separados; renderização e cache protegem todos os atores visíveis.
- Fontes e escala aprovadas preservadas. Ataques das outras classes e IA de expansão ficam para as próximas etapas.

# v0.8.1 — Escala das unidades

- Piqueiro/lanceiro: desenho a 75%, aproximando o corpo da escala do espadachim e preservando a lança longa.
- Cavaleiro: desenho a 5/6, mantendo a silhueta montada proporcional ao restante do exército.
- Mesma escala nos 16 quadros de cada classe, com paleta indexada e apoio inferior preservados.
- Metadados de seleção recalculados a partir dos pixels efetivamente compilados.
- Assets compartilhados pelas ROMs V9968, PREVIEW e V9990; navegação e mouse da v0.8 preservados.

# v0.8 — Circulação e mouse

- Detecção contínua de mouse enquanto ausente, nas duas portas; M refaz a procura sem reiniciar.
- Leituras iniciais descartadas para sincronizar o protocolo ao conectar; conexão não gera clique fantasma.
- Captura de cliques também entre blocos de upload das animações, evitando perda de comandos com várias unidades ativas.
- Busca de caminho com múltiplas entradas e depósitos: escolhe a borda alcançável pelo menor percurso e reserva o destino.
- Camponês preserva a ordem e a carga em congestionamentos, com novas tentativas espaçadas; retoma quando o acesso é liberado.
- Construtor não reinicia uma busca de caminho ainda em andamento, permitindo alcançar obras distantes.
- Alternância de entrada entre mineradores em espera, sem que os primeiros ocupem continuamente as duas portas.
- Entradas validadas novamente ao terminar a busca, respeitando unidades e reservas que mudaram durante o cálculo.
- Decodificador em banco fixo permite interrupções de raster durante leitura dos assets; borda oculta dos buffers recebe fundo do painel para esconder linhas fora da área útil.
- Cenários de bloqueio das duas portas da mina, ativação tardia do mouse, troca de porta, oito aliados e construções/coleta simultâneas.

# v0.7 — Arte aprovada integrada à jogabilidade

- Integração dos oito edifícios e seis classes de v0.6.3 no jogo completo.
- Unidades de 32/48 pixels, caminhada de quatro fases por direção e 104 poses; edifícios de 48/64 pixels.
- ROMs ASCII8 de 256 KiB, com chamadas de código em bancos e leitura comprimida de assets preservando o BIOS.
- V9990 em bitmap B1: duas áreas de apresentação, mapa residente e todas as poses em VRAM.
- V9968/PREVIEW: buffers circulares com scroll fino, atualização de faixas expostas, cache de poses e HUD separado por interrupção.
- Profundidade pelos pés, seleção pela silhueta e ocultação das unidades atrás dos edifícios.
- Construção, coleta, mineração e recrutamento conectados aos novos tamanhos; duas casas permitem oito aliados.
- Cliques e teclas são amostrados durante esperas do VDP; a prévia de construção só muda ao trocar de célula/validade.
- Rio sem ponte pronta automática. Sem combate nesta etapa.

# v0.6 — Reino humano

- Oito construções com arte nativa derivada da direção visual aprovada: castelo 48×48 e sete edifícios 32×32.
- Seis classes de unidade, 56 quadros, cavaleiro e aríete em versões compactas 16×16.
- Novos camponeses recrutáveis e capazes de coletar, minerar e construir.
- Casas ampliam capacidade de quatro para seis, teto atual do renderer.
- Até 12 edifícios, obras e produções independentes, requisitos tecnológicos e pesquisa militar preparada para o combate.
- Serrarias recebem madeira; trabalhadores escolhem depósito por distância de grade.
- Ponto de encontro por edifício, cancelamentos com devolução e espera por saída livre.
- Menu de construções e tropas no mouse, contexto com custos e barras de progresso.
- ROMs de 48 KiB: gráficos imutáveis na página 0 e BIOS restaurado antes das interrupções; nenhuma dependência de RAM Mapper.
- Padrões V9990 preparados e compactados no boot; nativo usa slots dinâmicos por unidade.
- Mina ampliada para 240 unidades; área inicial de construção ampliada.

Esta é a primeira implementação do reino, sem combate, reparação ou bônus militares ativos. População e dimensões dos sprites ainda usam os limites desta etapa do motor.

# v0.5 — construção, recrutamento e mouse

- Quartel 32×32 com posicionamento válido/inválido, custo, obra de seis segundos, pausa, retomada e cancelamento com devolução.
- Treinamento de quatro segundos, reserva de vaga, espera por saída livre, cancelamento e limite de seis aliados.
- Unidades recrutadas participam de seleção, rotas e comandos de grupo.
- Mouse MSX nas portas 1/2: seleção, ordens, ações do HUD, cancelamento e rolagem pela borda.
- Barras de progresso e ações no HUD; atualizações em painel oculto.
- Cursor do HUD nativo composto no painel oculto, sem troca de tabela de sprites no raster.
- Assets compactados com LZSS de janela 256 para manter ROMs lineares de 32 KiB.
- Novos testes de produção, saídas bloqueadas e ciclo completo com mouse.

# v0.4.5 — personagens e coleta (31/10/2026)

- Dois camponeses com roupas próprias, barba, aventais e ferramentas; soldados com elmo, penacho e viseira.
- Apenas os quatro aliados; os dois vermelhos foram retirados da PoC e das variantes de sprites.
- Retirados os trechos decorativos de ponte sobre terra e o tile isolado de caminho junto ao castelo.
- Mina: aproximação pela frente, entrada animada, permanência oculta, saída com a carga e retorno à base.
- Cancelamento/troca de tarefa dentro da mina aguarda a saída e conserva materiais.
- Mina vazia permanece como cenário, permitindo a saída dos trabalhadores.
- Madeira: poses de machado levantado e de golpe alternadas durante o trabalho.
- Cenário/fontes compactados na ROM e expandidos só na inicialização; as três ROMs continuam em 32 KiB.

# v0.4.4 — personagens direcionais (31/10/2026)

- Trabalhadores com chapéu, ferramenta baixa e ação de machado/picareta.
- Soldados com capacete, armadura, escudo e espada; azul/vermelho distingue os exércitos.
- Quatro direções e duas poses de caminhada, preservando a direção ao parar.
- Duas camadas disjuntas no renderer compatível, priorizando silhuetas e cursor e controlando oito sprites por linha.
- V9990 expande os mesmos masters em padrões coloridos na inicialização.
- Unit permanece com 16 bytes; lógica de coleta/movimento e HUD da v0.4.3 preservados.

# v0.4.3

- Copas vizinhas compostas por pixel, com fundo transparente e 30 combinações pré-calculadas.
- Árvores evitam locais onde a copa seria cortada por água, pontes, edifícios ou limites do mapa.
- HUD restrito a madeira, ouro, tempo decorrido e vagas livres/limite atual.
- Removidos textos de atividade/seleção, retratos e instruções do painel.
- Caracteres alterados são desenhados no painel oculto; troca de painel pronto, sem apagar a área visível.
- Trabalho de HUD solicitado somente por mudanças dos valores; seleção e atividade não provocam redesenho.
- Faixa raster e comandos VDP com seções críticas menores para estabilizar a moldura.
- Água e pontes preservadas, sem implementar construção/destruição nesta etapa.
- Novos testes de composição, VRAM, estabilidade de números e quadros consecutivos.

FREE mostra 0/4 porque os quatro lugares disponíveis no motor atual estão ocupados pelos quatro aliados. Recrutamento ainda não está implementado.
