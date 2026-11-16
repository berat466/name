"""SRTM .hgt dosyalarından Seul arazisini kesip oyun için ikili yükseklik verisi üretir.
Kullanım: python3 tools/build_terrain.py <hgt_klasoru> <cikis_klasoru>
"""
import sys, json, numpy as np

LAT_MIN, LAT_MAX = 37.42, 37.72
LON_MIN, LON_MAX = 126.76, 127.19
N = 3601  # SRTM1: 1 yay saniyesi

src, out = sys.argv[1], sys.argv[2]
w = np.fromfile(f"{src}/N37E126.hgt", dtype=">i2").reshape(N, N).astype(np.float32)
e = np.fromfile(f"{src}/N37E127.hgt", dtype=">i2").reshape(N, N).astype(np.float32)
full = np.concatenate([w, e[:, 1:]], axis=1)  # ortak kenar sütunu bir kez

r0 = round((38 - LAT_MAX) * 3600); r1 = round((38 - LAT_MIN) * 3600)
c0 = round((LON_MIN - 126) * 3600); c1 = round((LON_MAX - 126) * 3600)
h = full[r0:r1 + 1, c0:c1 + 1]

# Su yüzeyindeki SRTM gürültüsünü (negatif değerler) bastır
h = np.maximum(h, -2.0)

# SRTM bir yüzey modelidir: şehirde bina/ağaç gürültüsü içerir.
# Hafif Gauss yumuşatma (sigma ~1 hücre) ile zemini oyun için düzgünleştir.
k = np.array([1, 4, 6, 4, 1], np.float32); k /= k.sum()
pad = np.pad(h, 2, mode="edge")
tmp = sum(k[i] * pad[:, i:i + h.shape[1]] for i in range(5))
h = sum(k[i] * tmp[i:i + h.shape[0], :] for i in range(5)).astype(np.float32)

rows, cols = h.shape
lat_c = (LAT_MIN + LAT_MAX) / 2
m_per_deg_lat = 111320.0
m_per_deg_lon = 111320.0 * np.cos(np.radians(lat_c))
width_m = (LON_MAX - LON_MIN) * m_per_deg_lon
depth_m = (LAT_MAX - LAT_MIN) * m_per_deg_lat

h.astype("<f4").tofile(f"{out}/heights.f32")
meta = dict(cols=cols, rows=rows, width=width_m, depth=depth_m,
            minH=float(h.min()), maxH=float(h.max()),
            bbox=[LAT_MIN, LON_MIN, LAT_MAX, LON_MAX])
json.dump(meta, open(f"{out}/terrain.json", "w"), indent=1)
print(meta)
