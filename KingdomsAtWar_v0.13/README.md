# Kingdoms at War v0.13 — Desempenho

BigFive Studios · novembro de 2026

Jogo de estratégia em tempo real para MSX2 (C + Z80, MSXgl, ROM ASCII8 de 256 KiB). A v0.13 começou como versão de desempenho e acabou ganhando um adversário: as regras, a economia, o mouse, a seleção, o Fog of War e os assets humanos aprovados são os da v0.12, e agora há um **reino dos mortos** controlado por uma IA que constrói uma base, treina soldados e ataca em ondas, com vitória, derrota e reinício dentro do jogo. O relatório completo, com medições e decisões, está em [`../RELATORIO_v0.13.md`](../RELATORIO_v0.13.md).

## O que mudou na v0.13

- Rotinas quentes reescritas em Z80: Fog, comandos do V9990, rotas, combate, seleção de alvos, preparo e emissão de sprites, descompressão.
- V9990 com apresentação por interrupção de vertical blank, scroll incremental e Fog gravado no mundo.
- **Novo: `KAW9990P1.ROM`**, o V9990 em modo P1 (mapa de tiles com scroll por hardware e unidades como sprites). Com 4 unidades andando, roda a cerca de 30 quadros por segundo no openMSX a 3,58 MHz (29,6 medidos), contra cerca de 3,5 na v0.12. Nos cenários mais pesados (8 unidades grandes andando ou lutando) fica em 23 a 27; na primeira troca de golpes de uma onda inimiga contra 8 unidades grandes cai a cerca de 13,5 por uns 6 s (ver `../RELATORIO_v0.13.md`).
- A ROM `KAW68PV.ROM` (PREVIEW V9958) deixou de ser entregue.
- **Versão congelada:** os renderizadores bitmap V9990 (`KAW9990.ROM`) e V9968 (`KAW9968.ROM`) ficam congelados como binários no commit `3b6414d02c2941c67166ca010190f48b8dd5da9a` (marque com `git tag v0.13-final 3b6414d && git push origin v0.13-final`). A árvore de código atual compila **somente** `KAW9990P1.ROM`; o desenvolvimento segue só em P1.

- **Novo: facção inimiga, IA, defesa automática e fim de jogo** (seções abaixo). Tudo roda só na `KAW9990P1.ROM`.

## O reino dos mortos

O adversário é uma facção de múmias do deserto, com arte realista no estilo dos humanos: seis tipos de unidade (camponês, espadachim, lanceiro, arqueiro, cavaleiro de camelo e aríete-sarcófago) e oito tipos de prédio. As unidades usam uma paleta própria (grupo de paleta 1 dos sprites, sem custo por quadro). Os prédios usam a paleta do terreno, porque os tiles da camada B compartilham uma só: foram convertidos por uma rampa de luminosidade (`tools/convert_ramp.py`), com cinza para metal e cores próprias para vermelho e dourado. Os prédios inimigos têm versão tracejada no Fog lembrado e oclusão correta (a unidade ao lado do prédio passa por trás da borda dele). O canteiro de obras dos mortos também tem arte própria.

## IA inimiga: expansão e ondas

A IA (`src/banks/art_s23_b3.c`) não simula camponeses: tem uma economia abstrata (+1 madeira por segundo, +1 com serraria; ouro +2 a cada 3 s), com os mesmos custos dos humanos.

