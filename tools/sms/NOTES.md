# Smash T.V. (SMS, Europe): notas de engenharia reversa

Estas notas descrevem a **estrutura** do jogo (endereços, formatos, fluxo), não contêm código nem
dados da ROM. Símbolos em `smashtv.sym` (usados por `disasm.py --sym=`).

Legenda: **[C]** confirmado (código lido e/ou observado em execução), **[H]** hipótese.

## 1. Visão geral da execução

- Z80, IM 1. `reset $0000 → boot $15D5`: SP=`$D1B2`, mapper, copia `$0400→$C000` (`$11B4` bytes,
  imagem inicial da RAM), inicializa VDP, vai para o fluxo principal (`$57BF`). [C]
- **ISR `$160B`** (VBlank, a cada frame): lê o status do VDP (limpa o flag), `++C1EA`, testa o botão
  Reset (porta `$DD` bit 4 → `$164B`), seta `D02D=1`, decrementa o timer `C223`, troca o slot 2 para o
  banco 2 e chama o **driver de som `$BA09`**, restaura o banco. Não faz mais nada: **toda a lógica
  roda fora da interrupção**. [C]
- **Pacing:** `wait_frame_paced $2731` espera VBlanks até `C223` chegar a 0 e o recarrega com 3. O laço
  de jogo roda a **1/3 da taxa de VBlank** (~20 Hz em NTSC, ~16,7 Hz em PAL). [C]
  Isso torna a velocidade do jogo fácil de reproduzir no MSX2.
- **NMI `$0066`** = botão Pause: alterna `D038` e fica em loop até o próximo NMI. [C]
- Existe um monitor de depuração serial pela porta do joypad em `$00C0–$02A8` (resto de dev cart),
  sem uso no jogo. Pode ser ignorado no port. [C]

## 2. Laço de jogo (`play_room $2025`, laço `$2050–$2169`)

Ordem das chamadas em cada passada (nomes conforme `smashtv.sym`; `?` = ainda não analisada):

1. `read_input $7A20`, `tick_counter $76C3` (contador de ticks e máscara de fase)
2. `player_update $770C`, `player_fire $334D`, `player_misc $3307`
3. `player_vs_enemy_collide $2F3A`
4. `enemy_ai_dispatch $A12A` (IA por tipo)
5. `bullets_vs_enemies $4890`
6. `enemies_draw $4354`, `projectile_draw $373F`, desenhos de HUD e efeitos (`?`)
7. `script_step $57F0` (máquina de script da sala: ondas, portas, spawns)
8. `wait_frame_paced $2731` e, durante o VBlank, `oam_upload $2701` e `vram_queue_flush $442F`

A sala termina quando os testes em `$2139–$21A6` detectam fim (jogadores mortos ou saída aberta). Há também um modo de
**demo/attract** (`D03A≠0`), em que `demo_autoplay $7A77` pilota os dois jogadores. [C]

## 3. Entrada

`read_input $7A20` lê as portas `$DC/$DD` e grava, **1 = pressionado**: [C]

| bit | 0 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| sinal | cima | baixo | esquerda | direita | botão 1 | botão 2 |

`C1F1` = jogador 1, `C1F2` = jogador 2. `$7A5A` anula direções opostas. O jogo é *twin-stick* no
arcade; aqui o jogador 1 usa direcional (mover) e botões, e o esquema de mira é derivado dos bits de
direção (ver estrutura do jogador). `D03A`: 0 = entrada real, ≠0 = demo (2 = demo terminada por botão).

## 4. Estruturas de dados

### 4.1 Jogador (23 bytes, `IY`): P1 em `$C501`, P2 em `$C518`

