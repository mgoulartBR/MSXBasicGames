# Toolchain (fixada)

```
MSXgl:    v1.5.0  (tag; commit be3278424ec6a1dcd23563564c857e93aaec090d)  [ultimo release estavel em 2026-10-06]
SDCC:     4.6.0 #16555 (Linux)   [binario embutido em external/msxgl/tools/sdcc]
openMSX:  19.1 (pacote Ubuntu noble)  [estavel upstream atual: 21.0; nao instalado - ver nota]
C-BIOS:   pacote openmsx-data (maquina C-BIOS_MSX2)
Node:     v22 (usado pelo build tool do MSXgl)
Host:     Linux x86_64 (container de sandbox)
Build date: 2026-10-06
```

Notas:
- openMSX 19.1 foi usado por vir do apt; basta para testes headless. Se algum bug de emulacao
  aparecer, compilar o openMSX 21.0 (tag RELEASE_21_0) antes de culpar a ROM.
- Nenhum patch local em MSXgl/SDCC.
- O prompt-mestre citava MSXgl 1.4.1 como referencia; a consulta ao repositorio oficial mostrou v1.5.0.
- Testes feitos apenas em emulador (C-BIOS MSX2). **Nada testado em hardware real.**

## Comandos
```
scripts/setup.sh              # clona MSXgl v1.5.0 em external/, verifica SDCC/openMSX
scripts/build.sh [clean]      # compila e copia ROM para dist/
scripts/run.sh [rom]          # abre no openMSX
scripts/run.sh rom --shot f.png --wait 8   # headless + screenshot
```
