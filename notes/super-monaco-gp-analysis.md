# Super Monaco GP (Mega Drive, USA) — notas de análise para porte MSX2+/V9958

> Notas de engenharia reversa para estudo. Não contêm código, gráficos ou dados da
> ROM original. A ROM **não** deve ser versionada (veja `.gitignore`).
> Endereços são da ROM `Super Monaco GP (USA) (En,Ja).md` (512 KB, checksum `$8612` OK).
> Itens marcados **[hipótese]** ainda precisam de confirmação em emulador/depurador.

## 1. Arquitetura geral

| Endereço | Função |
|---|---|
| `$020E` | Reset: init padrão (TMSS, Z80, VDP, PSG) |
| `$0308` | Teste de região; tela de erro (texto + fonte) em `$3B0`–`$46A` |
| `$0642` | Início real: valida checksum, limpa RAM, lê PAL/NTSC (`$FF1A`/`$FF1B`) |
| `$0830` | Copia driver de som (`$763F0`, `$1C10` bytes) para a RAM do Z80 |
| `$077E` | VBlank: contador `$FC22`, callback em `$FF0C`, driver de som em `$75C6E` |
| `$0734` | Loop principal: incrementa `$FC20`, salta pelo ponteiro de estado `$FF10` |
| `$07BC` | Copia o handler de HBlank (de `$7D0`, 19 words) para a RAM em `$FFD2` |

Máquina de estados: o jogo grava 54 endereços distintos em `$FF10`
(`move.l #estado,$FF10.w`). Mais referenciados: `$2C9E` (10x, hub de menus
**[hipótese]**), `$28F4` (5x, título **[hipótese]**), `$3B62` (6x), `$D618`,
`$ECB4`, `$E968` (3x cada).

## 2. Corrida

- `$3B62` = **init da corrida**: configura VDP (plano A em `$E000`, scroll
  horizontal **por linha**, plano 64x64, HINT ligado com contador 0 = a cada
  scanline), instala estado por quadro `$3A18` e callback de VBlank `$418C`.
- `$3A18` = **loop por quadro da corrida**: chama ~25 sub-rotinas de lógica
  (física, IA, colisão, HUD...), depois espera `$FC24 >= 4` (sincronia com VBlank).
- `$418C` = **VBlank da corrida**: troca de buffer, dispara DMA de 224 words
  (RAM `$FFA0C0` -> VRAM `$E802`, com auto-incremento 4 => 1 word por scanline
  da tabela de scroll horizontal do plano B) e roda uma de 4 fases por quadro
  (índice em `$FC24`).

## 3. Técnica de pseudo-3D (confirmada por leitura do código)

A pista é o **plano B**, deformado por scanline:

1. **Scroll horizontal por linha** (224 valores): a CPU monta o buffer em RAM
   (`$A0C0`), e o VBlank o copia por DMA para a tabela de HScroll.
2. **Scroll vertical por linha**: o handler de HBlank (em RAM, auto-modificável)
   escreve a cada scanline um valor de `$9D40` na VSRAM (plano B) e avança o
   ponteiro lendo a tabela sequencialmente. Quando o contador `$FF64`
   (inicial `$DD`) estoura, desliga o HINT (`$8AFF`).
3. Ambos os buffers são **duplo-bufferizados** (alternância via XOR `$1C0` em
   `$9202`).

Produtor: rotina em `$6C20`–`$6ECx`. Para cada uma das 224 linhas:

- lê de `$9900` um **índice de profundidade** por linha (0 = sem pista/céu);
- VScroll = `$190 - linha + (prof-1) + tabelaV[prof]`;
- HScroll = `tabelaH[prof]` (`$9600`) — o deslocamento da curva por profundidade;
- as tabelas `$9900` (mapa linha->profundidade) e `$9600` são preenchidas por
  outras rotinas (`$4F96`, `$5310`, `$700C`–`$7032`, `$71A0`) — **ainda a mapear**.

Resumo: é o esquema clássico de "estrada por linhas" — o plano B contém a
textura da pista e cada scanline escolhe qual linha da textura mostrar (VScroll)
e quanto deslocá-la (HScroll, curvas).

## 4. Implicações para MSX2+ (V9958)