| off | significado | conf. |
|---|---|---|
| +0, +1, +4 | máscaras de direção (movimento atual, última, mira): b0 cima, b1 baixo, b2 esq, b3 dir | [H] |
| +2 / +3 | **X / Y** em pixels | [C] |
| +0x0A | arma atual (1..5; 0 = morto). É o tipo do projétil criado | [C] |
| +0x0B | número do jogador (0 = P1, 1 = P2) | [C] |
| +0x0C | estado de morte/respawn (0 = vivo; ≠0 durante a animação) | [C] |
| +0x0D | pontos de vida da vida atual (decrementa ao tocar inimigo; 0 = morre) | [C] |
| +0x0F | **vidas restantes** (`$C510` / `$C527`); o dígito do HUD acompanha | [C] |
| +0x10 | temporizador de power-up da arma; ao zerar volta para arma 1 | [C] |
| +0x11 | sub-tipo/flags da arma | [H] |
| +0x12 | **invulnerabilidade** pós-respawn (decrementa; ≠0 ignora colisão) | [C] |
| +0x13 | cópia de opção (`D036/D037`) | [H] |
| +0x14 | bit0 = jogador ativo (`$C515`/`$C52C`) | [C] |
| +0x16 | contador de acertos recebidos | [H] |

Caixa de colisão do jogador: 16 px de largura, e na vertical só a metade inferior (y+12…y+24). [C]

### 4.2 Pools de entidades (16 slots, *struct of arrays*)

O índice do slot é somado ao endereço-base; cada campo é um array de 16 bytes, com campos separados
por `$10`. Acesso típico: `IX = base + slot`, campo = `(IX + k)`. [C]

| `IX+k` | enemy pool (`IX=$C481+slot`) | projectile/pickup pool (`IX=$C391+slot`) |
|---|---|---|
| −0x70 | temporizador de cooldown/atordoamento | ? |
| −0x60 | **HP** (colisão subtrai o dano do projétil) | **dano** |
| −0x50 | ? (valor 2 constante nos dumps) | ? |
| −0x40 | flags de alvo (bits escolhem P1/P2) | **dono** (jogador) |
| −0x30 | timer/direção auxiliar (IA sorteia 1..8) | bit alto do frame |
| −0x20 / −0x10 | **altura / largura** (px) | altura / largura (px) |
| 0 | **tipo** (0 = livre) | **tipo** (0 = livre) |
| +0x10 | estado da IA (0 = não colidível) | estado (≠0 = já acertou) |
| +0x20 / +0x30 | **X / Y** (px) | **X / Y** (px) |
| +0x40, +0x50 | ?, contador de estado | ? |
| +0x60 | **bits de direção** (1 cima, 2 baixo, 4 esq, 8 dir) | ? |
| +0x70 | **frame** de animação | frame |

Tipos de projétil/pickup: 1..5 = tiros do jogador por arma; 6 = objeto ligado ao jogador (dono em −0x40);
7..12 sobrevivem à troca de sala (`projectiles_prune $4CDB`); 12, 14, 15, 16 usam descritores próprios
(`$CDAC/$CDEE/$CDC2/$CDD8`), o resto `$CF4E`. Significados finais (itens, prêmios) pendentes. [H]

Tipos de inimigo (IA em `enemy_ai_table $A164`, 19 entradas, tipos 0..0x12): [C]

| tipo | handler | observação |
|---|---|---|
| 0 | `$A18A` | livre |
| 1 | `$479D` | **grunt** (humanoide verde): confirmado em tela e em colisões |
| 2 | `$4819` | efeito/cadáver: não machuca o jogador [H] |
| 3..0x11 | `$9126, $7C63, $7E0B, $7E0E, $827E, $6D56, $6C84, $8717, $8475, $88DC, $89F7, $8BD9, $8DD6, $8F15, $9CC7` | por analisar |
| 0x12 | `$A0D9` | tocar nele zera o HP e causa dano: obstáculo/mina [H] |

