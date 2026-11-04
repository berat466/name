# iki dump dizini arasinda dizi-dizi karsilastirma: bit-ozdeslik, maks mutlak/bagil fark, farkli eleman sayisi
import sys, numpy as np, os
A, B = sys.argv[1], sys.argv[2]
dt = {'Fc': np.float32, 'AFF': np.float64, 'RF': np.float64, 'ACCB': np.float64, 'ACF': np.float64, 'RC': np.float64, 'Y0f': np.float32, 'X': np.float64, 'T': np.float64, 'U': np.float64, 'WC': np.float32, 'WF': np.float32, 'Zval': np.float32}
print('%-5s %-9s %-14s %-14s %-14s' % ('dizi', 'bit-ozdes', 'maks mutlak', 'maks bagil', 'farkli eleman'))
for k, t in dt.items():
    fa, fb = os.path.join(A, k + '.bin'), os.path.join(B, k + '.bin')
    if not (os.path.exists(fa) and os.path.exists(fb)): continue
    a = np.memmap(fa, dtype=t, mode='r'); b = np.memmap(fb, dtype=t, mode='r')
    if a.shape != b.shape: print(k, 'BOYUT FARKLI'); continue
    ndiff = 0; mx = 0.0; mr = 0.0; n = a.size; step = 1 << 24
    for s in range(0, n, step):
        x = np.asarray(a[s:s + step]); y = np.asarray(b[s:s + step]); m = x != y
        c = int(m.sum()); ndiff += c
        if c:
            d = np.abs(x[m].astype(np.float64) - y[m].astype(np.float64)); mx = max(mx, float(d.max()))
            den = np.maximum(np.abs(x[m].astype(np.float64)), np.abs(y[m].astype(np.float64))); mr = max(mr, float((d / den).max()))
    print('%-5s %-9s %-14.3e %-14.3e %d / %d (%.3g%%)' % (k, 'EVET' if ndiff == 0 else 'hayir', mx, mr, ndiff, n, 100.0 * ndiff / n))
