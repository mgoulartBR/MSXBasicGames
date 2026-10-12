#!/usr/bin/env python3 -I
"""Converte um subconjunto do banco do TVTower (XML) em tabelas C compactas para o MSX.

Uso: tools/convert_db.py <dir database/Default> <saida src/data/db_data.h>  (gera src/seg/seg_s3_b3.c e seg_s4_b3.c = segmentos 3 e 4 do mapper ASCII8)

Fonte da verdade: os XML originais. Nada aqui e escrito a mao; rode de novo para regenerar.
Mapeamentos TVTower -> categorias Mad TV estao em CATEGORIES (decisao de design documentada em docs/PORTING.md).
Flags TVTower usadas: 64 = X-rated (-> FSK18). Confirmado empiricamente: 160/194 filmes "Erotic" e 109 acao tem o bit.
"""
import sys, re, unicodedata, xml.etree.ElementTree as ET
from pathlib import Path

MOVIES_PER_CAT = 20
MAX_TITLE = 26
MAX_TITLE_CULTURE = 32   # Culture tem poucos titulos curtos; titulos longos sao cortados nas listas (TextN)
MAX_ADS = 24
NEWS_PER_AGENCY = 40     # 3 agencias x 40 = 120 noticias (segmento 4 do mapper)
NEWS_TITLE = 40         # tamanho fixo do titulo (registro fixo => copia simples por acessor)
AGENCIES = [("Politics", {0}), ("Showbiz", {1, 5}), ("Misc", {2, 3, 4})]
GENRE_TO_AGENCY = {g: i for i, (_, gs) in enumerate(AGENCIES) for g in gs}

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
        if d is None or r is None or (not d.get("year") and int(d.get("maingenre") or 0) != 6):   # Culture (genero 6): quase nenhum tem ano no banco -> usa 1985
            continue
        g = int(d.get("maingenre") or 0)
        if g not in GENRE_TO_CAT:
            continue
        t = en_title(p, lang)
        if not t or len(t) > (MAX_TITLE_CULTURE if GENRE_TO_CAT[g] == 5 else MAX_TITLE) or len(t) < 2:
            continue
        by_cat[GENRE_TO_CAT[g]].append(dict(
            guid=p.get("guid"), title=t, year=int(d.get("year") or 1985), cat=GENRE_TO_CAT[g],
            blocks=int(d.get("blocks") or 1),
            critics=int(r.get("critics") or 0), speed=int(r.get("speed") or 0), outcome=int(r.get("outcome") or 0),
            price=int(round(float(d.get("price_mod") or 1) * 100)),
            fsk18=1 if int(d.get("flags") or 0) & FLAG_XRATED else 0))
    movies = []
    for i in range(len(CATEGORIES)):
        lst = sorted(by_cat[i], key=lambda m: m["guid"])  # determinismo
        movies += lst[:MOVIES_PER_CAT]
    return movies

MAX_NAME = 18
N_ACTORS = 64
N_DIRECTORS = 24

def load_people(src):
    """guid -> dict(name, score). Nomes ja vem alterados (parodias) no banco do TVTower; sem nome = pessoa anonima (descartada)."""
    people = {}
    for f in ("database_people.xml", "database_people_fictional.xml"):
        for p in ET.parse(src / f).getroot().iter("person"):
            d = p.find("data")
            name = ((p.findtext("first_name") or "") + " " + (p.findtext("last_name") or "")).strip()
            if not name:
                name = (p.findtext("nick_name") or "").strip()
            name = ascii_fold(name)
            if not name or len(name) > MAX_NAME:
                continue
            a = {k: int(float(d.get(k) or 0)) for k in ("fame", "power", "humor", "charisma", "appearance", "affinity")} if d is not None else {}
            score = a.get("fame", 0) * 3 + sum(a.get(k, 0) for k in ("power", "humor", "charisma", "appearance")) + a.get("affinity", 0) // 3
            people[p.get("guid")] = dict(name=name, score=score)
    return people