Movimento comum (`enemy_move_dirbits $A18E`): move 4 px por passo conforme os bits +0x60 e desfaz
o passo se `$4D4D` indicar bloqueio. `enemy_aim_at_player $A2AA` calcula os bits em direção ao jogador.
A IA roda numa fase por passada (`C221` alterna 0/1). [C]

### 4.3 Descritores de asset (22 bytes, `IX`, a partir de `$C9B8`)

Usados para carregar tiles e desenhar *tilemaps* (inimigos grandes e cenário são feitos de tiles 8×8 no
nametable `$3800`; sprites de hardware ficam para o resto). [C]

| off | campo |
|---|---|
| +0 / +2 | x / y em tiles |
| +4 / +6 | largura / altura em tiles |
| +8/9 | deslocamento de frame no mapa |
| +10/11 | tamanho dos dados de tile |
| +12/13, +14 | ponteiro e **banco** dos dados de tile |
| +0x10/11, +0x12 | ponteiro e banco do mapa de tiles |
| +0x14/15 | índice-base dos tiles na VRAM (preenchido por `load_tiles_desc $1756`) |

`enemy_desc_ptr_table $CFBC` mapeia tipo de inimigo → descritor. `enemies_draw $4354` converte
X/Y/W/H do slot em tiles (÷8) e desenha pelo descritor.

## 5. Pontuação, vidas, estado

- **Pontuação**: 3 bytes BCD little-endian, P1 `$C226`, P2 `$C229`. `add_score $32DC` (B = jogador,
  C = índice) soma de `points_table $CFE2` (3 bytes BCD por índice). [C]
- **Vidas**: `$C510` / `$C527`. Com `D035≠0` o jogo as mantém em 6 (flag de cheat/teste). [C]
- **Arena**: limites em `$C2B4–$C2B7` (por exemplo `$10,$A8,$10,$E0` ou `$28,$90,$28,$C8`). [C]
- **Contadores**: `C1ED/C1EE` (16 bits, +1 por passada); `C1F3 = n xor (n−1)` (bit k vale 1 a cada 2^k ticks:
  usado para escalonar tarefas). `C1EA` conta VBlanks. [C]
- **Flags**: `C1F0` (bit0 = combate ativo, bit2 = saída aberta?) [H]; `D039` (bit0 = pular/avançar, bit1 =
  bloqueia pular) [C]; `D038` pausa [C]; `D030/D031` mudo de música/efeitos [C]; `D033` sala inicial [C];
  `C1EB` ponteiro da sala atual; `C208` PC da máquina de script. [C]
- **RNG** `$252C`: soma o registrador R (`LD A,R`) ao acumulador `C247`. Depende de hardware (no MSX2 o R
  também existe, mas o resultado não é reproduzível). [C]

## 6. Vídeo (SMS → o que importa para o MSX2)

- VDP em modo 4: **nametable `$3800`**, **SAT `$3F00`** (Y em `$3F00`, X/tile em `$3F80`), tiles 4bpp planares,
  sprites 8×8, display e IRQ de VBlank ligados. `C000`/`C080` são os *shadows* de SAT, enviados por
  `oam_upload` no VBlank. [C]
- **Fila de VRAM** (`C5B8`, ponteiro `C25A`): a lógica enfileira endereços e bytes e `vram_queue_flush $442F`
  despeja tudo no VBlank. [C]
- Paleta: 2×16 cores do SMS (6 bits RGB). `palette_fade $19D6` faz fade para uma tabela alvo. [C]
- **Atenção ao port:** os tiles do SMS têm 16 cores por tile. A **Screen 4 do MSX2 não** (1 bit + 2 cores
  por linha de 8 px), então a arte não mapeia direto. Recomendação: **Screen 5** (bitmap 256×212, 16 cores)
  com cópias de blocos via VDP (HMMM/LMMM) a partir de uma página de tiles, e sprites no modo 2. [H]

## 7. Som

