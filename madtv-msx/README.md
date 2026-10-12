# MadTV-MSX

Port/reimplementacao do **Mad TV** (Rainbow Arts, 1991) para **MSX2**, em C com [MSXgl](https://github.com/aoineko-fr/MSXgl), usando os dados do remake open source **TVTower**. O modelo de simulacao e proprio (nao e o do original); o jogo e escrito do zero para o V9938 (Screen 5).

**Versao atual: 2.9.** A 2.4 foi testada em MSX2 real pelo autor (desempenho bom); o restante foi verificado apenas no openMSX (C-BIOS).

## O que e o jogo
Voce dirige uma emissora de TV: compra filmes (agora com elenco e estrelas) e contratos de anuncio, monta a grade de programacao, acompanha a audiencia, monta o telejornal, disputa o Image e o premio **Sammy** com FunTV e SunTV (que sabotam, levam filmes do catalogo e ampliam seu alcance), amplia a cobertura (torres, cabo, satelite), produz seus proprios filmes e corteja a Betty. Voce escolhe a emissora no titulo (MadTV equilibrada, FunTV +500k de caixa e Image -5, SunTV Image +8 e -500k). Tres finais: casamento, magnata da TV ou falencia.

## Requisitos e build
- MSX2 com 128 KB de VRAM; ROM de **512 KB**, mapper **ASCII8** (com SRAM de 8 KB para salvar em 4 slots; sem SRAM salva por codigo de 140 caracteres). Mouse (porta 1 ou 2, autodetectado), MSX-MUSIC (FM) opcionais.
- `scripts/setup.sh && scripts/build.sh` -> `dist/madtv-msx-<versao>.rom` (versao em `VERSION`). Toolchain: MSXgl 1.5.0 + SDCC 4.6.0 (ver `docs/TOOLCHAIN.md`).
- `scripts/run.sh` abre no openMSX; para SRAM use `-romtype ASCII8SRAM8`.
- Dados: `tools/convert_db.py original/tvtower_db/Default src/data/db_data.h` (o banco do TVTower fica em `original/`, nao versionado).

## Controles
Setas/joystick = mover; Enter/Espaco/botao A = OK; Esc/botao B = voltar; TAB = velocidade; P = pausa; M = alterna o modo do mouse (esquerdo = clicar, direito = voltar). O relogio comeca PAUSADO a cada dia: planeje e aperte P. Titulo: Esc carrega (slots ou codigo).

## Como jogar (resumo)
Film agency (comprar) -> Ad agency (contrato) -> Programme grid (encaixar filmes e anuncios) -> Ratings. Alem disso: News room (telejornal), Archive (vender), Boss (credito), Supermercado e Betty (presentes e pedido de casamento), Roteiros + Estudios (producao propria), Corretor (cobertura), Porteiro (resumo), escritorios FUN/SUN (espiar a grade dos rivais). Detalhes das regras em `docs/RULES.md`; decisoes de projeto em `docs/PORTING.md`.

## Arte do jogo original (so em build local)
O repositorio **nao contem arte do Mad TV original**. Se voce tiver os arquivos do jogo, as ferramentas `tools/make_*.py` geram localmente (pastas e arquivos ignorados pelo git) o predio, as cenas das salas, a grade e a abertura a partir de capturas em `original/`. Sem elas o jogo usa telas de texto e um predio desenhado em codigo; os bonequinhos sao desenhados a mao (`src/sprites_fig.h`). Ver `docs/PORTING.md`.

## Fidelidade
Comparacao com o jogo original: `docs/FIDELITY.md`.

## Testes
`tests/unit.sh` (logica no PC), `tests/balance.sh` (autoplay; `CFLAGS_EXTRA=-DBALANCE_STATS`), `tests/smoke.sh` (openMSX; `SHOTS=1` para capturas fieis), `tests/sram.sh`, `tests/audio_fm.sh`, `tests/mouse_real.sh`; roteiro para MSX2 real em `tests/HARDWARE.md`. Capturas dos testes ficam em `screenshots/` (nao versionado: mostram a arte do original em builds locais).

## Licencas
Ver `LICENSES.md`. Os dados vem do TVTower (uso comercial exige permissao da equipe; versoes alteradas devem ser marcadas): modelo de pedido em `docs/LICENSE_REQUEST.md`. Mad TV e marca/obra de seus detentores; este projeto nao e afiliado a eles.

## Pendencias
Ver `TODO.md`.
