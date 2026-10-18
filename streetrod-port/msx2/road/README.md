# Protótipo da estrada de Street Rod para MSX2

Pista pseudo-3D (projeção por segmentos, modelo extraído de `SRSE.EXE`, ver `../../NOTES.md`) em
**Screen 5 (256x212, 16 cores), V9938**, com 2 páginas de VRAM e troca no vblank. Código 100% próprio
(C/SDCC + Z80 assembly). Os gráficos do original (cockpit, carro) **não** estão no repositório: `gen_assets.py` os extrai do `LIB2` da *sua* cópia
do jogo (`LIB2=.../LIB2 ./build.sh`) e embute na ROM. Sem `LIB2` o jogo roda com gráficos simples. ROM de 32 KB (código em 0x4000, assets LZ em 0x8000).

![Screen 5](docs/road_screen5.png)

## Controles (jogável)
| Tecla / joystick 1 | Ação |
|---|---|
| ↑ | acelerar |
| ↓ | frear |
| ← / → | direção |
| ESPAÇO ou botão A | subir marcha (câmbio manual) |
| SHIFT ou botão B | descer marcha |

Câmbio automático (carros com câmbio "Auto") troca sozinho. Fora da pista (|P| > 950) o motor perde potência. A zebra e
a pista seguem as curvas da pista de teste. `-DAUTOPILOT=1` liga o piloto automático (usado nos benchmarks).
Teste automatizado dos controles no openMSX: `control_test.tcl` (acelera, troca marchas, vira e freia).

## Corrida (arrancada contra um oponente)
- **Largada:** semáforo (vermelho, vermelho, verde em 3 s). Engatar a 1ª marcha antes do verde = **largada falsa** (derrota).
  Depois do verde a 1ª marcha entra sozinha ao passar de 2900 rpm. Câmbio automático em carros "Auto".
- **Pista:** 380 segmentos (arrancada, como no original). Vence quem cruza primeiro.
- **Resultado:** tela com `YOU WIN`/`YOU LOSE`, tempo (décimos de segundo), velocidade média; ESPAÇO reinicia.
- **Oponente:** mesmo modelo de carro/peças do jogo; câmbio manual troca em `rotação de corte - 100*habilidade` (parâmetro `a` do
  piloto, 20 = Mike ... 10 = The King), alivia nas curvas e não deixa a vantagem passar de ~9 segmentos (IA simplificada do original).
  Aparece na pista a partir de 2 segmentos à frente, na faixa da direita.
- Escolha: `CAR=17 OPP_CAR=12 OPP=0 SRSE_EXE=... HOT_DATA=... ./build.sh` (`OPP_CAR`: carro do oponente; `OPP`: piloto 0-21).
- Testes automatizados no openMSX: `race_test.tcl` (corrida com piloto automático `-DAUTOPILOT=1`) e `race_manual.tcl`
  (largada falsa → reinício com ESPAÇO → corrida completa com trocas manuais).
- Medido: ~9,5 fps com oponente na pista (janela 192x80). Corrida de exemplo (Thunderbird vs Mike): vitória em 21,7 s, média 99 mph.

## Como rodar
```
sudo apt install sdcc openmsx xvfb          # + C-BIOS (já vem com o openmsx)
CAR=17 SRSE_EXE=SRSE.EXE HOT_DATA=HOT_DATA ./build.sh   # gera road.rom (janela 192x80, modo 2, passo 4)
./bench.sh "-DDRAW_MODE=2 -DSTEP_SH=2" saida.png   # mede fps no openMSX (C-BIOS MSX2, 60 Hz) e salva a tela
./matrix.sh < configs.txt                    # várias configurações (linha: descricao|flags)
```
Flags: `DRAW_MODE` (0 = gramado em retângulo por faixa + zebra/pista; 2 = cada pixel pintado 1x;
4 = só pista, supõe fundo estático), `STEP_SH` (log2 de scanlines por retângulo), `VIEW_BOT`,
`VIEW_X0/X1` (janela), `DRY` (não emite comandos ao VDP: mede só a CPU).

