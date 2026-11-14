# Keen Dreams MSX 0.9.62 → 0.9.63-timp — otimização de desenho (sugestão do itch.io + lições do RTS)

> **Atualização:** a conversão para TIMP foi implementada (parcial, ver "Resultado da conversão" abaixo). ROM e patch em `0.9.63-timp/`. As seções seguintes são o estudo original.

## Veredito sobre a mensagem recebida

A sugestão está **correta e foi confirmada por medição**. O jogo desenha atores
mascarados em dois passes `LMMM`: `AND` com a máscara e `OR` com a imagem
(`src/actors.c`, `hero_bitmap_draw.inc`, `boobus_bitmap.c`, `brocco_bitmap.c`,
`cart_draw.c`, `city_bitmap.inc`, `tuberia.c`). Um único `LMMM` com `TIMP`
(cor 0 da origem = transparente) faz o mesmo em um passe.

### Medição (openMSX 19.1, C-BIOS MSX2+, Melon Mines, mesma entrada, determinística)

Ferramenta: sonda `VDP.commandExecuting` (tempo em que o VDP está ocupado) +
contagem de comandos. ROM base = 0.9.62 recompilada do zip (SHA idêntico:
`b1d7bd45…2e56f`). ROM PoC = só troca AND+OR por TIMP em `ActorDraw` (caminho
genérico) e `HeroBitmap`, **sem converter a arte** (imagem fica errada; serve só
para medir o custo real do VDP).

| Cenário | VDP ocupado (base → TIMP) | Comandos VDP | ms de VDP por quadro desenhado |
|---|---|---|---|
| Caso 1 (Keen + ator em cena) | 405 ms → 179 ms (**−56%**) | 639 → 306 | 50,6 → 25,5 |
| Caso 0 (rolagem dominada por restauração de fundo) | 796 → 780 ms (−2%) | 1114 → 1106 | 56,9 → 55,7 |

Conclusão: onde o custo é ator mascarado, **o tempo cai pela metade** (como o
autor da mensagem previu). Onde o custo é restauração de cenário/rolagem, o TIMP
não ajuda — esse é o próximo gargalo (ver abaixo). A ordem de grandeza bate com
`docs/VDP-PROFILE-087.md`: pares LMMM AND/OR eram ~84% do tempo ativo do VDP.

Atenção: isto mede VDP, não FPS na máquina real; o jogo só gera um quadro
quando algo muda, então “FPS” do emulador não serve de métrica aqui.

## Por que não dá para só trocar a operação (o ponto difícil)

TIMP trata o **índice de cor 0** da origem como transparente. Hoje o preto
(índice 0) é também cor visível (contornos dos sprites; “preto opaco”, README).
Para usar TIMP:

1. Nos pixels de sprite, o preto visível passa para um **índice B** com paleta
   preto; o transparente continua 0.
2. Precisa-se de um índice livre. Os 16 índices EGA estão em uso, mas o **9
   (azul-claro) é quase inexistente**: 46 px por atlas em Hill/Apple/Grove/Swamp,
   <620 px nos demais. Proposta: remapear 9 → 1 (azul) em *todos* os recursos e
   fazer `paleta[9] = preto`. Custo visual desprezível.
3. Converter somente a imagem de cor (a máscara deixa de existir). Isso é
   mudança de dados, sem layout novo: o preto visível (máscara 0 & cor 0) vira B
   e a área transparente fica 0.

Escopo real (por isso não apliquei no jogo — só medi):
- Atlas de 12 fases (`assets/*/atlas.bin`) e `build_*_assets.py`;
- Dados “streamed”: herói (`hero_bitmap`, metade máscara/metade cor, 32×32),
  carrinhos, Boobus, Brocco, City, Apple, Mines, Tater, castelo;
- Cache de HUD (`tools/build_hud_cache.py` verifica pixels), sprites de hardware
  de tomate/cenoura/herói, `title`, `tuberia` e os testes de pixel;
