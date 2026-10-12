# Licencas e creditos

| Componente | Licenca | Observacao |
|---|---|---|
| MSXgl (Aoineko) | CC BY-SA 4.0 | engine/ e codigo copiado dele herdam BY-SA |
| SDCC | GPL (runtime com excecao de linking) | so ferramenta de build |
| openMSX / C-BIOS | GPL / BSD | so ferramentas de teste |
| Dados do TVTower | zLib/LibPNG "restrita" (README oficial) | **uso comercial exige permissao**; atribuicao obrigatoria; versoes alteradas devem ser marcadas como tal |
| Mad TV (1991, Rainbow Arts) | copyright dos detentores | arquivos originais NAO sao versionados neste repositorio |

## Licenca do TVTower (texto recebido do mantenedor do port, reproduzido sem alteracao)
TVTower uses a "restricted" zLib/LibPNG-licence:
This software is provided 'as-is'. No warranty is given. The authors cannot be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, and to alter it and redistribute it freely, subject to the following restrictions:

1. Any commercial usage requires explicit permission from the original authors.
2. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product is required.
3. Altered source or binary versions of this software must be plainly marked as such, and must not be misrepresented as being the original software.
4. This notice may not be removed or altered from any source distribution.

### Como este projeto cumpre
- Nao comercial: gratis, sem venda, sem anuncios, sem doacoes atreladas ao download (a restricao 1 so vale para uso comercial). Qualquer uso comercial exige pedir permissao a equipe do TVTower (`docs/LICENSE_REQUEST.md`).
- Reconhecimento no produto (2): tela de creditos e abertura mostram "Data: TVTower (altered for MSX)"; README e este arquivo citam o TVTower.
- Versao alterada (3): os dados foram convertidos por `tools/convert_db.py` (ASCII, campos reescalados, elenco regenerado); o jogo e um port proprio do Mad TV e nao e o TVTower.
- Aviso mantido (4): o texto acima acompanha todo o codigo-fonte e toda distribuicao.

### Ressalva (honestidade)
O texto acima e a licenca do *software* TVTower. Os XML de `database/` incluem contribuicoes de usuarios e referencias a TMDB/IMDb/Wikidata; a licenca desse *conteudo* pode ser diferente. A publicacao e feita com base na licenca acima; se a equipe do TVTower ou algum detentor pedir, a ROM e as tabelas serao retiradas ou alteradas. O pedido de cortesia em `docs/LICENSE_REQUEST.md` continua recomendado, mas nao e mais condicao para a distribuicao gratuita.

- Este port e **nao comercial** e deve creditar: "Dados derivados do TVTower (c) 2002-2024 TVTower-Team,
  adaptados/alterados para MSX" e o jogo original Mad TV (Rainbow Arts, 1991).

## Arte do predio e sprites (0.5)
Desenhados para este projeto (retangulos procedurais e pixel art 16x16 propria em `src/building.c`); nao derivam de graficos do Mad TV nem do TVTower.
