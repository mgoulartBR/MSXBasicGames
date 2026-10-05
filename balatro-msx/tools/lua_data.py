"""Tiny extractor for Balatro's data tables (original/game.lua). The Lua source stays the single
source of truth: names, costs, rarities, sprite positions and blind parameters are read from it."""
import os, re

HERE = os.path.dirname(os.path.abspath(__file__))
ORIG = os.path.normpath(os.path.join(HERE, '..', 'original'))
TEX = os.path.join(ORIG, 'resources', 'textures', '1x')

def have_original():
    return os.path.isfile(os.path.join(ORIG, 'game.lua'))

def _field(v, name, num=True):
    if num:
        m = re.search(name + r'\s*=\s*(-?[\d.]+)', v)
        return float(m.group(1)) if m else None
    m = re.search(name + r'\s*=\s*["\']([^"\']+)["\']', v)
    return m.group(1) if m else None

def _pos(v):
    m = re.search(r'pos\s*=\s*\{x\s*=\s*(\d+),\s*y\s*=\s*(\d+)\}', v)
    return (int(m.group(1)), int(m.group(2))) if m else None

def centers():
    s = open(os.path.join(ORIG, 'game.lua'), encoding='utf8').read()
    out = {}
    for k, v in re.findall(r'^\s*((?:j|c|v|p|e|b|tag)_\w+)\s*=\s*\{([^\n]*)\},?\s*$', s, re.M):
        out[k] = dict(key=k, name=_field(v, 'name', False), cost=_field(v, 'cost'),
                      rarity=_field(v, 'rarity'), pos=_pos(v), line=v)
    return out

def blinds():
    s = open(os.path.join(ORIG, 'game.lua'), encoding='utf8').read()
    out = {}
    for k, v in re.findall(r'^\s*(bl_\w+)\s*=\s*\{([^\n]*)\},?\s*$', s, re.M):
        boss = re.search(r'boss\s*=\s*\{([^}]*)\}', v)
        out[k] = dict(key=k, name=_field(v, 'name', False), dollars=int(_field(v, 'dollars')),
                      mult=_field(v, 'mult'), pos=_pos(v), order=int(_field(v, 'order')),
                      boss=bool(boss), showdown=bool(boss and 'showdown' in boss.group(1)),
                      min=int(re.search(r'min\s*=\s*(\d+)', boss.group(1)).group(1)) if boss else 0)
    return out
