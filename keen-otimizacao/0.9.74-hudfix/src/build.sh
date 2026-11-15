#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MSXGL_ROOT="${MSXGL_ROOT:-$PROJECT_ROOT/../deps/MSXgl}"
mkdir -p "$PROJECT_ROOT/build"
python3 "$PROJECT_ROOT/tools/build_exit_zones.py"
python3 "$PROJECT_ROOT/tools/build_compact_hero.py"
python3 "$PROJECT_ROOT/tools/build_mines_assets.py"
python3 "$PROJECT_ROOT/tools/build_tuberia.py"
python3 "$PROJECT_ROOT/tools/build_rapids_assets.py"
python3 "$PROJECT_ROOT/tools/build_pass_assets.py"
python3 "$PROJECT_ROOT/tools/build_city_assets.py"
python3 "$PROJECT_ROOT/tools/build_apple_assets.py"
python3 "$PROJECT_ROOT/tools/build_grove_assets.py"
python3 "$PROJECT_ROOT/tools/build_bay_assets.py"
python3 "$PROJECT_ROOT/tools/build_swamp_assets.py"
python3 "$PROJECT_ROOT/tools/build_boobus_assets.py"
python3 "$PROJECT_ROOT/tools/build_castle_assets.py"
python3 "$PROJECT_ROOT/tools/build_solid_tiles.py"
python3 "$PROJECT_ROOT/tools/build_opaque_sprites.py"
python3 "$PROJECT_ROOT/tools/build_tight_sprites.py"
python3 "$PROJECT_ROOT/tools/trim_unused_defs.py"
python3 "$PROJECT_ROOT/tools/move_hill_tables.py"
python3 "$PROJECT_ROOT/tools/move_rapids_defs.py"
python3 "$PROJECT_ROOT/tools/build_enemy_spawns.py"
python3 "$PROJECT_ROOT/tools/build_fast_enemy.py"
python3 "$PROJECT_ROOT/tools/build_restore_bands.py"
python3 "$PROJECT_ROOT/tools/build_fast_stream.py"
python3 "$PROJECT_ROOT/tools/build_render_choices.py"
python3 "$PROJECT_ROOT/tools/build_timp_frames.py"
python3 "$PROJECT_ROOT/tools/check_fixed_textures.py"
python3 "$PROJECT_ROOT/tools/build_sprite_rows.py"
python3 "$PROJECT_ROOT/tools/build_hud_bounds.py"
python3 "$PROJECT_ROOT/tools/build_hud_cache.py"
bash "$PROJECT_ROOT/scripts/build-core15.sh"
cd "$PROJECT_ROOT/src"
rm -f out/keen_msx.rom
node "$MSXGL_ROOT/engine/script/js/build.js" "$@" 2>&1 | tee "$PROJECT_ROOT/build/compile.log"
# Upstream may report a compiler error without returning a failure exit status.
test -f out/keen_msx.rom
cp out/keen_msx.rom "$PROJECT_ROOT/build/keen_msx.rom"
cp out/keen_msx.map "$PROJECT_ROOT/build/keen_msx.map"
python3 "$PROJECT_ROOT/tools/seal_hardware_rom.py"
python3 - "$PROJECT_ROOT/build/keen_msx.rom" <<'PY'
import pathlib,sys,hashlib,re
p=pathlib.Path(sys.argv[1]); b=p.read_bytes()
assert len(b)==2097152, len(b)
assert b[:2]==b'AB', b[:2]
assert b[0xc0000:0xc4000]==(p.parent/'core15/CORE15.BIN').read_bytes(), 'Shared core payload differs'
assert b[0x94000:0x94000+54272]==(p.parent.parent/'assets/title/screen7.bin').read_bytes(), 'Title banks'
entry=int.from_bytes(b[2:4], 'little')
assert 0x4000 <= entry < 0x8000, entry
root=p.parent.parent
assert b[0x4000:0x14000]==(root/'assets/scene/atlas.bin').read_bytes(), 'Atlas bank packaging'
assert b[0x24000:0x34000]==(root/'assets/tuberia/atlas.bin').read_bytes()
tuber=(root/'assets/tuberia/packed_map.bin').read_bytes()
assert b[0x34000:0x34000+len(tuber)]==tuber
mapdata=(root/'assets/scene/packed_map.bin').read_bytes()
assert b[0x14000:0x14000+len(mapdata)]==mapdata, 'Map bank packaging'
scroll=(root/'assets/scene/scroll_diff.bin').read_bytes()
assert b[0x18000:0x18000+len(scroll)]==scroll, 'Scroll difference bank packaging'
collision=(root/'assets/scene/stage_data.bin').read_bytes()
assert b[0x17000:0x17000+len(collision)]==collision, 'Collision bank packaging'
assert b[0x80000:0x90000]==(root/'assets/bridge/atlas.bin').read_bytes()
for name,offset in [('packed_map.bin',0x90000),('stage_data.bin',0x93000)]:
    data=(root/'assets/bridge'/name).read_bytes();assert b[offset:offset+len(data)]==data
assert b[0x40000:0x50000]==(root/'assets/melon/atlas.bin').read_bytes()
for name,offset in [('packed_map.bin',0x50000),('stage_data.bin',0x53000)]:
    data=(root/'assets/melon'/name).read_bytes();assert b[offset:offset+len(data)]==data
assert b[0xf0000:0x100000]==(root/'assets/pass/atlas.bin').read_bytes()
for name,offset in [('load_data.bin',0xa1400),('stage_data.bin',0xa3800)]:
    data=(root/'assets/pass'/name).read_bytes();assert b[offset:offset+len(data)]==data
