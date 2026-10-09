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
