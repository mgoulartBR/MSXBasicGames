# REINOS / Kingdoms at War - plano para a versao completa (v1.0)

Decisões do autor (24/11/2026): mapper no máximo, sem teto de ROM; mapa de 64×64 células (1024×1024 px) como alvo, motor parametrizado até 128×128; música e efeitos em FM (OPLL); japonês só com kana; água como barreira com pontes e vaus (naval fica como expansão); facção inimiga = **Insepultos** (código antigo: "mortos"/mummy).

## Arquitetura
- **Estados do jogo:** TÍTULO → MENU (Novo jogo, Continuar/missão, Idioma, Créditos) → BRIEFING → JOGO → VITÓRIA / DERROTA / FINAL DA CAMPANHA. Um laço por estado; o `main()` atual vira o estado JOGO.
- **Texto:** tabelas `texto[idioma][id]` em bancos da ROM (PT, EN, ES, JA). Fonte latina com acentos e fonte kana (hiragana + katakana, ~146 glifos) em padrões da camada A. Seleção de idioma salva em RAM (e SRAM/flash se houver).
- **Mapas grandes (64×64 e 128×128):** o mapa completo fica **na VRAM** (512 KB, ~90 KB livres: 64×64×2 B = 8 KB, 128×128×2 B = 32 KB) e comprimido na ROM; a camada B (64×64 padrões = 512 px) é uma janela deslizante: ao mover a câmera, escrevem-se só as linhas/colunas novas da tabela de nomes. O fog deixa de usar `fog_memory` (1 B/célula) e passa a ser derivado dos nomes (plain/tracejado) + conjuntos de bits compactos só para a área ativa.
- **RAM:** continua o gargalo (C000–F380). Tudo que escalar com o mapa vai para VRAM ou ROM; a RAM guarda só unidades, prédios e estado.
- **Campanha:** cada missão = mapa + lista de prédios/unidades iniciais + objetivos + textos (briefing/vitória/derrota). Condições: destruir a base, sobreviver N minutos, escoltar, capturar ponto, defender.
- **Água:** tiles intransponíveis; pontes e vaus passáveis (flag de terreno); sem unidades navais na v1.
- **Áudio FM:** MSXgl `msx-music` (OPLL) + replayer próprio por quadro no ISR; músicas em formato tracker/VGM convertido; efeitos em canais reservados (percussão/ canais 7–9). Detectar OPLL e continuar mudo se ausente.
- **Interface:** HUD novo, ícones de comandos, minimapa, tooltips curtos; pequenas mecânicas (rolagem na borda inferior - feito, atalhos, grupos de controle).

## Ordem de entrega
1. Base: estados, título, menu, seleção de idioma, tabelas de texto (4 idiomas) e fontes. *(rolagem na borda inferior já corrigida)*
2. Campanha: formato de missão, objetivos, briefings, telas de vitória/derrota/fim, storyline (humanos × Insepultos).
3. Mapas grandes: janela deslizante da camada B, mapa e fog na VRAM — **feito (v0.15)**: campanha de 8 missões, todas em mapas 64×64 (1–4) e 128×128 (5–8); falta ponte/vau com arte própria (os mapas já usam código de terreno vau).
4. Áudio FM — **feito (v0.16)**: 6 músicas e 12 efeitos em OPLL; falta ajuste de timbre/volume por ouvido, opção de silenciar no menu e músicas por missão.
5. Interface e polimento — **minimapa, grupos de controle, atalhos e silenciar feitos (v0.17)**; faltam HUD novo/ícones de comandos, tooltips e menu de opções; balanceamento; testes no V9990 real a cada marco.

## Pendências de arte (do autor)
Tela de título, telas de vitória/derrota/fim, retratos para briefing, tiles de ponte/vau, novos cenários de mapa; ícones do HUD novo.
