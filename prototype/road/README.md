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


---

# Versão 2: paleta + zonas R#27 (`road_zones.asm`)

![screenshot](screenshot_zones.png)

Resolve o gargalo medido acima: **nenhum comando de VDP por quadro**.

- **Textura estática**: a pista é desenhada uma só vez no boot.
- **Listras por ciclo de paleta**: 4 subfaixas por período; as entradas 4–15 da
  paleta (asfalto, zebra, faixa central) giram em 4 fases, reescritas no VBlank
  (24 bytes via `OTIR`). As listras avançam em direção à câmera.
- **Curva por zonas**: 17 zonas (4, 8 e 16 linhas; mais finas perto do horizonte).
  Uma interrupção de linha (R#19) por zona escreve **R#26/R#27** (scroll
  horizontal); `R#27 = s & 7`, `R#26 = ((7 - s) >> 3) & 31`. O scroll com
  *wrap* mostra grama dos dois lados, então a emenda é invisível. O fino
  scroll mascara à esquerda com a cor de fundo (R#7), por isso R#7 = grama.
- **IM2** (tabela de 257 bytes iguais + stub `JP` em RAM): trata VBlank e
  interrupção de linha sem passar pela BIOS (que não conhece o flag de linha).

```sh
./build_zones.sh 0x00A0 0 0 -1 zones.rom     # velocidade paldir forcecurve iladj saida
openmsx -machine C-BIOS_MSX2+ -cart zones.rom
openmsx -machine C-BIOS_MSX2+ -cart zones.rom -script zbench.tcl   # fps e folga de CPU
```

## Resultados (openMSX 19.1, C-BIOS MSX2+; **emulador, não hardware real**)

| Versão | fps | CPU livre |
|---|---|---|
| 1: 56 bandas com `HMMM` | 11,8 | — |
| 2: paleta + zonas R#27 | **60** (travado no VBlank) | **≈ 61 %** |

A folga é medida por um contador de voltas ociosas no laço principal, depois de
descontar interrupções e o cálculo por quadro (17 multiplicações + escrita das zonas).

## Limitações conhecidas / próximos passos
- Curva "em degraus" (visível perto do horizonte); dá para afinar com mais zonas
  (cada interrupção custa ~170 T).
- A animação das listras tem só 4 fases (8 exigiria mais entradas de paleta).
- Faixas mais finas que 1 linha (longe) usam um padrão `r % 4` (aspecto de névoa).
- **Relevo (subidas/descidas) ainda não implementado**: exigiria deslocar o horizonte
  e/ou escolher outra linha da textura por zona.
- Sprites (carros), HUD e colisão ainda não existem; o HUD pode ocupar linhas
  sem zonas.
- Falta validar em hardware real (tempo exato das interrupções de linha e IM2).
