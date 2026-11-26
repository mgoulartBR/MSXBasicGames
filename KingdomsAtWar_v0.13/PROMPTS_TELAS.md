# Telas do jogo (título, vitória, derrota, final) — pacote de prompts para o ChatGPT

O conector do Higgsfield instalado no Claude **só lê** (lista modelos, créditos e gerações antigas); ele não gera imagens. Por isso seguimos o fluxo que já funcionou: você gera no ChatGPT com estes prompts e me envia as imagens; eu converto para a paleta do jogo (`tools/convert_ramp.py`) e integro.

## Regras técnicas (valem para todas as telas)
- Formato **4:3, 1024×768** (será reduzido para 256×192 = 32×24 tiles de 8 px).
- **Sem texto na imagem.** O título, os menus e as mensagens são desenhados pelo jogo, nos 4 idiomas.
- Só estas 15 cores: `#212121 #214A21 #4A6B4A #21214A #4A6B94 #4A2121 #944A21 #6B4A21 #DE944A #B5944A #DEDE94 #4A4A4A #949494 #DEDEDE #FFFFDE`.
- Contraste forte, formas grandes e poucas; nada menor que 8 px na imagem final (32 px em 1024×768). Sem contorno preto grosso, sem degradê suave (sombreado em 3 tons por material, tracejado permitido).
- Deixar uma **faixa escura calma** (sem detalhes) no terço de baixo ou no topo, onde o jogo escreve o texto.

## Bloco ESTILO (colar sempre)
> Gritty 1990s PC strategy-game key art, hand-painted pixel-art look, dull dirty realistic palette (like Warcraft II / Age of Empires I), strong light-dark contrast, big simple shapes, flat shading with 3 tones per material, dithered shadows, no gradients, no outline, no text, no logo, no UI. Use ONLY these 15 colours: #212121 #214A21 #4A6B4A #21214A #4A6B94 #4A2121 #944A21 #6B4A21 #DE944A #B5944A #DEDE94 #4A4A4A #949494 #DEDEDE #FFFFDE. 4:3 composition, 1024×768, keep the bottom third dark and calm for text.

## 1. Tela de título
> (Bloco ESTILO) A wide dramatic landscape at dusk: on the left a human stone castle with blue roofs lit by warm orange light, on the right a dark sandstone necropolis with a tall tower and pale bone-coloured walls under a cold blue-green glow, between them a narrow river and a plain of dark grass. Two small armies facing each other in silhouette in the middle distance (a few swordsmen on each side, simple shapes). Big low sun behind the castle, dark purple-blue sky, a few thin clouds. Centre top left empty for the game title.

## 2. Vitória (humanos)
> (Bloco ESTILO) The human army stands victorious in front of the ruined undead tower at dawn: a knight with a blue banner in the foreground raising a sword, three swordsmen behind, broken sandstone blocks and fallen bone-coloured columns on the ground, warm orange sunrise light from the left, long shadows. Triumphant, calm, no gore.

## 3. Derrota (humanos)
> (Bloco ESTILO) The human castle burns at night: dark sandstone-coloured undead warriors with dim blue-green glowing eyes walk through the gate carrying torn blue banners, the castle tower collapsing in orange flames, smoke over a dark blue sky, a lone fallen human helmet in the foreground. Grim, no gore.

## 2b. Vitória (Insepultos)
> (Bloco ESTILO) The undead faction stands victorious over the fallen human castle at night: a tall jackal-headed guard in tarnished gold plate armor in the foreground raising a curved scimitar, three bandaged mummy warriors behind him with dim blue-green glowing eyes, the broken human castle wall and torn blue banners on the ground, cold blue-green glow from the left, long dark shadows, a pale moon. Menacing and triumphant, no gore.

## 3b. Derrota (Insepultos)
> (Bloco ESTILO) The undead necropolis collapses at dawn: the tall sandstone tower cracking and falling, bone-coloured columns toppling, human swordsmen with blue banners marching through the broken gate, bandaged mummy warriors crumbling to dust on the ground, warm orange sunrise light breaking over a dark blue-purple sky, a lone broken scimitar in the foreground. Grim and final, no gore.

## 4. Final da campanha (humanos)
> (Bloco ESTILO) Peaceful epilogue: the human castle and a small village at sunrise seen from a hill, a narrow river, green fields, a soldier and a peasant standing side by side looking at the horizon, long warm light, soft clouds. Hopeful.

## 4b. Final da campanha (Insepultos)
> (Bloco ESTILO) Quiet epilogue of the undead campaign: at dusk a tall sandstone necropolis tower rises over the conquered river valley, a long line of bandaged mummy warriors and a jackal-headed guard walking home in silhouette along a dark road, torn blue human banners planted in the ground as trophies, a pale moon and a cold blue-green glow on the horizon, dark purple-blue sky. Solemn and eerie, victory without celebration, no gore.

## 5. (opcional) Menu / fundo de briefing
> (Bloco ESTILO) An old parchment map of a river valley with a castle on one side and a necropolis on the other, torn edges, dark brown-and-sand palette, big empty areas where text will be drawn.