- `opaque_sprites` continua válido (HMMM já faz uma passagem).

## Ganhos extras ao eliminar as máscaras
- VRAM: as áreas de máscara do atlas (~metade da área de sprites de cada fase)
  ficam livres. Dá para cachear mais poses (ver RTS) ou espelhar quadros.
- O herói grava 32 linhas × 32 bytes a cada desenho; com TIMP são 32 × 16 bytes
  (metade das escritas de VRAM pela CPU).
- `SpriteColorPass` (asm) deixa de ser necessário.

## Sprites de hardware para projéteis (segunda sugestão)
Válida. O projeto já tem a infraestrutura (`hw_tomato`, `hw_carrot`, `hw_hero`)
e **só se aplica ao modo MSX2+ com rolagem por registrador** (README: o MSX2
básico tem janela 256×176 sem isso). Projéteis (semente/flor) são 8×8–16×16:
um sprite de hardware (modo 2, 1 cor por linha) elimina restauração + 1–2
comandos por quadro. Perda de detalhe é pequena, como dito.
Cuidado (aprendido no RTS): limite de 8 sprites por linha; use as camadas de
atributos para decidir quando cair no desenho em software.

## Lições do RTS que se aplicam aqui
(relatório extraído da sessão do RTS; números são notas dela, só em emulador)
- Medir antes: sonda de PC/VDP no openMSX, quadros contados por emulação, não
  por tempo do host. Aqui: `mkbench.py`/`run.sh` (nesta pasta).
- **Trabalho só quando muda**: golpes/clarão só sujam a tela ao cruzar limiar;
  unidades paradas não são reavaliadas; barras de vida em cache. Equivalente
  aqui: ator parado e fora de rolagem não deve gerar restauração + redesenho.
- **RAM não zerada**: dois bugs de lógica custavam ~6% de CPU porque campos
  não inicializados mantinham `production()` ativa. Vale auditar `init_*`
  de inimigos/atores do Keen (flags “sujo” e contadores).
- Taxa menor para o que não precisa de 70 Hz (RTS: combate a 15 Hz).
- Desligar a varredura de teclado da BIOS (`SCNCNT=0xF3F6`): ~4% CPU no RTS,
  só validado em C-BIOS/WebMSX — testar em BIOS real.
- Tabelas pré-calculadas e laços em Z80 para varreduras quentes.
- Cuidado: leitura de input em Z80 não ajudou lá; poll a cada 2 ticks
  quebrou testes. Cada otimização foi validada por igualdade de pixel.

## Próximo passo sugerido (ordem de custo-benefício)
1. Conversão de paleta + atlas + herói para TIMP (maior ganho: ~−50% de VDP
   em atores). Validar por igualdade de pixel contra a 0.9.62 (o repo já tem
   `tests/*pixel*`/oráculos) e rodar as 12 fases.
2. Restauração de fundo: medir `cost_restore`/HMMM na rolagem (caso 0 acima,
   que o TIMP não resolve).
3. Projéteis em sprites de hardware no MSX2+.

## Arquivos desta pasta
- `poc-timp-medicao.patch` — a troca AND+OR → TIMP usada só para medir.
- `mkbench.py`, `run.sh` — benchmark do openMSX (requer `xvfb`, `openmsx`,
  `cbios`; usa `build/keydoor-test.tcl` gerado por `tests/make_keydoor_test.py`
  — rode também `make_game_test.py` e `make_world_test.py` antes).
- Build reproduzido com MSXgl 1.5.2 (commit `ab4b26f`, traz SDCC 4.6.0).
  **Não** substitua o SDCC do MSXgl pelo do sistema (4.2): o link falha.

---
## Resultado da conversão (0.9.63-timp)

**ROM:** `0.9.63-timp/keen_msx_0.9.63-timp.rom` (SHA-256 `872f1792…5a52`, ASCII16 2 MiB, compilada com MSXgl 1.5.2 + SDCC 4.6.0 do próprio MSXgl, sem erros).
Não foi testada em hardware nem em MSX real; só no openMSX 19.1 com C-BIOS (MSX2 e MSX2+).

