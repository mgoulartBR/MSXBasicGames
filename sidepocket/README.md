# Side Pocket (MSX2 / MSXGL)

Jogo de sinuca para **MSX2** (Screen 5, 256x212, ROM de 32 KB) baseado na versão **Mega Drive** de *Side Pocket*.
O código é novo, em C; os gráficos da mesa e das bolas são extraídos **da sua própria ROM** na hora do build.

> Nada derivado da ROM está neste repositório: `msx/gen/` e `msx/out/` ficam no `.gitignore`.
> O ROM de MSX2 gerado contém os gráficos originais (Data East): use apenas para estudo pessoal e não o distribua.

## Como montar
```
tools/build_msx.sh "Side Pocket (USA).md" <Genesis-Plus-GX> <MSXgl>
```
Pré-requisitos: `gcc`, `python3` + Pillow, `node`, um checkout do [Genesis Plus GX](https://github.com/ekeeke/Genesis-Plus-GX)
e um do [MSXgl](https://github.com/aoineko-fr/MSXgl). O script:
1. compila o núcleo do emulador e roda a ROM até a mesa (`tools/emu/reach_gameplay.txt`), despejando VRAM/CRAM;
2. recorta a mesa (256x166) e as bolas, e faz uma paleta conjunta de 16 cores (3 bits por canal, igual ao V9938);
3. gera arrays C (mesa comprimida em RLE: 21 KB -> 4,9 KB);
4. compila o jogo com o MSXGL e gera `msx/out/sidepocket.rom`.

Para testar sem MSX: `openmsx -machine C-BIOS_MSX2 -cart msx/out/sidepocket.rom`.

## Controles
Setas ou joystick: girar a mira. **Espaço** (ou botão A): segurar para carregar a força, soltar para tacar.
Pontos: 100 por bola encaçapada, -50 se a branca cair (ela volta ao ponto de partida). Sem bolas na mesa: novo rack.

## Layout
- `src/sp_logic.[ch]`: física (portátil, sem dependência de plataforma; testada no PC com `make test`).
- `msx/sidepocket.c`: tela, entrada, desenho. Página 0 = tela; página 1 = cópia limpa da mesa (para apagar) e células das bolas.
- `tools/`: extração dos gráficos, emulador-sonda (`tools/emu`), notas de engenharia reversa em `docs/spec.md`.

## Desempenho (MSX2 a 3,58 MHz)
Física em ponto fixo 8.8 sem divisões, e a varredura de vizinhança escrita em assembly Z80 inline. O desenho só toca as bolas que
mudaram de pixel: sobre feltro liso usa `HMMM` (~0,3 ms por bola, com os cantos já preenchidos com a cor do feltro); perto das tabelas
ou de outra bola usa `LMMM` com transparência (3-6 ms). A quebra do rack, com as 10 bolas em movimento, ainda roda abaixo de 60 fps:
a simulação acompanha o relógio do BIOS (até 3 passos por quadro), então o jogo não fica em câmera lenta, só menos fluido naquele instante.

## Pendências
Regras do Pocket Game (ordem 1-9, bônus), fases, modo 2 jogadores, sons, atrito/efeito calibrados com a ROM (veja `docs/spec.md`).