def build_cast(root, movies, src):
    """Elenco por filme (diretor + 2 atores) e 'estrelas' 0..3 por pessoa. Pessoas reais do TVTower quando ha nome (so ~14% dos vinculos),
    senao preenche de forma deterministica (hash do guid) com viés pela qualidade do filme. Decisao de design: o modelo do jogo e proprio."""
    import hashlib
    people = load_people(src)
    used_a, used_d = {}, {}
    for p in root.iter("programme"):
        st = p.find("staff")
        if st is None: continue
        for m in st.findall("member"):
            g = (m.text or "").strip()
            if g in people:
                if m.get("function") == "2": used_a[g] = people[g]
                elif m.get("function") == "5": used_d[g] = people[g]
    actors = sorted(used_a.items(), key=lambda kv: (-kv[1]["score"], kv[1]["name"], kv[0]))[:N_ACTORS]
    directors = sorted(used_d.items(), key=lambda kv: (-kv[1]["score"], kv[1]["name"], kv[0]))[:N_DIRECTORS]
    def tiers(lst):
        n = len(lst)
        return [3 if i < max(1, n * 12 // 100) else 2 if i < n * 35 // 100 else 1 if i < n * 70 // 100 else 0 for i in range(n)]
    persons = []                                   # (nome, estrelas)
    for (g, d), t in zip(actors, tiers(actors)): persons.append((d["name"], t))
    na = len(persons)
    for (g, d), t in zip(directors, tiers(directors)): persons.append((d["name"], t))
    idx = {g: i for i, (g, _) in enumerate(actors)}
    idx.update({g: na + i for i, (g, _) in enumerate(directors)})
    by = {m["guid"]: m for m in movies}
    q = sorted(movies, key=lambda m: (m["critics"] + m["outcome"], m["guid"]))
    pct = {m["guid"]: i / max(1, len(q) - 1) for i, m in enumerate(q)}
    staff = {}
    for p in root.iter("programme"):
        if p.get("guid") in by and p.find("staff") is not None:
            staff[p.get("guid")] = [(m.get("function"), (m.text or "").strip()) for m in p.find("staff").findall("member")]
    def rnd(guid, k):
        return int(hashlib.md5((guid + str(k)).encode()).hexdigest()[:8], 16)
    def pick(guid, k, lo, hi, p):                  # sorteia uma pessoa de persons[lo:hi] com viés de estrela por p (0..1)
        u = (rnd(guid, k) % 1000) / 1000.0
        ps, pk, pn = 0.02 + 0.20 * p, 0.15 + 0.35 * p, 0.45
        want = 3 if u < ps else 2 if u < ps + pk * 0.5 else 1 if u < ps + pk else 0
        cands = [i for i in range(lo, hi) if persons[i][1] == want] or list(range(lo, hi))
        return cands[rnd(guid, k + 100) % len(cands)]
    cast = []
    for m in movies:
        g = m["guid"]; p = pct[g]
        d_real = [idx[x] for f, x in staff.get(g, []) if f == "5" and x in idx and idx[x] >= na][:1]
        a_real = []
        for f, x in staff.get(g, []):
            if f == "2" and x in idx and idx[x] < na and idx[x] not in a_real: a_real.append(idx[x])
        a_real = a_real[:2]
        d = d_real[0] if d_real else pick(g, 1, na, len(persons), p)
        acts = list(a_real)
        k = 2
        while len(acts) < 2:
            a = pick(g, k, 0, na, p); k += 1
            if a not in acts: acts.append(a)
        stars = persons[d][1] + persons[acts[0]][1] + persons[acts[1]][1]
        cast.append((d, acts[0], acts[1], stars))
    return persons, cast

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
    """120 noticias (40 por agencia). Preco/qualidade vem do banco: price (x10 -> u8), qualidade = media(min,max).
    Noticias com intervalo de ano restrito (year_range) sao preferidas fora (so 'atemporais' ou com -1)."""
    by = {i: [] for i in range(len(AGENCIES))}
    for n in root.iter("news"):
        d = n.find("data")
        t = resolve(first_text(n, "title/en"), lang)
        if d is None or not t:
            continue
        g = int(d.get("genre") or 0)
        if g not in GENRE_TO_AGENCY:
            continue
        title = ascii_fold(t)
        if len(title) < 6 or len(title) >= NEWS_TITLE:
            continue
        q = (int(d.get("quality_min") or 50) + int(d.get("quality_max") or 70)) // 2
        by[GENRE_TO_AGENCY[g]].append(dict(guid=n.get("guid"), title=title, agency=GENRE_TO_AGENCY[g],
                                           price=min(255, int(round(float(d.get("price") or 0.5) * 100))), quality=min(100, q)))
    out = []
    for i in range(len(AGENCIES)):
        out += sorted(by[i], key=lambda x: x["guid"])[:NEWS_PER_AGENCY]
    return out

def emit(movies, ads, news, out, persons, cast):
    """Gera <out>.h (declaracoes/contagens) e <out>.c (definicoes em ROM)."""
    hdr = ("// GERADO por tools/convert_db.py - NAO EDITAR. Dados derivados do TVTower (c) 2002-2024 TVTower-Team,\n"
           "// adaptados/alterados para MSX (ver LICENSES.md).\n")
    H = [hdr, "#pragma once", '#include "../game_types.h"', ""]
    H.append("#define DB_NUM_CATEGORIES %d" % len(CATEGORIES))
    H.append("#define DB_NUM_MOVIES %d" % len(movies))
    H.append("#define DB_NUM_ADS %d" % len(ads))
    H.append("#define DB_NUM_NEWS %d" % len(news))
    H.append("#define DB_NEWS_TITLE %d" % NEWS_TITLE)
    H.append("#define DB_NUM_AGENCIES %d" % len(AGENCIES))
    H.append("")
    H.append("extern const char* const g_CategoryName[DB_NUM_CATEGORIES];")
    H.append("extern const Movie g_Movies[DB_NUM_MOVIES];")
    H.append("extern const Ad g_Ads[DB_NUM_ADS];")
    H.append("extern const char* const g_AgencyName[DB_NUM_AGENCIES];")
    H.append("extern const NewsRec g_NewsRec[DB_NUM_NEWS];     // SEGMENTO 4 (banco 3 so mapeado dentro de Db_News)")
    H.append("#define DB_NUM_PERSONS %d" % len(persons))
    H.append("#define DB_NAME_LEN 19")
    H.append("extern const Person g_Person[DB_NUM_PERSONS];    // SEGMENTO 4 (acesso por Db_Cast)")
    H.append("extern const u8 g_Cast[DB_NUM_MOVIES][4];        // SEGMENTO 4: diretor, ator 1, ator 2, soma de estrelas (0..9)")
    L = [hdr, '#include "../data/db_data.h"', '#ifndef HOST_BUILD', '#include "font/font_mgl_sample6.h"          // fonte (usada so em Ui_Init)', '#endif', ""]
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
    L.append("const char* const g_AgencyName[DB_NUM_AGENCIES] = { %s };" % ", ".join(c_str(n) for n, _ in AGENCIES))
    N = [hdr, '#include "../game_types.h"', '#include "../data/db_data.h"', "",
         "// price = k$ por item /1 ... ver sim.c; quality 0..100; agency 0..2", "const NewsRec g_NewsRec[DB_NUM_NEWS] = {"]
    for n in news:
        N.append("\t{ %s, %d, %d, %d }," % (c_str(n["title"]), n["agency"], n["quality"], n["price"]))
    N.append("};")
    N.append("")
    N.append("// elenco: nome (<= 18 caracteres) e estrelas 0..3 (ranking por fama no banco do TVTower)")
    N.append("const Person g_Person[DB_NUM_PERSONS] = {")
    for nm, st in persons: N.append("\t{ %s, %d }," % (c_str(nm), st))
    N.append("};")
    N.append("const u8 g_Cast[DB_NUM_MOVIES][4] = {")
    for c in cast: N.append("\t{ %d, %d, %d, %d }," % c)
    N.append("};")
    base = Path(out).with_suffix("")
    (base.parent.parent / "seg" / "seg_s4_b3.c").write_text("\n".join(N) + "\n", encoding="utf-8")
    base.with_suffix(".h").write_text("\n".join(H) + "\n", encoding="utf-8")
    # o arquivo .c vira o SEGMENTO 3 do mapper (banco 3, 0xA000): MSXgl compila *_s3_b3.c na area SEG3
    (base.parent.parent / "seg" / "seg_s3_b3.c").write_text("\n".join(L) + "\n", encoding="utf-8")

def main():
    src, out = Path(sys.argv[1]), sys.argv[2]
    lang = load_lang(src / "lang" / "en.xml")
    movies = pick_movies(ET.parse(src / "database_programmes.xml").getroot(), lang)
    ads = pick_ads(ET.parse(src / "database_ads.xml").getroot(), lang)
    news = pick_news(ET.parse(src / "database_news.xml").getroot(), lang)
    persons, cast = build_cast(ET.parse(src / "database_programmes.xml").getroot(), movies, src)
    emit(movies, ads, news, out, persons, cast)
    print("elenco: %d pessoas (%s estrelas 3/2/1/0), filmes com soma>=6: %d" % (len(persons), [sum(1 for p in persons if p[1] == t) for t in (3, 2, 1, 0)], sum(1 for c in cast if c[3] >= 6)))
    print("filmes=%d (por categoria: %s) anuncios=%d noticias=%d (segmento 4) -> %s" % (
        len(movies), [sum(1 for m in movies if m["cat"] == i) for i in range(len(CATEGORIES))], len(ads), len(news), out))

if __name__ == "__main__":
    main()
