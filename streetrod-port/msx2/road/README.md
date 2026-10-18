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
- A estrada é desenhada só no para-brisa (x 64..256, 80 linhas a partir de y=22). Tacômetro e velocímetro: a cada mudança, `HMMM` restaura o fundo
  limpo (página 2) e `LINE` desenha o ponteiro; o oponente é um `LMMM` com TIMP (cor 0 transparente) da folha de sprites.
- A ROM de 32 KB usa ENASLT no início para mapear o cartucho também na página 2 da CPU.
- `./shot.sh "<flags>" "t1 t2" prefixo` salva capturas de tela (VRAM→PNG) no openMSX.
