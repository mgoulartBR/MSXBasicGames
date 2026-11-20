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

---
## 0.9.66-enemy-calls: inimigos — o que os dados mostraram

**ROM:** `0.9.66-enemy-calls/keen_msx_0.9.66-enemy-calls.rom`. Patch acumulado sobre a 0.9.63-timp: `all-changes-since-0.9.63.patch` (HUD Z80, passo de 35 Hz do Keen, e as mudanças abaixo).

### Por que não levei os inimigos a 35 Hz
O perfil por linha (`pcline2.py`, amostragem de PC mapeada para linhas de C) mostrou que o custo "dos inimigos" em Hill era, em grande parte, **a chamada** de `Carrot`/`CarrotContact` para inimigos **inativos**: cinco chamadas por tick, cada uma com prólogo do SDCC (~0,2 ms) só para retornar na primeira linha. Reescrever o passo dos inimigos ativos com passo duplo (máquinas de estado com contadores de ticks: Carrot, `BridgeWalker`...) mexe no comportamento e, depois de remover essas chamadas, os ativos custam ~0,2 + 0,3 ms por tick, ou seja, no máximo ~4% a ganhar. Fiz no lugar disso mudanças de **equivalência exata** (sem alterar nenhum comportamento):

1. `Carrot`/`CarrotContact`: o teste de inatividade que eles já faziam na primeira linha passou para o ponto de chamada (`CARROT_STEP`, `CARROT_CONTACT`); o único efeito colateral, `a->vx=0`, é feito no lugar.
2. `ShotTick` só é chamado para tiros ativos (ele já retornava na primeira linha).
3. Laços sobre `hill_actors` com incremento de ponteiro em vez de `hill_actors+i` (o SDCC gerava deslocamento de 4 bits por elemento).
4. `InventoryTick`/`BonusTick`: testes de bit por byte em vez de `1UL<<i` (32 bits).

### Medição (openMSX MSX2+, 3,58 MHz; Hill com inimigos, 20 s)
| | 0.9.63-timp | 0.9.64 HUD | 0.9.65 35 Hz | 0.9.66 |
|---|---|---|---|---|
| fps | 3,65 | 4,00 | 5,20 | **6,25** (6,65 numa versão intermediária; a trajetória do Keen muda de uma ROM para outra, então ±5% é ruído) |
| `CombatTick` por tick | 4,3 ms | 4,3 ms | 4,1 ms | 2,6 ms |

### Verificação
- Mudanças de equivalência exata: Carrot/Contact/ShotTick/laços/bits, por inspeção (as condições movidas são as da primeira linha das funções).
- Telas: Melon idêntica à 0.9.63-timp; City com a diferença habitual de fase dos inimigos; Hill carrega e anda.
- Swamp (nível 10): o harness dá "Game Over" em ~34 s com o Keen parado **também na 0.9.63-timp** (mesmo número de mortes e mesma posição), então não é regressão.
- Não rodei jogo com mão humana.

### Aviso sobre o harness
Mudar a quantidade de variáveis estáticas muda o layout de RAM; os scripts de teste gerados (`tests/make_*_test.py`) embutem endereços e **precisam ser regenerados** depois de cada build (`make_game_test`, `make_world_test`, `make_keydoor_test`), senão o nível nunca carrega.

### Estado e próximos passos
Hill em 6,25 fps, ainda 5× abaixo de 30. `DrawScene` ~96 ms (60% da CPU): `RingScene` ~75, `ActorsPrepare` ~26, `HudDraw` ~16. Daqui para a frente, **o desenho** é o gargalo: faixas novas de fundo por quadro (`Columns/Rect/Rows`) e a propagação de dano de `ActorsPrepare`.

---
## 0.9.67-draw: ataque ao desenho (RingScene) — diagnóstico e resultado

**ROM:** `0.9.67-draw/keen_msx_0.9.67-draw.rom` (acumulado sobre a 0.9.66; patch: `all-changes-since-0.9.63.patch`).

### Onde está o tempo de um quadro (Hill, Keen andando, 3,58 MHz, média por quadro desenhado)
| Parte | ms | natureza |
|---|---|---|
| `ActorsPrepare` | 26 → **24,5** | CPU (laço de 29 atores 10 ms, `ActorsCollect` 8, `HWPrepare` 2,5) |
| `RingScene` sem atores | ~48 | CPU: restauração de fundo (`Rect`), colunas/linhas novas (`Columns`/`Rows`), `MapTile` |
| ↳ HUD (`HudDraw`) | ~12–16 | tempo de VDP: 18 cópias pequenas seguidas |
| `ActorsDraw` sem HUD | ~8 | CPU + VDP |
| `HWUpload`/`HWPrepare`/`HWFlip` | ~11 | upload de sprites de hardware por quadro |

### O que tentei no `RingScene` e por quê parou
- **Içar as tabelas** `tile_x/tile_y/solid_tiles` para fora dos laços, laços de ator com ponteiro, `CleanSave` com ponteiro: **0% medido** (226,9 → 228,3 ms e 108,3 → 110,5 ms num roteiro determinístico de rolagem; dentro do ruído, até levemente pior). Revertido: `ring.c` ficou idêntico ao da 0.9.63.
- Já estava ajustado por versões anteriores: `MapTile` é Z80 puro (≈150 ciclos), há fusão de ladrilhos iguais/sólidos em uma só cópia (`RepeatStrip`), restauração por faixas pré-calculadas, e uma rotina `RingCopy` em Z80 que **foi desligada** (`FAST_RING_COPY 0`) pelas medições de antes.
- O custo restante do `RingScene` é estrutural: cada ladrilho 16×16 novo é um comando de VDP (≈ 36 por coluna nova, em **cada uma das duas páginas**, que ficam 1–2 quadros defasadas) mais a decodificação do mapa. Isso escala com a velocidade de rolagem, não com a taxa de quadros, e não cabe em micro-otimização.

### O que entrou
- `ActorsPrepare`: atalho exato para slots de ator vazios (nem desenhados agora nem em qualquer página; o slot 14 do HUD mantém seu tratamento): 26,3 → 24,5 ms por quadro.

### O que falta para 30 fps (estimativa honesta)
Custos **fixos por quadro** ≈ 70 ms (prepare 25 + HUD 15 + sprites de hardware 11 + atores 8 + restauração/preâmbulo ~10); para 30 fps cabem ~20 ms. Alternativas que mudam a arquitetura, em ordem de retorno:
1. **HUD em 1–2 comandos** em vez de 18: exige 72×32 px contíguos na VRAM (hoje o cache usa buracos espalhados do atlas). Daria −12 ms por quadro. Possível se o atlas de cada fase for reempacotado liberando um bloco, ou se o painel for reduzido.
2. **Página única com rolagem por registrador e atores só em sprites de hardware** (sem restauração de fundo nem 2ª página): elimina `Rect`/CleanSave/ActorsPrepare quase por completo, mas muda o visual (limite de 8 sprites por linha, cores por linha) e exige reescrever o desenhista.
3. `HWUpload` incremental (enviar só o que mudou).

Medições deste ciclo usam `mkprof.py`/`runp.sh` (agora com `BENCH_SWEEP`, `BENCH_IDLE`, `BENCH_MOD`) e o conversor de PC para linha de C (`pcline2.py`).

---
## Protótipo de página única (Etapa 1) — resultado

**ROM de teste:** `proto-single-page/keen_msx_proto-single-page.rom`; patch sobre a 0.9.67: `single-page.patch`. **Não é jogável como jogo completo**: só Hill/níveis sem Melon Mines; mostra fundo + Keen em sprite de hardware; **sem inimigos, sem itens, sem HUD, sem primeiro plano** (a lógica roda normalmente, por isso o Keen pode morrer sem ver o que o mata).

### O que foi construído
- Uma única página exibida (anel 256×256, página 0), sem segunda página, sem restauração de fundo, sem `ActorsPrepare`/`ActorsDraw`/`CleanSave`.
- Cada quadro: colunas/linhas novas (reaproveitando `Columns`/`Rows`), `HWPrepare`/`HWUpload` do Keen, espera do vblank (`Halt`), e só então `R#23/R#26/R#27` + troca da tabela de sprites.
- **Truque da faixa mascarada:** o `R#25 MSK` já esconde os 8 px da esquerda; as colunas que entram (≤ 8 px por quadro) são escritas exatamente nessa faixa antes de mudar o scroll, então não há quebra visível. Por isso a câmera horizontal é limitada a 8 px por quadro (a vertical, 56 px, usa as 64 linhas ocultas do anel). Em 14 fps o Keen anda ~7,5 px por quadro, dentro do limite; abaixo de ~9 fps a câmera passa a ficar atrás do Keen.
- Tentei antes o modo de 512 px de largura do V9958 (`R#25 SP2`, que daria margem bem maior): o **openMSX 19.1 não o aplica** (mesmo com `R#25=3` e `R#26` de 6 bits ele continua em 256 de largura), então não dá para validar aqui e descartei.

### Medição (openMSX MSX2+ 3,58 MHz, Hill, Keen andando 16 s)
| | 0.9.67 (tudo desenhado) | protótipo (só fundo + Keen) |
|---|---|---|
| fps | ~6,3 | **13,4** |
| `DrawScene` | ~95 ms | **30 ms** (`HWUpload` 8, `HWPrepare` 2,4, `Halt` + colunas ≈ 18) |

Atenção: não é comparação equivalente — o protótipo não desenha inimigos, HUD nem itens. É o **teto** do que a arquitetura permite com a lógica atual (que ainda custa ~35% da CPU: `CombatTick` 4,8 ms/tick, `Game_Tick2` 4,8 ms/passo).

### Por que não passei direto para a Etapa 2 (o que falta e o risco)
1. **HUD.** O painel 72×32 não pode ser um sprite fiel (precisa de 3–4 cores por linha, 10 células de 16×16 → ~30 planos de sprite, e o limite é 8 por linha). Opções: (a) barra de HUD no topo em largura total com **divisão de tela por interrupção de linha** (a barra fica numa região oculta do anel, sem redesenho; custa 16 linhas de jogo, 192→176, e exige uma ISR de linha com variáveis em RAM — que está cheia); (b) redesenhar o painel todo quadro como hoje (~12–16 ms).
2. **Inimigos e itens como sprites.** Hoje só o Keen e alguns inimigos (cenoura, tomate) têm tabelas de sprite; Brocco, Tater/batatas, Aspar, Squash, Boobus (chefe grande), pickups e bônus são bitmaps com máscara. Cada tipo precisa de conversão para sprites de 16×16 com cores por linha (várias camadas com bit CC) e cabem no máximo 8 por linha: cenas com 3+ inimigos ao lado do Keen vão perder sprites. Trabalho por tipo de inimigo e por fase (12 fases).
3. **Primeiro plano** (túneis/pontes que passam na frente do Keen): hoje é desenhado por software por cima; com sprites a prioridade fundo/sprite é fixa (sprite sempre na frente) — exigiria truques.
4. **Atenção ao `Halt()`**: ~8 ms por quadro estão em espera de vblank; dá para eliminar aplicando os registradores na ISR de vblank (também precisa de RAM).
5. A **RAM persistente está cheia** (a verificação de build já barrou duas tentativas): qualquer estado novo da ISR exige liberar RAM antes.

### Decisão necessária
Ver **Etapa 2 em fatias verticais**: Hill completa (HUD por divisão de tela, cenouras/tater/pickups como sprites), medir fps real e só depois decidir pelas outras 11 fases. Estimativa honesta: não é uma tarefa de uma sessão; Hill sozinha já são vários ciclos (ISR de linha, conversão de sprites, prioridade do primeiro plano).

---
## Etapa 2 (página única + sprites só em Hill): inviável com a arte atual — dados

Antes de escrever a conversão fiz as contas com os dados do próprio jogo, e elas fecham a porta:

1. **Orçamento de sprites.** O V9958 mostra no máximo **8 sprites por linha** (32 planos). Pelas tabelas geradas (`hw_sprite_rows.h`), o Keen sozinho usa **8 sprites numa mesma linha em 54 dos 56 quadros**, e cenoura/tomate usam 7. Qualquer linha de varredura compartilhada pelo Keen e por um inimigo exigiria ≥ 15 sprites. Para caber seria preciso reduzir a fidelidade (menos cores por linha) do Keen **e** de cada inimigo, ou seja, redesenhar a arte.
2. **Atores em software numa página única exibida.** Apagar e redesenhar na página que está sendo exibida pisca, porque o pipeline atual (restaura todos os retângulos antigos, depois desenha todos os novos) deixa os atores invisíveis por vários quadros. Evitar isso exige restaurar+desenhar cada ator de forma atômica, com ordem de pintura e sobreposição — reescrever o motor de dano, não adaptar.
3. **HUD contíguo.** Não há retângulo 72×32 livre no atlas de Hill (o maior livre é 256×27, e essa faixa é onde ficam as tabelas de sprites de hardware), então não dá para cortar as 18 cópias do HUD num comando só.
4. **Divisão de tela para o HUD** exigiria ISR de linha com RAM que não existe (a RAM persistente está cheia).

O protótipo de 13,4 fps é, portanto, um teto que **não se realiza no jogo completo** com esta arte e esta RAM.

### O dado que decide o caminho: a mesma ROM em CPU mais rápida
0.9.67, Hill, Keen andando ao lado dos inimigos, tudo desenhado (renderizador de duas páginas, arte original):

| Z80 | fps | `DrawScene` |
|---|---|---|
| 3,58 MHz | 6,3 | 94,6 ms |
| 7,16 MHz | 17,9 | 42,6 ms |
| **10,7 MHz** | **29,7** | 27,9 ms |

(openMSX, `z80_freq`; não é medida de R800, mas indica a escala.) Ou seja, **o renderizador atual já entrega ~30 fps com 3× de CPU** (por exemplo OCM/Zemmix/Turbo R em modo Z80 turbo ou com clock aumentado), sem tocar na arte nem no comportamento. A 3,58 MHz o teto realista que sobra é ~8–9 fps.

### Caminhos restantes (para decidir)
- **A. Mirar CPU acelerada** (recomendado): a 0.9.67 já serve; só falta confirmar em hardware/emulador acelerado e, se quiser, ligar o modo turbo na inicialização (ex.: `CHGCPU`/porta do Turbo R) — a lógica de tempo usa o JIFFY e se adapta.
- **B. Ficar em 3,58 MHz**: sobram ganhos de 5–10% por item (`HWUpload` incremental, `ActorsCollect`, restauração por faixas) → ~8 fps; 30 fps não é alcançável sem refazer a arte/atores.
- **C. Redesenhar a arte para sprites** (menos cores por linha) e reescrever dano/atores: projeto de várias semanas, visual diferente do original.

---
## 0.9.68-vblank: troca de página/scroll na interrupção de vblank (alvo: 7,16 MHz)

**ROM:** `0.9.68-vblank/keen_msx_0.9.68-vblank.rom` (acumulado; patch sobre a 0.9.63: `all-changes-since-0.9.63.patch`).