## Física do carro (modelo real do jogo)
`gen_tables.py` lê o modelo de desempenho extraído do EXE (`../../tools/powertrain.py`) e gera as tabelas de marchas/curva de
torque do carro escolhido: `CAR=17 SRSE_EXE=... HOT_DATA=... ./build.sh ...` (sem os arquivos usa uma curva padrão).
A física roda a **6,0688 Hz** como no original (72,826/12 Hz) e a pista avança como o original a ~8,09 fps equivalentes.
Medido no openMSX (câmbio automático simples):

| Carro | 0-60 mph | 0-100 mph | máxima |
|---|---|---|---|
| 1960 Ford Thunderbird | 3,6 s | 6,6 s | 117 mph |
| 1953 Buick Skylark Conv. | 3,9 s | 7,5 s | 115 mph |
| 1934 Ford Woodie | 4,4 s | 8,3 s | 108 mph |
| 1939 Pontiac 2-dr coupe | 16,7 s | - | 81 mph |

(a simulação em Python `tools/physsim.py` prevê 3,2 s / 16 s para Thunderbird / Pontiac)

## Medições (openMSX, tempo emulado, 20 s)
O original ficava limitado a **~8 fps** (relógio de 72,8 Hz, 9 ticks por frame).

| Janela | Modo | fps | só CPU (DRY) |
|---|---|---|---|
| 256x112 | gramado por faixa, passo 4 | 7,1 | 15,0 |
| 256x112 | só pista (fundo estático), passo 4 | 8,4 | 17,8 |
| 192x80 | pixel 1x, passo 4 | 10,1 | – |
| 192x80 | só pista, passo 4 | 12,1 | – |
| 192x80 | só pista, passo 2 (mais suave) | 11,3 | – |
| 160x56 | pixel 1x, passo 4 | 14,1 | 22,9 |
| 160x56 | só pista, passo 4 | 16,1 | – |

Primeira versão em C puro: **3,4 fps** (janela 256x112).

## Custos medidos
- VDP: `HMMV` ≈ **2,76 µs/byte** (≈ 360 KB/s) e ≈ 160 µs de emissão por comando. Janela 256x112 pintada uma vez
  = 14,3 KB ≈ 40 ms só no VDP.
- CPU (Z80 3,58 MHz, openMSX): `project` (15 linhas) ≈ 13 ms; laço das faixas ≈ 33 ms; física (7 ticks) ≈ 6 ms.
- SDCC: multiplicação `long` ≈ 4,9 mil ciclos; foi substituída por rotinas em assembly (16x8 e 16x16).

## Lições (armadilhas)
- IX é o ponteiro de frame do SDCC: rotinas em assembly devem preservá-lo.
- Zerar a BSS no crt0 (flags com lixo desligaram o VDP e mascararam as medições).
- Escritas em pares no VDP (`R#17`, status) precisam de DI/EI curtos; a ISR da BIOS lê o status 0.
- `jr` não alcança rótulos distantes no assembler do SDCC; usar `jp`.

## Próximos passos de otimização
1. Física/projeção/laço em assembly puro (hoje ~50 ms de CPU por frame; dá para ~25 ms).
2. Gramado por rotação de paleta (só a pista é repintada) -> linha "só pista" da tabela.
3. Emitir menos registros por comando; juntar DI/EI do polling.
4. Janela do tamanho do para-brisa original (≈ 205x45): ~16 fps medidos com 160x56.
5. MSX turbo R (R800): CPU ~5-10x; o VDP continua igual, então o limite passa a ser o `HMMV`.

## Gráficos do original (cockpit)
- `gen_assets.py`: lê o item 14 da `LIB2` (cockpit 320x190), reduz para 256 colunas (descarta 1 coluna em 5, preservando o dither), mapeia as cores EGA
  para a paleta MSX2 (`palette_def.py`) e comprime em faixas de 4 KB (LZ4-like, decodificador Z80 de ~60 bytes em `vdpasm.s`). Também gera o carro
  visto por trás (item 248, fundo transparente por flood-fill) em 9 tamanhos, os ponteiros dos mostradores e `assets.h`.
