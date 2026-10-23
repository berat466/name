import re,os,sys
base='/home/user/name/formul_avcisi/'
def parse(path):
    d={'sweep':[], 'oz':{}, 'gec':{}, 'grp':{}, 'sec':None,'ridge':None}
    for l in open(path).read().splitlines():
        if l.startswith('  ') and '| eta=' in l:
            p=[x.strip() for x in l.split('|')]; d['sweep'].append(p)
        elif l.startswith('SECIM'):
            m=re.search(r'GLOBAL_KURAL_A: maske=(\S+) eta=(\S+) .*GLOBAL_KURAL_B: maske=(\S+) eta=(\S+) ',l); d['sec']=m.groups()
        elif l.startswith('OZET|'):
            f=l.split('|'); kv=dict(x.split('=') for x in f[3:])
            if f[1]=='RIDGE': d['ridge']=kv
            else: d['oz'][(f[1],f[2])]=kv
        elif l.startswith('GECIS|'):
            f=l.split('|'); m=re.search(r'kalibre top1: dogru->yanlis=(\d+) yanlis->dogru=(\d+) net=([+-]\d+)',f[3]); d['gec'][(f[1],f[2])]=[int(x) for x in m.groups()]
        elif l.startswith('GRUP|'):
            f=l.split('|'); d['grp'].setdefault((f[1],f[2]),{})[f[3]]=f[5:]
        elif l.startswith('RIDGE sure'):
            m=re.search(r'RIDGE sure=([\d.]+)s ; CE duzeltme \(gradyan\+D, egitim\) sure=([\d.]+)s',l); d['t']=[float(x) for x in m.groups()]
        elif l.startswith('SURE son'):
            d['tfin']=float(re.search(r'=([\d.]+)s',l).group(1))
        elif l.startswith('TOPLAM'):
            m=re.search(r'sure=([\d.]+)s tepe_RAM=(\d+)MB',l); d['tot']=[float(m.group(1)),int(m.group(2))]
    return d
def man(p): return [l.split('\t') for l in open(p).read().splitlines()]
def f(x): return float(x)
def avg(x): return sum(x)/len(x)
MASKS=['A_V20','B_sik0','C_sik.25','D_sik.5','E_.25/.75/1']
DESC={'A_V20':'A: 1/1/1 (V20)','B_sik0':'B: 0/1/1','C_sik.25':'C: 0,25/1/1','D_sik.5':'D: 0,5/1/1','E_.25/.75/1':'E: 0,25/0,75/1'}
R={}
for H in (1024,512):
    for s in (1,2,3): R[(H,s)]=parse(f'{base}v21_ham/v21log/v21_c2_H{H}_s{s}.txt')
