# V53/V54 model uretimi: asamalarin TEORIK aritmetik is yuku (FLOP = 2*madd; boyutlardan hesaplanan TAHMIN, sayac degil) / olculen sure -> ulasilan GF/s
import re, sys, glob, statistics as st
cfg = sys.argv[1]; logs = sorted(glob.glob(sys.argv[2]))
n, nva, nte, K, H, m, V = 113046, 4980, 4980, 192, 512, 513, 2001
t = open(logs[0]).read()
mc = int(re.search(r'C blogu=(\d+) seyrek', t).group(1)); g = re.search(r'sum sg\^2=([\d.e+]+) ; sum sg\^3/3=([\d.e+]+)', t); s2, s3 = float(g.group(1)), float(g.group(2))
y = re.search(r'yapisal sifir-olmayan \(grup,y\) konumlari = ([\d.e+]+) eleman .*yogun ([\d.e+]+) -> yoluk ([\d.e+]+) madd', t); nz, y0dense_madd, y0sp_madd = float(y.group(1)), float(y.group(2)), float(y.group(3))
G = 1e9
fl = {  # faz adi -> (aciklama, FLOP, tur)  tur: f32 / f64 / bellek
 'onhazir+okuma+F1': ('F1 gemm (n+va+te) x192x512 + tanh ~59M', 2 * (n + nva + nte) * K * H, 'f32'),
 'OOF/Q: K-tablolari+kapi': ('6 x sgemm 2001^3 + proj gemm + Sf gemm + kapi/top-k/qtab (FLOP yalniz gemm)', 6 * 2 * V ** 3 + 2 * (n + nva + nte) * 64 * H + 6 * 2 * V * V * 64, 'f32'),
 'V39 havuz': ('ortalama+karisim (bellek bagimli) ~4 flop/eleman', 4 * (n + nva + nte) * H, 'bellek'),
 'V49 donusumu': ('istatistik+turetilmis sutunlar ~12 flop/eleman', 12 * (n + nva + nte) * H, 'bellek'),
 'Gram: F^T F': ('n x 513^2 sgemm', 2 * n * m * m, 'f32'),
 'Gram: F^T Y': ('n x 513 skaler ekleme', n * m, 'bellek'),
 'Gram: slot sayimi (ACC,ACF,RC)': ('n x 2 slot x 513 ekleme', 2 * n * m, 'bellek'),
 'grup Cholesky + Y0 + X': ('Cholesky sum sg^3/3 + Y0 (yoluk) + X (513 sutun)', 2 * (s3 + y0sp_madd + 513 * s2), 'f64'),
 "T=ACF'X + U=ACF'Y0": ('T dgemm 513x513xmc + U (seyrek-tam-esdeger: nz x 513)', 2 * m * m * mc + 2 * nz * m, 'f64'),
 'Schur: W_F': ('Cholesky 513 + 513^2 x 2001 cozum', m ** 3 / 3 * 2 + 2 * m * m * V, 'f64'),
 'W_C = Y0 - X W_F': ('sgemm mc x 513 x 2001', 2 * mc * m * V, 'f32'),
 'Q_havuz tablosu': ('~10 komsu x V x V', 2 * 10 * V * V, 'bellek'),
}
dense_extra = 2 * (y0dense_madd - y0sp_madd) + (2 * m * V * mc - 2 * nz * m)   # V53 oncesi yogun Y0 cozumu + yogun U dgemm
PK = {'f32': 546.0, 'f64': 232.0}
rows = {}
for L in logs:
    for mm in re.finditer(r'^FAZ\|([^|]+)\|([\d.]+)\|([\d.]+)\|([\d.]+)', open(L).read(), re.M): rows.setdefault(mm.group(1), []).append((float(mm.group(2)), float(mm.group(3))))
print(f'### {cfg} (mc={mc}; {len(logs)} kosu medyan)\n\n| Faz | Is | TEORIK FLOP (GF) | Sure (s) | CPU-s | Cekirdek kullanimi | Ulasilan GF/s | Tepe ile orani |\n|---|---|---|---|---|---|---|---|')
tot_f = tot_t = 0
for ph, (ac, f, tur) in fl.items():
    if ph not in rows: continue
    w = st.median(x[0] for x in rows[ph]); c = st.median(x[1] for x in rows[ph]); gf = f / G; ach = gf / w
    ratio = f'%{100 * ach / PK[tur]:.0f} ({tur} tepe {PK[tur]:.0f})' if tur != 'bellek' else '(bellek/ozel fonksiyon)'
    print(f'| {ph} | {ac} | {gf:.2f} | {w:.3f} | {c:.2f} | {c / w:.2f} | {ach:.1f} | {ratio} |'); tot_f += f; tot_t += w
print(f'| **Toplam** | | **{tot_f / G:.1f}** | **{tot_t:.2f}** | | | **{tot_f / G / tot_t:.1f}** | sgemm tepe 546 GF/s |')
print(f'\nV53 oncesi (yogun Y0 cozumu + yogun U dgemm) ek is yuku: +{dense_extra / G:.1f} GF (yogun/seyrek FLOP orani Y0: {y0dense_madd / y0sp_madd:.1f}x, U: {2 * m * V * mc / (2 * nz * m):.1f}x)')
