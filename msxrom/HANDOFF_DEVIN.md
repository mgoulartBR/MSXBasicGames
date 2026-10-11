# Áudio MoonSound opcional

Driver em `src/e1m1_sound.h`, conversor em `tools/e1m1_sound.py`, integrados ao build.
Dez efeitos PCM8 11025 Hz, 105.680 bytes de sample RAM e sete bancos de ROM privados.
Oito vozes circulares; TL=20 (−15 dB), PCM mix=2 (−6 dB); wave IDs 384–393,
header bank=4 em 0x200000, pitch OCT=−1/FN=0. Término em 16 amostras de zero.
Detecção do ID OPL4 e probe de RAM no fim dos primeiros 128 KiB; BUSY/LD com timeout.
PSG fallback máx. 6/15, três quadros; continua baseado no FPS. Reset amortece as vozes.
`setup_e1m1_moonsound.py` instala XML RAM-only, sem ROM Yamaha redistribuída.
Testes: `test_e1m1_sound.py` e `E1M1_AUDIO=1` no teste openMSX; ausência da expansão
com `E1M1_EXPECT_MOON=0`. WAVs são saída real do emulador. Não há música/posição
estéreo, MSX-AUDIO ou streaming turbo R. FPGA/hardware seguem UNTESTED.

# HANDOFF — MSX DOOM E1M1 (V9968 + Geo3D)

Documento de passagem de tarefa. Resume **tudo** o que foi pedido, decidido, construído, medido, quebrado e corrigido nesta conversa, o estado atual do código, as restrições do usuário e o que falta fazer. Escrito em português porque o usuário trabalha em português.

> **Leia primeiro a seção 2 (restrições do usuário) e a seção 12 (o que está em aberto).**
> **Retomada posterior — HUD/animações/barris:** o estado atual está na seção correspondente do README.
> Rosto usa offsets Doom; números usam a própria largura; limpeza da munição não invade saúde.
> HUD ocupa 256×34, escala X=0,8/Y=34/32; pistola realocada na região livre 640–767.
> Atlas agora tem 58 patches em 256×128 com proporções/offsets preservados, incluindo caminhada,
> ataque, dor, cinco poses de queda por inimigo e cinco de explosão. Barris de 20 HP recebem tiros,
> aplicam dano radial com LOS/armadura, detonam outros barris e desaparecem. Cadáveres persistem.
> As referências abaixo a seis poses, células fixas, barris indestrutíveis e marcadores de boot são históricas.
> O teste openMSX foi corrigido para filtrar o latch do VDP e observar a troca de página sem polling tardio.
> Não comparar seus FPS com os históricos sem repetir a medição de ambas as ROMs pelo contador corrigido.
> Não há animação em oito direções, morte extrema ou correção nova de glitches geométricos.
> **Retomada posterior — teto:** máscara preta pós-Geo3D removida após as capturas X508 Y-3230 A2, X1615 Y-2511 A2 e X1765 Y-2540 A320.
> Fundo na cor do teto do setor atual; até quatro planos residentes na altura/cor dos setores, usando somente espaço livre do lote (nenhuma parede/piso/UV removido).
> Os 1232 lotes passaram no teste de preservação, com 2146 planos adicionados. BUILD OK, 14/14 SIMULATED, TAB SIMULATED e OPENMSX TESTED passaram.
> Comparação A/B no mesmo fork/perfil R800: início 9,22→10,89 FPS; posições das capturas 8,88→11,98, 7,92→10,27 e 9,22→11,98 (TAB desligado).
> Teto **liso, aproximado e sem textura**: near/oclusão continuam incompletos; em lacunas pode aparecer geometria distante acima do teto. Não é uma solução integral de visplanes.
> As descrições da máscara v11 abaixo são históricas; veja a seção “Revisão visual do teto” do README para o comportamento atual.
> Rótulos de validação usados neste projeto: **BUILD OK** (compila), **SIMULATED** (rodou no Z80 emulado do nosso harness com o modelo de referência do Geo3D), **USER-REPORTED** (o usuário viu no openMSX e relatou), **UNTESTED** (nunca executado). Nada aqui foi testado em FPGA ou hardware real.

---

## 1. Visão geral e objetivo

- Projeto: portar o **DOOM** para **MSX** usando o **V9968** (VDP estendido, SCREEN 8 com paleta EPAL de 256 cores) e o coprocessador **Geo3D** (faces 3D com textura via LRMM).
- Repositório: `mgoulartbr/MSXBasicGames`, branch de trabalho `claude/gracious-johnson-2x8fal`. Código novo está em `msxrom/`. O protótipo web inicial está em `msxdoom/`.
- Evolução do pedido do usuário:
  1. Protótipo web em Three.js (FPS retrô) → feito (`msxdoom/`).
  2. ROM real para MSX (turbo R / R800 ou Z80 com C-BIOS), com paredes texturizadas pelo Geo3D, inimigos billboard, HUD → feito.
  3. Fase original "HANGAR" (26×26) → feita, pública, em `msxrom/` (`out/msxdoom.rom`).
  4. Pedido atual e principal: **reprodução fiel do E1M1 do DOOM** (geometria, texturas, inimigos, formato do mapa) a partir do `doom1.wad` **do próprio usuário**.
