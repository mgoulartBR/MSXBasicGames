# Keen Dreams MSX 0.9.62 — otimização de desenho (sugestão do itch.io + lições do RTS)

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
