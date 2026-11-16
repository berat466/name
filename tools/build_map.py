"""Harita üretimi: yükseklik (GeoTIFF, UTM 52N) + ham Overpass JSON  ->  oyun verisi.

Kullanım:
  python3 tools/build_map.py <dem.tif> <overpass.json.gz> <cikis_klasoru>

Üretilenler:
  terrain.json   : ızgara boyutu, ölçek, koordinat dönüşümü
  heights.u16    : doğal arazi (yola göre DEĞİŞTİRİLMEZ; kesim-dolgu oyunda ayrı katman olarak uygulanır)
  water.png      : su maskesi (deniz, nehir, göl)
  urban.png      : yerleşim yoğunluğu (zemin dokusu için)
  roads.json/.bin: yol ağı + her nokta için çözülmüş yol kotu (boyuna profil)

Yol kotu tüm ağ üzerinde tek bir denklem sistemiyle çözülür:
  - yol doğal araziyi izler ama sınıfına göre yumuşatılır (otoyol uzun, sokak kısa ölçek)
  - kavşakta buluşan yollar aynı düğümü paylaştığı için aynı kotta buluşur
  - köprü açıklıkları serbesttir (iki uç arasında düz), altından geçen yola 5,5 m boşluk bırakır
  - eğim sınırı aşılan yerlerde yol daha fazla yumuşatılır (daha çok kesim-dolgu)
"""
import sys, json, gzip, math
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage, sparse
from scipy.sparse.linalg import cg

Image.MAX_IMAGE_PIXELS = None
tif_path, osm_path, out = sys.argv[1:4]

# ----------------------------------------------------------------------------- koordinat
def utm_fwd(lon, lat, lon0=129.0):
    """WGS84 -> UTM (Krüger serisi), zone 52N için lon0 = 129."""
    a, f, k0 = 6378137.0, 1 / 298.257223563, 0.9996
    n = f / (2 - f)
    A = a / (1 + n) * (1 + n * n / 4 + n ** 4 / 64)
    al = [0, n / 2 - 2 * n * n / 3 + 5 * n ** 3 / 16, 13 * n * n / 48 - 3 * n ** 3 / 5, 61 * n ** 3 / 240]
    phi, lam = np.radians(lat), np.radians(np.asarray(lon) - lon0)
    e2 = 2 * math.sqrt(n) / (1 + n)
    t = np.sinh(np.arctanh(np.sin(phi)) - e2 * np.arctanh(e2 * np.sin(phi)))
    xi, eta = np.arctan2(t, np.cos(lam)), np.arctanh(np.sin(lam) / np.sqrt(1 + t * t))
    E = 500000 + k0 * A * (eta + sum(al[j] * np.cos(2 * j * xi) * np.sinh(2 * j * eta) for j in (1, 2, 3)))
    N = k0 * A * (xi + sum(al[j] * np.sin(2 * j * xi) * np.cosh(2 * j * eta) for j in (1, 2, 3)))
    return E, N

im = Image.open(tif_path)
dsm = np.array(im).astype(np.float32)
tie, scl = im.tag_v2[33922], im.tag_v2[33550]
PX = float(scl[0]); E0, NTOP = float(tie[3]), float(tie[4])
RS, CS = dsm.shape
WIDTH, DEPTH = (CS - 1) * PX, (RS - 1) * PX          # piksel merkezleri arası
EC, NC = E0 + PX / 2 + WIDTH / 2, NTOP - PX / 2 - DEPTH / 2

def ll2w(lon, lat):
    E, N = utm_fwd(lon, lat)
    return E - EC, NC - N                              # x doğu, z güney

def w2px(x, z):                                        # dünya -> DEM piksel (kesirli sütun, satır)
    return (x + WIDTH / 2) / PX, (z + DEPTH / 2) / PX