- O usuário compara o resultado a um vídeo de um jogo "polido" e ao DOOM de PC. Cobra fidelidade visual e velocidade.

## 2. Restrições e preferências do usuário (inegociáveis)

1. **Não versionar o WAD, os dados convertidos nem a ROM do E1M1.** Eles ficam em `msxrom/private/` (já no `.gitignore`). Só o código do conversor/motor vai para o git. A ROM é de uso pessoal; é entregue ao usuário como arquivo anexo, nunca commitada.
2. **Não abrir Pull Request** a menos que o usuário peça explicitamente. Commits vão direto na branch `claude/gracious-johnson-2x8fal` e são empurrados com `git push -u origin <branch>`.
3. Trailers nos commits: `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>` e `Claude-Session: https://claude.ai/code/session_012mJU7gytMLKfLk34iWyQaW`.
4. **Honestidade sobre validação:** sempre declarar BUILD OK / SIMULATED / USER-REPORTED / UNTESTED. Nunca afirmar que algo funciona no openMSX sem relato do usuário.
5. Não deixar arquivos não rastreados no repositório (o ambiente tem um *stop hook* que reclama). Limpar ou commitar.
6. Pronomes: usar "o usuário" / neutro; não presumir gênero.
7. Preferências declaradas do usuário: Estatística, Ciência de dados, Python, R, IA (contexto, não afeta o código).
8. Idioma: responder em português do Brasil.

## 3. Cronologia resumida

| Etapa | O que aconteceu |
|---|---|
| Início | Usuário enviou `DOOM-master.zip` e pediu protótipo web + port para MSX com V9968/Geo3D (projetos do GitHub, não pastas locais). Enviou `MSX_V9968_Geo3D_REFERENCIAS_1.md`. |
| ROM esqueleto | "Sim, quero!" → ROM ASCII16 em SDCC. |
| ROM privada | "Gere uma ROM privada para mim" → ROMs enviadas como anexo. |
| Comparação | Usuário recebeu `wolf3d-engenharia-reversa.md` de alguém cujo jogo ficou "perfeito" → decidimos começar pela ROM com texturas. |
| Lentidão | "Tá muito lento… tem machine sem turbo R?" → máquina Z80 com C-BIOS. |
| Tela preta | openMSX abriu preto: faltava Geo3D. Desenvolvedor do Geo3D confirmou que era preciso usar os `.asm` do repositório `alexmoncks/V9968_Cartridge/geo3d`. Adotamos o modelo de referência dele (`geo3d/sim/gen_scenes.py`). |
| Otimização | Várias rodadas (PVS por bloco, uma só RUN, LOS em cache). Usuário: "MELHOROU MUITO!". |
| Máquinas | Usuário rodou na `C-BIOS_R800_V9968_Geo3D`; pediu `C-BIOS_Z80_V9968_Geo3D` → criamos o XML (UNTESTED na origem; depois funcionou após ajustes de ambiente). |
| E1M1 | Pediu recriar o E1M1; escolheu "fase original inspirada"; depois reprovou ("não parece nem um pouco E1M1") e pediu **fidelidade**; enviou `doom1.wad`. |
| Conversor | `wad.py`, `e1m1*.py`, motor `src/e1m1.c`, builds privados, testes. |
| Iterações de qualidade | Paredes faltando → PVS por direção; piso/teto transparentes → várias abordagens; lentidão; vazamento de textura por cima do teto. |
| Entrega Linux Mint | Zip com ROM + scripts; resolvemos `tcl-dev`, fork do openMSX, `make install`, etc. |
| Estado final | Teto preto sólido em tempo de execução (v11). Pedido da tecla de coordenadas (debug) **em andamento**. |

## 4. Hardware, restrições e fatos técnicos verificados

### 4.1 V9968 / SCREEN 8
- 256 KiB de VRAM = 1024 linhas × 256 bytes. Páginas de exibição: linhas 0–511 via `R#2 = 0x1F | (page<<5)`; cada página tem 256 linhas, só 212 visíveis. Linhas 512–1023 são área de texturas/dados.
- Paleta EPAL: `R#20` bit4; índice em `R#16`; porta `0x9A` recebe R,G,B (5 bits cada). `Port#4 (0x9C)` escrever 0 destrava `R#20/R#21`; `R#21 = 0` = modo V9968; `R#20 = 0x11` (EPAL + comandos em alta velocidade).
- Comandos usados: `HMMV 0xC0` (preencher), `LMMM 0x98` (blit com TIMP, índice 0 transparente), `LMMM 0x90` (cópia simples), `LRMM 0x30|lop` (cópia com passo 8.8).
- **LRMM** (de `vdp_command.v` do cartucho): ordem de escrita `R#32..R#45`, depois `R#47..R#50` (VX, VY), e por fim `R#46` (inicia). `NY>1` avança a origem a cada linha por `(-VY, +VX)`. O endereço de leitura em SCREEN 8 é `{sy[17:8], sx[15:8]}` → X dá wrap módulo 256 e Y módulo 1024, **mas** a *janela de origem* (`R#51..58`) substitui o pixel pela cor `CLR` fora dela. Por isso o código configura janela X 0..511, Y 0..1023. `SX/SY` **não têm parte fracionária** (só texels inteiros).

