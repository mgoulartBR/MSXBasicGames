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

## Física e estrada (SRSE.EXE desempacotado; DS real = imagem+0x3E960, **não** 0x3E830)

### Tempo
- PIT canal 0, modo 3, divisor 0x4000 -> **72,8 Hz**. A ISR (img 0x1FE2) incrementa o contador de 32 bits em DS:0x05F8.
- A física roda **por tick** (dt = contador - último contador); o desenho espera 9 ticks por frame
  (`0x282EE`, `add ax,9`) -> **teto de ~8,1 fps** no original (cai se o desenho demorar mais).

### Motor / câmbio (rotina por tick em img 0xD29D)
- `rpm += aceleração_do_segmento_da_curva_de_torque` (DS:0x79F0 += 0x79F6); teto DS:0x6CBA.
- Curva de torque por marcha: entradas de 8 bytes em DS:0x7A26 (24 bytes por marcha = 3 segmentos
  {rpm_min, rpm_max, aceleração}); razão da marcha em DS:0x7A16.
- `velocidade = rpm / razão[marcha]` (DS:0x79E2); troca de marcha quando rpm passa do teto
  (rpm := velocidade * razão[nova marcha]).
- Modo "Cruising in town" (img 0x8EA8): aceleração por marcha {42,26,12,8,4}, razões {_,200,125,71,56},
  troca automática a 4500 rpm, teto 4700 -> velocidade máx. ~83 mph na 4ª.
- Dados de peças (motor, câmbio, carburador, pneus) alteram esses arrays em tempo de execução
  (carregados de HOT_DATA e das partes do carro).

### Avanço na pista
- Posição = segmento (`DS:0x7D30`) + subposição 0..7 (`0x7D32`); 8 subunidades por segmento.
- Avanço por tick a partir da velocidade (img 0xD544):
  - estrada (modo >= 4): limiares de mph {13,27,41,55,69,83,97,101,105,109,113,117,121,125,129} -> 1..16 subunidades;
  - arrancada (modo < 4): v<=71 -> v/8+1, senão v/4-8.
- Oponente: mesma lógica (DS:0x7E8A pos, 0x7E8C sub, 0x7E8E passo). IA com "rubber banding":
  diferença de posição > 12 acelera/freia o oponente; < 5 segmentos ele colabora/bloqueia;
  alvo lateral do oponente muda por flags do segmento (curvas) e se aproxima 1/8 por tick.
- Largura/limites laterais: 0x5DC..0xCE4 (÷8 ≈ 187..409 px), faixa extra 0x7D0..0xAF0; lateral do jogador
  inicia em 0xA00; centro 0x960. Colisão com borda = flags 0x1000/0xF000 por segmento.

### Desenho da estrada (img 0x2834C -> 0x282EE; cálculo em 0x27B6D..0x2833C; desenho em 0x2645:0x4B8C)
- **Pseudo-3D por segmentos**, ~23 linhas de projeção por frame: 1+14 linhas distantes (z = 4, depois
  (12-sub)+8k) e 1+7 linhas próximas (z = 8, depois sub+8+8k).
- Projeção horizontal: `x = 0xA0 - ((deslocamento_lateral*4)/z)`; com curva acumulada
  `off += z * curva[seg] / 4`. Vertical (colinas): `y = 0x3C + ((altura[seg]-altura_câmera)*4)/z`
  (ou 0x118 - .. para o lado do chão). São 6 trilhas projetadas por linha:
  borda esq., borda dir., acostamentos (0xE74 e 0x44C), faixa central (0x76C) e a altura.
- Tabelas por segmento (word cada): flags (bits 0x80/0x20 largura, 0xF000 obstáculos/cenário),
  curvatura, altura, cenário esq./dir. Pista de estrada tem até 0x3FC (1020) segmentos; arrancada 0x17C (380).
  (As tabelas de pista são geradas/carregadas em tempo de execução: no EXE estão zeradas.)
- O desenho final usa listas de coordenadas por linha + rotina assembly EGA/CGA com panning por CRTC.

### Implicações para o MSX2
- 23 linhas x (grama/pista/acostamento/faixa) = ~90–140 comandos VDP (LMMV/HMMV) por frame, ou menos
  reusando fundo pré-desenhado: cabe com folga em Screen 5.
- Física a 72,8 Hz pode ser reamostrada para 50/60 Hz (tick do VDP) sem mudar o gameplay:
  integrar com dt fixo e usar tabelas pré-calculadas (rpm -> velocidade) em 8/16 bits.
- Meta realista: **10–15 fps** na estrada (acima dos ~8 fps do original).

## Protótipo MSX2 da estrada
Ver `msx2/road/README.md`: Screen 5, ~7 fps (janela 256x112) a ~16 fps (160x56), medidos no openMSX.