### O que mudou
1. **Paleta:** o índice 9 (azul-claro, ~46 px por fase) virou o "segundo preto". Pixels 9 de toda a arte passam a 1 (azul): `import_ega.planar` (níveis gerados no build) e `apply_static_assets.py` (atlas de Hill/Melon/Bridge/Tuberia, sprites de hardware da cenoura).
2. **Atlas dos atores (`TimpConvert`, em `actors.c`)**: no carregamento de cada fase o próprio VDP reescreve, no lugar, a área da máscara de cada quadro escolhido: `NOT` → `AND 9` → `TIMP` da imagem de cor. Resultado: 0 = transparente, cor, ou 9 = preto visível. Depois cada quadro é um único `LMMM+TIMP` (antes: `AND` + `OR` + `SpriteColorPass`). Custo no carregamento: 3 comandos de VDP por quadro (≈ 13–66 quadros por fase).
3. **Herói em bitmap (`HeroBitmap0/1`)**: dados de 32×32 → imagem TIMP única; 1 comando por pedaço em vez de 2, e metade dos bytes escritos na VRAM a cada troca de pose (16 em vez de 32 por linha).
4. **`FastEnemy`**: partes não opacas usam TIMP nos quadros convertidos.
5. `tools/build_timp_frames.py` (novo, no `build.sh`) escolhe os quadros convertidos.

### Por que só alguns quadros (a conversão **não** é total)
O atlas compartilha/sobrepõe retângulos entre quadros (dedupe) e há quadros com a mesma máscara e cores diferentes. Converter no lugar nesses casos corromperia outro quadro. Por isso o gerador só converte quadros cujo retângulo de máscara não toca nenhum outro retângulo (máscara/cor/tile), com máscara estritamente 0/15. O resto continua em AND/OR (correto, só não acelerado). Cobertura (quadros mascarados convertidos):

| Fase | Hill | Melon | Bridge | Rapids | Pass | City | Apple | Grove | Bay | Swamp | Boobus | Castle |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| convertidos / mascarados | 28/39 | 44/52 | 66/75 | 13/70 | 14/84 | 27/50 | 13/49 | 13/79 | 0/64 | 11/81 | 1/60 | 12/66 |

Hill/Melon/Bridge ficam bem cobertas; as demais ficam baixas (muitos quadros "mortos" herdados de Rapids aparecem nas listas e bloqueiam os vizinhos). **Não convertidos de propósito (continuam AND/OR):** carrinhos, Mines/Tater, Brocco, City carrot/french, Apple, Boobus, Castle-squash, `FastStream`, Tuberia (mapa) — cada um tem dados próprios em ROM que precisam de um conversor específico.

### Verificação feita
- Atlas convertido na VRAM **idêntico** ao esperado (`verify2.py`: T = f(máscara, cor) do `atlas.bin` da própria build) em Hill, Melon, Bridge, Rapids, Pass, City, Apple e Swamp: 0 divergências. Grove, Boobus e Castle **não** foram verificados (o roteiro de entrada não carregou Grove; Boobus/Castle exigem progresso na campanha); Bay não tem quadros convertidos.
- Telas: Melon no spawn **idêntica pixel a pixel** à 0.9.62; Bridge/City com pequenas diferenças só onde um inimigo anda (fase de animação diferente por tempo). Mapa-múndi idêntico. Hill/Pass/City/Apple/Swamp conferidos visualmente (contornos pretos corretos).
- O quadro com "trapézio ciano" perto do Keen em Melon existe também na 0.9.62 (tile de primeiro plano), não é regressão.
- **Não executei a suíte de testes do repositório** (vários scripts do pacote dependem de arquivos ausentes).