### 4.2 Geo3D (portas 0x9D índice/status, 0x9F dados)
- Vértices/faces/UVs: no máximo **255 de cada por RUN** (RAM do Geo3D). A ROM reserva espaço para sprites: `MAX_SPR=10` → `MAX_FACES=245`, `MAX_VERTS=215`.
- Face é **descartada** se algum vértice tiver `z < ZNEAR` (ZNEAR=2) ou saturar a projeção (|x|/z ≥ ~204). Culling traseiro por área projetada (não usa normal). Ordenação painter por **soma dos z** dos vértices (sem Z-buffer por pixel!).
- Textura: `TEXX/TEXY/TSTRIDE` (TSTRIDE é **8 bits**, máx 255). Face é texturizada se o byte `base` tem bit7 = 1; senão é cor lisa = `base + nível`. Nível de sombra = função de `normal · luz` (`lvl = 0 se shade<=0 senão min(6, shade*7>>14)`). Luz fixa no mundo `LW=(-5734,11207,-10486)`; paredes usam normal `(0,-16384,0)` → nível 0; sprites usam normal `(-1204,2353,-2202)` → nível 1 (atlas de sprites lido em `TEXY + 255 + v`, ou seja linha 768+v-1 → o código usa `v` com +1).
- Mapeamento de textura é **afim** (sem correção de perspectiva) → tiras longas e oblíquas distorcem/borram. Isso é a origem de muitas queixas ("textura vazando/borrada").
- Faces lisas usam `LOP=8` (TIMP): **cor 0 não é desenhada**; por isso cores lisas precisam ter índice ≠ 0 e < 128 (bit7 = textura).
- Registradores (índice → função): `0x18` F/CX/CY/ZNEAR/W/H; `0x40` VADDR/EADDR/NVERT/NEDGE/COLOR/LOP; `0x50` fluxo de vértices; `0x52` fluxo de faces (11 bytes: 4 índices, 3×s16 normal, 1 base); `0x53` fluxo de UVs (8 bytes/face); `0x58` FADDR/NFACE; `0x5A` luz; `0x60..0x64` textura; `0x65` TADDR; `0x46` página Y; `0x48` CTRL (bit0 RUN, bit1 faces, bit2 texturas).
- Modelo de referência bit-exato: `alexmoncks/V9968_Cartridge` → `geo3d/sim/gen_scenes.py::render_faces` e `gen_vectors.py::model`. O harness importa isso (variável `GEO3D_SIM`, padrão `/home/user/alexmoncks/v9968_cartridge/geo3d/sim`).

### 4.3 ROM ASCII16
- Registradores do mapper: `0x6000` (janela 0x4000) e `0x7000` (janela 0x8000). Banco 0: cabeçalho `AB` + assinatura `ROM_AS16` em 0x4010 (o openMSX detecta sozinho) + boot. Banco 1: código em 0x8000 (ENASLT para o slot do cartucho). Bancos 2+: dados lidos pela janela 0x4000 com `BANK(n)`.
- Limite do mapper: 256 bancos = **4 MiB** (a ROM atual do E1M1 usa 256 → no limite).
- O código precisa caber em **um** banco de 16 KiB (hoje ~12,7 KB no E1M1).

### 4.4 SDCC 4.2.0 (armadilhas reais encontradas)
- `(s32)a*b` dá resultado errado quando o multiplicador é negativo → usar `mulq14` em assembly (`src/math.s`, `(a*b)>>14`, validado 600/600).
- `int*int` estoura em 16 bits sem aviso (ex.: `c*51` com `c=16384`); usar `mulq14` ou pré-escalar. Isso já causou um bug de passo de piso.
- Variáveis `static` não aparecem no `.map` (testes precisam de símbolos globais: `th`, `px`, `py`, `yaw`, `hp`, `dbg`…).
- Globais inicializadas exigem a cópia `_INITIALIZER → _INITIALIZED` no `crt0.s` (já feita).
- Convenção sdcccall(1): 1º arg em HL, 2º em DE, retorno de 16 bits em DE.

## 5. Layout do repositório (relevante)

