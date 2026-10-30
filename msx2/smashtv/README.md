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

São 4 salas, cada uma com seu script de ondas (`rooms.inc`): grunts (verdes), runners (vermelhos, rápidos) e
tanks (roxos, 3 HP). Limpe a sala para a porta amarela abrir na parede de cima e entre nela para ir à próxima;
depois da quarta, volta à primeira. O número da sala aparece no meio do HUD. Encostar num inimigo custa uma
vida (2 s de invulnerabilidade, sprite piscando); sem vidas o jogo reinicia. Apertando **F1**, a cor da borda mostra onde vai o tempo de cada tick (desligado por padrão): verde = lógica,
vermelho escuro = desenho, preta = espera. Se o vermelho dominar, o desenho estourou o tick.

## Armas e itens (`items.inc`)

Quatro armas, que duram 15 s (o HUD mostra o número da arma e os segundos) e depois voltam para a pistola:

| arma | efeito |
|---|---|
| 1 pistola | um tiro a cada 2 ticks |
| 2 spread | três tiros em leque (a direção da mira e as duas vizinhas), a cada 3 ticks |
| 3 rápida | um tiro por tick |
| 4 perfurante | dano 2, atravessa os inimigos que mata, a cada 3 ticks |

Itens no chão (sprites de hardware, 6 ao mesmo tempo, pisca antes de sumir): as três armas, **vida extra** (máximo 9),
**escudo** (5 s de invulnerabilidade), **bomba** (mata todos os inimigos da tela, com flash branco no chão) e
**prêmio** (+100 pontos). Os inimigos soltam itens ao morrer (grunt ~12 %, runner ~18 %, tank quase sempre) e o
script das salas pode colocá-los com `DROP`. Cada inimigo vale 10, 20 e 50 pontos.

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
  com "página limpa + inimigos nas posições desenhadas" e confere o HUD pixel a pixel. Outro teste garante que a página exibida
  nunca muda enquanto está na tela (sem *tearing*) e que sprites e página trocam juntos, logo após o início do VBlank.

**O simulador é a minha leitura da documentação do V9938, não um emulador exato.** Passar nos testes
mostra que a lógica e o uso do VDP são coerentes, mas não substitui o openMSX. O tempo dos comandos do VDP
é uma estimativa grosseira.

## Arquitetura

- **Screen 5** (256×212, 16 cores), **duplo buffer**: páginas 0 e 1 são os dois buffers de exibição;
  página 2 (linha 512) = cópia limpa da arena, usada para apagar objetos; página 3 (linha 768) = folha de
  gráficos (fonte e quadros dos inimigos). As tabelas de sprites ficam nas linhas livres da página 0 e não mudam.
- **Sprites de hardware (modo 2, 16×16)**: jogador (plano 0) e 16 balas (planos 1–16). Atenção ao limite de
  8 sprites por linha: uma rajada longa na horizontal perde balas.
- **Inimigos por blit**, sempre na página escondida: `HMMM` restaura a área antiga a partir da página limpa
  e `LMMM`+TIMP desenha o quadro novo (cor 0 transparente). Primeiro apaga todos, depois desenha todos, para
  não apagar um vizinho. Cada buffer guarda as últimas posições desenhadas (`E_OX/E_OY` e `E_OXB/E_OYB`);
  inimigos mortos são apagados nos dois buffers (tipos 2 e 3) antes de liberar o slot. O HUD é redesenhado
  nos dois buffers (`score_dirty` conta 2).
- **Troca de página sem tearing** (`flip`): termina o desenho, espera o VBlank, escreve `R#2` e envia a
  tabela de sprites no mesmo VBlank, para fundo e sprites mudarem juntos.
- **Armas e itens**: os tiros têm tipo (normal/perfurante) e dano próprio; os itens usam os planos de sprite 17–22,
  com a tabela de cores reconstruída só quando um item aparece ou some e enviada no mesmo VBlank da troca de página.
- **Máquina de script das salas** (`script.inc`, `rooms.inc`, documentada em [`SCRIPT.md`](SCRIPT.md)): ondas,
  laços, sub-scripts, escolha aleatória, espera até limpar, porta e progressão de salas.
- **Laço a VBlank/3** (≈20 Hz NTSC), como o original, usando o `JIFFY` da BIOS.
- **Dados como no original**: pools de 16 slots em *struct of arrays* com passo `$10` (`IX+0` tipo,
  `IX+0x20` X, ...), direções em bits (1 cima, 2 baixo, 4 esquerda, 8 direita), pontuação BCD de 3 bytes,
  RNG com `LD A,R`.
- Só usa a BIOS para `CHGMOD`, `WRTVDP`, `GTSTCK`, `GTTRIG` e `SNSMAT`. O resto é acesso direto ao VDP.

## Desempenho (simulado, 16 inimigos na tela)

Com 0, 8 e 16 inimigos o desenho custa cerca de 8 mil, 28 mil e 51 mil T-states por tick, para um
orçamento de 179 mil (3 VBlanks), com uma troca de página por tick. Cresce de forma linear (2 comandos de VDP por inimigo). Isso depende da
velocidade real dos comandos, que precisa ser medida no openMSX.

## Próximos passos

1. Conferir no openMSX/WebMSX (cores, sprites, limite de 8 por linha, velocidade dos comandos).
2. Mais tipos de inimigo, e salas no estilo do original (a partir de `tools/sms/NOTES.md`).
3. Gráficos reais e driver de som (PSG/SCC).
