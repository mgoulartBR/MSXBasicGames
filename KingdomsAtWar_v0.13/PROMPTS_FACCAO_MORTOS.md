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
Anexe o prédio humano correspondente (`anexo_predio_*.png`) e use o **bloco ESTILO** adaptado: *"…same isometric 3/4 view, same texture and shading as the attached human building, desert undead architecture: weathered sandstone, dark wood, tarnished gold trim, oxidized bronze, dim verdigris torch flames, small dirt/sand base like the reference. No characters. Transparent background. Entrance facing down-left like the reference."*

| Anexo | Tamanho | Item |
|---|---|---|
| `anexo_predio_0_castelo.png` | 64×64 | Fortaleza-pirâmide com torre e salão, bandeira vinho |
| `anexo_predio_1_casa.png` | 48×48 | Ossuário (cabana com urnas e crânios) |
| `anexo_predio_2_quartel.png` | 64×64 | Salão das múmias com portal de olho |
| `anexo_predio_3_serraria.png` | 64×64 | Oficina de madeira com serra de bronze |
| `anexo_predio_4_ferraria.png` | 64×64 | Forja com fornalha esverdeada |
| `anexo_predio_5_estabulo.png` | 64×64 | Estábulo de camelos, cercas de osso |
| `anexo_predio_6_torre.png` | 64×64 | Obelisco-torre com olho no topo |
| `anexo_predio_7_oficina_cerco.png` | 64×64 | Galpão com rodas e sarcófago em obra |

Peça cada prédio em **imagem separada**, no tamanho exato, base alinhada à linha de baixo.

## Resultado já aprovado
Guerreiro-múmia (folha realista) aprovado em novembro de 2026 como referência de estilo para o restante.