```
msxrom/
  README.md                 documentação técnica incremental (v1…v11)
  HANDOFF_DEVIN.md          este arquivo
  build.sh                  build da ROM pública HANGAR (out/msxdoom.rom)
  machines/C-BIOS_Z80_V9968_Geo3D.xml   máquina Z80 com Geo3D embutido (montada por mim a partir de dois XML do usuário)
  src/crt0.s, math.s        boot ASCII16, mulq14
  src/main.c                motor da fase HANGAR
  src/e1m1.c                motor do E1M1 (inclui "e1m1_assets.h" gerado em private/)
  tools/
    wad.py                  leitor de WAD (lumps, PLAYPAL, picture, texturas compostas, mapa)
    e1m1.py                 peças de parede (regras de peg do Doom), atlas de texturas (shelf packing)
    e1m1_map.py             raster de 8 unidades: abertura por inundação, setor por célula
    e1m1_pvs.py             PVS por bloco+direção, tiras LOD, faces distantes, tiles lisos, lids
    vis.c (+ ctypes)        LOS 2D em C (compila para private/vis.so)
    e1m1_build.py           gera private/e1m1_assets.h e private/e1m1_banks.bin
    build_e1m1.sh           build completo do E1M1 → private/e1m1.rom
    mkrom.py                monta a ROM ASCII16 a partir do .ihx + bancos de dados
    z80_harness.py          Z80 emulado + mapper + V9968 (HMMV/LMMM/LRMM com NY e janela/LINE) + Geo3D (modelo de referência)
    e1m1_sim.py / e1m1_tour.py / e1m1_view.py / e1m1_holes.py / e1m1_profile.py / test_e1m1.py
    (HANGAR) make_assets.py, pvs.py, level.py, test_boot.py, test_game.py, tour.py, profile.py
  private/                  (gitignored) WAD convertido, e1m1.rom, imagens de diagnóstico
```

O WAD do usuário estava em `/tmp/claude-0/wad_in/doom1.wad` (fora do repo). Em uma nova sessão será preciso pedir o WAD ao usuário de novo.

## 6. Pipeline do E1M1 (conversor → ROM)

### 6.1 Entradas e regras de conversão
1. `wad.py` lê `doom1.wad`: diretório de lumps, `PLAYPAL`, imagens no formato *picture* (colunas/posts), `PNAMES`/`TEXTURE1/2` (texturas compostas), e o mapa `E1M1` (VERTEXES, LINEDEFS, SIDEDEFS, SECTORS, THINGS). Bug corrigido: lado "sem sidedef" é `0xFFFF` e deve virar `-1`.
2. `e1m1.wall_pieces`: para cada lado de linedef gera peças `mid` (linhas de um lado), `upper` e `lower` (linhas de dois lados), com alinhamento de textura conforme Doom: `xoffset/yoffset`, `ML_DONTPEGTOP (8)`, `ML_DONTPEGBOTTOM (16)`. Convenção: observador à **direita** de A→B.
3. `split_piece` divide cada peça nas repetições da textura (multiplos de largura/altura) → *subquads*.
4. Portas: na fase 1 são **abertas para sempre** (`open_doors`: teto = `max(piso+72, min(teto vizinho, piso+72))`; marca `is_door`).
5. Texturas: atlas 256×128 (linhas VRAM 512–639), escala 0,25 texel/unidade por padrão, subindo para 0,5 nas texturas de maior área enquanto couber (`choose_scales(budget_px=30000)`); empacotamento por prateleiras de altura variável.
6. Paleta de 256 cores: índice 0 transparente; índice 1 quase-preto (teto/tampas); índices 2..(1+NF) = 2 tons por flat usado; em seguida 1 cor média por textura de parede (para faces lisas distantes, hoje não usado); depois as cores base (corte mediano ponderado). **Cores lisas de faces precisam ficar com índice < 128.**
7. Fontes 5×7 geradas em código (`GLYPHS`), 41 caracteres `" !./0123456789-A..Z"`; letras acentuadas e `%` não existem.
8. HUD: `STBAR`, `STARMS`, dígitos `STTNUM*`, rostos `STFST*0`/`STFDEAD0`, pistola `PISGA0..D0` e flash `PISFA0` (escala 0,65), todos reduzidos e quantizados na mesma paleta.
9. Sprites 24×32 por célula (10 colunas × até 4 linhas, linhas VRAM 768+): zombieman, shotgun guy, imp (6 frames cada), itens, barril, lâmpada, pilar, candelabro, explosão do barril (sprite existe, **explosão não implementada**).
10. Coisas (THINGS) filtradas por skill (padrão 3 = flag 2; `4/5` = flag 4; `1/2` = flag 1) e sem flag 16 (multiplayer). Coisas fora da área acessível são ignoradas e listadas.
11. Raster 577×357 células de 8 unidades; valor = índice do setor ou 255 (bloqueado). Inundação a partir do start do jogador; linhas com `special != 0` são atravessáveis; degraus **não** bloqueiam a grade (o motor impede subir >24 e permite descer). Setor de cada célula = lado do linedef mais próximo.

