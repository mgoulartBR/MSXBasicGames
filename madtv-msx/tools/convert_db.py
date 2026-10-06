#!/usr/bin/env python3 -I
"""Converte um subconjunto do banco do TVTower (XML) em tabelas C compactas para o MSX.

Uso: tools/convert_db.py <dir database/Default> <saida src/data/db_data.h>  (gera tambem db_data.c)

Fonte da verdade: os XML originais. Nada aqui e escrito a mao; rode de novo para regenerar.
Mapeamentos TVTower -> categorias Mad TV estao em CATEGORIES (decisao de design documentada em docs/PORTING.md).
Flags TVTower usadas: 64 = X-rated (-> FSK18). Confirmado empiricamente: 160/194 filmes "Erotic" e 109 acao tem o bit.
"""
import sys, re, unicodedata, xml.etree.ElementTree as ET
from pathlib import Path

MOVIES_PER_CAT = 12
MAX_TITLE = 26
MAX_ADS = 24
MAX_NEWS = 24

# categorias Mad TV (indice) -> (nome, generos TVTower)
CATEGORIES = [
    ("Lovestory",  {15}),
    ("Action",     {2, 1, 18}),
    ("Monumental", {13, 11}),
    ("Comedy",     {5}),
    ("Crime",      {4, 17, 14}),
    ("Culture",    {6}),
    ("SciFi",      {16, 10}),
    ("Other",      {3, 7, 9, 12}),
]
GENRE_TO_CAT = {g: i for i, (_, gs) in enumerate(CATEGORIES) for g in gs}
FLAG_XRATED = 64

def ascii_fold(s):
    s = s.replace("\u00bd", "1/2").replace("\u2019", "'").replace("\u2013", "-").replace("\u2014", "-")
    s = unicodedata.normalize("NFKD", s)
    s = "".join(c for c in s if not unicodedata.combining(c))
    return "".join(c for c in s if 32 <= ord(c) < 127).strip()

def load_lang(path):
    d = {}
    for el in ET.parse(path).getroot().iter():
        if el.text and el.text.strip() and len(el) == 0:
            d[el.tag.lower()] = el.text.strip()
    return d

def resolve(text, lang):
    if text is None:
        return None
    def sub(m):
        return lang.get(m.group(1).lower(), m.group(0))
    out = re.sub(r"\$\{([A-Za-z0-9_]+)\}", sub, text)
    return None if "${" in out else out

def first_text(el, path):
    t = el.find(path)
    return t.text if t is not None else None

def en_title(p, lang):
    for path in ("title/en", "title/all", "title/de"):
        t = resolve(first_text(p, path), lang)
        if t:
            return ascii_fold(t)
    return None

def c_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

def pick_movies(root, lang):
    by_cat = {i: [] for i in range(len(CATEGORIES))}
    for p in root.iter("programme"):
        if p.get("product") != "1" or p.get("licence_type") != "1":
            continue
        d, r = p.find("data"), p.find("ratings")
        if d is None or r is None or not d.get("year"):
            continue
        g = int(d.get("maingenre") or 0)
        if g not in GENRE_TO_CAT:
            continue
        t = en_title(p, lang)
        if not t or len(t) > MAX_TITLE or len(t) < 2:
            continue
        by_cat[GENRE_TO_CAT[g]].append(dict(
            guid=p.get("guid"), title=t, year=int(d.get("year")), cat=GENRE_TO_CAT[g],
            blocks=int(d.get("blocks") or 1),
            critics=int(r.get("critics") or 0), speed=int(r.get("speed") or 0), outcome=int(r.get("outcome") or 0),
            price=int(round(float(d.get("price_mod") or 1) * 100)),
            fsk18=1 if int(d.get("flags") or 0) & FLAG_XRATED else 0))
    movies = []
    for i in range(len(CATEGORIES)):
        lst = sorted(by_cat[i], key=lambda m: m["guid"])  # determinismo
        movies += lst[:MOVIES_PER_CAT]
    return movies

