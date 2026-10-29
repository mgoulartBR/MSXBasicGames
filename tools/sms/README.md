# Ferramentas de engenharia reversa: Smash T.V. (Master System)

Ferramentas para estudar a ROM de Master System e portar a lógica para MSX2.
**Não** versione a ROM nem os arquivos gerados (`out/`, `*.sms`, `*.vram`...): são
material com copyright. O `.gitignore` já cobre isso.

## Fluxo

```bash
pip install z80 z80dis pillow
ROM="Super Smash T.V. (Europe).sms"

# 1. Trace dinâmico: roda a ROM headless (mapper Sega + VDP mínimo + joypad aleatório)
#    e grava cada (banco, pc) executado. Use várias seeds para ampliar a cobertura.
python3 tools/sms/trace.py "$ROM" out/t1.json 12000 1 [snapshot_a_cada_N_frames]

# 2. Disassembly recursivo por banco, semeado com os traces (o banco do slot 2
#    é escolhido por dados em tempo de execução, então só análise estática não basta)
python3 tools/sms/disasm.py "$ROM" out/asm out/t*.json     # -> out/asm/bankNN.asm, coverage.txt

# 3. Gráficos: sheets de tiles (paleta BG e sprite) e screenshot a partir do dump de VRAM
python3 tools/sms/render.py out/t1 out/png
```

## O que se sabe da ROM

- 256 KB, 16 bancos de 16 KB, mapper Sega (`$FFFC-$FFFF`). Slot 1 fixo no banco 1.
- Todo chaveamento do slot 2 passa por `$170C` (`LD ($FFFF),A / RET`); o valor vem de
  campos de objeto (`IX+0x12`), por isso o trace dinâmico é necessário.
- Reset em `$0000` -> `$15D5`. A RAM inicial (`$C000`, `$11B4` bytes) é copiada de `$0400`.
- Código está nos bancos 0, 1 e 2. Os bancos 3-F são dados (gráficos, música, níveis).
- ISR em `$160B` (VBlank). NMI em `$0066` só faz pausa.
