"""OpenStreetMap yollarını arazi koordinatlarına (metre) çevirir.

Girdi: Overpass JSON (out geom) veya Geofabrik .osm.pbf dosyası.
Kullanım: python3 tools/build_roads.py <roads.json|bolge.osm.pbf> <terrain.json> <cikis.json>
"""
import sys, json, math

src, meta_path, out = sys.argv[1:4]
meta = json.load(open(meta_path))
lat_min, lon_min, lat_max, lon_max = meta["bbox"]
lat_c = (lat_min + lat_max) / 2
mlat = 111320.0
mlon = 111320.0 * math.cos(math.radians(lat_c))
W, D = meta["width"], meta["depth"]

# Avrupa / GTA 5 ölçeği:
# sınıf -> (varsayılan şerit, şerit genişliği m, kaldırım genişliği m (0 = yok), sol omuz, sağ omuz)
CLASS = {
    "motorway":     (3, 3.75, 0.0, 1.0, 3.0),   # otoban: tek yön 3 şerit + 3 m emniyet şeridi
    "trunk":        (3, 3.50, 0.0, 0.75, 2.0),  # şehir otoyolu
    "primary":      (4, 3.50, 4.0, 0.5, 0.5),   # ana cadde
    "secondary":    (4, 3.25, 3.5, 0.5, 0.5),
    "tertiary":     (2, 3.25, 3.0, 0.4, 0.4),
    "unclassified": (2, 3.00, 2.5, 0.3, 0.3),
    "residential":  (2, 3.00, 2.5, 0.3, 0.3),   # mahalle sokağı
}

def ways_from_overpass(path):
    for el in json.load(open(path))["elements"]:
        if el.get("type") == "way" and "geometry" in el:
            yield el["tags"], [(g["lat"], g["lon"]) for g in el["geometry"]]


def ways_from_pbf(path):
    import osmium
    for w in osmium.FileProcessor(path).with_locations().with_filter(osmium.filter.KeyFilter("highway")):
        if not w.is_way():
            continue
        try:
            pts = [(n.location.lat, n.location.lon) for n in w.nodes]
        except osmium.InvalidLocationError:
            continue
        if not any(lat_min <= la <= lat_max and lon_min <= lo <= lon_max for la, lo in pts):
            continue
        yield dict(w.tags), pts


def to_int(v, default):
    try:
        return int(str(v).split(";")[0])
    except ValueError:
        return default


reader = ways_from_pbf if src.endswith(".pbf") else ways_from_overpass
roads = []
for tags, geom in reader(src):
    hw = tags.get("highway", "")
    base = hw.replace("_link", "")
    if base not in CLASS:
        continue
    if tags.get("tunnel") in ("yes", "building_passage"):
        continue  # tüneller arazi altında; şimdilik çizilmez
    default_lanes, lane_w, walk_w, sh_l, sh_r = CLASS[base]
    link = hw.endswith("_link")
    oneway = tags.get("oneway") in ("yes", "1", "-1") or base == "motorway" or link
    # OSM'de şerit sayısı varsa onu kullan; yoksa sınıf varsayılanı (tek yönlü yolda yarısı)
    lanes = to_int(tags.get("lanes"), 1 if link else
                   (default_lanes if base in ("motorway", "trunk") else
                    max(1, default_lanes // 2) if oneway else default_lanes))
    lanes = max(1, min(lanes, 10))
    if link:
        sh_l, sh_r = min(sh_l, 0.5), min(sh_r, 1.5)
    if tags.get("sidewalk") == "no":
        walk_w = 0.0
    pts = []
    for la, lo in geom:
        x = (lo - lon_min) * mlon - W / 2
        z = (lat_max - la) * mlat - D / 2
        pts += [round(x, 1), round(z, 1)]
    if len(pts) < 4:
        continue
    roads.append({
        "c": base, "n": lanes, "o": 1 if oneway else 0,
        "lw": lane_w, "sl": sh_l, "sr": sh_r,
        "w": round(lanes * lane_w + sh_l + sh_r, 2),   # toplam asfalt genişliği
        "s": walk_w,
        "b": 1 if tags.get("bridge") in ("yes", "viaduct") else 0,
        "l": to_int(tags.get("layer"), 0),
        "p": pts,
    })

json.dump({"roads": roads}, open(out, "w"), separators=(",", ":"))
print(len(roads), "yol yazıldı")
