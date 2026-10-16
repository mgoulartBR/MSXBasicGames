# Street Rod SE — notas de engenharia reversa

Os arquivos originais do jogo **não** estão no repositório (copyright). Só as ferramentas e as anotações.

## Arquivos do pacote (SRSE, 2014, baseado no Street Rod 1989)
| Arquivo | Tamanho | O que é |
|---|---|---|
| SRSE.EXE | 259.785 | MS C 5.x + **EXEPACK**. Desempacotado: 284.048 bytes (`tools/unexepack.py`) |
| HOT_DATA | 4.064 | Dados dos carros à venda (nome, descrição, preço) e tabelas |
| LIB1 / LIB1C | 76.662 / 23.412 | Biblioteca de imagens tela cheia: **EGA/Tandy (4bpp)** / **CGA (2bpp)** |
| LIB2 / LIB2C | 334.794 / 333.734 | Biblioteca de sprites/peças (EGA / CGA), centenas de itens |

## Fatos confirmados
- Modos de vídeo suportados: CGA, EGA 320x200x16, VGA 320x200x16, Hercules (n/d), Tandy 320x200x16.
- Cabeçalho das LIBs (little-endian): `u16 n_itens, u16 largura_max, u16 altura_max, u16 bytes_max`.
  - LIB1: 9 itens, 320x200, 32000 B (=4bpp). LIB1C: 4 itens, 320x200, 16000 B (=2bpp).
  - LIB2: 268 itens, 184x26, 2392 B. LIB2C: 273 itens, 296x189.
- Depois do cabeçalho vem um bloco de 22 bytes (tabela de ~16 símbolos ordenados, parece
  dicionário de compressão) e então registros por imagem com
  `[u16 ?][u16 largura][u16 altura][u16 bytes descomprimidos][tabela]...[dados]`.
  Os dados têm entropia 6.1–7.1 bits/byte, ou seja, **são comprimidos**. O algoritmo ainda não foi identificado.
- `hot_data` é lido em registros de 0x17 (23) bytes (rotina em 0x160A5 da imagem, DS base ≈ 0x3E830).
- O jogo tem toda a economia em texto: ~200 strings de regras (compra/venda, barganha, apostas,
  quebra de motor/câmbio/pneus, multa por velocidade, "pink slips", calendário até o fim do verão).

## Relevância para o port MSX2
- Mecânicas = máquina de estados de menus + tabelas. Cabe fácil em Z80.
- Arte original é CGA/EGA 320x200; o MSX2 Screen 5 é 256x212 -> reamostrar ou redesenhar.

## Pendências
1. Achar a rotina de descompressão das LIBs (disassembly em torno do carregador de `?:lib1`).
2. Extrair imagens para PNG para dimensionar o trabalho de conversão.
3. Decodificar HOT_DATA (campos dos carros) e as tabelas de física/corrida em SRSE.EXE.
