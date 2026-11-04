import re,glob
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
T=rd(base+'v25_ham/v25log/v25_c2_H1024_s1_ABCDE.txt')
rows=[]
for l in T.splitlines():
    if not l.startswith('RES|'): continue
    f=l.split('|'); name,tg=f[1],f[2]; lam=f[3].split('=')[1]
    g=lambda p,t: re.search(p,t).groups()
    al,Tt=f[4].split('=')[1],f[5].split('=')[1]
    tr=[float(x) for x in g(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',f[6])]
    va=[float(x) for x in g(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',f[7])]
    te=[float(x) for x in g(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',f[8])]
    gap=float(f[9].split('=')[1]); rel=[float(x) for x in f[10].split('=')[1].split('/')]; kl=[float(x) for x in f[11].split('=')[1].split('/')]
    cg=g(r'CE=([\d.]+) top1=([\d.]+) \(n=(\d+)\); yalniz bigram: CE=([\d.]+) top1=([\d.]+)',f[12])
    rows.append(dict(name=name,tg=tg,lam=lam,al=al,T=Tt,tr=tr,va=va,te=te,gap=gap,rel=rel,kl=kl,cg=cg,sec=f[13].split('=')[1] if len(f)>13 else ''))
sysn=[]
for r in rows:
    if r['name'] not in sysn: sysn.append(r['name'])
best={}
for r in rows:
    k=(r['name'],r['tg'])
    if k not in best or r['va'][0]<best[k]['va'][0]: best[k]=r
man=[l.split('\t') for l in rd(base+'v25_ham/v25log/manifest.tsv').splitlines()]
sist=re.findall(r'^SISTEM\|(.*?)\|mz=(\d+) P=(\d+) toplam=(\d+)\|gram\+RHS suresi=([\d.]+)s',T,re.M)
tri=re.search(r'^NG\|trigram\|test\(egitim sayimlari\)\|CE=([\d.]+)\|PPL=([\d.]+)\|top1=([\d.]+)\|top5=([\d.]+)',T,re.M).groups()
o=[];P=o.append
P('# TEK HAMLE V25 — Öğrenme bilgisi nerede kayboluyor? (ölçüm/teşhis)\n')
P('Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v25.cpp` (V24 kopyası, ölçüm aracı) · ham loglar/komutlar `v25_ham/`. Aynı 50k token, bölme, sözlük; öğretici (EM ağırlıklı trigram, eğitim sayımları, **eğitim satırları leave-one-out**) ve tüm ağırlıklar yalnız eğitimden; doğrulama yalnız λ ve (α,T) seçimi; test yalnız son ölçüm. **H=1024, tohum 1** (H=512 ve 3 tohum süre nedeniyle yapılmadı). Tüm modeller aynı kural: λ ∈ küçük ızgara, (α,T) ve λ doğrulama kalibre CE ile. Her sistemde iki hedef ayrı çözülür: `W_Y` (etiket) ve `W_Q` (trigram dağılımı), her biri tek `(ZᵀZ+λI)⁻¹ZᵀHedef` çözümü.\n')
P('**Önemli not (hata düzeltmesi):** ilk koşu, Gram matrisinin yalnız bir üçgenini kopyalamam yüzünden yanlıştı (ağırlıklar kayıp gibi görünüyordu); simetrik doldurma ile yeniden çalıştırıldı. Aşağıdaki sayılar düzeltilmiş koşudan.\n')
P('## 1. Özet tablo (λ doğrulama CE ile seçilmiş; test)\n')
P('Ölçülen iki ayrı başarı: **(i) Q\'ya yaklaşma** = açıklanan varyans `1 − ‖ZW−Q‖²/‖Q−π‖²` (π eğitim unigram), KL(Q‖öğrenci); **(ii) gerçek etiketlerde CE/PPL**. İkisi aynı şey değildir.\n')
P('| sistem (Z) | boyut | hedef | λ | Q açıklanan varyans eğitim / doğr. / test | KL(Q‖öğr.) test | eğitim PPL | doğr. PPL | **test PPL** | test Top-1 / Top-5 | eğitim–doğr. CE farkı |'); P('|---|---|---|---|---|---|---|---|---|---|---|')
dims={s[0]:(s[1],s[2],s[3]) for s in sist}
for n in sysn:
    for tg in ('Y','Q'):
        r=best.get((n,tg))
        if not r: continue
        ev=[1-x for x in r['rel']]
        evs=f'{ev[0]:.3f} / {ev[1]:.3f} / {ev[2]:.3f}' if tg=='Q' else '– (W_Y, Q\'yu hedeflemez)'
        P(f'| {n} | {dims[n][2]} ({dims[n][0]} yoğun + {dims[n][1]} seyrek) | W_{tg} | {r["lam"]} | {evs} | {r["kl"][2]:.3f} | {r["tr"][1]:.1f} | {r["va"][1]:.1f} | **{r["te"][1]:.2f}** | {100*r["te"][2]:.1f}% / {100*r["te"][3]:.1f}% | {r["gap"]:.3f} |')
P(f'| *trigram n-gram (referans, aynı protokol)* | – | – | – | 1 / 1 / 1 (Q\'nun kendisi) | 0 | – | 145.6 | **{float(tri[1]):.2f}** | {100*float(tri[2]):.1f}% / {100*float(tri[3]):.1f}% | – |')
P('')
P('Yoğun sütun sayıları farklı (A: 193, B: 1025, C: 1025 + seyrek); kapasite farkı açıktır, sonuçlar yalnız "temsil kalitesi" olarak okunmamalıdır. Tüm λ satırları ve (α,T) seçimleri `v25_ham/v25log/` dosyasındadır.\n')
P('## 2. Tüm λ değerleri (test PPL / Q açıklanan varyans test)\n')
P('| sistem | hedef | λ | (α,T) | eğitim PPL | doğr. PPL | test PPL | Q açıklanan varyans (test) | trigram bağlamı GÖRÜLEN satırlarda test CE / Top-1 | yalnız bigram görülen CE / Top-1 |'); P('|---|---|---|---|---|---|---|---|---|---|')
for r in rows:
    P(f'| {r["name"]} | W_{r["tg"]} | {r["lam"]} | {r["al"]},{r["T"]} | {r["tr"][1]:.1f} | {r["va"][1]:.1f} | {r["te"][1]:.2f} | {1-r["rel"][2]:.3f} | {r["cg"][0]} / {100*float(r["cg"][1]):.1f}% | {r["cg"][3]} / {100*float(r["cg"][4]):.1f}% |')
P('')

P('PLACEHOLDER')
open(base+'TEK_HAMLE_V25_RAPOR.md','w').write('\n'.join(o)+'\n')