### Medições (openMSX, MSX2, Melon, mesmo roteiro; tempo de chamada, ms emulados)
| | 0.9.62 | 0.9.63-timp |
|---|---|---|
| `HeroBitmap` (por desenho, MSX2) | 21,7 | 13,4 (−38%) |
| `ActorDraw` médio (MSX2) | 6,3 | 4,8 (−23%) |
| `DrawScene` médio | 229,5 | 228,3 (≈ igual) |

No MSX2+ o herói quase sempre é sprite de hardware, então o ganho vem só dos demais atores. **O `DrawScene` total quase não muda nestes roteiros: o custo dominante é rolagem/restauração de fundo, não atores.** Ressalva: as trajetórias do jogador diferem entre as ROMs (o tempo de desenho muda a sincronia da lógica), então as médias por chamada são comparáveis, mas não são um teste A/B rigoroso de FPS; nenhuma medida em hardware real.

### Limites e riscos conhecidos
- Pixel de cor 9 que escape do remapeamento (ex.: dados de outra origem que eu não encontrei) aparece preto. Auditei atlas, hero, HUD, sprites de hardware e imagens streamed (todos 0 após o remapeamento); não varri 100% da ROM.
- As diferenças de layout de atlas em Rapids+ entre 0.9.62 e esta versão vêm do dedupe após a fusão 9→1 (esperado).
- Grove (nível 8), Boobus e Castle: carregam código de conversão mas não foram exercitados.

### Como reproduzir
```bash
python3 0.9.63-timp/apply_static_assets.py <development062>
patch -p1 -d <development062> < 0.9.63-timp/timp-sources.patch
cp 0.9.63-timp/build_timp_frames.py <development062>/tools/
MSXGL_ROOT=/caminho/MSXgl bash <development062>/scripts/build.sh
```
(`timp_frames.h` é gerado pelo build.) Benchmarks/validação: `mkbench2.py`, `run2.sh`, `mkshots.py`, `shots.sh`, `alllev.sh`, `verify2.py` (ajuste os caminhos no topo dos `.sh`).

### Próximos passos
1. Cobertura nas fases 4–12: conversor com destino separado (a área de cor intacta + nova área TIMP) ou, melhor, gerar a imagem TIMP direto nos `build_*_assets.py`, onde o layout é conhecido.
2. Mesma conversão para os dados "streamed" (carrinhos, Mines, Brocco, City, Apple, Boobus, Castle).
3. Restauração de fundo/rolagem (o gargalo real nestes testes).
4. Sprites de hardware para projéteis no MSX2+.

---
## Por que ainda não chega a 30 fps (perfil, 0.9.63-timp, openMSX, Z80 a 3,58 MHz)

Cenário: Hill, Keen andando (esq./dir.) ao lado dos Carrot Couriers, 20 s emulados (`mkprof.py`/`runp.sh`).

| Onde vai o tempo | por quadro apresentado | % da janela |
|---|---|---|
| Lógica: `Game_Tick` (física do Keen) | 3,5 ms **por tick**, 70 ticks/s | 24% |
| Lógica: `CombatTick` (inimigos, itens, tiros) | 4,3 ms por tick | 30% |
| `DrawScene` | 114 ms | 41% |
| ↳ `RingScene` (fundo/restauração/atores) | 84 ms | |
| ↳ `ActorsPrepare` | 26 ms (CPU pura) | |
| ↳ `HudDraw` | 20 ms (espera do VDP) | |
| ↳ `HWUpload` | 8 ms | |

Resultado: **3,65 fps**. A CPU fica saturada (VDP ocupado só 11–24% do tempo): o gargalo é CPU (C compilado pelo SDCC), não VDP nem atores.

Ablações (sem recompilar):
- Inimigos desligados (`enemy_mode=1`): só **5,6 fps**. Mesmo sem inimigos o motor não chega perto de 30.
- Mesma ROM com o Z80 mais rápido: 3,58 MHz → 3,65 fps; **7,16 MHz → 15,8 fps; 10,7 MHz → 19,7 fps**. Os valores ficam perto de 60/4 e 60/3 porque a apresentação é sincronizada com o vblank (fps quantizado: 30, 20, 15, 12…).