### 6.2 Layout da ROM e da VRAM
- Bancos: `2` = tiles de flats 16×16 (linhas 212–275 da VRAM; **hoje não usado pelo motor** desde que removemos o piso texturizado, mas ainda enviado), `3` = fontes (linhas 468–…), `4..11` = linhas 512–1023, `META_BANK` (tabelas), `RAST_BANK0…` (raster, 13 bancos, 28 linhas por banco), depois blobs de PVS.
- VRAM 512–1023: atlas paredes (512–639), *regiões de piso/teto antigas* 640–767 (**hoje sem uso**), sprites 768–895, barra de status/ARMS/dígitos/rostos/pistola 896–1023.
- Uploads de VRAM: `set_vram_write(blk, addr)` recebe **bloco de 16 KiB + deslocamento dentro do bloco** (bug antigo: passar endereço absoluto 0xD400 quebrava o upload das fontes). Linhas 212 → `set_vram_write(3,0x1400)`; linha 468 → `set_vram_write(7,0x1400)`.
- META: `sin_tab` (256×s16), paleta (768 B), setores (10 B cada: floor, ceil, f0, f1, c0, c1, special, sky, fid, cid), tabela de blocos (3 B: banco + offset), grade de blocos (u16), things (7 B).
- Blobs de PVS: `nv, nf` + vértices (6 B) + faces (11 B: 4 índices + normal + base) + UVs (8 B/face). Empacotamento first-fit decrescente (economizou ~20 bancos). Total de bancos hoje 256 (limite).

### 6.3 PVS (visibilidade) — o coração do problema
- Grade de **blocos de 128 unidades**, **4 conjuntos por bloco** (um por direção do olhar, cones de ±92°, histerese ~5° no motor: troca quando `|yaw - centro|` > 36/256).
- Visibilidade 2D em C (`vis.c`): ponto de vista = células abertas no bloco (≈16); amostras em 3 pontos de cada subquad deslocados 1,5 unidade para o lado visível; bloqueadores = linhas de um lado + linhas de dois lados sem altura livre.
- **Orçamento**: ≤245 faces e ≤215 vértices por conjunto. Preenchimento guloso por **importância** = área projetada ≈ `comprimento × altura / (dist+48)²`; o que não cabe é descartado (os menores/mais distantes primeiro).
- Tiras por distância: regra `t = clamp(k·dmin, 24, 230)`; `K_LIST=(1.0,1.5,2.2)` — tiras longas distorcem a textura (mapeamento afim).
- Peças distantes (> `DFLAT=260` do bloco): uma face por faixa vertical com **uma repetição da textura esticada**, em pedaços de ≤256 unidades (mais longos que isso o borrão afim fica feio).
- Tiles lisos de piso (retângulos de 32 a 320 unidades por setor, cor do flat) com peso baixo (0,25×) — corrigem pisos de outros setores (lago, degraus). Tetos como tiles e "tampas pretas" (`make_lids`) foram **implementados e depois desligados** (custavam vértices demais); o código ainda existe em `e1m1_pvs.py` (`make_lids`, `visible_lids`, `lid_in_cone`), mas `e1m1_build.py` passa `alll=[]`.
- Métrica de qualidade: `tools/e1m1_holes.py` compara a máscara de paredes do PVS com a de um modelo sem limites (último valor medido: **perda média ~5%, p90 ~13%** em 30 amostras; já foi 17% antes da correção por direção).

## 7. Motor (`src/e1m1.c`)

