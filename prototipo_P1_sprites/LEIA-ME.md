# Protótipo: V9990 em modo P1 (tiles + sprites)

ROM de teste (`PROTOP1.rom`, 32 KiB), separado do jogo. Mostra um mapa de tiles 64x64 rolando por registrador, 13 atores feitos de sprites de hardware (4 de 32x32 = 4 sprites cada, 9 de 48x48 = 9 sprites cada, 97 sprites no total) e um envio de pose por quadro.

Rodar para ver:

```bash
openmsx -machine C-BIOS_MSX2 -ext gfx9000 -cart PROTOP1.rom -script ver.tcl
```

`ver.tcl` deve conter `set videosource GFX9000`. Sem isso o openMSX mostra a tela do MSX2, não a do V9990.

Medição: `medir.tcl` (variável de ambiente `MODE`, endereços fixos desta ROM). Tudo a 3,58 MHz, 60 Hz, openMSX 19.1 com C-BIOS MSX2.

## Resultados (ms por iteração da operação, medidos)

| Modo | Operação | ms |
|---|---|---|
| 7 | só registradores de scroll (C do MSXgl) | 0,40 |
| 5 | gravar 388 bytes de atributos de sprite | 2,85 |
| 6 | montar atributos de 97 sprites (C simples) | 17,4 |
| 4 | scroll + mover 13 atores + montar + gravar atributos | 25,6 |
| 8 | enviar pose 48x48 pela CPU (rotina da biblioteca) | 34,8 |
| 9 | enviar pose 48x48 pela CPU (OUTI em Z80) | 20,3 |
| 10 | enviar pose 32x32 pela CPU (OUTI em Z80) | 9,3 |
| 13 | copiar pose 48x48 com o blitter (LMMM) e esperar | 4,2 |
| 14 | copiar pose 32x32 com o blitter (LMMM) e esperar | 2,4 |
| 0 | tudo junto, envio de pose pela CPU (rotinas em C) | 55,6 |

## O que o protótipo confirma

- Os sprites e o scroll em P1 funcionam no openMSX; a pose fica contínua entre sprites vizinhos.
- A memória de padrões de sprite é um bitmap de 256 px de largura (128 bytes por linha). Os comandos do blitter funcionam em P1 com esse endereçamento: uma pose inteira é **uma** cópia LMMM, se os sprites do ator usarem padrões vizinhos na grade (`base + sx + 16*sy`). Verificado lendo a VRAM de volta.
- Com muitos sprites alinhados na mesma linha, os últimos somem (captura `captura_estresse.png`). O corte observado é compatível com cerca de 16 sprites por linha. Não medi o número exato.

## Limites do protótipo

- O C usado aqui para mover e montar atributos é lento de propósito; num jogo real só os atores que se movem seriam atualizados, em Z80.
- Não há Fog, HUD, seleção, setas nem sobreposição de copas de árvore.
- A estimativa de quadro completo do jogo em P1 é uma extrapolação, não uma medição.
