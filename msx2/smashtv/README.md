# Smash T.V. para MSX2: esqueleto técnico

ROM de 16 KB (`smashtv.rom`, carregada em `$4000`), em assembly Z80. É um **esqueleto para validar a
arquitetura de vídeo e o laço de jogo**, com arte provisória desenhada em ASCII (`tools/msx/gen_gfx.py`).
Nenhum código nem gráfico do jogo original foi usado aqui.

## Como rodar no openMSX

```bash
openmsx -machine C-BIOS_MSX2 -cart msx2/smashtv/smashtv.rom
# ou, com BIOS reais:
openmsx -machine Panasonic_FS-A1WSX -cart msx2/smashtv/smashtv.rom
```

| ação | controle |
|---|---|
| mover | setas ou joystick 1 |
| mirar/atirar (twin-stick) | W A S D ou joystick 2 (atira sempre que há direção de mira) |
| atirar na última direção | barra de espaço ou botão A do joystick 1 |

Matar grunts dá 10 pontos. Encostar num grunt custa uma vida (2 s de invulnerabilidade, sprite piscando);
sem vidas o jogo reinicia. Apertando **F1**, a cor da borda mostra onde vai o tempo de cada tick (desligado por padrão): verde = lógica,
vermelho escuro = desenho, preta = espera. Se o vermelho dominar, o desenho estourou o tick.

## Build e testes

```bash
pip install z80 z80dis pillow
tools/msx/build.sh          # gera gfx.inc, monta a ROM e roda os testes no simulador
```

- `tools/msx/z80asm.py`: assembler Z80 em Python (sintaxe compatível com sjasmplus), gerado da tabela do
  `z80dis` e validado com um round-trip das 800 formas de instrução (`z80asm.py selftest`).
- `tools/msx/msxsim.py`: simulador mínimo de MSX2 (Z80, BIOS simulada, V9938: registradores, VRAM,
  paleta, Screen 5, HMMV/HMMM/LMMM/LMMV, sprites modo 2, IRQ de VBlank).
- `tools/msx/test_smashtv.py`: testes de comportamento. O mais importante compara, a cada tick, a tela
  com "página limpa + inimigos nas posições desenhadas" e confere o HUD pixel a pixel.

**O simulador é a minha leitura da documentação do V9938, não um emulador exato.** Passar nos testes
mostra que a lógica e o uso do VDP são coerentes, mas não substitui o openMSX. O tempo dos comandos do VDP
é uma estimativa grosseira.

## Arquitetura

- **Screen 5** (256×212, 16 cores). Página 0 = tela; página 1 (`$8000`) = folha de gráficos (fonte e
  quadros dos inimigos); página 2 (linha 512) = cópia limpa da arena, usada para apagar objetos.
- **Sprites de hardware (modo 2, 16×16)**: jogador (plano 0) e 16 balas (planos 1–16). Atenção ao limite de
  8 sprites por linha: uma rajada longa na horizontal perde balas.
- **Inimigos por blit**: `HMMM` restaura a área antiga a partir da página limpa e `LMMM`+TIMP desenha o
  quadro novo (cor 0 transparente). Primeiro apaga todos, depois desenha todos, para não apagar um vizinho.
- **Laço a VBlank/3** (≈20 Hz NTSC), como o original, usando o `JIFFY` da BIOS.
- **Dados como no original**: pools de 16 slots em *struct of arrays* com passo `$10` (`IX+0` tipo,
  `IX+0x20` X, ...), direções em bits (1 cima, 2 baixo, 4 esquerda, 8 direita), pontuação BCD de 3 bytes,
  RNG com `LD A,R`.
- Só usa a BIOS para `CHGMOD`, `WRTVDP`, `GTSTCK`, `GTTRIG` e `SNSMAT`. O resto é acesso direto ao VDP.

## Desempenho (simulado, 16 inimigos na tela)

Com 0, 8 e 16 inimigos o desenho custa cerca de 10 mil, 31 mil e 54 mil T-states por tick, para um
orçamento de 179 mil (3 VBlanks). Cresce de forma linear (2 comandos de VDP por inimigo). Isso depende da
velocidade real dos comandos, que precisa ser medida no openMSX.

## Próximos passos

1. Conferir no openMSX (cores, sprites, limite de 8 por linha, velocidade dos comandos).
2. Trocar para **dois buffers** (alternar `R#2` entre páginas) para evitar o *tearing* do desenho direto.
3. Máquina de script das salas (ondas) e os outros tipos de inimigo, a partir de `tools/sms/NOTES.md`.
4. Gráficos reais e driver de som (PSG/SCC).
