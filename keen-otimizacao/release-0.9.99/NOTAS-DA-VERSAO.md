# Keen Dreams MSX2 — 0.9.99 (correções sobre a 0.9.98)

Atualização de correção, a partir do feedback do byturn. Mesma jogabilidade e desempenho da 0.9.98.

## Português
- **Tela de título não aparecia** (bug desde a 0.9.76, visível na 0.9.98): as interrupções da barra de HUD liam `bar_on` antes de ele ser inicializado; com lixo na RAM (máquinas reais, blueMSX, openMSX) elas reescreviam os registradores de página/rolagem a cada quadro e escondiam o título (a tela ficava na página errada). Agora `bar_on` é zerado no início. Verificado no openMSX: antes, tela preta com R#2 = 0x7F; depois, título normal.
- **MSX2 (V9938) não é mais suportado.** Na emulação de MSX2 o jogo travava ao rolar para a esquerda (a pilha era corrompida dentro de `ChangedBounds`, que troca o banco da janela de 0x8000 enquanto executa dela). A rolagem por software do V9938 não tem solução barata no espaço restante do banco fixo (20 bytes livres), então o requisito mínimo passa a ser **MSX2+ (V9958)**. Em um MSX2 a ROM mostra uma mensagem e para.

## English
- **The title screen was not shown** (bug since 0.9.76, visible in 0.9.98): the HUD-bar interrupts read `bar_on` before it was initialised; with garbage in RAM (real machines, blueMSX, openMSX) they rewrote the page/scroll registers every frame and hid the title. `bar_on` is now cleared at start-up. Checked in openMSX: before, a black screen with R#2 = 0x7F; after, the normal title.
- **MSX2 (V9938) is no longer supported.** In MSX2 emulation the game hung when scrolling left (the stack got corrupted inside `ChangedBounds`, which remaps the 0x8000 window while running from it). There is no cheap fix for the V9938 software scroll in the space left in the fixed bank (20 bytes), so the minimum requirement is now **MSX2+ (V9958)**. On an MSX2 the ROM shows a message and stops.

## Limites conhecidos / Known limits
Medido só no openMSX; sem teste em hardware real. Em modo R800 (turbo R) a barra de HUD sai corrompida (a detecção por velocidade não distingue o R800). A física do Keen está a 35 Hz.
