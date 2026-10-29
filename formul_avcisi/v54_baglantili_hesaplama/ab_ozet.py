import glob,re,statistics as st,json
def parse(f):
    t=open(f).read(); x=dict(re.findall(r'(\w+) ([\d.]+)',open(f[:-4]+'.time').read())); th={m.group(1):float(m.group(2)) for m in re.finditer(r'^TH\|([^|]+)\|([\d.]+)',t,re.M)}
    mh=float(re.search(r'V53H_MODEL_HAZIR\|([\d.]+)',t).group(1)); ev=th.get('e1a val logit+okuma secimi',0)+th.get('e1b val Q-karisim alpha secimi+metrik',0)
    g=lambda k: th.get(k,0.0); y0=g('g45 grup cozum duvar suresi (Cholesky+Y0+X+Xf)')
    return dict(model=mh,wall=float(x['WALL']),cpu=float(x['USER'])+float(x['SYS']),rss=int(x['MAXRSS_KB'])/1024,sys=float(x['SYS']),
        oofq=g('f2 V51 K-tablolari+kapi'),feat=g('f0 hazirlik toplam (ozellik+K+gram F)')-g('f2 V51 K-tablolari+kapi')-g('f6 F^T F (AFF)')-g('f7 F^T Y (RF)'),
        gram=g('f6 F^T F (AFF)')+g('f7 F^T Y (RF)')+g('g1 v31solve basi->sayim oncesi (cat+AFF/RF+id+ayirma)')+g('g2 sayim hazirlik (grup/yoluk/tampon)')+g('g3 sayim (ACC,ACF,RC)'),
        schur=y0+g("g8 T=ACF'X (dgemm)")+g("g9 U=ACF'Y0"), grup=y0, tu=g("g8 T=ACF'X (dgemm)")+g("g9 U=ACF'Y0"), agirlik=g('g10 Schur cozumu W_F (Cholesky 513 + V sag taraf)')+g('g11 W_C toplam'), q=g('q1 Q_havuz tablosu'), pre=g('m1 run_chain basi (program basindan)'), valeval=ev)
out={}
for cfg in ('full','hafif'):
    out[cfg]={}
    for v in ('v53','v54'):
        rows=[parse(f) for f in sorted(glob.glob(f'ham/ab54/{cfg}_{v}_[0-9].txt'))]
        out[cfg][v]={k:st.median(r[k] for r in rows) for k in rows[0]}; out[cfg][v]['n']=len(rows); out[cfg][v]['min']=min(r['model'] for r in rows); out[cfg][v]['max']=max(r['model'] for r in rows)
json.dump(out,open('ham/ab54_ozet.json','w'),indent=1)
lab=[('model','TAM MODEL HAZIR (s)'),('pre','Program basi -> zincir (okuma+on hazirlik)'),('oofq','OOF/Q (K-tablolari + kapi)'),('feat','Ozellik uretimi (F1, V39, V49)'),('gram','Gram/RHS (F^TF, F^TY, slot sayimi)'),('grup','Grup Cholesky + Y0 + X'),('tu','T = ACF^T X + U = ACF^T Y0'),('agirlik','Saf agirlik uretimi (W_F + W_C)'),('q','Q_havuz tablosu'),('valeval','Validation degerlendirmesi (ayri)'),('wall','Tum surec duvar suresi'),('cpu','CPU suresi (user+sys, s)'),('sys','  sys (sayfa hatasi vb.)'),('rss','Tepe RAM (MB)')]
for cfg in ('full','hafif'):
    a,b=out[cfg]['v53'],out[cfg]['v54']; print(f'\n### {cfg} (n={a["n"]} tur medyan; model hazir min-maks: V53 {a["min"]:.2f}-{a["max"]:.2f}, V54 {b["min"]:.2f}-{b["max"]:.2f})\n\n| Olcum | V53 (897727b) | V54 | Degisim |\n|---|---|---|---|')
    for k,l in lab: print(f'| {l} | {a[k]:.3f} | {b[k]:.3f} | {100*(b[k]/a[k]-1):+.0f}% |' if k!='rss' else f'| {l} | {a[k]:.0f} | {b[k]:.0f} | {100*(b[k]/a[k]-1):+.0f}% |')
