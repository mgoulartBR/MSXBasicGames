# TODO (DONE / PARTIAL / TODO / BLOCKED)

## Critical
- DONE   0.1 pipeline: MSXgl v1.5.0 + SDCC 4.6.0 -> ROM 32K -> openMSX (C-BIOS MSX2) -> screenshot
- TODO   Confirmar licenca do conteudo do banco TVTower antes de distribuir
- DONE   0.6 Betty e presentes: Supermercado (10 presentes), escritorio da Betty (simpatia <= Image, rivais cortejam, efeito cai 30%/uso), falencia de emissora (Image 0), pedido de casamento + tela final; savegame v5 (98 caracteres)
- DONE   0.7 campo da Betty (Bnn) no cabecalho; autoplay com presentes (casa em 64% das partidas no Normal, ver docs/PORTING.md); Image < 20 = anunciantes pagam 20% menos e o chefe fecha o credito; catalogo de 144 filmes; dificuldade Easy/Normal/Hard no titulo (setas ou clique); savegame v6 (109 caracteres)
- TODO   Escolher a emissora do jogador (so a dificuldade foi feita; a emissora seria so cosmetica) ; autoplay ainda joga com heuristicas simples (um humano pode ir melhor ou pior)
- TODO   Culture tem so 4 filmes no catalogo (filtro de titulo <= 26 letras do conversor)
- DONE   0.5.2 mouse com autodeteccao (cursor sempre visivel; M cicla auto/porta1/porta2/off)
- DONE   0.5.1 mouse (cursor-sprite, hit-test em todas as telas, M liga/desliga, direito = voltar)
- TODO   Validar mouse em hardware MSX real (sensibilidade, clique curto); opcao de sensibilidade
- DONE   0.5 predio navegavel (5 andares, elevador, sprites do jogador/rivais, viagem ate as portas, escritorio como submenu)
- DONE   0.4 noticias, arquivo (venda), chefe (credito), salvar/carregar por codigo, codigo banked (segmentos 5-8)
- DONE   0.3 nucleo de gestao: relogio, grade, audiencia, Image, contratos, dinheiro, falencia, rivais heuristicos
- DONE   0.2 navegador de filmes (dados convertidos do TVTower, input teclado/joystick, smoke test)
- DONE   Mapper ASCII8 (128 KB) + engine enxuto (-7,9 KB) + RAM nao zerada corrigida (0.3.3)
- DONE   Acessor de noticias com troca de segmento (Db_News)
- TODO   Catalogo maior: mais filmes (segmentos de dados extras) e acessor tambem para filmes
## Gameplay
- PARTIAL relogio/grade/audiencia/Image/financas/contratos feitos (0.3); balanceamento validado por autoplay (tests/balance.sh); revisar quando entrarem torres/juros/presentes
- TODO   Abrir salas fechadas do predio (porteiro, supermercado, roteiros, estudios, corretor, Betty); rivais interagindo (visitas, sabotagem)
- TODO   Betty e presentes, producao propria + estudios, Sammys, sabotagem, torres/satelites, escolher emissora/dificuldade, grade de amanha, Image<20 com consequencias
## Graphics
- PARTIAL predio com arte procedural simples (portas coloridas + rotulos); falta detalhe/animacao de portas e arte por sala
- TODO   Fonte, UI Screen 5 (paleta), cenario do predio
## Audio
- TODO   PSG (musica/SFX)
## Performance
- DONE   texto ~3x mais rapido (fonte em VRAM com 6 variantes de cor)
- TODO   Catalogo maior (hoje 88 filmes) com bancos ASCII16
## Polish / Testing
- TODO   Smoke tests automatizados (tests/)
