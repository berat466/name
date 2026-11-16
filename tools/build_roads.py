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

# sınıf -> (varsayılan şerit sayısı, kaldırım var mı)
CLASS = {
    "motorway": (6, False), "trunk": (6, False), "primary": (4, True),
    "secondary": (4, True), "tertiary": (2, True), "unclassified": (2, True),
    "residential": (2, True),
}
LANE_W = 3.3


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
    default_lanes, sidewalk = CLASS[base]
    link = hw.endswith("_link")
    oneway = tags.get("oneway") in ("yes", "1", "-1") or base == "motorway" or link
    lanes = to_int(tags.get("lanes"), (1 if link else default_lanes // (2 if oneway and base == "motorway" else 1)))
    lanes = max(1, min(lanes, 10))
    pts = []
    for la, lo in geom:
        x = (lo - lon_min) * mlon - W / 2
        z = (lat_max - la) * mlat - D / 2
        pts += [round(x, 1), round(z, 1)]
    if len(pts) < 4:
        continue
    roads.append({
        "c": base, "n": lanes, "o": 1 if oneway else 0,
        "w": round(lanes * LANE_W + 1.0, 1),
        "s": 1 if sidewalk and tags.get("sidewalk") != "no" else 0,
        "b": 1 if tags.get("bridge") in ("yes", "viaduct") else 0,
        "l": to_int(tags.get("layer"), 0),
        "p": pts,
    })

json.dump({"roads": roads}, open(out, "w"), separators=(",", ":"))
print(len(roads), "yol yazıldı")
