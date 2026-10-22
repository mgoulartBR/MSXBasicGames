# Street Rod MSX2 — código-fonte

Conteúdo: código do port (C/SDCC + Z80), geradores de assets em Python, scripts de build e de teste (openMSX/Tcl), ferramentas de engenharia reversa do formato do original e a documentação.
**Os arquivos do jogo original (SRSE.EXE, HOT_DATA, LIB1, LIB2) não estão incluídos**: o build precisa deles na sua máquina.

## Requisitos
`sdcc` (com `sdasz80` e `sdld`), `python3` + `numpy` + `Pillow` (+ `librosa` só para transcrever músicas novas), `openmsx` + `xvfb` (opcionais, para testar).

## Como gerar a ROM
```
cd streetrod-port/msx2/road
SR=/caminho/para/os/arquivos/originais ./fullbuild.sh        # LIB2, HOT_DATA e SRSE.EXE dentro de $SR
grep -c "DEFAULT CAR" textdata.bin                            # tem de ser 0
# resultado: road.rom (512 KB, mapper ASCII16)
```
Flags de teste (`-D...`) são repassadas: `./fullbuild.sh -DNO_NAME -DNO_PAC -DSTART_CAR=16`.

## Documentação
- `PORTING_GUIDE.md` — guia completo do port (engenharia reversa, arquitetura, assets, regras, lições, plano do jogo futuro).
- `streetrod-port/NOTES.md`, `GAME_DESIGN.md`, `msx2/road/README.md` — notas e diário técnico.
