# Validação da KAW9990P1.ROM em outras máquinas

Ambiente de teste: openMSX 19.1 (único emulador com V9990 disponível no ambiente de build) e as BIOS C-BIOS 0.29a. Não havia BIOS reais.

## O que foi validado (`tests/machines_smoke.sh`)

| Máquina (openMSX) | perf012 | camera011 | combat011 | flashseq_p1 | box011 (mouse) | fps (4 unidades) |
|---|---|---|---|---|---|---|
| C-BIOS_MSX2 | ok | ok | ok | ok | ok | 29,7 |
| C-BIOS_MSX2_JP | ok | ok | ok | ok | ok | 29,7 |
| C-BIOS_MSX2_BR | ok | ok | ok | ok | ok | 29,7 |
| C-BIOS_MSX2+ | ok | ok | ok | ok | ok | 29,7 |
| C-BIOS_MSX2_EU (PAL) | ok | ok | ok | ok | ok | 28,7 |
| C-BIOS_MSX2+_EU (PAL) | ok | ok | ok | ok | ok | 28,7 |

- Nas máquinas PAL a simulação segue a 30 Hz (60 ticks em 2 s): o jogo força o VDP a 60 Hz. Só o desenho fica ≈1 fps abaixo.
- As BIOS EU do pacote `cbios` precisam ser copiadas para `~/.openMSX/share/systemroms` (o openMSX do Ubuntu não as instala).
- Sem V9990 no barramento a ROM mostra "NEEDS A V9990 (GFX9000). NONE FOUND ON THIS MSX." na tela de texto e para (antes: tela azul vazia).

## O que continua sem validação

- **MSX com BIOS real e GFX9000 físico.** A ROM escreve `0xFF` em `SCNCNT` (`F3F6h`, contador de varredura de teclado da BIOS) a cada vertical blank para a BIOS não varrer o teclado (≈4% de CPU). O C-BIOS usa esse byte (3 referências na ROM principal) e o efeito apareceu no perfil. Em BIOS real a variável é documentada com o mesmo significado, mas não foi testado. Se algo de teclado se comportar de forma estranha, remover a linha `*((volatile u8*)0xF3F6)=255;` de `irq_p1` em `src/video.h`.
- **Outros emuladores com V9990** (blueMSX com GFX9000, MAME `msx2` + `gfx9000`): não instalados no ambiente de build, sem BIOS proprietária para MAME.
- **Mouse real** (porta 1 e 2) e tempo de carga em V9990 físico.
