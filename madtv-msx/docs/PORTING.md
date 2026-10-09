# Decisoes de portabilidade

```
Original system : Mad TV (1991, Rainbow Arts) - DOS/Amiga/ST, simulacao de gestao em tempo real
Data source     : banco do remake open source TVTower (XML) - ver LICENSES.md
MSX target      : MSX2 (V9938), 64 KB RAM minimo, cartucho ROM (nada testado em hardware real)

Video strategy  : Screen 5 (256x212, 16 cores), texto bitmap do MSXgl + barras com VDP HMMV.
                  Jogo de menus/telas estaticas: sem scroll; sprites de hardware so para cursor/personagens depois.
Audio strategy  : PSG (a definir; nenhuma implementacao ainda)
Input strategy  : teclado (setas) + joystick 1, ambos simultaneos. Mouse original -> cursor por setas/botao.
Memory strategy : tabelas const em ROM geradas por tools/convert_db.py; RAM so para estado do jogo.
ROM mapper      : ROM_ASCII8 (128 KB, 16 segmentos de 8 KB). Segmentos 0-2 = codigo fixo (4000h-9FFFh, 24 KB); banco 3 (A000h-BFFFh)
                  = janela de dados (segmento 3 = tabelas do catalogo; novos lotes = segmentos 4..15 via `db_data_s<N>_b3.c`).

Features preserved : dia 17h-1h em tempo real, grade 18h-0h, categorias, atributos de filme (critica/ritmo/bilheteria, FSK18, blocos),
                     compra de filmes, contratos de publicidade (preco/multa/repeticoes/prazo/audiencia minima), audiencia por hora,
                     Image dividido em 100 pontos com a regra de transferencia por slot, 3 emissoras, falencia.
Features adapted   : catalogo do TVTower reduzido a subconjunto curado (88 filmes, 24 anuncios, 24 noticias);
                     8 categorias (TVTower tem generos mais finos -> CATEGORIES em tools/convert_db.py);
                     preco de filme provisorio = media(criticas,ritmo,bilheteria) * price_mod (NAO e a formula do TVTower).
Features omitted   : Betty/presentes, noticias, producao propria, estudios, Sammys, sabotagem, torres/satelites, credito do chefe,
                     venda de filmes, salvar/carregar, audio, escolha da emissora, dificuldade, grade de "amanha".

Known limitations  : textos so em ingles/ASCII; titulos com ${...} nao resolvidos sao descartados.
Performance issues : texto resolvido com fonte em VRAM (ver Medicoes); ROM_32K quase cheia.
```

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
- Preco do filme (k$) = media(atributos) x price_mod/10 x blocos/2 + 50. Custo fixo diario 120k$. Falencia: caixa < -2000k$.
- Rivais (FunTV/SunTV): grade preenchida por heuristica (melhor de 6 candidatos por slot), sem dinheiro/contratos.

## Mapper: decisao (0.3.3)
Por que ASCII8 e nao ASCII16: ASCII16 so tem 2 bancos de 16 KB - para ter uma janela de dados sem tirar codigo do ar seria
preciso que todo o codigo coubesse em 16 KB. Com ASCII8 o codigo fixo ocupa 3 bancos (24 KB) e sobra um banco de 8 KB para
janela de dados/codigo. ASCII8 e suportado por openMSX, flash carts comuns e FPGA. Alternativas descartadas: ROM_48K/64K
(dependem de a cartucho decodificar a pagina 0/3 - menos portavel) e Konami (sem vantagem aqui).
Passo 1 (config): desligar recursos nao usados do MSXgl em `msxgl_config.h` (modos de video, sprites, Print FX/format/32 bits,
BIOS sub/disk): **31 907 -> 24 037 bytes (-7,9 KB)** sem mudar o comportamento (verificado por teste + screenshot).
Passo 2 (mapper): codigo fixo 20 170 bytes (4000h-8EC9h) de 24 576 disponiveis; dados no segmento 3: 3 895 de 8 192 bytes.
Regra atual: o banco 3 fica sempre mapeado no segmento 3 (dados lidos por ponteiro direto). **Quando o catalogo passar de
um segmento, introduzir acessores que mapeiam o segmento (SET_BANK_SEGMENT(3, n)) antes de ler** - previsto no 0.4 (noticias).

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