- Na carga a tela é apagada e as faixas vão para a VRAM: cockpit nas páginas 0 e 1 (as duas telas) e uma cópia limpa na página 2; sprites na página 3.
- A estrada é desenhada só no vidro (x 0..256, y 46..94: termina na linha do capô, com as linhas ciano e o painel sempre intactos). A moldura da esquerda (porta/coluna A) é recopiada por cima em ~14 retângulos HMMM. Tacômetro e velocímetro: a cada mudança, `HMMM` restaura o fundo
  limpo (página 2) e `LINE` desenha o ponteiro; o oponente é um `LMMM` com TIMP (cor 0 transparente) da folha de sprites.
- A ROM de 32 KB usa ENASLT no início para mapear o cartucho também na página 2 da CPU.
- `./shot.sh "<flags>" "t1 t2" prefixo` salva capturas de tela (VRAM→PNG) no openMSX.

## Semáforo e som
- Semáforo: item 13 do `LIB2` (duas cabeças de 24x32) em 3 estados gerados por recolorização (apagado, vermelho, verde), copiado por HMMM da folha de
  sprites; some ao fim de ~2 s da largada (a região é restaurada da cópia limpa do cockpit).
- Som no PSG (escrita direta nas portas 0xA0/0xA1): motor = 2 tons (canais A/B) com frequência `rpm/15` Hz e volume maior com acelerador; bipes do
  semáforo no canal C (2 graves e 1 agudo e longo na largada). O mixer escreve `0xB8` (bit 7 = porta B como saída, necessária à BIOS para ler joystick).

## Menu de seleção (carro, oponente, piloto)
Depois da tela de título aparece o menu: ↑/↓ escolhe o campo (seu carro / carro do oponente / piloto) e ←/→ troca o valor (segurar repete); ESPAÇO
inicia. Ao fim da corrida, ESPAÇO volta ao menu. Os 26 carros e 22 pilotos vêm de `HOT_DATA`/`SRSE.EXE`: `gen_tables.py` gera `cars.h` com as marchas,
curvas de torque e linha vermelha de cada carro (modelo `tools/powertrain.py`) e a habilidade do piloto; o oponente troca de marcha em
`rotação de corte - 100*habilidade`. Vista traseira do oponente: 3 modelos (sedã genérico, perua/woodie, Corvette) e cor de pintura sorteada
(cor 12 da paleta). Os assets ficam no fim da ROM de 32 KB (hoje ~0,3 KB de folga: próximo passo é uma ROM maior com mapper).

## ROM com mapper ASCII16 (64 KB)
- Layout: banco 0 (0x4000, fixo) + banco 1 (0x8000, padrão) = código e tabelas (até 32 KB); bancos 2+ = assets comprimidos (16 KB cada), que só são
  mapeados em 0x8000 pelo carregador (`load_assets` em `vdpasm.s`, junto com `lz_unpack`/`vram_blit`, tudo na página 1 fixa: o linker coloca `vdpasm`
  primeiro). A faixa de assets nunca cruza dois bancos (`gen_assets.py`).
- `crt0.s` escreve 0 em 0x6000 e 1 em 0x7000. As escritas a 0x6000/0x7000 fazem emuladores (openMSX, WebMSX) detectarem ASCII16; se o seu não detectar,
  escolha o mapper "ASCII16" ao carregar a ROM.
- Para adicionar assets basta chamar `add()` em `gen_assets.py`: novos bancos são criados automaticamente (a ROM cresce em potências de 2).