# ----------------------------------------------------------------------------- OSM
els = json.load(gzip.open(osm_path))["elements"]
ways = [e for e in els if e["type"] == "way" and "geometry" in e]
print("OSM yol/su objesi:", len(ways))

# sınıf -> (şerit, şerit genişliği, kaldırım, sol omuz, sağ omuz, yumuşatma ölçeği m, en fazla eğim)
CLASS = {
    "motorway":      (3, 3.60, 0.0, 1.00, 2.5, 150, 0.05),
    "trunk":         (3, 3.50, 0.0, 0.75, 1.5, 120, 0.06),
    "primary":       (4, 3.40, 3.5, 0.40, 0.4,  80, 0.08),
    "secondary":     (4, 3.30, 3.0, 0.40, 0.4,  70, 0.10),
    "tertiary":      (2, 3.25, 2.5, 0.30, 0.3,  50, 0.12),
    "unclassified":  (2, 3.00, 1.5, 0.25, 0.25, 35, 0.15),
    "residential":   (2, 2.75, 1.5, 0.25, 0.25, 30, 0.17),
    "living_street": (1, 4.00, 0.0, 0.20, 0.2,  25, 0.20),
    "service":       (1, 4.00, 0.0, 0.20, 0.2,  20, 0.20),
}
CLS = list(CLASS)
SKIP_SERVICE = {"parking_aisle", "driveway", "drive-through", "emergency_access", "driving_learning", "운전연습로"}
STEP = 12.0                                            # profil örnekleme aralığı (m)

def to_int(v, d):
    try: return int(str(v).split(";")[0])
    except ValueError: return d

