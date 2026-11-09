# Kingdoms at War v0.13 — Desempenho

BigFive Studios · novembro de 2026

Jogo de estratégia em tempo real para MSX2 (C + Z80, MSXgl, ROM ASCII8 de 256 KiB). A v0.13 é uma versão de desempenho: as regras, a economia, o mouse, a seleção, o Fog of War e os assets aprovados são os da v0.12. O relatório completo, com medições e decisões, está em [`../RELATORIO_v0.13.md`](../RELATORIO_v0.13.md).

## O que mudou na v0.13

- Rotinas quentes reescritas em Z80: Fog, comandos do V9990, rotas, combate, seleção de alvos, preparo e emissão de sprites, descompressão.
- V9990 com apresentação por interrupção de vertical blank, scroll incremental e Fog gravado no mundo.
- **Novo: `KAW9990P1.ROM`**, o V9990 em modo P1 (mapa de tiles com scroll por hardware e unidades como sprites). Com 4 unidades andando, roda a cerca de 19 quadros por segundo no openMSX a 3,58 MHz, contra cerca de 3,5 na v0.12.
- A ROM `KAW68PV.ROM` (PREVIEW V9958) deixou de ser entregue.
- **Versão congelada:** os renderizadores bitmap V9990 (`KAW9990.ROM`) e V9968 (`KAW9968.ROM`) ficam congelados como binários no commit `3b6414d02c2941c67166ca010190f48b8dd5da9a` (marque com `git tag v0.13-final 3b6414d && git push origin v0.13-final`). A árvore de código atual compila **somente** `KAW9990P1.ROM`; o desenvolvimento segue só em P1.

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

**S / STOP** interrompe ataque e aproximação dos militares selecionados. Um passo já iniciado termina. **H / HOLD** mantém posição e permite atacar unidades inimigas dentro do alcance de cada classe. Não há perseguição em HOLD. Uma nova ordem de movimento ou ataque cancela a postura. Os botões do rodapé também funcionam com grupos mistos que contenham militares.

## Cenário de combate

O jogador inicia ao norte com dois camponeses e dois espadachins. Ao sul, na mesma margem do rio, ficam dois defensores com detalhes ocres (linha 23) e um castelo ocre (colunas 3–6, linhas 27–30). Use as setas ou a borda da tela para chegar ao acampamento. Os defensores reagem por proximidade, sem perseguir nem construir.

Barras mostram vida; a unidade militar selecionada mostra HP no HUD. Os ataques reutilizam poses existentes como gestos provisórios. A morte remove a unidade e libera sua vaga; o próximo recrutamento pode reutilizá-la. Destruir o castelo libera o terreno. A arte e as escalas aprovadas foram preservadas.

Esta versão não inclui IA de expansão, tela de vitória/derrota, novas animações definitivas, cadáveres ou ruínas.

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

Abra `https://webmsx.org/?MOUSE_MODE=1` para habilitar mouse na porta 1. O WebMSX não emula V9990 nem V9968; este arquivo fica como referência do mouse.

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

`tests/suite012.sh` verifica a memória (código fixo abaixo de A000h, RAM com margem para a pilha) e roda os testes de lógica, desempenho (`perf012`), Fog, combate, trabalhadores, câmera, sobreposições, seleção por área com o mouse e, conforme a variante, defesa ou reino. A variante `V9990P1` roda também `occl_p1` (oclusão por prédios) e `flash_p1` (clarão de dano). `tests/kernel012.tcl` valida a rotina Z80 do Fog contra a referência.

O openMSX **grava `z80_freq` nas configurações**. `tests/helpers.tcl` fixa 3,58 MHz em todo teste; sem isso, medições herdam uma frequência de execuções anteriores.

Todos os recursos dos testes são coletados por comandos normais. O depurador lê RAM/VRAM para verificar resultados, sem escrever saldos, unidades, obras ou posições, com exceção do kernel012 (dados sintéticos para validar a rotina Z80) e dos testes `occl_p1` e `flash_p1`, que posicionam unidades e forçam vida e contadores de dano para produzir as capturas. Ver `test_report.md` (v0.12) e `../RELATORIO_v0.13.md`.

## Próximas etapas

Comportamento adversário com movimentação e condições formais de vitória/derrota. Depois, balanceamento e animações definitivas de ataque. Minimap, áudio e save/load ficam para etapas futuras. O rio continua como barreira; pontes não são colocadas automaticamente. O conjunto representa um reino humano.