## Hub / garagem (marco M1)
Título → ESPAÇO → **garagem** (cena do `LIB2` item 32, paleta EGA própria carregada em `load_garage`; a cópia limpa fica na página 2 para o menu poder ser apagado
com HMMM). Barra de status: dinheiro, data (agosto-setembro de 1963) e carro atual. Menu: YOUR CARS (escolher o carro, até 4), NEWSPAPER (em breve),
BOBS DRIVE-IN (escolhe oponente e piloto, corre, o resultado altera dinheiro/dia: +$50 vitória, -$25 derrota; provisório) e QUIT GAME (volta ao título).
Estado: `money`, `day`, `own[4]`, `cur` (`new_game()` começa com $1000 e um Lincoln Capri 1957). Assets em grupos: 0 = corrida, 1 = garagem (`g_group`).
Testes: `hub_test.tcl` (percorre as telas e salva capturas) e `loop_test.tcl` (hub↔corrida em loop, `-DRACE_LEN=40 -DAUTOPILOT=1`).

## Jornal (marco M2)
Menu NEWSPAPER: **USED CARS** (6 ofertas por dia, sorteio determinístico por dia com preço 80–120% do preço de tabela e sempre uma pechincha entre os
carros mais baratos; compra ao preço pedido ou com oferta de 10/20/30% a menos, que o vendedor aceita com chance 80/50/15%, senão a oferta some no dia),
**AUTO PARTS** (43 peças do `HOT_DATA` por categoria: motores, câmbios, carburadores, coletores, pneus; até 12 peças avulsas) e **SELL SPARE PARTS** (metade do
preço). O jogo começa sem carro ($1000): Bob's Drive-In e a lista de carros avisam "buy a car first". Mensagens no estilo do original
("NO GO, HOT SHOT / YOU ARE $x SHORT", "IT'S YOURS!"). Limite de 4 carros. Testes: `news_test.tcl`/`news_test2.tcl` (`-DSTART_MONEY=6000`),
`-DSTART_CAR=16` dá um carro inicial para os testes de corrida (`loop_test.tcl`).
Detalhe de ROM: `ihx2rom.py` falha se a ROM tiver o byte-padrão `LD (nn),A` para registradores de banco de outros mapper (ex.: `32 00 50`),
porque o openMSX/WebMSX os usam para adivinhar o mapper; a tabela de preços das peças guarda `preço+1` por isso.

## Garagem (marco M3)
YOUR CARS → escolher um carro → **DRIVE THIS CAR** (carro com que corre), **CAR PARTS** (troca peça por categoria usando as peças avulsas compatíveis com a
montadora do carro; a peça antiga vai para o estoque), **TUNE TIMING** (retard/advance, mostra a potência), **CAR INFO** (potência, marchas, rotação máxima,
câmbio, desgaste, ponto) e **SELL IT** (o revendedor oferece 55% do valor; pedir 20% ou 40% a mais é aceito com chance 55%/20%). O carro atual é marcado com `*`.
O desempenho vem do modelo do jogo (`tools/powertrain.py`) portado para inteiros em `ptmodel.h` (`pt_quality`/`pt_power`/`pt_build`), recalculado na
largada com as peças instaladas, o desgaste e o ponto (dano ainda não usado). Verificado contra o Python: 14.580 combinações no PC (erro ≤ 1 unidade) e
casos no próprio Z80 via `pt_dbg.c` (compilar à mão: `sdcc ... pt_dbg.c`, ler 0xF000.. no openMSX). Obs.: o SDCC gerou código errado para a expressão
composta do desgaste/ternário em `i32`; o código foi reescrito em passos simples (ver comentário em `ptmodel.h`).
Testes: `garage_test.tcl` (compra peça, troca, CAR INFO) e `sell_test.tcl` (venda).

