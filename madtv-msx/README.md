# MadTV-MSX

Port/reimplementacao do Mad TV (1991) para MSX2 em C + MSXgl, usando os dados do remake open source TVTower.
Status: **0.5.1 - nucleo de gestao jogavel (comprar filmes, assinar contratos, montar a grade, competir por audiencia/Image).** Sem predio, Betty, noticias nem audio ainda.

- Alvo: MSX2 (V9938), ROM 128 KB com mapper ASCII8. Nao testado em hardware real.
- Build: `scripts/setup.sh && scripts/build.sh` -> `dist/madtv-msx-0.2.rom` (versao em `VERSION`)
- Rodar: `scripts/run.sh` (openMSX, C-BIOS MSX2); `scripts/run.sh dist/x.rom --shot s.png` headless.
- Dados: `tools/convert_db.py original/tvtower_db/Default src/data/db_data.h` (o banco fica em `original/`, nao versionado)
- Teste: `tests/smoke.sh` (joga um dia inteiro por teclas injetadas, confere estado interno; screenshots em `screenshots/test/`)
- Mouse: tecla M liga/desliga (porta 1 -> porta 2 -> off); esquerdo = clicar, direito = voltar; clique na velocidade do cabecalho troca a velocidade.
- Controles: setas/joystick = mover; Enter/Espaco/botao A = OK; Esc/botao B = voltar; TAB = velocidade; P = pausa.
- Predio (0.5): setas escolhem a porta, OK faz o personagem andar/subir de elevador ate la; salas com aviso "Closed" ainda nao existem. Escritorio (andar 3) = grade, audiencias e salvar/carregar.
- Como jogar (0.4): alem do fluxo abaixo, assine agencias e monte o telejornal na News room (mais audiencia), venda filmes no Archive, tome credito com o Boss e salve por codigo em Save/Load (no titulo, Esc carrega).
- Testes: `tests/unit.sh` (logica no PC), `tests/balance.sh` (autoplay), `tests/mouse_real.sh` (mouse emulado + xdotool), `tests/smoke.sh` (openMSX; `SHOTS=1` para screenshots fieis, ~2,5 min).
- Como jogar: Film agency (comprar) -> Ad agency (assinar contrato) -> Programme grid (encaixar filmes e anuncios nos horarios) -> acompanhar em Ratings.
- Docs: docs/PORTING.md, docs/TOOLCHAIN.md, docs/RULES.md, LICENSES.md, TODO.md
- Material original do Mad TV e o banco do TVTower ficam em `original/` (nao versionado).
