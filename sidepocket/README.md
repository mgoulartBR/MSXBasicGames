# Side Pocket (MSX2 / MSXGL) — game logic

Reimplementação em C da lógica de um jogo de sinuca estilo *Side Pocket*,
usando a versão **Mega Drive** apenas como especificação de comportamento.
Nenhum código ou asset da ROM original está neste repositório.

## Estado
- `src/sp_logic.[ch]`: física (bolas, tabelas, caçapas, atrito), tacada e rack. C portátil, sem dependências de plataforma.
- `test/test_logic.c`: testes no PC (`make test`).

## Próximos passos
1. **Calibrar com a ROM do Mega Drive** (dump próprio, fora do repo): atrito, perda nas tabelas, raio da bola/caçapa, ângulos de tacada, potência. Anote os valores em `docs/spec.md`.
2. Regras: sequência 1→9, pontuação, bônus, fases, estágios de "Pocket Game".
3. Camada MSXGL: Screen 5, sprites das bolas, mira e barra de potência, entrada por joystick/teclado.
4. Otimizar para Z80 (SDCC): trocar `int32_t` e divisões por tabelas.

## Convenções
Posições em 12.4 (1/16 px), velocidades em 8.8 px/frame, ângulos 0–255 (256 = volta completa, como no jogo original).
