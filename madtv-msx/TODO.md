# TODO (DONE / PARTIAL / TODO / BLOCKED)

## Critical
- DONE   0.1 pipeline: MSXgl v1.5.0 + SDCC 4.6.0 -> ROM 32K -> openMSX (C-BIOS MSX2) -> screenshot
- TODO   Confirmar licenca do conteudo do banco TVTower antes de distribuir
- DONE   0.4 noticias, arquivo (venda), chefe (credito), salvar/carregar por codigo, codigo banked (segmentos 5-8)
- DONE   0.3 nucleo de gestao: relogio, grade, audiencia, Image, contratos, dinheiro, falencia, rivais heuristicos
- DONE   0.2 navegador de filmes (dados convertidos do TVTower, input teclado/joystick, smoke test)
- DONE   Mapper ASCII8 (128 KB) + engine enxuto (-7,9 KB) + RAM nao zerada corrigida (0.3.3)
- DONE   Acessor de noticias com troca de segmento (Db_News)
- TODO   Catalogo maior: mais filmes (segmentos de dados extras) e acessor tambem para filmes
## Gameplay
- PARTIAL relogio/grade/audiencia/Image/financas/contratos feitos (0.3); balanceamento validado por autoplay (tests/balance.sh); revisar quando entrarem torres/juros/presentes
- TODO   Betty e presentes, producao propria + estudios, Sammys, sabotagem, torres/satelites, escolher emissora/dificuldade, grade de amanha, Image<20 com consequencias
## Graphics
- TODO   Fonte, UI Screen 5 (paleta), cenario do predio
## Audio
- TODO   PSG (musica/SFX)
## Performance
- DONE   texto ~3x mais rapido (fonte em VRAM com 6 variantes de cor)
- TODO   Catalogo maior (hoje 88 filmes) com bancos ASCII16
## Polish / Testing
- TODO   Smoke tests automatizados (tests/)
