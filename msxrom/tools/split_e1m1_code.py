"""Move groups of SDCC Z80 functions to boot-loaded turbo R RAM."""
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
assembly = path.read_text()
markers=(('_door_blocked', '_ECODE'), ('_rast', '_CODE'),
                        ('_door_tick', '_ECODE'), ('_damage_player', '_CODE'),
                        ('_emit_water_rect', '_ECODE'), ('_render', '_CODE'))
positions=[assembly.index(symbol+':') for symbol,_ in markers]
if positions!=sorted(positions):
    raise ValueError('SDCC function order changed; review RAM segment boundaries')
for symbol, segment in markers:
    assembly, count = re.subn(r'^(' + symbol + r':)$',
                              r'\t.area ' + segment + r'\n\1', assembly,
                              count=1, flags=re.MULTILINE)
    if count != 1:
        raise ValueError(f'Cannot locate {symbol} in SDCC output')
path.write_text(assembly)