`music_play $9BFF` (A = faixa, 0 = para; respeita `D030`) e `sfx_play $9BD6` (A = id; prioridade pela tabela
`$9C13` e `C292`; respeita `D031`) chamam o driver (`$B936`). `sound_tick $BA09` roda na ISR e atualiza
**6 canais** em `D400 + 0x21·n` e globais em `D4C7`. Saída para o SN76489 (porta `$7F`); no MSX será preciso
um driver novo (PSG e/ou SCC), reaproveitando os dados de música. [C]/[H]

## 8. Bancos

Banco 0 fixo em `$0000`, banco 1 fixo em `$4000`, **banco 2 mantido em `$8000`** durante o jogo. Toda troca do
slot 2 passa por `$170C` (`LD ($FFFF),A / RET`), com o banco vindo de dados dos descritores (`IX+14`,
`IX+0x12`). Bancos 3–F são dados (tiles, mapas, música). [C]

## 9. Pendências

(ver também a seção 10)


- Handlers dos tipos de inimigo 3..0x12 e identificação dos nomes (grunts, orbs, tanques, etc.).
- Máquina de script da sala (`$57F0`, tabela `$581A`): documentar cada opcode e o formato das ondas.
- Lista de efeitos (`$C2B9`, 20 entradas), colisão `$4D4D` e função de morte `$69EE`.
- Formato dos dados de música e do driver de som.

## 10. Máquina de script das salas (`script_step $57F0`)

Confirmado lendo os handlers e traçando a primeira arena (`$3E29`). **Descritor de sala** (ponteiro em `C1EB`):
`+0/1`, `+2/3`, `+4/5` = três palavras (saídas; ≠ 0 faz `$4CF1` desenhar a seta), `+8/9` = ponteiro do script,
`+0x0B…` = nome da sala em texto. `C1EB` vem de `D033` (sala inicial). [C]

- **Execução**: `C208` = PC do script, `C20A` = contador de espera (decrementa a cada chamada, e o script
  só avança quando chega a 0). **Um opcode por chamada**, buscado em `$581A + 2·op` (80+ entradas, os
  endereços de `$4E3C…` e `$67A0…` também guardam stubs de opcodes). [C]
- **Chamada/retorno**: `29` lê 1 byte *n*, empilha o endereço seguinte (pilha em `C21F`, 2 bytes por entrada)
  e salta para a palavra em `$4E3C + n`; `2A` desempilha. As salas têm tabelas de sub-scripts de ondas. [C]
- **Laço**: `4E` lê 3 bytes (`C28F`, `C290` = contagem, `C291` = tipo alvo) e depois faz um `29` com o 4º byte;
  `4C` salva o ponteiro (`C20D`) e inicia o contador `C20C = C290 − 1`; `4D` volta ao ponteiro enquanto
  `C20C ≠ 0`. [C]
- **Spawn**: `0C` cria um grunt (tipo 1) numa posição aleatória livre (valida com `$4D4D`); falhou, repete
  o opcode (`JP $5BF3`). [C] `60` é um gerador aleatório de objetos pequenos (tipos 8–11). [H]
- **Esperas**: `4F` bloqueia enquanto existir inimigo do tipo `C291` (`$FF` = não espera); `27`/`28` mudam a
  porta (`C256` = 0/1), esperam 8 ticks e tocam o efeito `$35`/`$36`; `0A` imprime o nome da sala, limpa
  `C1F0` bit 0 e espera `$20`; `0B` liga `C1F0` bit 0 (combate ativo). [C]
- **Estado de vídeo**: `0F`/`10` empilham/desempilham o cursor de tiles da VRAM (`$1725`/`$1739`), para cada
  sala carregar seus gráficos e liberar depois. Os opcodes `14`, `5A`, `5D`, `01`… carregam descritores de
  asset (`IX = $C9CE…` etc.). [C]

O script da arena 1 tem 4 sub-ondas selecionadas por `29` e termina com `4F`; a lista completa de opcodes e
das outras salas segue pendente.