Para 30 fps (≤ 33 ms por quadro) a 3,58 MHz seria preciso reduzir lógica + desenho em ~6×: p.ex. lógica ≤ 1,5 ms/tick e `DrawScene` ≤ 20 ms. Isso exige reescrever em Z80 os caminhos quentes (lição do RTS: ordenação/emissão/varreduras em asm, pular o que está ocioso, taxa menor para o que não precisa de 70 Hz), medindo cada passo. Em 7 MHz+ (Turbo R no modo Z80 turbo, OCM, emulador acelerado) 30 fps fica ao alcance com ~2× de ganho.

---
## 0.9.64-hud-z80: primeiro kernel Z80 (HUD) — resultado

**ROM:** `0.9.64-hud-z80/keen_msx_0.9.64-hud-z80.rom` (SHA-256 `41a94b1d…0c77`), patch sobre a 0.9.63-timp: `hud-z80.patch` (só `src/hud.c`).

O que foi feito: o laço por-quadro de `HudDraw` (cópias do cache em até 18 pedaços, compilado pelo SDCC com locais em IX a 19 ciclos/instrução) virou um kernel Z80 (`HudFast`) que emite um comando de VDP preparado por pedaço. Se algum pedaço cruza a borda do buffer circular 256×256, ele devolve 0 e o laço C original faz o trabalho. Parâmetros vão pela pilha porque a RAM persistente está cheia (uma primeira versão com variáveis estáticas falhou na verificação de RAM do build).

Medição (Hill, Keen andando com inimigos, 20 s, openMSX MSX2+ 3,58 MHz):

| | 0.9.63-timp | 0.9.64-hud-z80 |
|---|---|---|
| `HudDraw` | 19,7 ms | 11,4 ms (−42%) |
| `DrawScene` | 113,7 ms | 103,5 ms |
| fps | 3,65 | 4,00 |

Os 11 ms restantes são tempo de VDP (18 cópias HMMM seguidas, cada uma esperando a anterior): CPU livre não adianta aqui. Imagem do HUD verificada: Melon idêntica pixel a pixel; em City só difere o dígito de munição (estado de jogo diferente entre execuções, o desenho em si está correto).

### Honestidade sobre a meta de 30 fps
Este foi o item mais barato e deu +10%. O perfil por amostragem de PC (`pcsym2.py`, resolve funções `static`) mostra que o resto também está pulverizado, como no RTS: `Game_Tick` 9%, `CombatTick` 9%, `MapTileKernel` 6%, `Columns`+`Rect`+`Rows` ~8%, `Carrot` 5,5%, `InventoryTick` 3%, `ActorsPrepare` 4%, `HillTick` 3%, `BonusTick` 2,4%… e boa parte já foi reescrita em Z80 em versões anteriores (`MapTile`, `MakeBounds`, `TouchCached`).
Estimativa: cada item restante rende 5–15%. Com a lógica a 70 Hz (55% da CPU) e `DrawScene` em ~100 ms, **30 fps a 3,58 MHz não é alcançável sem mudar a arquitetura** (por exemplo, lógica dos inimigos a 35 Hz e redesenho/restauração de fundo por sprites de hardware), o que muda o comportamento do jogo. Itens candidatos, em ordem de custo/benefício:
1. `ActorsPrepare` (26 ms/quadro): propagação de dano O(n²) entre atores; só cabe reduzir com algoritmo novo (grade/listas), não com micro-asm.
2. `RingScene` (74 ms): colunas/linhas novas por quadro (`Columns/Rows/Rect`): cada ladrilho = `MapTile` + comando; dá para pré-gerar listas de comandos por faixa.
3. Lógica: `CombatTick` a 35 Hz (itens/bônus/inventário/dobradiças) e `Game_Tick` com cache de `Wall()` por tick.
4. Sprites de hardware para HUD/projéteis para sair da restauração de fundo.

