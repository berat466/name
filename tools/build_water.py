"""OpenStreetMap su alanlarını (Han Nehri, göller, büyük dereler) arazi koordinatlarına (metre) çevirir.
Kullanım: python3 tools/build_water.py <bolge.osm.pbf> <terrain.json> <cikis.json>
"""
import sys, json, math
import osmium

src, meta_path, out = sys.argv[1:4]
meta = json.load(open(meta_path))
lat_min, lon_min, lat_max, lon_max = meta["bbox"]
mlat = 111320.0
mlon = 111320.0 * math.cos(math.radians((lat_min + lat_max) / 2))
W, D = meta["width"], meta["depth"]
MIN_AREA = 20000.0   # m² : küçük gölet ve havuzları atla

def ring_xy(ring):
    return [((n.lon - lon_min) * mlon - W / 2, (lat_max - n.lat) * mlat - D / 2) for n in ring]

def area_m2(r):
    return abs(sum(r[i][0] * r[(i + 1) % len(r)][1] - r[(i + 1) % len(r)][0] * r[i][1] for i in range(len(r)))) / 2

def simplify(r, tol=2.0):
    # basit seyreltme: ardışık noktalar tol içindeyse atla
    o = [r[0]]
    for p in r[1:]:
        if math.hypot(p[0] - o[-1][0], p[1] - o[-1][1]) >= tol:
            o.append(p)
    return o

polys = []
# A) etiketli kapalı alanlar (göller, kapalı nehir kıyıları)
fp = osmium.FileProcessor(src).with_areas().with_filter(osmium.filter.KeyFilter("natural", "waterway", "landuse"))
for a in fp:
    if not a.is_area():
        continue
    t = a.tags
    water = (t.get("natural") == "water" or t.get("waterway") == "riverbank"
             or t.get("landuse") in ("reservoir", "basin"))
    if not water or t.get("water") in ("wastewater", "pond"):
        continue
    try:
        for outer in a.outer_rings():
            o = ring_xy(outer)
            if len(o) < 4 or area_m2(o) < MIN_AREA:
                continue
            ring_list = [simplify(o)]
            for inner in a.inner_rings(outer):
                i = ring_xy(inner)
                if len(i) >= 4 and area_m2(i) > 2000:
                    ring_list.append(simplify(i))
            polys.append([[round(c, 1) for p in r for c in p] for r in ring_list])
    except Exception:
        continue


# B) nehir ilişkileri (water=river, multipolygon): çıkarma sınırında kesik olabilir, parçaları kendimiz birleştiririz.
rel_members = {}   # relation id -> [(way id, role)]
for r in osmium.FileProcessor(src, osmium.osm.RELATION).with_filter(osmium.filter.KeyFilter("water")):
    t = r.tags
    if t.get("type") == "multipolygon" and t.get("natural") == "water" and t.get("water") in ("river", "lake", "reservoir"):
        rel_members[r.id] = [(m.ref, m.role) for m in r.members if m.type == "w"]
need = {w for ms in rel_members.values() for w, _ in ms}
ways = {}
for w in osmium.FileProcessor(src, osmium.osm.NODE | osmium.osm.WAY).with_locations():
    if w.is_way() and w.id in need:
        try:
            ways[w.id] = ([n.ref for n in w.nodes], ring_xy(w.nodes))
        except osmium.InvalidLocationError:
            pass

def assemble(chains):
    """(düğüm kimlikleri, koordinatlar) zincirlerini halkalara birleştirir; açık kalanları en yakın uçlarla bağlar."""
    chains = [(list(i), list(c)) for i, c in chains if len(i) > 1]
    rings, opens = [], []
    while chains:
        ids, xy = chains.pop()
        grew = True
        while grew and ids[0] != ids[-1]:
            grew = False
            for k, (i2, c2) in enumerate(chains):
                if i2[0] == ids[-1]: ids += i2[1:]; xy += c2[1:]
                elif i2[-1] == ids[-1]: ids += i2[-2::-1]; xy += c2[-2::-1]
                elif i2[-1] == ids[0]: ids = i2[:-1] + ids; xy = c2[:-1] + xy
                elif i2[0] == ids[0]: ids = i2[:0:-1] + ids; xy = c2[:0:-1] + xy
                else: continue
                chains.pop(k); grew = True; break
        (rings if ids[0] == ids[-1] else opens).append(xy)
    # açık zincirler (çıkarma sınırında kesilmiş): en yakın uçları birleştir, kalanı kapat
    dist = lambda p, q: math.hypot(p[0] - q[0], p[1] - q[1])
    while len(opens) > 1:
        best = None
        for i in range(len(opens)):
            for j in range(i + 1, len(opens)):
                for ei in (0, -1):
                    for ej in (0, -1):
                        d = dist(opens[i][ei], opens[j][ej])
                        if best is None or d < best[0]: best = (d, i, j, ei, ej)
        _, i, j, ei, ej = best
        a, b = opens[i], opens[j]
        if ei == 0: a = a[::-1]          # a'nın sonu birleşme ucu olsun
        if ej == -1: b = b[::-1]         # b'nin başı birleşme ucu olsun
        opens = [o for k, o in enumerate(opens) if k not in (i, j)] + [a + b]
    if opens: rings.append(opens[0])
    return rings

for rid, ms in rel_members.items():
    outer = assemble([ways[w] for w, role in ms if w in ways and role != "inner"])
    inner = assemble([ways[w] for w, role in ms if w in ways and role == "inner"])
    for o in outer:
        if len(o) < 4 or area_m2(o) < MIN_AREA:
            continue
        rl = [simplify(o)] + [simplify(i) for i in inner if len(i) >= 4 and area_m2(i) > 2000]
        polys.append([[round(c, 1) for p in r for c in p] for r in rl])

json.dump({"polys": polys}, open(out, "w"), separators=(",", ":"))
print(len(polys), "su alanı yazıldı")