- O V9958 **não tem** tabelas de scroll por linha. Alternativas:
  a) interrupção de linha (R#19) + escrever R#27 (scroll H) a cada N linhas;
  b) desenhar a pista por bandas com `HMMM`/`LMMM` (VDP -> VDP), copiando
     fatias da textura com deslocamento X por banda.
- Orçamento Z80 @ 3,58 MHz: ~228 ciclos/linha; usar bandas de 2–4 linhas.
- turbo R (R800) dá folga; mirar MSX2+ como mínimo e R800 como ideal.
- Como não há VSRAM, o "VScroll por linha" vira "qual linha da textura copiar"
  (opção b) — natural para `HMMM`.

## 5. Tabelas de profundidade (passo 2) — mapeamento

Duas rotinas por quadro na corrida, chamadas pelo estado `$3A18` (`$6ED0` e `$70CE`):

| Tabela (RAM) | Conteúdo | Quem preenche |
|---|---|---|
| `$9600` (120 words) | deslocamento X (curva) por passo de profundidade | `$7000`–`$7040`: rampa linear acumulada (16.16) com inclinação derivada da curvatura atual (`$87A` faz a divisão); depois soma `$9500` |
| `$9500` (96 words) | offset lateral/relevo por profundidade (também lido por `$9BE0` p/ inclinação do carro) | **a mapear** |
| `$9400`, `$9300` | versões subamostradas (cada 4 words) de `$9600` | `$7020`, `$70B8` |
| `$9800`–`$9850` | atributos por segmento (34 entradas na 1ª passada, 40 na 2ª) | `$7040`–`$70B0` / `$70CE`–`$7150` |
| `$9700` (96 words) | **altura de tela projetada de cada passo de profundidade** (inclui relevo) | 2ª passada `$70CE`–`$7150` (escreve de trás p/ frente a partir de `$97C0`); `$6750` só inicializa uma estrada plana |
| `$9900` (224 words) | **mapa linha de tela -> índice de profundidade** (0 = céu) | `$71A0`: percorre `$9700`; entre duas alturas consecutivas preenche as linhas com o mesmo índice; se a altura recua (crista de ladeira) preenche para trás e marca `$AEDA` |

Dados de pista em RAM (carregados na entrada da corrida; origem na ROM ainda não localizada):

- `$FF5B00`: segmentos de **curvatura** por posição (1ª passada, `$7040`); byte com bit 7 = marcador de volta/fim.
- `$FF7300`: segmentos de **relevo/ladeira** (2ª passada, `$70CE`).
- `$FF8300`: um byte por posição (-> `$920A`, provavelmente inclinação/zona atual).
- `$AE1A`: posição do jogador na pista (índice = `$AE1A >> 2`); `$9206`: comprimento da volta; `$9232`: contador de voltas.

Consumidores da projeção (só leitura de `$9700`/`$9600`): `$12E6`, `$9C4E`, `$52F0`
(posicionam carros/sprites usando a mesma função profundidade -> Y/X de tela).

A prévia de pista do menu usa `$4F92` (rampa linear simples, estado `$5344`).

Fórmula por linha (já confirmada, §3):
`VScroll[y] = 0x190 - y + (prof-1) + tabV[prof]`, `HScroll[y] = $9600[prof-1]`.

## 6. Implicações para o porte

A pista pode ser **pré-computada por posição**, pois depende só de (curvatura,
relevo, posição fracionária). Em MSX2+ a ideia é manter só:
96 passos de profundidade -> altura Y (`$9700`) e X (`$9600`), em 8 bits, e
desenhar por bandas com `HMMM`. A rampa linear de `$9600` é barata em Z80
(soma acumulada de 16 bits por passo).

## 7. Pendências

- [ ] Origem na ROM dos dados de pista (`$FF5B00`, `$FF7300`, `$FF8300`) e formato dos segmentos (rotinas `$72C6`, `$7322`, `$737C`).
- [ ] Preenchimento de `$9500` e tabela V (`a6` em `$6C20`).
- [ ] Confirmar rótulos dos 54 estados em emulador (BlastEm com depurador).
- [ ] Protótipo Z80/V9958 da pista com dados sintéticos; medir FPS.