- **Expansão:** constrói um prédio por vez, com 15 s de obra, nesta ordem: quartel, serraria, casa, torre, estábulo, oficina de cerco, casa, torre. A base fica ao sul do mapa (a partir da linha 19) e deixa um corredor livre no meio. As torres inimigas atiram nas suas unidades dentro de seis células.
- **Treino:** os dois soldados inimigos (o motor só comporta dois atores inimigos ao mesmo tempo) são treinados de novo quando morrem: 15 s para espadachim, lanceiro e arqueiro, 25 s para cavaleiro e aríete, pagando o custo.
- **Ondas:** quando os dois estão vivos e passou o intervalo, saem juntos pelo corredor central e atacam a unidade ou o prédio humano mais próximo. A primeira onda sai aos 2:30 (guardas esqueleto); depois vêm lanceiro com arqueiro, cavaleiro com espadachim, cavaleiro com arqueiro, dois cavaleiros, aríete com cavaleiro, e dois cavaleiros se repetindo. O intervalo entre lançamentos é de 40 s.
- **Equilíbrio:** as constantes (`AI_FIRST_WAVE`, `AI_WAVE_GAP`, `AI_TRAIN_LIGHT/HEAVY`, a tabela de ondas) estão no topo do arquivo. Foram ajustadas por modelo (`tools/ai_timeline.py`), não por partidas reais: a IA só tem dois soldados por vez, então a pressão vem do ritmo e da qualidade, e um jogador competente com oito unidades e torres deve vencer sempre. Precisa de ajuste depois de partidas.

## Defesa automática

Um soldado parado, sem ordem e fora de **HOLD**, escolhe sozinho um inimigo visível a até 5 células e o persegue (aríete nunca persegue unidades). Ele guarda o posto onde estava e, se a perseguição o levar a mais de 8 células, desiste e volta; depois da luta também volta ao posto. Qualquer ordem do jogador apaga o posto. **HOLD** continua só revidando no alcance.

## Vitória, derrota e reinício

Se o castelo humano cai, é **derrota**; se o castelo inimigo cai, **vitória** (a derrota vence um empate). O jogo congela, a faixa do HUD mostra "VICTORY: ENEMY KEEP DESTROYED" ou "DEFEAT: YOUR KEEP DESTROYED" e a linha de baixo "PRESS ENTER TO PLAY AGAIN". **Enter** reinicia tudo do zero por um reset suave (desliga a interrupção do V9990 e salta para o vetor de reset da BIOS); a carga inicial se repete. O reset por `jp 0` não foi testado com BIOS real.

## Funcionalidades da v0.11 preservadas

**Torres** prontas disparam automaticamente contra unidades inimigas visíveis. Alcance de seis células até a borda da construção, flechas de dano 6, voo de 16 passos e recarga de 42 passos (1,4 s), com aquisição ociosa escalonada. Árvores, minas e construções intermediárias bloqueiam o disparo. Há seis vagas compartilhadas para flechas das torres; uma torre espera se estiverem ocupadas. Torres não atacam edifícios.

**Ferraria:** a pesquisa existente custa 16 de madeira e 16 de ouro, dura seis segundos e aplica +2 de ataque e +1 de armadura aos militares aliados atuais e futuros, incluindo o aríete. O bônus de ataque é somado antes do multiplicador do piqueiro contra cavalaria. A armadura reduz em um o dano recebido, com mínimo de um. Camponeses e construções não ganham armadura. Torres também recebem +2 de dano. O HUD militar mostra A (ataque efetivo) e D (armadura); a ferraria mostra ATK+2 ARMOR+1 após a conclusão.

**Reparação:** selecione camponeses e clique direito numa construção aliada pronta e danificada. Eles procuram a borda, usam a animação de trabalho e gastam uma unidade de madeira para recuperar até quatro pontos de vida a cada segundo por trabalhador. O último reparo custa uma madeira mesmo que faltem menos de quatro HP. Sem madeira, a tarefa aguarda. Movimento, coleta ou nova obra cancelam a reparação daquele camponês; a carga carregada é preservada. Reparar não cancela treinamento/pesquisa do edifício. Construções completas permanecem no máximo de HP, sem reconstrução gratuita de edifícios destruídos.

Os defensores inimigos também atacam construções aliadas adjacentes quando não há uma unidade adjacente para enfrentar. Isso permite pressão sobre obras e reparação sob ataque, mantendo os defensores estacionários.

**Fog of War:** área desconhecida oculta, terreno explorado fora da visão escurecido por uma trama de pixels e área visível com cores normais. A câmera não revela o mapa. Aliados e edifícios revelam uma área octogonal; cavaleiros e torres veem mais longe. Trabalhadores dentro da mina não são fontes de visão. Nesta versão, a visão atravessa obstáculos; o teste de linha de tiro continua separado.

