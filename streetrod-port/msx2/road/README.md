# Protótipo da estrada de Street Rod para MSX2

Pista pseudo-3D (projeção por segmentos, modelo extraído de `SRSE.EXE`, ver `../../NOTES.md`) em
**Screen 5 (256x212, 16 cores), V9938**, com 2 páginas de VRAM e troca no vblank. Código 100% próprio
(C/SDCC + Z80 assembly); nada do jogo original está aqui. Roda em cartucho ROM de 16 KB.

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