EX={s:parse(f'{base}v21_ham/v21ext/v21ext_c2_H1024_s{s}.txt') for s in (1,2,3)} if os.path.exists(base+'v21_ham/v21ext/DONE') else None
out=[];P=out.append
P('# TEK HAMLE V21 — Seçici CE ağırlık düzeltmesi (`W = W0 − η (D ⊙ M)`)\n')
P('Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v21.cpp` (V20 kopyası, önceki dosyalara dokunulmadı) · ham loglar/komutlar `v21_ham/`.\n')
P('Yöntem: tek Ridge (`W0`), tek CE gradyanı (eğitim, α0=0,2/T0=0,01), tek `D=A⁻¹FᵀG`; maske `M` D\'nin **çıkış sütunlarına** kelime sıklık grubuna göre katsayı verir (V20 grup tanımı aynen: eğitim hedef kitlesinin kümülatif %50\'si = sık (44 tür), %50–80 orta (284), kalan nadir (1673)). Maske değiştirmek yeni Cholesky/gradyan istemez: logitler `Z = Z0 − η·(Zd ⊙ m)` olarak hesaplanır (Z0=F·W0, Zd=F·D bir kez). Her (maske, η) için α,T doğrulama CE ile seçildi; test yalnız seçimden sonra. Maske katsayısı 0 → o sütunun Ridge ağırlığı bit düzeyinde değişmez. Seed-1 A maskesi V20 sonuçlarını birebir yeniden üretti (PPL 199,84 / 199,33) — [Kesin] kod doğrulaması.\n')
# main table
P('## 1. Ana sonuç — doğrulama kuralıyla seçilen (maske, η) ve test sonucu\n')
P('Seçim iki aşamalı ve yalnız doğrulamadan: önce her maske için η, sonra tüm maskeler arasında en iyi (GLOBAL). Kural A: en düşük val CE; kural B: en düşük val CE ve val Top-1 ≥ Ridge − 0,2 pp.\n')
for H in (1024,512):
    P(f'**H={H}** (3 tohum, γ/λ V17 seçimi)\n')
    P('| yöntem | seçilen (maske, η) s1 / s2 / s3 | PPL s1 / s2 / s3 | PPL ort. | Top-1 ort. | Top-5 ort. | test CE ort. | train–val CE farkı ort. |'); P('|---|---|---|---|---|---|---|---|')
    rr=[R[(H,s)]['ridge'] for s in (1,2,3)]
    P(f'| Ridge (η=0) | – | '+' / '.join(r['PPL'] for r in rr)+f' | **{avg([f(r["PPL"]) for r in rr]):.2f}** | {100*avg([f(r["top1"]) for r in rr]):.2f}% | {100*avg([f(r["top5"]) for r in rr]):.2f}% | {avg([f(r["test_CE"]) for r in rr]):.4f} | {avg([f(r["gap"]) for r in rr]):.3f} |')
    for rule,idx,key in (('GLOBAL kural A',0,'MASKE_KURAL_A'),('GLOBAL kural B',2,'MASKE_KURAL_B')):
        rows=[];sel=[]
        for s in (1,2,3):
            sc=R[(H,s)]['sec']; mk,eta=sc[idx],sc[idx+1]; sel.append(f'{mk} η={eta}')
            o=R[(H,s)]['oz'][(key,mk)]; rows.append(o)
        P(f'| {rule} | '+' / '.join(sel)+' | '+' / '.join(o['PPL'] for o in rows)+f' | **{avg([f(o["PPL"]) for o in rows]):.2f}** | {100*avg([f(o["top1"]) for o in rows]):.2f}% | {100*avg([f(o["top5"]) for o in rows]):.2f}% | {avg([f(o["test_CE"]) for o in rows]):.4f} | {avg([f(o["gap"]) for o in rows]):.3f} |')
    P('')
P('## 2. Her maske ayrı (doğrulamayla η seçilmiş, test sonucu; 3 tohum ort.)\n')
for H in (1024,512):
    P(f'**H={H}**\n')
    P('| maske (sık/orta/nadir) | kural | η (s1/s2/s3) | PPL ort. | Top-1 ort. | Top-5 ort. | test CE | train–val farkı | net Top-1 geçişi (s1/s2/s3) |'); P('|---|---|---|---|---|---|---|---|---|')
    for mk in MASKS:
        for key,nm in (('MASKE_KURAL_A','A'),('MASKE_KURAL_B','B')):
            os_=[R[(H,s)]['oz'][(key,mk)] for s in (1,2,3)]; g=[R[(H,s)]['gec'][(key,mk)] for s in (1,2,3)]
            P(f'| {DESC[mk]} | {nm} | '+'/'.join(o['eta'] for o in os_)+f' | {avg([f(o["PPL"]) for o in os_]):.2f} | {100*avg([f(o["top1"]) for o in os_]):.2f}% | {100*avg([f(o["top5"]) for o in os_]):.2f}% | {avg([f(o["test_CE"]) for o in os_]):.4f} | {avg([f(o["gap"]) for o in os_]):.3f} | '+'/'.join(f'{x[2]:+d}' for x in g)+' |')
    P('')