O terreno escurecido guarda a última aparência observada. Mudanças ocultas não atualizam essa memória. Unidades inimigas e seus indicadores desaparecem fora da visão, não podem ser inspecionados e perdem travas de ataque dos aliados. Flechas já lançadas podem terminar o voo. Clicar direito em terreno desconhecido ordena exploração por movimento, sem revelar antecipadamente um recurso ou adversário. Recursos explorados podem receber ordens de coleta. Novas construções exigem a área inteira atualmente visível.

A atualização do Fog acompanha mudanças de célula/estado das fontes de visão. As áreas octogonais usam máscaras pré-calculadas em ROM; a leitura de entrada continua sendo amostrada durante o cálculo. A visão atual é consolidada por quadro e o percurso observado entre quadros é preservado. A composição restaura as regiões alteradas nos buffers ocultos; não há uma rotina periódica de apagar o mapa inteiro.

## Funções das unidades

| Unidade | Vida | Dano por golpe | Alcance (células) | Intervalo mínimo |
| --- | ---: | ---: | ---: | ---: |
| Espadachim | 48 | 8 | 1 | 0,8 s |
| Piqueiro | 48 | 7; 14 contra cavalaria | 2 | 1 s |
| Arqueiro | 32 | 5 | 4 | 1,2 s |
| Cavaleiro | 72 | 12 | 1 | 0,8 s |
| Aríete | 96 | 36, somente construções | 1 | 2 s |

O camponês mantém coleta e construção, com 32 pontos de vida. O cavaleiro conserva sua velocidade maior; o aríete é mais lento. A tabela contém os atributos básicos, antes da pesquisa. Os números são valores iniciais de balanceamento. Alcance usa distância Manhattan até a célula mais próxima do alvo: uma diagonal conta duas células.

Piqueiros atacam de duas células sem precisar encostar no adversário; o bônus contra cavalaria não garante vitória individual. Arqueiros lançam flechas visíveis, com voo de 16 passos da simulação (cerca de 0,53 s). O dano ocorre na chegada. Árvores, minas e construções intermediárias bloqueiam ataques a distância; arqueiros podem disparar sobre a água. Outras unidades não bloqueiam a trajetória. A flecha já lançada pode concluir seu voo após STOP ou a morte do atirador; um alvo morto ou da mesma facção não recebe o impacto.

## Seleção e comandos militares

Segure o botão esquerdo e arraste para selecionar aliados. Shift acrescenta à seleção. Clique simples continua selecionando uma unidade ou inspecionando o inimigo. Construções e adversários não entram no grupo por arrasto.

Clique direito em um defensor ou castelo inimigo ordena o ataque aos militares selecionados compatíveis com o alvo. Camponeses preservam suas tarefas; aríetes ignoram ordens contra unidades. A busca de caminhos reserva posições distintas, respeitando alcance e obstáculos. Clique direito em chão livre move o grupo; em árvore ou mina envia os camponeses para a coleta.

**S / STOP** interrompe ataque e aproximação dos militares selecionados. Um passo já iniciado termina. **H / HOLD** mantém posição e permite atacar unidades inimigas dentro do alcance de cada classe. Não há perseguição em HOLD (a defesa automática vale para soldados sem HOLD). Uma nova ordem de movimento ou ataque cancela a postura. Os botões do rodapé também funcionam com grupos mistos que contenham militares.

## Cenário de combate

O jogador inicia ao norte com dois camponeses e dois espadachins. Ao sul, na mesma margem do rio, fica o reino dos mortos: um castelo (colunas 3–6, linhas 27–30), dois guardas esqueleto (linha 23) e, com o tempo, os prédios que a IA constrói. Use as setas ou a borda da tela para explorar. A IA começa a construir no primeiro segundo e lança a primeira onda aos 2:30.

Barras mostram vida; a unidade militar selecionada mostra HP no HUD. Os ataques reutilizam poses existentes como gestos provisórios. A morte remove a unidade e libera sua vaga; o próximo recrutamento pode reutilizá-la. Destruir o castelo libera o terreno. A arte e as escalas aprovadas foram preservadas.

