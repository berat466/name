import re, glob, statistics as st
def f(t, p): return float(re.search(p, t).group(1))
for cfg in ('full', 'hafif'):
    rows = []
    for fn in sorted(glob.glob(f'ham/orig/{cfg}_[0-9].txt')):
        t = open(fn).read()
        rd = f(t, r'ZAMAN\|veri dosyalarini okuma=([\d.]+)s'); oz = f(t, r'V31_ZINCIR_OZELLIK\|[^\n]*hazirlik=([\d.]+)s'); gr = f(t, r'V31_HAZIRLIK\|[^\n]*sure=([\d.]+)s'); sc = f(t, r'SURE\|lamC=85 Cholesky\(C blogu\)\+Schur hazirlik=([\d.]+)s')
        ws = float(re.search(r'V50_SECIM\|B yalniz son asama GERCEK[^\n]*agirlik cozme\(rho\)=([\d.]+)s', t).group(1)); qt = f(t, r'V50_SURE\|Q_havuz tablosu \(gercek\+karisik\)=([\d.]+)s')
        g234 = f(t, r'TT\|M2/3/4 grup-grup Cholesky\+Y0\+X \(V41\)\|([\d.]+)s'); tot = f(t, r'TOPLAM sure=([\d.]+)s'); ram = f(t, r'tepe_RAM=(\d+)MB')
        rows.append(dict(okuma=rd, ozellik=oz, gram=gr, schur=sc, w=ws, q=qt, model=rd + oz + gr + sc + ws + qt, g234=g234, toplam=tot, ram=ram))
    print(cfg, {k: round(st.median([r[k] for r in rows]), 2) for k in rows[0]}, 'model hazir tum:', [round(r['model'], 1) for r in rows])