---
## 0.9.65-35hz: lógica a 35 Hz — resultado

**ROM:** `0.9.65-35hz/keen_msx_0.9.65-35hz.rom` (SHA-256 `e3a6be91…31ab`). Patch sobre a 0.9.63-timp (inclui o kernel do HUD da 0.9.64): `35hz-and-hud.patch`.

### O que muda no comportamento
- **Keen:** `Game_Tick2` avança dois ticks de 70 Hz com **uma** passada de física. O original aplica gravidade e aceleração horizontal só nos ticks ímpares, então dois ticks equivalem a gravidade/aceleração uma vez e deslocamento 2×v; o resto (salto, `jump_hold`, animação de caminhada, borda de saída) foi escalado do mesmo jeito. Cai para dois ticks normais quando há situação especial: poste (subindo/descendo ou recém-saído), tecla R, nível com carrinhos (Melon Mines e Apple Acres), ou se o passo duplo bateria em parede/teto (a física é desfeita e refeita em dois ticks normais, para não enfiar o Keen num canto).
- **Inimigos, tiros e demais lógica de combate:** continuam a 70 Hz (cada passo chama `CombatTick` duas vezes, com a paridade de tick correta). **Pickups e bônus** (`InventoryTick`/`BonusTick`) passam a 35 Hz: o Keen se move < 3 px entre dois ticks e os itens têm ≥ 8 px.
- Mudança de estrutura: a seleção de atores por fase saiu do banco fixo (que estava a 17 bytes do limite) para `LevelSelect` em banco paginado.

### Medição (openMSX MSX2+, 3,58 MHz; Hill, Keen andando ao lado dos inimigos, 20 s)
| | 0.9.63-timp | 0.9.64-hud-z80 | 0.9.65-35hz |
|---|---|---|---|
| fps | 3,65 | 4,00 | **5,20** |
| `Game_Tick` por tick | 3,5 ms | 3,5 ms | 2,3 ms (`Game_Tick2` 4,67 ms por passo) |
| `DrawScene` | 114 ms | 103 ms | 96 ms |

Melon Mines **não melhora** (1,70 fps nas duas): lá o caminho rápido fica desligado por causa dos carrinhos.

### Verificação de equivalência (no PC, `host-step2/`)
Teste diferencial compilando a física real (`game.c`) com os dados de Hill, Bridge, Rapids e Pass: roteiros aleatórios de entradas, comparando duas passadas de `Game_Tick` com uma de `Game_Tick2` a cada passo.
- **Hill:** 92% dos passos idênticos; desvio máximo 4,75 px; nenhuma trajetória divergente em 180 roteiros; mesma altura de pulo e velocidade.
- **Rapids / Pass / Bridge:** 76–90% / 78–82% / 46–55% dos passos idênticos; 4–13 em 60 roteiros terminam em trajetórias diferentes (>25 px) — diferença de fase em rampas e bordas (o Keen chega à borda 1–2 px antes ou depois), não correção errada. Estados "Keen dentro de parede" (métrica própria): iguais ao original em Rapids/Pass; ~20% a mais em Bridge (317 vs 296, 669 vs 514, 751 vs 625 por 20 mil passos).
- Esses números são da física, no PC; **não testei a jogabilidade da ROM com mão humana**. No emulador só conferi que o Keen anda e a tela fica correta.

### Onde está o tempo agora (Hill)
`CombatTick` 4,1 ms por tick (29% da CPU), `DrawScene` 96 ms (50%: `RingScene` 69 ms, `ActorsPrepare` 26 ms, `HudDraw` 13 ms), `Game_Tick2` 16%. Faltam ~6× para 30 fps. Próximos candidatos: inimigos a 35 Hz (exige reescrever cada inimigo com passo duplo), `CombatTick` (laço de 1,3 ms por chamada sobre uma tabela em `world_map+9600`), `ActorsPrepare`, e permitir o passo duplo com carrinhos.
