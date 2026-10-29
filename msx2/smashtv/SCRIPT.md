# Máquina de script das salas

Cada sala é um pequeno programa em bytecode (`rooms.inc`) executado por `script.inc`. A ideia vem do
original (ver `tools/sms/NOTES.md`, seção "Máquina de script"): opcodes de 1 byte com operandos inline,
pilha de retorno para sub-ondas, laço com contador, esperas em ticks e "esperar até limpar". O conjunto de
opcodes é próprio, mais enxuto e sem nenhum dado do jogo original.

## Execução

`script_step` roda uma vez por tick (depois das colisões). Se `scr_wait` ≠ 0, decrementa e só segue quando
chega a zero (então `WAIT n` retoma exatamente n ticks depois). Depois executa opcodes em sequência até um
deles bloquear, com no máximo **32 por tick**, então um script defeituoso não trava o jogo.

Um opcode que falha (pilha estourada ou vazia, opcode desconhecido) **encerra o script** (`scr_done = 1`).

## Opcodes

| op | nome | operandos | efeito |
|---|---|---|---|
| 00 | `END` | | encerra o script |
| 01 | `WAIT` | n | retoma n ticks depois (bloqueia) |
| 02 | `SPAWN` | tipo | um inimigo numa borda aleatória; se o pool está cheio, **espera** e tenta no tick seguinte |
| 03 | `SPAWNAT` | tipo, x, y | um inimigo numa posição fixa (mesma regra de pool cheio) |
| 04 | `WAITCLEAR` | tipo | bloqueia enquanto houver inimigo vivo desse tipo (`$FF` = qualquer) |
| 05 | `LOOP` | n | repete o corpo até o `ENDLOOP` correspondente, n vezes (aninhamento até 4) |
| 06 | `ENDLOOP` | | |
| 07 | `CALL` | addr16 | chama um sub-script, que termina em `RET` (aninhamento até 4) |
| 08 | `RET` | | |
| 09 | `JUMP` | addr16 | |
| 0A | `CALLR` | n, addr16 × n | chama um dos n sub-scripts, escolhido ao acaso |
| 0B | `EXIT_OPEN` | | abre a porta na parede de cima; encostar nela inicia a próxima sala |

Tipos de inimigo: `KIND_GRUNT` (1 HP, velocidade 2, 10 pontos), `KIND_RUNNER` (1 HP, velocidade 3, 20 pontos) e
`KIND_TANK` (3 HP, velocidade 1, 50 pontos). As tabelas estão em `game.inc` (`kind_hp`, `kind_speed`,
`kind_points`) e a arte de cada tipo é gerada por `tools/msx/gen_gfx.py`.

## Exemplo

```asm
; sala 2: três pares de grunts, cada par fechado por um runner
room2:  db OP_WAIT, 8
        db OP_LOOP, 3
        db OP_CALL
        dw wave_pair
        db OP_WAIT, 14
        db OP_ENDLOOP
        db OP_WAITCLEAR, 0xFF
        db OP_EXIT_OPEN
        db OP_END

wave_pair:
        db OP_LOOP, 2
        db OP_SPAWN, KIND_GRUNT
        db OP_WAIT, 3
        db OP_ENDLOOP
        db OP_SPAWN, KIND_RUNNER
        db OP_RET
```

Para criar uma sala, escreva o script em `rooms.inc`, acrescente o rótulo a `room_table` e aumente
`ROOM_COUNT`. Depois da última sala o jogo volta à primeira.

## Fluxo de uma sala

`room_start` aposenta os inimigos vivos (o renderizador os apaga dos dois buffers), zera balas, fecha a
porta, volta o script ao início e põe o jogador embaixo com 20 ticks de proteção. `check_exit` testa se a
porta está aberta e o jogador (16 px) está inteiro dentro da fresta. A porta, como o HUD, é desenhada na
página escondida, uma vez por buffer, para a página exibida nunca mudar enquanto está na tela.

## Testes (`tools/msx/test_smashtv.py`)

Scripts feitos à mão são injetados na RAM e verificam: tempo exato do `WAIT`, `LOOP` e laços aninhados,
`CALL`/`RET`, distribuição do `CALLR`, `WAITCLEAR` (geral e por tipo), `SPAWN` com pool cheio, `SPAWNAT`,
porta nas três páginas, seis formas de script defeituoso e um `JUMP` infinito. Um bot joga as quatro salas
reais, com vidas extras, e o teste confere a sequência 1 → 2 → 3 → 4 → 1, a porta fechada em cada sala, os
inimigos antigos apagados da tela e o número da sala no HUD.
