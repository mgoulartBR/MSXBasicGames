# Facção dos mortos (múmias do deserto) — pacote de prompts no estilo REALISTA dos humanos

Decisão de direção de arte: **realista e sujo**, igual aos humanos. Os mortos anteriores (estilo cartoon) ficam só como provisórios.

## Como usar
1. Para cada item, **anexe a imagem de referência correspondente** (pasta `reference/humanos_estilo/anexos/`) e cole o prompt abaixo (bloco ESTILO + bloco do item).
2. Gere no ChatGPT. Se vier com contorno preto grosso, cores vivas ou cabeça grande, responda: *"Match the reference style more strictly: no black outline, dull dirty palette, small head, dithered shading."*
3. Me envie a folha. Eu converto com `python3 tools/convert_units.py --realista ...` (sombreado por difusão de erro) e integro com `tools/integrate_units.py`.

## Bloco ESTILO (colar sempre)
> The attached image is a reference sprite sheet from the human faction of the same game. Create the undead faction's sprite sheet in EXACTLY the same rendering style as the reference: gritty 1990s PC strategy game sprites (Warcraft I / Dune II feel), top-down 3/4 view. NO continuous black outline: edges are defined by darker shades of the object's own colors. Each material has 3–4 tones with dithered, noisy, textured shading and a few stray highlight pixels. Realistic body proportions: small head (about one sixth of the body height), lean limbs, weary hunched posture. Dull, dirty, low-saturation palette: dust/bone beige, dried brown bandages, tarnished dark gold, oxidized bronze, dim blue-green (verdigris), dried-blood rust, charcoal — no bright cream, no pure white, no saturated turquoise or yellow. The undead are desiccated, gaunt and menacing — NOT cute, NOT chibi, NOT cartoon. Same frame size, same camera angle, same layout as the reference. Transparent background, no anti-aliasing, max 16 colors, no gradients. Make the face read clearly at 32 px (dark skull face, two small dim blue-green eye pixels). Make accents distinct: verdigris patina on bronze clearly blue-green (not brown), steel bright gray, tarnished-yellow gold highlights. Feet always on the same baseline, identical proportions in every frame.

## Unidades

| # | Anexo | Item |
|---|---|---|
| 1 | `anexo_1_guerreiro_(espadachim).png` | **Guerreiro-múmia**, 32 px, 4 linhas (baixo, direita, cima, esquerda) × 4 quadros de caminhada |
| 2 | `anexo_2_campones.png` | **Camponês-escravo**, 32 px, 6 linhas × 4 colunas: linhas 1–4 caminhada, linha 5 trabalho A, linha 6 trabalho B (colunas = 4 direções) |
| 3 | `anexo_3_arqueiro.png` | **Arqueiro esquelético**, 32 px, 4 × 4 |
| 4 | `anexo_4_lanceiro.png` | **Lanceiro-chacal**, 48 px, 4 × 4 |
| 5 | `anexo_5_cavaleiro.png` | **Cavaleiro de camelo de osso**, 48 px, 4 × 4 |
| 6 | `anexo_6_ariete.png` | **Aríete-sarcófago**, 48 px, 4 × 4 |

Pedidos (depois do bloco ESTILO):
1. *"Gaunt mummy warrior: ragged dark-brown bandages over bone, dim blue-green glowing eyes, tarnished curved scimitar in dull steel, worn leather belt. Character 26 px tall in a 32×32 frame."*
2. *"Emaciated mummy slave carrying a worn clay urn on the back, bound with rope, tools: a wooden mallet and an iron pick. Same posture family as the human peasant in the reference. Character 26 px tall in 32×32 frames. Rows 5 and 6: work poses (A: tool raised; B: tool striking) for each of the 4 directions."*
3. *"Skeletal archer in tattered dark rags, recurve bow of dark wood, quiver of bone arrows on the back. Character 26 px tall in a 32×32 frame."*
4. *"Jackal-headed guard (Anubis style) in dull tarnished-gold plate armor, round bronze shield, long spear. Realistic canine skull proportions, gritty. Character 40 px tall (spear included) in a 48×48 frame."*
5. *"Skeletal rider on a bone-armored camel, rags and tarnished gold, curved sword. Same detail level as the human knight in the reference. Character about 44 px wide in a 48×48 frame."*
6. *"Siege engine: a stone sarcophagus on heavy wooden wheels with a battering head shaped like a scarab, pushed by two gaunt mummies. Same detail level as the human ram. Fills 46 px of the 48×48 frame."*

