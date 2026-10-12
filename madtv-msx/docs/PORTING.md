# Decisoes de portabilidade

```
Original system : Mad TV (1991, Rainbow Arts) - DOS/Amiga/ST, simulacao de gestao em tempo real
Data source     : banco do remake open source TVTower (XML) - ver LICENSES.md
MSX target      : MSX2 (V9938), 64 KB RAM minimo, cartucho ROM (nada testado em hardware real)

Video strategy  : Screen 5 (256x212, 16 cores). Texto = fonte em VRAM copiada pelo VDP (LMMM); retangulos/barras com HMMV; telas
                  montadas fora da tela (pagina 2) e copiadas de uma vez (sem cintilacao). Sem scroll.
Sprites         : so no predio (hub): figuras 16x16 do jogador e dos 2 rivais + elevador, sprite mode 2 (uma cor por linha:
                  cabeca / tronco na cor da emissora / pernas), tabelas na pagina 3 da VRAM (padroes 1F000h, cores 1F800h,
                  atributos 1FA00h), 4 sprites, nunca mais de 4 por linha.
Audio strategy  : PSG (a definir; nenhuma implementacao ainda)
Input strategy  : teclado (setas) + joystick e **mouse** (0.5.1), todos simultaneos. O MSX nao distingue mouse de joystick na porta
                  (sem mouse a linha le como direcao parada/lixo), por isso o mouse e ligado pelo jogador: tecla **M** cicla
                  desligado -> mouse na porta 1 (joystick passa para a porta 2) -> mouse na porta 2. Esquerdo = selecionar/ativar
                  ("dentro"), direito = voltar ("fora"), como no original; clique na velocidade do cabecalho troca a velocidade.
Memory strategy : tabelas const em ROM geradas por tools/convert_db.py; RAM so para estado do jogo.
ROM mapper      : ROM_ASCII8 (128 KB, 16 segmentos de 8 KB), 3 regioes: segmentos 0-1 = codigo FIXO (4000h-7FFFh, 16 KB);
                  banco 2 (8000h-9FFFh) = janela de CODIGO banked (segmentos 5-8, chamadas `__banked` por trampolim do MSXgl);
                  banco 3 (A000h-BFFFh) = janela de DADOS (segmento 3 = catalogo filmes/anuncios; 4 = noticias).

Features preserved : dia 17h-1h em tempo real, grade 18h-0h, categorias, atributos de filme (critica/ritmo/bilheteria, FSK18, blocos),
                     compra e venda de filmes (Arquivo), Sala de Noticias com 3 agencias, credito do chefe com juros, salvar/carregar por codigo,
                     contratos de publicidade (preco/multa/repeticoes/prazo/audiencia minima), audiencia por hora,
                     Image dividido em 100 pontos com a regra de transferencia por slot, 3 emissoras, falencia.
Features adapted   : catalogo do TVTower reduzido a subconjunto curado (88 filmes, 24 anuncios, 24 noticias);
                     8 categorias (TVTower tem generos mais finos -> CATEGORIES em tools/convert_db.py);
                     preco de filme provisorio = media(criticas,ritmo,bilheteria) * price_mod (NAO e a formula do TVTower).
Features omitted   : Betty/presentes, producao propria, estudios, Sammys, sabotagem, torres/satelites, audio, escolha da emissora,
                     dificuldade, grade de "amanha". Salas ainda FECHADAS no predio (porta existe, mostra aviso): porteiro, supermercado,
                     roteiros, estudios, corretor, Betty; escritorios dos rivais (trancados).

Known limitations  : textos so em ingles/ASCII; titulos com ${...} nao resolvidos sao descartados.
Performance issues : texto resolvido com fonte em VRAM (ver Medicoes); ROM_32K quase cheia.
```