## Dados do jogo (`tools/hotdata.py` extrai tudo para CSV/JSON; validado contra os preços nos textos)
HOT_DATA = 13 blocos consecutivos (tamanhos 396,260,260,312,520,156,1850,2,200,18,18,24,48), carregados em DS:
| Bloco | Tamanho | Conteúdo |
|---|---|---|
| 0 | 22 x 18 B | pilotos oponentes: `[1]`,`[2]` = parâmetros (somam 30), `[6..7]` = offset do nome no pool; `[8..9]` = carro (-1, preenchido em execução); `[10]` ... última = "The King" |
| 1 | 26 x 10 B | carros à venda: `u16 preço, u8 desempenho(0-8), u8 índice da figura, u16 flags, u16 offset do nome, u16 ptr atributos` (25 carros + slot extra) |
| 2 | 26 x 10 B | por carro: contagem + 4 ids de peças/atributos (DS:0x04xx = pequenos vetores de cor/variante) |
| 3,4,5 | 312 / 520 / 156 B | geometria/camadas das figuras (retângulos por carro); formato ainda não decodificado |
| 6 | 1850 B | pool de texto: `\x02`+nome do carro, descrição com `$preço`; `\x01`+nomes (22 pilotos, "The King", "The Geek", "1963 Corvette") |
| 8 | 200 B | textos do placar ("USA 1", "Eagle", "Not 4 sale") |
| 9,10,11 | 18/18/24 B | tabelas de words usadas pela decisão do oponente aceitar/recusar a aposta |
- **Catálogo de peças**: 43 entradas de 8 bytes em DS:0x4808: `variante, máscara de marca (1 GM, 2 Ford, 4 Chrysler, 7 todas), categoria (0 motor, 1 câmbio, 2 carburador, 3 coletor, 4 pneus), 0, ptr texto (DS:0x2C02+ptr), ...`; o **preço é o word em DS:0x4806+8k**.
- Os 25 carros e 43 peças do EXE têm preços idênticos aos escritos nos textos (validação 100%).

## Modelo de desempenho do carro (`tools/powertrain.py`, porte de SRSE.EXE img 0xDCBE)
**Peças de fábrica a partir dos `flags` do carro** (u16 do registro de carro): `bits 1-3` = marca (0 GM, 1 Ford, 2 Chrysler),
`4-5` pneus (0 comum, 1 marca, 2 slicks), `6-7` câmbio (0 automático, 1 3 marchas, 2 4 marchas, 3 corrida),
`8-10` motor (0 V-6, 1 V-8 pequeno, 2 V-8 grande), `11-13` coletor (0-4), `14-15` carburador (0 2brl, 1 4brl, 2 racing).
Confere com as descrições ("V-8 3sp", "4 brl. 4sp", "Racing slicks").

**Fórmula** (tudo em ponto flutuante duplo no original; tabelas em DS:0x54B2..0x556E):
```
acc  = 0,09*f1 + 0,05*f2 + 0,01*f3            # avarias (bits de dano do carro)
     + T_corpo[perf]*0,1/88                    # T_corpo = 40,44,48,60,64,68,80,84,88
     + T_combo[5*carb+coletor]*0,15/100       # 0,0,100,0,0 | 0,50,0,100,75 | 0,0,0,50,100
     + T_motor[motor]*0,25/100                # 0,50,100
     + T_cambio[cambio]*0,15/100              # 0,33,66,100
     + (1 - |trim/8|)*0,2                     # trim do carburador -8..8
acc *= (10000 - desgaste_motor)/10000
B = (0,8 se automático senão 1)*acc*0,3 + 0,7 ;  A = min(130, 130*B)
por marcha g = 0..4:
  razão[g] = int(5700 / (T_razao[5*cambio+g]*A/100))        # 0 -> x = 0,2 (marcha neutra/inexistente)
  v = int(T_acel[5*motor+g]*B)                               # 500,600,300,100,70 | 600,700,350,120,90 | 600,760,380,140,90
  curva: 3 segmentos [0,w1) [w1,w2) [w2,5700)  com (w1,w2) = (2900,4500) (2900,5000) (2500,5200) por motor
  aceleração: seg0 = v/(g?5:1) * (g==1 ? (slicks ? 3 : 2) : 1); seg1 = v; seg2 = v/(g?10:1)
velocidade (mph) = rpm / razão ; rotação de corte = 4500/5000/5200 ; nº de marchas = 3,3,4,4
```
Resultado típico: 1939 Pontiac potência 98,7 (razões 231/115/57 -> ~100 mph); T-bird Roadster 124,1 (153/91/57/45 -> ~126 mph).
Desgaste dos pneus -> risco de falha: DS:0x54B2 (5 níveis por tipo de pneu: 2,5,8,11,14 | 0,2,4,6,8 | 0,0,1,2,4 %).
Limitação: transcrito do código; **não** comparado com execução do original (só consistência com preços/descrições dos 25 carros).

## Pendências
1. Gerador/carregador das tabelas de pista (curvatura, altura, flags, cenário) - ou desenhar pistas próprias.
2. Unidade de tempo da física (quanto vale 1 tick de aceleração) e calibrar contra as velocidades máximas acima.
3. Blocos 3/4/5 do HOT_DATA (camadas das figuras) e mapa das LIBs (garagem, cockpit, carros).
