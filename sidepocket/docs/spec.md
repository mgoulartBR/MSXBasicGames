# Especificação extraída da ROM (Side Pocket, Mega Drive, USA, T-13 1992)

Notas de engenharia reversa para calibrar `src/sp_logic.c`. Só contêm endereços e
comportamento; nenhum byte da ROM é distribuído aqui. Rode `tools/rom_probe.py <rom>`
com o seu próprio dump para reproduzir.

## Estrutura geral
- Entrada: `$47C` (init TMSS/VDP/Z80) → `$576` → idle loop em `$260`. O jogo roda inteiro no **V-int (`$690`)**.
- O V-int despacha por `$FFF107` (estado) numa tabela de longs em `$6EA`: 0 = nada, 1 = `$6F8` (frame de jogo: `$22A40`, `$23FE8`, entrada `$760`), 2 = `$710`.
- Entrada de controle: `$FFF100`/`$FFF102` (pads), convertida para máscara em `$FFC014`.
- Pontuação: 16 bits em `$FFC056/57`, convertida para dígitos decimais em `$76A8` (÷10000, ÷1000, ÷100, ÷10).
- RAM do jogo: base `$FFC000`; estruturas de objeto em `$FFD000+` (ex.: efeito de espiral usa `$FFD144–$FFD14E`).

## Trigonometria (confirmado)
- Rotina de seno em **`$7A1C`**: entrada ângulo 8 bits em `$FFC033` (256 por volta); saída valor 8 bits em `d1` e sinal em `$FFC026`.
- Tabela em **`$7B1C`**: 64 bytes, quarto de onda `round(255·sin(π/2·i/64))`. Os outros quadrantes são obtidos por espelhamento (`eori #$ff` + offset) e inversão de sinal. Reproduzido exatamente em `sin_q[]`.
- O cosseno é o seno com ângulo + `$40`.

## Velocidade da bola a partir de ângulo e potência
- `$1ACC6`: `comp = sin8(ângulo em $FFC061)`; depois `comp = (comp × magnitude em $FFC060) >> 8` (`mulu.w` de 8×8 bits), com sinal separado em `$FFC066`. A magnitude (`$FFC060`) é escrita em `$314E`.
- `$19BA6`: ângulo em `$FFC07D` → componentes 16 bits com sinal em `$FFC082` (x) e `$FFC084` (y), módulos em `$FFC083/85`.
- Variantes com outros parâmetros em `$1ADAA`, `$1AFF6`, `$1B06A` (multiplicadores `$FFC0B3`, `$FFC0B5`).

## Ainda NÃO encontrado (pendente)
Atrito/desaceleração, rebote nas tabelas, colisão bola-bola e detecção de caçapa. O código está
em regiões acessadas por tabelas de salto que a varredura estática ainda não segue. Próximo passo:
rodar a ROM em emulador com debugger (BlastEm/Exodus), colocar watchpoint nos endereços de
posição das bolas e rastrear quem os escreve, para então anotar as constantes aqui.

## Parâmetros atuais do port (estimados, ainda não calibrados)
Atrito 1/64 por frame, perda de 25% nas tabelas, raio 4 px, velocidade máx. 12 px/frame.
