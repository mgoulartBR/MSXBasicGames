# MSXBasicGames
Collection of MSX-Basic games/apps created with AI or extracted and modified from old books and magazines.

AI games created using a GPT agent that I created in ChatGPT: https://chatgpt.com/g/g-684353e724b88191bda39c99828f820f-assistente-de-programacao-para-msx

## msxdoom/
FPS 2.5D estilo DOOM em Three.js (abra `msxdoom/index.html`). Render 256x212 com paleta adaptativa de 256 cores (estilo V9968 SCREEN 8/EPAL), texturas, sprites e HUD em 3 painéis. F3 mostra orçamento Geo3D. Veja `msxdoom/HARDWARE.md`. Controles: ↑↓ andar, ←→ strafe, mouse olhar, botão esquerdo/ESPAÇO atirar.

## msxrom/
ROM MSX (turbo R + V9968 + Geo3D), ASCII16 de 256 KiB, em C/SDCC: SCREEN 8 com paleta EPAL de 256 cores e paredes/inimigos texturizados pelo Geo3D. BUILD OK e testada só em simulação (Z80 emulado + modelo próprio do V9968/Geo3D); sem teste em emulador MSX, FPGA ou hardware. Veja `msxrom/README.md`.
