# MadTV-MSX

Port/reimplementacao do Mad TV (1991) para MSX2 em C + MSXgl, usando os dados do remake open source TVTower.
Status: **0.2 - navegador de filmes (dados do TVTower convertidos), sem gameplay ainda.**

- Alvo: MSX2 (V9938), ROM. Nao testado em hardware real.
- Build: `scripts/setup.sh && scripts/build.sh` -> `dist/madtv-msx-0.2.rom` (versao em `VERSION`)
- Rodar: `scripts/run.sh` (openMSX, C-BIOS MSX2); `scripts/run.sh dist/x.rom --shot s.png` headless.
- Dados: `tools/convert_db.py original/tvtower_db/Default src/data/db_data.h` (o banco fica em `original/`, nao versionado)
- Teste: `tests/smoke.sh` (boot + input + screenshots em `screenshots/test/`)
- Controles: setas (teclado ou joystick 1) trocam categoria/filme; ESC sai.
- Docs: docs/PORTING.md, docs/TOOLCHAIN.md, docs/RULES.md, LICENSES.md, TODO.md
- Material original do Mad TV e o banco do TVTower ficam em `original/` (nao versionado).