def pick_ads(root, lang):
    """Contratos jogaveis no MSX: audiencia minima 0.5..9.0 milhoes (alcance maximo do jogo ~12M), prazo >= 1 dia.
    Amostra espalhada ao longo da faixa de audiencia (nao so os primeiros por guid)."""
    ads = []
    for a in root.iter("ad"):
        c, d = a.find("conditions"), a.find("data")
        t = resolve(first_text(a, "title/all") or first_text(a, "title/en"), lang)
        if c is None or d is None or not t:
            continue
        ma = float(c.get("min_audience") or 0)
        if not (0.5 <= ma <= 9.0) or int(d.get("duration") or 0) < 1:
            continue
        ads.append(dict(guid=a.get("guid"), title=ascii_fold(t)[:MAX_TITLE], min_audience=ma,
                        reps=int(d.get("repetitions") or 1), days=int(d.get("duration") or 1),
                        profit=int(d.get("profit") or 0), penalty=int(d.get("penalty") or 0)))
    ads.sort(key=lambda x: (x["min_audience"], x["guid"]))
    step = max(1, len(ads) // MAX_ADS)
    return ads[::step][:MAX_ADS]

def pick_news(root, lang):
    out = []
    for n in root.iter("news"):
        d = n.find("data")
        t = resolve(first_text(n, "title/en"), lang)
        if d is None or not t:
            continue
        out.append(dict(guid=n.get("guid"), title=ascii_fold(t)[:MAX_TITLE * 2],
                        genre=int(d.get("genre") or 0), price=int(round(float(d.get("price") or 1) * 10))))
    return sorted(out, key=lambda x: x["guid"])[:MAX_NEWS]

def emit(movies, ads, news, out):
    """Gera <out>.h (declaracoes/contagens) e <out>.c (definicoes em ROM)."""
    hdr = ("// GERADO por tools/convert_db.py - NAO EDITAR. Dados derivados do TVTower (c) 2002-2024 TVTower-Team,\n"
           "// adaptados/alterados para MSX (ver LICENSES.md).\n")
    H = [hdr, "#pragma once", '#include "../game_types.h"', ""]
    H.append("#define DB_NUM_CATEGORIES %d" % len(CATEGORIES))
    H.append("#define DB_NUM_MOVIES %d" % len(movies))
    H.append("#define DB_NUM_ADS %d" % len(ads))
    H.append("#define DB_NUM_NEWS %d" % len(news))
    H.append("")
    H.append("extern const char* const g_CategoryName[DB_NUM_CATEGORIES];")
    H.append("extern const Movie g_Movies[DB_NUM_MOVIES];")
    H.append("extern const Ad g_Ads[DB_NUM_ADS];")
    H.append("extern const News g_News[DB_NUM_NEWS];")
    L = [hdr, '#include "db_data.h"', ""]
    L.append("const char* const g_CategoryName[DB_NUM_CATEGORIES] = { %s };" % ", ".join(c_str(n) for n, _ in CATEGORIES))
    L.append("")
    L.append("// year = ano-1850; price = price_mod*100 (preco real e calculado em runtime); fsk18 = flag X-rated do TVTower")
    L.append("const Movie g_Movies[DB_NUM_MOVIES] = {")
    for m in movies:
        L.append("\t{ %s, %d, %d, %d, %d, %d, %d, %d, %d }," % (
            c_str(m["title"]), m["cat"], m["year"] - 1850, m["blocks"], m["critics"], m["speed"], m["outcome"], m["price"], m["fsk18"]))
    L.append("};\n")
    L.append("// min_audience = milhoes x10")
    L.append("const Ad g_Ads[DB_NUM_ADS] = {")
    for a in ads:
        L.append("\t{ %s, %d, %d, %d, %d, %d }," % (c_str(a["title"]), int(round(a["min_audience"] * 10)), a["reps"], a["days"], a["profit"], a["penalty"]))
    L.append("};\n")
    L.append("const News g_News[DB_NUM_NEWS] = {")
    for n in news:
        L.append("\t{ %s, %d, %d }," % (c_str(n["title"]), n["genre"], n["price"]))
    L.append("};")
    base = Path(out).with_suffix("")
    base.with_suffix(".h").write_text("\n".join(H) + "\n", encoding="utf-8")
    base.with_suffix(".c").write_text("\n".join(L) + "\n", encoding="utf-8")

def main():
    src, out = Path(sys.argv[1]), sys.argv[2]
    lang = load_lang(src / "lang" / "en.xml")
    movies = pick_movies(ET.parse(src / "database_programmes.xml").getroot(), lang)
    ads = pick_ads(ET.parse(src / "database_ads.xml").getroot(), lang)
    news = pick_news(ET.parse(src / "database_news.xml").getroot(), lang)
    emit(movies, ads, news, out)
    print("filmes=%d (por categoria: %s) anuncios=%d noticias=%d -> %s" % (
        len(movies), [sum(1 for m in movies if m["cat"] == i) for i in range(len(CATEGORIES))], len(ads), len(news), out))

if __name__ == "__main__":
    main()
