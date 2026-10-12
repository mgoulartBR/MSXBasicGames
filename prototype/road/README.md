# Protótipo de pista pseudo-3D — MSX2+ (V9958), Z80 puro

Mede o que o V9958 aguenta para uma pista "por bandas" (sem scroll por linha).
**Dados 100% sintéticos e originais** (`gen_tables.py`); nada vem da ROM da Sega.

![screenshot](screenshot.png)

## Técnica
SCREEN 5 (256×212). Textura da pista em VRAM (página 3): 112 linhas × 2 variantes
de listra, desenhadas no boot com `LMMV`. Por quadro, 56 bandas de 2 linhas são
copiadas com `HMMM` para a página oculta (páginas 0/1 em duplo buffer); a curva
é um deslocamento X por banda (2 `HMMM` quando há curva). A fase das listras
vem de uma tabela 1/profundidade; o deslocamento de curva é `curva × q[banda]`.

## Como rodar
```sh
sudo apt install pasmo openmsx          # C-BIOS MSX2+ já vem com o openMSX
./build.sh 1 0x0180 road.rom            # args: vsync(0|1) velocidade saida.rom [tela_desligada]
openmsx -machine C-BIOS_MSX2+ -cart road.rom
```
Contador de quadros em `$C100` (word); o script `bench.tcl` mede quadros por
segundo em tempo **emulado** (`peek16 0xC100` entre t=8 s e t=18 s, pois a C-BIOS
leva ~3 s para iniciar). Para screenshots use `set frameskip 0` e capture após ~18 s.

## Resultados (openMSX 19.1, C-BIOS MSX2+; **medido em emulador, não em hardware real**)

| Teste | fps | Taxa de cópia/fill |
|---|---|---|
| Renderizador completo, 56 bandas, sem vsync | **11,8** | — |
| idem, tela desligada | 12,4 | — |
| 1× `HMMM` 256×112 (14 336 B) por quadro | 15,4 | ≈ 220 KB/s |
| 56× `HMMM` 256×2 (mesma área) | 14,0 | ≈ 200 KB/s |
| 1× `HMMV` 256×112 (fill por byte) | 25,0 | ≈ 360 KB/s |
| 1× `LMMV` 256×112 (fill lógico) | 6,3 | ≈ 90 KB/s |

## Conclusões
1. **O gargalo é a banda do VDP, não o Z80.** Copiar 14 KB/quadro custa ~65 ms.
   O custo por comando é pequeno (~5 ms por 56 comandos) e desligar a tela quase
   não ajuda. O Z80 gasta bem menos que o VDP e roda em paralelo.
2. O turbo R **não** acelera o VDP; só dá folga de CPU.
3. Para 30 fps o orçamento é ~7 KB copiados por quadro (metade da pista atual).
   `LMMV` deve ser evitado; `HMMV` é ~1,6× mais rápido que `HMMM`.

## Caminhos para ganhar desempenho (estimativas, a validar)
- **Listras por ciclo de paleta**: a textura carrega N tons de asfalto/grama e a
  animação é só girar a paleta (16 cores, custo de VDP ≈ 0).
- **Curva por zonas com R#27 + interrupção de linha (R#19)**: scroll horizontal
  por zona (8–14 zonas), sem copiar nada; curva "em degraus", barata em VDP, mas
  usa CPU nas interrupções.
- **Copiar só a janela suja** (asfalto + zebra + margem), e **reduzir a altura**
  da pista (ex.: 80–96 linhas).
- Alvo realista: 30 fps em retas e 15–20 fps copiando em curvas, ou 30 fps
  constante com o esquema paleta + zonas.
