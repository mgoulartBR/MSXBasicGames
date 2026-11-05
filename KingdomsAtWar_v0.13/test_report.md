# v0.12 — Auditoria e correções de desempenho

02/11/2026. MSXgl 6fd8858dce9a5a674529b78ec13d64262810eb34, SDCC 4.6.0 #16555, openMSX 19.1 / C-BIOS 0.29a. ROM ASCII8 de 256 KiB.

## Medição reproduzível

Mesmo roteiro e entradas normais na v0.11 oficial e na v0.12. CPU Z80 padrão, sem turbo. Conta apresentações efetivas pelo contador rendered_frames, usando tempo emulado; não velocidade do computador hospedeiro. Cada caso é uma execução determinística, não uma estimativa estatística do jogo inteiro. O tempo inclui processamento da ordem e planejamento do caminho. O cenário de quatro unidades sucede o de uma unidade; os destinos são distintos e conferidos.

| Renderer | Unidades | v0.11 (quadros/s) | v0.12 (quadros/s) | Ganho | Tempo da ordem: antes → depois |
|---|---:|---:|---:|---:|---|
| V9990 | 1 | 4.41 | 5.99 | 35.7% | 3.17 s → 3.01 s |
| V9990 | 4 | 1.80 | 3.45 | 91.7% | 4.44 s → 3.47 s |
| PREVIEW / V9958 | 1 | 2.08 | 2.60 | 25.4% | 3.37 s → 3.07 s |
| PREVIEW / V9958 | 4 | 0.99 | 1.45 | 47.1% | 5.07 s → 4.14 s |

Os intervalos entre apresentações constam em tests/performance012.json e nos logs CADENCE. Eles incluem períodos de planejamento e telas sem alterações, portanto não são a frequência de atualização do monitor nem uma medição isolada de stuttering. No cenário de quatro unidades do V9990, a mediana caiu de 601 para 300 ms e o maior intervalo de 1118 para 467 ms. No PREVIEW, a mediana caiu de 1126 para 868 ms; ainda há pausas relevantes. Não há promessa de 30/60 FPS.

## Causas encontradas e correções

| Causa concreta | Correção implementada |
|---|---|
| Visão de construções imóveis recalculada em cada deslocamento | Plano estático de 128 bytes, invalidado ao criar, concluir ou destruir construção |
| Cálculo e clipping do octógono repetidos por linha | Máscaras exatas para cada coluna e raio em ROM; sem reduzir alcance ou frequência da visão |
| Consolidação de planos do Fog em laços caros de C | Kernel Z80 preservando IX/IY; mantém exploração ao longo do percurso e dano separado nos dois buffers |
| Fog percorria cada célula da janela e restaurava blocos adjacentes separadamente | Percurso de máscaras por byte, salto de trechos vazios e restauração de faixas contíguas; preserva alterações fora da câmera |
| Varreduras de projéteis e reparação sem atividade | Máscara de projéteis mantida já no lançamento e indicador de reparações pendentes |
| Novas poses nativas decodificavam e enviavam margens transparentes | Payload recortado exatamente aos limites de desenho; 92.160 → 42.942 bytes brutos, mantendo pixels, tamanho e poses |
| Chamadas por byte e cópias de descritores pequenas e repetidas | Transferência de VRAM em Z80 com espaçamento de 32 T por byte; cópias contíguas com builtin do SDCC |

Não foi removido Fog, coleta, combate, animação, seleção por área ou o mouse. A velocidade lógica das classes e as regras de combate foram mantidas. Não foi introduzida limpeza periódica de tela inteira. Key click continua desativado por ld a,0 / ld (0xF3DB),a.

## Pesquisa técnica

A investigação considerou custos clássicos do MSX: contenção de VRAM entre CPU, desenho e comandos; custo de muitos comandos pequenos; aritmética e laços gerados pelo compilador; chamadas entre bancos; reconstrução de elementos sem alterações; e busca de caminhos concorrente com renderização. As mudanças foram escolhidas pelo perfil do jogo, em vez de atribuir toda lentidão ao VDP.

Fontes primárias consultadas:

- [openMSX — medições de temporização da VRAM do V9938](https://openmsx.org/vdp-vram-timing/vdp-timing.html): referência para concorrência de acessos e custo dos comandos. São medições do V9938, não medições do V9968.
- [Manual oficial do SDCC](https://sdcc.sourceforge.net/doc/sdccman.pdf): cópias em bloco no Z80 e convenções de chamadas banked. A consulta ao manual não alterou a versão fixada do compilador.
- [Projeto V9968 Cartridge de hra1129](https://github.com/hra1129/V9968_Cartridge): alvo nativo, distinto do PREVIEW V9958.

## Verificação

- 5.171 verificações de visão, torres, pesquisa e reparação; 5.120 cobrem todos os centros do mapa e raios de visão usados.
- Kernel real do Z80: 257 casos em cada renderer emulado, 230.272 bytes de saída por execução e preservação de IX/IY. Este teste isolado injeta dados sintéticos e chama o kernel; não é usado como medição de gameplay.
- 3.000 cenas de dano visual, 27.648.000 comparações de pixels e três casos direcionados.
- 2.000 casos para faixas do Fog: cobertura exata, ausência de restauração duplicada e preservação dos bits fora da câmera.
- Conferência dos payloads originais e dos recortes nas ROMs; conferência dos pixels do cache nativo lidos da VRAM.
- Cenários reais por teclado/mouse: Fog, combate, morte/recrutamento, trabalho na mina e árvore, construções, reparação, pesquisa, torre, câmera, ghost de construção e seleção por arrasto. Os recursos foram coletados normalmente.
- Comparação de pixels da VRAM para câmera, HUD, terreno desconhecido/visível/explorado e overlays.

Os logs usados são ligados aos hashes das ROMs em tests/validation012.json. Relatórios e scripts acompanham o pacote.

## Memória e limites

| Alvo | Final do código fixo | Final dos dados |
|---|---|
| V9990 | 0x9acf | 0xee16 |
| PREVIEW V9958 | 0x9fd3 | 0xee53 |
| V9968 | 0x9ffe | 0xee53 |

Todos os bancos têm até 8 KiB; código fixo abaixo de A000h e dados abaixo de F380h. O espaço de código fixo do V9968 está muito próximo do limite; ampliar funcionalidades exigirá mover rotinas para bancos livres. O build verifica esse limite.

O openMSX disponível não emula o cartucho V9968 específico. A ROM nativa foi compilada e a detecção de ausência do dispositivo foi testada; o teste positivo em hardware permanece pendente. Os números do PREVIEW medem V9958, não a aceleração do V9968. WebMSX não foi medido diretamente nesta rodada; o protocolo de mouse foi testado no emulador por eventos SDL reais.
