# v0.13 — Desempenho (novembro de 2026)

- **KAW9990P1.ROM** (novo): V9990 em modo P1, com mapa de tiles e scroll por registrador, Fog como escolha de padrão, HUD em camada própria, unidades, barras de vida, setas, anéis, ponteiro e oclusão por prédios como sprites, e caixa de arrastar, barras de construção e fantasma de construção pintados pelo blitter no vertical blank. 4 unidades andando: cerca de 19 quadros/s (v0.12: cerca de 3,5).
- V9990 (modo bitmap): apresentação por interrupção, scroll incremental, Fog gravado no mundo residente, comandos do blitter em Z80. 4 unidades: cerca de 9,6 quadros/s.
- V9968: poses guardadas sem compressão, envio de poses em Z80, scroll incremental, composição por linha de peças. 4 unidades: cerca de 4,2 quadros/s.
- Comum: rotas com mapa de busca persistente e restauração em Z80, busca de alvos e subsistemas ociosos puláveis, Fog (`fog_merge`) sem varreduras redundantes, entrada por uma varredura de teclado por quadro.
- Removido: `KAW68PV.ROM` (PREVIEW V9958).
- **Desempenho P1:** ordenação dos atores e o laço de emissão de sprites (envio de pose, oclusão, barra de vida, bloco, anel) em Z80. 4 unidades andando: ≈29 quadros/s medidos sem as esperas ociosas do teste (`tests/fps_active.py`); o `perf012` bruto inclui essas esperas e dá ≈22.
- **Cenários pesados (`tests/heavy012.tcl`, `tests/fps_heavy.py`):** 8 unidades grandes (cavaleiros/aríetes de 48 px) andando e luta em grupo; o teste escreve ouro/unidades/tipos pelo depurador só para medir. Antes: 17,8 e 16,6 quadros/s; agora: 20,8 e 19,0. Mudanças: barra de vida, anel, preparo de unidades não-trabalhadoras e corte de visibilidade em Z80; largura das barras de prédios em cache; **a interrupção deixa de acionar a varredura de teclado da BIOS** (`SCNCNT`=255 a cada vertical blank; o jogo lê a matriz direto), que custava ≈4% do tempo. Cenário de 4 unidades: ≈30 quadros/s.
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