Esta versão não inclui animações definitivas de ataque, cadáveres nem ruínas. Os canteiros de obras dos dois reinos têm arte própria. Sem BIOS real, GFX9000 físico ou outro emulador, nada do que está aqui foi testado fora do openMSX.

## ROMs e execução

| ROM | Destino | Evidência |
|---|---|---|
| KAW9968.ROM (congelada) | V9968 principal/secundário | Compilação e recusa de V9958; congelada; script `v9968_shim.tcl` no kit; execução positiva no hardware pendente |
| KAW9990.ROM (congelada) | MSX2 + V9990/GFX9000, modo bitmap | Executada no openMSX pela saída GFX9000 |
| KAW9990P1.ROM | MSX2 + V9990/GFX9000, modo P1 (tiles e sprites) | Executada no openMSX pela saída GFX9000; mais rápida, carregamento inicial mais longo |

As três ROMs têm **256 KiB, mapper ASCII8**. Use um carregador compatível com esse mapper. Código e gráficos usam bancos separados; não há requisito de RAM Mapper. A leitura dos assets preserva o BIOS e restaura o banco de código.

```sh
./play-v9990.sh      # modo bitmap
./play-v9990-p1.sh   # modo P1 (kit KAW_V9968_TestKit)
```

Os scripts usam openMSX e conectam mouse MSX à porta 1. O jogo detecta o mouse nas duas portas e continua procurando enquanto ele estiver ausente. **M** refaz a detecção durante a partida, por exemplo após trocar a porta no emulador. Testes positivos no V9968 real e em mouse/MSX físicos continuam pendentes. No openMSX é preciso `set videosource GFX9000` para ver a saída do V9990.

## WebMSX e mouse

Abra `https://webmsx.org/?MOUSE_MODE=1` para habilitar mouse na porta 1. Este arquivo fica como referência do mouse. O autor relatou ter testado a P1 no WebMSX ("excelente", sem detalhes de versão); o suporte ao V9990 depende da versão do emulador.

Se habilitar o mouse depois do início, aguarde cerca de um segundo para a detecção. **M**, sem Alt, solicita nova procura nas duas portas. **Alt+M** é um comando do próprio WebMSX e alterna o modo do periférico. **Alt+Caps Lock** prende/libera o ponteiro no WebMSX. Clique esquerdo seleciona; direito dá ordens. O cursor MSX trabalha com deslocamento relativo, portanto pode ter posição diferente da seta do sistema quando o ponteiro estiver solto.

## Construções

Custos em madeira/ouro, tempos em segundos de trabalho efetivo do camponês.

| Construção | Custo | Tempo | Função e requisito |
|---|---:|---:|---|
| Castelo | Inicial | — | Depósito de ouro/madeira, recruta camponeses |
| Casa | 16/0 | 8 | +2 vagas de população, até o teto atual do motor |
| Quartel | 24/16 | 6 | Espadachim e piqueiro; arqueiro após serraria |
| Serraria | 24/8 | 8 | Depósito de madeira e requisito tecnológico |
| Ferraria | 24/16 | 8 | Requer quartel e serraria; pesquisa +2 ataque / +1 armadura |
| Estábulo | 32/24 | 8 | Requer ferraria; recruta cavaleiro |
| Torre | 24/8 | 8 | Requer serraria; dispara flechas contra unidades visíveis |
| Oficina de cerco | 40/24 | 8 | Requer ferraria; produz aríete |

Até 12 edifícios simultâneos, incluindo os dois castelos iniciais. O castelo e os edifícios maiores ocupam 4×4 células; casas ocupam 3×3. Edifícios do mesmo tipo podem ser repetidos. Cada obra admite um construtor; trabalhadores diferentes podem tocar obras diferentes ao mesmo tempo.

Dar outra ordem ao construtor pausa a obra. Clique direito na obra com um camponês selecionado para retomar. C ou clique direito no HUD cancela a obra selecionada/atribuída e devolve integralmente seu custo. Obras prontas não são demolidas por esse comando.

## Unidades e produção