## Bob's Drive-In (marco M4)
BOBS DRIVE-IN → **CHALLENGE WHO?** (22 pilotos, de Mike a The King) → o jogo mostra o piloto e o carro dele (carro sorteado na classe de desempenho que cresce com o
nível do piloto) → **tipo de corrida**: DRAG RACE (reta e plana, 380 segmentos), ROAD RACE (pista com curvas e ladeiras, 520), JUST FOR KICKS (sem aposta) e
PINK SLIPS (o carro de um contra o do outro; exige vitórias: `1 + índice/2`; o oponente aceita com chance de 70%) → **aposta** $10/$25/$50/$100 (o oponente aceita
com chance 100-15*nível; falta de dinheiro avisa) → corrida. Depois: vitória soma a aposta, derrota desconta; pink slip transfere o carro (se não houver vaga,
vende por metade do valor); mensagens do original ("BIG DEAL.", "YOU JUST WON A HELLUVA CAR!", "YA JUMPED THE LIGHT, SPEEDY!"). Cada corrida passa 1 dia.
Detalhes: a pista (`curve_ram`/`hill_ram`, lidos por `proj.s`) é escolhida na largada; os nomes (carros, peças, pilotos) vivem num banco da ROM e são
copiados por `bank_copy` (rotina na página 1) — liberou ~1,7 KB de código. Testes: `loop_test.tcl` (LAPS=n), `bobs_test.tcl`, `pink_test.tcl`
(`-DSTART_CAR=17 -DSTART_MONEY=500 -DSTART_WINS=20 -DTEST_ACCEPT [-DTEST_LOSE]`). A decisão de aceitar apostas é uma regra simples minha; as tabelas do bloco
9/10/11 do HOT_DATA ainda não foram decifradas.