P('η=0.01 ızgara kenarıdır; B/C/D/E maskeleri için val CE η=0,01\'de hâlâ düşüyordu (bölüm 5\'e bakın). [Kesin: log]\n')
P('## 3. Doğrulama taraması (H=1024, tohum 1)\n')
P('| maske | η | val CE | (α,T) | val Top-1 | val Top-5 |'); P('|---|---|---|---|---|---|')
for p in R[(1024,1)]['sweep']:
    P(f'| {p[0]} | {p[1].replace("eta=","")} | {p[2]} | {p[3]} | {p[4]} | {p[5]} |')
P('')
P('## 4. Sıklık gruplarına göre etki (test, H=1024, kural A\'nın maske-başına seçimi, tohum 1)\n')
P('| maske | η | grup | ΔCE (yeni−Ridge) | Top-1 Ridge → yeni | Top-5 Ridge → yeni |'); P('|---|---|---|---|---|---|')
for mk in MASKS:
    o=R[(1024,1)]['oz'][('MASKE_KURAL_A',mk)]
    for g in ('sik','orta','nadir'):
        x=R[(1024,1)]['grp'][('MASKE_KURAL_A',mk)][g]
        dce=re.search(r'fark ([+-][\d.]+)',x[0]).group(1); t1=re.search(r'ridge=([\d.]+) yeni=([\d.]+)',x[1]).groups(); t5=re.search(r'ridge=([\d.]+) yeni=([\d.]+)',x[2]).groups()
        P(f'| {DESC[mk]} | {o["eta"]} | {g} | {dce} | {100*f(t1[0]):.1f}% → {100*f(t1[1]):.1f}% | {100*f(t5[0]):.1f}% → {100*f(t5[1]):.1f}% |')
P('')
P('Doğru→yanlış / yanlış→doğru geçişleri (kalibre Top-1, test, H=1024, kural A seçimi):\n')
P('| maske | tohum 1 | tohum 2 | tohum 3 |'); P('|---|---|---|---|')
for mk in MASKS:
    P(f'| {DESC[mk]} | '+' | '.join('%d→yanlış / %d→doğru (net %+d)'%tuple(R[(1024,s)]['gec'][('MASKE_KURAL_A',mk)]) for s in (1,2,3))+' |')
P('')
if EX:
    P('## 5. EK (görev dışı): genişletilmiş η ızgarası {…, 0,015, 0,02, 0,03} — H=1024, 3 tohum\n')
    P('Görevin η ızgarası 0,01\'de bitiyor ve maskeli varyantlar orada kenarda seçiliyor; maskeli düzeltmenin daha büyük η isteyip istemediğini görmek için aynı kodla `-DEXT` ile yeniden çalıştırıldı. Seçim yine yalnız doğrulamadan. Bu bölüm görev ızgarasının sonucunu **değiştirmez**; ayrı raporlanır.\n')
    for tag,dd in (('Maske başına, kural A',None),):
        pass
    P('| maske | kural | η (s1/s2/s3) | PPL ort. | Top-1 ort. | Top-5 ort. | test CE | train–val farkı |'); P('|---|---|---|---|---|---|---|---|')
    for mk in MASKS:
        for key,nm in (('MASKE_KURAL_A','A'),('MASKE_KURAL_B','B')):
            os_=[EX[s]['oz'][(key,mk)] for s in (1,2,3)]
            P(f'| {DESC[mk]} | {nm} | '+'/'.join(o['eta'] for o in os_)+f' | {avg([f(o["PPL"]) for o in os_]):.2f} | {100*avg([f(o["top1"]) for o in os_]):.2f}% | {100*avg([f(o["top5"]) for o in os_]):.2f}% | {avg([f(o["test_CE"]) for o in os_]):.4f} | {avg([f(o["gap"]) for o in os_]):.3f} |')
    P('')
    for rule,idx,key in (('GLOBAL kural A',0,'MASKE_KURAL_A'),('GLOBAL kural B',2,'MASKE_KURAL_B')):
        rows=[];sel=[]
        for s in (1,2,3):
            sc=EX[s]['sec']; mk,eta=sc[idx],sc[idx+1]; sel.append(f'{mk} η={eta}'); rows.append(EX[s]['oz'][(key,mk)])
        P(f'- {rule} (genişletilmiş): {" / ".join(sel)} → PPL {" / ".join(o["PPL"] for o in rows)} (ort. **{avg([f(o["PPL"]) for o in rows]):.2f}**), Top-1 ort. {100*avg([f(o["top1"]) for o in rows]):.2f}%, Top-5 ort. {100*avg([f(o["top5"]) for o in rows]):.2f}%')
    P('')
