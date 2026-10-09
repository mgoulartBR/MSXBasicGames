# MadTV-MSX

Port/reimplementacao do Mad TV (1991) para MSX2 em C + MSXgl, usando os dados do remake open source TVTower.
Status: **0.8 - Porteiro e escritorios dos rivais abertos (sobre a 0.7: dificuldade, 160 filmes, Image baixo; 0.6: Betty e presentes).** Predio navegavel, gestao, noticias, credito, salvar por codigo, mouse e agora o Supermercado e a Betty (final feliz = casar). Sem audio ainda.

- Alvo: MSX2 (V9938), ROM 128 KB com mapper ASCII8. Nao testado em hardware real.
- Build: `scripts/setup.sh && scripts/build.sh` -> `dist/madtv-msx-<versao>.rom` (versao em `VERSION`)
- Rodar: `scripts/run.sh` (openMSX, C-BIOS MSX2); `scripts/run.sh dist/x.rom --shot s.png` headless.
- Dados: `tools/convert_db.py original/tvtower_db/Default src/data/db_data.h` (o banco fica em `original/`, nao versionado)
- Teste: `tests/smoke.sh` (joga um dia inteiro por teclas injetadas, confere estado interno; screenshots em `screenshots/test/`)
- Mouse: autodetectado nas portas 1 e 2 (cursor sempre visivel); tecla M cicla auto -> porta 1 -> porta 2 -> off; esquerdo = clicar, direito = voltar; clique na velocidade do cabecalho troca a velocidade.
- Controles: setas/joystick = mover; Enter/Espaco/botao A = OK; Esc/botao B = voltar; TAB = velocidade; P = pausa.
- Predio (0.5): setas escolhem a porta, OK faz o personagem andar/subir de elevador ate la; salas com aviso "Closed" ainda nao existem. Escritorio (andar 3) = grade, audiencias e salvar/carregar.
- Como jogar (0.4): alem do fluxo abaixo, assine agencias e monte o telejornal na News room (mais audiencia), venda filmes no Archive, tome credito com o Boss e salve por codigo em Save/Load (no titulo, Esc carrega).
- Testes: `tests/unit.sh` (logica no PC), `tests/balance.sh` (autoplay), `tests/mouse_real.sh` (mouse emulado + xdotool), `tests/smoke.sh` (openMSX; `SHOTS=1` para screenshots fieis, ~2,5 min).
- Como jogar: Film agency (comprar) -> Ad agency (assinar contrato) -> Programme grid (encaixar filmes e anuncios nos horarios) -> acompanhar em Ratings.
- Docs: docs/PORTING.md, docs/TOOLCHAIN.md, docs/RULES.md, LICENSES.md, TODO.md
- Material original do Mad TV e o banco do TVTower ficam em `original/` (nao versionado).
- Porteiro (andar 0, esquerda): renda/custos do dia, ranking de Image e dicas. Escritorios FUN/SUN (andar 3): espiam a grade de hoje dos rivais e as audiencias ja medidas (nao ha sabotagem ainda).
- Dificuldade (0.7): no titulo, setas esquerda/direita (ou clique na linha) escolhem Easy/Normal/Hard (caixa inicial 3500/2500/2000k e rivais a 55/68/74% da audiencia). Image < 20: anunciantes pagam 20% a menos e o chefe nega credito. O cabecalho mostra a simpatia da Betty (Bnn).
- Betty (0.6): compre presentes no Supermercado (andar 0) e de-os a Betty (andar 4). Simpatia nunca passa do Image da emissora e cai 1/dia sem presente; os rivais tambem a cortejam. Casar exige simpatia 100, rivais falidos (Image 0) e uma Dream trip em estoque. Formulas proprias, nao as do Mad TV.
