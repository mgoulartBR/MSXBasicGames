"""Tetos residentes: preservacao das paredes e limites de TODOS os lotes do WAD local."""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import e1m1_pvs as P

L = P.load(os.environ['E1M1_WAD'])
count = total = 0
for bx, by in P.open_blocks(L):
    vps = P.block_viewpoints(L, bx, by)
    if not vps:
        continue
    visible = sorted(P.visible_subs(L, vps))
    tiles = P.visible_tiles(L, vps, bx, by)
    for hd in range(P.NHEAD):
        angle = P.head_deg(hd)
        ids = [j for j in visible if P.inview(L, j, bx, by, angle, P.CONE)]
        floors = [t for t in tiles if P.tile_in_cone(L, t, bx, by, angle, P.CONE)]
        model = P.build_block(L, bx, by, ids=ids, tiles=floors)
        before = {key: list(model[key]) for key in ('verts', 'faces', 'uvs', 'flat')}
        kept = model['kept']
        added = P.ceiling_faces(L, model, bx, by, tiles, angle)
        assert 0 <= added <= P.MAX_CEIL_FACES
        assert len(model['verts']) <= P.MAX_VERTS
        assert len(model['faces']) <= P.MAX_FACES
        assert model['kept'] == kept
        for key, values in before.items():
            assert model[key][:len(values)] == values, (bx, by, hd, key)
        for face, uv, flat in zip(model['faces'][len(before['faces']):],
                                  model['uvs'][len(before['faces']):],
                                  model['flat'][len(before['faces']):]):
            kind, sector = flat
            assert kind == 'C' and L.m.sectors[sector]['ctex'] != 'F_SKY1'
            assert all(model['verts'][v][1] == L.m.sectors[sector]['ceil'] for v in face)
            assert uv == (0,) * 8
        count += 1
        total += added
assert total > 0
print(f'PASS {count} lotes: paredes/UVs/vertices preservados; {total} tetos adicionados sem exceder o orcamento')