## Mouse (0.5.1)
Leitura por `Mouse_Read` do MSXgl (protocolo do PSG R#15/R#14). Cursor = 2 sprites (seta branca + contorno preto, arte propria),
sempre ativos (indices 0-1; os do predio sao 2-5), 1 contagem do mouse = 1 pixel, cursor preso a [2..253]x[2..209].
Cada tela tem `X_Mouse(x, y, btn)` (segmentos banked): passar por cima realca (move o cursor da tela, sem redesenho extra),
clique esquerdo seleciona e ativa (chama o mesmo `X_Input(IN_OK)` do teclado - um unico caminho de logica). Areas clicaveis:
portas do predio, linhas de todas as listas/menus, abas de genero (`<` `>`), agencias de noticias, caracteres do codigo de save.
**Cuidado em listas de compra:** um clique na linha da Agencia de filmes compra o filme (como o original: clique = agir).
Testes: `tests/smoke.sh` injeta posicao/clique em `g_PtrX/g_PtrY/g_PtrInject` (mesmo hit-test do mouse real);
`tests/mouse_real.sh` usa mouse **emulado** do openMSX movido por xdotool (M, mover, passar sobre a porta, clicar, entrar).
**Nao testado em mouse fisico MSX.** A sensibilidade (1:1) pode precisar de ajuste; um clique muito curto (< 1 quadro) pode nao ser lido.
Fixo: 15 591 de 16 384 bytes - o proximo codigo novo deve ir para segmentos banked.

## O predio (0.5)
Substitui o menu de texto como hub. Corte transversal do edificio com 5 andares, elevador central (x=128) e 14 portas
(`k_Rooms` em building.c). Andar 0: porteiro*, arquivo, supermercado*; 1: agencia de filmes, publicidade, noticias; 2: roteiros*,
estudios*, corretor*; 3: seu escritorio, escritorios de FunTV/SunTV (trancados); 4: chefe, Betty*. (*fechada nesta versao.)
Controles: setas movem o cursor entre portas (esq/dir = vizinha no andar, cima/baixo = andar acima/abaixo na porta mais proxima);
OK = o personagem caminha ate o elevador, sobe/desce e caminha ate a porta, entao entra (como o clique do original, mas por teclado).
O tempo do jogo continua correndo durante a viagem (~0,5 s por andar a 60 Hz). Ao sair de uma sala voce reaparece na porta dela.
Rivais: decoracao - andam pelo andar, "entram" nas salas e reaparecem; ainda nao interagem com o jogo.
Escritorio = submenu: grade de programacao, audiencias/Image, salvar/carregar (mapa de torres e extrato: ainda nao).
**Arte:** o predio e desenhado proceduralmente (retangulos) e as figuras sao pixel art propria; **nenhum grafico do Mad TV original
foi extraido ou reutilizado**.
Armadilha encontrada: HMMV com largura de 1 pixel vira 0 bytes e preenche a linha inteira ate a borda (corrigido em `Ui_Fill`: w>=2).
Armadilha 2: o VDP ignora os bits 8-7 do registrador 5 - a tabela de atributos de sprites efetiva fica em 1FA00h, nao 1FB80h.

## Medicoes (openMSX 19.1, C-BIOS MSX2, JIFFY = interrupcoes do VDP; build com `MADTV_PROF=1` no 0.2)
| Operacao | antes (texto via Print RAM) | agora (fonte em VRAM + LMMM) |
|---|---|---|
| 12 linhas de ~30 caracteres | ~45 jiffies (lista completa, 0.2) | **17 jiffies** (~3x mais rapido) |

Texto 0.3: `Print_SetVRAMFont` desempacota a fonte branca nas linhas 212-251 da pagina 0; `Ui_Init` copia 6 variantes de cor
para a pagina 1 (preenche com a cor e aplica AND com a fonte branca) e `Ui_Text` copia cada glifo com LMMM/TIMP.
Armadilhas encontradas: (1) `VDP_UNIT_U16` trava `Print_SetVRAMFont`; (2) as tabelas de sprite do Screen 5
(0x7400-0x7A00) coincidem com a fonte em VRAM -> sprites fantasma; resolvido com `VDP_EnableSprite(FALSE)`.

### 0.3.1 - redesenho incremental (relatado: "apaga e reimprime" a cada minuto)
Causa: a cada minuto de jogo o cabecalho inteiro era limpo e redesenhado (~50 glifos) e, a cada hora, a tela inteira.
Correcao: cabecalho com cache por campo (so redesenha dia/hora/dinheiro/Image/velocidade que mudaram) e atualizacao
so dos dados (`D_DAT`) no Hub, na Grade e nas Audiencias, sem limpar a tela.
Medicao (openMSX, escrita na porta 0x9B do VDP = parametros de comandos, velocidade 3, tela Ratings, 10 s emulados):
**52 191 -> 12 022 escritas (-77%)**. O loop principal nao estava saturado (nenhuma das duas versoes perde frames por CPU);
o ganho e menos comandos VDP/flicker.

### 0.3.2 - sem apagar/reescrever ao mover o cursor
Relato: ao mexer o cursor com o teclado os textos eram apagados e reescritos. Causa: todo movimento de cursor marcava a tela
inteira como suja (`ClearContent` + redesenho completo). Correcao em duas camadas:
1. **Redesenho minimo:** cada tela tem `DrawRow_*` (uma linha), `DrawDetail_*` (painel de detalhes) e lista completa; o cursor
   marca so as linhas antiga/nova (`D_ROWS`), a rolagem/troca de genero so a lista (`D_LIST`), e os detalhes so se mudaram (`D_DET`).
   A tela so e limpa ao *entrar* na tela.
2. **Composicao fora da tela:** `Ui_Begin()/Ui_End()` desenham na pagina 2 da VRAM e copiam o retangulo pronto para a tela
   de uma vez (HMMM), entao o estado intermediario "apagado" nunca fica visivel. Cabecalho, mensagens e todas as telas usam isso.
Medicao (escritas na porta 0x9B do VDP por acao, jogo pausado, openMSX):
| Acao | antes (0.3.1) | agora (0.3.2) |
|---|---|---|
| mover cursor no Hub | 1589 | 513 (-68%) |
| mover cursor na Agencia de Filmes | 3534 | 1385 (-61%) |
| mover cursor na Agencia de Publicidade | 2928 | 1424 (-51%) |
| trocar de genero (lista inteira) | 3369 | 3108 |
O numero de comandos inclui as copias HMMM da composicao. **Nao foi possivel observar cintilacao ao vivo no sandbox**: a
ausencia de estado intermediario e garantida pelo desenho (so a copia final toca a tela), e as telas finais foram conferidas
por screenshot. Grade/lista de escolha usam o mesmo mecanismo mas nao foram medidas separadamente.

## Modelo de simulacao 0.3 (PROPRIO - nao e o do Mad TV nem o do TVTower; ver src/sim.c)
- Tempo: 1 minuto de jogo = 1/2, 1/5 ou 1/12 s reais (3 velocidades + pausa), normalizado para 50/60 Hz.
  Dia = 17:00 a 01:00 (480 min). Grade 18:00-00:00 (7 slots); :55 mede audiencia e exibe o anuncio.
- Audiencia (0,1 milhao) = alcance x %TV-ligada(hora) x qualidade(filme) x afinidade(categoria,hora) x desgaste x aleatorio(90-110%).
  qualidade = (3*critica + 4*bilheteria + 3*ritmo)/10; desgaste = 100-12% por exibicao recente (min 40%);
  FSK18 antes das 21h = x0,4; slot vazio = 5%. Alcance: 12,0 / 11,0 / 10,0 M (MadTV/FunTV/SunTV).
- Image: a cada slot, maior taxa de audiencia tira 1 ponto da menor (regra do manual). Soma sempre 100.
- Contratos de anuncio (dados do TVTower, min_audience lido em milhoes): cada exibicao com audiencia >= minima conta;
  ao zerar as repeticoes paga `profit`; expirado o prazo com repeticoes pendentes cobra `penalty`. Max 4 contratos.
- Preco do filme (k$) = media(atributos) x price_mod/20 x blocos/2 + 50. Caixa inicial 2500k$. Custo fixo diario 100k$. Falencia: caixa < -2000k$.
- Rivais (FunTV/SunTV): grade preenchida por heuristica (melhor de 6 candidatos por slot) **so entre os filmes da propria biblioteca**
  (comeca com 5 e ganha 1 por dia, ordem aleatoria por partida); audiencia dos rivais x0,88 (dificuldade normal); sem contratos.

- Noticias (0.4): 3 agencias (Politics = genero 0 do TVTower; Showbiz = 1 e 5; Misc = 2, 3 e 4), taxa de 30k$/dia por agencia assinada.
  A cada hora cheia cada agencia assinada entrega (50%) 1 noticia ao pool (8 itens, expiram em 10 h). O jogador compra itens
  (custo = price/4 k$) para o telejornal de 3 posicoes; frescor = 100% - 10%/hora (min 10%); qualidade do telejornal = media(qualidade x frescor).
  Audiencia do programa seguinte x (70% + 30% x qualidade); rivais fixos em x0,80. Telejornal vazio = x0,70.
- Credito do chefe: limite = 3000k + 40k por ponto de Image; juros 6%/dia sobre a divida; emprestimo/pagamento em passos de 500k.
- Arquivo: revenda = metade do preco x (100% - 8% por exibicao recente, min 20%); nao vende filme que esta na grade.
- Multa de contrato efetiva = 60% da multa do banco de dados (TVTower e mais duro que as audiencias deste modelo).
- Salvar/carregar: sem SRAM no ASCII8 -> codigo de 82 caracteres (alfabeto de 32, sem I/O/Y/Z) com checksum Fletcher-16.
  Salva: dia, hora, caixa, divida, Image, filmes possuidos, agencias, contratos, grade de hoje, semente. Nao salva: desgaste
  dos filmes, noticias do pool/telejornal, ofertas do dia, grade de hoje dos rivais (regerados). Tempo para na tela de codigo.

## Balanceamento (autoplay: `tests/balance.sh [dias] [partidas]`, mesma `src/sim.c` compilada no PC)
Politicas: *idle* (nao faz nada), *naive* (assina todos os contratos), *careful* (assina so se a qualidade maxima permite), *smart*
(usa a audiencia real do dia anterior, so assina o que consegue cumprir, compra filmes ate acompanhar a biblioteca dos rivais).
Achados que mudaram o jogo (primeira versao: **100% de falencia em todas as politicas**):
1. rivais tinham o catalogo inteiro de graca -> Image do jogador ia a ~0 em poucos dias; agora tem biblioteca limitada (+1/dia) e x0,88.
2. filmes caros (preco/10) e caixa inicial 1500k$ deixavam o jogador sem capital para uma grade completa; agora preco/20 e 2500k$.
3. multas dos contratos sao maiores que o pagamento (dados do TVTower: ate 2500k$ vs 1583k$) - assinar sem conferir a audiencia e fatal.
Resultado antes das noticias (0.3.4): smart 7% de falencia, Image 32. **Com noticias, credito e multa efetiva de 60% (0.4)**
(rivais x0,85 e telejornal fixo x0,80; politica *smart* agora tambem atualiza o telejornal e usa o credito como colchao),
500 partidas / 30 dias: *smart* **~1% de falencia**, Image medio **~52**; *naive* e *careful* **100%** de falencia (assinam sem
checar audiencia); *idle* termina o mes com -1400k$. (60 dias, 300 partidas: smart 4%, Image 63.) Achado: a primeira versao com noticias
falia 35-43% porque o jogador sem telejornal perde 30% de audiencia - o harness tambem tinha um bug (filmes de 3 blocos
sobrepunham os ja colocados) que so apareceu ao corrigir a politica. **Limites:** as politicas sao heuristicas minhas, nao jogadores
humanos; o caixa cresce muito para o jogador competente (sera drenado por torres/estudios/presentes/juros nos proximos milestones).

## Mapper: decisao (0.3.3) e arquitetura de segmentos (0.4)
Por que ASCII8 e nao ASCII16: ASCII16 so tem 2 bancos de 16 KB - uma janela de dados tiraria codigo do ar. Com ASCII8 ha 4 bancos
de 8 KB e e suportado por openMSX, flash carts comuns e FPGA. Alternativas descartadas: ROM_48K/64K (dependem de o cartucho decodificar
as paginas 0/3 - menos portavel) e Konami (sem vantagem aqui).
Passo 1 (0.3.3): desligar recursos nao usados do MSXgl em `msxgl_config.h`: **31 907 -> 24 037 bytes**. No 0.4 tambem sai o modulo
Print inteiro (a fonte e desempacotada por `FontUnpack` em ui.c): -2 KB.
**0.4: o codigo passou de 24 KB e foi dividido em segmentos banked** (BankedCall do MSXgl; o trampolim troca o banco 2 durante a chamada):
| Segmento | Banco | Conteudo | Bytes (de 8192) |
|---|---|---|---|
| 0-1 (fixo) | 4000h | crt0, MSXgl, ui.c, main.c, sim.c (nucleo), db.c | 14 214 de 16 384 |
| 3 | A000h | catalogo: 88 filmes, 24 anuncios | 3 073 |
| 4 | A000h | noticias (115 registros fixos de 43 bytes) | 4 945 |
| 5 | 8000h | telas Grade / Agencia de filmes / Publicidade / Audiencias | 5 051 |
| 6 | 8000h | sim_ext: IA dos rivais, noticias, ofertas, salvar/carregar | 4 596 |
| 7 | 8000h | telas Noticias + Arquivo | 2 218 |
| 8 | 8000h | telas Chefe (credito) + Salvar/Carregar | 2 951 |
| 9 | 8000h | predio (sprites, movimento) + escritorio | 4 068 |
Regras: (1) o banco 3 fica no segmento 3; dados de outros segmentos so sao lidos dentro de `Db_News` (copia para RAM e restaura o
banco); (2) funcoes banked recebem/retornam so valores ou ponteiros para RAM/ROM fixa; (3) literais de codigo banked ficam no
proprio segmento (`--codeseg`); (4) estado compartilhado entre telas fica em RAM fixa (`app.h`) e e inicializado em `main()`.

### Bug latente encontrado na migracao: RAM nao zerada
O crt0 do MSXgl nao zera o BSS; o codigo dependia de variaveis estaticas valerem 0 (o emulador mascarava). Ao mudar o layout o
teste falhou (grade abriu na coluna de anuncios). Em hardware real a RAM contem lixo -> **todo estado e inicializado
explicitamente em `main()`/`Ui_Init()`/`Sim_Init()`**. Regra do projeto: nunca confiar em zero-init.

## Orcamento de memoria (0.3.3)
ROM: 128 KB (cart); codigo fixo 20 170 / 24 576 B (82%); segmento de dados 3 895 / 8 192 B. RAM: `Game` ~480 B + UI/estado ~200 B.
VRAM (Screen 5): pagina 0 = tela (linhas 0-211) + fonte branca (212-251); pagina 1 = 6 variantes de cor da fonte (256-495);
pagina 2 = buffer de composicao (512-723).

## Mapeamento TVTower -> categorias Mad TV (tools/convert_db.py)
Lovestory=Romance | Action=Acao,Aventura,Western | Monumental=Monumental,Historia | Comedy=Comedia |
Crime=Crime,Thriller,Misterio | Culture=Documentario | SciFi=Ficcao cientifica,Fantasia | Other=Animacao,Drama,Familia,Terror.
Flag X-rated (64) do TVTower -> FSK18 (confirmado empiricamente: 160/194 filmes do genero Erotic e 109 de Acao tem o bit).

## Betty e presentes (0.6)
- Modelo proprio (src/sim_ext.c). Presentes (k$): 20,40,80,120,200,350,600,900,1500,2500; efeito em simpatia 1,2,3,4,6,8,10,12,14 (a Dream trip, 10o, so serve para o casamento). Cada uso reduz o efeito em 30% (recupera aos poucos); estoque max 3 de cada.
- Simpatia <= Image da emissora; sem presente no dia = -1; passar uma producao de Culture completa = +1. Rivais ganham simpatia aleatoria diaria.
- Image 0 = emissora falida (audiencia zero, sai da disputa); o jogador com Image 0 perde. Para casar: simpatia 100, os dois rivais falidos e 1 Dream trip.
- Savegame v5: bloco da Betty anexado (sym, alive, estoque, usos); codigo de 98 caracteres.
- Telas novas em src/screens_d.c (segmento 10); titulo e fim de jogo foram movidos para screens_c.c (segmento 8) para liberar o segmento fixo (agora 15.623 de 16.384 bytes).
- Balanceamento (autoplay smart, sem presentes): 30 dias 3% de falencia, Image ~55; 150 dias 12% de falencia, Image ~89. O final feliz nao foi alcancado por autoplay; so testado forcando o estado.

## 0.7: catalogo, dificuldade, Image baixo, autoplay com presentes
- Catalogo: 144 filmes (20 por categoria; Culture so 4 - ver correcao na 0.8). O segmento 3 (dados) ficou com ~4,6 KB de 8 KB, entao **nao foi preciso ASCII16 nem trocar de mapper**; filmes continuam acessiveis por ponteiro direto (indice u8 cabe ate 255). Codigo de save passou a 109 caracteres (bitmask de filmes possuidos = 18 bytes).
- Dificuldade: tabelas em src/sim.c (`k_RivalQ` 65/78/82 e `k_StartMoney` 3500/2500/2000); guardada no nibble alto do byte de versao do savegame (v6).
- Image < 20 (`IMAGE_LOW`): contratos pagam 80% e `Sim_CreditLimit()` = 0. Isso piorou o autoplay (sem a regra: 4% de falencia em 30 dias; com a regra: 17%), o que e o efeito desejado, mas o bot depende do credito do chefe como colchao.
- Autoplay smart com presentes (`tests/balance.sh 200 100`, caixa folgado >= 2500k para comprar presente, Dream trip no fim; `BAL_DIFF=0|1|2`, `NO_GIFTS=1`):

| Dificuldade | falencia | casou | dia medio do casamento |
|---|---|---|---|
| Easy | 15% | 85% | 48 |
| Normal | 36% | 64% | 85 |
| Hard (82%; com 85% ja so 3% casam, com 90% 100% perde) | 75% | 15% | 113 |

- Esses numeros sao do bot heuristico (mesma sim.c compilada no PC), nao de jogadores humanos, e o balanceamento e extremamente sensivel a `k_RivalQ` (variar de 78 para 85 no Normal leva o casamento de 64% para quase 0%).
- Com 144 filmes o Image medio do bot caiu em relacao a 88 filmes no mesmo cenario (~30 vs ~52 em 30 dias, sem presentes); suspeita: o bot compra por qualidade/preco e os filmes extras (ordenados por GUID, nao por qualidade) o atraem para filmes ruins. Nao investiguei mais.

## 0.8: Porteiro e escritorios dos rivais
- `src/screens_d.c` (segmento 10, agora ~3,7 KB): `Porter_*` e `Rival_*` (uma tela para FunTV e SunTV; `g_Screen - SCR_FUN + 1` = emissora). Atualizacao horaria via `ScreenDyn` (redesenha a tela inteira, ~poucos jiffies).
- Espionagem e gratuita e so leitura: mostra `g_Game.slot[st]` (a grade que a IA montou para hoje) e `aud[st][slot]` quando `aud_done`. Nao altera o balanceamento (o autoplay nao usa).
- Correcao do catalogo: a categoria Culture tinha 4 filmes porque 60 dos 67 candidatos do TVTower nao tem `year` no banco (o conversor os descartava, nao era o filtro de titulo, como escrevi antes). Agora Culture assume o ano 1985 quando falta. Total 160 filmes, 20 por categoria; as outras categorias nao mudaram. O balanceamento do autoplay **nao foi refeito** com o catalogo novo.
- Segmento fixo: 15.830 de 16.384 bytes.
- **Rebalanceamento 0.8** (o catalogo com Culture mudou o autoplay: Normal caiu de 64% para 0% de casamentos com os mesmos parametros). `k_RivalQ` agora 55/68/74. Autoplay smart com presentes, 200 dias, 100 partidas:

| Dificuldade | falencia | casou | dia medio |
|---|---|---|---|
| Easy (55%) | 12% | 88% | 69 |
| Normal (68%) | 47% | 53% | 128 |
| Hard (74%) | 76% | 12% | 172 |

(a tabela da 0.7 acima ficou obsoleta; valem estes numeros, tambem de um bot e nao de humanos)
- Armadilha (0.8): `Ui_Text` usa x em `u8`; texto que passa de x=255 **da a volta** e aparece no canto esquerdo da mesma linha (fantasma "ng" sobre o titulo da tela dos rivais). Regra: 6 px/caractere, x + 6*len <= 256. Corrigidos tambem 3 textos antigos que estouravam por 2-6 px.

## 0.9: Corretor e torres
- Alcance efetivo `Sim_Reach(st)` = alcance base x `k_TowerPct` {100,112,125,140,155}%. Jogador: nivel comprado (`g_Game.tower`); rivais: nivel = min(4, dia/30).
- Custos (`k_TowerCost`): 1500, 3000, 5000, 8000 k$; manutencao +40 k$/dia por nivel (alem dos 100 fixos).
- Mudanca de regra: o Image passa a ser disputado pela audiencia **absoluta** (antes pela quota = % do proprio alcance); sem isso torres nao teriam efeito no Image. `Sim_Quota` continua sendo a % do proprio alcance (so exibicao).
- Savegame v7: +1 byte (nivel da torre); SAVE_BYTES = 51 + bitmask de filmes -> 114 caracteres.
- Autoplay smart (compra a proxima torre quando sobra caixa > custo + 3000k; `NO_TOWERS=1` desliga), 200 dias, 60 partidas:

| Dificuldade | sem torres: falencia / casou | com torres: falencia / casou | dia medio |
|---|---|---|---|
| Easy | 18% / 81% | 28% / 71% | 60 |
| Normal | 73% / 26% | 50% / 48% | 80 |
| Hard | 100% / 0% | 83% / 16% | 92 |

  Os parametros de dificuldade (55/68/74) nao foram alterados. De novo: numeros de um bot, nao de humanos; o bot gasta com torres e presentes e isso explica falencia maior no Easy.
- Tela: `Realtor_*` em src/screens_d.c; segmento fixo 15.984 de 16.384 bytes.

## 1.0: producao propria (modelo PROPRIO, nao e o do Mad TV nem o do TVTower)
- Estado em `Game`: `own[4]` (filmes prontos: cat, blocks, q, name), `pstate` (0 nada / 1 roteiro comprado / 2 filmando) + `pcat/pblocks/pname/pdays/pbudget`, `soffer[3]` (roteiros a venda; regenerados todo dia, nao salvos).
- Indices de filme: 0..159 catalogo (ROM), 160..163 producoes (RAM, `g_OwnMv[]` + titulos em buffer). Acesso uniforme por `Mov(idx)` (sim.c) - todas as leituras de `g_Movies[idx]` que podem ver uma producao passaram a usar `Mov`; o catalogo continua sendo lido direto onde so ha filmes do catalogo (agencia, IA dos rivais).
- Titulo gerado = palavra A (16) + palavra B (16): "Golden Harbor". Genero/duracao (1-3 blocos) sorteados.
- Custos: roteiro 40 + 40*blocos k$; filmagem 150/250/400 k$; dias 2/3/4; qualidade = 30/50/70 + 0..25. Se a biblioteca estiver cheia o filme espera pronto ate liberar espaco (vender no Archive). Producao propria: +10% de audiencia (exclusividade).
- Savegame v8: bitmask de filmes passou a cobrir 164 indices (21 bytes) + 15 bytes de producao -> 140 caracteres (digitar 140 caracteres e longo; a tela de salvar quebra em linhas).
- Autoplay smart com producao (`NO_PROD=1` desliga, `PROD_BUDGET=0|1|2`), Normal, 200 dias, 60 partidas: sem producao 38% falencia / 61% casou; medio 31% / 60%; alto 38% / 56%. A producao ajuda pouco no bot (ele so a usa com folga de caixa) e as diferencas estao dentro do ruido (o RNG do dia mudou com os roteiros, entao nao comparar com os numeros da 0.9). Nao retunei a dificuldade.
- Segmento fixo: 16.020 de 16.384 bytes (restam ~360). Codigo novo em src/studio.c (segmento 11).

## 1.1: audio PSG, opcoes e revisao de balanceamento
- `src/audio.c` (segmento 12): escrita direta no PSG (portas A0h/A1h). Canais A = melodia (32 passos, 9 quadros cada, envelope por quadro), B = baixo, C = efeitos (0 mover, 1 confirmar, 2 erro, 3 novo dia, 4 conquista). Registro 7 = B8h (como a BIOS: 3 tons ligados, ruido desligado); R14/R15 (joystick) nao sao tocados. Musica e efeitos sao originais do projeto.
- Integracao no laco principal: `Audio_Tick()` a cada quadro (~50 bytes fixos); efeitos disparados por teclas de direcao, OK+mensagem (`Audio_Result` classifica a mensagem de recusa/sucesso por palavras-chave), novo dia, inicio do jogo e final feliz.
- **Nao testado ao vivo**: o smoke so confere contadores (`g_SfxCount`, `g_MusicPos`), o mixer (R7 = B8h) e que desligar os efeitos funciona. Ninguem ouviu o resultado; em hardware real o tempo da musica varia com 50/60 Hz (passo fixo em quadros).
- Mouse: sensibilidade com acumulador de resto, multiplicadores x0,5 / x1 / x2 / x3 (`g_PtrSpeed`).
- Espaco: a fonte (1,5 KB) saiu do segmento fixo para o segmento 3 (dados, banco 3, sempre mapeado no boot e lido so em `Ui_Init`): fixo 15.060 de 16.384 bytes.
- Revisao de balanceamento (autoplay, 100 partidas, 30 dias; smart): Easy 0% falencia (casou em 41% ate o dia ~26), Normal 2%, Hard 11%; politicas ingenuas/cuidadosas ~100% de falencia (como desejado: o jogo exige ler audiencias). Em 200 dias (60 partidas): Easy 71-88%, Normal ~50%, Hard ~15% de casamentos (ver tabelas de 0.8-1.0). Nao alterei parametros nesta versao; sao numeros de bot, nao de humanos.

## 1.2: correcoes do playtest
Relatos do usuario: musica "simplória e irritante", tempo rapido demais, textos "piscando", titulo piscando ~4 vezes.
- **Piscar (causa raiz medida)**: contador `g_DirectDraws` (Ui_Text/Ui_Fill fora de Ui_Begin) mostrou 2 desenhos diretos a cada movimento do cursor no predio e ~4-10 a cada troca de tela: era `Hint()`, que apagava a linha de dica (HMMV) e reescrevia o texto direto na tela. Tambem eram diretos o titulo, a tela de fim e o `Ui_Clear` de `StartGame`. Agora `Hint` compoe no buffer (`Ui_DirectBegin` zera profundidade e `Begin/End` proprios copiam o retangulo de uma vez), titulo/fim idem, e `StartGame` nao limpa a tela (o predio redesenha tudo). O smoke exige <= 8 desenhos diretos entre o inicio e o fim (medido: 4, todos do recarregar de codigo/cabecalho).
- **Titulo piscando**: `Draw_Title` era redesenhado (Ui_Clear direto + ~14 textos letra a letra, visivel como "digitando") a cada (a) mouse achado pela autodeteccao, (b) tecla M, (c) LEFT/RIGHT de dificuldade. Com um mouse na porta, enquanto a deteccao nao concluia a porta lia como joystick "fantasma" (direcoes/botoes aleatorios) e disparava LEFT/RIGHT => varios redesenhos (e a dificuldade mudava sozinha para Hard!). Correcoes: (1) entrada do joystick ignorada nos primeiros 90 quadros no modo automatico; (2) deteccao tambem por deltas que VARIAM (mouse em movimento) e nao so por (0,0) x4; (3) titulo atomico e `Title_Opts()` atualiza so as linhas de dificuldade/mouse.
- **Tempo**: velocidade 1 = 1 s por minuto de jogo (era 0,5 s), 2 = 0,5 s, 3 = 1/6 s. O jogo e cada novo dia (exceto se a velocidade for 3) comecam pausados com aviso; `P` retoma a velocidade anterior. Motivo: a viagem ate as salas leva ~5 s e o 1o programa e as 18:00 (60 min depois do inicio).
- **Musica** (`src/audio.c`): nova composicao original (secao A 8 compassos, secao B 8 compassos; Cmaj7 Am7 Dm7 G7 ... Fmaj7 Em7 A7 Dm7 G7), 96 BPM (colcheia = 15 quadros a 50 Hz / 18 a 60 Hz), melodia + baixo caminhando + acorde sincopado. Tocada em FM se a BIOS MSX-MUSIC ("APRLOPLL" em 4018h de algum slot; habilita E/S em 7FF6h) for achada: canal 1 Flute, 2 Acoustic bass, 3 Guitar; senao no PSG (A melodia com envelope, B baixo "beliscado", C acorde curto quando nao ha efeito). SCC nao e possivel (exigiria mapper Konami SCC, nosso mapper e ASCII8).
- **Teste**: `tests/audio_fm.sh` (C-BIOS_MSX2+ tem MSX-MUSIC emulado): deteccao, instrumentos 43/E2/26 e key-on nos 3 canais. Nunca ouvi o resultado; a escolha de instrumentos e andamento e uma suposicao.

## 1.3: salvar em SRAM
- Mapper: ASCII8 com SRAM (openMSX: `ASCII8SRAM8` / `ASCII8SRAM2` / `ASCII8SRAM32`). A SRAM aparece no banco 3 (A000h) quando se escreve em 7800h um valor com o "bit de habilitacao". No openMSX esse bit e a menor potencia de 2 >= numero de segmentos da ROM: **10h para 128 KB** (medido: com 10h a escrita/leitura em A000h funciona; 20h/40h/80h leem a ROM). Se a ROM crescer para 256 KB o bit passa a 20h.
- `src/slots.c` (segmento 13): `Sram_Detect()` na inicializacao sonda 10h, 20h, 40h e 80h num deslocamento livre (0600h) com dois padroes (A5h/5Ah e 5Ah/A5h) restaurando o conteudo; o primeiro que se comporta como RAM vira `g_SramBit` (0 = sem SRAM). Enquanto a SRAM esta mapeada o banco 3 nao le dados da ROM; as rotinas copiam de/para RAM e restauram o segmento anterior (como `Db_News`).
- Formato: cabecalho "MTVS" + versao 1; 4 slots de `1 + SAVE_BYTES` bytes (flag A5h + o mesmo bloco de bytes do codigo, com checksum Fletcher-16). `Sim_SaveCode`/`Sim_LoadCode` foram separados em `Sim_SaveBytes`/`Sim_LoadBytes` + codificacao base-32, entao SRAM e codigo compartilham validacao e formato (v8).
- Interface: Escritorio > Save/Load -> menu (salvar em slot / carregar de slot / codigo). Cada slot mostra dia, caixa e dificuldade. Carregar deixa o relogio pausado. Sem SRAM o item abre direto a tela do codigo. Esc no titulo abre os slots se algum estiver usado; senao a tela de codigo.
- Testes: `tests/sram.sh` roda 3 execucoes do openMSX: (A1) salva no slot 1 com o dinheiro alterado, (A2) numa NOVA execucao, Esc no titulo carrega o slot e o dinheiro volta (persistencia entre execucoes do arquivo de SRAM do openMSX), (B) ROM sem SRAM: `g_SramBit` = 0 e a tela abre direto no codigo. O smoke tambem salva/altera/carrega o slot 1.
- **Nao verificado**: hardware real. Cartuchos/flash carts com SRAM ASCII8 podem usar outro bit de habilitacao ou tamanho (por isso a sonda); se nenhum se comportar como RAM, o jogo usa o codigo. Escrever em A000h numa ROM comum nao tem efeito, e a sonda restaura o que alterar.
- Segmento fixo: 15.796 de 16.384 bytes.

## 1.4: interface (casca visual)
- **Paleta global** (`src/chrome.c`, `k_Pal`): as telas do Mad TV original capturadas no DOSBox usam so ~31 cores; reduzi para 16 mantendo o "papel" de cada indice do MSX (1 = preto = fundo, 14 = cinza do painel, 15 = branco, 0 = cinza escuro das sombras, 5 = azul dos campos, 6/12/13 = madeira...). Todo o jogo muda de cor (predio, barras, sprites) sem tocar nas telas.
- **Relevo**: `Chrome_Box` (luz em cima/esquerda, sombra embaixo/direita; face por HMMV quando alinhada a byte, bordas de 1 px por LMMV - `Ui_FillPx`). HUD (`Hud_Reset/Hud_Draw`, antes em main.c) com campos azuis e texto PRETO (7a variante da fonte, na VRAM abaixo do buffer: linha 730), caixas de dica (cinza) e mensagem (azul).
- **Titulo** estilo "Select a player": painel cinza, moldura de madeira, botoes Easy/Normal/Hard e New game/Load game com clique do mouse (`Title_Click`).
- **Arte do original**: os icones ($, relogio, coracao) sao recortados de uma captura do jogo (`tools/make_art.py original/ref2/mtv_000.png src/data/art_icons.h`) e ficam so na maquina local (`src/data/art_*.h` e `original/` no .gitignore). `scripts/build.sh` cria um `art_icons.h` com `ART_ICONS 0` se nao existir (HUD com texto). A ROM que eu envio tem os icones.
- **Bugs achados**: (1) `Ui_Fill` com largura 256 virava 2 pixels (u8): agora w = 0 significa 256; as colunas 254-255 do conteudo nao eram limpas. (2) **Texto passado para outro segmento**: `Hint("...")` chamado de uma tela banked recebia um literal que vive no segmento de quem chamou; ao virar chamada banked para `Chrome_Hint` esse segmento deixava de estar mapeado, o ponteiro lia lixo e o desenho rodava ~2,4 s (loop sem fim ate achar um zero). Correcao: `Hint` (codigo fixo) copia o texto para a RAM antes. **Regra: nunca passar para uma funcao banked um ponteiro para dado/literal de outro segmento banked; so RAM ou dados do codigo fixo.**
- Verificado nos screenshots fieis do smoke (todas as telas) e nos testes (unit, smoke, mouse, FM, SRAM).
- **Limites**: o "console inferior" do original (TV, ticker, 80 px de altura) nao coube: nossas telas precisam de ~160 px de conteudo. O fundo das salas continua preto/texto; as cenas ilustradas ficam para uma ROM de 256 KB. A conversao dos icones por captura em 320x200 reduzida para ~10 px perde detalhe.

## 1.5: o predio com a arte do original
- `tools/make_hub.py original/user/n1.png` compoe o fundo do predio (256x160) com recortes de uma captura do Mad TV (vista do predio): andares alternando verde/azul-ceu, portas (3 estilos), portas do elevador, plantas e quadros com transparencia (cor de fundo do recorte vira transparente), cabine do porteiro (andar 0), colunas de tijolos desenhadas em codigo e placas de andar 1-5. Quantiza cada pixel para a paleta global (sem dithering) e grava 3 segmentos de dados (`src/seg/seg_s16..18_b3.c`, 64 linhas x 128 bytes cada) + `src/data/art_hub.h` (`ART_HUB 1`). Tudo isso esta no .gitignore; `scripts/build.sh` cria `art_hub.h` com `ART_HUB 0` se nao existir, e entao o predio procedural da 0.5 e desenhado.
- ROM passou de 128 KB para 256 KB (`ROMSize = 256`): o bit de habilitacao da SRAM passa de 10h para 20h (a sonda da 1.3 ja cobre; os testes agora esperam 32). Segmentos: 0-1 fixo, 3-4 dados, 5-14 codigo, 16-18 arte.
- Desenho: `Ui_BlitRow` copia bytes 4 bpp direto para a VRAM (tela ou buffer); `ArtRect` (building.c) mapeia o segmento de arte no banco 3 por linha. O predio inteiro e 160 linhas x 128 bytes (24 jiffies); cada porta e redesenhada sozinha (celula 28x31) ao mudar a selecao, com rotulo preto (amarelo de fundo quando selecionada) e moldura amarela.
- O sprite do carro do elevador foi escondido quando ha arte (o fundo ja traz as portas); jogador e rivais continuam sprites.
- Verificado: build com e sem arte, unit, smoke, SRAM (sbit 32 e sem SRAM), FM, mouse.
- Limite: a arte foi recortada de UMA captura; o original rola verticalmente (13 andares) e tem o letreiro MAD TV no topo, o horizonte e a Betty; nao reproduzi isso.

## 1.6: cenas das salas
- `tools/make_scenes.py original/user` recorta 3 cenas de capturas do original (chefe n4.png, escritorio n3.png, agencia de publicidade 5.png - versao Amiga), quantiza para a paleta global e grava `src/seg/seg_s19..s23_b3.c` (linhas de `stride` bytes; `rps` linhas por segmento) + `src/data/art_scenes.h` (`ART_SCENES 1`, tabela `SCN_INIT`). Tudo no .gitignore; `scripts/build.sh` cria o cabecalho com `ART_SCENES 0` se faltar (telas de texto da 1.4/1.5).
- `Scene_Draw(id, dx, dy)` (chrome.c, segmento 14) mapeia o segmento de dados no banco 3 por linha e usa `Ui_BlitRow` (VDP direto). A cena do chefe/escritorio tem 256x111, a da agencia 120x111.
- Layouts: chefe = cena + painel cinza a direita (fala em 2 linhas, caixa, divida, limite, juros e o menu com a opcao selecionada num campo azul afundado); escritorio = cena + painel cinza embaixo com os 4 itens; agencia de publicidade = cena a esquerda (120 px), lista de ofertas no painel direito e detalhes (audiencia minima, spots, pagamento, multa) num painel embaixo. Clique do mouse sobre a cena nao faz nada.
- Custo: o desenho completo leva ~27-30 jiffies (~0,5 s: 14 KB por VDP direto + troca de segmento); por isso a tela do chefe deixou de ser redesenhada a cada hora (`ScreenDyn`).
- A cena da agencia recortada da versao Amiga inclui o cursor de seta vermelho da captura; e as cores vem da paleta global (16 cores), entao tons de pele ficam salmao.
- Verificado: build com e sem arte, unit, smoke (inclui travessia ate as salas), SRAM, FM, mouse.

## 1.7: cenas em mais salas
- `tools/make_scenes.py` agora le as capturas do DOS em `original/dos/*.png` (DOSBox, 320x200) e recorta 111 linhas (y=4..114) por sala: chefe, escritorio, publicidade, agencia de filmes, roteiros, estudios, biblioteca (arquivo), corretor, Betty, porteiro e supermercado; 13 segmentos (19-31), `SCN_*` em `art_scenes.h`. Tudo git-ignorado; o repositorio publico continua sem a arte.
- `Room_Frame(id)` (chrome.c): limpa a area, desenha a cena (120 px) e o painel cinza direito (x=122). Cada tela desenha a lista dentro do painel e os detalhes num painel de 256x48 em y=138. Sem arte (`ART_SCENES 0`) cada tela mantem o layout de texto.
- Cache da cena: ela fica no buffer (pagina 2); `g_BufScene` guarda o id e `Goto` o invalida. Sem isso, cada redesenho completo (D_CON) custava ~0,5 s e um aperto de tecla chegava a ser perdido (smoke: Estudios).
- Como o DOS foi capturado: DOSBox sem janela real, ponteiro por movimentos relativos (calibracao 3x), clique no elevador ate aparecer o quadro de salas; Ctrl+F5 salva a captura.
- Verificado so no emulador (openMSX, C-BIOS): build com e sem arte, unit, smoke (SHOTS=1 confere as telas), SRAM, FM, mouse. O teste `balance.sh` precisa de `CFLAGS_EXTRA=-DBALANCE_STATS`.

## 1.8: noticias e grade
- `make_scenes.py`: `FIXED_SEG` coloca a cena `news` (crop 150..270 de `original/dos/news.png`) no segmento 15, livre no mapa (0-1 codigo, 2 e 15 sem uso): sem aumentar a ROM. `.gitignore` cobre `seg_s15_b3.c`.
- Noticias: linha 0 do cursor (agencias) passou para o painel de baixo; as 11 linhas (3 do telejornal + 8 recebidas) ocupam o painel direito sem cabecalho; titulo completo, preco e frescor da linha selecionada ficam embaixo.
- Grade e escolha de filme/contrato: painel cinza 256x160 com campos afundados (hora, programa, anuncio; selecionado em azul). E uma aproximacao: nao ha capturas da grade original no repositorio local.
- Verificado so no emulador (openMSX, C-BIOS): build com e sem arte, unit, smoke (SHOTS=1), SRAM, mouse, FM.

## 1.9: sabotagem e apagar slot
- `Sim_RivalDay` (sim_ext.c, chamada em `EndDay`): cada rival vivo com Image > Image do jogador sorteia `SABOTAGE_PCT + 6*dificuldade` %; no maximo uma sabotagem por dia, escolhida entre difamacao (-2 Image), suborno dos reporteres (+6 h de idade nas noticias do telejornal) e roubo de um contrato de anuncio (sem multa, o slot fica sem anuncio). Sem estado novo: savegame inalterado. Efeito no bot "smart" (200 partidas, normal): falencia 1% -> 3%, caixa mediano 2526k -> 2572k (numeros de um bot, nao de humano). Para comparar sem sabotagem: `CFLAGS_EXTRA="-DBALANCE_STATS -DSABOTAGE_PCT=-6"`.
- Slots: menu "Erase a slot"; o 1o OK arma o slot (mensagem pede confirmacao), o 2o apaga; mudar a selecao desarma. `tests/sram.sh` agora apaga o slot 1 e confere que Esc no titulo cai na tela do codigo.
- Verificado so no emulador (openMSX, C-BIOS).

## 2.0: cobertura e finais
- `TOWER_MAX` 4 -> 6 (4 torres, rede a cabo, satelite): `k_TowerPct` {100,112,125,140,155,175,200}; custos 1500/3000/5000/8000/12000/20000; rivais continuam em no maximo `RIVAL_TOWER_MAX` = 4. O byte `tower` do save ja comportava o valor: formato igual. Alcance base 12,0 milhoes x2 = 24,0 cabe em u8.
- Finais: `g_Game.won` 1 = casamento, 2 = magnata (`TYCOON_CASH` = 10000k com os dois rivais falidos, checado no fim do dia), 0 + `game_over` = falencia / Image zerado. `Over_Draw` usa a cena da Betty, do estudio de noticias ou do chefe e um painel com dias, Image, caixa e cobertura; `main` esconde os sprites do predio ao entrar na tela final.
- Autoplay (bot "smart", normal): sem diferenca perceptivel (o bot raramente junta caixa para cabo/satelite); o magnata e raro no bot.
- Verificado so no emulador (openMSX, C-BIOS): os tres finais foram vistos em capturas (casamento pelo smoke; magnata e falencia por um script avulso que zera os rivais/caixa e acelera o dia).

## 2.1: tela de abertura
- `make_scenes.py` ganhou a entrada `title` (arte do usuario em `original/user/title.webp`, git-ignorada): recorte da moldura (130,10)-(1456,981), reducao LANCZOS para 256x196 (~4% mais estreita que a original) e quantizacao para a paleta global; nos segmentos 32-35 (`FIXED_SEG`), 128 bytes por linha, 64 linhas por segmento. `SPLASH_BG` = cor da borda (preenche as 8 linhas de cima e de baixo).
- ROM de 512 KB (`ROMSize` 512): bit de SRAM 40h, achado pela sondagem; testes passaram a esperar 64. Sem a arte o espaco extra fica vazio (a ROM publica continua com 512 KB).
- Fluxo: `g_Splash = 1` no boot; enquanto ligado, so um toque (tecla ou clique) e aceito e apenas desenha o titulo. Titulo, Esc-carregar e novo jogo ficam como antes. Testes: smoke, sram e mouse_real mandam um Esc antes de tudo para dispensar a abertura.
- Verificado so no emulador (openMSX, C-BIOS); a build sem arte tambem foi testada (abertura em texto). Em 128 KB/256 KB reais o flash cart precisa suportar 512 KB.

## 2.2: opcoes na SRAM e diagnostico
- Cabecalho da SRAM (8 bytes): magic "MTVS" [0-3], versao [4], opcoes [5-7]: `0xA5` valido, valor = sfx | musica<<1 | velocidade do mouse<<2, e valor^5Ah como checagem. `Options_Save/Load` em slots.c; `Options_Load` roda no boot depois de `Sram_Detect`; sem SRAM nada muda.
- Options redesenha so a linha mudada (`OptRow`): com o redesenho completo (agora com o bloco "This MSX") a 2a tecla de uma sequencia rapida era perdida.
- Testes: `sram.sh` ganhou F1/F2 (desliga efeitos + mouse rapido, reinicia, confere). Descoberta: o openMSX guarda a SRAM em `~/.openMSX/persistent/roms/<nome da rom>/`, entao a limpeza antiga (`persistent/*madtv*`) nunca apagava nada e as execucoes herdavam estado; smoke.sh e sram.sh agora limpam `persistent/roms/madtv-*`.
- `tests/HARDWARE.md`: roteiro de 10 testes para MSX2 real. Continua nao testado em hardware.

## 2.3: console inferior, Sammy, bonequinhos
- Console: HUD fica em y=162..187 da tela (TV da Betty | painel cinza hora/caixa | TV das emissoras + velocidade), dica em 188..199 e mensagem em 200..211. As telas de conteudo nao mudaram de coordenadas: `Ui_End` copia o buffer `YSHIFT` (24) linhas acima, exceto quando `g_Screen` e titulo/final ou `g_Splash`. O console e desenhado nas linhas 0..25 do buffer (`CB` = 94, u8 dando a volta) e copiado com `Ui_EndTo`. Cuidado: a fonte preta vive em 218.. do buffer (pagina 2, y 730) - usar linhas ali corrompe o texto preto (visto como texto "esfarelado").
- Mouse: `ScreenMouse(x, y + YSHIFT)`; cliques em y >= 162 so tratam a velocidade (TV da direita). Sprites do predio subtraem `YSHIFT` do Y.
- Sammy (`Sim_SammyDay`, fim dos dias 7, 14, ...): nota do jogador = `Sim_NewsQuality()` no fim do dia; rivais vivos = 25 + rnd(35) + 5*dificuldade. No bot "smart": falencia 3%, caixa mediano 2338k (era 2572k na 2.0: os Sammys dos rivais dao Image a eles).
- Sprites: planos 2-4 preenchimento, 5 elevador, 6-8 contorno (NSPR 7). Arte em `tools/make_sprites.py` -> `src/sprites_fig.h` (arte propria, versionada). O usuario achou feio: o desenho de 16x16 com cor por linha e limitado. Higgsfield/outra IA de imagem nao esta disponivel nesta sessao; o caminho e o usuario mandar uma folha de referencia e eu reduzir para 16x16 (2 camadas).
- Verificado so no emulador (openMSX, C-BIOS); a build sem arte tambem foi testada.

### 2.3 (cont.): bonequinhos a partir da folha do usuario
- A 1a versao (16x16 desenhada a mao) nao agradou; o usuario mandou uma folha gerada por IA (8 personagens: preview/fill/outline) e pediu algo como os personagens 3 (entregador de bone vermelho) e 5 (bone verde). O Pixa (Nano Banana) tambem foi tentado, mas o download das imagens e bloqueado pela rede do ambiente (403 em mcp.pixa.com).
- `tools/make_sprites.py` le a folha: painel *outline* (limiar de escuro) -> silhueta (flood fill) -> reducao BOX para ~12x22 px; painel *fill* -> cor dominante por linha (palette do jogo, sem os indices 0 e 1); cinza/pele abaixo do rosto herda a cor de cima; 2 ultimas linhas = sapato (6). Quadro 2 = pernas espelhadas (linha >= 16); direcao esquerda = espelho. Rival 2 = personagem 3 recolorido (8 -> 5, 13 -> 4).
- Boneco = 16x24 = 2 sprites 16x16 empilhados (cabeca/tronco e pernas) x 2 camadas = 4 planos; 3 bonecos + elevador + 2 do cursor = 15 planos; no maximo 2 camadas de cada boneco por linha de varredura (2 + 3x2 = 8 no pior caso: o limite do VDP).
- Padroes em `SPR_PAT_LO + 256` (slot 8 em diante), `k_SprPat[forma][direcao][quadro][parte][32]`; colunas de cor por plano em `SPR_COL_LO + plano*16`. A arte gerada fica em `src/data/sprites_art.h` (git-ignorado; `SPR_ART 1`); sem ela o build usa `src/sprites_fig.h` (versionado).

## 2.4: grade do original e micro-fonte
- Capturas do DOS: DOSBox sob Xvfb (`/tmp/dos.sh`, nao versionado). Fluxo ate a grade: `e` (English) -> Enter varias vezes -> clicar no 1o slot de nome, digitar um nome, Enter -> escritorio do Archie -> clicar no computador (262,62). O 1o clique precisa "capturar" o mouse (`xdotool mousemove` + click dentro da janela) antes dos movimentos relativos. Saidas locais: `original/dos/grid*.png`.
- `make_scenes.py`: cena `gridcol` (colunas esquerda 28 px + direita 20 px lado a lado, 48x106) no segmento 2; `Scene_Part(id, byte_inicial, nbytes, dx, dy)` desenha so parte das colunas. Bug achado: `rps` e u8 em `SceneDef` (cena com stride 24 tem 8192/24 = 341 linhas por segmento -> estourava e lia o segmento seguinte): gerador limita a 255.
- Grade ART: linhas de 16 px a partir de y=30 (7 linhas); hora x=32..64, programa x=66..168, anuncio x=170..232; rosa (cor 9) = hora que ja comecou (o original usa o mesmo rosa); detalhes em y=154..186. Setas: y 36-52/52-70 (x < 30), lixeira y 114-134; icones x >= 232, 17 px por icone.
- Micro-fonte: 36 glifos 3x5 (celula 4x6) em `k_Micro`, expandidos para VRAM (pagina 1, linhas 500-505; livres: variantes coloridas vao ate 495 e a preta vive na pagina 2) por `Micro_Init` em `Building_Reset`; `Micro()` copia com LMMM/TIMP. Rotulos das portas ficam 4 px/letra em vez de 6.

## 2.5: console vivo e desempenho
- Console: esquerda = titulo do filme no ar (`OnAir`: slot do jogador na hora atual) e, em micro-fonte BRANCA, "MAD34 FUN33 SUN33"; direita = ticker (`s_Tick`, copia na RAM da manchete; `Hud_Tick` a cada 12 frames, janela de 15 caracteres) e "DAY n BETTY n X1/PAUSE". A micro-fonte ganhou uma 2a faixa branca (VRAM pagina 1, linhas 506..511): `Chrome_MicroW`.
- Grade: o menu suspenso (`PK_*`) e desenhado por cima da grade ja pronta; 7 linhas (`PK_ROWS`), mouse so dentro da caixa.
- Perfil (variavel temporaria lida pelo openMSX): numa sala com cena, `Scene_Draw` custava 10 de 24 jiffies e `ClearContent` 2. Dividir/multiplicar por linha em `Scene_Part` (SDCC chama rotinas de biblioteca de ~300 ciclos) era a maior parte; agora um laco por segmento. `Room_Frame` so preenche a fresta de 2 px entre a cena e o painel.
- Medido sem ganho: `VDP_EnableSprite(FALSE)` ou `VDP_EnableDisplay(FALSE)` durante o desenho (1-2 jiffies, e a tela piscava) - removido. A escrita de VRAM pela CPU e limitada pelo VDP (~100-150 KB/s), por isso blits grandes continuam caros; guardar cenas na pagina 3 (224 linhas livres) e copiar com HMMM seria o proximo passo (cabem 4 cenas de 120x111 lado a lado).
- Verificado so no emulador (openMSX, C-BIOS); a build sem arte tambem foi testada.

## 2.6: elenco e cache de cenas
- `tools/convert_db.py` ganhou `build_cast`: pessoas = guids com nome (<= 18 caracteres) em `database_people*.xml` que aparecem como ator (function 2) ou diretor (function 5) em algum programa; score = fame*3 + power + humor + charisma + appearance + affinity/3; 64 atores + 24 diretores; estrelas por ranking (12% = 3, ate 35% = 2, ate 70% = 1). Dos 1380 vinculos ator/diretor dos 160 filmes so 191 tem nome no banco (anonimizado): o resto e sorteado por hash(guid) com probabilidade de estrela subindo com (critica + bilheteria) do filme. `g_Person[]` e `g_Cast[][4]` (diretor, ator 1, ator 2, soma) ficam no segmento 4; `Db_Cast`/`Db_Stars` em db.c copiam para a RAM.
- `Chrome_CastLines(m)` desenha as 2 linhas de micro-fonte (y 173/180) nos paineis de detalhes; micro-fonte nao tem "." nem ",": pontuacao some.
- Cache: `k_ScPin` = {filmes, anuncios, noticias, arquivo} nas vagas 0-3 (cada vaga 120x111 px: 2 por banda, bandas em 770 e 881; a fonte preta ocupa ate a linha 769). `Scene_PreStep` (chamada a cada quadro fora de titulo/final) carrega 12 linhas por vez; `Scene_Draw` usa HMMM da VRAM. Medicao (agencia): 20 -> 17 jiffies; a escrita da CPU na VRAM e a copia HMMM sao ambas limitadas (~100-130 KB/s no emulador), por isso o ganho e de ~15% so. `g_LastDraw` (main.c) e o gancho de teste.
- Verificado so no emulador (openMSX, C-BIOS); a build sem arte tambem foi testada.

## 2.7: fechamento
- `Sim_TakenBy(m)`: x = hash16(m, dia/TAKEN_WINDOW); levado se x%100 < TAKEN_BASE_PCT + estrelas*TAKEN_PER_STAR; quem leva = bit 9 de x (o outro se aquele faliu; nenhum se os dois faliram). `Sim_Buy` devolve 3. Bot do `balance.c` pula filmes levados.
- Escritorios dos rivais: `make_scenes.py` ganhou `rival1`/`rival2` (office.png cols 180-300 e 20-140; `inpaint_cursor` apaga a seta vermelha do DOS que vinha na captura) nos segmentos 36 e 37.
- Historico: `git filter-branch --index-filter 'git rm -r --cached --ignore-unmatch madtv-msx/screenshots' main..HEAD` + force-push com `--force-with-lease`; `screenshots/` esta no .gitignore.
- Verificado so no emulador (openMSX, C-BIOS); a build sem arte tambem foi testada.

## 2.8: audio, grade, emissora
- Audio: `s_Theme` escolhe `k_Lead/k_Root/k_Comp` (tema 0) ou `k_Lead2/k_Root2/k_Comp2`; troca recomeca a musica (`g_MusicPos = 0`). Efeitos agora `NSFX` = 7. Nada disso foi ouvido: so os registros no emulador.
- Regra reaprendida: ponteiros para literais devem apontar para o codigo FIXO se forem lidos de outros segmentos (`g_StationName` apontando para o segmento 38 saiu como lixo na tela do console; corrigido com `g_StName3` em sim.c).
- Segmentos: erro de link "Data overwrite at offset C000h" = o segmento 6 (sim_ext) passou de 8 KB. Solucao: `sim_ext2.c` + `screens_r.c` em `seg_s38_b2.c`. Os testes de PC compilam `sim_ext2.c` tambem.
- Save: bits 6-7 de b[0] = emissora; dificuldade nos bits 4-5 (leituras antigas `b[0] >> 4` viraram `(b[0] >> 4) & 3`).
- Verificado so no emulador (openMSX, C-BIOS); a build sem arte tambem foi testada.