## Prédios (estilo dos prédios humanos)
Anexe o prédio humano correspondente (`anexo_predio_*.png`), cole o **bloco ESTILO de prédios** e depois o pedido do prédio.

**Bloco ESTILO de prédios**
> The attached image is the human faction's building from the same game. Create the undead faction's equivalent building in EXACTLY the same rendering style: gritty 1990s PC strategy game building sprite, same isometric-ish 3/4 top-down camera, same scale, same level of detail, same dithered noisy texture and shading, same small dirt base under the building. NO black outline; edges are darker shades of the object's own colors. Weathered sandstone, dark aged wood, tarnished dark gold trim, oxidized bronze with clear blue-green verdigris, dried-blood red cloth, charcoal shadows. Dull dirty low-saturation palette (no bright yellow, no pure white, no saturated turquoise). Desert undead architecture: ossuary, tomb, obelisk motifs, torches with dim blue-green flames. No characters in the image. Transparent background, no anti-aliasing, max 16 colors, no gradients. Entrance faces down-left exactly like the reference. Exact canvas size given below; the building fills the canvas and its base touches the bottom edge.

| Anexo | Tamanho | Pedido (colar depois do bloco) |
|---|---|---|
| `anexo_predio_0_castelo.png` | 64×64 | "Castle: a small stepped sandstone fortress-tomb with a tall corner tower and a pyramid-capped hall, a dark arched portal, two verdigris-bronze braziers with dim blue-green flames, a tattered dried-blood-red banner on the tower." |
| `anexo_predio_1_casa.png` | 48×48 | "House: a low rounded ossuary hut of sandstone with a flat thatched roof, a stack of clay urns and bones at the door, one brazier." |
| `anexo_predio_2_quartel.png` | 64×64 | "Barracks: a long low sandstone hall with a big dark doorway carved with a stylized eye, racks of dull scimitars and shields at the wall, a tarnished-gold gong, tattered red cloth hanging." |
| `anexo_predio_3_serraria.png` | 64×64 | "Sawmill: an open timber workshop with a big wooden frame holding a dull bronze saw blade, stacked logs and planks, a sandstone footing, sawdust on the ground." |
| `anexo_predio_4_ferraria.png` | 64×64 | "Forge: a sandstone furnace with a chimney glowing dim blue-green, an anvil, tarnished-gold ingots, bronze tools hanging." |
| `anexo_predio_5_estabulo.png` | 64×64 | "Stable: a half-open sandstone stall with fences made of bone and wood, hay, a camel skull totem above the entrance, a water trough." |
| `anexo_predio_6_torre.png` | 64×64 | "Tower: a tall tapered sandstone obelisk-tower with archer slits, tarnished-gold trim bands, a verdigris-bronze eye-shaped ornament and a dim blue-green flame at the top." |
| `anexo_predio_7_oficina_cerco.png` | 64×64 | "Siege workshop: a wide timber-and-sandstone shed with big wooden wheels leaning outside, an unfinished sarcophagus on trestles, ropes and beams." |

**Obra em andamento (opcional, 64×64):** "Construction site: wooden stakes and a rope fence around a stone footing, a few sandstone blocks, a half-built scaffold, same style, same dirt base." (O jogo usa uma única imagem de obra para todos os prédios; hoje é gráfica simples e compartilhada.)

Dicas: um prédio por imagem, no tamanho exato; se o prédio sair maior que o quadro, peça "scale the building so it fits entirely inside the canvas with 2 px margin at the sides"; se sair com contorno preto, use a frase de correção do bloco de unidades.

## Resultado já aprovado
Guerreiro-múmia (folha realista) aprovado em novembro de 2026 como referência de estilo para o restante.

## Prédios: paleta obrigatória (descoberta técnica)
Os prédios são tiles e compartilham a paleta do terreno e dos humanos (16 cores fixas); só as unidades têm paleta própria. Por isso, **gere os prédios usando SOMENTE estas cores** (imagem `paleta_do_jogo.png`):

- `#212121` quase preto
- `#214A21` verde escuro
- `#4A6B4A` verde (grama)
- `#21214A` azul escuro
- `#4A6B94` azul acinzentado
- `#4A2121` vinho escuro
- `#944A21` laranja queimado
- `#6B4A21` marrom
- `#DE944A` laranja claro
- `#B5944A` areia escura
- `#DEDE94` creme
- `#4A4A4A` cinza escuro
- `#949494` cinza
- `#DEDEDE` cinza claro
- `#FFFFDE` creme claro

