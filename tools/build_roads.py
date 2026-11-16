"""OpenStreetMap (Overpass JSON) yollarını arazi koordinatlarına (metre) çevirir.
Kullanım: python3 tools/build_roads.py <roads.json> <terrain.json> <cikis.json>
"""
import sys, json, math

src, meta_path, out = sys.argv[1:4]
meta = json.load(open(meta_path))
lat_min, lon_min, lat_max, lon_max = meta["bbox"]
lat_c = (lat_min + lat_max) / 2
mlat = 111320.0
mlon = 111320.0 * math.cos(math.radians(lat_c))
W, D = meta["width"], meta["depth"]

# Sınıf -> (şerit genişliği metre, öncelik)
WIDTH = {"motorway": 22, "trunk": 18, "primary": 14, "secondary": 10, "tertiary": 8}

roads = []
for el in json.load(open(src))["elements"]:
    if el.get("type") != "way" or "geometry" not in el:
        continue
    hw = el["tags"].get("highway", "")
    base = hw.replace("_link", "")
    if base not in WIDTH:
        continue
    width = WIDTH[base] * (0.5 if hw.endswith("_link") else 1.0)
    pts = []
    for g in el["geometry"]:
        x = (g["lon"] - lon_min) * mlon - W / 2
        z = (lat_max - g["lat"]) * mlat - D / 2
        pts += [round(x, 1), round(z, 1)]
    if len(pts) < 4:
        continue
    bridge = el["tags"].get("bridge") in ("yes", "viaduct")
    tunnel = el["tags"].get("tunnel") in ("yes", "building_passage")
    if tunnel:
        continue  # tüneller arazi altında; şimdilik çizilmez
    roads.append({"c": base, "w": width, "b": 1 if bridge else 0,
                  "l": int(el["tags"].get("layer", "0") or 0) if el["tags"].get("layer", "0").lstrip("-").isdigit() else 0,
                  "p": pts})

json.dump({"roads": roads}, open(out, "w"), separators=(",", ":"))
print(len(roads), "yol yazıldı")
