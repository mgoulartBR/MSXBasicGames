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