Acrescente ao pedido de cada prédio: *"Use ONLY these 15 colors (exact hex values), nothing else, no anti-aliasing: [lista]. Map: sandstone = #...; wood = brown; red cloth = dark wine red; gold trim = light orange/sand; verdigris/teal accents = blue-grey; metal = greys."* (substitua pelos hex acima). Mantenha o resto do bloco de estilo.

## Prédios v2 — formas simples (recomendado após teste de suavização)

Bloco de estilo (cole antes de cada prédio):

> Isometric pixel-art building, hand-painted for a retro 16-colour game, viewed from a 3/4 top-down angle, transparent background, single building centred, nothing else. Desert necropolis style: sandstone blocks, bleached bone, tattered dark-blue cloth, green oxidised bronze. FLAT SHADING ONLY: each material uses just 3 tones (light, mid, dark) with hard edges, no gradients, no textures, no cracks, no vines, no scattered debris, no tiny details. Big bold readable shapes, strong silhouette, thick roof planes, one light source from the top-left (right/bottom faces darker). Maximum 15 colours, ONLY these: #212121 #214A21 #4A6B4A #21214A #4A6B94 #4A2121 #944A21 #6B4A21 #DE944A #B5944A #DEDE94 #4A4A4A #949494 #DEDEDE #FFFFDE. Design it as if it will be shrunk to 64x64 pixels: every feature must be at least 4 pixels wide at 256x256 size. No outline, no text.

Um prédio por imagem (acrescente depois do bloco):

1. **Castelo (QG):** a squat sandstone keep with one tall square tower on the left, a big pyramid-shaped roof on the right, one dark arched door, two bone-white banners.
2. **Casa:** a small flat-roofed mud-brick hut with a single dark doorway and a dark-blue cloth awning. Smaller than the others.
3. **Quartel:** a wide low sandstone hall with a flat roof, a large dark arched entrance, two bone pillars, a skull emblem above the door (one big shape).
4. **Serraria:** an open-sided wooden shed with a big sloped roof, one large round saw blade on the front, a stack of 3 thick logs.
5. **Forja:** a stone building with a thick chimney on the right, a glowing orange furnace opening on the front, one large anvil shape beside it.
6. **Estábulo:** a wide open-front stable with a thick sloped roof, two big posts, a hay pile and a bone-white horse skull above the entrance.
7. **Torre:** a tall tapered round-square sandstone tower with a crenellated top (4 big blocks), one narrow dark window, a blue flame on top.
8. **Oficina de cerco:** a wide open workshop with a sloped roof, a big wooden wheel on the left front, a large ram log resting on the right.

Dica: se o gerador insistir em detalhes finos, adicione "paint it like a children's wooden toy: chunky, simple, large flat colour areas".

## Canteiro de obras dos mortos

Uma única imagem, que o jogo usa para todo prédio inimigo em obra (em 64×64 para os prédios 4×4 e reduzida a 48×48 para a casa). Segue as lições dos prédios: poucas formas grandes, contraste forte entre claro e escuro, nada de detalhe fino.

Cole depois do bloco de estilo dos prédios:

> Construction site for an undead desert faction, NOT a finished building. Same gritty realistic pixel-art style as the previous buildings, isometric 3/4 view, transparent background, one object only, centred, filling a square footprint about as wide as it is tall (the whole picture must read as a flat-ish square lot, low profile, nothing taller than half the width). Content: a low wooden scaffolding frame made of 4 thick dark-brown posts and 2 horizontal beams, around a rough stack of pale sandstone blocks (light faces, darker shadowed faces), a few loose blocks in front, one coil of rope, one bleached bone beam leaning on the frame, one small torn dark cloth hung on a post. IMPORTANT for readability at 64x64 pixels: strong light-dark contrast (pale sandstone against dark wood and a dark ground patch), big simple shapes, flat shading with 3 tones per material, every feature at least 4 pixels wide at 256x256 size. Forbidden: skulls, banners, torches, hieroglyphs, small tools, text, plants, outlines, tiny debris. Use ONLY these colours: #212121 #214A21 #4A6B4A #21214A #4A6B94 #4A2121 #944A21 #6B4A21 #DE944A #B5944A #DEDE94 #4A4A4A #949494 #DEDEDE #FFFFDE.

Dicas: se o gerador entregar um prédio quase pronto, peça "much lower, only the frame and the stone pile, clearly unfinished". Se vier com fundo xadrez "transparente", não precisa corrigir (a conversão remove).
