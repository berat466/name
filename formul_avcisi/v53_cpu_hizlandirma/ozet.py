# ham/olc/*.txt -> medyan tablolari (Markdown). Asamalar birbirini iki kez saymaz (ayrik bolum + ayri alt kalemler).
import re, glob, statistics as st, sys, json
def parse(f):
    t = open(f).read(); th = {}; tt = {}
    for m in re.finditer(r'^TH\|([^|]+)\|([\d.]+)', t, re.M): th[m.group(1)] = float(m.group(2))
    for m in re.finditer(r'^TT\|([^|]+)\|([\d.]+)', t, re.M): tt[m.group(1)] = float(m.group(2))
    mh = re.search(r'V53H_MODEL_HAZIR\|([\d.]+) s.*tepe RSS=(\d+)', t); rd = re.search(r'ZAMAN\|veri dosyalarini okuma=([\d.]+)s', t)
    sc = re.search(r'V53H_SONUC\|VAL CE=([\d.]+).*Top1=([\d.]+) Top5=([\d.]+)', t)
    return th, tt, float(mh.group(1)), int(mh.group(2)), float(rd.group(1)), sc.groups()
def g(d, k): return d.get(k, 0.0)
def tt_key(tt, pre): return sum(v for k, v in tt.items() if k.startswith(pre))
def stages(f, ratio_y0=None, orig=False):
    th, tt, model, rss, rd, sc = parse(f)
    A = g(th, 'm1 run_chain basi (program basindan)'); B = g(th, 'f2 V51 K-tablolari+kapi'); C = g(th, 'f0 hazirlik toplam (ozellik+K+gram F)') - B - g(th, 'f6 F^T F (AFF)') - g(th, 'f7 F^T Y (RF)')
    D = g(th, 'f6 F^T F (AFF)') + g(th, 'f7 F^T Y (RF)') + g(th, 'g1 v31solve basi->sayim oncesi (cat+AFF/RF+id+ayirma)') + g(th, 'g2 sayim hazirlik (grup/yoluk/tampon)') + g(th, 'g3 sayim (ACC,ACF,RC)')
    g45 = g(th, 'g45 grup cozum duvar suresi (Cholesky+Y0+X+Xf)') or g(th, 'g45 grup cozum duvar suresi')
    if orig:
        m3 = tt_key(tt, 'M3 '); m4 = tt_key(tt, 'M4 '); E = g45 + m3 + m4 + g(th, 'g8 T=ACF\'X (dgemm)') + g(th, 'g9 U=ACF\'Y0'); F = tt_key(tt, 'M6 ')
        y0conv = m3; y0tail = 0.0
    else:
        E = g45 + g(th, 'g8 T=ACF\'X (dgemm)') + g(th, 'g9 U=ACF\'Y0'); F = g(th, 'g10 Schur cozumu W_F (Cholesky 513 + V sag taraf)') + g(th, 'g11 W_C toplam'); y0conv = 0.0; y0tail = g(th, 'g11b Y0 ekleme/isaret')
    G = g(th, 'q1 Q_havuz tablosu')
    r = ratio_y0 if ratio_y0 is not None else (g(th, 'g5 Y0 cozum (CPU-toplam, tahsisli)') / max(1e-9, g(th, 'g4 grup Cholesky (CPU-toplam)') + g(th, 'g5 Y0 cozum (CPU-toplam, tahsisli)') + g(th, 'g6 X cozum (CPU-toplam, tahsisli)')))
    Y0s = g45 * r; Y0all = Y0s + y0conv + g(th, 'g9 U=ACF\'Y0') + y0tail
    ev = g(th, 'e1a val logit+okuma secimi') + g(th, 'e1b val Q-karisim alpha secimi+metrik')
    return dict(okuma=rd, onhazir=A, oofq=B, ozellik=C, gram=D, schur=E, y0solve=Y0s, y0all=Y0all, agirlik=F, qtab=G, model=model, diger=model - (A + B + C + D + E + F + G), valeval=ev, rss=rss, valce=sc[0], t1=sc[1], t5=sc[2])
def split_ratio(cfg):
    th, tt, *_ = parse(f'ham/olc/{cfg}_once_split.txt'); return g(th, 'g5 Y0 cozum (CPU-toplam)') / (g(th, 'g4 grup Cholesky (CPU-toplam)') + g(th, 'g5 Y0 cozum (CPU-toplam)') + g(th, 'g6 X cozum (CPU-toplam)'))
out = {}
names = [('okuma', 'Veri okuma'), ('onhazir', 'Program basi -> zincir (okuma+gomme genisletme+n-gram ogretici)'), ('oofq', 'OOF/Q (V51 K-tablolari + kapi)'), ('ozellik', 'Ozellik uretimi (F1, V39 havuz, V49)'), ('gram', 'Gram/RHS (F^TF, F^TY, C-blok sayimi, slot/tampon)'),
         ('schur', 'Schur/Cholesky (grup Cholesky + Y0 + X + T + U)'), ('y0solve', '  alt kalem: Y0 cozumu (duvar payi)'), ('y0all', '  alt kalem: Y0 ile ilgili toplam (cozum + float donusum/U + ekleme)'), ('agirlik', 'Saf agirlik uretimi (W_F cozumu + W_C)'), ('qtab', 'Q_havuz tablosu'), ('diger', 'Diger/olculmeyen'), ('model', 'TAM MODEL HAZIR'), ('valeval', 'Validation degerlendirmesi (ayri)'), ('rss', 'Tepe RAM (MB)')]
md = []
for cfg in ('full', 'hafif'):
    ro = split_ratio(cfg); modes = {'once': ('ONCE (orijinal aritmetik, yogun Y0)', True), 'sonra': ('SONRA (hizli, tam-esdeger)', False), 'sonraU1': ('SONRA-U1 (hizli + seyrek U)', False)}
    res = {}
    for k, (lab, orig) in modes.items():
        fs = sorted(glob.glob(f'ham/olc/{cfg}_{k}_[0-9]*.txt')); rows = [stages(f, ratio_y0=(ro if orig else None), orig=orig) for f in fs]
        res[k] = {s: st.median([r[s] for r in rows]) for s in rows[0] if s not in ('valce', 't1', 't5')}; res[k]['n'] = len(rows); res[k]['valce'] = rows[0]['valce']; res[k]['min'] = min(r['model'] for r in rows); res[k]['max'] = max(r['model'] for r in rows)
    out[cfg] = res
    md.append(f'\n### {cfg} (n={res["once"]["n"]} tur, medyan; model hazir min-maks: once {res["once"]["min"]:.2f}-{res["once"]["max"]:.2f}, sonra {res["sonra"]["min"]:.2f}-{res["sonra"]["max"]:.2f}, sonra-U1 {res["sonraU1"]["min"]:.2f}-{res["sonraU1"]["max"]:.2f})\n')
    md.append('| Asama | ONCE | SONRA | SONRA-U1 |\n|---|---|---|---|')
    for s, lab in names:
        fmt = (lambda v: f'{v:.0f}') if s == 'rss' else (lambda v: f'{v:.3f}')
        md.append(f'| {lab} | {fmt(res["once"][s])} | {fmt(res["sonra"][s])} | {fmt(res["sonraU1"][s])} |')
    md.append(f'| Validation CE | {res["once"]["valce"]} | {res["sonra"]["valce"]} | {res["sonraU1"]["valce"]} |')
open('ham/olc_ozet.md', 'w').write('\n'.join(md)); json.dump(out, open('ham/olc_ozet.json', 'w'), indent=1); print('\n'.join(md))
