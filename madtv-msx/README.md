# MadTV-MSX

Port/reimplementacao do Mad TV (1991) para MSX2 em C + MSXgl, usando os dados do remake open source TVTower.
Status: **0.1 - pipeline de build/teste funcionando; sem gameplay ainda.**

- Alvo: MSX2 (V9938), ROM. Nao testado em hardware real.
- Build: `scripts/setup.sh && scripts/build.sh` -> `dist/madtv-msx-0.1.rom`
- Rodar: `scripts/run.sh` (openMSX, C-BIOS MSX2); `scripts/run.sh dist/x.rom --shot s.png` headless.
- Docs: docs/TOOLCHAIN.md, docs/RULES.md, LICENSES.md, TODO.md
- Material original do Mad TV e o banco do TVTower ficam em `original/` (nao versionado).
