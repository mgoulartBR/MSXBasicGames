# Street Rod → MSX2: o guia completo do port

> Documento de engenharia do port de **Street Rod** (California Dreams, 1989, PC/DOS — edição "SE" de 2014) para **MSX2** (V9938, Z80, mapper ASCII16).
> Escrito ao fim do projeto, para servir de **manual de reprodução** e de **base para um jogo futuro "rigorosamente igual" ao Street Rod, com assets originais e tema brasileiro**.
> Tudo aqui foi medido ou decodificado durante o projeto; o que é estimativa ou hipótese está marcado como tal. Endereços do EXE original são `img` (offset na imagem desempacotada) ou `DS:` (offset no segmento de dados; `DS = img + 0x3E960`).

## Sumário

0. [Resumo executivo](#0-resumo-executivo)
1. [Visão geral do projeto e resultado final](#1-visão-geral-do-projeto-e-resultado-final)
2. [Material de partida e engenharia reversa do original](#2-material-de-partida-e-engenharia-reversa-do-original)
3. [Plataforma-alvo: MSX2, V9938 e as restrições que moldaram tudo](#3-plataforma-alvo-msx2-v9938-e-as-restrições-que-moldaram-tudo)
4. [Cadeia de ferramentas, ROM, mapper e organização em bancos](#4-cadeia-de-ferramentas-rom-mapper-e-organização-em-bancos)
5. [Pipeline de assets](#5-pipeline-de-assets)
6. [Mapa de VRAM e técnicas de apresentação](#6-mapa-de-vram-e-técnicas-de-apresentação)
7. [Arquitetura do código](#7-arquitetura-do-código)
8. [O motor da corrida](#8-o-motor-da-corrida)
9. [Sistemas de jogo](#9-sistemas-de-jogo)
10. [Áudio](#10-áudio)
11. [Entrada: teclado, joystick, mouse](#11-entrada-teclado-joystick-mouse)
12. [Salvar, carregar e ranking](#12-salvar-carregar-e-ranking)
13. [Testes, ferramentas de depuração e metodologia de verificação](#13-testes-ferramentas-de-depuração-e-metodologia-de-verificação)
14. [Cronologia completa: marcos, decisões, bugs e como foram resolvidos](#14-cronologia-completa-marcos-decisões-bugs-e-como-foram-resolvidos)
15. [Catálogo de armadilhas (o que dói de verdade)](#15-catálogo-de-armadilhas-o-que-dói-de-verdade)
16. [Orçamentos e medições](#16-orçamentos-e-medições)
17. [Estado final, limitações e pendências](#17-estado-final-limitações-e-pendências)
18. [Plano para o jogo futuro (versão brasileira, regras idênticas)](#18-plano-para-o-jogo-futuro-versão-brasileira-regras-idênticas)
19. [Apêndices](#19-apêndices)

---

## 0. Resumo executivo

**O que foi feito.** Um jogo completo em MSX2 (ROM de 512 KB, mapper ASCII16, ~6.200 linhas de código entre C/SDCC, Z80 assembly e Python de geração), que reproduz o ciclo do Street Rod: comprar carros usados no jornal, trocar peças abrindo o capô (com parafusos), afinar o ponto de ignição, pintar e enfeitar, abastecer, dirigir pela cidade até o Bob's Drive-In, desafiar 21 pilotos e o Rei em arrancadas e corridas de estrada (apostando dinheiro ou o carro), lidar com quebras, multas e o calendário (1º de agosto a 15 de setembro de 1963), salvar/carregar e entrar no ranking.

**Como foi possível.**
1. **Engenharia reversa do EXE** (desempacotar o EXEPACK, decifrar o formato das bibliotecas de imagens `LIB1/LIB2`, decifrar o `HOT_DATA`, ler o modelo de física e as regras de aposta direto do código x86).
2. **Pipeline de assets automático**: as imagens EGA de 320×200 são reamostradas para 256×212 (Screen 5, 16 cores), recoloridas por uma paleta MSX2 fixa, comprimidas e embutidas em bancos da ROM.
3. **Engine própria** da estrada pseudo-3D no V9938 usando **comandos de VDP** (HMMV/HMMM/LMMM/LINE), com ~6,9 fps medidos (o original é limitado a ~8 fps).
4. **Disciplina de teste**: scripts do openMSX que dirigem o jogo, tiram capturas da VRAM e comparam com o original; checagem estatística/numérica de modelos.

**Lições-chave (as 10 que mais custaram caro).**
1. Descobrir cedo **onde estão as regras no código original** (e não "inventar regras inspiradas"): as regras de aceitar/recusar apostas, o gate do Rei e o fim do jogo só ficaram certos depois de decodificar o x86.
2. **Banking planejado desde o início**. O núcleo de 16 KB estourou várias vezes; `bankify.py` (que redistribui funções por `#pragma codeseg`) salvou o projeto, mas a regra "função `__banked` só recebe escalares" custou dias.
3. **VDP é o gargalo**, não a CPU: o custo é ~1,2 ms por comando mais a área pintada. Pense em "quantos retângulos por quadro".
4. **Nunca confiar no emulador para hardware real**: mouse (leitura rápida demais), disco (BDOS) e tempo de VRAM divergem. Testar cedo no MSX real.
5. **Double-buffer de janelas e de cenas** (página oculta + troca de página/paleta no mesmo quadro) elimina piscadas e telas pretas; deveria ser a arquitetura desde o início.
6. **Medir antes de ajustar "no olho"** (rodas nas arcadas, cabeça do motorista): a medição no contorno do desenho acertou os 26 carros de uma vez; o ajuste visual por constantes errava carro a carro.
7. **SDCC gera código errado em casos reais** (expressão composta com `i32` e ternário; hoisting que explodiu o tempo): escrever passo a passo e conferir o assembly das rotinas críticas.
8. **Preservar IX/IY** em toda rotina Z80/BIOS chamada de C (SDCC usa IX como frame pointer).
9. **Dados do original estão em tabelas pequenas**: a "alma" do jogo (pesos, limiares) cabe em poucas centenas de bytes — copie, não reinvente.
10. **Documente as unidades**: o original usa 140 unidades de combustível, tick de 72,8 Hz, 8 subunidades por segmento… misturar unidades gerou bugs sutis.

---

## 1. Visão geral do projeto e resultado final

### 1.1 Entregáveis finais
| Item | Onde |
|---|---|
| ROM final | `release/streetrod_msx2_final.rom` (524.288 bytes, ASCII16, 512 KB) |
| Código-fonte | `release/streetrod_msx2_source.zip` (e o repositório em `streetrod-port/`) |
| Este guia | `release/PORTING_GUIDE.md` |
| Notas de engenharia reversa | `NOTES.md` |
| Desenho do jogo | `GAME_DESIGN.md` |
| Diário técnico do port | `msx2/road/README.md` (cronológico, com testes) |

### 1.2 Requisitos para jogar
- **MSX2** (ou superior) com **64 KB de RAM**, V9938/V9958, PSG; rodou em C-BIOS MSX2 (openMSX) e em MSX real.
- Controle: teclado (setas + ESPAÇO + SHIFT), **joystick** na porta 1 ou **mouse** na porta 1 (o original é de mouse).
- Opcional: **MSX-MUSIC / FM-PAC (YM2413)** para a música em FM. **PAC (SRAM) ou disco** (Disk BASIC/MSX-DOS) para salvar.
- Mapper: **ASCII16** (a ROM escreve em `0x6000`/`0x7000` para os emuladores detectarem; se não detectar, escolher "ASCII16").

### 1.3 O que o jogo tem (lista funcional)
- Título e créditos (arte original da LIB1), nome do jogador (carteira DMV com teclado na tela), HOW TO DRIVE.
- **Garagem clicável**: carros (até 4), peças reservas (até 12), jornal, calendário, rádio, posto, compressor (pintura), capô (motor), porta (salvar/carregar/créditos/sair), rua.
- **Jornal**: 14 carros usados/dia com preço 80–120%, barganha, 43 peças, venda de peças a 50%.
- **Motor**: janela com motor + coletor + carburadores, parafusos (2 cliques soltam, 2 apertam), troca de peças por categoria, **TUNING** (ponto −8..+8).
- **Pintura**: 12 cores, 8 adesivos, teto rebaixado, sem para-choques.
- **Posto Gus' Gas**, **Calendário**, **Rádio** (tema original + 5 estações).
- **Rua**: viagem automática de 420 segmentos (radares, multas, consumo).
- **Bob's Drive-In**: carros chegam sozinhos; 22 pilotos (21 + Rei); retratos individuais; regras de aposta do original.
- **Corridas**: arrancada (380 segmentos) e estrada (520 segmentos, curvas e ladeiras), semáforo, queima de largada, câmbio manual/automático, oponente com IA, retrovisores com vista traseira e oponente, quebra de motor/câmbio/pneus, perda do carro, destruição, pink slips.
- **Progressão**: dinheiro, dia (46 dias), vitórias, "King of the County", ranking persistente (10 melhores).

### 1.4 Números do código final
| Arquivo | Linhas | Papel |
|---|---|---|
| `main.c` | ~2.980 | jogo inteiro (uma única unidade de compilação) |
| `vdpasm.s` | ~1.360 | VDP, carregador de assets, LZ, som, entrada, save, ISR |
| `proj.s` | ~210 | projeção da estrada em assembly |
| `gen_assets.py` | ~650 | geração de todos os assets e tabelas |
| `cars_art.py` | ~230 | carros: corpo, rodas, arcadas, janelas, variantes |
| `music_gen.py` + `theme_data.py` | ~360 + dados | estações de rádio e tema original |
| `gen_tables.py` | ~145 | tabelas de física por carro |
| `bankify.py` | ~90 | distribuição de código em bancos |
| `ihx2rom.py` | ~40 | montagem da ROM |
| `ptmodel.h` | ~55 | modelo de desempenho do carro (inteiros) |
| `tools/*.py` | — | `unexepack`, `srlib`, `hotdata`, `powertrain`, `physsim`, `extract_lib` |

---

## 2. Material de partida e engenharia reversa do original

### 2.1 O pacote de partida
Edição **Street Rod SE (2014)**, baseada no Street Rod de 1989 (California Dreams). Arquivos:

| Arquivo | Tamanho | Conteúdo |
|---|---|---|
| `SRSE.EXE` | 259.785 B | C da Microsoft 5.x + **EXEPACK**; desempacotado = 284.048 B |
| `HOT_DATA` | 4.064 B | 13 blocos de dados: pilotos, carros, textos, tabelas de aposta |
| `LIB1` / `LIB1C` | 76.662 / 23.412 B | 9 telas cheias, EGA/Tandy (4 bpp) / CGA (2 bpp) |
| `LIB2` / `LIB2C` | 334.794 / 333.734 B | **268 itens** (sprites, cockpit, carros, peças, cenários) EGA / CGA |
| `README` | — | créditos da edição SE (dados de carros e correção da abertura) |
| Manual (PDF) | — | descreve as regras em linguagem natural; **leia primeiro** |

**O manual é uma fonte de regras tão importante quanto o EXE.** Várias mecânicas (parafusos "dois cliques para soltar", chamar o piloto encostado no poste, "alguns só aceitam estrada depois de uma arrancada", "os melhores só correm depois de vitórias", "carro bonito = mais disposição") só foram corretamente implementadas depois de relermos o manual com o código ao lado.

### 2.2 Desempacotar o EXE (EXEPACK)
- Cabeçalho MZ; o ponto de entrada aponta para o stub EXEPACK com assinatura `RB` (`0x4252`) no cabeçalho de 16 ou 18 bytes.
- O dado comprimido termina logo antes do cabeçalho; a descompressão é **de trás para frente** (comandos de preenchimento `0xB0`/`0xB2` e cópia). `tools/unexepack.py` implementa isso (entrada: `SRSE.EXE`; saída: imagem de 284.048 B, mais a tabela de relocações).
- **Segmento de dados real:** `DS = img + 0x3E960` (não `0x3E830`, que foi a primeira hipótese e produziu endereços errados durante dias). Confirmação: o texto "Chicken" está em `img 0x4278D` = `DS:0x3E2D`.
- A pilha/stub do runtime C começa em `img 0x0000`; a `main()` chama uma sequência de rotinas "far" (`call 0x1E16:xxxx` = segmento de biblioteca do MS C).
- **Dica de processo:** desmontar com um desassembler linear (`ndisasm -b16`) gera ~116 mil linhas (`all.asm`). O desassembly **linear desalinha** em tabelas de dados embutidas e por isso produz *falsos positivos* em buscas por imediatos (ex.: o valor `0x3D14` aparece dentro de outras instruções). Regra de ouro: **confirme sempre pelo contexto** (a instrução anterior faz sentido? há um `ret` antes?).

### 2.3 Formato das bibliotecas de imagens (`LIB1`, `LIB2`)
Rotina original em `img 0xE926` (chamada em `0x1592E`). Implementação de referência: `tools/srlib.py`.

```
u16  n_itens
n_itens × 30 bytes:
    u16 largura, u16 altura, u16 bytes_descomprimidos, u8 flag,
    u8  tab[16], u8 ?, u32 offset, u16 tam_comprimido-1
dados: cada item começa e termina com o byte 0xBC
```
Descompressão (entre os dois `0xBC`):
- `00` ou `FF` seguido de `N`  → N+1 cópias do byte;
- byte presente em `tab[i]` → "corrida" predefinida *i* (contagens `1,1,2,3,4,2,3,5,6,6,4,7,7,5,8,8`, valores `00,FF,00,00,00,FF,FF,00,FF,00,FF,FF,00,FF,FF,00` — constantes em `DS:0x501E/0x502E`);
- qualquer outro byte → literal.

Resultado EGA = **4 planos sequenciais** (plano 0, depois 1, 2, 3; cada um `altura × ceil(largura/8)` bytes). `flag 2` = um plano só (máscara). Para obter o pixel: `idx = b0 | b1<<1 | b2<<2 | b3<<3`.
**Verificação:** o título (LIB1 item 0) bate **pixel a pixel** com uma captura do jogo original. Foram validados os 554 itens (268 de LIB2 + 9 de LIB1, em EGA e CGA).

**Índices úteis da LIB2** (os usados no port; os números são os do próprio arquivo):
| Item(ns) | Conteúdo |
|---|---|
| 13 | semáforo (duas cabeças 24×32) |
| 14 | **cockpit** 320×190 |
| 15 / 16 | carteira DMV / livro da Highway Patrol (o 16 vira a tela de nome) |
| 21 / 22-26 | primeira página do jornal / manchetes do dia |
| 28, 65, 106 | pneus: faixa branca, cromado, slick |
| 32 | **garagem** |
| 39…201 (26) | **corpos de carro** (lista `BODY` em `cars_art.py`; o 26º é o Corvette = 201) |
| 57, 11, 249 / 248 | visão traseira do oponente (sedã, perua, Corvette) / carro por trás |
| 71 | tela **TUNING** (200×175) |
| 76-78 / 79-83, 104, 105 / 84-86 | motores / coletores / carburadores |
| 87, 88, 89, 138, 94, 95 | parafuso erguido / meio / apertado, furo, fiação solta / ligada |
| 90 | posto **Gus' Gas** |
| 93 | **Bob's Drive-In** |
| 99-102 | câmbios; 65-68 pneus (peças) |
| 186-193 | adesivos |
| 194-199, 202-217 | 22 retratos dos pilotos |
| 219 / 221 | calendários JUN-JUL / AGO-SET |
| 227-232 | painel e botão/alavanca do câmbio |
| 236, 238, 258 | placas DRIVE-IN, GUS' GAS, COUNTY LINE |
| 267 | colinas + grama do horizonte |
**LIB1:** 0 título, 2 "California Dreams presents", 3 créditos (placa "AD 1989"), 4 ranking "King Street Rodders", 8 vitória; 7 = "jukebox" (era a **trava de proteção** do jogo — acertar a cor perguntada; ignorada de propósito).

### 2.4 `HOT_DATA`: 13 blocos
Carregados por uma rotina que lê o arquivo bloco a bloco para endereços fixos do `DS`: o nome do arquivo (`hot_data`) está em `DS:0x4E0A`, a **tabela de destinos** (13 words) fica logo antes da **tabela de tamanhos** (`DS:0x4EBE`, 13 words):

| Bloco | Tamanho | Destino (DS) | Conteúdo |
|---|---|---|---|
| 0 | 22×18 B (396) | `0x7FF8` | **pilotos** |
| 1 | 26×10 B (260) | `0x7D86` | **carros à venda** |
| 2 | 260 | `0x8DF0` | por carro: contagem + 4 ids de atributos |
| 3, 4, 5 | 312 / 520 / 156 | `0x7682`, `0x70E4`, `0x818C` | geometria/camadas das figuras (**não decodificado**; não foi necessário) |
| 6 | 1850 | `0x2C02` | **pool de texto** (nomes de carros, descrições, nomes dos pilotos) |
| 7 | 2 | `0x7D16` | contador |
| 8 | 200 | `0x239E` | textos do placar ("USA 1", "Eagle", "Not 4 sale") |
| 9 | 18 | `0x7D68` | tabela T9 (9 words) |
| 10 | 18 | `0x8BAE` | tabela T10 (9 words) |
| 11 | 24 | `0x8DD8` | tabela T11 (12 words) |
| 12 | 48 | `0x784A` | 8 entradas de 6 bytes (**não usado no port**; parece o cabeçalho do ranking) |

**Registro de carro (10 bytes):** `u16 preço`, `u8 desempenho (0..8)`, `u8 índice da figura`, `u16 flags`, `u16 offset do nome` (no pool; os nomes iniciam com `\x02`), `u16 ptr de atributos`.
`flags` (u16): bits 1-3 marca (0 GM, 1 Ford, 2 Chrysler), 4-5 pneus (0 comum, 1 marca, 2 slick), 6-7 câmbio (0 automático, 1 de 3, 2 de 4, 3 de corrida), 8-10 **motor** (0 V-6, 1 V-8 pequeno, 2 V-8 grande), 11-13 coletor (0-4), 14-15 carburador (0 2 bocas, 1 4 bocas, 2 racing).
**Registro de piloto (18 bytes):** `[0]` nível (0 = Mike…, 1, 2 = os melhores/Rei), `[1],[2]` parâmetros de habilidade (somam 30; `a` = "habilidade" na troca de marcha), `[4..5]` word não decodificado (valores ~0x04AC; provavelmente ponteiro para atributos), `[6..7]` offset do nome no pool, `[8..9]` carro (preenchido em execução), **`[0xb]` estado** (1 = já apareceu, `0xFF` = foi embora), **`[0xc]` corridas valendo contra ele**, **`[0xd]` vezes que ele ganhou**, `[0xe]` opções cosméticas (bits 4/2/1 = flags 13-15 do carro), **`[0xf]` adesivo** (5 bits), **`[0x10]`** "já trocou de carro uma vez".
O 22º piloto (índice 21) é **The King** (tem carro fixo, o 26º slot = 1963 Corvette).
**Catálogo de peças:** 43 entradas de 8 bytes em `DS:0x4808`: `variante, máscara de marca (1 GM, 2 Ford, 4 Chrysler, 7 todas), categoria (0 motor, 1 câmbio, 2 carburador, 3 coletor, 4 pneus), 0, ptr texto (DS:0x2C02+ptr)`; o **preço é o word em `DS:0x4806+8k`**. Os 25 carros e 43 peças têm preços **idênticos** aos escritos nas descrições (validação 100%) — ótimo teste de integridade de extração.

### 2.5 Strings e mensagens
- O texto vive em `DS:0x2400…0x4E00` como strings terminadas em `0`, **cada uma começando com um byte de contagem de linhas** (`\x01`, `\x02`, `\x03`) usado pela caixa de diálogo.
- A crase `` ` `` aparece como separador em locais de pontuação (`Chicken`!`) e como "espaço não-quebrável" (`don't``have``enough``dough`) — é artefato do formatador de texto do original; ao converter, trate-a conforme o contexto.
- **Mensagens são referenciadas por ID, não por ponteiro:** `call 0x0:0x3AB8` recebe um inteiro; `string_offset = id + 0x2C01` (em `DS`). Foi por isso que buscas por ponteiros nunca achavam as mensagens. Exemplo: `0x1761` → "You're gonna hafta get more gas"; `0x1113` → "I don't got time for amateurs like you!"; `0x122C` → "Chicken!"; `0x131B` → "You don't have enough dough for that bet!".
- Tabelas de respostas de aposta: recusas em `DS:0x51FC` (5 por tipo, passo 10 bytes) e aceites em `DS:0x5210` (3 por tipo, passo 6 bytes); `tipo 0` = arrancada, `1` = estrada.

### 2.6 Tempo e física (`img 0xD29D` e vizinhança)
- **Relógio:** PIT canal 0, modo 3, divisor `0x4000` → **72,8 Hz**; ISR em `img 0x1FE2` incrementa um contador de 32 bits em `DS:0x05F8`.
- **Física por tick** (`dt = contador − último`); o desenho espera 9 ticks por quadro (`add ax,9` em `0x282EE`) → **teto de ~8,1 fps** no original.
- **Motor/câmbio:** `rpm += aceleração_do_segmento` (3 segmentos de curva de torque por marcha, entradas de 8 bytes em `DS:0x7A26`); `velocidade = rpm / razão[marcha]`; troca de marcha quando o rpm passa do teto (`rpm := velocidade × razão[nova]`).
- **Avanço na pista:** posição = segmento (`DS:0x7D30`) + subposição 0..7 (`0x7D32`). Por tick: estrada (modo ≥ 4): limiares de mph `{13,27,41,55,69,83,97,101,105,109,113,117,121,125,129}` → 1..16 subunidades; arrancada: `v ≤ 71 → v/8+1; senão v/4−8`.
- **Oponente:** mesma lógica (`DS:0x7E8A` posição, `0x7E8C` sub, `0x7E8E` passo). IA com *rubber banding*: diferença > 12 segmentos acelera/freia; < 5 colabora/bloqueia; alvo lateral muda com flags do segmento.
- **Limites laterais:** `0x5DC..0xCE4`; jogador inicia em `0xA00`; centro `0x960`; colisão com borda = flags `0x1000/0xF000` por segmento.
- **Modo "Cruising in town"** (`img 0x8EA8`): aceleração por marcha `{42,26,12,8,4}`, razões `{_,200,125,71,56}`, troca automática a 4500 rpm, teto 4700 → ~83 mph na 4ª.

### 2.7 O modelo de desempenho do carro (`img 0xDCBE`, `tools/powertrain.py`)
```
acc  = 0,09*f1 + 0,05*f2 + 0,01*f3            # avarias (bits de dano do carro)
     + T_corpo[perf]*0,1/88                    # T_corpo = 40,44,48,60,64,68,80,84,88
     + T_combo[5*carb+coletor]*0,15/100        # 0,0,100,0,0 | 0,50,0,100,75 | 0,0,0,50,100
     + T_motor[motor]*0,25/100                 # 0,50,100
     + T_cambio[cambio]*0,15/100               # 0,33,66,100
     + (1 - |trim/8|)*0,2                      # trim -8..8
acc *= (10000 - desgaste_motor)/10000
B = (0,8 se automático senão 1)*acc*0,3 + 0,7 ;  A = min(130, 130*B)
por marcha g = 0..4:
  razão[g] = int(5700 / (T_razao[5*cambio+g]*A/100))
  v = int(T_acel[5*motor+g]*B)            # 500,600,300,100,70 | 600,700,350,120,90 | 600,760,380,140,90
  curva: [0,w1) [w1,w2) [w2,5700) com (w1,w2) = (2900,4500) (2900,5000) (2500,5200) por motor
  aceleração: seg0 = v/(g?5:1) * (g==1 ? (slick ? 3 : 2) : 1); seg1 = v; seg2 = v/(g?10:1)
velocidade (mph) = rpm/razão; rotação de corte = 4500/5000/5200; nº de marchas = 3,3,4,4
Risco de falha de pneus (DS:0x54B2): 5 níveis por tipo: 2,5,8,11,14 | 0,2,4,6,8 | 0,0,1,2,4 %
```
Exemplos: 1939 Pontiac 98,7 (≈100 mph); T-bird Roadster 124,1 (≈126 mph). No port, o modelo vive em `ptmodel.h` em **inteiros**; foi conferido contra o Python em **14.580 combinações** (erro ≤ 1) e no próprio Z80 (`pt_dbg.c`).
**Limitação honesta:** o modelo foi transcrito do código e **não** comparado com uma execução do original (só consistência com preços/descrições). Para o jogo futuro vale instrumentar um emulador DOS e comparar tempos de 0-60.

### 2.8 A estrada pseudo-3D (`img 0x2834C`, `0x27B6D..0x2833C`, `0x2645:0x4B8C`)
- Pseudo-3D por segmentos, ~23 linhas de projeção por quadro: 1+14 distantes (z = 4, depois `(12-sub)+8k`) e 1+7 próximas (z = 8, depois `sub+8+8k`).
- Projeção horizontal `x = 0xA0 − (deslocamento_lateral×4)/z`; curva acumulada `off += z×curva[seg]/4`; vertical `y = 0x3C + ((altura[seg]−altura_câmera)×4)/z`.
- **Seis trilhas** por linha: bordas esq./dir., acostamentos (0xE74 e 0x44C), faixa central (0x76C) e altura.
- Tabelas por segmento (word): flags (bits 0x80/0x20 largura, 0xF000 obstáculos/cenário), curvatura, altura, cenário esq./dir. Estrada: até 1020 segmentos; arrancada: 380 (`0x17C`). **As tabelas de pista são geradas em execução** (no EXE estão zeradas) — o port **desenhou pistas próprias** (`curve_tab/hill_tab`, 256 entradas repetidas).
- Desenho final: listas de coordenadas por linha + rotina assembly EGA/CGA com panning por CRTC.

### 2.9 As regras de aposta (`img 0x9BC5`, o coração social do jogo)
Função `aceita(tipo)` (retorna 1 = aceita). Variáveis:
- `tipo` (kind): 0 = só por diversão, 1 = $10, 2 = $50, 3 = $25, 4 = $100, 5 = pink slip. **Arrancada** oferece {kicks, $10, $50}; **estrada** oferece {$25, $100, pink slip}.
- `col` (coluna de T10): 0 kicks; 1 qualquer dinheiro; 2 pink slip. `row` (linha de T11): 0 kicks; 1 {$10,$50}; 2 {$25,$100}; 3 pink slip.
- `tier` = `piloto[0]` (0,1,2): pilotos 0-3 → 0; 4-10 → 1; 11-21 → 2.
- `A = T10[tier*3 + col]`.
- `classe(carro)`: `perf < 3 → 0; perf ≥ 7 → 2; senão 1`.
- `B = T9[classe(carro_do_oponente)*3 + motor_do_oponente]` e `C = T9[classe(seu_carro)*3 + motor_seu]` (T9: **menor = mais forte**).
- `rel = 0 se A+B < C; 2 se B > C; senão 1`.
- `p = T11[row*3 + rel]`.
- `cool = adesivo==0 ? 0 : (adesivo ímpar ? 4 : 6)` (bits 8-12 de `carro+0x24`: o **adesivo**, não a pintura).
- `net = 2×piloto[0xd] − piloto[0xc]` (ele ganhou menos você ganhou).
- `chance = p + trunc(net×p/4) + cool`; se `[0x610] != 0` (flag global não identificada) `chance += chance/4 + 10`.
- **Aceita se `rand(100) <= chance`**.
Tabelas: `T9 = {210,180,150, 175,150,125, 140,120,100}`; `T10 = {60,45,30, 55,40,20, 50,30,10}`; `T11 = {40,60,90, 70,65,25, 90,70,15, 90,35,0}`.
**Fluxo de desafio (`img 0x9D6D`):** (1) se combustível < 7 → "You're gonna hafta get more gas" (unidades: tanque 140; `+0x22` do carro); (2) menu tipo: arrancada ou estrada (ESC/tempo → "Chicken!"); estrada exige combustível ≥ 17; (3) até **2 tentativas** de aposta: sem dinheiro → "You don't have enough dough…" e conta tentativa; recusa → frase aleatória sem repetir a anterior (tabela por tipo); aceite → frase aleatória (3 por tipo); (4) acabaram as tentativas → o piloto vai embora.
**Pós-corrida (`img 0xAEF3`):** se foi vitória e não era "só por diversão": `wins[tipo]++` (arrancada/estrada); total `DS:0x4970++` (qualquer corrida valendo); `piloto[0xc]++` (todas valendo); se perdeu `piloto[0xd]++`.
**O gate do Rei (`img 0x9556`)**, `G ∈ {0..5}`: `total = vitórias_arrancada + vitórias_estrada`; `total < 3 → 1`; `total < 7 e ambas < 6 → 2`; `total ≥ 7 e ambas < 6 → 3`; `arrancada < 6 → 4`; `estrada < 6 → 5`; ambas ≥ 6: `total < total_corridas/2 → 3`, senão `0` (**liberado**). Também `0` se **todos os 21 pilotos têm `[0xb] = 0xFF`**. Mensagens: `G=2` "I don't got time for amateurs like you!", `3` "You're doing OK kid, but you're gonna need to win more races!", `4` "…win more drag races!", `5` "…win more road races!". O Rei exige combustível ≥ 22. Primeira corrida com o Rei = arrancada por diversão (`DS:0x4976 = 0`); vencendo, `0x4976 = 1` e a segunda é **estrada valendo o carro** (vencer = fim do jogo). Perder zera: `0x4976 = 0`, `arrancadas −3`, `estradas −2`.
**Perda do carro no pink slip (`img 0xC83C`):** se `piloto[0x10] == 0` e não é o Rei: o piloto ganha um carro melhor (um da lista com desempenho > o que você ganhou) e `[0x10] = 1`; senão `[0xb] = 0xFF` (**vai embora de vez**).

### 2.10 Combustível, multa e quebra (unidades originais)
- `carro+0x22` = combustível em unidades: **inicial 40, máximo 140**; cruzeiro gasta 2; corrida gasta **5 (arrancada) ou 15 (estrada)**; "FILL IT UP" soma 30 por vez; teste "full tank" a 140.
- Estas unidades **não** foram copiadas 1:1: o port usa galões (tanque 20, 7 un. = 1 galão). Em um jogo "rigorosamente igual" **use as unidades originais**.

### 2.11 Som do original
Só **alto-falante do PC**: PIT canal 2 (`out 0x42`) e `port 0x61` — rotinas de **ronco do motor** (divisor `0x7530 + |v|<<10`) e bipes. **Não há música** no `SRSE.EXE` (nem AdLib nem Tandy; a única referência a `0xC0` é incidental). A música do port é uma **transcrição** de uma gravação fornecida (ver §10). Para um jogo futuro, a música é trabalho **de criação**, não de extração.

### 2.12 Metodologia de busca no código (reutilizável)
1. **Localize dados por valores conhecidos** (preços, nomes) → obtém o `DS` e o layout de blocos.
2. **Ache a rotina de carga** (`hot_data` em `DS:0x4E0A`, tabela de tamanhos em `0x4EBE`) → endereços de cada bloco.
3. **Busque por acessos indexados** (`[bx+si+offset_do_bloco]`) em vez de imediatos: as tabelas são lidas com `shl bx,1; imul` e deslocamentos negativos (`[bx+si-0x7452]` = `0x8BAE`).
4. **Siga o fluxo por mensagens**: IDs de mensagem identificam o ramo (ex.: ID de "amadores" = gate G==2).
5. **Reconstrua a função em pseudocódigo** e **teste em Python** contra o comportamento esperado do manual (ex.: "o rei só corre depois de vitórias").
6. **Cuidado com falsos positivos** do desassembler linear (ver §2.2).

---

## 3. Plataforma-alvo: MSX2, V9938 e as restrições que moldaram tudo

### 3.1 Por que MSX2 e Screen 5
| Critério | Original (EGA 320×200×16) | MSX2 Screen 5 (256×212×16) |
|---|---|---|
| Cores | 16 de 64 (paleta EGA) | 16 de **512** (paleta programável de 3 bits por canal) |
| Resolução | 320×200 | 256×212 (reduz 1 coluna em 5) |
| Páginas | — | 4 páginas de 32 KB na VRAM de 128 KB (com 212 linhas por página) |
| Aceleração | nenhuma | **comandos de VDP** (cópia, preenchimento, linha) que rodam em paralelo à CPU |

Escolhas (análise de projeto; as alternativas não foram prototipadas):
- **Screen 5**, 4 bits por pixel (2 pixels por byte, 128 bytes por linha, `0x8000` por página). Screen 8 (256 cores) deixaria só 2 páginas na VRAM de 128 KB e dobraria a área a pintar por quadro; Screen 7 (512 px) dobra os bytes por quadro. Para o original (16 cores EGA) o Screen 5 é o encaixe natural.
- **Paleta fixa por contexto**, em vez de dinâmica por quadro: a paleta EGA do original é mapeada para uma paleta de 16 cores do MSX2 (`palette_def.py`), e trocas de paleta acontecem **junto com troca de página** (invisível ao jogador).
- A redução 320→256 **descarta 1 coluna em 5** (`x % 5 == 4`), o que **preserva o dither** das imagens EGA (reamostragem por média destruiria o padrão ordenado e criaria cores novas).

### 3.2 O V9938: o que importa
- **Comandos** (registradores R#32..R#46): `HMMV` (preenche bytes), `HMMM` (copia bytes, x e largura **pares**), `LMMM` (copia pixels, com operação lógica; `TIMP` = cor 0 transparente), `LMMV` (preenche pixels), `LINE`. Cada comando custa **~1,2 ms de overhead de emissão da CPU** (preparar 11 registradores) mais a **área × tempo por byte** no VDP (medidos: `HMMV` ≈ 2,76 µs/byte ≈ 360 KB/s; ≈ 160 µs de emissão quando encadeados com cuidado).
- **Consequência de projeto:** o quadro é limitado por *número de comandos × 1,2 ms* **e** por *área ÷ 360 KB/s*. A estrada precisa ser desenhada com **poucos retângulos grandes** (HMMV por faixa), não por pixel.
- **Escrita em pares de registradores** (R#17 de acesso indireto, leitura de status) exige `DI/EI` curtos: a ISR da BIOS lê o status 0.
- **Sprites:** modo 1 (8×8/16×16, 1 cor por linha, **2 planos sobrepostos = 2 cores**). O cursor é um sprite 16×16 de 2 planos (contorno + miolo) — ver §11. `R#8` bit 1 desliga sprites; bit 5 (TP) = 1 torna cor 0 transparente.
- **Display ligado/desligado (`R#1` bit 6):** com a tela apagada, o acesso à VRAM não tem espera (carregas muito mais rápidas) — por isso o projeto original apagava a tela em trocas de cena; depois substituímos por **carga em páginas ocultas** (§6).

### 3.3 Mapa de memória do Z80
- **0x0000-0x3FFF:** BIOS (ROM principal).
- **0x4000-0x7FFF (página 1):** **banco 0 do cartucho = núcleo** (sempre visível): `crt0`, `vdpasm`, rotinas de VDP, carregador de assets, ISR, UI comum, entrada, som.
- **0x8000-0xBFFF (página 2):** **janela de banco** (16 KB, selecionável por escrita em `0x7000`): bancos de código 1..5 ou bancos de assets (6+).
- **0xC000-0xFFFF (página 3):** RAM (dados e pilha). `HIMEM` normal = `0xF380`; sem disco a pilha inicia abaixo de `0xF300` (o gancho do BDOS em `0xF37D` não pode ser sobrescrito).
- O cartucho ocupa páginas 1 **e** 2 (`ENASLT` no `crt0`); a página 0 é BIOS.
- **Chamadas à BIOS** (`GTSTCK 0x00D5`, `GTTRIG 0x00D8`, `GTPAD 0x00DB`, `SNSMAT 0x0141`, `CHSNS 0x009C`, `CHGET 0x009F`, `RSLREG 0x0138`, `RDSLT/ENASLT`) **destroem IX/IY**: toda rotina que as chama a partir de C precisa preservar IX/IY (SDCC os usa como ponteiro de frame).

### 3.4 Tempo
- `JIFFY` (`0xFC9E`) incrementa a 60 Hz (NTSC) ou 50 Hz (PAL): o jogo detecta via bit 1 de `R#9` (espelho em `0xFFE8`) e escalona `tick_inc`, `adv_k`, `go_j` por `jhz`.
- A física do original (72,826 Hz) é reamostrada para **6,0688 Hz de passos de física** (72,826/12), acumulando ticks fracionários em ponto fixo 8.8 (`tickacc`, `tick_inc`).

---

## 4. Cadeia de ferramentas, ROM, mapper e organização em bancos

### 4.1 Ferramentas
- **SDCC** (`sdcc -mz80`, `sdasz80`, `sdld`) para C e assembly; **Python 3** (numpy, Pillow) para geração de assets; **openMSX** (+ `xvfb-run` para rodar sem tela) e **C-BIOS** para teste; `ffmpeg/librosa` apenas para transcrever a música.
- Build completo: `./fullbuild.sh` (define `LIB2`, `HOT_DATA`, `SRSE_EXE` e chama `build.sh`). **Sempre confira** `grep -c "DEFAULT CAR" textdata.bin` = 0 antes de distribuir uma ROM: sem `HOT_DATA`/`SRSE_EXE` o gerador cai em nomes genéricos.

### 4.2 `build.sh` passo a passo
1. `gen_tables.py` → `cars.h`, `tables.h` (marchas, curvas de torque, linha vermelha por carro e habilidade do piloto), `textdata.bin` (nomes/descrições).
2. `gen_assets.py` → `assets.bin`, `assets.inc` (tabela lida pelo carregador em assembly), `assets.h`, `assets_race.h`, `assets_scn.h` (tabelas e posições).
3. `sdasz80` monta `crt0.s`, `vdpasm.s`, `proj.s`.
4. `sdcc -S` compila `main.c` para assembly.
5. **`bankify.py`** reescreve o assembly distribuindo funções e tabelas em áreas `BANKn` conforme os `#pragma codeseg`; **falha o build** se alguma referência cruzar bancos sem ser `__banked`.
6. `sdasz80` + `sdld` com `-Wl-bBANKn=0xn8000` (endereço virtual `n<<16 | 0x8000`).
7. **`ihx2rom.py`** posiciona cada banco no arquivo (`n×0x4000`), concatena `assets.bin` a partir do banco `ASSET_BANK0` (6) e preenche até potência de 2 (512 KB).

### 4.3 Mapper ASCII16
- **Dois registradores:** escrever em `0x6000` seleciona o banco da janela `0x4000-0x7FFF`; em `0x7000`, o da janela `0x8000-0xBFFF`. O jogo mantém o **núcleo fixo no banco 0 da janela 1** e troca só a janela 2.
- **Detecção pelo emulador:** openMSX e WebMSX adivinham o mapper contando instruções `LD (nn),A` para `0x6000/0x7000/0x77FF`. O `crt0` escreve 0 em `0x6000` e 1 em `0x7000` no início. Por isso (a) **nenhum outro código** pode conter o padrão `32 00 50`-like para outros mappers (a tabela de preços das peças guarda `preço+1` por isso) e (b) o `ihx2rom.py` **falha** se detectar o padrão. O código usa `ld (hl),a` para gravar nesses registradores.
- **SRAM no cartucho** não é possível: no ASCII16+SRAM o bit 4 do registrador liga a SRAM; a ROM usa bancos 16-26.

### 4.4 Organização final em bancos
| Banco | Conteúdo |
|---|---|
| 0 (núcleo, ≤ 16.352 B; final **16.214 B**) | `crt0`, VDP, UI (`ui_list`, `pick_hot`, textos), entrada, mouse, cursor, som base, `ptmodel.h`, carregador |
| 1 (**15.242 B**) | corrida (física, projeção, sprites, HUD, `run_race`), rotinas asm da estrada, tabelas da pista, `font_init`, `cursor_init`, `fmt_price` |
| 2 (**15.365 B**) | jogo: título, garagem, jornal, Bob's (tela), posto, calendário, nome, hub |
| 3 (**8.810 B**) | Bob's (lógica de aposta), fim de corrida/jogo, oficina, créditos, **player de música** |
| 4 (**13.324 B**) | janelas do motor, parafusos, PARTS, TUNING, menu de carros |
| 5 (**7.162 B**) | cenário da corrida (`draw_scn`, `rearview`, pórtico) |
| 6+ (assets) | 23 bancos de 16 KB com imagens e bancos de música (um por estação) |

### 4.5 Regras de código para bancos (as que importam)
1. `main.c` é **uma só unidade de compilação**; `#pragma codeseg CODE|BANKn` marca regiões (e vale também em cabeçalhos).
2. Função chamada de outro banco → `__banked` (macro `BK`); **BK só recebe argumentos escalares** (ponteiros para dados de outro banco ficariam invisíveis); o retorno também.
3. **Constantes pertencem à região onde foram definidas** (literais de string vão para o banco das funções que as usam — `bankify.py` resolve).
4. `bankify.py` lê as definições **na ordem do arquivo**: uma função não pode aparecer duas vezes em `#ifdef` com bancos diferentes (usar macro).
5. Dados usados por mais de um banco → núcleo ou copiar via `bank_copy` (rotina fixa na página 1).
6. O **núcleo estoura primeiro**: tudo que não for chamado de vários bancos deve sair do núcleo. Em todo marco grande o núcleo ficou entre 15,4 e 16,2 KB — reserve folga desde o começo.

---

## 5. Pipeline de assets

### 5.1 Princípio
Nenhum asset é desenhado à mão: `gen_assets.py` lê a `LIB2`/`LIB1` **da cópia do usuário** e produz tudo. Sem `LIB2` o jogo roda com gráficos simples. **Para um jogo novo (assets próprios)**: substitua `srlib.py` e as chamadas `garage_pixels(lib2, item)` por leitores dos **seus** PNGs/paletas; todo o resto do pipeline (reamostragem, paleta, compressão, bancos) é reaproveitável.

### 5.2 Etapas por imagem
1. **Decodificar** (EGA 4 planos → matriz de índices 0..15).
2. **Reamostrar** 320→256 (descarte de 1 coluna em cada 5; funções `garage_pixels`, `garage_pixels_thin`, `garage_full`).
3. **Mapear cores**: `MAP[ega] → índice MSX` por menor distância RGB (`nearest`), com paleta `palette_def.PAL` (índices **fixos** com papel: 0 preto/transparente, 6 vermelho (pintura/ponteiros), 7 branco, 8 amarelo, 10 vermelho escuro, 11 cinza escuro, 12 pintura do oponente, 13 verde, 15 cinza claro; 1 e 2 reutilizados para o capô).
4. **Empacotar** 2 pixels/byte (`a[:,0::2]<<4 | a[:,1::2]`).
5. **Comprimir** em faixas de 4 KB com um LZ estilo LZ4 (`lz_encode`; decodificador Z80 de ~60 bytes em `vdpasm.s`); **uma faixa nunca cruza dois bancos**.
6. **Registrar** `(banco, grupo, offset, vaddr, nblocos, cópias)` em `assets.inc`.

### 5.3 Grupos e cópias
- **Grupo** = conjunto de faixas carregadas juntas (`g_group`). Cópias: `1` = só uma página, `3` = páginas 0, 1 e 2 (cena visível + espelho + cópia limpa).
- **Mapa de grupos:** 0 cockpit/corrida (páginas 0-2 + sprites na 3); 1 garagem; 2+id e `100+3·id+v−1` corpos de carro (variantes); `28+tipo` rodas; `180+n` adesivos; 190 jornal; 197 Bob's; `220+p` retratos; 242 calendário; 243 posto; 244 carteira DMV; 250/251 telas de título; 252 ranking; 253 vitória; 254 créditos; 60-62 painéis de câmbio; 63-65 botões de câmbio; 66-68 folhas de peças; 69 TUNING.
- `g_hide0` (flag do carregador): carrega **só as páginas 1 e 2**, para a tela continuar mostrando a cena antiga (§6.3).

### 5.4 Carros (`cars_art.py`)
- Cada carro = corpo lateral (item `BODY[i]`) + rodas (3 tipos) + adesivo + cabeça do piloto. Índice 0 = **transparente**; 6 = pintura; 7 = reflexo; 1 = preto/contorno (na arte do carro). O `set_paint` troca só as cores 6 e 7 da paleta — **12 pinturas sem recarregar nada**.
- **4 variantes por carro**: teto rebaixado (remove linhas dos montantes entre capota e cintura — `chop`) × sem para-choques (apaga cinzas/branco nas pontas — `strip`). Total 26×4 corpos.
- **Rodas e arcadas:** o centro x de cada roda sai do **contorno inferior do corpo** (`arch_centers`: colunas onde a última linha opaca sobe >7 px acima da soleira; centro = média das colunas até 4 px abaixo do ápice) e o y do **ápice da arcada dianteira + 13**. Carros com saia no traseiro (rodas escondidas) mantêm o entre-eixos antigo a partir da roda da frente. **Método anterior** (detecção grosseira + `_refine` + `_arch` + deslocamentos fixos de ±4 px) deixava rodas fora das arcadas em vários carros.
- **Motorista:** `driver_window` encontra os vidros (transparente cercado pelo corpo, flood-fill a partir da borda) e escolhe a maior janela cujo centro está entre 30% e 80% do entre-eixos; fallback = meio do entre-eixos + `top_row+10`. Emitido em `car_hx/car_hy`. O retrato é desenhado **por baixo** do corpo; só aparece pela janela.
- **Animação** (`car_slide`): o carro pronto é montado uma vez numa faixa da página 2 e cada quadro faz HMMM da faixa limpa + HMMM de ≤3 retângulos opacos + LMMM só das bordas (`best_rects` calcula as faixas de cada corpo: 40 bytes por variante/carro). Medido: ~1,5 s de entrada (antes 2,6 s), 2-4 jiffies por quadro.

### 5.5 Cenários
- Garagem: item 32 com paleta EGA própria (`pal_garage`).
- Bob's: item 93 com **os dois carros pintados originais apagados** na geração (cópia de faixas limpas) e depois **restaurados como cenário** (só o oponente entra/sai).
- Jornal: item 21 + manchete; o microtexto ilegível das colunas é apagado (`NEWS_BODY_TOP`) e o jogo escreve texto curto legível com a fonte da garagem (`news_body[]`).
- Créditos: o texto miúdo do original vira lixo em 256 colunas → apagado e refeito com a fonte do jogo.
- Cockpit: ver §8.6 (janela esquerda, vidro, capô).

### 5.6 Fonte
- Menus: fonte 6×8 da **BIOS** (`CGTBL`, ponteiro em `0x0004`): `font_init` copia 96 glifos para a página 3 em **preto e branco** e `gtext` desenha por LMMM (TIMP).
- Corrida/título: fonte 3×5 em 2×2 (pixels próprios; também gera "CITY LIMITS").

---

## 6. Mapa de VRAM e técnicas de apresentação

### 6.1 Mapa de VRAM (Screen 5: 128 bytes por linha; página *n* começa na linha `256·n`)
**Menus / garagem / Bob's / posto / jornal**
| Região | Linhas | Uso |
|---|---|---|
| Página 0 (`0x00000`) | 0-211 | **tela visível** |
| Página 0, linhas 232-255 | `0x7400` cores, `0x7600` atributos, `0x7800` padrões | **sprites do cursor** (não visíveis: Screen 5 mostra 212 linhas) |
| Página 1 (`0x08000`) | 256-467 | espelho da cena (para a animação do carro) **e folhas de arte** (grupos 66-69: motores/peças/TUNING a partir de 256) |
| Página 2 (`0x10000`) | 512-723 | **cópia limpa da cena** (`hub_restore` copia daqui para apagar janelas) |
| Página 3 (`0x18000`), 768 | 768-851 | **faixa limpa do chão** (`BAND_COPY_Y`, 84 linhas) |
| Página 3, 848 | 848-974 | **buffer das janelas** (`WBUF_Y`, usa a área do rascunho do carro; o `draw_car` recarrega o carro depois) |
| Página 3, 852 / 920 / 948 | rascunhos | corpo do carro (`CAR_Y0`) / roda (`WHEEL_Y0`) / adesivo ou retrato (`STK_Y0`) |
| Página 3, 976 | — | fonte da garagem em preto e branco (`font_init`) |
| Página 3, 972 | — | painéis e botões do câmbio (`GSRC_Y`) |

**Corrida**
| Região | Uso |
|---|---|
| Páginas 0 e 1 | **double-buffer** (a visível alterna; `R#2`) |
| Página 2 | cockpit limpo (restaura mostradores e vidro) + folhas de placas B/C (704-767) |
| Página 3 | folha de sprites do oponente (`SPR_Y0`=808), colinas+grama do horizonte (`BG_Y`=768, `BG_H`=15), painel do câmbio (972) |
| Páginas 0/1, linhas 212-255 / 468-511 | árvores e placas (folhas de cenário) |

**Conflito conhecido:** os dados do cenário da corrida sobrescrevem a área do sprite do cursor (linhas 232+ da página 0). Por isso `race_go` chama `cursor_init()` ao voltar (bug "mouse sumiu depois do Bob's").

### 6.2 Troca de página e sem cintilação
- A página visível **nunca é apagada**: desenha-se na outra e troca-se `R#2` no vblank (esperando `JIFFY` mudar).
- `vdp_wait` (poll do bit CE do status 2) separa comandos dependentes.
- Barra de status **nas duas páginas** (senão piscava a cada quadro da animação).

### 6.3 Troca de cena sem tela preta (`scene_load` + `scene_present`)
Antes: `R#1` bit 6 = 0 (tela apagada) → carregar → ligar (≈0,5–1,7 s de preto). Depois:
1. `scene_load(grupo)`: `R#2 = 0x1F` (mostra página 0 = cena antiga); `g_hide0 = 1`; `load_assets()` grava só nas páginas 1 e 2.
2. Desenha extras (texto do jornal etc.) na página 2.
3. `scene_present()`: HMMM página 2→1 (1 comando de 27 KB); espera o vblank; **carrega paleta (`pal_load2`) e troca `R#2` para a página 1** no mesmo quadro; HMMM página 1→0; volta `R#2` para a página 0.
- **Corrida** (`init_static`): idem, com `pal_load()` + `race_pal()` (cores 12, 1 e 2) no instante do flip, e `hud_reset/gear_init` **depois** do flip (escrevem nas páginas 0-2).
- Resultado medido: o bit de display do `R#1` **nunca** cai após a tela de título.

### 6.4 Janelas sem piscar (`win_frame` / `win_flush`)
As janelas do motor e de PARTS são **montadas fora da tela** (página 3, linha 848): `ybase` aponta para o buffer, tudo (`push_fill`, `gtext`, `art_put`) soma `ybase` ao y; `win_flush` copia o retângulo da janela com **1 HMMM**. `win_new` pede um apagamento da região da tela só quando outra janela (lista, TUNING) esteve por cima. **Lição:** isso deveria ser o padrão para *toda* janela; no jogo final, TUNING e as listas ainda redesenham direto.

### 6.5 Fila de comandos e DRY
`cmd_buf`/`cmd_n` guardam comandos de VDP enfileirados; `fill_g` emite HMMV imediatamente (esperando o anterior), com recorte horizontal `clip_l/clip_r`. `-DDRY` pula emissão (mede só CPU).

---

## 7. Arquitetura do código

### 7.1 Estado e módulos (`main.c`)
Regiões `#pragma codeseg` (ver §4.4). Principais estados globais: `money`, `day`, `nown`, `cur`, `gc[4]` (carros do jogador), `spare[12]`, `wins_drag`, `wins_road`, `races_tot`, `king_st`, `pil_net[22]`/`pil_st[22]`, `game_end`, `mus_st`.
- `Car` (16 bytes): `id, eng, trn, man, carb, tir, trim, wear(u16), fuel, brk, tp, paint, roof, bump, stick`.
- `main()` → `title_screen()` → loop: `new_game()` → `hub()` (garagem) → ações → `race_go()` → `settle_race()` → `race_result()` → fim de jogo → `victory_and_ranking()`.
- **Estados de depuração** (`dbg_state` em `0xF014`): 0 largada, 1 corrida, 2 resultado, 7 menu, 9 título — testes do openMSX leem a RAM para sincronizar.

### 7.2 Sistema de UI
- `ui_list(titulo, n, sel)`: lista rolável (8 linhas visíveis; botões X, ▲▼); seleção por teclado ou **mouse/cursor** (passar seleciona, clique escolhe); `ui_pick` (array de strings) e `ui_note` (aviso de 2 linhas).
- `pick_hot(allow_back)`: seletor genérico de **objetos clicáveis** (`Hot{x0,y0,x1,y1,id}`); `hot_at` decide o objeto sob o cursor (carro = YOUR CARS; terço dianteiro do carro = capô); `hot_label` escreve o nome na barra superior; `pick_tick` roda a cada quadro (animações, teclado da tela).
- Cursor: **chave de boca** de 16×16 em 2 planos (contorno cor 10 = `cur_wa`, miolo cor 11 = `cur_wb`), ponta no ponto de clique (`g_sx = cur_x−3`, `g_sy = cur_y−1`). A arte é desenhada pelo EXE (não existe na LIB).

### 7.3 Fluxos principais
- **Hub:** `hub_draw` (carrega garagem, cursor, barra, carro) → `scene_pick` (objetos: carros, jornal, calendário, rádio, posto, rua, porta, compressor, capô) → ação.
- **Rua → Bob's:** `street_trip()` (corrida em `street_mode`) → `bobs_front()` (cena do Bob's, `bobs_tick` anima chegadas) → clique na cabeça → CHALLENGE → `bobs_race(p)` → `race_go()`.
- **Fim de corrida:** `settle_race()` (desgaste, quebras, dinheiro, carros, vitórias, rivalidade, Rei) → `race_result()` (janela) → volta ao hub (a mensagem final `pmb` é copiada porque quem escreve (banco 3) não é quem mostra (banco 2)).

---

## 8. O motor da corrida

### 8.1 Pipeline por quadro
1. `read_input` (BIOS: GTSTCK 0/1, GTTRIG 0/1/3, SNSMAT 6/7) + `combine_input` (mescla joystick/teclado/ISR, **toca música**).
2. `handle_input` (acelerador, freio, direção, trocas) e `snd_engine`.
3. **Física** a 6,0688 passos/s equivalentes (`tickacc`, `veh_tick` por jogador/oponente): rpm, marcha, velocidade, `advance` (avanço em subunidades), `steer_frame` (lateral).
4. **Projeção** (`proj.s`, `project_asm`): ~15 linhas, multiplicações 16×8/16×16 em assembly (SDCC faz `long` em ~4,9 mil ciclos).
5. **Desenho:** fundo (céu + colinas por HMMM), grama em **um retângulo**, estrada por faixas com `strip_run` (asfalto preto, borda amarela, tracejado branco pela paridade do segmento `s_dash`), cenário (`draw_scn`, BANK5), retrovisores (`rearview`), mostradores (HMMM do fundo limpo + `LINE` do ponteiro), botão do câmbio, troca de página.

### 8.2 Pistas
- Arrancada: reta, 380 segmentos (`RACE_LEN`). Estrada: 520 (`ROAD_LEN`) com `curve_tab/hill_tab` de 256 entradas (pista sintética; o original gera em execução — não replicado). Rua: 420 (`STREET_LEN`).
- 8 subunidades por segmento; `pos`, `sub`; oponente `opos`, `osub`.

### 8.3 Cenário (`draw_scn`)
- `scn_tab[256]` (gerada em `gen_assets.py`) por segmento: árvores, prédios, placas (258 COUNTY LINE, 236 DRIVE-IN, 238 GUS' GAS em 7 tamanhos), postes brancos. Tamanho/posição pela meia largura da pista na linha do chão: `hw = 2·(y−HZ) + ((y−HZ)>>5)` (a forma original `(y−HZ)·2074` estourava 16 bits).
- Placas e prédios em **HMMM** (opacos); árvores em **LMMM** com transparência.
- **SDCC hoisting:** a primeira versão de `draw_scn` custava ~18 ms por culpa de "içar" cálculos; reestruturada em funções por objeto (`scn_side/scn_blit/scn_pole`).

### 8.4 Retrovisores
- `rearview(x0,w,wy,h)` desenha, dentro do espelho (superior: x194-254, y22-42; lateral esquerdo: x8-31, y103-123), céu, grama, pista em trapézio, tracejado e os objetos dos 4 segmentos que ficaram para trás (`scn_tab[pos−1−j]`).
- **Oponente:** quando você passa (`pos > opos`), um carro em 5 retângulos (corpo laranja = cor 12, janela, para-choque, lanternas) com tamanho `cw = w·40/(100+4·d8)+1` e posição lateral pela diferença de pista; visível até 14 segmentos de distância (`d8 ≤ 112`).

### 8.5 Rua (viagem automática)
`street_mode`: sem oponente nem semáforo; o jogo inicia andando; **piloto automático** (`handle_input`/`veh_tick`/`steer_frame`): ~40 mph, troca de marcha sozinha, segue a pista. Pórtico **CITY LIMITS** no segmento 8 com faixa amarela transversal. Radares nos segmentos 110/230/350: acima de 45 mph multa `$5 + (mph−45)/4`. Seletor pula a viagem; ESC desiste.

### 8.6 Cockpit, vidro, janela esquerda, capô
- O cockpit (item 14) é copiado para as páginas 0, 1 e 2. A **estrada só é desenhada no vidro**; a linha de capô ("linhas ciano") define o corte (`overlay()`: `cuts`/`sm`, faixas de largura variável com tolerância `STRIP_TOL`); o resto é recolocado por HMMM (retângulos `ov_*`).
- **Janela esquerda:** o vidro da janela lateral vira cenário já na imagem do cockpit (`left_glass` + `bg_image`: céu, colinas, grama); só o interior do retrovisor (x7-33, y101-125) fica opaco. Custo: `OV_LEFT=0` (nenhum retângulo por quadro).
- **Capô colorido** (`HOOD`): o preto entre a linha ciano e o painel recebe o índice 2 da paleta; a linha ciano (índice 1) vira o "reflexo". `set_hood(paint)` ajusta as duas entradas da paleta por corrida. O limite inferior (`yb`) é a mediana de 9 colunas do primeiro pixel claro (7/15), no máximo 14 linhas abaixo da linha — **sem serrilhado**.

### 8.7 Desempenho (openMSX, C-BIOS MSX2, 60 Hz, piloto automático)
| Fase | ms/quadro |
|---|---|
| Entrada + física | 10 |
| Projeção | 14 |
| Fundo + grama | 10 |
| **Estrada (VDP)** | **61** |
| Cenário | 19 |
| Moldura/cockpit | 24 |
| HUD | 7 |
**Total ≈ 145 ms → 6,9 fps** (original ≤ 8,1). Evolução: 3,4 fps (C puro) → 7,1 fps (assembly + faixas) → 8,0 fps (sem cenário) → 6,9 fps (com cenário completo).
Parâmetros de build: `DRAW_MODE` (0 faixa+zebra, 2 pixel único, 4 só pista com fundo estático — **padrão**), `STEP_SH`, `VIEW_BOT`, `VIEW_X0/X1`, `DRY`, `-DPROF` (somas em `prof[]`), `-DNO_SCN/-DNO_BG/-DSCN_SKIP`.

---

## 9. Sistemas de jogo

> **Legenda de fidelidade:** 🟢 = regra **decodificada do original**; 🟡 = **inferida do manual/mensagens** (comportamento igual, números prováveis); 🔴 = **regra minha** (inventada para o port). **Para o jogo futuro "rigorosamente igual", todo 🟡 e 🔴 é dívida de engenharia reversa** (ver §18.3).

### 9.1 Economia e progressão
| Regra | Valor no port | Fidelidade |
|---|---|---|
| Dinheiro inicial / carros | $1000, nenhum carro | 🟡 (manual: começar comprando no jornal) |
| Preços de carros e peças | tabelas do `HOT_DATA` / catálogo | 🟢 |
| Ofertas do jornal | 14 carros/dia, preço 80–120% do de tabela, sorteio determinístico por dia, **sempre** uma pechincha entre os 5 mais baratos | 🔴 |
| Barganha na compra | 10/20/30% a menos: aceita com 80/50/15% | 🔴 |
| Venda de carro | oferta de 55% do valor; pedir +20%/+40% aceita com 55%/20% | 🔴 |
| Venda de peça | 50% do preço | 🟡 |
| Limite de carros / peças | 4 / 12 | 🟡 |
| Duração | 1 dia por corrida; fim em **dia 46 = 15/set** ("YOU'RE DONE FER THE SUMMER") | 🟢 (o 15/set está circulado na arte do calendário) |
| Fim por falta de dinheiro | sem carro, sem peças e < $300 | 🔴 |
| Fim com vitória | vencer o Rei na estrada valendo o carro | 🟢 |

### 9.2 Garagem
- **YOUR CARS** → carro → *DRIVE THIS CAR / CAR PARTS / TUNE TIMING / CAR INFO / FIX OR JUNK IT / GAS UP / SELL IT / BACK*.
- **Compressor (PAINT AND BODY):** pintura **$20** (12 cores: 8 do jogo + preto, branco, rosa, marrom), adesivo **$5** (8 itens da LIB2 + "NO STICKER"), **CHOP ROOF / REBUILD ROOF $70**, **STRIP / RESTORE BUMPERS $15** — **preços 🟢 (strings do original)**, cores/adesivos disponíveis 🔴/🟡. Uma compra por visita; o carro é redesenhado ao sair.
- **Calendário:** página AGO/SET (item 221) com a data marcada por moldura verde (agosto/1963 começa numa quinta; setembro num domingo).
- **Rádio:** abre `CATCH SOME TUNES` (7 itens).
- **Porta (TIME TO QUIT):** SAVE GAME / LOAD GAME / HOW TO DRIVE / CREDITS / QUIT TO TITLE.
- **Posto (WANNA TANK?):** o carro entra pela esquerda e para ao lado da bomba; clique na bomba → *FILL IT UP $x / 5 GALLONS $y / NO THANKS*; **$1 por 3 galões** (🔴), tanque de 20 galões (🔴; original: 140 unidades).

### 9.3 O motor (clique no capô) 🟢
- **Janela do motor:** motor, coletor, carburadores nas **posições medidas no jogo original** (casando cada item da LIB2 com uma captura nativa 320×200 pixel a pixel: motor 78 em (112,77), coletor 104 em (152,69), carburador 86 em (167,60)/(211,60), parafusos 87/89, fiação 95 em (67,50)). Regras: o coletor começa na 1ª coluna do topo do motor (`eng_lf`) com a base na borda frontal (`eng_r`); cada carburador fica sobre uma almofada do coletor (base 9 linhas abaixo do topo do coletor) com **2 parafusos** (`+9` e `+w−13`, y`+6`); o coletor tem 1 parafuso em cada ponta (`+10`, `w−10`, y`+3`); o motor tem 1 parafuso no "rosto" de baixo + a fiação. Conversão de coluna nativa para arte: `AX(d) = d − (d+1)/5`.
- **Parafusos (manual oficial):** estados `0 apertado, 1 meio solto, 2 meio apertado, 3 erguido sobre o furo, 4 furo vazio`; **2 cliques soltam** (0→1→4), clicar no furo devolve o parafuso (4→3), **2 cliques apertam** (3→2→0); fiação 1 clique. Com todos fora a peça some e vai para as reservas; a peça nova chega com parafusos erguidos.
- **Ordem obrigatória:** carburadores → coletor → motor (e o inverso para montar). Só dá para afinar com tudo apertado. Sair com peça/parafuso solto deixa o carro "NOT RUNNING" (`bay_bad`) e **impede salvar**.
- **PARTS:** 5 categorias; clicar numa abre a lista de peças reservas compatíveis com a **marca** do carro (máscara 1/2/4/7); nas categorias do motor a peça antiga precisa ter sido desparafusada.
- **TUNING:** tela inteira do item 71 (200×175 → 160×175); setas: cima = RETARD, baixo = ADVANCE (ponto −8..+8); a correia dentada (colunas 71-75, linhas 53-81, período de 4 linhas) é regerada em 4 fases e copiada por LMMM conforme o ponto; painéis `TIMING ±n` e `POWER n`.

### 9.4 Bob's Drive-In
- Chegada de carros (`bobs_tick`): um carro por vez, **piloto aleatório** (exceto o que acabou de sair e os que **foram embora**), carro da **classe de desempenho do piloto** (`pil·8/21 ± 1`; quem já perdeu um carro sobe 2 classes), cor aleatória, retrato do piloto na janela; entra deslizando (12 px/quadro), fica parado 8-12 s, sai se ninguém o desafia; 2-4 s depois chega outro.
- **Retratos:** 22 itens (194-199, 202-217), um por piloto (grupo `220+p`); o carro desenha o retrato **por baixo** do corpo.
- **CHALLENGE** → menu tipo (🟢) → apostas (🟢) → aceita/recusa (🟢, §2.9) → corrida.
- **Rei (🟢):** ver §2.9 (gate G, arrancada por diversão e depois estrada valendo o carro, penalidades, "todos os 21 fora").
- **Pink slip (🟢):** vencer transfere o carro do oponente (se não houver vaga: vende por metade do valor); perder: você perde **o carro com que correu** ("YOU LOST YOUR CAR! — PINK SLIP GONE"). Pink slip é sempre estrada.

### 9.5 Quebras e consequências (corrida) 🔴 (exceto pneus 🟢)
- Desgaste: motor `+100 + 30·ticks_acima_do_corte`, pneus `+6` (`+10` na estrada), gasolina `−3` galões.
- Quebras (a corrida vira derrota; a aposta se perde): motor `3% por tick acima do corte + desgaste/1000` (máx. 60%); câmbio `8% por troca com o motor passado` (só manual, máx. 70%); pneus pela tabela do jogo (5 níveis × 3 tipos).
- Destruição: perdendo a mais de 90 mph, **6%** de chance de perder o carro (sucata = 10% do preço): "LUCKY YOU WEREN'T KILLED!".
- Largada falsa (queimar o semáforo): "YA JUMPED THE LIGHT, SPEEDY!" — corrida cancelada, nada se perde.
- **FIX OR JUNK IT:** consertar custa o preço das peças quebradas; sucatear paga a sucata.

### 9.6 Resultado e ranking
- `race_result`: janela "YOU WIN!/YOU LOSE!" com tempo (décimos) e velocidade média; mensagens do original ("BIG DEAL.", "FIFTY SMACKERS.", "YOU JUST WON A HELLUVA CAR!").
- **Ranking "King Street Rodders"** (LIB1 item 4): 10 melhores **bankrolls finais**; o jogador entra com o nome da carteira DMV (linha amarela); persistência em PAC `+0x1B00` (155 B) ou `STREETRO.RNK`. Os 10 nomes iniciais (KING $2900 … ROOKIE $350) são **inventados** (🔴; o ranking original não foi extraído).

---

## 10. Áudio

### 10.1 Efeitos (PSG, escrita direta nas portas `0xA0/0xA1`)
- **Motor:** 2 tons (canais A/B) com frequência `rpm/15` Hz, volume maior com acelerador; **semáforo:** 3 bipes no canal C (2 graves e 1 agudo e longo). O mixer usa `0xB8` (bit 7 = porta B como **saída**, necessária à BIOS para ler joystick; bit 6 = 0 = porta A entrada).
- **Importante:** a leitura do joystick/mouse da BIOS usa o R#15 do PSG; qualquer código seu que escreva R#7 deve preservar os bits 6-7 (`0x80` ligado, `0x40` desligado).

### 10.2 Música
- **Formato de estação:** `N` linhas × 4 bytes `[A melodia, B baixo, C harmonia, bateria]`; notas: `0` = continua a nota anterior, `1` = silêncio, `≥12` = nota MIDI; bateria (bits do registrador de ritmo `0x0E` do OPLL): 16 bumbo, 8 caixa, 4 tom, 2 prato, 1 chimbal. Cada linha dura `row_ms` ms; loops sem emenda.
- **Player** (`mus_tick`, banco 3): roda de dentro de `combine_input()` (todo laço de tela chama), relógio `JIFFY`; lê 4 bytes por linha por `bank_copy`; envelope por canal (`mus_env`: volume inicial, piso, jiffies por degrau); chimbal/caixa = ruído curto/longo no canal C. Se uma carga atrasa o relógio >40 jiffies, recomeça o relógio.
- **PSG:** períodos de nota em tabelas (`mus_tab`, banco de assets, 128 entradas); **FM (YM2413):** `fm_detect()` procura a ROM `APRLOPLL` (MSX-MUSIC) ou `PAC2OPLL` (FM-PAC) em `0x4018` dos slots (via `RDSLT`; no FM-PAC liga as portas escrevendo 1 em `0x7FF6`), instrumentos por estação (`mus_ins`), bateria no modo de ritmo; números F calculados por `fmn_` (bloco/F do OPLL).
- **Estações:** (1) **STREET ROD THEME** (padrão), (2) ROCK 'N' ROLL, (3) SLOW BLUES, (4) SWING JAZZ, (5) DOO-WOP, (6) SURF ROCK, (7) RADIO OFF. As 5 estações são **composições originais** de `music_gen.py` (blues de 12 compassos, boogie etc.).
- **Corrida:** a música é **desligada** (`mus_pause`: silencia PSG e `fm_off`); volta ao terminar.

### 10.3 Como o TEMA foi transcrito de uma gravação (receita reutilizável)
Entrada: MP3 de 56,9 s ("Intro_Ingame1").
1. `ffmpeg` → mono 22,05 kHz; `librosa`: CQT com 36 bins/oitava (C2…B6), hop 128.
2. **Grade:** busca de passo (0,1436–0,1456 s) e fase maximizando a soma da força de onset → **0,14496 s por passo** (~103,5 bpm em semicolcheias), offset 0,092 s.
3. **Loop:** correlação cruzada do CQT entre os 27,7 s → **27,6956 s = 191,06 passos** (similaridade 0,988): a gravação é o **mesmo laço tocado duas vezes** (o primeiro passo com nota é o 7).
4. **Vozes por passo:** mediana do CQT na janela (descarta 30 ms iniciais), **máximo + remoção de harmônicos 2..6**, até 4 candidatos; filtra vazamento (±1 a ±3 semitons de uma nota mais forte) e energia < 30% da maior.
5. **Atribuição:** `A` = nota mais aguda ≥ C4 (a mais forte; segunda com ≥ 60% vai para C), `B` = nota mais grave ≤ G#3, `C` = o que sobrar (intermediárias 58–59 e acordes).
6. **Limpeza:** preencher buracos de 1 passo quando a mesma nota continua; codificar repetições como `0`.
7. **Média das duas repetições** reduz o ruído; **conferência:** render do ROM gravado no openMSX × original → similaridade de croma 0,77 e 87% da energia coberta.
**Limites:** sem percussão (a gravação não tem energia > 4 kHz relevante); timbre diferente; a resolução de 16avos perde ornamentos; **o ouvido deve revisar**. Para um jogo novo, a mesma cadeia serve para **trazer músicas próprias** — ou componha direto no formato de linhas.

---

## 11. Entrada: teclado, joystick, mouse

### 11.1 Leitura
`read_input` (BIOS): `GTSTCK(0)` setas, `GTSTCK(1)` joystick 1, `GTTRIG(0/1/3)` espaço/botão A/botão B, `SNSMAT(6/7)` SHIFT/ESC/STOP/BS. Uma ISR enganchada em `H.TIMI` (`0xFD9F`) (`_key_isr`) lê as linhas 6 e 8 do teclado a cada interrupção e **trava as bordas** (`ev_space`, `ev_shift`) para não perder toques curtos entre dois quadros.
- Controle: ↑ acelera, ↓ freia, ←/→ dirige, **ESPAÇO/A** sobe marcha, **SHIFT/B** desce marcha, ESC volta. Joystick e teclado são mesclados em `combine_input`.

### 11.2 O mouse (a parte que mais deu trabalho)
- **Leitura:** `GTPAD` da BIOS: `12` dispara a leitura, `13` = ΔX, `14` = ΔY (complemento de 2). O mouse está na **porta 1**, **a mesma do joystick**: os pinos de direção carregam o protocolo do mouse.
- **Bug 1 (cursor derivando para o canto inferior direito no MSX real):** `GTSTCK(1)` lia os nibbles do mouse como "direção". **Correção:** depois do primeiro movimento do mouse (`mouse_seen`), ignora-se `GTSTCK(1)`.
- **Bug 2 (deriva ao abrir qualquer janela):** `ui_list` rodava **sem sincronismo**, lendo o mouse milhares de vezes por segundo; o mouse real devolve lixo constante. **Correção:** 1 leitura por quadro (espera `JIFFY`), `DI` durante a leitura, e descarte de deslocamento idêntico por mais de 50 quadros.
- **Regra geral:** leia o mouse **uma vez por quadro**, logo após a interrupção, com interrupções desligadas durante a sequência `12→13→14`; nunca misture com `GTSTCK(1)`.
- **Testabilidade:** o openMSX não injeta movimento; `-DTEST_MOUSE` simula (setas = deslocamento de 4 px, ESPAÇO = botão).

### 11.3 Teclado na tela e nome
Carteira DMV: 26 letras + SPACE/DEL/DONE como *hotspots* (`khot[29]`) e teclado físico via `CHSNS/CHGET` (`key_get`); RETURN termina; BS apaga (não é "voltar" aqui). Máx. 12 letras; vazio vira SPEEDY.

---

## 12. Salvar, carregar e ranking

- **Meios:** **PAC/FM-PAC** (SRAM de 8 KB, testado no openMSX com `-ext pac`) ou **arquivo `STREETRO.SAV` em disco** (BDOS do Disk BASIC/MSX-DOS; testado só com um **BDOS falso** — `-DFAKE_DISK` — porque o ambiente não tinha ROM de disco real).
- **Detecção do PAC** (`sv_init`): para cada slot, uma rotina **copiada para a RAM** (`sr_exec`; a página 1 troca de slot) destranca (`0x4D/0x69` em `0x5FFE/0x5FFF`), testa se `0x4000` grava, tranca e testa de novo (se ainda grava, é RAM comum). Sem PAC usa disco se o gancho `0xF37D` existe.
- **BDOS** (`bd_exec`, rotina na RAM): `0x13` apagar, `0x16` criar, `0x0F` abrir, `0x1A` DTA, `0x15` gravar, `0x14` ler, `0x10` fechar; **DTA antes de ler/gravar**; registro de 128 bytes. Depois remapeia o cartucho nas páginas 1 e 2.
- **Formato (versão 3, 128 B):** `'S','R',3,tamanho`, estado (`nown, cur, day, nspare, wins_drag, wins_road, sel_car, money`), nome (13), peças reservas (12), `gc[]` (4×16), **22 bytes de rivalidade+estado dos pilotos** (6 bits de `pil_net` e 2 bits de `pil_st`), `races_tot`, `king_st`, checksum e complemento. Grava e **relê para comparar**.
- **Bugs reais já achados:** `u8 k += 128` nunca passava de 256 e o laço de 2 registros do ranking travava; IX/IY não preservados nas rotinas de RAM; pilha podia cair sobre o gancho do BDOS.
- **Ranking:** `RK_N = 155` bytes em PAC `+0x1B00` ou `STREETRO.RNK`.
- **Pendência crítica:** **teste em disco real e em PAC real**.

---

## 13. Testes, ferramentas de depuração e metodologia de verificação

### 13.1 openMSX como laboratório
- Rodar **sem tela**: `SDL_AUDIODRIVER=dummy xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart road.rom -script teste.tcl`.
- **Scripts Tcl** (`*_test.tcl`, ~70) dirigem o jogo por `keymatrixdown/up` e **lêem a RAM do jogo** (`peek 0xF014` estado, `0xF018` resultado, `0xF030` dinheiro, `0xF034` carros…) para sincronizar; tiram **capturas da VRAM** (`save_debuggable VRAM`) e a paleta (`debug read {VDP palette}`).
- **`vram2png.py`** converte VRAM + registrador 2 (página visível) em PNG (+ paleta); opções `garage` (paleta EGA) e `spr` (renderiza sprites). **O `screenshot` do openMSX sai preto em modo sem throttle** — por isso a conversão própria.
- **`record start -audioonly`** do openMSX grava o áudio (conferência da música).
- **`-DAUTOPILOT=1`** liga o piloto automático para corridas sem entrada.
- **Medição de fps:** `fps.tcl`/`bench.sh` (tempo emulado em 20 s), `-DPROF` + `prof2.tcl` (soma por fase).
- **Log do bit de display:** amostrar `debug read {VDP regs} 1` a cada 20 ms prova que **não há tela preta**.

### 13.2 Flags de build úteis (`-D...`)
`OLD_MENU` (menu de texto antigo para testes), `SKIP_STREET`, `NO_NAME`, `NO_PAC`, `START_CAR=n`, `START_MONEY`, `START_DAY`, `START_WINS`, `START_END`, `RACE_LEN`, `STREET_LEN`, `AUTOPILOT`, `TEST_ACCEPT` (oponente aceita sempre), `TEST_LOSE`, `TEST_BRK=n` (quebra forçada), `TEST_SPARES`, `FORCE_PARTS=0x252`, `BOLT_TEST` (roteiro de parafusos), `BAY_OPEN`, `BAY_CUR_X/Y`, `PARTS_CAT`, `TEST_TUNE`, `TEST_MOUSE`, `TEST_MIRROR`, `TEST_SAVE/LOAD/CYCLE`, `FAKE_DISK`, `FORCE_FM/NO_FM`, `NO_SCN/NO_BG/NO_HUD/NO_OV/NO_SPR`, `DRAW_MODE`, `DRY`, `PROF`, `DEBUG_HUD`.

### 13.3 Metodologia (o que funcionou)
1. **Comparar com o original pixel a pixel** quando possível (título, posições do motor).
2. **Validar dados por invariantes** (preços = descrições: 100%; modelo de desempenho × Python: 14.580 combinações).
3. **Teste numérico de heurísticas** de arte (alinhamento de rodas): compor os 26 carros numa folha e ver tudo de uma vez.
4. **Teste estatístico do oponente** (aceita/recusa) — implementado e **a revisar** com simulação (ver §18.3).
5. **Todo bug de hardware real vira regra** neste documento (mouse, tempo de VRAM, SRAM) — emulador não substitui o MSX.
6. **Regressão por captura**: cada marco tem script que reproduz as telas.

---

## 14. Cronologia completa: marcos, decisões, bugs e como foram resolvidos

### 14.1 Fase 0 — Reconhecimento e engenharia reversa
1. **Inventário** dos arquivos e dos formatos: `EXEPACK` desempacotado; formato das LIBs decifrado e **validado contra captura** (título).
2. **DS errado:** a primeira hipótese `DS = img + 0x3E830` gerou ponteiros inconsistentes; `0x3E960` é o valor correto (validado pelas strings e pelos preços).
3. **HOT_DATA:** 13 blocos; preços de 25 carros e 43 peças **batem 100%** com os textos.
4. **Modelo de desempenho** transcrito do x86 (`powertrain.py`, `physsim.py` simulador) e depois **portado para inteiros** (`ptmodel.h`).
5. **Física e estrada** (tempo de 72,8 Hz, limiar de avanço, projeção).

### 14.2 M0 — Protótipo da estrada (Screen 5)
- Primeira versão **em C puro: 3,4 fps**. Passos: multiplicações longas do SDCC (~4,9 mil ciclos) → rotinas assembly 16×8/16×16; faixas de grama em retângulos; pista só com fundo estático; janela reduzida; física a 6,0688 Hz com tick fracionário.
- **Lições:** IX/IY, BSS zerada no `crt0` (lixo desligou o VDP e mascarou medições), `jr` curto vs `jp` no assembler do SDCC, DI/EI para pares de registradores.
- **ROM 32 KB → 64 KB:** o código encheu; **mapper ASCII16** e carregador de assets na página 1.

### 14.3 M1–M6 — Jogo em modo texto
- **M1** hub (estado de jogo + menus de texto), **M2** jornal (25 carros usados + 43 peças), **M3** garagem (trocar carro, vender, instalar peças, afinar, info), **M4** Bob's (22 pilotos, tipos de corrida, apostas, pink slip), **M5** consequências (quebras, perda de carro, fim de jogo), **M6** rua (cruzeiro, radares, gasolina).
- **SDCC e `i32`:** uma expressão composta do desgaste com ternário gerou código errado; reescrita em passos simples. **Regra:** inteiros de 32 bits, só em passos explícitos.
- **Núcleo cheio:** introdução de **bancos de código** (`bankify.py`).

### 14.4 M7 — Acabamento com a arte e o comportamento do original (a fase longa)
Ordem aproximada e problemas notáveis:
1. **Garagem clicável** com cursor (sprite) e mouse; objetos como `Hot[]`.
2. **Carro desenhado** (itens 39…201), rodas, sombra elíptica, variantes e cores de pintura.
3. **Fonte da BIOS** (6×8) na garagem.
4. **Jornal** com arte original (carregador de assets com **contador de 8 bits** só lia 5 faixas de 261 → contador de 16 bits).
5. **Bob's** com retratos e animações; **cenário intacto** (não apagar os carros pintados de fundo).
6. **Calendário** (descobrir que o fim do verão é **15/set**, circulado na arte, e não 1º/out).
7. **Posto** com carro passando **entre** a bomba e a construção (retângulos restaurados por cima).
8. **Título, resultado e fim**; **créditos**; **ranking com pontuação real**; **nome do jogador** (carteira); **HOW TO DRIVE**.
9. **Rádio e música** (5 estações; depois o tema).
10. **Salvar/carregar** (PAC e disco; BDOS falso para teste) e **ranking persistente**.
11. **Câmbio com manopla original** (itens 227-232).
12. **Motor aberto** com peças, **parafusos** do manual, **TUNING** original, **cursor de chave de boca**.
13. **Cenário da corrida** (árvores, prédios, placas, postes, horizonte), **janela esquerda**, **retrovisores**, **pórtico CITY LIMITS** e **faixa amarela**.
14. **Ajustes de revisão do usuário:** janela lateral transparente; vista traseira; mais carros no jornal (6 → 14); ida ao Bob's automática; motor não explode sempre; oponente nos retrovisores.
15. **Correções de hardware real** (mouse), **sem tela preta**, **janelas sem piscar**, **regras do original** (aposta e Rei), **capô colorido**, **entre-eixos e motorista**.
16. **Tema original** como estação padrão.

### 14.5 Bugs e correções marcantes (por assunto)
| Assunto | Sintoma | Causa | Solução |
|---|---|---|---|
| Loader de assets | só 5 faixas lidas | `djnz` com contador de 8 bits | contador de 16 bits |
| Detecção de mapper | emulador não achava ASCII16 | padrão `LD (nn),A` espúrio | `ld (hl),a`; `ihx2rom.py` verifica; preços guardam `preço+1` |
| Save em disco | ranking travava | `u8 k += 128` dava a volta | `u16` |
| Pilha vs BDOS | corrupção sem disco | pilha cai sobre `0xF37D` | pilha ≤ `0xF300` |
| IX/IY | travamentos aleatórios | rotinas ASM chamando BIOS | preservar |
| Tela de nome | BS saía da tela | BS = "voltar" global | BS só apaga |
| Cursor sumiu pós-corrida | invisível | cenário da corrida sobrescreve `0x7400-0x7FFF` | `cursor_init()` no `race_go` |
| Cursor derivando | canto inferior direito | `GTSTCK(1)` com mouse; leitura rápida demais | §11.2 |
| Bob's | carro invadia a "porta" do jogador | base do carro y=157 sobre a moldura (começa em 157) | base y=154 |
| Parafusos | estados invertidos | índices de arte trocados (13=87 alto, 15=89 cabeça) | mapeamento corrigido |
| PARTS | lista não abria por categoria | fluxo antigo | clique na categoria abre a lista |
| Janelas | piscar a cada clique | apagar+redesenhar na tela | buffer fora da tela |
| Trocas de cena | 0,5–1,7 s de preto | tela apagada para carregar | cena em páginas ocultas |
| Rodas | fora das arcadas | detecção grosseira + ±4 px | contorno inferior + ápice |
| Cabeça do piloto | fora da janela | % fixo da altura | centro da janela do motorista |
| Corrida | queda a 6,9 fps com cenário | hoisting do SDCC | funções por objeto |
| Overflow 16 bits | `(y−HZ)×2074` | multiplicação em `i16` | `2·(y−HZ)+((y−HZ)>>5)` |
| Capô | listras e serrilhado | limite no primeiro pixel não-preto | mediana de 9 colunas |
| Música | 3 vozes ruidosas | CQT com vazamento ±1 semitom | filtros + média das 2 repetições |

### 14.6 Histórico de versões da ROM (sessão final)
| ROM | Mudança principal |
|---|---|
| m15b | janela lateral sem preto, retrovisor esquerdo |
| m15c | oponente nos dois retrovisores |
| m15d | cursor volta após a corrida; arte do motor sem apagar a tela |
| m15e | música desligada na corrida (inclusive FM) |
| m15f | regras de aceitar/recusar portadas do original |
| m15g / m15j | correções do mouse (GTSTCK(1); leitura por quadro e DI) |
| m15h / m15i | capô na cor do carro (versão final: superfície sólida) |
| m15k → m15l | (vidros opacos revertido) base do carro/sombra no Bob's acima da moldura da porta |
| m15m | janelas sem piscar |
| m15n | trocas de cena sem tela preta |
| m15o | regras do Rei (pilotos saem; "todos fora" libera) |
| m15p | tema original como estação padrão |
| m15q | rodas nas arcadas e motorista na janela |
| **final** | = m15q |

---

## 15. Catálogo de armadilhas (o que dói de verdade)

**Engenharia reversa**
1. **DS errado** → endereços falsos. Valide o `DS` com strings conhecidas **antes** de seguir.
2. **Desassembler linear** → falsos positivos de imediatos. Confirme o contexto.
3. **Mensagens por ID**, não por ponteiro: buscar o ponteiro da string não acha nada.
4. **Tabelas indexadas** com deslocamento negativo (`0x8BAE` = `-0x7452`): procure por **soma de base + índice**, não pelo endereço direto.
5. **"É regra do jogo ou minha?"** Marque a fidelidade de cada regra (🟢🟡🔴) desde o primeiro dia.
6. **Unidades** (combustível 140, tick 72,8 Hz, subunidades de segmento) — decida **a unidade do jogo** e converta nas bordas.

**SDCC / Z80**
7. **IX/IY** pertencem ao compilador; BIOS e rotinas ASM os destroem.
8. **`jr` curto**: use `jp` em rótulos distantes.
9. **`i32` composto**: reescrever em passos e testar contra Python.
10. **Hoisting**: o otimizador pode içar cálculos caros para fora de condicionais; meça (`PROF`) e separe em funções.
11. **BSS** precisa ser zerada no `crt0`.
12. **`__banked` com ponteiros** é inviável; passe **escalares**.
13. **Constantes** ficam no banco onde são definidas; acesso de outro banco → `bank_copy`.
14. **Um núcleo de 16 KB** acaba: mantenha ≥ 1 KB de folga; planeje o que sai quando estourar.

**VDP**
15. **Comandos executam em paralelo**: espere (`vdp_wait`) antes de reusar registradores/VRAM que o comando ainda usa.
16. **HMMM/HMMV exigem x e largura pares.** Quadros com x ímpar precisam de caminho LMMM.
17. **TIMP** (cor 0 transparente) torna o **preto** transparente: reserve um índice de "preto opaco" (no port, o contorno escuro do botão do câmbio usa o índice 11, cinza escuro).
18. **Cópias de página inteira** custam ~27 KB por HMMM (alguns ms): contem como quadro inteiro.
19. **Troca de paleta e de página no mesmo quadro** (vblank) para não ver cores trocadas.
20. **Carga de VRAM com tela ligada** é mais lenta; aceitável para páginas ocultas.
21. **Sprites e VRAM:** a área dos sprites (`0x7400-0x7FFF` da página 0) fica fora das 212 linhas visíveis (que terminam em `0x6A00`), mas **qualquer carga que use as linhas 212-255 da página 0 a sobrescreve** (o cenário da corrida faz isso): recrie os padrões depois.
22. **V9938 ≠ V9958:** se mirar em MSX2+, há scroll horizontal por registrador (R#26/27) — ver §18.5.

**Hardware real**
23. **Mouse:** 1 leitura por quadro, `DI`, nunca `GTSTCK(1)` com mouse.
24. **Disco:** DTA antes de ler/gravar; **a pilha** não pode cair em `0xF37D`.
25. **PAC:** detectar com rotina **em RAM**; página 1 troca de slot.
26. **Detecção do mapper** por emuladores usa padrões de `LD (nn),A`.
27. **Tempo:** PAL (50 Hz) vs NTSC (60 Hz) muda `JIFFY`; tudo que depende de tempo precisa de `jhz`.
28. **MSX1 não serve:** o jogo exige V9938.

**Arte**
29. **320→256 por descarte de 1 em cada 5 colunas** preserva dither; média cria cores novas.
30. **Arcadas medidas pelo contorno** são mais robustas que detecção de "vãos".
31. **Janela do motorista** = transparente cercado por corpo (flood-fill a partir da borda).
32. **Texto miúdo** de telas EGA vira lixo em 256 colunas: apague e **refaça com a fonte do jogo**.

**Processo**
33. **Medir antes de afirmar**: fps, bytes de banco, correlação de arte.
34. **Capturas da VRAM** (não `screenshot`).
35. **Fixar por flags de teste** (`-DSTART_CAR`, `-DTEST_ACCEPT`) para tornar testes determinísticos.
36. **Conferir `DEFAULT CAR`** antes de enviar ROM.

---

## 16. Orçamentos e medições

### 16.1 Espaço (ROM final)
| Item | Valor |
|---|---|
| Arquivo | 524.288 B (512 KB; 32 bancos de 16 KB) |
| Núcleo (banco 0) | **16.214 / 16.352 B** (folga de 138 B) |
| Bancos de código 1-5 | 15.242 / 15.365 / 8.810 / 13.324 / 7.162 B |
| Assets + música | bancos 6-28 (23 bancos; 387 faixas de dados comprimidos) |
| Música | 6 estações (1 banco cada) |

### 16.2 Tempo
| Operação | Custo |
|---|---|
| Comando de VDP (emissão) | ~1,2 ms (CPU) |
| `HMMV` | ~2,76 µs/byte (~360 KB/s) |
| Janela 256×112 pintada uma vez | ~40 ms |
| `HMMM` de 27 KB (página inteira) | alguns ms |
| `project` (15 linhas) | ~13 ms |
| Entrada de carro na garagem | ~1,5 s |
| Troca de cena (carga + apresentação) | mantém a tela sempre visível |
| Quadro da corrida | ~145 ms (6,9 fps) |

### 16.3 Evolução do núcleo
13,3 KB (M5) → 13,2 KB (após BANK1/3) → 15,4 KB (mouse/cursor) → 15,8 KB (janelas) → 16,0 KB → 16,2 KB (final). Cada marco empurrou funções para o BANK2/3/4/5.

---

## 17. Estado final, limitações e pendências

### 17.1 Concluído
Todo o ciclo do jogo (§1.3), com regras de aposta e do Rei **decodificadas do original**, arte original reamostrada, música tema, salvar/carregar, ranking.

### 17.2 Limitações conhecidas
- **Sem teste em disco real / PAC real** (só BDOS falso e SRAM do PAC no openMSX).
- **Deriva do cursor**: correção aplicada, sem confirmação em hardware.
- **Janela esquerda estática** na corrida (custaria velocidade tornar dinâmica).
- **TUNING e listas** ainda piscam (não usam o buffer fora da tela).
- **Núcleo no limite** (138 B de folga).
- **Regras 🔴/🟡** (§9) não decodificadas: preços de oferta, barganha, venda, quebras (exceto pneus), multas, ranking inicial, pistas de estrada (geradas em execução no original).
- **Fidelidade do oponente:** IA simplificada (rubber banding mantido, mas sem decodificar tudo).
- **Flag global `[0x610]`** do original (bônus de aceitação) não identificada.
- **Unidades de combustível** convertidas (7 un. = 1 galão).
- **Música:** transcrição aproximada; sem percussão; volume decai a cada nota no PSG.
- **Fps** 6,9 (< 8,1 do original).

### 17.3 Ideias não feitas
Música por canal de FM com timbres do original (se existirem); animações do Bob's (garçonete, etc.); usar sprites para o oponente; `V9958`/`turbo R`.

---

## 18. Plano para o jogo futuro (versão brasileira, regras idênticas, assets originais)

> **Premissa do projeto novo:** um jogo **mecanicamente idêntico** ao Street Rod (mesmos números, mesmas regras, mesmo fluxo), mas com **conteúdo novo e temático do Brasil** (carros, cidade, personagens, textos, música, arte). Isso separa o projeto em **três camadas**: (A) *regras* (reaproveitadas/decodificadas), (B) *engine* (reaproveitada e melhorada), (C) *conteúdo* (100% novo).

### 18.1 Decisões de arquitetura que eu tomaria desde o dia 1
1. **Tudo orientado a dados.** O Street Rod já é assim (HOT_DATA). No jogo novo: carros, peças, pilotos, tabelas de aposta, textos (com IDs), pistas, cenários e sons ficam em **arquivos de dados** compilados em bancos (formato binário simples + gerador). O código **não contém nenhum número de regra** além do que está nas tabelas. Isso permite trocar o tema inteiro sem mexer em C.
2. **Banco de regras documentado e testado.** Uma planilha/Python (`rules/`) com **cada regra numerada**, sua fonte (🟢🟡🔴) e um teste. O mesmo Python gera as tabelas do jogo e testa o Z80 por simulação (como foi feito com `powertrain.py` × `ptmodel.h`).
3. **Núcleo pequeno e banked desde o início.** Planeje o núcleo para **≤ 10 KB**; todo o resto em bancos. Padronize chamada entre bancos com **tabela de saltos** (jump table) em vez de `__banked`, para poder passar **ponteiros para estruturas em RAM** (o limite "só escalares" custou caro).
4. **Double-buffer como regra**: toda janela, toda cena, toda troca de paleta. Uma única API `present()`.
5. **Mouse/joystick/teclado em uma camada única** (`input.c`) com política "1 leitura por quadro, DI, nunca misturar GTSTCK(1) com mouse".
6. **Resolução nativa 256×212**: **desenhe a arte já em 256 colunas** (não reamostre de 320): evita o artefato do descarte de colunas e economiza tempo.
7. **Paleta por contexto (16 de 512 cores)** com um **mapa de índices fixos** (reservar índices para pintura, reflexo, contorno opaco, oponente, capô, etc.) — o Street Rod passou por 3 revisões dessa reserva.
8. **Build reprodutível** (`make`), CI local, **testes automáticos no openMSX** para cada marco, **captura da VRAM** padrão.
9. **Medição contínua** de bytes por banco e fps (falhar o build se o núcleo > limite).
10. **MSX2+/turbo R opcional:** se o público incluir MSX2+/turbo R, medir e usar R#26/27 (scroll horizontal) e CPU rápida (§18.5); manter **MSX2 puro como alvo mínimo**.

### 18.2 O que reaproveitar do port atual
| Peça | Reuso |
|---|---|
| `bankify.py`, `ihx2rom.py`, `build.sh` | direto (considerar substituir por tabela de saltos) |
| Carregador de assets + LZ4-like + `scene_load/present` + `win_frame/flush` | direto |
| Projeção e `strip_run` (asm) | direto (ajustar para outras proporções) |
| Física e `ptmodel.h` | **direto** (as regras são as mesmas) |
| UI (`ui_list`, `pick_hot`, `hot_at`) | direto |
| `cars_art.py` (arcadas, janelas, variantes) | adaptar para a **nova arte** (contorno inferior + janelas fechadas continuam válidos) |
| Música (`music_gen.py` + player + FM) | direto (formato de linhas); **componha** novas estações |
| Save/PAC/disco | direto (**e testar em hardware real!**) |
| Ferramentas de teste (Tcl, `vram2png.py`) | direto |
| `srlib.py`, `unexepack.py`, `hotdata.py` | só para consulta ao original; **não entram no jogo novo** |

### 18.3 Dívida de engenharia reversa (para "rigorosamente igual")
Lista priorizada do que **ainda não é igual** ao original e como obter:
| # | Item | Hoje | Como decodificar |
|---|---|---|---|
| 1 | **Pistas** (curvas, alturas, flags, cenário por segmento) | sintéticas, 256 segmentos repetidos | achar o **gerador** da pista no EXE (tabelas zeradas no arquivo; preenchidas por rotina em tempo de execução) e portar; ou capturar a pista rodando num emulador DOS (dump de `DS:` por segmento) |
| 2 | **IA do oponente** | simplificada (rubber banding, troca em `corte − 100·a`) | decodificar `img 0xD544` e vizinhança: limiares 5/12, alvo lateral, "colabora/bloqueia" |
| 3 | **Ofertas do jornal** (quais carros, preço, barganha) | regra minha | rotinas do jornal (procure a mensagem "How about $%d?" ID e `rand`) |
| 4 | **Venda de carro e peças** | 55% / 50% | mensagens "I'll take it off your hands for nothing" / "You must be crazy" → ramos de preço |
| 5 | **Quebras e desgaste** (motor, câmbio) | regra minha | busque os acessos ao desgaste (`DS:` de `wear`/`trim`) e as mensagens "Blew your engine again?" etc. |
| 6 | **Multas e radar** | regra minha | `speed limit`, `Max speed`, mensagens de multa |
| 7 | **Combustível** | unidades convertidas | usar 40/140/2/5/15/30 originais |
| 8 | **Ranking inicial e pontuação** | inventado | LIB1 item 4 e tabela do EXE |
| 9 | **Flag global `[0x610]`** | ignorada | achar quem a escreve (`DS:0x610`, valores 0, `0x62`, `0x63`) |
| 10 | **Blocos 3/4/5 do HOT_DATA** | ignorados | geometria das figuras (para fidelidade de carros na rua) |
| 11 | **Dano** (`f1,f2,f3` da fórmula) | ignorado | ver quem seta os bits de dano |
| 12 | **Tabela de pistas por modo** | — | mesmo que #1 |
| 13 | **Tempo de resultado** (premiação) | texto/valores do port | `Big deal`, `Fifty smackers`… |
| 14 | **Música/efeitos do original** | só o motor no speaker | sem fonte; fazer novo |

**Método recomendado:** rodar o **original num emulador DOS com depurador** (DOSBox-X debugger) e instrumentar: *breakpoints* na rotina de aposta (`0x9BC5`), de pós-corrida (`0xAEF3`), de pista (gerador) e de oponente (`0xD544`); registrar entradas/saídas por centenas de execuções e ajustar o Python até **reproduzir exatamente**. A **verificação estatística** (milhares de desafios) é o teste de aceitação.

### 18.4 Conteúdo temático do Brasil (proposta de adaptação mantendo as regras)
**Premissa de ambientação (sugestão):** o jogo original se passa no **verão de 1963 (EUA)** — 46 dias de agosto a 15 de setembro. Uma versão brasileira poderia usar o **fim de semana de "racha" dos anos 70** ou um **verão carioca/paulistano** (ex.: 45 dias de janeiro a meados de fevereiro, terminando no Carnaval). A estrutura de 46 dias e o "fim de verão" são **números de regra** — mantê-los.

| Original | Equivalente brasileiro (sugestão) |
|---|---|
| Bob's Drive-In | **lanchonete/trailer** em avenida beira-mar ("Lanchonete do Zé", "Dog do Alemão") |
| Gus' Gas | **posto Ipiranga/Shell-style fictício** ("Posto Pelé"/"Auto Posto Seu Tião") |
| Los Angeles News | **jornal local fictício** ("Folha do Povo"/"Diário da Cidade") |
| DMV | **DETRAN** (carteira "CNH" — manter a tela de nome) |
| Highway Patrol | **Polícia Rodoviária** |
| The King | **"O Rei do Asfalto"** |
| Pink slip | **o documento do carro (CRLV/DUT) — "apostar o documento"** (mesma regra, termo local) |
| GM / Ford / Chrysler (máscaras 1/2/4) | **Chevrolet / Ford / Dodge/Chrysler** (e VW como 4ª marca: ampliar a máscara para 4 bits) |
| Carros 1930–1960 | **Fusca, Karmann-Ghia, Variant, Brasília, Chevette, Opala (4 e 6 cil.), Maverick V8, Dodge Dart/Charger R/T, Corcel, Puma, Gordini, Kombi, Landau, Galaxie, Belina, Passat, Gol GT** etc. (**26 carros**, mantendo a distribuição de desempenho 0..8 da tabela) |
| Carburadores | **Solex, Weber, Brosol, DFV** (3 níveis: 1 corpo, 2 corpos, racing) |
| Coletores / motores | **coletores de escape/admissão** nacionais; motores 4 cil. / 6 cil. / V8 (três tamanhos, como o original) |
| Pneus | **comum / Pirelli Cinturato (marca) / slick** (3 tipos, mantendo as tabelas de desgaste) |
| Rádio | **ritmos brasileiros** como "estações" (rock nacional dos 70, samba-rock, MPB, jovem guarda, funk/soul) |

**Assets novos (especificação mínima, em 256×212 nativos, 16 cores com mapeamento fixo):**
| Categoria | Qtde | Formato / tamanho | Notas |
|---|---|---|---|
| Cockpit (visão do motorista) | 1 + variantes (3 câmbios, manual/auto) | 256×190 + painel de câmbio 38×43 | **paleta reservada** para capô (2 cores) e pintura |
| Garagem | 1 | 256×190 | com objetos clicáveis (rádio, calendário, jornal, galão, compressor, macaco, porta, relógio) |
| Carros laterais | 26 × (corpo) + 3 rodas + 8 adesivos | ~190–220×55–67 | **janelas fechadas** (para o retrato) e arcadas bem definidas; camada de pintura = índices 6/7 |
| Retratos de pilotos | 22 | ~30×30 | o do Rei destacado |
| Peças do motor | motores (3), coletores (7), carburadores (3), câmbios (4), pneus (4), parafusos (3 estados) + furo + fiação (2) | variados | **medir as posições de encaixe** como foi feito |
| Telas: título, créditos, ranking, vitória | 4 | 256×212 | + **tela de proteção** (opcional) |
| Jornal (página + manchetes) | 1 + 5 | 256×190 | legível em 256 colunas |
| Lanchonete (cenário) | 1 | 256×190 | com carros e personagem de cenário |
| Posto, calendário (2 meses) | 2 | | |
| Cenário da corrida | árvores, prédios, placas (em 7 tamanhos cada), postes, horizonte | | **combine com a paleta** |
| Retrovisores | 2 máscaras | | |
| Cursor | 1 (16×16, 2 planos) | | |
| Sprites do oponente por trás | 3 modelos | 9 tamanhos | |

**Textos:** manter **IDs de mensagem** e a estrutura de 1-3 linhas por mensagem (ver §2.5); traduzir/criar **todos** os textos (frases do oponente, jornais, recibos) e **preservar o tom** (gírias).
**Música:** ver §10.3 — escrever **estações novas** (3 vozes + percussão no FM); manter o **formato de linhas**.

### 18.5 Desempenho: como passar de ~7 fps para 12–15 fps
Medidas atuais: 145 ms por quadro, sendo **61 ms só de VDP na estrada**.
1. **Menos pixels pintados:** estrada em **menos faixas** (menor resolução vertical de projeção nas linhas próximas ao horizonte), **grama estática** (paleta de dois tons ou padrão estático), **zebra** só nas bordas (menos retângulos).
2. **Reaproveitar o quadro anterior** (copiar linhas mudadas) com `HMMM` em vez de repintar tudo.
3. **MSX2+ (V9958):** **scroll horizontal por registrador** (R#26/R#27) e **interrupção de linha** (R#19) para deslocar cada faixa da estrada sem repintá-la: a estrada vira *1 cópia de textura + deslocamento por linha*. É pesquisa; promete 2–3× (não implementado aqui; hipótese).
4. **turbo R (R800):** CPU ~5–10×; o VDP continua igual, então o gargalo vira o `HMMV`.
5. **Sprites** para o oponente e para os retrovisores (liberam comandos).
6. **Código em assembly** nas fases de projeção, `scn_*` e HUD.
7. **Menos cenário por quadro** (cenário só cada 2 quadros).
Meta realista: **10–12 fps no MSX2** (acima do teto de 8,1 do original) e 15+ no MSX2+.

### 18.6 Fases e esforço (estimativa de ordem de grandeza; ajustar ao seu ritmo)
| Fase | Conteúdo | Saída | Esforço relativo |
|---|---|---|---|
| 0 | **Especificação de regras**: fechar a dívida §18.3 (itens 1-9) com emulador DOS | `rules/` + testes Python | alto (o maior risco) |
| 1 | **Ferramentas**: pipeline de assets de PNG, geradores, build, testes | repositório base | médio |
| 2 | **Engine mínima**: estrada (arrancada), cockpit placeholders, física | corrida jogável | médio |
| 3 | **Jogo em modo texto/placeholders** (garagem, jornal, Bob's, apostas, save) | jogo completo feio | médio |
| 4 | **Arte** (a maior fatia do trabalho): garagem, cockpit, 26 carros, peças, telas | arte final | alto |
| 5 | **Áudio**: efeitos e 4-6 estações | trilha | médio |
| 6 | **Polimento**: double-buffer em tudo, mouse, textos, telas finais | beta | médio |
| 7 | **Hardware real**: MSX2 com mouse, disco, PAC, FM-PAC, PAL/NTSC | RC | médio (e **imprescindível**) |
| 8 | **Documentação, manual, empacotamento** | release | baixo |

### 18.7 Checklist de qualidade (aceitação)
- [ ] 100% das regras 🟢 com teste automático e **simulação estatística** para as aleatórias.
- [ ] Partida completa automatizada (início → fim do verão) sem travar, **10 sementes**.
- [ ] Salvar/carregar em **disco real** e **PAC real**; ranking persistente.
- [ ] Mouse real: sem deriva em **nenhuma** tela; joystick e teclado ok.
- [ ] PAL e NTSC.
- [ ] Sem tela preta e sem piscar em troca de cena/janela.
- [ ] Núcleo com folga ≥ 1 KB; relatório de bytes por banco.
- [ ] Fps ≥ meta em MSX2; sem quedas.
- [ ] Música: loop sem emenda; FM e PSG.
- [ ] Textos revisados; sem "DEFAULT"/placeholders.

### 18.8 Registro de riscos
| Risco | Probabilidade | Impacto | Mitigação |
|---|---|---|---|
| Decodificar a **geração de pistas** do original é difícil | média | alto | usar pistas próprias desenhadas com as mesmas proporções; capturas de emulador |
| Arte nativa 256×212 consome muito tempo | alta | alto | começar com o **conjunto mínimo** (garagem, 6 carros, cockpit) e crescer |
| Núcleo de 16 KB estourando | alta | médio | núcleo mínimo + tabela de saltos entre bancos |
| Bug só em hardware real (mouse/disco) | alta | alto | **testar em hardware** a cada marco, não só no fim |
| Desempenho aquém do original | média | médio | §18.5; medir a cada fase |
| Direitos autorais (se reutilizar qualquer asset do original) | — | alto | o projeto novo usa **assets originais**; as **regras** são uma ideia de jogo, não texto; decisão é sua |

### 18.9 "Cola" rápida para recomeçar
1. Reuse este repositório como `engine/`; apague `srlib/unexepack/hotdata` do build do jogo novo.
2. Crie `content/` (PNG, CSV, textos) e um `pack.py` que gera os bancos no **mesmo formato** do `assets.inc`.
3. Substitua `gen_assets.py` etapa por etapa por leitores de `content/` (cada função `garage_pixels(lib2, item)` vira `load_png('garage.png')`).
4. Mantenha `main.c` como está, **troque as tabelas** (`cars.h`, `tables.h`, `textdata.bin`, `assets*.h`).
5. Execute a bateria de testes `*_test.tcl` e compare capturas.

---

## 19. Apêndices

### A. Inventário de arquivos (`streetrod-port/`)
- `NOTES.md` — engenharia reversa (formatos, física, dados).
- `GAME_DESIGN.md` — fluxo e marcos.
- `tools/`: `unexepack.py` (EXEPACK), `srlib.py` (LIBs), `extract_lib.py`, `hotdata.py` (HOT_DATA → CSV/JSON), `powertrain.py` (modelo), `physsim.py` (simulador).
- `msx2/road/`:
  - **Fonte:** `main.c`, `vdpasm.s`, `proj.s`, `crt0.s`, `ptmodel.h`, `pt_dbg.c`.
  - **Geradores:** `gen_assets.py`, `gen_tables.py`, `cars_art.py`, `music_gen.py`, `theme_data.py`, `palette_def.py`.
  - **Build:** `build.sh`, `fullbuild.sh`, `bankify.py`, `ihx2rom.py`, `config.inc` (gerado), `bench.sh`, `matrix.sh`.
  - **Gerados:** `assets.bin/.inc/.h`, `assets_race.h`, `assets_scn.h`, `cars.h`, `tables.h`, `textdata.bin`, `*.rom`.
  - **Testes/ferramentas:** ~70 `*.tcl`, `vram2png.py`, `rshot.sh`, `shot.sh`, `prof.py`, `run_test.sh`.
  - **Docs:** `README.md` (diário), `docs/` (capturas).

### B. Tabelas do original
**Modelo do carro** (§2.7). **Limiares de avanço** (estrada, mph→subunidades): `13→1, 27→2, 41→3, 55→4, 69→5, 83→6, 97→7, 101→8, 105→9, 109→10, 113→11, 117→12, 121→13, 125→14, 129→15` (e `>129`→16); arrancada: `v ≤ 71: v/8+1; senão v/4−8`.
**Aposta:** `T9 = {210,180,150,175,150,125,140,120,100}`, `T10 = {60,45,30,55,40,20,50,30,10}`, `T11 = {40,60,90,70,65,25,90,70,15,90,35,0}`.
**Gate do Rei:** limiares 3, 7, 6 e metade das corridas; combustível 7/17/22 (de 140).
**Ranking inicial (port):** KING 2900, SLICK 2300, BIG AL 1800, DUCKTAIL 1500, RED 1200, VINNIE 1000, BOBBY 800, MOUSE 650, LEFTY 500, ROOKIE 350 (inventados).

### C. Mensagens (IDs → texto) principais
| ID | Texto |
|---|---|
| `0x1113` | I don't got time for amateurs like you! |
| `0x113D` | You're doing OK kid, but you're gonna need to win more races! |
| `0x117D` / `0x11A7` | You're gonna hafta win more drag / road races! |
| `0x11D1` | You're on! Let's hit the drag race! |
| `0x11F7` | Cool, dude! Get your pink slip out, and let's go! |
| `0x122C` | Chicken! |
| `0x131B` | You don't have enough dough for that bet! |
| `0x1761` | You're gonna hafta get more gas |
| `0x172E` | You don't got enough gas to cruise down |
| `0x198C` / `0x199F` / `0x19B1` | What's his name? / Gas ain't free! / You've got a full tank! |
Respostas de aposta (arrancada): recusa "Blow it out your ear!", "Forget it, geek!", "Beat it, twerp!", "No way, ya nerd!", "Get lost!"; aceite "Cool!", "O.K.!", "Let's hit it!". (estrada): recusa "Get lost!", "Give me a break!", "Get outta my face!", "Buzz off!", "Whadda loser!"; aceite "You're on!", "Alright!", "Let's go!".

### D. Registradores do V9938 que o jogo usa
`R#0..R#1` modo/ligação de tela (bit 6 do R#1 = display; bit 1 = sprites 16×16), `R#2` página visível (`0x1F`/`0x3F`/`0x5F`…), `R#7` cor da borda, `R#8` bit 1 = sprites off, bit 5 = TP, `R#9` bit 1 = 50 Hz, `R#14` A16-A14 de VRAM, `R#16` índice de paleta (+ porta `0x9A`), `R#17` acesso indireto, `R#32..R#46` comandos (SX, SY, DX, DY, NX, NY, CLR, ARG, CMD), status 2 bit 0 = CE (comando em andamento).

### E. Glossário
- **Arcada:** abertura da roda na carroceria.
- **Banco:** bloco de 16 KB da ROM selecionável (ASCII16).
- **CQT:** Constant-Q Transform (espectro por semitons).
- **Hotspot:** retângulo clicável.
- **HMMM/HMMV/LMMM/LMMV/LINE:** comandos de VDP (cópia de bytes, preenchimento de bytes, cópia lógica de pixels, preenchimento lógico, linha).
- **Núcleo:** banco 0 fixo (página 1).
- **Pink slip:** a propriedade do carro apostada na corrida.
- **TIMP:** operação lógica "cópia exceto cor 0" (transparência).
- **Gate do Rei:** função `G` (§2.9).

### F. Referência cruzada de endereços do EXE (img)
`0x9BC5` aceita aposta · `0x9D6D` fluxo do desafio · `0x9556` gate do Rei · `0x98BA` chegada de piloto · `0xAEF3` pós-corrida · `0xC83C` pós pink slip · `0xDCBE` modelo do carro · `0xD29D` física por tick · `0xD544` avanço e IA · `0x8EA8` cruzeiro · `0x2834C` desenho da estrada · `0x1FE2` ISR do relógio · `0x3AB8` mensagem por ID · `0xE926` descompressão de LIB.

### G. Lista de verificação ao distribuir uma ROM
1. `./fullbuild.sh` (com `LIB2/HOT_DATA/SRSE_EXE`). 2. `grep -c "DEFAULT CAR" textdata.bin` = 0. 3. Bancos dentro do limite. 4. Rodar `trans_test.tcl` e `bobsarr_test.tcl`. 5. Abrir no openMSX (C-BIOS MSX2) e **em MSX real**. 6. Conferir mouse e salvar.

---
*Fim do guia.*