- **Boot:** SCREEN 8, paleta do banco META, uploads de VRAM, `load_meta()`, `new_game()`, desenho estático do HUD nas duas páginas, detecção do Geo3D (porta 0x9D devolve 0xFF se não existir → mensagem `GEO3D NAO ENCONTRADO`), marcadores de progresso nos 4 quadradinhos do HUD (vermelho = Geo3D não terminou um RUN).
- **Loop:** `read_keys → update → render → hud_update → vblank + troca de página`. Duas páginas (double buffer).
- **Controles:** setas giram/andam, ESPAÇO atira (e reinicia), ENTER = usar (interruptor de saída). Matriz de teclado: linha 8 (`ppi_c` low nibble 8): bit0 espaço, bit4 ←, bit5 ↑, bit6 ↓, bit7 →; linha 7: bit7 RET, bit3 TAB.
- **Colisão:** raster de 8 unidades; `can_stand` testa 5 pontos (centro + cantos de raio 14 jogador / 12 monstros); sobe ≤24, desce qualquer altura. `los()` usa passo ~16 unidades.
- **Jogo:** HP 100, munição 50 (pistola, hitscan), armadura (absorve 1/3), itens (estimulante +10, medkit +25, bônus de saúde/armadura, armadura verde/azul, clip +10, caixa +50, cartuchos, shotgun só dá munição), **nukage** (setores especial 5/7/16/4: 10/5/20 de dano a cada 32 quadros), saída (interruptor com ENTER a <110 unidades), morte e reinício.
- **Monstros** (zombieman 20 HP, sargento 30, imp 60): dormem até verem o jogador (LOS ≤1100) ou tiro ouvido (≤900), perseguem a 2 unidades/quadro, atacam em hitscan probabilístico (3–15; imp 3–24; sargento 3 balas). Projéteis do imp **não existem** (ataque instantâneo).
- **Sprites:** até `MAX_SPR=10` por quadro, os mais próximos visíveis (LOS em cache a cada 4 quadros), como quads voltados à câmera anexados às faces do bloco no mesmo RUN.
- **Câmera:** altura do olho = piso do setor + 41 com suavização (`eye += (tgt-eye+3)>>2`).
- **Fundo (v11):** `fill` preto no teto, faixas de cor do setor 96 unidades à frente no piso (2 tons) → Geo3D (paredes/tiles/sprites) → **teto preto sólido pintado depois**: `ceil_probe` (leque de 8 raios, 12 passos de 32 de *profundidade*, recalculado a cada 2 quadros) calcula por faixa a linha `max_i(89 − 160·H_i/d_fim_i)` com `H_i = teto_i − olho`; `ceil_fill` interpola em 16 tiras (se a diferença entre faixas vizinhas >24 usa a menor) e pinta de preto acima da linha. Isso corrige o "vazamento" de paredes altas por cima de um teto rebaixado.
- Banco de código: **12,7 KB / 16 KB**.

## 8. Ferramentas de verificação

- **`z80_harness.py`**: Z80 (`pip install z80`) + ASCII16 + V9968 simplificado (VRAM 256 KiB, paleta, HMMV/LMMV/LMMM, LRMM com `NY` e janela de origem, LINE horizontal) + Geo3D via modelo de referência. **Limitações:** não modela tempo do VDP, não modela `DIY/DIX`, ignora algumas diferenças do openMSX. **Aprendizados:** (a) por muito tempo ele ignorava comandos LINE (faces lisas) — as imagens de simulação "boas" eram enganosas para tiles/tetos; (b) a simulação não reproduz o desempenho nem alguns artefatos que o usuário vê no openMSX.
- `e1m1_sim.py` (quadros), `e1m1_tour.py` (folha de contato a partir de pontos `x y ang_doom`), `e1m1_view.py` (render direto do PVS), `e1m1_holes.py` (métrica de buracos), `e1m1_profile.py` (T-states por quadro e por função), `test_e1m1.py` (14 testes de lógica; exige `E1M1_WAD=/caminho/doom1.wad`).
- **Convenção de ângulo:** `yaw=0` = norte (+y do Doom). `yaw = (90 − ang_doom)·256/360`. Um bug de sinal nas ferramentas já foi corrigido; evite reintroduzir.
- Perfil de CPU medido (Z80 emulado, só CPU, sem esperas): base sem planos ≈ 0,5 M T-states/quadro; versão com piso/teto texturizado por LRMM chegou a 5,8 M (inviável) e foi otimizada até ~1,4 M; o estado atual (v11) ≈ 0,63 M.

## 9. Como construir e rodar

```sh
# dependências (Linux): SDCC >= 4.2, Python 3 (numpy, pillow, z80), gcc
cd msxrom
tools/build_e1m1.sh /caminho/doom1.wad 3        # skill 3 (HMP) → private/e1m1.rom (4 MiB)
E1M1_WAD=/caminho/doom1.wad python3 -I tools/test_e1m1.py
python3 -I tools/e1m1_tour.py 1056 -3616 90 ...      # x y ang_doom, repete em trios
python3 -I tools/e1m1_profile.py 1056 -3616 90       # T-states por quadro
python3 -I tools/e1m1_holes.py /caminho/doom1.wad 5 30
```

**Rodar no openMSX (Linux Mint, relatado pelo usuário):**
- Precisa do **fork** `https://github.com/alexmoncks/openMSX` (ramo `geo3d`), compilado em `~/openMSX`; o openMSX do `apt` **não** conhece a versão de VDP `V9968` (erro "Unknown VDP version").
- Pacotes: `tcl-dev` (não `libtcl8.6-dev`) além de SDL2, GLEW, freetype, png, zlib, ogg/vorbis/theora, xml2, ALSA.
- Binário: `~/openMSX/derived/x86_64-linux-opt/bin/openmsx`. É necessário `sudo make install` (cria `/opt/openMSX/share` com `unicodemaps`, máquinas e C-BIOS) — sem isso o erro era `Couldn't find unicodemaps/unicodemap.jp_ansi`.
- Comando: `OPENMSX=~/openMSX/derived/x86_64-linux-opt/bin/openmsx ./rodar.sh [maquina]` (script do zip entregue ao usuário). Máquinas: `C-BIOS_R800_V9968_Geo3D` (padrão, preferir) ou `C-BIOS_Z80_V9968_Geo3D` (Z80 a 3,58 MHz é inerentemente lento). **O tipo de ROM tem de ser ASCII16.**
- O usuário rodou no openMSX 21.0 com "C-BIOS MSX2+ JP V9968 + Geo3D (Z80)" e relatou lentidão mesmo a 10,74 MHz.

