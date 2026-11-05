"""Freeze v0.11 sources and bind fresh functional/pixel evidence to final ROMs."""
from pathlib import Path
import json,hashlib,shutil
R=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
hashes={n:sha(R/(n+'.ROM')) for n in ['KAW9990','KAW68PV','KAW9968']};logs=[]
for v,n in [('V9990','KAW9990'),('PREVIEW','KAW68PV'),('V9968','KAW9968')]:
 names=[f'native_missing-{v}.log'] if v=='V9968' else [f'{c}-{v}.log' for c in [('defense011' if v=='V9990' else 'kingdom011'),'fog011','combat011','worker011','camera011','overlay011','perf091']]+[f'box011-{v}-port1.log']
 for name in names:
  s=(R/'tests'/name).read_text();assert s.startswith('ROM_SHA256 '+hashes[n]+'\n'),name
  assert 'FAIL' not in s and 'RESULT failures=0' in s,name;logs.append({'file':name,'sha256':hashes[n],'checks':s.count('PASS ')})
reports={}
for name in ['art011','fog011']:
 d=json.loads((R/'tests'/(name+'.json')).read_text());assert d['passed'] and d['rom_sha256']==hashes,name;reports[name]=d['checks']
d=json.loads((R/'tests/features011.json').read_text());assert d['passed']
for n,h in d['source_sha256'].items():assert sha(R/n)==h,n
assert json.loads((R/'tests/damage011.json').read_text())['passed']
for n in ['KAW68PV','KAW9990']:shutil.copyfile(R/(n+'.ROM'),R/'webmsx'/(n+' [ASCII8].rom'))
(R/'SHA256SUMS.txt').write_text(''.join(h+'  '+n+'.ROM\n' for n,h in hashes.items()))
frozen={str(p.relative_to(R)):sha(p) for p in (R/'src').rglob('*') if p.is_file() and p.suffix in {'.c','.h','.js'} and 'out' not in p.parts}
for n in ['build.sh','tools/prepare09.py','tools/validate011.py','tools/package011.py','tests/features011.py','tests/damage011.py','tests/verify_fog011.py','tests/verify_art011.py','tests/kingdom011.tcl','tests/defense011.tcl','tests/fog011.tcl','tests/helpers011.tcl','tests/combat011.tcl','tests/worker011.tcl','tests/camera011.tcl','tests/overlay011.tcl','tests/box011.tcl','tests/run_tests.py','tests/run_mouse.py']:
 frozen[n]=sha(R/n)
(R/'tests/source011.json').write_text(json.dumps(frozen,indent=2)+'\n')
v={'version':'0.11','passed':True,'rom_sha256':hashes,'scenario_logs':logs,'reports':reports,'limits':['V9968 physical positive test pending; PREVIEW is V9958','WebMSX and physical mouse not retested','Stationary defenders; no expansion AI or formal victory','Sight ignores obstacles; weapon LOS blocks them']}
(R/'tests/validation011.json').write_text(json.dumps(v,indent=2)+'\n')
(R/'test_report.md').write_text('''# v0.11 — Defesa, reparação e Fog of War

02/11/2026. MSXgl 6fd8858dce9a5a674529b78ec13d64262810eb34, SDCC 4.6.0 #16555, openMSX 19.1 / C-BIOS 0.29a. Arte aprovada preservada.

## Compilação

ROMs ASCII8 de 256 KiB para V9990, V9968 nativo e PREVIEW V9958. Verificação de bancos <=8 KiB, código fixo abaixo A000h e dados abaixo F380h. Layout e hashes em tests/art011.json e SHA256SUMS.txt.

## Fluxo real

Na PREVIEW, kingdom011 coleta madeira/ouro por ordens normais, constrói uma casa junto a um defensor e recebe dano real. Ordena reparação, verifica consumo de madeira e animação, derrota o agressor e acompanha a recuperação completa da casa. Constrói a árvore tecnológica, recruta as seis classes, conclui pesquisa, usa a torre contra o segundo defensor e destrói o castelo com aríete melhorado. A conservação de recursos inclui reparação.

fog011 verifica visão inicial, inimigos ocultos sem seleção/ordem direta, câmera sem revelar terreno, exploração por unidade, memória após a retirada, ausência de rebuild de visão em cena parada e posicionamento proibido sem visão. Captura RAM/VRAM sem alterar o estado do jogo.

Regressões: combate existente com exploração prévia, camponês recrutado/mina/obras, câmera, cancelamento de posicionamento, chegada de uma unidade, seleção por arrasto e ordens coletivas com mouse SDL real emulado. O teste de mouse explora antes de atacar, como exige o Fog. No V9990, defense011 confere reparação sob ataque e torre com coleta/construção normais; o fluxo completo de recrutamento e pesquisa é coberto pela PREVIEW e pelas funções C reais.

Todos os testes em emulador usam entradas normais e leituras de RAM/VRAM. Não há injeção de materiais, unidades, vida ou visibilidade. As contagens e hashes dos logs estão em tests/validation011.json.

## Lógica e imagem

- 5.171 verificações compilando funções C reais (incluindo 5.120 posições/raios de visão), com stubs controlados de vídeo/navegação: memória de terreno oculto, fontes de visão e mina, linha de tiro, torre/recarga/pool cheio, pesquisa/armadura, aríete/piqueiro/arqueiro e reparação com arredondamento, falta de madeira e destruição do alvo.
- 3.000 cenas aleatórias, 27.648.000 comparações de pixels, com funções C reais damage_mask, arrow_damage e fog_damage: sobreposições, restauração de fundo, regiões alteradas pelo Fog, terreno, seleção e posicionamento. É um modelo de composição, não emulação do VDP.
- Dumps reais com comparação de pixels dos três estados do Fog, janela inteira durante scroll sem atores, bordas/cancelamento do ghost, atlas da fonte e contexto militar nos dois painéis ocultos do HUD.
- 651 verificações de arte/payload/layout contra v0.10.

O cenário perf091 mede chegada e gera evidência de restauração. No V9990 clássico, a medição final apresentou 14 quadros em 3,10 s (42 loops/90 passos), contra 4 quadros em 4,75 s na primeira implementação do Fog. A v0.10 registrava 31 quadros em 3,00 s no mesmo percurso. A correção reduz o custo inicial, mas o Fog ainda deixa esse cenário mais pesado que a v0.10. São medições pontuais do emulador, não um benchmark completo 1/4/8 unidades nem uma medição de hardware físico. Os logs comparativos acompanham o pacote. A visão só é consolidada quando fontes mudam de célula/estado ou edifícios mudam, no máximo uma vez por quadro. Máscaras pré-calculadas, preservação do percurso observado e cache de invalidação evitam o recálculo por pixel e as varreduras repetidas. A memória do terreno atualiza apenas novas áreas visíveis/observadas e alterações explícitas do mapa. O Fog é aplicado junto à restauração regional, mantendo o HUD em buffers ocultos.

## Limites

Teste positivo físico do V9968 pendente; PREVIEW executa no V9958. WebMSX e mouse físico não foram retestados nesta entrega. A visão é octogonal e atravessa obstáculos; tiros têm obstrução separada. O terreno guarda a última aparência vista, sem marcadores fantasmas de unidades inimigas. Defensores permanecem estacionários, reagindo também a construções adjacentes. Não há IA de expansão nem condição formal de vitória/derrota. As animações de ataque reutilizam poses existentes.
''')
print(json.dumps({'passed':True,'scenario_checks':sum(d['checks'] for d in logs),'reports':reports,'rom_sha256':hashes},indent=2))
