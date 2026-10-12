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

## 5. Pendências

- [ ] Mapear as rotinas que preenchem `$9900`/`$9600`/tabela V (`a6`).
- [ ] Localizar tabelas de dados de pista (curvas, relevo, zonas) na ROM.
- [ ] Confirmar rótulos dos 54 estados em emulador (BlastEm com depurador).
- [ ] Protótipo Z80/V9958 da pista com dados sintéticos; medir FPS.