P('## 6. Eski–yeni karşılaştırma (H=1024, test, 3 tohum ort.)\n')
def sel_row(name,key,idx):
    rows=[R[(1024,s)]['oz'][(key,R[(1024,s)]['sec'][idx])] for s in (1,2,3)]
    return f'| {name} | {avg([f(o["PPL"]) for o in rows]):.1f} | {100*avg([f(o["top1"]) for o in rows]):.1f}% |'
rr=[R[(1024,s)]['ridge'] for s in (1,2,3)]
P('| Referans | PPL | Top-1 |'); P('|---|---|---|')
P(f'| V17 Ridge (bu çalışmada yeniden ölçüm) | {avg([f(r["PPL"]) for r in rr]):.1f} | {100*avg([f(r["top1"]) for r in rr]):.1f}% |')
P('| V19 (rapor) | 199,9 | %14,9 |')
for mk,nm in (('A_V20','V20 A (maske A, kural A)'),):
    pass
a=[R[(1024,s)]['oz'][('MASKE_KURAL_A','A_V20')] for s in (1,2,3)]; b=[R[(1024,s)]['oz'][('MASKE_KURAL_B','A_V20')] for s in (1,2,3)]
P(f'| V20 A (yeniden üretildi) | {avg([f(o["PPL"]) for o in a]):.1f} | {100*avg([f(o["top1"]) for o in a]):.1f}% |'); P(f'| V20 B (yeniden üretildi) | {avg([f(o["PPL"]) for o in b]):.1f} | {100*avg([f(o["top1"]) for o in b]):.1f}% |')
P(sel_row('**V21 GLOBAL kural A** (doğrulama seçimi)','MASKE_KURAL_A',0)); P(sel_row('**V21 GLOBAL kural B** (doğrulama seçimi)','MASKE_KURAL_B',2))
P('')
P('## 7. Maliyet\n')
t=[R[(1024,s)]['t'] for s in (1,2,3)]; tf=[R[(1024,s)]['tfin'] for s in (1,2,3)]
P(f'- H=1024: Ridge çözümü ort. {avg([x[0] for x in t]):.2f} s; CE düzeltmesi (gradyan + D, eğitim) ort. {avg([x[1] for x in t]):.2f} s; son modelde (eğitim+doğrulama) {avg(tf):.2f} s. Maske uygulaması ek Cholesky/gradyan gerektirmez; her (maske, η) için yalnız `Z0 − η m Zd` eleman-bazlı işlemi (+ val kalibrasyon taraması) yapılır.')
P('- Not: V20\'ye göre toplam süre/RAM yüksek (48 s, 1,5 GB), çünkü bu koşu 21 (maske,η) × α,T taraması + eğitim logitleri (train–val farkı için) + 10 test raporu içeriyor; dağıtım maliyeti değil, deney maliyeti. Dağıtım maliyeti V20 ile aynı: 1 Ridge + 1 gradyan + 1 D.\n')
P('| komut | wall s | user CPU s | RAM MB |'); P('|---|---|---|---|')
for p in ('v21log',)+(('v21ext',) if EX else ()):
    for m in man(f'{base}v21_ham/{p}/manifest.tsv'):
        kv=dict(x.split('=') for x in m[2:]); P(f'| `{m[0]}` | {kv["wall_s"]} | {kv["user_cpu_s"]} | {kv["maxrss_MB"]} |')
P('')
open(base+'v21_gen_out.md','w').write('\n'.join(out)+'\n')