### O que muda
No clock de 7,16 MHz o perfil por PC mostrou o `Halt()` de sincronismo do vblank (CPU parada dentro de `DrawScene`) em **15,5% do tempo** (~9 ms por quadro), mais 9,9% esperando o VDP. Agora `DrawScene` não espera: grava em RAM tudo o que precisa mudar junto no vblank (página `R#2`, tabela de sprites `R#5/R#6/R#8/R#11`, scroll `R#23/R#26/R#27` calculado de `drawn_camera_*`) e marca `flip_flag`; a **ISR do vblank** (`SoundISR`, gancho `H.TIMI`) aplica tudo de uma vez e zera a marca, enquanto a CPU já roda a lógica do próximo quadro. O início de `DrawScene` e `HWHide` esperam `flip_flag==0`.
- RAM: **zero bytes novos**: as 6 variáveis `flip_*` ocupam o lugar dos 6 contadores de profiling antigos (`cost_*`, `stamp`), cujas atualizações foram removidas.
- Bug encontrado no caminho: `flip_flag` precisa ser zerado no boot (a RAM não é zerada; um valor aleatório escrevia lixo em `R#8` e corrompia a VRAM). `R#8` é escrito como constante (0x28 / 0x2A com sprites desligados), porque o espelho de `R#8` do MSXgl não é confiável.
- Menores (exatos): faixas sem sinal e câmera em variável local em `BonusCollect`, laço de pickups e `HillSelect`.

### Medição (openMSX MSX2+, Hill, Keen andando ao lado dos inimigos, 20 s)
| Z80 | 0.9.67 | 0.9.68 | `DrawScene` |
|---|---|---|---|
| 3,58 MHz | 6,3 fps | **7,55 fps** (+20%) | 94,6 → 77 ms |
| 7,16 MHz | 17,9 fps | **22,75 fps** (+27%) | 42,6 → 32,8 ms |
| 10,7 MHz (0.9.67) | 29,7 fps | — (não medido) | — |

Imagem verificada nos dois clocks e também em MSX2 (sem rolagem por registrador): idêntica à da 0.9.67.

### Onde está o tempo a 7,16 MHz agora (PC)
`VDP_CommandWait` 13% (CPU esperando comandos do VDP), `ActorsPrepare` ~11%, `HWUpload` 6,5%, `CombatTick` 4,5%, `Columns` 4%, `HillSelect` 3,8%, `ActorsCollect` 3,7%, `MapTileKernel` 3,6%, `Game_Tick2` 3,4%, `BonusCollect` 3,3%.

### Riscos / limites
- Testado só no openMSX (C-BIOS MSX2 e MSX2+). A ISR de vblank escreve em `R#2/5/6/8/11/23/26/27` com as escritas em pares do VDP dentro da interrupção; o jogo só mexe nesses registradores fora de `DrawScene` pela própria ISR, e todo código de VDP do MSXgl usa DI/EI, mas **precisa de teste em hardware real** (sobretudo MSX2+/Turbo R).
- 30 fps a 7,16 MHz ainda não: faltam ~25%.

## 0.9.69-scan (item 3: laços de coleta em Z80)

- `HWUpload`: laço de atributos com invariantes fora do laço (+0,4%).
- `scanwin.inc`: varredura em Z80 (`ScanWin`) da janela de tela para bônus e pickups; o C só trata os candidatos visíveis (+0,9%).
- Hill, 7,16 MHz: 22,75 → **23,05 fps**. Imagem verificada (bônus/pickups visíveis).
- Perfil do `RingScene`: custo próprio de desenho ≈15% (Columns 3,9, MapTileKernel 3,5, Rect 3,0, RingScene 2,3); o resto são esperas do VDP (HudFast 51%, restaurações 45%).
- Experimento: HUD em 1 comando em vez de 18 → 24,0 fps (+5% no teto); não há retângulo 72×32 livre no atlas de Hill.
- Conclusão: restam ganhos de 1–5% por item; 30 fps a 7,16 MHz exige reduzir trabalho do VDP/redesenho (alternativas 1 e 2 propostas).

## HUD em faixas contíguas (experimento, NÃO incluído na ROM)

- `hud-strips/`: `build_hud_cache.py` (ativado com `HUD_STRIPS=1`) aloca 2 faixas 72×16 livres por fase e `hud.c` aceita chunks de altura variável (asm usa `h`, laços em C também).
- Faixas livres só existem nas fases 4–11; **Hill, Melon, Bridge e Castle não têm nenhuma janela livre de 72 px de largura** (Hill: 0 posições mesmo com 4 linhas). Hill, a fase de referência, não ganha nada.
- Em Apple (fase 7) o HUD apareceu corrompido com as faixas (`apple_strips_corrupt.png`): provável colisão com regiões escritas em tempo de execução (`AppleBitmap`/streams) marcadas como livres. Sem verificar fase a fase, não é seguro distribuir.
- Hill medido: 22,75 fps (sem diferença). ROM não publicada.

## 0.9.70-hudmove (corte de trabalho do VDP: HUD movido em 1 comando)

- Em vez de restaurar o painel antigo do HUD (≈15 cópias de tile) e redesenhá-lo (18 cópias do cache) a cada quadro de scroll, `RingScene` copia o painel dentro da página (1 comando HMMM, ou LMMM se a paridade de X for ímpar, com DIX/DIY para sobreposição) e restaura só o "L" descoberto, com pixel exato (`RectX(..., exact=1)`).
- `ActorsPrepare` decide (`HudMoveClear`): só usa o caminho curto se o HUD não mudou (valores iguais), |Δx|≤16, |Δy|≤4 e nenhum ator visível (varredura Z80 `ScanWin`) toca as caixas antiga/nova do painel; senão cai no caminho normal. `RingScene` também cai no normal se a caixa cruza a borda do anel.
- Hill, 7,16 MHz: 23,05 → **25,20 fps** (+9%); comandos VDP por quadro 23 → ~11. 3,58 MHz: 7,35 fps (CPU-bound, sem ganho). Capturas: ~250 quadros em Hill sem HUD ausente/fantasma; fases 2, 4, 7 com HUD correto (Apple já mostrava HUD duplicado antes desta mudança).
- Aprendizado: o custo da verificação importa — a primeira versão (laço C sobre 58 registros) custava 10% e anulava o ganho; com `ScanWin` e verificando só os registros novos, ficou ~2%.

## 0.9.71-prepare (ActorsPrepare + HWUpload)

- `SlotScan` (Z80): lista só os slots que o primeiro laço de `ActorsPrepare` precisa visitar (HUD, visíveis agora ou em qualquer página); o laço C itera ~10 slots em vez de 29. `ActorsPrepare` 11,2 → 10,0 ms/chamada; 25,2 → 25,8 fps.
- `HWUpload`: laço de atributos desenrolado por linha de 8 sprites com ponteiro (o SDCC gastava ~7% do tempo ali); 25,8 → **26,7 fps** (Hill, 7,16 MHz). Sprite do Keen verificado em captura.
- Soma desde 0.9.68: 22,75 → 26,7 fps (+17%). Faltam ~12% para 30 fps: `VDP_CommandWait` ~9%, `Columns` ~5%, `HillSelect` ~4%, `ScanWin`/`SlotScan` ~3–4% cada (os núcleos Z80 usam IX/IY e gastam ~300 T por item; uma versão com HL só deve cortar metade).

## 0.9.72-z80 (HillSelect + núcleos Z80 sem IX)

- `HillSelect`: janelas comparadas em 1/16 px com aritmética u16 (sem deslocamentos por ator); equivalência com a versão original verificada por simulação (3 M casos, 0 divergências para posições < ~3900 px; Hill tem < 2000 px). 26,7 → 27,4 fps.
- `SlotScan` reescrito com HL/BC/DE e ponteiros relativos (sem IX; o slot do HUD entra sempre porque `visible`=1): 27,4 → 28,4 fps.
- `ScanWin` reescrito com DE como ponteiro do registro e H/L como contador/índice, teste de X só com registradores e pulo por `stride-1`/`stride-3`: 28,4 → **29,05 fps** (Hill, 7,16 MHz). 3,58 MHz: 7,55 → 8,75 fps.
- Caveat: duas capturas isoladas em t=15 s (uma sem HUD/Keen, outra com sprite embaralhado sobre o HUD) não reproduziram em 8 execuções de 5 capturas e ~250 quadros extras; provavelmente quadro de transição do cenário de teste, não confirmado.
- Soma desde 0.9.68: 22,75 → 29,05 fps (+28%).

## 0.9.73-ring (laços do RingScene/CleanSave)

- `SlotScan` virou `slotscan.inc` (compartilhado) e `RingScene` o usa para visitar só os atores com algum bit visível; o laço de `CleanSave` anda por ponteiro. Hill 7,16 MHz: 29,05 fps (sem mudança); 5,37 MHz: 18,85; 3,58 MHz: 8,75 → 8,90.
- **Achado:** o quadro agora cabe em 2 vblanks (33 ms) na maior parte do tempo, e o desenho espera o flip do quadro anterior (`while(flip_flag)Halt()`). O ritmo fica travado perto de 30 fps (NTSC); 29,05 = ~97% dos quadros em 2 vblanks. Ganhos adicionais de CPU a 7,16 MHz só reduzem os quadros lentos (teto 30,0); por isso o fps não mudou apesar de menos CPU.
- O `Columns` não foi alterado: o tempo dele é espera do VDP (≈12 comandos HMMM pequenos em série por quadro de scroll), sem cópia equivalente mais barata (copiar da página da frente carregaria atores).

## Teste das demais fases com a 0.9.73-ring (openMSX, 7,16 MHz, 20 s por fase, Keen parado/idle)

| Fase | Resultado | fps medido |
|---|---|---|
| 1 | HUD, bônus e inimigos corretos | 33,5 (cenário sem movimento) |
| 2 | OK | 16,75 |
| 3 | OK | 22,0 |
| 4 | OK (Keen morre no teste e a fase termina; HUD correto antes disso) | 6,05 |
| 5 | OK | 11,6 |
| 6 | OK | 5,8 |
| 7 | OK (HUD duplicado em alguns quadros, já existia antes das otimizações) | 15,05 |
| 8 | OK (roteiro corrigido: a fase leva ~27 s para carregar; HUD e cenário corretos) | 12,05 |
| 9 | OK | 12,55 |
| 10 | carrega e mostra HUD; o Keen morre no teste e volta ao mapa | – |
| 11 | OK (roteiro corrigido: a entrada exige 12 bombas, o teste as define; HUD, cenário e chefe corretos) | 13,7 |
| 12 | OK (HUD e cenário corretos) | 9,8 |

Os fps das fases 2–12 vêm de um Keen parado/morrendo e não são comparáveis entre si nem com Hill. Nenhuma regressão visual foi vista nas 12 fases.

Roteiro (`mkprof.py`): espera a carga com polling (sem timeout curto) e define `boobus_bombs=12` ao testar a fase 11.

## HUD fantasma na fase 7 (Apple) — CAUSA ENCONTRADA E CORRIGIDA (0.9.74-hudfix)

- Rastreio dos comandos de VDP (`BENCH_VDPLOG`, `vdpdump.py`): a restauração do fundo do HUD antigo cobria só **48×16 px** (caixa do sprite 35), não 72×32; sobravam o terço direito e a metade de baixo do painel antigo (os "pedaços" que apareciam à direita/esquerda/embaixo).
- Causa: `build_hud_bounds.py` corrigia 48×16 → 72×32 em `assets/apple/load_data.bin`, mas a ROM empacota as definições de sprite da fase 7 a partir de `map_part0..4.bin` (fatias de 2048 bytes geradas ANTES da correção). A RAM em tempo de execução mostrava `48,16` em `world_map[6700+35*8+4]`. As demais fases usam `load_data.bin` direto (ou são verificadas por `build.sh`) e já estavam em 72×32.
- Correção: `build_hud_bounds.py` regera `map_part0..4.bin` a partir do `load_data.bin` corrigido; `build.sh` ganhou uma asserção (partes == `load_data.bin` e 72×32 em Apple).
- Verificação: 36 capturas da fase 7 sem fantasmas (antes: fantasmas na maioria dos quadros); fases 5, 6 e 9 com HUD normal; Hill 28,7 fps (sem mudança).
- O defeito vinha da ROM original 0.9.62 (o patch parcial era do build original).
- Hipóteses descartadas no caminho (mapa, tabelas de tiles, sobreposição do cache, movimento do HUD de 0.9.70) estão no histórico do git.

## Fases 8 e 11 com a 0.9.74-hudfix (openMSX, 7,16 MHz, 14 capturas por fase)

- ROM testada = ROM publicada (SHA256 `4ebe86da…`).
- Fase 8: carrega; HUD presente e íntegro em todas as capturas após a carga (12 de 12), sem pedaços fantasma. 12,05 fps (Keen parado/idle).
- Fase 11 (12 bombas definidas pelo roteiro): carrega; HUD presente e íntegro em 12 de 12; o chefe aparece. 13,7 fps (o Keen morre durante o teste).
- Observação (não investigada): sprites de inimigos aparecem com ruído/fragmentos em alguns quadros (fase 8) e o chefe da 11 tem bordas ruidosas; o mesmo aspecto já aparecia nas capturas da 0.9.73.

## Ruído em sprites — CAUSA E CORREÇÃO (0.9.75-vdpfix)

- Também aparece na ROM original 0.9.62. Causa: **perda de bytes nas escritas de dados no VDP a 7,16 MHz** (OTIR = 21 T/byte; o VDP não aceita esse ritmo e descarta bytes). `attrcheck.py` (tabelas de atributos de sprite íntegras?): 3,58 MHz 0/8 corrompidas; 5,37 MHz 0/8; 7,16 MHz 7/8, em Hill e na fase 8.
- Correção: `VPF` (em `main.c`, banco fixo — o banco 0x8000 do chamador precisa continuar mapeado), mesma assinatura de `VDP_WriteVRAM_128K`, usa `outi+nop` (32 T/byte) quando `vdp_fast`=1 e `otir` caso contrário. `DetectClock` (em `keen_msx_s21_b1.c`) conta um laço de 35 T durante um tick do BIOS (JIFFY): `n*hz > 160000` ⇒ rápido (medido: 3,58 MHz ≈ 81k, 5,37 MHz ≈ 135k, 7,16 MHz ≈ 181k, ISR incluída). Todas as escritas de VRAM do jogo (herói, bitmaps de textura/chefes, HUD, carga de atlas, mapa-múndi) passam por `VPF`. Para liberar bytes no banco fixo foi desligado `PRINT_USE_GRAPH` (caixas do Print, sem uso).
- Verificação a 7,16 MHz: 0 tabelas corrompidas em 8 dumps por fase (1, 2, 4, 7, 8, 11); capturas do herói e dos inimigos íntegras na fase 8 e do chefe da fase 11. A 3,58 e 5,37 MHz `vdp_fast`=0 (sem custo): Hill 3,58 MHz 8,8 fps (antes 8,9).
- Custo a 7,16 MHz (só quando `vdp_fast`=1): Hill ≈ 26,9–28,7 fps conforme o trajeto (antes 28,7).
- Falhas no caminho (para a memória): meu primeiro experimento usou um helper `__banked` e as imagens saíam riscadas — o helper trocava o banco em 0x8000 e escondia os dados do chamador; não era perda de bytes.
- Não testado: máquina PAL (50 Hz) — o limiar usa `n*hz`, mas não foi medido; hardware real.

## Teste visual da 0.9.75-vdpfix: fases restantes e PAL (openMSX)