## 10. Histórico de feedback do usuário (e o que foi feito)

1. **Paredes faltando / achatadas** → causa: conjunto único de 360° por bloco estourava o limite de vértices (só ~30% das faces úteis para o frustum). Correção: 4 conjuntos por direção + faces distantes esticadas + métrica de buracos.
2. **Piso e teto "transparentes" (cor lisa)** → implementamos várias soluções: faixas por setor, LRMM por linha (afim em perspectiva), leque de raios, etc. Todas tinham defeitos (ver seção 11).
3. **Textura do chão "surgindo"/movendo ao andar** → só as linhas mais próximas eram texturizadas; depois, tremor por SX/SY inteiros. Mitigação: câmera de mapeamento encaixada em texels (não resolve 100%).
4. **Teto rebaixado no início, pé-direito maior adiante; pisos com texturas diferentes; cor mudando ao andar** (usuário enviou prints do PC) → portanto precisamos de plano por setor.
5. **Lentidão** (usuário): "super lento", mesmo com Z80 a 10,74 MHz. Hipótese **não confirmada**: o gargalo no openMSX é o trabalho do VDP (HMMV da tela toda + texels do Geo3D), não só a CPU.
6. **Vazamento de textura por cima do teto**, "teto deveria ser preto sólido" → solução atual: pintura do teto depois do Geo3D (v11).
7. Pedido atual: **tecla que mostra X, Y e ângulo na tela** para o usuário reportar posições exatas dos problemas.

## 11. Abordagens tentadas e descartadas (não repetir)

- **Piso/teto texturizado por LRMM** (regiões replicadas 256×64 no VRAM, tile 16×16 por 64 unidades): funciona matematicamente (erro 0,3–0,9 texel), mas (a) custa 4–11× de CPU, (b) tremor por start inteiro, (c) um único plano por quadro vaza em setores vizinhos, (d) fan de 8 raios corrige parcialmente mas ainda tem artefatos nos degraus. Removido na v11 (código ainda no histórico do git: commits `c74caa7`..`bbb1ab7`).
- **Tampas pretas ("lids")**: faces lisas acima de paredes de setores baixos. Resolviam a oclusão, porém consumiam ~30–45 faces e derrubavam a cobertura de paredes (`kept` de 0,95 para ~0,4). Código mantido mas desligado.
- **Faces lisas de cor média para paredes distantes**: "achatado", usuário reprovou. Voltou a textura esticada em pedaços ≤256.
- **Teto como tiles lisos no PVS**: cobertura incompleta por orçamento de vértices.
- **PVS por bloco sem direção**: 360° → buracos grandes.

## 12. Estado atual e pendências (priorizadas)

### 12.1 Estado do repositório
- Último commit: `29c074f` (v11, teto preto sólido em tempo de execução). Testes: **14/14 SIMULATED** passam.
- **Tecla de coordenadas (commitada, BUILD OK + SIMULATED, testes 14/14):** tecla **TAB** alterna `dbg` e `debug_text()` mostra `X<px> Y<py> A<ângulo>` no topo da tela (fonte cinza, y=14). Verificado na simulação (`X1300 Y-3150 A0` para yaw 64 = leste); **ainda não validada no openMSX pelo usuário**. Cuidado: o ângulo deve sair em graus de Doom `90 − yaw·45/32` (módulo 360) e o texto só usa caracteres da fonte (`X`, `Y`, `A`, dígitos, `-`, espaço).
- A ROM mais recente enviada ao usuário foi a **v11** (zip `msxdoom_e1m1.zip`; a ROM gerada é 4 MiB).