| Unidade | Origem | Custo madeira/ouro | Tempo |
|---|---|---:|---:|
| Camponês | Castelo | 0/8 | 3 s |
| Espadachim | Quartel | 8/8 | 4 s |
| Lanceiro | Quartel | 8/8 | 4 s |
| Arqueiro | Quartel + serraria pronta | 8/12 | 4 s |
| Cavaleiro | Estábulo | 16/24 | 6 s |
| Aríete | Oficina | 24/16 | 6 s |

Quatro aliados iniciais: dois camponeses e dois espadachins. A população começa em 4; uma casa pronta aumenta para 6. **O teto simultâneo é oito unidades**, alcançado com duas casas. Os seis tipos podem coexistir na mesma partida.

Cada edifício possui uma vaga de produção independente. O custo e a vaga de população são reservados uma única vez. Saídas bloqueadas mantêm o produto pronto aguardando. Cancelar devolve custo e vaga. Clique direito no chão com o edifício selecionado define seu ponto de encontro; a unidade recém-criada recebe a ordem se o destino estiver livre.

Novos camponeses podem coletar, entrar/sair da mina e construir, independentemente do índice de sua unidade. A ferraria possui pesquisa única de 16/16, com 6 s de duração; seu nível é guardado para a implementação dos atributos militares. O nível da pesquisa ainda não aplica bônus ao combate.

Cavaleiros deslocam-se mais rápido; aríetes, mais devagar. O combate dos espadachins está ativo; os demais papéis militares serão ampliados nas próximas versões. Camponês, espadachim e arqueiro usam telas de 32×32; lanceiro, cavaleiro e aríete usam 48×48. A caminhada usa quatro fases em cada direção e avança conforme a distância percorrida.

## Controles

| Ação | Mouse | Teclado |
|---|---|---|
| Cursor | Movimento | I/J/K/L |
| Selecionar unidade/edifício | Esquerdo | Z |
| Acrescentar/remover seleção | Shift + esquerdo | Shift + Z |
| Selecionar todos | — | A |
| Mover/coletar/retomar obra/atacar inimigo | Direito | X |
| Parar militares | STOP no rodapé | S |
| Manter posição e reagir no alcance | HOLD no rodapé | H |
| Escolher construção | HOU/BAR/SAW/FOR/STA/TOW/SIE no rodapé, com camponês selecionado | Q ativa e percorre os tipos |
| Confirmar posicionamento | Esquerdo no terreno | Z |
| Cancelar posicionamento | Direito | Esc |
| Recrutar/pesquisar | Opção no rodapé com edifício selecionado | R aciona a opção principal |
| Cancelar obra/produção | Direito no rodapé | C |
| Ponto de encontro | Direito no chão com edifício selecionado | X |
| Câmera | Permanecer na borda do mapa | Setas |
| Perfil Classic/Advanced | — | Espaço |
| Reiniciar depois da vitória ou derrota | — | Enter |

No quartel, o rodapé oferece SWORD, SPEAR e ARCHER. Os tipos avançados exigem suas construções prontas. O contexto de posicionamento mostra nome, custo, validade O/X e L quando bloqueado por tecnologia. A câmera por borda só atua sobre o mapa, após cerca de 0,2 s. Arraste com o botão esquerdo para selecionar uma área; Shift acrescenta unidades ao grupo.

## Coleta, arte e HUD

Árvores: 32 unidades; carga: 8. A mina inicial passou a 240 para sustentar a árvore tecnológica. Madeira e ouro só entram no saldo na entrega. Ouro volta ao castelo; madeira procura o castelo ou serraria pronta mais próxima por caminho transitável mais curto. As duas entradas da mina são disputadas com reservas, inclusive por trabalhadores recrutados.

O cenário preserva as árvores compostas e suas sobreposições. O castelo usa 64×64 pixels, a casa 48×48 e os demais edifícios 64×64, com transparência sobre o terreno. Os arquivos aprovados de v0.6.3 foram integrados sem redesenho em `art07/native`. O gerador reproduzível é `tools/prepare09.py`.