roads = []
for e in ways:
    t = e.get("tags", {})
    hw = t.get("highway", "")
    base = hw.replace("_link", "")
    if base not in CLASS or t.get("area") == "yes": continue
    if base == "service" and t.get("service") in SKIP_SERVICE: continue
    link = hw.endswith("_link")
    lanes0, lw, walk, sl, sr, ell, gmax = CLASS[base]
    tunnel = t.get("tunnel") in ("yes", "avalanche_protector")
    bridge = t.get("bridge") not in (None, "no") and not tunnel
    oneway = t.get("oneway") in ("yes", "1", "-1") or base == "motorway" or link or t.get("junction") == "roundabout"
    lanes = to_int(t.get("lanes"), 1 if link else (lanes0 if base in ("motorway", "trunk") else max(1, lanes0 // 2) if oneway else lanes0))
    lanes = max(1, min(lanes, 8))
    if link: sl, sr, walk = min(sl, 0.4), min(sr, 1.0), 0.0 if base in ("motorway", "trunk") else walk
    if t.get("sidewalk") in ("no", "none") or bridge and base in ("motorway", "trunk"): walk = 0.0
    layer = to_int(t.get("layer"), 1 if bridge else -1 if tunnel else 0)
    lon = np.array([g["lon"] for g in e["geometry"]]); lat = np.array([g["lat"] for g in e["geometry"]])
    x, z = ll2w(lon, lat)
    if len(x) < 2: continue
    if x.max() < -WIDTH / 2 or x.min() > WIDTH / 2 or z.max() < -DEPTH / 2 or z.min() > DEPTH / 2: continue
    roads.append(dict(c=CLS.index(base), n=lanes, o=int(oneway), lw=lw, sl=sl, sr=sr, w=round(lanes * lw + sl + sr, 2), s=walk,
                      b=bridge, t=tunnel, layer=layer, ell=ell, gmax=gmax, nodes=e["nodes"], x=x, z=z))
print("çizilecek yol:", len(roads))

# ----------------------------------------------------------------------------- arazi (DSM -> zemin)
# Copernicus su yüzeylerini tam düz verir: 3x3 komşuluğu birebir eşit olan büyük bölgeler sudur.
flat = (ndimage.maximum_filter(dsm, 3) - ndimage.minimum_filter(dsm, 3)) == 0
lab, nlab = ndimage.label(flat)
sizes = ndimage.sum(flat, lab, range(1, nlab + 1))
water_dem = np.isin(lab, 1 + np.nonzero(sizes >= 40)[0])
water_dem = ndimage.binary_dilation(water_dem)           # düzlük testi kıyıda 1 piksel içeride kalır
sea = dsm <= 0

# yerleşim yoğunluğu: yol uzunluğu / piksel
dens = np.zeros_like(dsm)
for r in roads:
    if r["t"] or r["b"]: continue
    c, rr = w2px(r["x"], r["z"])
    ok = (c >= 0) & (c < CS - 1) & (rr >= 0) & (rr < RS - 1)
    np.add.at(dens, (rr[ok].astype(int), c[ok].astype(int)), 1.0)
urban = np.clip(ndimage.gaussian_filter(dens, 5) / 0.12, 0, 1)
urban = urban * urban * (3 - 2 * urban)

# DSM bina/ağaç yüksekliği içerir. Yerleşim içinde morfolojik açma (7 px ~ 210 m) binaları siler;
# yerleşim dışında yalnızca hafif yumuşatma (doğal topoğrafya korunur).
opened = ndimage.gaussian_filter(ndimage.grey_opening(dsm, size=7), 1.2)
light = ndimage.gaussian_filter(dsm, 0.7)
dtm = light * (1 - urban) + opened * urban
dtm[sea] = 0.0
dtm = np.maximum(dtm, 0.0)

GN = 2049                                                 # oyun ızgarası (2048 aralık = 32 x 64'lük parça)
gi = np.linspace(0, CS - 1, GN)
gy, gx = np.meshgrid(np.linspace(0, RS - 1, GN), gi, indexing="ij")
H = ndimage.map_coordinates(dtm, [gy, gx], order=1).astype(np.float32)
HSCALE = 0.02
(H / HSCALE).round().astype("<u2").tofile(f"{out}/heights.u16")
print("arazi:", H.shape, "en yüksek %.0f m" % H.max())

def nat(x, z):                                            # doğal zemin (çift doğrusal)
    c, r = w2px(np.asarray(x), np.asarray(z))
    return ndimage.map_coordinates(dtm, [r, c], order=1, mode="nearest")

# ----------------------------------------------------------------------------- su maskesi
WM = 4096
wm = Image.new("L", (WM, WM), 0)
dr = ImageDraw.Draw(wm)
sx, sz = WM / WIDTH, WM / DEPTH
RIVER_W = {"river": 30, "canal": 14}
for e in ways:
    t = e.get("tags", {})
    if "highway" in t: continue
    lon = np.array([g["lon"] for g in e["geometry"]]); lat = np.array([g["lat"] for g in e["geometry"]])
    x, z = ll2w(lon, lat)
    pts = list(zip(((x + WIDTH / 2) * sx).tolist(), ((z + DEPTH / 2) * sz).tolist()))
    closed = e["nodes"][0] == e["nodes"][-1] and len(pts) >= 4
    if closed and (t.get("natural") == "water" or t.get("waterway") in ("riverbank", "dock") or t.get("landuse") in ("reservoir", "basin")):
        dr.polygon(pts, fill=255)
    elif t.get("waterway") in RIVER_W:
        dr.line(pts, fill=255, width=max(1, round(RIVER_W[t["waterway"]] * sx)))
wosm = np.array(wm, np.float32) / 255
wdem = ndimage.zoom((water_dem | sea).astype(np.float32), WM / CS, order=1)[:WM, :WM]
wmask = ndimage.gaussian_filter(np.maximum(wosm, wdem), 1.0)
Image.fromarray((wmask * 255).astype(np.uint8)).save(f"{out}/water.png", optimize=True)
Image.fromarray((ndimage.zoom(urban, 512 / CS, order=1) * 255).astype(np.uint8)).save(f"{out}/urban.png", optimize=True)

# ----------------------------------------------------------------------------- yol ağı: örnekleme
node_ix = {}                                              # OSM düğümü -> bilinmeyen indeksi
PX_, PZ_ = [], []                                         # bilinmeyenlerin konumu
def new_pt(x, z):
    PX_.append(float(x)); PZ_.append(float(z)); return len(PX_) - 1

for r in roads:
    idx = []
    for k, nid in enumerate(r["nodes"]):
        if nid not in node_ix: node_ix[nid] = new_pt(r["x"][k], r["z"][k])
        i = node_ix[nid]
        if k:
            ax, az, bx, bz = r["x"][k - 1], r["z"][k - 1], r["x"][k], r["z"][k]
            n = max(1, math.ceil(math.hypot(bx - ax, bz - az) / STEP))
            for s in range(1, n): idx.append(new_pt(ax + (bx - ax) * s / n, az + (bz - az) * s / n))
        idx.append(i)
    r["idx"] = np.array(idx)
P = np.stack([np.array(PX_), np.array(PZ_)], 1)
NP_ = len(P)
print("profil noktası:", NP_)
natP = nat(P[:, 0], P[:, 1])

# kenarlar (ardışık örnekler)
EI = np.concatenate([r["idx"][:-1] for r in roads]); EJ = np.concatenate([r["idx"][1:] for r in roads])
ER = np.concatenate([np.full(len(r["idx"]) - 1, k) for k, r in enumerate(roads)])
EL = np.maximum(np.hypot(*(P[EI] - P[EJ]).T), 0.05)
ell = np.array([r["ell"] for r in roads])[ER]; gmax = np.array([r["gmax"] for r in roads])[ER]
free = np.array([r["b"] or r["t"] for r in roads])[ER]    # köprü / tünel kenarı: araziye bağlı değil
isb = np.array([r["b"] for r in roads])[ER]

# veri ağırlığı: zemin yolu kenarlarının uzunluğu düğümlere paylaştırılır
mass = np.zeros(NP_)
np.add.at(mass, EI, np.where(free, 0, EL / 2)); np.add.at(mass, EJ, np.where(free, 0, EL / 2))
mass = np.maximum(mass, 1e-3)

# ----------------------------------------------------------------------------- kesişmeler (köprü / alt-üst geçit)
layer_r = np.array([r["layer"] for r in roads])
CELL = 60.0
def cell_ids(i, j):
    mx, mz = (P[i, 0] + P[j, 0]) / 2, (P[i, 1] + P[j, 1]) / 2
    return np.floor(mx / CELL).astype(np.int64), np.floor(mz / CELL).astype(np.int64)
cx_, cz_ = cell_ids(EI, EJ)
grid = {}
for k, key in enumerate(zip(cx_.tolist(), cz_.tolist())): grid.setdefault(key, []).append(k)

def seg_x(p, p2, q, q2):
    d1, d2 = p2 - p, q2 - q
    den = d1[0] * d2[1] - d1[1] * d2[0]
    if abs(den) < 1e-9: return None
    w = q - p
    t = (w[0] * d2[1] - w[1] * d2[0]) / den; s = (w[0] * d1[1] - w[1] * d1[0]) / den
    return (t, s) if 0 <= t <= 1 and 0 <= s <= 1 else None

crossings = []                                            # (üst i1, i2, t, alt j1, j2, s)
special = np.nonzero(layer_r[ER] != 0)[0]
for k in special.tolist():
    for dx in (-1, 0, 1):
        for dz in (-1, 0, 1):
            for m in grid.get((cx_[k] + dx, cz_[k] + dz), ()):
                la, lb = layer_r[ER[k]], layer_r[ER[m]]
                if la <= lb: continue                     # yalnızca "üstteki" taraftan bak
                a1, a2, b1, b2 = EI[k], EJ[k], EI[m], EJ[m]
                if len({a1, a2, b1, b2}) < 4: continue    # ortak düğüm: hemzemin bağlantı
                ts = seg_x(P[a1], P[a2], P[b1], P[b2])
                if ts: crossings.append((a1, a2, ts[0], b1, b2, ts[1]))
print("alt-üst geçit kesişmesi:", len(crossings))
CLEAR = 5.5

# ----------------------------------------------------------------------------- çözüm
def solve(stiff_mul, active_x, lift_pts, y0):
    k = ell * ell / EL * stiff_mul
    rows = np.concatenate([EI, EJ, EI, EJ]); cols = np.concatenate([EI, EJ, EJ, EI])
    vals = np.concatenate([k, k, -k, -k])
    diag = mass.copy(); b = mass * natP
    if len(lift_pts):                                     # araziye gömülen köprü noktaları: zeminin üstüne
        diag[lift_pts] += 40.0; b[lift_pts] += 40.0 * (natP[lift_pts] + 1.8)
    A = sparse.coo_matrix((vals, (rows, cols)), shape=(NP_, NP_)).tocsr() + sparse.diags(diag)
    if len(active_x):
        K = 60.0
        r_, c_, v_ = [], [], []
        for (a1, a2, t, b1, b2, s) in active_x:
            ids = [a1, a2, b1, b2]; co = [1 - t, t, -(1 - s), -s]
            for p in range(4):
                b[ids[p]] += K * CLEAR * co[p]
                for q in range(4): r_.append(ids[p]); c_.append(ids[q]); v_.append(K * co[p] * co[q])
        A = A + sparse.coo_matrix((v_, (r_, c_)), shape=(NP_, NP_)).tocsr()
    M = sparse.diags(1.0 / A.diagonal())
    y, info = cg(A, b, x0=y0, rtol=1e-7, maxiter=6000, M=M)
    return y

stiff = np.ones(len(EI))
bridge_pts = np.unique(np.concatenate([EI[isb], EJ[isb]]))
# köprünün zemin yoluna bağlandığı uçlar (zemin kenarı da olan düğümler) kaldırılmaz
ground_touch = np.zeros(NP_, bool); ground_touch[EI[~free]] = True; ground_touch[EJ[~free]] = True
y = natP.copy(); active, lift = [], np.array([], int)
for it in range(5):
    y = solve(stiff, active, lift, y)
    def at(c): return (1 - c[2]) * y[c[0]] + c[2] * y[c[1]] - ((1 - c[5]) * y[c[3]] + c[5] * y[c[4]])
    was = set(active)
    active = [c for c in crossings if c in was or at(c) < CLEAR]
    low = bridge_pts[(y[bridge_pts] < natP[bridge_pts] + 1.0) & ~ground_touch[bridge_pts]]
    lift = np.union1d(lift, low)
    g = np.abs(y[EI] - y[EJ]) / EL
    over = g > gmax
    # en fazla 16 kat (yumuşatma ölçeği 4 kat): dağ yolunu aşırı düzleştirip dev yarma açmasın
    stiff = np.where(over, np.minimum(stiff * np.clip((g / gmax) ** 2, 1.5, 4), 16), stiff)
    print(f"  tur {it}: eğim sınırını aşan kenar %{100 * over.mean():.2f}, etkin kesişme {len(active)}, kaldırılan köprü noktası {len(lift)}")
y = solve(stiff, active, lift, y)
g = np.abs(y[EI] - y[EJ]) / EL
print("son: eğim sınırını aşan kenar %%%.2f, en büyük eğim %%%.0f" % (100 * (g > gmax * 1.05).mean(), 100 * g.max()))

# ----------------------------------------------------------------------------- çıktı
# uç bayrakları: zemin yolunun ucu köprü/tünele bağlanıyorsa arazi düzeltmesi o uçta düz kesilir
deg_free = np.zeros(NP_, int)
for r in roads:
    if r["b"] or r["t"]: deg_free[r["idx"][[0, -1]]] += 1
# gerçek kavşaklar (3+ yol ucu ya da bir yolun ortasından geçen başka yol)
ends = np.zeros(NP_, int); mids = np.zeros(NP_, int); hwmax = np.zeros(NP_)
for r in roads:
    if r["t"]: continue
    on = np.array([node_ix[n] for n in r["nodes"]])
    np.add.at(ends, on[[0, -1]], 1); np.add.at(mids, on[1:-1], 1)
    np.maximum.at(hwmax, on, r["w"] / 2)
jn = np.nonzero((ends >= 3) | ((mids >= 1) & (ends + mids >= 2)))[0]

rows, XS, ZS, YS = [], [], [], []
osm_nodes = set(node_ix.values())
for r in roads:
    idx = r["idx"]
    s = np.concatenate([[0], np.cumsum(np.hypot(*(P[idx[1:]] - P[idx[:-1]]).T))])
    yy = y[idx]
    keep = np.array([i in osm_nodes for i in idx.tolist()])
    keep[[0, -1]] = True
    # ara noktalardan profili bozanları geri ekle (Douglas-Peucker benzeri, yinelemeli)
    while True:
        ki = np.nonzero(keep)[0]
        err = np.abs(np.interp(s, s[ki], yy[ki]) - yy)
        worst = int(err.argmax())
        if err[worst] < 0.08: break
        keep[worst] = True
    ki = np.nonzero(keep)[0]
    flags = (1 if r["b"] else 0) | (2 if r["t"] else 0)
    if not (r["b"] or r["t"]):
        flags |= (4 if deg_free[idx[0]] else 0) | (8 if deg_free[idx[-1]] else 0)
    rows.append([r["c"], r["n"], r["o"], r["lw"], r["sl"], r["sr"], r["w"], r["s"], flags, len(ki)])
    XS.append(P[idx[ki], 0]); ZS.append(P[idx[ki], 1]); YS.append(yy[ki])
X = np.concatenate(XS).astype("<f4"); Z = np.concatenate(ZS).astype("<f4"); Y = np.concatenate(YS).astype("<f4")
with open(f"{out}/roads.bin", "wb") as f:
    f.write(X.tobytes()); f.write(Z.tobytes()); f.write(Y.tobytes())
junc = np.stack([P[jn, 0], P[jn, 1], hwmax[jn]], 1).round(2).ravel().tolist()
json.dump({"cls": CLS, "points": int(len(X)), "roads": rows, "junc": junc}, open(f"{out}/roads.json", "w"), separators=(",", ":"))
print("yol:", len(rows), "nokta:", len(X), "kavşak:", len(jn))

# koordinat dönüşümü (enlem/boylam <-> dünya): 60 km'de doğrusal uyum birkaç metre hassasiyetinde
lo, la = np.meshgrid(np.linspace(128.72, 129.42, 9), np.linspace(34.9, 35.46, 9))
wx, wz = ll2w(lo.ravel(), la.ravel())
A1 = np.stack([np.ones(81), lo.ravel(), la.ravel()], 1); A2 = np.stack([np.ones(81), wx, wz], 1)
fit = lambda A, b: np.linalg.lstsq(A, b, rcond=None)[0].tolist()
meta = dict(cols=GN, rows=GN, width=WIDTH, depth=DEPTH, hscale=HSCALE, maxH=float(H.max()),
            ll2w=[fit(A1, wx), fit(A1, wz)], w2ll=[fit(A2, lo.ravel()), fit(A2, la.ravel())],
            start=[35.1532, 129.1187], name="Busan",
            attribution="Yükseklik: Copernicus DEM GLO-30 © DLR e.V. ve © Airbus Defence and Space GmbH (Copernicus, AB ve ESA). "
                        "Yol ve su: © OpenStreetMap katkıda bulunanlar (ODbL).")
json.dump(meta, open(f"{out}/terrain.json", "w"), indent=1)
print("tamam")