## Consequências e fim de jogo (marco M5)
- **Desgaste por corrida:** motor `+100 + 30·(ticks acima da rotação de corte)`, pneus `+6` (`+10` na estrada), gasolina `-3` galões (tanque de 20; Bob's exige 3).
- **Quebras** (a corrida vira derrota, a aposta se perde): motor `2% + 6%·ticks acima da rotação + desgaste/500`, câmbio `1% + 12%·trocas com o motor passado`, pneus pela
  tabela do jogo (`pm_wtire`, 5 níveis de desgaste por tipo de pneu). Carro quebrado não corre ("YOUR CAR'S NOT RUNNING, SPEEDY!").
- **Destruição:** perdendo a mais de 90 mph há 6% de chance de perder o carro (sucata = 10% do preço): "LUCKY YOU WEREN'T KILLED!".
- **Garagem:** no menu do carro, **FIX OR JUNK IT** ("DAMN. IT'LL COST $x": preço das peças quebradas; consertar zera o desgaste do motor/pneus; sucatear paga a sucata) e
  **GAS UP** (3 galões por dólar). CAR INFO mostra combustível, desgaste dos pneus e o que está quebrado.
- **Fim de jogo:** acabou o verão (dia 61 = 1º de outubro) → "YOU'RE DONE FER THE SUMMER!"; venceu The King numa corrida real → "YOU DESERVE TO BE KING OF THE COUNTY!";
  sem carro, sem peças e com menos de $300 → "YOU'RE OUTTA DOUGH.". Mostra o resumo (dinheiro, carros, vitórias) e volta ao título.
- Testes: `cons_test.tcl` (`-DTEST_BRK=1`: quebra de motor forçada, Bob's recusa, FIX IT), fim do verão com `-DSTART_DAY=59 -DTEST_BRK=0`.
Os números (percentuais, custos) são regras minhas inspiradas nas mensagens do original; a tabela de pneus é a do jogo.

## Infraestrutura de bancos de código (overlays)
A região fixa de código (32 KB) estava cheia; agora o código é dividido em **bancos ASCII16** (sem limite prático de tamanho da ROM):
- **Banco 0 = núcleo**, em 0x4000-0x7FFF (página 1, sempre visível): `crt0`, `vdpasm.s`, `proj.s`, tabelas, texto/UI, som, modelo do motor (`ptmodel.h`), bibliotecas do SDCC. Hoje ~13,3 KB de 16 KB.
- **Bancos 1..3 = código**, na janela 0x8000-0xBFFF (página 2). Hoje: banco 1 = corrida (física, projeção, sprites, HUD, `run_race`), banco 2 = jogo (título, garagem, jornal, Bob's, consequências, `hub`). Cada um tem 16 KB.
- **Bancos 4.. = assets** (`ASSET_BANK0=4`, variável de ambiente de `gen_assets.py`/`ihx2rom.py`) e o banco de texto.
- Como funciona: `main.c` continua uma só unidade de compilação; os `#pragma codeseg CODE|BANK1|BANK2` marcam as regiões do fonte. `build.sh` compila para
  assembly (`-S`), `bankify.py` redistribui funções/tabelas nas áreas `BANKn` (e **falha o build** se uma referência cruzar bancos sem passar por uma função
  `__banked`), e o `sdld` põe cada área em `0x{n}8000` (endereço virtual); `ihx2rom.py` reposiciona no arquivo da ROM. Chamadas entre bancos usam o `__banked`
  do SDCC (`BK` no fonte) com `___sdcc_bcall_ehl` + `get_bank/set_bank` (`vdpasm.s`; `_cur_bank` é restaurado pelos carregadores de assets/texto).
- Regras para novas funções: funções usadas por mais de um banco vão para o núcleo (`#pragma codeseg CODE`) ou são `BK`; **funções `BK` só recebem argumentos escalares**
  (um ponteiro para string/tabela de outro banco ficaria invisível). Para criar o banco 3 basta colocar `#pragma codeseg BANK3`.
- Verificado: `loop_test`, `cons_test`, `garage_test`, `sell_test` e `pink_test` dão os mesmos resultados de antes (diferenças só em sorteios, que dependem do tempo de boot).

## Rua (marco M6)
Em **BOBS DRIVE-IN** o jogo agora faz primeiro o trajeto: "CRUISING IN TOWN... MAX SPEED 45 MPH" e você dirige (mesma engine da corrida, pista com curvas, sem
oponente/semáforo, 420 segmentos) até o Bob's. Três **radares** (nos segmentos 110/230/350): acima de 45 mph a multa é `$5 + (mph-45)/4` por radar ("SPEEDING
TICKET(S)!", "FINE $x", descontada do dinheiro). **Gasolina:** 1 galão por trajeto (2 se a média passar de 50 mph); com menos de 3 galões depois do trajeto avisa
"YOU'RE LOW ON GAS!" e não abre o Bob's (a corrida gasta 3). **ESC/SHIFT** desiste e volta à garagem sem gastar. Implementado com `street_mode` em `run_race`
(BANK1) e `street_trip` (BANK2). Teste: `street_test.tcl` (`-DAUTOPILOT=1 -DSTART_CAR=16`). As multas e o consumo são regras minhas.

## Garagem clicável (M7, parte 1)
O menu principal virou a própria cena da garagem com um **cursor** (sprite 16x16 de 2 planos: miolo branco + contorno preto, padrões em 0x7800, cores 0x7400, atributos 0x7600 — a
sobra da página 0 da VRAM; `spr_put` em `vdpasm.s`). Setas/joystick movem (aceleram ao segurar), **ESPAÇO / botão A clica**, e um **mouse na porta 1** é lido via
`GTPAD` da BIOS (`read_mouse`; ativa só quando chega deslocamento sem direção do joystick — não testado em mouse real, o openMSX não injeta movimento).
Objetos (retângulos em `hot[]`, o nome aparece no alto da cena): prancheta e macaco = **YOUR CARS**, jornal no chão = **NEWSPAPER**, calendário = datas/dias
restantes, rádio = **CATCH SOME TUNES** (jingle de 8 notas no PSG), galão verde = **WANNA TANK?** (gasolina), abertura da garagem à esquerda = **HIT THE STREET**
(rua → Bob's), relógio = **TIME TO QUIT** (confirma). Sprites só ficam ligados no hub (R#8 bit 1); nos menus/notas o cursor some.
Testes automáticos: `-DOLD_MENU -DSKIP_STREET` recompõe o menu de texto antigo e pula a rua (todos os `*_test.tcl` antigos); `click_test.tcl`/`click2_test.tcl`
(`-DCUR_X/-DCUR_Y` posicionam o cursor) e `vram2png.py ... garage spr` (renderiza os sprites; o `screenshot` do openMSX sai preto em modo sem throttle).
Pendente no M7: telas de título/jornal/Bob's/resultado com a arte da LIB1/LIB2, o carro desenhado na garagem, pintura/adereços, música, ranking.

## O carro na garagem (M7, parte 2)
O carro atual aparece desenhado na garagem: corpo lateral (LIB2: itens 39…201, escolhidos pelo `pic_index` do HOT_DATA; figura 255 = Plymouth conversível = item 179, 26º = Corvette 201)
+ duas rodas (tipo de pneu: 28 = faixa branca, 65 = cromada, 106 = slick), tudo com a largura reduzida a 4/5 como a cena (`cars_art.py`). A posição das rodas vem da detecção dos vãos
das arcadas (3 carros têm posição manual). As imagens usam a paleta da cena: **índice 0 = transparente (LMMM TIMP), 6 = pintura, 7 = reflexo** (6/7 não são usados pela cena, então
`set_paint` troca só essas duas cores — cada carro tem `paint` 0..7 = a paleta de pinturas do oponente). Cada carro é um grupo de assets (2+id), cada pneu outro (28+tipo), descomprimidos
no rascunho da página 3 (`CAR_Y0`/`WHEEL_Y0`); `draw_car` copia a faixa limpa do chão (guardada em `BAND_COPY_Y` por `band_save`) para as páginas 0-2 e compõe o carro por cima (a página 2
continua servindo de cópia para `hub_restore`). É redesenhado ao voltar de qualquer ação do hub (troca/compra/venda/conserto). O carro é clicável (= YOUR CARS). Nota: o banco 2 de código
está com ~14,5 KB de 16 KB — o próximo grande bloco de código deve ir para um BANK3 (`#pragma codeseg BANK3`).

## Fonte e janelas da garagem; entrada/saída do carro (M7, parte 3)
- **Fonte:** na garagem o texto agora usa a fonte 6x8 da BIOS (tabela `CGTBL`, ponteiro em 0x0004; `font_init` copia as 96 letras para a página 3 da VRAM, em preto e em branco, ao carregar a cena)
  e `gtext` desenha letra a letra com LMMM (TIMP). Minúsculas/símbolos existem na fonte; o jogo ainda manda os textos em maiúsculas. A fonte 3x5 em 2x2 continua só nas telas de corrida/título.
- **Visor:** a barra branca no alto da cena mostra o nome do objeto sob o cursor (texto preto centralizado). **Barra de status** branca com texto preto (dinheiro, data, carro; "ESC BACK" à direita nos submenus).
  **Janelas:** moldura preta, fundo branco, título sublinhado, linhas de 12 px, item selecionado = barra preta com texto branco; os textos cabem nas janelas (até ~36 letras).
- **Carro:** base das rodas no chão (y=190). Ao trocar de carro: garagem vazia → o carro novo entra pela esquerda; garagem cheia → o antigo sai de ré (para a esquerda) e o novo aparece no lugar.
  A animação desenha na página oculta e troca a página (sem cintilação), 8 px por quadro (`car_slide`). `new_game` zera `shown_id`; ao voltar de uma corrida o mesmo carro não é reanimado.

### Ajuste de posição do carro (referência: captura do jogo original)
Base das rodas em y=152 (`CAR_FLOOR`), centralizado, com sombra elíptica preta sob o carro (`cshadow`); faixa de restauração y=80..164 (`BAND_Y/BAND_H`); rascunho da página 3 em `CAR_Y0=856`/`WHEEL_Y0=930`.
Os objetos do chão (jornal, macaco, compressor, galão) ficam livres abaixo do carro, como no original.

## Pintura e adereços (M7, parte 4)
Clicar no **compressor roxo** (chão, à direita) abre **PAINT AND BODY**: **NEW PAINT JOB $20** (12 cores: as 8 do jogo + preto, branco, rosa, marrom; o índice 6 da paleta é a pintura e o 7 o reflexo),
**STICKERS $5** (8 adesivos da LIB2, itens 186-193, desenhados na porta), **CHOP ROOF / REBUILD ROOF $70** (teto rebaixado) e **STRIP / RESTORE BUMPERS $15**. Uma compra por visita; o carro é
redesenhado ao sair. O `Car` ganhou `roof`, `bump`, `stick` (e `paint` 0..11). Os preços são os das strings do original; as cores e adesivos são escolhas minhas.
**Variantes:** `cars_art.py` gera 4 corpos por carro (`variants`: teto rebaixado = tira linhas dos montantes entre a capota e a cintura; sem para-choques = apaga o cromado nas pontas) — grupos de assets
2+id (normal) e 100+3·id+v-1; adesivos = grupos 180+n; página 3 da VRAM: 768 cópia limpa da faixa, 852 corpo, 920 roda, 948 adesivo, 976 fonte.

### Reorganização dos bancos
O núcleo estava estourando (a ROM só roda com o núcleo em 0x4000-0x7FFF). Agora: **BANK1** (corrida) também leva as rotinas em assembly da estrada (`strip_run`, `draw_road_asm`, `mul12`, `mul16x8`, `project_asm`:
áreas `BANK1` dentro de `vdpasm.s`/`proj.s`) e as **tabelas da estrada** (`tables.h` tem `#pragma codeseg BANK1`; `bankify.py` agora lê os `#pragma` dos cabeçalhos); **BANK3** = Bob's, fim de corrida/jogo e a oficina;
`ASSET_BANK0` = 6 (bancos 1..5 livres para código). Núcleo: 13,2 KB de 16 KB. A ROM tem 512 KB (as 4 variantes × 26 carros + adesivos viraram ~11 bancos de assets).
`street_test.tcl`/`body_test.tcl` (`-DCUR_X/-DCUR_Y` no compressor) são os testes desta parte.

## Jornal com a arte original (M7, parte 5)
Clicar no jornal no chão da garagem mostra a **primeira página do "Los Angeles News"** (LIB2 item 21, 320x136 → 256x136) com a **manchete do dia** (itens 22-26, 54 linhas; `day % 5`) — 190 linhas no total.
Cursor como na garagem: **AUTO PARTS** (faixa de cima) e **USED CARS** (faixa do meio) abrem as listas de sempre (janelas brancas sobre a página); o **título "Los Angeles News"** vende peças avulsas;
o nome do objeto aparece numa etiqueta no pé da página; **ESC** volta (recarrega a garagem). `pick_hot` virou o seletor genérico de objetos (`hs/hn/hnames/lab_mode`). Grupos de assets 190 (página) e 191-195 (manchetes).
**Bug corrigido:** o carregador de assets contava as faixas em 8 bits (`djnz`); com 261 faixas só as 5 primeiras eram lidas — agora usa contador de 16 bits.
Outros itens de arte já localizados na LIB2 (para as próximas telas): 90 (posto "Gus' Gas"), 93 (**Bob's Drive-In**), 219/221 (calendários JUN-JUL e AUG-SEP), 15 (carteira de motorista DMV), 16 (livro da Highway Patrol), 14 (cockpit).

### Jornal: sem a faixa de manchetes
A faixa inferior (manchete do dia, itens 22-26) foi retirada: o texto dela é pré-renderizado ilegível e competia com a etiqueta e a barra de status. A página (item 21) fica no alto e o resto da tela é preto
(grupo 190; os grupos 191-195 deixaram de existir).

### Animação do carro sem piscar a barra de status
A animação alterna as páginas 0 e 1; a barra de status só existia na página 0 e "piscava" a cada quadro. `hub_status` agora desenha a barra nas **duas** páginas, e o hub a chama **antes** de `draw_car`.
`cshadow` também recorta a sombra fora da tela (carro entrando/saindo).
