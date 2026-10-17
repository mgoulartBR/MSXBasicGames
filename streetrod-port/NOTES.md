# Street Rod SE — notas de engenharia reversa

Os arquivos originais do jogo **não** estão no repositório (copyright). Só as ferramentas e as anotações.

## Arquivos do pacote (SRSE, 2014, baseado no Street Rod 1989)
| Arquivo | Tamanho | O que é |
|---|---|---|
| SRSE.EXE | 259.785 | MS C 5.x + **EXEPACK**. Desempacotado: 284.048 bytes (`tools/unexepack.py`) |
| HOT_DATA | 4.064 | Dados dos carros à venda (nome, descrição, preço) e tabelas |
| LIB1 / LIB1C | 76.662 / 23.412 | Biblioteca de imagens tela cheia: **EGA/Tandy (4bpp)** / **CGA (2bpp)** |
| LIB2 / LIB2C | 334.794 / 333.734 | Biblioteca de sprites/peças (EGA / CGA), centenas de itens |

## Formato das LIBs (decifrado, valida os 554 itens)
Rotina original: `SRSE.EXE` desempacotado, offset 0xE926 (chamada em 0x1592E). Implementação: `tools/srlib.py`.

```
u16 n_itens
n_itens x 30 bytes: u16 largura, u16 altura, u16 bytes_descomprimidos, u8 flag,
                    u8 tab[16], u8 ?, u32 offset, u16 tam_comprimido-1
dados: cada item começa e termina com 0xBC
```
Descompressão (entre os dois 0xBC):
- `00` ou `FF` + N  -> N+1 cópias do byte;
- byte presente em `tab[i]` -> corrida pré-definida i (contagens 1,1,2,3,4,2,3,5,6,6,4,7,7,5,8,8;
  valores 00,FF,00,00,00,FF,FF,00,FF,00,FF,FF,00,FF,FF,00 — constantes em DS:0x501E/0x502E);
- qualquer outro byte -> literal.

Resultado em EGA: **4 planos sequenciais** (plano0 depois plano1... cada um `altura x ceil(largura/8)` bytes);
`flag 2` = um plano só (máscara). CGA (LIB1C/LIB2C): 2 bpp, mesma compressão (ainda não renderizado).
Verificado: título de LIB1 item 0 bate **pixel a pixel** com o screenshot do jogo.

## Conteúdo
- LIB1 (9 telas): logo California Dreams, créditos, "King Street Rodders" (ranking), retratos
  da garota, lábios, jukebox (seleção de cores), tela de vitória. Item 0 = título.
- LIB2 (268 itens, 857 KB descomprimidos / 326 KB comprimidos): painéis do cockpit, carros
  (corpo, rodas, capô, teto em camadas; a camada ciano é a máscara de cor da pintura), rodas,
  carta DMV, semáforos, bombas, catálogo de peças, botões.
- Outras fatos: modos de vídeo CGA/EGA/VGA/Tandy; ~200 strings de regras no EXE
  (barganha, apostas $10–$100, pink slips, quebras de motor/câmbio/pneus, multa, calendário).

## Relevância para o port MSX2
- Dados: 857 KB em 4bpp/planar; reamostrar 320x200 -> 256x212 (Screen 5, 4bpp = metade desse volume, ~430 KB
  antes de compressão; com RLE similar ao original, ~170–250 KB: cabe em MegaROM de 512 KB).
- Carros em camadas + cor de pintura por paleta: encaixa bem com a paleta de 512 cores do V9938.

## Pendências
1. Extrair HOT_DATA (carros à venda) e as tabelas de peças/preços do EXE.
2. Localizar a física de corrida (drag/road) e a lógica da estrada em pseudo-3D.
3. Renderizar as LIBs CGA (2 bpp) e mapear qual item é qual (garagem, cockpit, peças).