V9990 P1: mapa de tiles em duas camadas com scroll por registrador, unidades e efeitos como sprites, retângulos pelo blitter no vertical blank. (Bitmap B1 e V9968: ver commit congelado.) A ordem de profundidade por posição dos pés e sobreposição dos edifícios. O HUD é preparado em área oculta e trocado inteiro; os números atualizam apenas os glifos alterados. Cliques curtos são amostrados durante esperas do VDP e entre os blocos de carregamento das animações.

## Compilação e testes

```sh
python3 tools/prepare09.py
python3 tools/prepare012.py
MSXGL_ROOT=/caminho/MSXgl bash ./build.sh V9990P1
bash tests/suite012.sh V9990P1
```

`build.sh` compila a variante `V9990P1` (as variantes bitmap e V9968 estão no commit congelado). `KAW_FAST=1` usa a otimização padrão do SDCC em vez de `OPTIMIZED` (compila em cerca de 3 minutos em vez de 9, com código um pouco maior).

MSXgl fixo em `6fd8858dce9a5a674529b78ec13d64262810eb34`, SDCC 4.6.0 #16555. Requer Node.js, Python/Pillow/NumPy, GCC para o teste de eventos do mouse, openMSX e C-BIOS. O SDK externo não acompanha o pacote.

`tests/suite012.sh` verifica a memória (código fixo abaixo de A000h, RAM com margem para a pilha) e roda os testes de lógica, desempenho (`perf012`), Fog, combate, trabalhadores, câmera, sobreposições, seleção por área com o mouse e, conforme a variante, defesa ou reino. A variante `V9990P1` roda também `occl_p1` (oclusão por prédios), `flash_p1` e `flashseq_p1` (clarão de dano), `mine_crop_p1`, `enemy_kinds`, `defend011` (defesa automática e limite de perseguição), `enemy_tower`, `endgame` (vitória, derrota e reinício) e `enemy_visual` (oclusão e tracejado dos prédios inimigos). Fora da suíte, `ai_enemy.tcl` e `ai_waves.tcl` testam a expansão e as ondas da IA, e `wave_perf.tcl`/`fps_wave.py` medem o desempenho com a IA. Resultado atual da `suite012.sh V9990P1`: 206 verificações, nenhuma falha. A IA, a defesa automática e o fim de jogo têm chaves em RAM (`ai_enabled`, `auto_defend`, `end_enabled`); `tests/helpers.tcl` as desliga nos testes antigos, que pressupõem a base inimiga vazia, salvo `KAW_AI=1`, `KAW_DEF=1` ou `KAW_END=1`. `tests/kernel012.tcl` valida a rotina Z80 do Fog contra a referência.

O openMSX **grava `z80_freq` nas configurações**. `tests/helpers.tcl` fixa 3,58 MHz em todo teste; sem isso, medições herdam uma frequência de execuções anteriores.

Todos os recursos dos testes são coletados por comandos normais. O depurador lê RAM/VRAM para verificar resultados, sem escrever saldos, unidades, obras ou posições, com exceção do kernel012 (dados sintéticos para validar a rotina Z80) e dos testes que montam cenários (`occl_p1`, `flash_p1`, `enemy_kinds`, `defend011`, `enemy_tower`, `endgame`, `enemy_visual`, `ai_waves`), que posicionam unidades e prédios ou forçam vida e contadores para produzir a situação testada. Ver `test_report.md` (v0.12) e `../RELATORIO_v0.13.md`.

## Próximas etapas

Jogar partidas reais para ajustar o equilíbrio da IA (primeira onda, intervalo, treino, ouro e tabela de ondas). Mais de dois soldados inimigos ao mesmo tempo (exige rever o layout da memória de sprites). Reduzir o custo da luta com muitas unidades grandes (o pior caso medido é de cerca de 13,5 quadros/s por uns 6 s). Testar em BIOS real e GFX9000 físico, em especial a varredura de teclado desligada (`SCNCNT`) e o reinício por `jp 0`. Animações definitivas de ataque, minimap, áudio e save/load ficam para etapas futuras. O rio continua como barreira; pontes não são colocadas automaticamente.
