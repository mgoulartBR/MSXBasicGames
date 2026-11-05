# Créditos e dependências

- Projeto e protótipo Kingdoms at War: BigFive Studios, 2026.
- MSXgl: Guillaume “Aoineko” Blanchard e colaboradores. https://github.com/aoineko-fr/MSXgl
- Revisão usada: 6fd8858dce9a5a674529b78ec13d64262810eb34.
- MSXgl e fonte bitmap incluída no binário: Creative Commons Attribution-ShareAlike 4.0 International, texto preservado em `LICENSE_MSXgl.txt`.
- SDCC 4.6.0 #16555 e MSXhex 1.0.0: ferramentas externas de compilação, não redistribuídas neste pacote.
- openMSX 19.1 e C-BIOS 0.29a: ferramentas externas de teste, não redistribuídas neste pacote.

A configuração `msxgl_config.h` deriva do template MSX2 do MSXgl. Os fontes do jogo foram adaptados do POC fornecido para APIs e build reais. Não foram necessárias modificações no checkout do MSXgl. `input.c` já agrega o driver de mouse e teclado; não devem ser compilados novamente como módulos separados.

Nesta edição DualVDP, o adaptador V9990 usa as APIs públicas e o exemplo P1 do MSXgl. A implementação própria para V9968 foi orientada pelo repositório de t.hara/hra1129; consulte LICENSE_V9968.txt e reference/SOURCES.md. Os assets do exemplo de GrafxKid não foram usados: os tiles/soldados são os do protótipo, com conversão da fonte bitmap MSXgl.

Na v0.4.4, os personagens são desenhos originais em pixels nativos, mantidos em tools/unit_assets.py e art/units/. Cenário em art/source/objects_source.png foi gerado para o projeto em etapa anterior e convertido pelo gerador visual_assets.py. A fonte bitmap MSXgl e seus créditos/licença foram preservados. Retratos-fonte continuam disponíveis, fora do HUD ativo.

Na v0.4.5 foram redesenhados os camponeses/soldados e introduzidos estados próprios de mineração e coleta. O codec PackBits do cenário/fontes é implementação do projeto, expandindo os mesmos dados para VRAM no boot. A compactação não altera os créditos/licença da fonte MSXgl.

Na v0.5, quartel, obra e contornos são desenhos originais em tools/gameplay_assets.py. O codec foi substituído por LZSS de janela 256, implementado pelo projeto. O teste de mouse usa eventos normais SDL2; o cabeçalho de referência e sua licença estão preservados em tests/host/SDL_events_reference.h. Nenhuma ferramenta de teste é ligada à ROM.

Na v0.6, os edifícios e classes adicionais foram desenhados originalmente em pixels pelo projeto, em tools/kingdom_assets.py e tools/kingdom_units.py, seguindo a direção visual conceitual aprovada. O fonte do BIOS e das ferramentas não é redistribuído. O uso temporário da página 0 emprega Sys_SetPage0Slot do MSXgl, mantendo os créditos e a licença do SDK.

Na v0.7, a arte aprovada nas amostras v0.6.1–v0.6.3 foi incorporada ao jogo. As imagens-fonte geradas para o projeto e os dados de geração estão em art07/source; os PNGs indexados usados pela ROM estão em art07/native. tools/prepare07.py prepara os bancos, fonte e poses. O renderer V9990 passou a bitmap B1. As rotinas próprias de banco e de renderização não alteram o SDK externo. Os scripts e resultados antigos não usados pela validação v0.7 foram omitidos do pacote final.