- NTSC, 7,16 MHz, 8 dumps/capturas por fase: fases **3, 5, 6, 9, 10 e 12** — 0 tabelas de atributo corrompidas, `vdp_fast`=1, herói/inimigos/HUD íntegros nas capturas. (Com 1, 2, 4, 7, 8 e 11 já testadas, **as 12 fases estão cobertas**.)
- PAL (`C-BIOS_MSX2+_EU`, R#9 NT=1 confirmado; ROM EU copiada de `/usr/share/cbios/` para `~/.openMSX/share/systemroms/`): fase 8 a 3,58 / 5,37 / 7,16 MHz → `vdp_fast` = 0 / 0 / 1 e 0 tabelas corrompidas nas três; Hill a 7,16 MHz 26,85 fps e fase 11 (chefe) íntegros, 0 corrompidas.
- Limites: openMSX (sem esperas de E/S no `z80_freq`), não hardware real; fases com o Keen parado/morrendo no roteiro, não jogadas de ponta a ponta.

## Teto a 3,58 MHz (ablação; medição, nada disto entra em ROM)

Hill, `BENCH_Z80=3579545`, 30 s, mesmo trajeto, ROM 0.9.75 com chaves de desligamento (`teto-358/ablate.h`, `ablrun.sh`). fps = quadros desenhados por segundo.

| Variante | fps | vs base |
|---|---|---|
| Base | 9,13 | – |
| Sem inimigos de Hill | 9,83 | +8% |
| Sem HUD (não desenha nem restaura) | 12,33 | **+35%** |
| Viewport redesenhada com 160 linhas / 128 linhas | 9,30 / 9,77 | +2% / +7% |
| Lógica de combate a metade da taxa (1 `CombatTick` por par) | 10,77 | +18% |
| Sem lógica de combate | 11,97 | +31% |
| Desenhar só a cada 2ª iteração | 8,37 | −8% |
| Sem HUD + combate a metade | 13,93 | +53% |
| Sem HUD + sem combate + sem inimigos + viewport 128 (teto) | 18,67 | +104% |

- Perfil a 3,58 MHz (base): `CombatTick` 8,9%, `Game_Tick2` 7,8%, `Columns` 7,0%, espera do VDP 5,2%, `MapTileKernel` 5,0%, `ActorsPrepare` 4,4%… A lógica (combate, física, Hill/Bonus/Inventory) passa de ~27% e custa o mesmo por segundo qualquer que seja o fps (rodar menos quadros não a reduz).
- Mesmo removendo tudo isso, 18,7 fps: **30 fps a 3,58 MHz não é alcançável** por essas mudanças. Resto no perfil do teto: `ActorsPrepare` 8,4%, `ActorsDraw` 7,7%, `Game_Tick2` 7,3%, `Columns` 7,0%, `HWUpload` 6,5%.
- Maior item isolado: **HUD (+35%)** — é custo de projeto (painel flutuante redesenhado no anel a cada deslocamento), não só de jogabilidade.
- Limites: um trajeto de Hill, ablações não aditivas, "desenhar a cada 2ª iteração" é uma aproximação grosseira.

## HUD só quando muda (ancorado) — medição a 3,58 MHz, NÃO incluído em ROM

Implementação (`hud-anchor/`): o painel fica ancorado no anel e só é re-ancorado quando se afasta mais que uma tolerância da posição nominal na tela (ou ao iniciar a fase); sem movimento, nada é copiado/restaurado. Hill, 3,58 MHz, 30 s, mesmo trajeto (base 9,13 fps; "sem HUD" 12,33).

| Tolerância (esq/dir × cima/baixo) | fps | vs base | aspecto |
|---|---|---|---|
| ±24 × ±8 px | 9,97 | +9% | HUD "balança" até 24 px; pode ficar 8 px cortado à direita |
| ±48 × ±16 px | 11,23 | +23% | HUD vaga até 48 px e fica **cortado na borda direita** em vários quadros |
| ±96 × ±32 px | 11,37 | +25% | HUD quase sai da tela |
| assimétrica 32 esq / 12 dir / 4 cima / 8 baixo (mantém o painel visível) | 9,33 | +2% | pequeno balanço, sempre inteiro |

Motivo: a 3,58 MHz a câmera anda ~10 px ou mais entre quadros desenhados; qualquer tolerância pequena obriga a re-ancorar quase todo quadro. Os ganhos só aparecem com tolerâncias que estragam o visual. O teto "sem HUD" (+35%) não é atingível por esse caminho. ROM de referência continua a 0.9.75-vdpfix.

## Barra de HUD fixa no topo — protótipo de viabilidade (`hud-bar-proto/`, NÃO incluído em ROM)

Mecanismo: o HUD vira um bitmap de 8 linhas em linhas livres da VRAM (atlas); no início de cada quadro (hook H.TIMI) o VDP é configurado para mostrar essas linhas (R#2 = página do atlas, R#23 = linha da barra, R#26/27 = 0) e uma interrupção de linha (hook H.KEYI, `LineISR`) logo depois da 8ª linha restaura página/scroll do jogo (`cur_r2/r23/r26/r27`, copiados no flip do vblank). A faixa de 8 linhas do topo do cenário fica coberta pela barra; o resto do código de anel não muda. O HUD antigo deixa de ser desenhado (slot 14 invisível).

Achados no openMSX: (1) R#19 é comparado com o contador de linha **já com R#23 somado** (linha da interrupção = R#23_da_barra + 7); (2) sprites do herói e scroll corretos abaixo da barra (Hill e Swamp); (3) a interrupção de linha chega via H.KEYI, que precisa ler S#1 (R#15 = 1) para limpar FH.

Medição (Hill, barra de teste com conteúdo placeholder, sem HUD desenhado no anel; faixa de teste nas linhas 94+ do atlas, que em Hill não estão livres — só o custo importa aqui):

| Clock | Base 0.9.75 | Com barra | |
|---|---|---|---|
| 3,58 MHz | 9,13 fps | **11,77 fps** | +29% |
| 7,16 MHz | 26,9–28,7 fps | **30,4 fps** | no teto do vblank (30) |

Falta para uma versão utilizável (não feito): (a) 8 linhas livres por fase (Hill, Melon, Bridge, Rapids, Pass, Grove, Apple e Castle não têm: exige recompactar as tabelas de sprite de hardware ou o atlas); (b) arte e desenho do novo HUD de 8 linhas (placar, vidas, chaves, munição), desenhado só quando muda; (c) desligar a barra em título/mapa-múndi/telas de fim e na troca de fase; (d) PAL; (e) tolerância a interrupções adiadas (DI) — borda de guarda de 1 linha; (f) conferir em hardware real (mudança de R#2/R#23 no meio do quadro).

## Barra de HUD fixa no topo — versão com 4 segmentos (`hud-bar-full/`, NÃO incluída em ROM)

Implementado: faixa de 8 linhas montada de 4 trechos de linhas livres da página 2 (linhas 70–71, 75, 77–79, 86–87, que sobram entre as tabelas de sprites do herói em todas as fases com 64–90 livres), `LineISR` (hook H.KEYI) que troca R#23/R#2/R#26/R#27 seguindo o flag de retraço horizontal (S#2 bit 5), `BarDraw` (placar, vidas, munição; só redesenha o campo que mudou), HUD do anel desligado em MSX2+, desligamento em `HWHide`.

Medição em Hill (openMSX, com a barra rodando): 3,58 MHz 9,5–9,7 fps (base 9,13); 5,37 MHz 23,97 (base ~18,85); 7,16 MHz 34,3 (base 26,9–28,7). Sem o mecanismo da barra mas com o HUD oculto: 12,87 fps a 3,58 MHz — o `LineISR` ocupado (≈4–7% do tempo) e a quantização por vblank comem a maior parte do ganho a 3,58 MHz.

**Bloqueio (por isso não é utilizável):** a troca de segmentos precisa de sincronia exata de linha, mas a interrupção de linha chega com atraso de **~3 linhas** no openMSX/C-BIOS a 3,58 MHz (medido com linhas-marcador; o atraso cai com o clock e depende da BIOS). A primeira troca é exigida na linha 2 — antes de a ISR sequer começar — então as linhas 2–3 mostram a tabela de cores do herói (ruído) e os segmentos seguintes ficam deslocados. Medições também mostram que o FH só dispara em linhas de exibição (nada na área de vblank), então não dá para disparar antes da linha 0.

Alternativas avaliadas: (1) 8 linhas contíguas livres — impossível sem refazer a ligação `base+1024` das 10 rotinas `HeroData*` (bancos sem espaço: s26 com 15 bytes livres); (2) barra no RODAPÉ (linhas 184–191): a ISR dispara com folga (~8 linhas) e sincroniza com o próprio flag FH (S#1 bit 0), sem depender do atraso — robusto, mas muda o pedido (topo → rodapé); (3) tomar o vetor de interrupção (IM2) para latência ~0: robusto no emulador, risco em hardware real; (4) barra com 4 linhas de preenchimento no topo.

## 0.9.76-barra — barra de HUD fixa no RODAPÉ (incluída em ROM)

SHA256 `7aa30a61e82250624cbe58e1a9a375b049da03140d39b88fb0603d8a9b4619cb`. Só nos modos de scroll por hardware (MSX2+); MSX2 comum segue com o HUD desenhado no anel.

Mecanismo: bitmap de **7 linhas** (linhas de tela 185–191) montado de 7 linhas livres da página 2 (71, 75, 77–79, 86–87). A interrupção de linha (R#19 = linha 183, IE1 em R#0) é usada só como gatilho: a rotina `LineISR` espera o FH por *polling* de S#1 e então segue o flag HR (retrace horizontal, S#2 bit 5) de forma síncrona, trocando R#23 nas bordas de linha (e R#2/R#26/R#27/R#8 na primeira). O handler de vblank restaura os registros do jogo. Registradores alternativos são salvos/restaurados.

O que fez funcionar (depois da tentativa do topo, que falhou): (1) a rotina do FH chega no *fim* da linha 183, então o 1º HR já é a fronteira 184/185 — a contagem de bordas estava deslocada em uma linha; (2) código reto, valores pré-carregados em registradores (`out (c),reg`), 10 `out` no 1º bloco (≈130 T) — com despacho por `cp` a troca caía no meio da linha seguinte (rasgo horizontal visível); (3) o bitmap da barra usa as 7 linhas de fonte do HUD (sem linha em branco).

Teste (marcadores coloridos por linha, `screenshot -raw`, `tear.py`): linhas 185–191 exatas a 7,16 MHz; a 3,58 MHz exatas com um pequeno rasgo (≈5 colunas à esquerda) só na linha 190.

Fps (Hill, mesmo cenário, 30 s emulados, mesma métrica de renders/s):

| Clock | 0.9.75 | 0.9.76 |
|---|---|---|
| 3,58 MHz | 9,13 | 8,73 (−4%: custo da ISR de linha ≈ 9% da CPU) |
| 5,37 MHz | 19,70 | 23,43 (+19%) |
| 7,16 MHz | 26,47 | 33,07 (+25%; >30 porque há quadros só de herói) |

Verificação: 10 fases (1–10) com a barra correta (placar, vidas, munição), `attrcheck` 0 tabelas de atributo corrompidas a 3,58 e 7,16 MHz. Não verificados nesta rodada: fases 11/12, PAL, transições mapa↔fase com barra ativa (BarOff é chamado em HWHide).
Observação: a 3,58 MHz (alvo principal) a barra *não* ajuda — o ganho aparece de 5,37 MHz em diante.

### 0.9.76-barra — testes das fases 11 e 12, PAL e mapa (openMSX)

- **Fases 11 e 12** (7,16 e 5,37 MHz, 3 capturas e dumps por rodada): barra correta (fase 11 mostra as 12 bombas), herói/chefe/cenário íntegros, `attrcheck` 0 tabelas corrompidas nas 4 rodadas.
- **PAL** (`C-BIOS_MSX2+_EU`, R#9 NT=1 confirmado), Hill: 5,37 MHz 23,7 fps e 7,16 MHz 31,1 fps, 0 tabelas corrompidas, barra no lugar (8/8 capturas a 7,16 MHz limpas). 3,58 MHz: ver abaixo.
- **Defeitos remanescentes da barra** (8 capturas por configuração, linhas 185–191): 7,16 MHz NTSC/PAL limpa; **5,37 MHz** tem em cerca de metade das capturas pequenos resíduos (um segmento preto de ~15 px à esquerda na 1ª linha e 1–2 pixels soltos perto do placar, sprites/rasgo do 1º bloco de escritas); **3,58 MHz** (NTSC e PAL) a barra fica **corrompida em parte dos quadros** (trocas de R#23 fora de linha, faixas coloridas/pretas) — o teste de marcadores a 3,58 MHz tinha dado certo por acaso de fase. Com o alvo ≥ 5,37 MHz isso fica fora do escopo, mas a 3,58 MHz a 0.9.76 não deve ser usada.
- **Volta ao mapa**: `BarOff` (em `HWHide`) é chamado em toda troca de mapa/título e não foi exercitado de ponta a ponta — o roteiro `world.sh` produz tela preta também na 0.9.75, então não vale como evidência.

## 0.9.77-barra2 — resíduo da barra corrigido (alvo ≥ 5,37 MHz)

SHA256 `7d414e302b47845f3358d1a7a8fd14e221a2c7dcd330daa9f089f17175421c7e`.

Diagnóstico (diferença pixel a pixel contra um quadro limpo, `resid2.py`): todo o resíduo estava na 1ª linha da barra (185). O 1º bloco de escritas (R#23, R#2, R#26, R#27) levava ~0,3 linha a 5,37 MHz e as escritas de rolagem R#26/R#27 caíam **dentro** da linha 185: o trecho inicial saía com a rolagem do jogo (dígito do placar duplicado/deslocado, segmento preto à esquerda).

Correção: (1) a barra voltou a ter **8 linhas (184–191)**, com a linha 184 como espaçador liso (cor de fundo); o bloco de escritas agora cai no retrace da linha 183 e qualquer rasgo cai na linha lisa, invisível; para isso o gatilho FH passou a R#19 = linha 182 e as contagens de bordas HR foram deslocadas (+1); (2) ordem das escritas: R#26/R#27 primeiro, depois R#23/R#2; (3) R#8 (sprites off) escrito logo após o sincronismo, fora do bloco; (4) em `hud.c` a fonte de 7 linhas fica nas linhas 185–191.

Fallback: `DetectClock` agora também define `bar_ok` (n·hz > 110000, ≈ ≥ 5 MHz; 3,58 MHz ≈ 81k, 5,37 MHz ≈ 135k) e é chamado antes da 1ª cena. Abaixo disso a barra não liga e o HUD volta a ser desenhado no anel como na 0.9.75 (a 3,58 MHz a barra era corrompida). `bar_ok`/`vdp_fast` são zerados no boot (a RAM não é limpa).

Medição (Hill, mesmo cenário; `resid2.py` = pixels diferentes nas linhas 185–191 em relação ao quadro de referência):
| Máquina / clock | fps | pixels ≠ (4–12 capturas) | attrcheck |
|---|---|---|---|
| NTSC 5,37 MHz | 23,3 | 0 em todas | 0 corrompidas |
| NTSC 7,16 MHz | 31,7 | 0 em todas | 0 |
| PAL 5,37 MHz | 23,6 | 0 em todas | 0 |
| PAL 7,16 MHz | 31,1 | 0 em todas | 0 |
| NTSC 3,58 MHz | 8,4 (HUD no anel, sem barra) | – | 0 |
Fases 2–12 a 5,37 MHz: barra correta e 0 tabelas corrompidas; a fase 6 chegou à tela de "GAME OVER" no roteiro (Keen morto) sem restos da barra. Limites: openMSX (sem esperas de E/S de hardware real); a margem de sincronismo em hardware real a 5,37 MHz pode ser menor.

### 0.9.77-barra2 — volta ao mapa saindo de uma fase completa (openMSX, `retmap.py`)

Roteiro: título → mapa → entra em Horseradish Hill → teletransporta o Keen para perto da saída (a saída da fase 1 é à ESQUERDA, linha 31) e anda até completá-la → tela "LEVEL COMPLETE" → volta ao mapa → anda no mapa. Registradores lidos em cada etapa.

| Etapa | 5,37 MHz (barra) | 3,58 MHz (HUD no anel) |
|---|---|---|
| na fase | `bar_on`=1, R#0=0x16 (int. de linha), R#19=31, R#23=112 | `bar_on`=0, R#0=0x06 |
| fim da fase (`level_done`=2) | `bar_on`=0, R#0=0x06, R#8=42, R#23/26/27=0 | idem |
| mapa (`world_active`=1, `completed`=1) | R#0=0x06, R#23=16, R#2 normal, sem restos da barra | idem |

Capturas (`screenshot -raw`): fase com a barra → "LEVEL COMPLETE" limpo → mapa com a placa DONE e o Keen no caminho, sem restos da barra; idêntico a 3,58 MHz com o HUD no anel. Nota de método: com `set throttle off` o `screenshot` devolve quadros velhos/pretos; as capturas só valem com o acelerador padrão.
Não coberto: retorno após **morte** (já visto: tela de GAME OVER limpa na fase 6) e saídas das fases 2–12.

## Perfil da 0.9.77 a 5,37 MHz (Hill, 30 s emulados, 42778 amostras de PC; 23,3 fps)

Inclusivo por função (`mkprof.py`): `DrawScene` 65,5% da janela (28,1 ms/quadro), dentro dela `ActorsPrepare` 25,8% (11 ms), `RingScene` 24,9% (17,5 ms, em 428 de 699 quadros), `ActorsDraw` 4,6%; fora dela `CombatTick` 12,2% (3 chamadas/quadro, 1,74 ms cada), `Game_Tick` 2,9%, `ReadInput` 0,9%.

Amostragem de PC (perfil plano, nada passa de 7,3%): `Columns` 7,3 · `CombatTick` 5,8 · **`LineISR` 5,8** · **`HWUpload` 5,6** · `ActorsPrepare` 5,2 · `Game_Tick2` 4,7 · `MapTileKernel` 4,2 · `ActorsCollect` 4,1 · `ActorsDraw` 3,9 · `HillSelect` 3,9 · `VPF` 3,2 · **`HWPrepare` 3,0** · **`Keyboard_Read` 2,9** · `SlotScan` 2,7 · restantes ≤1,8 cada.
Agrupado: atores/inimigos ≈27%, lógica (combate+física+bônus+mapa) ≈23%, anel/VDP (Columns, VPF, esperas) ≈15%, sprites de hardware do herói (HWUpload+HWPrepare) 8,6%, rotina de linha da barra 5,8%, teclado 2,9%.
Para ir de 23,3 a 30 fps é preciso cortar ≈22% do tempo de quadro.

## 0.9.78-heroattr — atributos de sprite do herói em Z80 (item 1 do perfil)

SHA256 `82996e31f70773379c11d0f562bc0b00dc3203170c6d15a83a2952a9cf782ff6`.

O perfil mostrou que o custo de `HWUpload` (5,5%) estava nas linhas que montam as entradas de atributos (4,2% do total), não no envio de padrões (já em cache por tabela). `HWAttr` (Z80, em `keen_msx_s21_b1.c`) monta as 16/24 entradas (y, x, padrão, 0) com ponteiros e sem cálculo por entrada (índices de `hero_indices[]` ou sequência de passo 4). Um cache "atributos inalterados" foi tentado e descartado: o herói quase nunca repete (frame, x, y) e a RAM está a 10 bytes do limite do build (`< 0xF000`).

Medição (Hill, mesmo cenário, 30 s): 5,37 MHz **23,4 → 25,3 fps (+8%)**; 7,16 MHz 31,7 → 34,6 (acima do teto de 30 pela métrica); 3,58 MHz 8,4 → 8,6; PAL 5,37 MHz 25,3. `attrcheck`: 0 tabelas corrompidas em fases 1, 3, 4, 7, 8, 9, 11, 12 (5,37 MHz) e 1 (7,16 MHz, PAL); barra sem diferença (8/8 igual a antes); herói correto em 8 capturas de caminhada.
Folga de RAM: 0xEFFD de 0xF000 (3 bytes).

## Teclado (item 4 do perfil) — premissa errada, nada alterado

`ReadInput` custa 0,6% do quadro (0,23 ms, `mkprof.py`), não 2,9%. O "2,9% em `Keyboard_Read`" da amostragem de PC era atribuição errada: a tabela de símbolos agrupava no último símbolo anterior o código seguinte (a trampolina de chamada entre bancos, `___sdcc_bcall_ehl`, em 0x7CE8, e `set_bank`). Um `ReadInput` com 4 leituras de linha em vez de 11 foi testado: 25,30 → 25,43 fps (dentro do ruído); revertido, a 0.9.78 continua a mais recente.
Achado útil da correção: custo real de **chamadas entre bancos** ≈ 4,2% do quadro (`___sdcc_bcall_ehl` 2,26% + `set_bank` 1,47% + `get_bank` 0,43%), mais `MapTileKernel` 4,3% e `LineISR` 5,7% (ambos reais).

## 0.9.79-combat — laço dos atores de Hill desenrolado em `CombatTick`

SHA256 `c36d7345984d36b322ed3df94f2f9a967bd195fe8245200ca5a23395c2a30025`.

Perfil por linha: 3,7% do quadro estava nas duas linhas que percorrem os 4 `hill_actors` (passo e contato). O laço usava um ponteiro `static` em RAM e re-lia `active_level` a cada volta. Como `hill_actors` fica em endereço fixo (`world_map+9600`), os dois laços viraram 4 expansões de macro (`HSTEP(n)`/`HCONTACT(n)`) com endereçamento absoluto; a lógica é a mesma (inativo ⇒ `vx=0`, senão `Carrot()`; contato só se ativo e `state==0`).

Medição (Hill, mesmo cenário, 30 s): `CombatTick` 1,74 → 1,25 ms por chamada. 5,37 MHz **25,3 → 26,3 fps (+3,8%)**; 7,16 MHz 34,6 → 35,7; 3,58 MHz 8,6 → 10,9 (ali o combate pesa mais). `attrcheck` 0 corrompidas (fases 1, 2, 3); capturas das fases 1–3 normais. Não comparei o estado do jogo tick a tick com a ROM anterior (a posição final do Keen no roteiro difere em 2 unidades, esperado: o roteiro depende de tempo).

## 0.9.80-banks — menos travessias de banco

SHA256 `89e7ea16160f4be4c5d813d98328afd568163122655909d906d5fc434e1032ab`.

Contagem das chamadas entre bancos (`BENCH_BC` em `mkprof.py`: ponto de parada na trampolina, alvo = HL/E): 37 318 chamadas em 30 s de Hill a 5,37 MHz (≈47 por quadro desenhado), ≈180 T cada ⇒ 4,2% do tempo. Mais frequentes: `KeyDoorTick` 3522, `CombatTick` 3521, `HillTick` 3521, `LevelSelect`/`HillSelect`/`TomatoSelect`/`ReadInput`/`FollowCamera` 2001 cada, `BonusTick` 1760, `HudDirty` 1103, `HudDamage` 783, `BarDraw` 790.

Duas mudanças:
1. **Portões no chamador** (mesmo efeito, sem a chamada): `KeyDoorTick` não roda nas fases 1, 3, 9, 10, 11 (não faz nada nelas); `BarDraw` só quando placar/vidas/munição mudaram (variáveis de cache exportadas); `HudDirty`/`HudDamage` pulados com a barra fixa (`hardware_scroll && bar_ok`); `TomatoSelect` só nas fases 2, 4, 12 (nas outras só zera `tomato_count`/`tomato_budget`, como antes). 26,27 → 26,70 fps.
2. **Trampolina própria** `___sdcc_bcall_ehl` (em `main.c`, banco fixo) no lugar da da biblioteca do SDCC: mesmo contrato (e = banco, hl = função; preserva A/F, usa BC depois do retorno), mas com `get_bank`/`set_bank` embutidos (3 pares call/ret a menos por chamada). 26,70 → 27,33 fps.

Medição (Hill, mesmo cenário, 30 s): 5,37 MHz **26,3 → 27,3 fps (+4%)**; 7,16 MHz 35,7 → 36,9; 3,58 MHz 10,9 → 10,9; PAL 5,37 MHz 28,0. `attrcheck` 0 corrompidas (Hill nos 3 clocks e PAL; fases 2–12 a 5,37 MHz); barra com 0 pixels diferentes (PAL 5,37 MHz, 4 capturas); volta ao mapa (`retmap.py`) igual à 0.9.77.
Limite: a trampolina nova assume o mesmo mapeamento ASCII16 (registrador em 0x77FF, cópia do banco em 0xC004) que `set_bank` do crt0; mudar o crt0 exige rever as duas.

## Análise de `Columns` e `MapTileKernel` (0.9.80, 5,37 MHz; sem alteração de código)

Chamadas a `MapTile` em 30 s de Hill (`BENCH_BC`+`BENCH_RA` em `mkprof.py`, chave = endereço de retorno): ≈41 mil no total, das quais ≈8,7 mil vêm de `HotInit` (verificação única na carga do nível, fora do laço do jogo). No jogo: física (`Wall`, uma chamada em `game.c`) 15,3 mil ≈ 500/s ≈ 14 por tick de 35 Hz; `Columns` 12,1 mil (3 pontos de chamada: avanço vertical, laço de fusão sólida, laço de fusão de cópia); `HWPrepare` 3,5 mil (cache de colisão do herói); `Rows` ≈0,8 mil.
`MapTile` é um desvio `ld de,(hot_entry); push de; ret` (≈41 T) para o núcleo (`MapTileKernel`, cópia em RAM): ≈120 T, dos quais ≈75 T são o laço `srl d/rr e` (deslocamento de 0–7 bits, média 3,5 voltas × 29 T). Total ≈210 T por consulta ≈ 4,9% do quadro (4,2% núcleo + 0,7% desvio).
`Columns`: 7,4% do quadro só no próprio código (≈1000 T por tile: aritmética de 16 bits com variáveis `static` + montagem do comando de VDP), espalhado por 20 linhas, nenhuma passa de 0,8%.
Opções avaliadas (ganho estimado em fps, risco): (a) chamar o núcleo direto nos 4 pontos quentes e trocar o laço de deslocamento por salto calculado: +1 a +1,5%, baixo; (b) cache da coluna de tiles entre quadros em `Columns`: tira ≈1,2% de `MapTile`, mas a RAM do build está a 3 bytes do limite e o cache usa ≈28; (c) reescrever o laço interno de `Columns` em Z80: até +3 a +4%, alto esforço (montagem de comandos de VDP e três variantes de cópia).

## Opção (a) de `MapTile` — implementada e descartada (ganho dentro do ruído)

`MapTile` virou 3 bytes de RAM com `jp nn` (opcode gravado no boot, operando = `hot_entry`), eliminando o desvio `ld de,(hot_entry); push de; ret` (≈31 T por consulta). Medição (Hill, 30 s): 5,37 MHz 27,33 → 27,43 fps (+0,4%); 7,16 MHz 36,87 → 36,97 (+0,3%); 3,58 MHz 10,87 → 10,83. Menos de 0,5%: não vale o risco de código auto-modificável nem 1 byte de RAM (a folga do build é de 3). Trocar o laço de deslocamento do núcleo por salto calculado não rende: deslocar 4 bits por troca de nibble custa ≈85 T contra ≈116 T do laço, e só para 4 ≤ s ≤ 7. Código revertido; a ROM mais recente continua a 0.9.80-banks.
Erro de método encontrado: `tests/make_*_test.py` precisam do argumento `.` para regenerar `build/*-test.tcl`; sem ele os scripts de teste ficam com endereços de símbolos antigos (isso só trava o harness quando o layout da RAM muda, como aqui; as medições anteriores usaram endereços válidos).

## 0.9.81-lineisr — interrupção de linha armada mais tarde

SHA256 `f5ce7f2001b4a300be2069dbe91f6c3dfe3c6412f561c955c0fe70c3fe23dc58`.

A `LineISR` custa ≈5,7% porque fica em espera ocupada do disparo (linha 175 + latência) até a linha 190, a cada vblank (60 Hz): ≈15 linhas ≈ 0,95 ms. O trecho da barra (linhas 183–189) não encurta (as trocas de R#23 exigem sincronismo por HR), então a única folga é **começar mais tarde**: a linha em que `SoundISR` arma o R#19 passou de 175 para 178 (`sound_irq.c`).
Varredura do ponto de armamento (16 capturas, linhas 185–191 contra a referência): 179 e 180 funcionam; **181 falha** a 5,37 MHz (a interrupção chega depois da linha 182 e a barra some; a 7,16 MHz falha em parte dos quadros) — o penhasco está em 181 (latência ≈ 3 linhas). Adotado **178** (3 linhas de margem ≈ 190 µs; 179/180 deixam 1–2 linhas).
Medição (Hill, 30 s): 5,37 MHz **27,33 → 27,57 fps (+0,9%)**; 7,16 MHz 36,87 → 37,03; PAL 5,37 MHz 28,69. Barra com 0 pixels diferentes em 16 capturas (NTSC 5,37 e 7,16 MHz, PAL 5,37 MHz, PAL 7,16 MHz 14 capturas), `attrcheck` 0 corrompidas; retorno ao mapa OK.
Limite: a margem de 3 linhas foi medida no C-BIOS do openMSX; num MSX real, a latência até `H.KEYI` depende da BIOS e do clock. Se a barra piscar, voltar a 175 (uma constante em `sound_irq.c`, linha do `add a,#178`).

## 0.9.82-columns — laço de `Columns` reescrito em Z80

SHA256 `292eb48be5ff1a8871521cf54f9fbae323dd3c5c1accaab1ce173be3231c5c56`.

`Columns` (colunas expostas do anel, `ring.c`) é agora `ColumnsAsm` (Z80, ≈600 bytes): mesma lógica do ramo `FAST_RING_COPY=0` do laço em C — largura/offset de tile, `map=(camera_py>>4)*MAP_WIDTH+(wx>>4)` (multiplicação 8×16 por deslocamento e soma), altura por tile, fusão vertical de tiles iguais (cópia) ou de cor sólida (Hill, `HMMV`), `RepeatStrip` vertical chamado de volta em C (`ColumnsRepeat`), comandos `SetupR32`/`SetupR36` do MSXgl. As variáveis são as mesmas 18 bytes de RAM de antes (a folga do build é de 3 bytes).
**Verificação de equivalência por comandos de VDP** (`BENCH_COLDIAG` em `mkprof.py` + `cdcmp.py`): uma ROM de diagnóstico chama a versão em C e a versão em Z80 com os mesmos argumentos e o openMSX registra todas as escritas nas portas 0x99/0x9B de cada uma (excluindo as das ISRs e as esperas de comando); fluxos idênticos em 414 chamadas de Hill (84 182 bytes) e nas fases 2, 3, 4, 5, 6, 7, 8, 11, 12 (fase 9 sem chamadas no roteiro): **0 divergências**. Comparação de capturas entre ROMs não serve: o roteiro não é determinístico quadro a quadro.
Medição (Hill, 30 s): 5,37 MHz **27,57 → 28,00 fps (+1,6%)**; 7,16 MHz 37,03 → 37,10; 3,58 MHz 10,83 → 11,13; PAL 5,37 MHz 28,85. `ColumnsAsm` 6,5% do quadro (antes 7,4%): o ganho ficou abaixo do esperado — o custo remanescente está espalhado nas sub-rotinas (`ca_dxdy`, `ca_adv`, `ca_color`, `ca_edge`) e em ≈700 T por tile; o grosso do trabalho por tile é a emissão do comando de VDP (`SetupR32`, 15 bytes por `otir`) e o núcleo de `MapTile`, que continuam em chamadas.
`attrcheck` 0 corrompidas (Hill nos 3 clocks, PAL, fases 2–12 a 5,37 MHz); barra com 0 pixels diferentes (8 capturas NTSC e PAL); volta ao mapa OK.

## 0.9.83-hill — `ActorsDraw`, `HillSelect`, `HillTick`, `HillCollect`

SHA256 `ea709921d9f3c3fd4ffd5375285b790bcf78209796855e4a8978b9c50f1d810f`.

Perfil linha a linha dos blocos de atores (0.9.82, 5,37 MHz): o "`ActorsCollect` 5%" é a soma de linhas pequenas (varredura `ScanWin`, laço de picapes, preenchimento de `visible`), sem ponto único; o custo grande estava no **laço de `ActorsDraw`** (29 espaços por chamada, `actors_next+slot` com multiplicação por 6 a cada volta: linhas 143–144 = 4,4% do quadro) e em `HillSelect` (4,8%).
1. **`ActorsDraw`**: tabela `vis_off[]` (deslocamento slot·6+5 do campo `visible`) elimina a multiplicação da varredura; o resto do corpo só roda para os espaços com `visible&2`. **28,00 → 28,67 fps (+2,4%)**.
2. **`HillSelect`** em Z80 (`HillSelectAsm`): 12 registros de 16 bytes (11 em `world_map+9600` + `enemy`), subtrações de 16 bits com `sbc`, `dy` só quando `dx` não decide, e o preenchimento de `wall_key` por `ldir`; usa `ca_x/ca_r/ca_wx/ca_wy` (do anel, sem uso nessa hora) como rascunho para não gastar RAM. `HillAnyTater` evita o laço de `HillTick` quando nenhum Tater está ativo. **28,67 → 29,73 fps (+3,7%)**.
3. **`HillCollect`** em Z80 (`HillCollectAsm`): janela de visibilidade com soma/subtração de 16 bits sem sinal, deslocamento de 24/4 px nos Taters. **29,73 → 30,37 fps (+2,2%)**.
Verificação por oráculo no emulador (`BENCH_HSDIAG`/`BENCH_HCDIAG` em `mkprof.py`: o Tcl guarda a entrada no ponto de entrada da rotina e recalcula o resultado pela regra original ao retornar): `HillSelect` 42 300 registros / 63 mudanças de estado / 0 divergências (varredura da câmera) e 29 904 em caminhada; `HillCollect` 24 112 + 20 878 registros (334 visíveis) / 0 divergências.
Medição (Hill, 30 s): 5,37 MHz **28,00 → 30,37 fps**; 7,16 MHz 37,10 → 37,50; 3,58 MHz 11,13 → 12,93; PAL 5,37 MHz 31,1. `attrcheck` 0 corrompidas (Hill nos 3 clocks, PAL, fases 2–12 a 5,37 MHz); barra com 0 pixels diferentes (8 capturas NTSC e PAL); volta ao mapa OK.
Observação: a métrica conta renders por segundo e passa de 30 porque há quadros só de herói; o teto real de quadros completos é o vblank (30 fps NTSC).

## Outras fases a 5,37 MHz — medição andando (0.9.75 × 0.9.83, `walkall.sh`)

Roteiro: o Keen vai e volta a cada 1,5 s a partir do ponto de entrada de cada fase, 30 s (fps = renders/s). Só valem as fases em que o Keen sobrevive à janela:

| Fase | 0.9.75 | 0.9.83 | Observação |
|---|---|---|---|
| 1 Hill | 28,4 | 37,8 | sobrevive nas duas |
| 2 Mines | 8,5 | 10,7 | sobrevive |
| 3 Bridge | 10,0 | 13,9 | sobrevive |
| 7 Apple | 9,4 | 9,3 | sobrevive; sem ganho |
| 8 Grove | 6,9 | 5,0 | andando, o Keen quase não sai do início e os trajetos divergem; parado no ponto de entrada (repetível): 7,00 → 6,87 |
| 9 Bay | 8,0 (parado) | 8,0 | morre andando nas duas |
| 12 Castle | 9,2 | 12,8 | a 0.9.75 morre 3 vezes na janela: não comparável |
| 4, 5, 6, 10, 11 | – | – | o Keen morre em 2–3 s no roteiro (e parado), nas duas ROMs: sem medição válida (fase 10: 0 renders) |

Conclusão: o piso de 30 fps a 5,37 MHz **só vale no Hill**. Nas demais fases o desenho de atores/cenário domina e o jogo roda a 5–14 fps; os ganhos desta série (ActorsDraw, trampolina de banco, Columns) rendem +25–40% nas fases 2, 3 e 12, e nada nas 7, 8 e 9. Fase 8 (Grove) parada: ≈7 fps com 9% do VDP ocupado — o gargalo é CPU, não VDP.
Limite do método: parado/andando em janelas curtas e sem rota de jogo real; fases com mortes súbitas precisam de roteiro que evite os inimigos.

## Outras fases a 5,37 MHz — andando e pulando (0.9.75 × 0.9.83, `jumpall.sh`)

Roteiro: o Keen vai e volta a cada 1,5 s e segura/solta o pulo a cada 0,5 s (`BENCH_JUMPT=0.5` em `mkprof.py`), janela de 25 s, fps = renders/s. Com os pulos o Keen sobrevive na maioria das fases (nas 4, 5, 6, 9, 10 e 11 o contador `defeats` do roteiro marca 4–6 mas `lives`=3 e o Keen termina perto do ponto de entrada: ele morre e reaparece; só a fase 12 termina com 0/255 vidas).

| Fase | 0.9.75 | 0.9.83 | ganho |
|---|---|---|---|
| 1 Hill | 22,4 | 32,6 | +45% |
| 2 Mines | 7,5 | 9,6 | +28% |
| 3 Bridge | 12,9 | 18,3 | +42% |
| 4 Rapids | 15,8 | 23,8 | +51% |
| 5 Pass | 16,4 | 24,0 | +46% |
| 6 City | 16,2 | 26,5 | +63% |
| 7 Apple | 8,0 | 8,5 | +5% |
| 8 Grove | 6,3 | 5,0 | −21% (ver nota) |
| 9 Bay | 11,0 | 14,4 | +30% |
| 10 Swamp | 19,4 | 26,6 | +37% |
| 11 Boobus | 16,8 | 18,6 | +11% |
| 12 Castle | 5,8 | 6,6 | +14% (o Keen perde as vidas na janela; não confiável) |

Nota fase 8: os trajetos divergem (0.9.75 fica junto à parede esquerda, x≈40 px; 0.9.81/0.9.83 chegam a x≈160 px, com mais cenário e inimigos na tela), então a queda não prova regressão. Medição repetível com o Keen parado no ponto de entrada: 7,00 (0.9.75) → 6,77 (0.9.81) → 6,87 (0.9.83) fps, mesma posição final (x=768, y=8463) nas três ROMs.
Conclusão: a série dá +30–60% na maioria das fases, mas só o Hill passa de 30 fps; as fases 4, 5, 6 e 10 ficam em 24–27 fps, as 3 e 9 em 14–18, e as fases 2, 7, 8 e 12 continuam abaixo de 10 fps.

## Perfil da fase 8 (Grove) a 5,37 MHz — Keen parado no ponto de entrada (0.9.83)

Cenário repetível (resultado idêntico em duas execuções): 6,87 fps, `DrawScene` 66 ms por chamada (45% da janela), `ActorsPrepare` 48 ms, `CombatTick` 3,9 ms × 2099 chamadas (27%), VDP só 4% ocupado: o gargalo é CPU. Amostragem de PC (42 785 amostras):
| % | função / linha |
|---|---|
| 17,2 | `LineISR` — 13,6 pontos no laço de espera do FH (timeout) |
| 19,8 | `GroveStatic` — 18,0 na linha `for(i=0;i<34;++i){bit=i<32?1UL<<i:…}` (34 bônus, deslocamentos de 32 bits e dois testes por volta) |
| 13,4 | `GroveTick` — 12,8 no laço de 39 atores só para testar `a->active` |
| 6,8 | `GroveStaticPut` (teste de visibilidade por pickup/bônus) |
| 5,5 | `GroveSelect` — laço de 39 atores com `>>4` e subtrações de 16 bits |
| 5,2 | `Game_Tick2` |
| 2,8 | `GroveCollect` |
| 2,6 | `VDP_CommandWait` |
Só os 3 laços de Grove (`GroveStatic` 18%, `GroveTick` 12,8%, `GroveSelect` 5,5%) somam ≈36% do tempo; todos são varreduras de listas de itens que quase nunca mudam.
Achado extra: a `LineISR` gastava 17% porque, com o quadro lento, a interrupção de linha chegava atrasada e a rotina entrava (na verdade no vblank, com FH ainda marcado) e girava até 256 voltas esperando um FH que não viria mais. 0.9.84-lineisr2: no caminho com FH marcado lê-se VR (S#2 bit 6) e abandona se estiver no vblank, e o limite do laço caiu de 256 para 120 voltas (≥2× a espera legítima a 5,37 MHz). `LineISR` 17,2% → 4,6% na fase 8; `DrawScene` 66 → 53 ms por chamada; fps medido 6,87 → 6,97 (a métrica só conta quadros apresentados; com o quadro lento ela é limitada pelo que muda na cena). Hill, PAL e 7,16 MHz inalterados (30,42 / 31,08 / 37,73 fps; barra com 0 pixels diferentes em 16 capturas cada; `attrcheck` 0 corrompidas). SHA256 `1e6ef503128db5ff9230f0264e11686400e667c47320c3c55e0dd0c4aa7fe07a`.

## 0.9.85-grove — os três laços de Grove em Z80

SHA256 `580f860d143628b0d19d9a730f10c7f9b1a9464373b1811a271c816955d54465` (inclui a correção da `LineISR` da 0.9.84).

1. `GroveTick`: `GroveNext(i)` (Z80) devolve o primeiro ator ativo com índice ≥ i entre os 39 registros de 16 bytes; o laço em C só roda o corpo para os ativos (a verificação é refeita depois de cada corpo, como no teste `if(!a->active)continue` original).
2. `GroveSelect`: `GroveSelectAsm` — janela por ator com subtração de 16 bits sem sinal (manter ativo: −96 ≤ x ≤ 352, −96 ≤ y ≤ 288; ativar: −48 ≤ x ≤ 304, −48 ≤ y ≤ 240; `state==1` força ativo); `EnemyCache()` continua em C.
3. `GroveStatic`: `GroveBonusList(idx)` (Z80) devolve os índices, em ordem crescente, dos 34 bônus ainda não coletados (bits de `bonus_taken` + `bonus_high`) que podem aparecer na tela segundo um filtro **superconjunto** do teste exato (−382 < dx < 256, −382 < dy < 192; o teste exato continua em `GroveStaticPut`, que descarta o resto); evita os 34 deslocamentos de 32 bits por chamada.
Verificação por oráculo no emulador (`BENCH_GVDIAG` em `mkprof.py`: o Tcl guarda a entrada, recalcula pela regra original e compara na volta): `GroveSelect` 56 472 + 35 412 registros / 31 mudanças de estado / 0 divergências; `GroveNext` 10 771 + 8 899 chamadas / 0; `GroveBonusList` 1 448 + 908 chamadas, 1 622 itens listados / 0.
Medição (fase 8, 5,37 MHz, 25 s): parado no ponto de entrada, `DrawScene` 53 → 22,6 ms por chamada (739 chamadas em 30 s contra 284), `CombatTick` 3,35 → 1,78 ms; a métrica de renders não muda parado (7,00 fps) porque só conta quadros que mudam. Andando: 17,8 fps (antes 4,97); andando e pulando: **15,9 fps (0.9.75: 6,3; 0.9.83: 5,0)** — ≈ 2,5× a 0.9.75. Fases 7/9/12 sem mudança (8,4 / 14,4 / 6,6). Hill 30,42, PAL 31,08 fps; `attrcheck` 0 corrompidas nas fases 2–12; barra 0 pixels diferentes; volta ao mapa OK.

## 0.9.86-static — varredura dos itens estáticos nas fases 7, 9, 12 (e 8)

SHA256 `c570513056a0846173a04cdd069c136c0e4c89bb689df0548e943ab5993e67b8`.

Perfil a 5,37 MHz andando e pulando (fases 7, 12, 9 da 0.9.85): o padrão de Grove se repete. Fase 7 (Apple): `AppleStatic` 18,9% + `AppleStaticPut` 6,7% (laço de 36 bônus com deslocamentos de 32 bits; laço de picapes com `bit<<=1` em 32 bits); fase 12 (Castle): `CastleObjectsTick` 6,3, `CastleStatic` 6,3, `CastleVisible` 6,2, `CastleTick` 4,5; fase 9 (Bay): `BayStatic` 9,8 + `BayStaticPut` 5,2 (o resto é `VPF`/`VDP_CommandWait` e a tela de GAME OVER, pois o Keen morre na janela).
Mudança: a rotina Z80 de Grove virou `BonusList(idx)` no banco fixo (main.c, ≈110 bytes; folga do banco fixo 207 → ~90 bytes), compartilhada pelas quatro fases. Parâmetros nas variáveis de rascunho do anel (`ca_x` = tabela, `ca_r` = nº de itens, `ca_wx` = tamanho do registro, `ca_wy` = endereço da máscara de 32 bits): serve para os bônus (registros de 5 bytes, `bonus_taken` + `bonus_high`) e para os picapes (6 bytes, `pickup_taken`; os índices ≥ 32 continuam sendo colocados sempre, como no laço original, onde `bit<<=1` zera em 32 bits). O filtro de tela é o mesmo superconjunto do teste exato de cada `*StaticPut`, que continua decidindo. Aplicado a `GroveStatic`, `AppleStatic`, `BayStatic` e `CastleStatic` (bônus e picapes).
Oráculo (`BENCH_GVDIAG` estendido): bônus e picapes das fases 7, 8, 9 e 12 em 40 s andando e pulando a 7,16 MHz — 2 418 / 2 996 / 1 806 / 2 444 chamadas, 16 001 / 4 555 / 7 224 / 6 719 itens listados, **0 divergências**.
Medição (andando e pulando, 25 s, 5,37 MHz; 0.9.75 → 0.9.85 → 0.9.86): fase 7 **8,0 → 8,4 → 15,0** (+88% sobre a 0.9.75); fase 8 6,3 → 15,9 → 16,4; fase 9 11,0 → 14,4 → 15,4; fase 12 5,8 → 6,6 → 7,4 (o Keen perde as vidas na janela). Hill 30,42, PAL 31,08 fps; `attrcheck` 0 corrompidas nas fases 2–12; barra 0 pixels diferentes; volta ao mapa OK.
Restam na fase 7: `Game_Tick` 6,2%, `AppleObjectsTick` 4,4% (laço de 6 itens com multiplicações), `BonusTick` 4,3%, `AppleCollect` 4,1%, `cart_cache_valid` 3,9% e ≈9% em `MapTile` (núcleo e cópia em RAM); na 12: `CastleObjectsTick` 6,3%, `CastleTick` 4,5%, `CastleVisible` 3,4%, `DictionaryMapTile` 3,9% (a 12 usa o mapa em dois níveis).

## 0.9.87-castle — Castle, divisões por tick e `BonusList` mais rápida

SHA256 `ffe98cc1e13112440d9bcd7d84df9aa34aea2c56f4bb71d6aae92dc3166fb205`.

1. **Castle** (`castle.c`): `CastleNext(i)` e `CastleSelectAsm` (mesmas rotinas de Grove, base `world_map+7200`, 17 atores) em `CastleTick` e `CastleSelect`; `CastleObjectTouch` com a janela horizontal num único teste sem sinal (−255 ≤ d < w em vez de duas comparações com sinal). Oráculo (`BENCH_GVDIAG` com base/contagem/bancos parametrizados): `CastleSelect` 22 270 + 20 961 registros, 11 mudanças de estado, 0 divergências; `CastleNext` 3 886 + 3 334 chamadas, 0. Fase 12: 7,4 → 9,5 fps.
2. **Divisões**: `__divuint`/`__moduint` (≈1000 T cada, sem instrução de divisão) eram chamados ≈360×/s na fase 7 (`simulation_ticks%20`, `simulation_ticks/10&1`). `Mod40(t)` (Z80, banco fixo, ≈30 bytes; 256 mod 40 = 16, depois subtração de 40·2^k; exaustivamente correto para todos os 65 536 valores) e `ticks.h` (`Tick20Zero`, `TickTenthOdd`) substituem as 7 ocorrências de `%20` (Apple, Castle, City, Grove, Rapids, Pass, Swamp) e a de `/10&1` (Apple). `keydoor.c` não foi alterado (também é compilado no núcleo isolado `core15`, que não liga `main.c`). `__divuint` 5,0% → 3,4% na fase 7; restam divisões por `timer/20` e `%3`/`%4` em `AppleActor`/`AppleMove`.
3. **`BonusList`** (8,4% do tempo na fase 7): o teste de `x` passou a vir primeiro (um `sbc` contra `camera_px−382` e comparação por bytes), depois o de `y`, e só os poucos candidatos pagam o teste do bit `taken`. Oráculo: fases 7/8/9/12 a 7,16 MHz, 2 624 / 3 272 / 1 958 / 2 682 chamadas, 0 divergências; fase 7 a 7,16 MHz 20,2 → 22,3 fps.
Medição (andando e pulando, 25 s, 5,37 MHz; 0.9.75 → 0.9.87): fase 7 **8,0 → 16,4**; fase 8 **6,3 → 18,4**; fase 9 11,0 → 15,4; fase 12 5,8 → 9,5 (o Keen perde as vidas na janela). Hill 30,42 / PAL 31,08 fps; `attrcheck` 0 corrompidas nas fases 2–12; barra 0 pixels diferentes em 20 capturas (uma captura isolada de 8 mostrou 19 pixels diferentes numa varredura e não se repetiu em duas varreduras de 10); volta ao mapa OK.
Fase 3 (Bridge): os trajetos divergem entre ROMs (a partir da 0.9.81 o Keen cai para outra região) e o fps andando varia entre 11,8 e 18,3 sem relação com as mudanças; a medição repetível dessa fase precisa de roteiro com posição controlada.

## 0.9.88-fix — defeito de congelamento na barra (0.9.76–0.9.87) e roteiro de posição controlada

SHA256 `8f6877e7d4a8ec51a6838c611791527595b62bc8b10704486355c1b1111b26bb`.

**Defeito corrigido (grave).** O novo roteiro (abaixo) travou a fase 5 com a 0.9.87: a máquina entrava em tempestade de interrupções (PC preso na ISR da BIOS e no gancho H.KEYI; o jogo parava de andar). Causa: corrida na barra. `BarOn` escrevia R#0 (IE1) **antes** de `bar_on=1`; se o FH estivesse marcado de quadros anteriores (o R#19 continua casando linhas mesmo com IE1 desligado), a interrupção de linha chegava com `bar_on==0`, a `LineISR` saía sem ler S#1, o FH ficava marcado, a linha de interrupção continuava ativa e o processador só executava a ISR. Era uma janela de poucos ciclos que dependia da fase do quadro (a troca de `simulation_ticks%20` por `Mod40` em `pass.c` mudou o tempo e a expôs na fase 5), mas existe desde a 0.9.76 e podia congelar o jogo em hardware real.
Correção: (1) `BarOn` grava `bar_on=1` e só depois R#0; `BarOff` desliga R#0 e só depois zera `bar_on`; (2) a `LineISR`, com a barra desligada, passa a ler S#1 (reconhece o FH) antes de sair; R#15 volta a 0 como nos outros caminhos. Verificação: fase 5 em modo invulnerável 70 s sem travar (antes: travava em 15–30 s com `Mod40` em `pass.c`); todas as fases passam a produzir quadros (renders > 0) no teste de capturas; Hill 30,42 / PAL 31,04 / 7,16 MHz 37,73 fps; barra com 0 pixels diferentes (8 capturas × 3 configurações); `attrcheck` 0 corrompidas; volta ao mapa OK.
Observação de método: as verificações `attrcheck`/capturas feitas com a 0.9.86–0.9.87 na fase 5 usaram arquivos de execuções anteriores (a execução não produzia quadros e não sobrescrevia as capturas); agora cada execução é conferida por `renders`.

**Roteiro de posição controlada** (`BENCH_GOD`, `BENCH_TRACK`, `BENCH_CKPT`/`BENCH_SEG` em `mkprof.py`; `trackall.sh`, `ckall.sh`, `ckpts.json`): o Keen é invulnerável (escrita em `death_ticks` é desfeita por um watchpoint), uma corrida de 90 s na 0.9.75 grava a posição a cada 2 s, e dessas posições saem 4–14 pontos de parada por fase (`ckpts.json`). Cada medição teleporta o Keen a cada ponto (velocidade zero) e o faz andar e pular por 3 s; o fps é renders/s por segmento e o total é renders ÷ tempo. A teletransporte refaz o anel a cada ponto, então os valores absolutos são menores que andando de verdade, mas a comparação entre ROMs é repetível. Resultado a 5,37 MHz (0.9.75 → 0.9.88):
| Fase | 0.9.75 | 0.9.88 | |
|---|---|---|---|
| 1 Hill | 9,1 | 14,9 | +63% |
| 2 Mines | 6,8 | 7,4 | +9% |
| 3 Bridge | 5,1 | 6,1 | +21% |
| 4 Rapids | 6,8 | 7,2 | +6% |
| 5 Pass | 10,1 | 12,0 | +18% |
| 6 City | 4,6 | 5,9 | +29% |
| 7 Apple | 6,7 | 10,9 | +63% |
| 8 Grove | 3,7 | 8,5 | +129% |
| 9 Bay | 8,7 | 11,2 | +29% |
| 10 Swamp | 7,0 | 11,0 | +57% |
| 11 Boobus | 4,6 | 4,7 | +2% |
| 12 Castle | 6,3 | 10,3 | +65% |
(Fase 3: o último ponto não renderiza em nenhuma ROM — posição inválida; fase 10: o primeiro ponto também.) As fases com pouco ganho são justamente as que ainda não receberam a varredura em Z80: **2 (Mines), 3 (Bridge), 4 (Rapids), 6 (City) e 11 (Boobus)**.

## Perfil das fases 2, 3, 4, 6 e 11 (posição controlada) e 0.9.89-lists

SHA256 `b5471117440a1b82aa9e6d797f02d22ac3aefa0bdd28ff2fa7cc339e7a3b0c9f`.

Perfil a 5,37 MHz nos pontos de parada (`pq*`; janela total / chamadas):
- **Fase 2 Mines** (7,6 fps): `RingScene` 24,9% da janela (50 ms por chamada, VDP 12% ocupado), `DrawScene` 43%; `CombatTick` 16%; por amostra: `Game_Tick` 6,5, `CartStep` 5,4, `MapTileKernel` 5,0, `Rows` 4,9, `cart_cache_valid` 4,1, `MinesTick` 3,4, `CartContact` 2,8, `KeyDoorTick` 2,7.
- **Fase 3 Bridge**: o ponto de parada da saída acaba a fase (`LevelComplete`/`WorldMap` aparecem no perfil); `VDP_CommandWait` 12,5%, `BridgeTick` 2,3.
- **Fase 4 Rapids** (7,3 fps): `RapidObjectsTick` 9,8%, `RapidTick` 4,1, `RapidObjectTouch` 3,5, `VDP_CommandWait` 8,2.
- **Fase 6 City** (5,7 fps): `CombatTick` 31,8% da janela (4,6 ms × 3 051), `RingScene` 28%; `CityTick` 6,7, `DictionaryMapTile` 6,1, `CityObjectsTick` 4,5.
- **Fase 11 Boobus** (4,6 fps): `RingScene` 56,5% (141 ms por chamada), `VDP_CommandWait` 26,8%, `BoobusTick` 12,4, `VPF` 10,3, `BoobusTexture` 4,7; VDP 37% ocupado.

Mudanças (0.9.89): `ActorNext` (Z80, cópia por módulo em `actornext.inc`) nos laços de atores de Mines, Bridge, City e Rapids; `ObjTouch` (Z80, `objtouch.inc`) nos testes de chaves/portas/bombas de Rapids, Pass, Castle (e nas bombas de Grove/Swamp). A fase 6 (`s50`, banco com 87 bytes livres) ficou sem `ObjTouch` nem Apple: o banco estourava em 91 bytes; o banco fixo tem 12 bytes livres (por isso as rotinas são cópias por módulo, não compartilhadas). Oráculos: `ActorNext` 3 796 / 2 654 / 10 608 / 6 039 chamadas (Mines/Bridge/City/Rapids), 0 divergências; `ObjTouch` 13 266 (8 verdadeiros) / 7 043 (6) / 3 811 (1) chamadas em Rapids/Castle/Pass com o Keen posto sobre os objetos, 0 divergências.
Medição com posição controlada, agora com regime estável (renders/s depois de 1,5 s de cada teletransporte, 0.9.75 → 0.9.89): fase 1 10,2 → 17,5; fase 2 7,0 → 8,2; fase 3 3,6 → 5,1; fase 4 6,7 → 7,5; fase 6 4,9 → 5,1; fase 11 4,0 → 3,9. Os ganhos de lógica são pequenos nessas fases porque o quadro é dominado pelo anel (`RingScene` 25–56%, VDP 12–37%) e, na 6, pelo `CombatTick`. Regressão: Hill 30,42 / PAL 31,04 fps; barra 0 pixels diferentes; `attrcheck` 0 corrompidas com quadros em todas as fases (renders > 0); volta ao mapa OK; fases 5 e 12 sem congelar em 50 s invulnerável.

## 0.9.90-rows — `Rows` em Z80

SHA256 `962709e160a9df861a7b246c160420d6f529c44a6eefc2e2f401245c8678bf4b`.

`Rows` (desenho das linhas expostas do anel e do redesenho completo) virou `RowsAsm`: mesma lógica do ramo `FAST_RING_COPY=0` do C (altura por linha, largura/offset do tile, fusão horizontal de tiles iguais na cópia, fusão de cor sólida no Hill com `HMMV`, `RepeatStrip` horizontal chamado de volta em C por `RowsRepeat`, comandos `SetupR32`/`SetupR36`). Sem RAM nova: reaproveita as variáveis do anel (`ca_*`, `ca_w` serve de limite inferior) e guarda `l`, `wx0` e `w` (que chega a 256) num quadro de pilha apontado por IX.
Verificação (ROM de diagnóstico com a versão em C e a em Z80 chamadas em sequência com os mesmos argumentos; `BENCH_COLDIAG` + `BENCH_COLEXITALWAYS` + `cdcmp.py`): fluxos de comandos de VDP idênticos em 11 fases — 172 / 2 / 139 / 38 / 108 / 110 / 2 / 2 / 27 / 34 / 6 chamadas (fases 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12), ≈ 290 mil bytes comparados. A única diferença são 2 chamadas (fases 4 e 6) em que o registrador CLR de um comando de cópia (`HMMM`, que o ignora) traz um valor antigo diferente (136 contra 187): resto de uma cor de preenchimento anterior, sem efeito.
Medição (posição controlada, regime estável, 5,37 MHz, 0.9.89 → 0.9.90): fase 6 4,7 → 6,1 (+29%); fases 2, 4 e 11 sem mudança (≈ 8,2 / 7,5 / 3,9: nelas o anel quase só usa `Columns`); fase 1 14,9 → 14,2 no teletransporte, mas 30,37 fps andando (era 30,42) e `RingScene` 35,5 → 33,0 ms por chamada no perfil do Hill (587 → 614 renders em 44 s). Hill 30,37 / PAL 31,27 / 7,16 MHz 37,73 fps; barra 0 pixels diferentes (8 capturas × 3 configurações); `attrcheck` 0 corrompidas; volta ao mapa OK.
Pendência: numa execução da fase 5 (Keen morto, 25 quadros) a captura de t=15 s mostrou o quadro com a configuração da barra (página 2 e rolagem da barra) em vez da do jogo; nas 4 repetições posteriores (mesmos comandos e outros) e na 0.9.89 isso não ocorreu. Parece um quadro transitório na transição de morte/`BarOff`; não reproduzi, então não está resolvido nem atribuído ao `Rows` (a ROM sem `Rows` em Z80 também passou limpa, mas uma amostra não prova ausência).

## Varredura do CombatTick da fase 6 — resultado negativo (sem nova ROM)

Perfil (5,37 MHz, posição controlada): `CombatTick` ≈ 23–25% da janela, mas já sem um ponto dominante: `CityObjectsTick` 4,7%, `CityTick` 2,4%, `CombatTick` próprio 2,2%; o resto do quadro é plano (`VDP_CommandWait` 7,6, `DictionaryMapTile` 5,9, `Game_Tick2` 5,5, `LineISR` 4,6, `MapTileKernel` 3,9, `ColumnsAsm` 3,6).
Tentativa: `CityObjectsTick`/`AppleObjectsTick` com o teste de toque como dois testes sem sinal (x primeiro) e divisão `/20 %3` içada em `CityCollect`. O oráculo (`BENCH_BTDIAG` em `mkprof.py`) achou 88/2678 divergências: **bug do SDCC** — depois de carregar IY com `&player_x`, o compilador reutilizou IY como se ainda apontasse para a linha de spawn e leu o byte Y errado. Corrigido lendo `sx`/`sy` antes; o oráculo passou a dar 0 divergências (Apple 0/1512; City 1/2400, uma corrida com o teletransporte do roteiro).
Medida: `CityObjectsTick` 0,88 ms (original) → 0,91 ms por chamada (sem ganho: o custo está no preâmbulo do laço — índice da tabela de spawn, `city_rise` — e não nos testes, que quase sempre saem no x); `CityCollect` 7,5 → 5,4 ms (6 chamadas/s, ≈1% do quadro). fps no roteiro: 6,1 → 5,7 (variação do roteiro, que é caótico por segmento; a fase 7, sem mudança, dá 10,94 igual). Conclusão: **não gerei ROM nova**; a 0.9.90-rows continua a última. Melhorar a fase 6 exige reduzir o custo estrutural (anel/VDP, mapa de dois níveis), não a lógica de bombas.
Ajuste de ferramenta: `BENCH_GOD=2` também desfaz `restart_pending` e repõe 3 vidas (a invulnerabilidade antiga deixava o Keen chegar a GameOver em algumas ROMs e distorcia o fps).

## Custo do anel e do VDP na fase 6 (análise, sem mudança de código)

Perfil a 5,37 MHz (roteiro de posição controlada, 17 s, 84 quadros, VDP 19% ocupado): `RingScene` 34% da janela (69 ms por chamada), dos quais `RowsAsm` 6,2% (29 redesenhos completos, quase todos causados pelos teletransportes do roteiro), `ActorsDraw` 10,5%, `SlotScan` 0,6%, `CleanSave` 0,3%; o resto (~16%) são as restaurações de retângulos sob os atores (`RectX`, embutida) e as esperas do VDP.
Amostragem de PC (24 243 amostras): `VDP_CommandWait` 7,6%, `DictionaryMapTile` 5,9%, `MapTileKernel` 3,9%, `ColumnsAsm` 3,6%, `VPF` 1,7%, `RepeatStrip` 1,4%, `VDP_CommandSetupR32` 1,4%, `RingScene`+`RowsAsm`+`RectX`+`ActorsDraw` 4,7% — ≈30% no total, sem nenhum item acima de 8%.
Candidatos e limites: (1) busca de tile em dois níveis (`DictionaryMapTile`+`MapTileKernel`, 9,8%): o custo é o laço de deslocamento variável de 9 bits (≈100 T por busca); um salto calculado para uma sequência desenrolada economizaria ≈45 T, mas não é posição-independente (o `MapTileKernel` é copiado para a RAM) e o banco fixo tem só 12 bytes livres — ganho estimado ≤2–3% do quadro; (2) sobrepor lógica de jogo às cópias do VDP em vez de esperar (`VDP_CommandWait` 7,6%): teto de ≈7%, mas exige reestruturar a ordem do quadro (risco alto de corrupção do anel). Nenhum dos dois move a fase 6 de ≈6 fps de forma perceptível; não implementei.

## Sobreposição VDP/lógica na fase 6 — medida do teto (sem mudança de código)

Antes de reestruturar o quadro medi onde a CPU espera o VDP: das 24 243 amostras de PC, 1 870 (7,7%) estão em `VDP_CommandWait`; pelo endereço de retorno na pilha, **67% delas (≈5,2% do tempo) vêm de dentro de `VDP_CommandSetupR32`** (a espera pelo comando anterior antes de gravar o próximo, nos laços de `ColumnsAsm`/`RowsAsm`/restaurações, onde a CPU já termina a preparação antes do VDP) e só ≈29% (≈2,3% do tempo) de esperas isoladas no fim do `DrawScene`/`ActorsDraw`/`HudDraw`.
Consequências: (a) a espera dentro dos laços é limite de vazão do VDP (cópias HMMM em SCREEN 5); a CPU só poderia aproveitá-la rodando lógica de jogo intercalada nos laços (corrotina), o que não cabe no desenho atual; (b) a espera final antes de `HWFlip` poderia ser adiada para o `SoundISR` (que adiaria o flip enquanto S#2 bit CE estivesse ligado), mas o ganho máximo é ≈2% do quadro, com risco de flip sobre cópia incompleta (tearing/corrupção) — não implementei. A fase 6 fica limitada pelo VDP e pelo anel; ≈6 fps a 5,37 MHz é o piso atual.

## 0.9.91-flip — flip de vblank adiado no `SoundISR`

SHA256 `801f911f9138b81be804b6e7da0f03115ade4de6e0040ca25fc630080b8171c4`.

O `DrawScene` não espera mais o VDP terminar as últimas cópias antes de pedir o flip (`VDP_CommandWait` antes de `HWFlip` removido). O `SoundISR` agora lê S#2 (R#15=2, restaura S#0) e, se o bit CE (comando em execução) estiver ligado, deixa `flip_flag` pendente para o próximo vblank; o `DrawScene` seguinte já espera `flip_flag` zerar antes de tocar na página. Assim a lógica do jogo roda enquanto o VDP termina a cópia.
Banco fixo: o ISR cresceu 23 bytes (só tinha 12 livres, o link falhou com "Data overwrite"); liberei ≈190 bytes desligando `VDP_USE_VRAM16K` em `msxgl_config.h` (funções de VRAM de 16 K da MSXgl, sem uso no jogo). `city.c` leva só o içamento de `anim` (sem mudança visível).
Verificação: oráculo `BENCH_FLIPCHK` (`mkprof.py`): breakpoint no ponto em que o ISR aplica o flip lê S#2 — 628 flips (fase 6) e 621 (fase 12) com 0 flips sobre comando em execução; a deferência em si nunca disparou nessas corridas (o VDP termina antes do vblank seguinte a 5,37 MHz). Barra 0 pixels diferentes em Hill/Pass/Castle e PAL; volta ao mapa OK; fases 1, 5 e 12 andando 50 s invulnerável sem congelar; capturas sem corrupção.
Medida (5,37 MHz): roteiro de posição controlada fase 6 6,12 → 6,26 fps, fase 11 ≈4,0 → 4,3; andando (50 s): Hill 38,06 → 38,42, Pass 8,74 → 8,76, Castle 12,20 → 14,56 (trajetória diferente: 3551 contra 4552 comandos de VDP, não comparável); PAL Hill 41,13 → 41,07. Ganho real ≈ 1–2%, como o teto previsto.
Limites: só openMSX; nenhuma espera de E/S do VDP de hardware real foi medida. O teste `shots.sh` aqui devolveu capturas em branco (não usado como evidência).

## Busca de tile em dois níveis — resultado negativo (sem nova ROM)

Reescrevi `MapTileKernel`/`CityMapTile`/`CastleMapTile` (`world.c`): extração de 9 bits por rotina comum `Shift9` (deslocamento à direita para offsets 0–3, à esquerda no registrador de 24 bits A:D:E para 4–7, no máximo 4 passos), e no segundo nível `offset = 9·d + r` e deslocamento `r` direto (sem recalcular `j>>3`). Banco fixo não era mais limitação (≈190 bytes liberados na 0.9.91).
Verificação: modelo exaustivo da extração (65 536 palavras × 8 deslocamentos) 0 erros; oráculo no emulador (`BENCH_MTORC` em `mkprof.py`: breakpoint na entrada, confere `DE` no endereço de retorno contra a fórmula original, ponto de saída de uso único) 25 441 chamadas na City, 25 859 no Castle, 49 947 no `MapTileKernel` da fase 1, 0 divergências. (Primeira versão do oráculo dava falsos positivos que a ROM antiga também tinha — o breakpoint de retorno disparava de novo em laços; corrigido.)
Medida (5,37 MHz, roteiro controlado, nova × 0.9.91): fase 6 5,93 × 6,26; fase 12 9,53 × 9,67; fase 1 14,00 × 14,33; fase 2 7,63 × 7,63; fase 11 4,67 × 4,33 — **nenhum ganho** (dentro do ruído, tendendo a pior). Motivo: o custo fixo da nova rotina (cp/ramo/`inc b`/`jr` ≈ 34 T de preparação + `jp` ≈ 10 T) anula a economia nos offsets 0–3, que são metade dos casos; só os offsets 5–7 ficam mais rápidos. A conta de cabeça de −40 T por busca ignorava essa preparação.
Revertido: `world.c` volta ao da 0.9.91-flip (que continua a ROM mais recente). O oráculo `BENCH_MTORC` fica como ferramenta.

## 0.9.92-bombas — laço de bombas em Z80 (fases 6 e 7)

SHA256 `4d9373582f90f81a952fba24be1bdbb8d915a2b0815cc42d358b096a07849c9a`.

`CityObjectsTick`/`AppleObjectsTick` agora chamam uma rotina Z80 (`CityBombsAsm`/`AppleBombsAsm`, uma por módulo, no banco 50): contagem regressiva de `*_rise` e teste de toque das 6 bombas com a mesma caixa do C (−255 ≤ px+64−x ≤ 255 e −447 ≤ py+48−y ≤ 255, x,y = tile·256), em duas comparações de byte alto (o x primeiro: quase todas saem aí), sem multiplicações, sem o preâmbulo de 6 iterações do SDCC. (A tentativa anterior em C só trocava as comparações e não ganhou nada: o custo era o preâmbulo e a indexação da tabela.)
Verificação: oráculo `BENCH_BTDIAG` com o Keen posto sobre as bombas: City 2 724 / 3 280 / 2 539 chamadas e 6 + 6 + 6 coletas, Apple 3 079 / 1 880 chamadas e 6 + 3 coletas, **0 divergências** (valores de `rise`, máscara de bombas e contador). Modelo exaustivo da lógica de bytes contra a fórmula original sobre todo `player_x`/`player_y` de 16 bits: a única diferença é o caso `d = 0x8000` exato, em que o C original (`(i16)` sem sinal estourado) contava toque a 32 768 de distância — só ocorre com o Keen em x=960 *e* y≈a linha da bomba, posição que a fase não permite; a nova rotina é a correta.
Medida (5,37 MHz): `CityObjectsTick` 0,91 → 0,42 ms por chamada; fase 6 no roteiro controlado 6,26 → 7,21 fps e andando 50 s 12,36 → 13,58; fase 7 no roteiro 10,89 → 11,72 e andando 16,46 → 17,58. Fases 1, 5, 12 inalteradas (38,42 / 8,76 / 14,56 andando). Flip adiado: 0 flips sobre comando em execução (5 fases, 50 s); barra igual à da 0.9.91 (Hill/Castle 19, Pass 31–34 como antes); volta ao mapa OK; sem capturas corrompidas.
Limites: só openMSX; o roteiro de posição controlada é caótico por segmento (±5%), mas o ganho aparece igual no perfil por chamada e nas corridas andando.

## Perfil das fases 11 (Boobus) e 3 (Bridge) na 0.9.92 (5,37 MHz, posição controlada)

**Fase 11 (4,3 fps, VDP 38% ocupado).** `DrawScene` 66% da janela (153 ms por render); `RingScene` 56%; **`ActorsDraw` 41% = `BoobusBitmap` 37,6% (101 ms por chamada, 74 chamadas)**. `BoobusTick` 17% inclusive (2,44 ms × 1 394); `CombatTick` 20%. Amostragem de PC: `VDP_CommandWait` 27,4% (80% dela vem de dentro de `VDP_CommandSetupR32`: a CPU espera o VDP entre comandos), `BoobusTick` 12,4%, `VPF` 10,5% (envio da textura do chefe à VRAM), `BoobusTexture` 5,0%, `MapTileKernel` 4,2%.
- Gargalo principal: **o desenho do chefe** (sprite de 54–96 × 102–104 px). Cada render faz, por metade de 52 linhas, duas passagens `LMMM` (AND da máscara + OR da cor) sobre a área inteira (≈5,6–9,9 mil px × 2), mais o reenvio da textura (≈6 KB por VPF) a cada troca de quadro de animação e de página. É limite de vazão do VDP, não lógica.
- Oportunidade concreta (medida nos dados, ainda não implementada): ≈70% dos pixels de cada quadro são opacos (máscara = 0) e o **maior retângulo opaco alinhado a pixel par cobre 42–50% da área** dos quadros 108–115 (2 556–2 822 px, 71–83 linhas) e 31% do 116 (3 078 px). Esse retângulo pode ser copiado com **uma `HMMM` a partir da metade de cor da textura** (máscara 0 ⇒ AND+OR ≡ cópia da cor), deixando as passagens AND/OR só para as 4 faixas ao redor. `HMMM` é várias vezes mais rápida que `LMMM` por pixel; a economia estimada é da ordem de 40% do tempo de VDP do chefe (≈ 15–20% do render da fase 11). Os retângulos (x, y, largura, altura por quadro) saem de um cálculo offline sobre `boss_N.bin`, sem nova RAM.
- `BoobusTick`: laço de paredes (`BoobusWall` → `MapTile` + multiplicação de 16 bits por chamada, ≈ 600 T) rodando todo tick (`combat_dirty=1` incondicional força redesenho a cada tick) — candidato menor (≤ 5% do quadro), tipo o das bombas: pode virar Z80 com a tabela de colisão.

**Fase 3 (5,7 fps no roteiro; o perfil inclui trechos de `LevelComplete`/`WorldMap` dos pontos de saída, ≈ 25% das amostras, que descartei).** Normalizado ao que sobra: `VDP_CommandWait` ≈ 17% (74% dela dentro de `VDP_CommandSetupR32`), `VPF` ≈ 10%, `RowsAsm` 5%, `MapTileKernel` 4,7%, `Game_Tick2` 4%, `LineISR` 3,6%, `RepeatStrip` 3,2%, `InventoryTick` 2,8%, `BonusTick` 2,7%. **Sem gargalo de CPU localizado** — perfil plano; o tempo é anel + VDP + envio de texturas.

Conclusão: a fase 11 tem um gargalo parecido com o das bombas só no sentido de ser localizado e atacável (o desenho do chefe, 37% do tempo), mas a natureza é outra: não é lógica em C, é trabalho do VDP; a saída é reduzir os pixels em `LMMM`. A fase 3 não tem.

## Chefe da fase 11: cópia do retângulo opaco com HMMM — resultado negativo (sem nova ROM)

Implementei em `BoobusBitmap` (caminho de textura em cache, quadros 108–115): o maior retângulo opaco de cada quadro (tabela `boss_rect`, 42–50% da área) vai com **uma `HMMM` a partir da metade de cor**, e só as 4 faixas ao redor usam AND+OR (`BoobusPass`, laço com `y/52` e `y%52` trocados por comparação/subtração). Condições: sprite sem recorte, X de destino par.
Verificação de equivalência (`BENCH_BBORC` em `mkprof.py` + `bbcheck.py`): ROM de oráculo que executa a função duas vezes por chamada (antiga e nova, reentrando no ponto de entrada), registra cada comando de VDP e um modelo em Python (LMMM AND/OR, HMMM, fundo aleatório, textura montada dos `boss_N.bin`) aplica as duas listas: 98 chamadas, 67 com HMMM, **0 divergências**.
Medida (mesmo oráculo, tempo entre o primeiro e o último comando de cada versão, mesmas chamadas): **antiga 43,4 ms × nova 61,8 ms** — a nova é mais lenta. O VDP gasta ≈ 4–7 µs/px em LMMM e ≈ 2 µs/px em HMMM, mas dividir os 2 × 2 comandos em 14–20 (as faixas atravessam a divisão das duas metades de 52 linhas da textura) custa mais que a economia de pixels; o tempo entre comandos é dominado pela espera do VDP, não pela CPU (tirar as divisões não mudou nada). Também a fase 11 inteira não melhorou (3,95–4,06 × 4,3 fps no roteiro, dentro do ruído).
Revertido (`boobus_bitmap.c` volta ao da 0.9.92). Ficam as ferramentas (`BENCH_BBORC`, `bbcheck.py`). O caminho que ainda não foi tentado e que ataca o mesmo custo é o da própria sugestão original: converter os quadros do chefe para **TIMP em passe único** (metade dos pixels em LMMM e metade dos bytes enviados por `VPF` a cada troca de quadro), o que exige recolorir os pixels pretos opacos do chefe para o "segundo preto" (índice 1) como nas outras fases.

## Item "cobertura do TIMP nas fases 4–12": medição do teto — NÃO vale a reescrita (sem nova ROM)

Ferramentas novas em `mkprof.py`: `BENCH_CMDSTAT` (cada comando de VDP: tipo, pixels, tempo até o próximo comando e rotina chamadora via endereço de retorno; `cs2sym.py` resolve os nomes), `BENCH_FRAMESTAT` (histograma de quadros em `ActorDraw`), `BENCH_VRAMDUMP`; `timpwhy.py` reaplica a lógica de `build_timp_frames.py` e diz por que cada quadro desenhado não foi convertido.
**Onde o AND/OR ainda manda no VDP (5,37 MHz, posição controlada; fatia do tempo de comandos):** fase 8 Grove 75% (`ActorDraw`, AND), fase 4 Rapids 52% (+17% TIMP), fases 5 Pass e 9 Bay 47% (OR em `BroccoBitmap`, caminho próprio), fase 12 Castle 42%, fase 10 Swamp 26%, fase 7 Apple 17%, fase 6 City 11%, fase 11 Boobus 28% (+ HMMM de anel 65%).
**Por que os quadros não foram convertidos** (ponderado pelos desenhos): 60–80% dos desenhos em Rapids/Grove/Castle/Swamp caem em "retângulo de máscara/cor compartilhado ou sobreposto a outro quadro" — o atlas aproveita pixels idênticos entre retângulos que se cruzam, então converter no lugar corromperia o vizinho. Uma conversão da área de **cor** no lugar (com rascunho) elevaria a cobertura só em Apple (0 → 50% dos desenhos), City (+17%), Swamp (+17%), Grove (+13%), Rapids (+10%), Castle (0). O resto só se resolve **refazendo o empacotador do atlas** (12 construtores `build_*_assets.py`, sem área livre de VRAM: as 4 páginas estão ocupadas), uma mudança grande.
**O teto de ganho é pequeno:** a CPU só espera o VDP de 4 a 9% do tempo nessas fases (`VDP_CommandWait`: fase 4 7,4%, 5 4,0%, 8 8,6%, 9 6,1%, 10 8,4%, 12 5,4%; VDP ocupado 9–18% da janela). Mesmo eliminando todo o AND/OR, o fps subiria no máximo 4–9%; cortando metade, 2–4%. O TIMP já entregue reduziu bastante o *tempo de VDP* (−56% no caso medido), mas o gargalo de fps dessas fases é CPU, não VDP. Só as fases 11 (27%) e 3 (13%) esperam mais pelo VDP.
**CPU nessas fases (amostragem de PC, % do tempo):** fase 4 `RapidObjectsTick` 9,9; fase 5 `PassTick` 12,8 + `PassCollect` 3,4; fase 8 `GroveGrape` 6,2 + `GroveObjectsTick` 4,8 + `GroveNext` 3,5; fase 9 `BayStaticPut` 4,4 + `BayStatic` 3,6; fase 10 `VPF` 6,9 + `Carrot` 3,2; fase 12 `CastleObjectsTick` 8,1 + `DictionaryMapTile` 5,4. Esses são os próximos alvos de maior retorno (laços Z80 como o das bombas), não o TIMP.

## 0.9.93-pass — `PassTick` e `PassCollect` com `ActorNext` (fase 5)

SHA256 `5248f1a25da72ada7842863819cff12078f57fb3cc36d75820437525fe9b518e`.

Perfil por linha: dos 12,8% de `PassTick` na fase 5, quase tudo era o próprio laço sobre os 10 atores e as 3 batatas (fritas): o SDCC calcula `pass_actors+i` com um deslocamento de 4 bits sobre variáveis em IX (≈ 450 T por entrada, inclusive as inativas, a cada tick) — nenhuma das rotinas `PassSquasher`/`PassFrench`/`PassFall`/`PassWall` aparecia no perfil. `PassTick` e `PassCollect` passaram a visitar só os ativos com `ACTOR_FIRST/ACTOR_AFTER` (o `ActorNext` em Z80 que já servia Mines, Bridge, City e Rapids); em `PassCollect` o teste `enemy_mode&2` foi içado para fora do laço.
Verificação: oráculo de instantâneos (`BENCH_SNAP` em `mkprof.py`): o bloco de atores+fritas (288 bytes de `world_map+9000`) gravado a cada 50 ticks de simulação, Keen parado e invulnerável, 40 s, ROM antiga (0.9.92 reconstruída, SHA idêntico) × nova: 76 instantâneos, todos com atores em movimento, **0 diferenças** (a lógica é guiada por ticks, então as ROMs se alinham mesmo com velocidades de desenho diferentes).
Medida (5,37 MHz): `PassTick` 2,31 → 0,80 ms por chamada, `PassCollect` 3,54 → 1,81 ms, `CombatTick` 3,23 → 1,67 ms; fase 5 no roteiro controlado **12,04 → 15,12 fps (+26%)** e andando 50 s 8,76 → 11,06 fps (+26%). Fases 1 e 6 inalteradas (38,42 / 13,58). Regressão: 0 flips sobre comando de VDP em execução, barra igual à da 0.9.92 (Hill 19, Pass 31–34), volta ao mapa OK, capturas sem corrupção.
Observação de método: laços do tipo `for(i<N){a=base+i; if(!a->active)continue;…}` custam ≈ 450 T por entrada em SDCC com IX; `ActorNext` os reduz a ≈ 25 T por entrada inativa. Vale procurar o mesmo padrão nas demais fases (Swamp, Bay, Apple, Boobus).

## 0.9.94-listas — varredura dos laços de atores/objetos nas demais fases

SHA256 `3e20f49d1137c10391680d1a926416f6196ce5662e7a6d8dc96c647f83cc540a`.

Varredura (`grep` de todos os laços `for(i<N){a=base+i; if(!a->active)continue;…}` e dos laços de chaves/bombas por fase) e correção dos que o perfil mostrava caros:
1. **`ObjList` (Z80, banco fixo, ≈ 110 bytes)**: lista de chaves/bombas — contagem regressiva de `rise`, caixa de toque do `ObjTouch` e coleta — usada por `RapidObjectsTick` (chaves e bombas), `CastleObjectsTick` (chaves) e `GroveObjectsTick` (bombas). Parâmetros nas variáveis de rascunho do anel (`ca_x` = tabela de spawn, `ca_map` = `rise`, `ca_tile` = máscara de coletados, `ca_w` = quantidade, `ca_wx/ca_wy` = caixa, `ca_h` = 0 bombas / 1 chaves). Portas continuam em C.
2. **`ActorNext`/`GroveNext`/`CastleNext`** nos laços de coleta de desenho (`GroveCollect`, 39 entradas; `CastleCollect`, 17; `BayCollect`, `SwampCollect`) e nos de `BoobusTick`/`BoobusCollect` (12 efeitos), `BayTick`, `SwampTick`.
3. **`VDP_USE_VRAM16K` já desligado** (0.9.91) deu a folga de banco fixo que coube o `ObjList`. Não coube no banco 50 (Apple+City) o `ActorNext` extra em `AppleTick`/`AppleCollect`/`CityCollect` (estouro de 175 bytes): ficaram como estão.
Verificação: oráculo `BENCH_OLORC` (`mkprof.py`: lê os parâmetros de `ObjList` na entrada, recalcula a regra original com `(i16)` no retorno, Keen posto sobre as chaves/bombas): Rapids 5 268 chamadas (2 chaves + 3 bombas coletadas), Castle 2 334 (2 chaves), Grove 2 509 (3 bombas), **0 divergências**. Instantâneos por tick (`BENCH_SNAP`, Keen parado, 0.9.93 × nova): Boobus (chefe + efeitos, 400 bytes) 87 instantâneos, Bay 91, Swamp 12, **0 diferenças**. Os laços de coleta só trocam a forma de visitar os ativos (mesma ordem, `enemy_mode&2` içado); validados por capturas e fps, sem oráculo próprio.
Medida (5,37 MHz, 0.9.93 → 0.9.94): `GroveCollect` 6,60 → 0,72 ms por chamada, `GroveObjectsTick` 1,07 → 0,47 ms. Roteiro controlado: fase 8 7,91 → 16,12 fps; fase 12 9,67 → 10,67; fase 4 7,67 → 8,42; fases 9, 10, 11, 2 sem mudança. Andando 50 s: **fase 8 18,94 → 33,74 fps, fase 12 14,56 → 20,06, fase 4 11,46 → 13,08**, fase 9 12,36 → 12,74, fase 11 4,04 → 4,12; fases 1 e 5 idênticas (38,42 / 11,06). O salto da fase 8 é maior que a conta direta porque o quadro cruzou o limiar de vblank (o jogo trava em múltiplos de 1/60 s).
Regressão: 0 flips sobre comando de VDP em execução (fases 8 e 12; endereço do ponto de teste recalculado: 0x58D9), volta ao mapa OK, capturas sem corrupção. A barra de HUD na fase 5 mostra um quadro anômalo (1 736 pixels diferentes) em capturas isoladas — **reproduzido também na 0.9.93 sem esta mudança** (era a pendência "captura transitória da fase 5"); não é regressão, segue sem causa.

## 0.9.95-boobus — passe de chão do chefe só quando precisa (fase 11)

SHA256 `092961116eaecd73e8800d7042ddb02ec4b948ae2eb2530d9e034415dd0379aa`.

Perfil por linha de `BoobusTick`: o custo estava no passe de chão que roda **todo tick** (`dy=|dx|+16` com o chefe parado no chão, depois o laço `BoobusWall` → `MapTile` + multiplicação nas linhas ao redor do pé, e o corte volta o `y` ao mesmo ponto). Com o chefe em estado 0, no chão (`ground`) e `dx = 0` (9 de cada 10 ticks, andando; sempre, parado), o resultado é determinístico: o corte `a->y += surface−newbottom` depende só da superfície sob o meio do chefe, não do `y` anterior, e deixa `ground=1`, `vy=0` — então o passe inteiro é pulado (`goto touch`), mantendo o teste de toque do Keen.
Verificação: instantâneos por tick do bloco do chefe+efeitos (400 bytes de `world_map+7200`, a cada 25 ticks, `BENCH_SNAP`), Keen parado e invulnerável em **6 posições** (`BENCH_PLACE`), 40 s cada, 0.9.94 × nova: 159 / 159 / 159 / 159 / 159 / 158 instantâneos comuns, 105–159 estados distintos por posição, **0 diferenças**.
Medida (5,37 MHz): `BoobusTick` 2,00 → 0,93 ms por chamada, `CombatTick` 2,46 → 1,40 ms; fase 11 no roteiro controlado 4,28 → 4,78 fps (+12%), andando 50 s 4,12 → 4,38 fps (+6%). O resto da fase é o desenho do chefe (VDP/VPF), não a lógica. 0 flips sobre comando de VDP em execução; capturas sem corrupção. O quadro anômalo da barra (1 736 pixels diferentes em capturas isoladas) apareceu aqui na ROM antiga (0.9.94) e não na nova: é aleatório por temporização, não ligado a uma fase nem a esta mudança.

## 0.9.96-chefe-timp — chefe da fase 11 em TIMP de passe único

SHA256 `64a2ab15afd121ed635d4848c59c660f2f74caa4daf3e9d20f033bfe484f7101`.

Os 9 quadros do chefe (`boss_0..8.bin`) deixaram de ter máscara + cor por linha e passaram a ter **uma imagem TIMP por linha** (`w/2` bytes: 0 = transparente, índice de cor, ou 9 — o "segundo preto" da 0.9.63, que tem o mesmo RGB do índice 0 — nos pixels opacos pretos; `tools/build_boobus_assets.py`, com asserção de que nenhum pixel opaco já era 9). No `BoobusBitmap` o desenho passou de duas passagens `LMMM` (AND + OR) para uma (`LMMM`+`TIMP`), e `BoobusTexture`/`BoobusRows` enviam a metade dos bytes pelo `VPF` (largura `sp->w>>1`) — nos dois caminhos (textura em cache com as duas metades de 52 linhas, e faixas de 32 linhas do quadro largo 116). Os dados ocupam metade do espaço em ROM e a textura em cache metade da largura.
Verificação: (1) **dados**: os 57 088 pixels dos 9 quadros, formato antigo (ROM 0.9.95 reconstruída, SHA idêntico) × novo: máscara 15 ⇔ novo 0; opaco ⇔ novo = cor ou 9 — 0 diferenças (40 747 pixels opacos). (2) **comandos do jogo real** (`BENCH_BBORC` com 5.º argumento 0 = só registro; `bbcheck2.py`): para cada chamada, modelo de VRAM em Python com fundo aleatório aplica os comandos novos sobre a textura nova e compara cada pixel de destino com a fórmula antiga `(destino AND máscara) OR cor` calculada dos dados antigos (9 ≡ 0): caminho em cache, 199 chamadas e 1 076 224 pixels, **0 divergências**; caminho em faixas (quadro 116, forçado com `BENCH_FORCEFRAME`), 89 chamadas completas e 880 032 pixels, 0 divergências (74 chamadas recortadas na borda não entraram no modelo simples). Capturas do chefe conferidas visualmente (contornos pretos e transparência corretos).
Medida (5,37 MHz): `BoobusBitmap` 106 → 60 ms por chamada, `DrawScene` 161 → 117 ms; fase 11 no roteiro controlado 4,78 → **6,61 fps (+38%)**, andando 50 s 4,38 → **6,24 fps (+42%)**; VDP ocupado 43% → 36%. 0 flips sobre comando de VDP em execução; barra sem diferença (o quadro anômalo de 1 736 pixels apareceu de novo em captura isolada, agora na ROM nova; já aparecia na antiga em outras execuções — aleatório por temporização, sem causa conhecida).
Limite: só openMSX. Não converti os outros dados "streamed" (carrinhos, Tater, Brocco, City, Apple, Castle); o mesmo método (imagem TIMP por linha, recolorindo pretos opacos para 9) se aplica a cada um, com seus próprios desenhadores.

## 0.9.97-scan — reconstrução das listas de bônus e picapes próximos com `ScanWin`

SHA256 `783f5f3dee0f70767e09fdecaea09da921bb7bb94526e1955436baac3b1646c2`.

Perfil da fase 2 (Mines; plano, sem item dominante): `BonusTick` 3,7% e `InventoryTick` 3,0% — por linha, quase tudo era o laço que **varre todos os bônus/picapes da fase a cada vez que o Keen troca de tile** (≈ 5 tiles/s), com conversões `(i16)(x>>4)−tx` em C. Agora as duas reconstruções usam o `ScanWin` em Z80 (o mesmo da coleta e do `ActorsPrepare`): janela em px `ox=(tx−1)·16`, `w=64`, `oy=(ty−2)·16`, `h=96` (bônus; `h=80` nas picapes), índices em ordem crescente, truncados em 8 (bônus) e 10 (picapes) como o vetor original (`campaign_impl.c`, `inventory.c`).
Verificação: modelo exaustivo — para as 12 tabelas de bônus e todos os `pickups.json`, todos os pares (tx, ty) de 0 a 255 (786 432 janelas cada): lista original (testes `(i16)` de C) × lista da janela sem sinal de 16 bits, **0 diferenças**. No emulador, com o Keen posto sobre bônus e picapes (`BENCH_PLACE`, `BENCH_VARS` novo em `mkprof.py`, 0.9.96 × nova): placar, máscara `bonus_taken`, vidas, munição e `pickup_taken` idênticos em 6 bônus e 2 picapes da fase 2.
Medida (5,37 MHz): `BonusTick` por chamada 1,13 → 0,53 ms (fase 12), 0,93 → 0,40 (fase 9); `InventoryTick` 0,55 → 0,30 e 0,35 → 0,23. Roteiro controlado: fase 2 7,63 → 8,48 fps (+11%), fase 5 15,12 → 16,17 (+7%), fase 12 10,67 → 10,87, fase 1 igual. Andando 50 s as trajetórias divergem entre ROMs (fase 2 +6%, fase 5 +3%, fases 9 e 12 −5% e −6% por trajetória diferente, não por custo: o custo por chamada caiu em todas). 0 flips sobre comando de VDP em execução, barra sem diferença, volta ao mapa OK, capturas sem corrupção.
