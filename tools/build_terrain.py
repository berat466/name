"""SRTM .hgt dosyalarından istenen bölgeyi kesip oyun için ikili yükseklik verisi üretir.
Kullanım: python3 tools/build_terrain.py <hgt_klasoru> <cikis_klasoru> <lat_min> <lon_min> <lat_max> <lon_max>
Bölgeyi kaplayan tüm karolar (örn. N35E128.hgt) klasörde bulunmalı.
"""
import sys, json, math, numpy as np

N = 3601  # SRTM1: 1 yay saniyesi
src, out = sys.argv[1], sys.argv[2]
LAT_MIN, LON_MIN, LAT_MAX, LON_MAX = map(float, sys.argv[3:7])

la0, la1 = math.floor(LAT_MIN), math.floor(LAT_MAX - 1e-9)
lo0, lo1 = math.floor(LON_MIN), math.floor(LON_MAX - 1e-9)
tile = lambda la, lo: f"{'N' if la >= 0 else 'S'}{abs(la):02d}{'E' if lo >= 0 else 'W'}{abs(lo):03d}.hgt"
rows_ = []
for la in range(la1, la0 - 1, -1):                # kuzeyden güneye
    cols_ = []
    for lo in range(lo0, lo1 + 1):                # batıdan doğuya
        t = np.fromfile(f"{src}/{tile(la, lo)}", dtype=">i2").reshape(N, N).astype(np.float32)
        cols_.append(t if not cols_ else t[:, 1:])  # ortak kenar sütunu bir kez
    row = np.concatenate(cols_, axis=1)
    rows_.append(row if not rows_ else row[1:, :])  # ortak kenar satırı bir kez
full = np.concatenate(rows_, axis=0)

r0 = round((la1 + 1 - LAT_MAX) * 3600); r1 = round((la1 + 1 - LAT_MIN) * 3600)
c0 = round((LON_MIN - lo0) * 3600); c1 = round((LON_MAX - lo0) * 3600)
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