### 12.2 Pendências em ordem de valor
1. **Pedir ao usuário que valide a tecla TAB (coordenadas) no openMSX** e use-a para reportar pontos com problema.
2. **Desempenho no openMSX**: medir de verdade. Ideias: reduzir a área preenchida por quadro (viewport menor, ex. 256×144; repintar só o que as paredes cobrem), reduzir faces/vertices por conjunto, alternar atualização de piso a cada 2 quadros, medir custo de HMMV vs LMMM no fork, checar se `R#20` bit0 (alta velocidade) é respeitado pelo fork. Pedir ao usuário um fps aproximado e o número de faces/quadro.
3. **Fidelidade geométrica restante**: ~5% de área de parede perdida em média; salas grandes/janelas ainda perdem paredes distantes. Possíveis ganhos: melhorar fusão de strips, compartilhamento de vértices, dois RUNs por quadro (longe/perto), aumentar `MAX_VERTS` reduzindo `MAX_SPR`, trocar tamanhos de bloco.
4. **Teto sólido**: avaliar casos de borda da pintura (pilares/blocos baixos no meio da sala, janelas, setores com teto variável dentro de uma faixa). A abordagem depende da geometria 2D do raster; pode precisar de mais raios ou de interpolação por parede.
5. **Fidelidade de gameplay**: portas funcionais, elevadores (`special 88` etc.), iluminação por setor (flat/luz), animação do nukage, shotgun como arma, barris explodindo, projéteis do imp, sons, HUD de armas (ARMS) e teclas de chave. Itens/monstros em áreas só alcançáveis por segredos hoje são ignorados (4 itens do closet sudeste e 1 imp).
6. **Piso texturizado**: só vale revisitar se houver ganho de VRAM/CPU; ver seção 11.
7. **Qualidade do código**: remover do conversor o que ficou sem uso (tiles de flats no banco 2 e regiões 640–767 da VRAM; `make_lids`), liberar bancos de ROM (hoje 256/256), documentar o formato de `private/e1m1_assets.h`.
8. **Testes**: o harness precisa de modelagem de tempo de VDP para prever lentidão; adicionar testes para ceil_probe/ceil_fill e regressão visual (hashes de quadros).

### 12.3 Riscos e dúvidas abertas
- Diferenças entre o modelo de referência/harness e o fork do openMSX **não estão mapeadas** (o usuário vê artefatos que a simulação não mostra; o *bug de LINE ignorado* já provou isso).
- Limite duro de 4 MiB de ROM ASCII16 e de 16 KiB de código em um banco.
- Qualidade da textura de paredes caiu quando reduzimos o atlas para 128 linhas (abrimos espaço para piso texturizado, hoje removido). Dá para devolver ~128 linhas ao atlas de paredes: **ganho de nitidez barato** (ajustar `NBANDS`, `budget_px` e descartar as regiões de piso/teto).
- O usuário usa openMSX 21.0 do fork; versões diferentes podem se comportar diferente.

## 13. Armadilhas operacionais (aprendidas à força)

- Posição de upload na VRAM: sempre `(bloco de 16 KiB, deslocamento)`.
- Cor lisa de face ≥128 vira textura; cor 0 não é desenhada (TIMP).
- `TSTRIDE` é 8 bits → `255`; sprites usam `v+1`.
- Simulador ≠ openMSX. Sempre perguntar ao usuário a posição (X, Y, ângulo) e a máquina usada.
- Ao mudar o formato do blob, mudar também `blk_load` (`src/e1m1.c`), o escritor em `e1m1_build.py` e o harness.
- O build do E1M1 leva ~10–40 s (PVS em Python + C); o PVS é o passo caro.
- Stop hook do ambiente: não deixar arquivos não rastreados; `private/` e `out/*` já estão ignorados.
- Entrega ao usuário: ROM e zip como anexo (SendUserFile), nunca no git. O zip `msxdoom_e1m1` contém `MSXDOOM_E1M1.rom`, `rodar.sh`, `instalar_maquina_z80.sh`, `machines/C-BIOS_Z80_V9968_Geo3D.xml`, `LEIA-ME.txt`.

## 14. Referências

- Geo3D e cartucho V9968: `https://github.com/alexmoncks/V9968_Cartridge` (pasta `geo3d`: `README.md`, `sim/gen_scenes.py`, RTL em `fpga/.../src/geo3d/geo3d_engine.v` e `src/v9968/vdp_command.v`).
- openMSX com Geo3D: `https://github.com/alexmoncks/openMSX` (ramo `geo3d`, `README.geo3d.md`).
- Perfil Windows com C-BIOS V9968: `https://github.com/renatus-xxxx/openmsx-v9968-windows-setup`.
- Formato de WAD (público): cabeçalho, diretório de lumps, lumps de mapa (THINGS, LINEDEFS, SIDEDEFS, VERTEXES, SECTORS), PLAYPAL, PNAMES, TEXTURE1/2, flats 64×64, imagens *picture*.
- DOOM (fonte público): `https://github.com/id-software/doom`. **Não** redistribuir o WAD.

## 15. Mensagem rápida para quem assume

1. Peça ao usuário o `doom1.wad` (ou use o que ele indicar) e coloque fora do repositório.
2. Rode a seção 9, confirme que os 14 testes passam e que `private/e1m1.rom` é gerada.
3. Peça ao usuário posições (X, Y, A) via TAB dos pontos ruins e reproduza com `e1m1_tour.py`.
4. Atacar desempenho no openMSX antes de acrescentar recursos de gameplay.
5. Sempre informar BUILD OK / SIMULATED / USER-REPORTED / UNTESTED; nunca commitar WAD, dados convertidos ou ROM.
