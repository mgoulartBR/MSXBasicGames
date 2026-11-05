# Fontes técnicas e créditos

- MSXgl, Guillaume “Aoineko” Blanchard e colaboradores: https://github.com/aoineko-fr/MSXgl — revisão 6fd8858dce9a5a674529b78ec13d64262810eb34. Templates, biblioteca e fonte bitmap sob CC BY-SA; licença preservada em LICENSE_MSXgl.txt.
- V9968, t.hara / hra1129: https://github.com/hra1129/V9968_Cartridge — árvore consultada 02006e458267743a25edb792fc2f33f374961378. Licença MIT preservada em LICENSE_V9968.txt.
- Detecção de VDP principal/secundário: fpga/V9968_Cartridge_TangNano20K/src/v9968/detect/check_vdp_type.asm.
- Portas, configuração e inicialização: fpga/V9968_Cartridge_TangNano20K/src/v9968/devcon/msx_vdp.c.
- Scroll nativo: background_scroll(), em fpga/V9968_Cartridge_TangNano20K/src/v9968/devcon/devcon.c.
- V9990 P1: exemplo MSXgl projects/samples/s_v9990.c e headers engine/src/v9990.h / v9990_reg.h.
- Detalhes da emulação V9958, interrupção de linha e R#23: https://github.com/openMSX/openMSX/blob/master/src/video/VDP.cc e SpriteChecker.cc / PixelRenderer.cc. O openMSX executado localmente é 19.1; a leitura do código público foi usada como referência, não como alegação de identidade entre revisões.

O jogo mantém o mapa e a navegação da v0.2. Os assets de terreno e soldados são os desenhos originais do protótipo. O gerador converte a fonte MSXgl em padrões de 4 bpp para os dois adaptadores. Não foram incluídos sprites de terceiros dos exemplos V9990.

Na v0.4, a mina, a base e o trabalhador são desenhos originais do projeto. O script tools/update_assets.py insere esses padrões nos atlas já existentes, preservando a paleta de 16 cores.

A v0.4.2 substitui a base, mina e copas pelos desenhos-base em art/source/objects_source.png, gerados para este projeto com a ferramenta integrada de imagens. O atlas nativo é produzido por tools/visual_assets.py. Retratos e moldura fazem parte do bitmap/tiles do HUD; sprites têm quadros de caminhada e trabalho sem aumentar a quantidade simultânea.

## Estado atual v0.4.4

A composição de árvores e os quatro indicadores estáveis da v0.4.3 foram preservados. Retratos não fazem parte do HUD ativo. Os personagens foram redesenhados em pixels nativos no gerador tools/unit_assets.py, sem usar sprites dos exemplos do MSXgl. Base e detalhes geram src/unit_art.h, compartilhado entre os renderers. O V9990 expande esses dados para quatro bits por pixel; o caminho compatível admite detalhes no orçamento de oito sprites por linha. A constante de término da lista de sprites (Y=216) é documentada em engine/src/vdp.h do SDK usado.

## Estado atual v0.4.5

A PoC tem somente quatro aliados; não há sprites inimigos vermelhos. Trabalhadores possuem duas roupas e poses de levantar/golpear o machado. A mineração usa estados de entrada, permanência oculta e saída, com células dianteiras reservadas. Caminho e pontes isoladas em terra foram retirados; a travessia do rio permanece. Os arrays de cenário/fontes são guardados em PackBits e expandidos apenas no boot, com referências não compactadas e conferência da VRAM nos dois renderers. Fonte do codec: implementação própria em tools/pack_assets.py e src/asset_codec.h.

## Estado atual v0.5

A capacidade passou a seis aliados, com quatro iniciais. Quartel, construção, treinamento e mouse integram o ciclo de recursos. O codec ativo agora é LZSS com janela de 256 bytes (implementação própria nos mesmos arquivos); as menções PackBits acima descrevem a v0.4.5. O driver do mouse é o Mouse_Initialize/Mouse_Read do MSXgl fixado neste pacote. O teste utiliza eventos SDL no host e o mouse MSX emulado nas portas 1 e 2, sem escrever nos registradores PSG ou no estado do jogo. tests/host/SDL_events_reference.h preserva o cabeçalho SDL2 usado para conferir a estrutura de eventos.

## v0.6

Novos desenhos-fonte originais em tools/kingdom_assets.py e kingdom_units.py. Dados gráficos separados por página pelo suporte ROM_48K do MSXgl. Referências locais do SDK fixado: system.h/system.c (Sys_GetPageSlot, Sys_SetPage0Slot), engine/src/crt0/crt0_rom48.asm e engine/script/js/build.js (segmentos *_p0). BIOS restaurado antes de EI; padrões V9990 gerados offline e compactados por LZSS-256. Sem uso de RAM Mapper ou MDO.