assert b[0xd8000:0xe8000]==(root/'assets/rapids/atlas.bin').read_bytes()
for name,offset in [('packed_map.bin',0xe8000),('stage_data.bin',0xeb000)]:
    data=(root/'assets/rapids'/name).read_bytes();assert b[offset:offset+len(data)]==data
symbols={n:int(v,16) for v,n in re.findall(r'^\s+([0-9A-F]{8})\s+([sl]__\w+)\s*$',(root/'build/keen_msx.map').read_text(),re.M)}
for area in ('CODE','HOME','INITIALIZER','GSINIT','GSFINAL'):
    n=symbols.get('l__'+area,0)
    if n: assert 0x4000<=symbols['s__'+area] and symbols['s__'+area]+n<=0x8000, 'Code spills into data bank'
assert symbols['s__SEG7']==0x78000 and symbols['l__SEG7']<=0x4000, 'Banked code overflow'
assert symbols['s__SEG14']==0xe8000 and symbols['l__SEG14']<=0x4000
assert symbols['s__SEG21']==0x158000 and symbols['l__SEG21']<=0x4000, 'Hardware sprite bank overflow'
for bank in (53,59):
    assert symbols['s__SEG'+str(bank)]==bank*65536+0x8000 and symbols['l__SEG'+str(bank)]<=0x4000
for bank in range(22,32):
    assert symbols['s__SEG'+str(bank)]==bank*65536+0x8000 and symbols['l__SEG'+str(bank)]<=0x4000, 'Hero data bank overflow'
assert symbols['s__SEG49']==0x318000 and symbols['l__SEG49']<=0x4000, 'Core result bank overflow'
assert symbols['s__SEG47']==0x2f8000 and symbols['l__SEG47']<=0x4000, 'Phase 4 bank overflow'
assert symbols['s__SEG45']==0x2d8000 and symbols['l__SEG45']<=0x4000, 'Phase 3 bank overflow'
assert symbols['s__SEG44']==0x2c8000 and symbols['l__SEG44']<=0x4000, 'Phase 2 bank overflow'
assert symbols['s__SEG43']==0x2b8000 and symbols['l__SEG43']<=0x4000, 'Benchmark code overflow'
assert symbols['s__SEG41']==0x298000 and symbols['l__SEG41']<=0x4000, 'Title code overflow'
assert symbols['s__SEG101']==0x658000 and symbols['l__SEG101']<=0x4000, 'Tater attack image bank overflow'
assert symbols['s__SEG100']==0x648000 and symbols['l__SEG100']<=0x4000, 'Aspar image bank overflow'
assert symbols['s__SEG50']==0x328000 and symbols['l__SEG50']<=0x4000, 'Mines FX bank overflow'
assert symbols['s__SEG15']==0xf8000 and symbols['l__SEG15']<=0x4000, 'Enemy code bank overflow'
assert symbols['s__SEG8']==0x88000 and symbols['l__SEG8']<=0x4000, 'World code overflow'
for bank in (112,113,114,115):
    assert symbols['s__SEG'+str(bank)]==bank*65536+0x8000 and symbols['l__SEG'+str(bank)]<=0x4000, 'Cache/opaque renderer bank overflow'
assert symbols['s__CODE']+symbols['l__CODE']+symbols['l__HOME']<=0x8000, 'Fixed bank overflow'
assert symbols['s__DATA']+symbols['l__DATA']+symbols.get('l__INITIALIZED',0)<0xF000, 'RAM leaves too little stack room'
for i,(bank,off) in enumerate([(30,4096),(41,4096),(49,4096),(45,4096),(44,4096),(47,12288)]):
    data=(root/'assets/city'/('atlas_part%d.bin'%i)).read_bytes();offset=bank*16384+off
    assert symbols['l__SEG'+str(bank)]<=off, 'City atlas overlaps code'
    assert b[offset:offset+len(data)]==data
assert symbols['l__SEG29']<=4096, 'City map overlaps code'
for name,offset in [('load_data.bin',0x75000),('stage_data.bin',0x77300)]:
    data=(root/'assets/city'/name).read_bytes();assert b[offset:offset+len(data)]==data
# Every raw resource must be byte exact and start after that bank's executable code.
ranges=[]
for offset,name in re.findall(r'offset: (0x[0-9a-f]+), file: "([^"]+)"',(root/'src/project_config.js').read_text()):
    start=int(offset,16);data=(root/'src'/name).read_bytes();end=start+len(data)
    assert b[start:end]==data, name
    bank=start//16384;off=start%16384
    if 'l__SEG'+str(bank) in symbols:assert symbols['l__SEG'+str(bank)]<=off, (name,'overlaps code')
    for lo,hi,old in ranges:assert end<=lo or start>=hi,(name,'overlaps',old)
    ranges.append((start,end,name))
assert len((root/'assets/apple/load_data.bin').read_bytes())==8908
apple_parts=b''.join((root/'assets/apple'/('map_part%d.bin'%i)).read_bytes() for i in range(5))
assert apple_parts==(root/'assets/apple/load_data.bin').read_bytes() and apple_parts[6700+35*8+4:6700+35*8+6]==bytes([72,32]), 'Apple HUD bounds not packaged'
print(f'ROM verified: {len(b)} bytes, entry={entry:04X}, SHA256={hashlib.sha256(b).hexdigest()}')
PY
