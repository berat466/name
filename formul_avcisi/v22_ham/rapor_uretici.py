import re
base='/home/user/name/formul_avcisi/'
def fl(x): return float(x)
D={}
for s in (1,2,3):
    d={'hed':{}, 'app':{}, 'cur':[], 'sonlu':[], 'kes':{}, 'gk':{}}
    for l in open(f'{base}v22_ham/v22log/v22_c2_H1024_s{s}.txt').read().splitlines():
        if l.startswith('RIDGE'):
            m=re.search(r'RIDGE sure=([\d.]+)s ; CE duzeltme \(gradyan\+D\) sure=([\d.]+)s ; cozum artigi .*=([\d.e+-]+) ',l); d['ridge']=m.groups()
        elif l.startswith('L0 '):
            m=re.search(r'egitim=([\d.]+) \(PPL ([\d.]+)\) dogrulama=([\d.]+) \(PPL ([\d.]+)\) test=([\d.]+) \(PPL ([\d.]+)\).*= (-?[\d.]+)$',l); d['L0']=m.groups()
        elif l.startswith('SONLU_FARK'):
            m=re.search(r'SONLU_FARK (\S+) h=(\S+) a=(\S+) b=(\S+)',l); d['sonlu'].append(m.groups())
        elif l.startswith('KATSAYI'):
            m=re.search(r'egitim: L0=(\S+) a=(\S+) \(analitik (\S+)\) b=(\S+) .*dogrulama: L0=(\S+) a=(\S+) b=(\S+) en kucuk CE=(\S+) eta\*=(\S+)',l); d['kat']=m.groups()
        elif l.startswith('EGRI ') and '|' in l:
            p=[x.strip() for x in l[5:].split('|')]
            if p[0]!='eta': d['cur'].append(p)
        elif l.startswith('HEDEF|'):
            f=l.split('|'); kv=dict(x.split('=',1) for x in f[2:]); d['hed'][(f[1],f[2].split('=')[1])]=kv
        elif l.startswith('UYGULA|'):
            f=l.split('|'); ppl=f[2].split('=')[1]
            def g(p,t): return re.search(p,t).groups()
            e=g(r'eta_kullanilan=(\S+)',f[3])[0]
            tr=g(r'egitim CE=([\d.]+) PPL=([\d.]+)',f[5]); va=g(r'dogr CE=([\d.]+) PPL=([\d.]+)',f[6]); te=g(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+)',f[7])
            er=f[8].split('=')[1]; cal=g(r'alpha=(\S+),T=(\S+)\): test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',f[9])
            pr=f[4].split('=')[1]
            d['app'][(f[1],ppl)]=dict(eta=e,pred=pr,tr=tr,va=va,te=te,err=er,cal=cal,note='ULASILAMAZ' in f[3])
        elif l.startswith('GERCEK_EGRI_KESISIM'):
            f=l.split('|'); d['kes'][(f[1],f[2].split('=')[1])]=(f[4],f[5].split('=')[1])
        elif l.startswith('UYGULA_GERCEK_KOK'):
            f=l.split('|'); ppl=f[2].split('=')[1]; d['gk'][ppl]=dict(eta=f[3].split('=')[1],tr=re.search(r'CE=([\d.]+) PPL=([\d.]+)',f[4]).groups(),va=re.search(r'CE=([\d.]+) PPL=([\d.]+)',f[5]).groups(),te=re.search(r'CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+)',f[6]).groups())
        elif l.startswith('SURE'):
            m=re.search(r'TOPLAM=([\d.]+)s tepe_RAM=(\d+)MB',l); d['tot']=m.groups()
    D[s]=d
man=[l.split('\t') for l in open(base+'v22_ham/v22log/manifest.tsv').read().splitlines()]
T=['190','175','150','137','100']; CEs={'190':'5,247','175':'5,165','150':'5,011','137':'4,920','100':'4,605'}
o=[];P=o.append
def avg(x): return sum(x)/len(x)
P('# TEK HAMLE V22 — Hedef hata oranından doğrudan ağırlık hesaplama\n')
P('Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v22.cpp` (V21 kopyası; önceki dosyalara dokunulmadı) · ham loglar/komutlar `v22_ham/`. H=1024, c=2, aynı metin/bölme, tohum 1/2/3 (γ,λ = V17: (4,10),(4,100),(2,1)).\n')
P('**Yöntem.** Ridge `W0` (Cholesky, eğitim), tek CE gradyanı → `D=A⁻¹FᵀG` (α0=0,2, T0=0,01 sabit). Eğitim CE\'sinin doğrultu boyunca ikinci derece modeli `CE(W0−ηD) ≈ L0 + aη + ½bη²`: `L0` ve `a=−⟨D,FᵀG⟩/n` eğitimden **analitik/doğrudan**, `b` eğitim CE\'sinin h=0,001 merkezi sonlu farkından (±h için 2 ek eğitim CE değerlendirmesi, ağırlık güncellemesi değil). Hedef denklemi çözülür, `W_hedef = W0 − η_hesaplanan·D` bir kez üretilir. **Bu raporda tüm modeller yalnız eğitimden üretildi** (V20/V21\'deki eğitim+doğrulama son modeli kullanılmadı) ki doğrulama kontrol, test tamamen bağımsız kalsın; bu yüzden Ridge test PPL\'i burada ≈208,5 (V17: 206,8 eğitim+doğrulama modeliyle). Sabit α0,T0 CE\'si (kalibrasyon taraması yok) denklemin tanımlandığı niceliktir; ek olarak doğrulama-kalibre (α,T) test PPL\'i de verilir. Test hiçbir η/parametre seçimine girmedi.\n')
# summary
P('## 0. Özet (3 tohum ort., test)\n')
rows=[]
for t in T:
    a=[D[s]['app'][('EGITIM_DENKLEMI',t)] for s in (1,2,3)]
    rows.append((t,a))
P('Görev denklemi (eğitim CE\'si) için sonuç:\n')
P('| hedef PPL (CE) | η_hesaplanan (s1/s2/s3) | tahmin edilen eğitim CE | gerçek eğitim PPL | doğrulama PPL | **test PPL** | test Top-1 | doğr.-kalibre test PPL |'); P('|---|---|---|---|---|---|---|---|')
for t,a in rows:
    P(f'| {t} ({CEs[t]}) | '+'/'.join(f"{fl(x['eta']):.5f}" for x in a)+f' | {avg([fl(x["pred"]) for x in a]):.4f} | {avg([fl(x["tr"][1]) for x in a]):.1f} | {avg([fl(x["va"][1]) for x in a]):.1f} | **{avg([fl(x["te"][1]) for x in a]):.1f}** | {100*avg([fl(x["te"][2]) for x in a]):.1f}% | {avg([fl(x["cal"][3]) for x in a]):.1f} |')
P('')
a0=[D[s]['L0'] for s in (1,2,3)]
P(f'Ridge (η=0, tüm veri eğitim-yalnız, sabit α0,T0): eğitim PPL {avg([fl(x[1]) for x in a0]):.1f}, doğrulama PPL {avg([fl(x[3]) for x in a0]):.1f}, **test PPL {avg([fl(x[5]) for x in a0]):.1f}**.\n')
P('Bulgular:\n')
P('- [Kesin] Denklem **eğitim** CE\'sine yazıldığı için hedefe eğitimde yaklaşılıyor, test PPL\'i hedeften bağımsız olarak ≈ 203–208\'de kalıyor: hiçbir hedef test\'te gerçekleşmedi. Hedef 100 için denklem eğitim PPL\'ini yalnız 116–122\'ye indirdi (model 100 öngördü), test PPL 201,7–202,9. Modelden bağımsız gerçek-eğri kökünde (bölüm 3) eğitim PPL 100\'e inebiliyor (tohum 1 ve 3), ama test PPL 205–206\'ya çıkıyor ve Top-1 %14\'e düşüyor; tohum 2\'de bu doğrultuda eğitim PPL\'i 103,9\'un altına inemiyor (hedef 100 ulaşılamaz).')
P('- [Kesin] 190, 175 hedefleri eğitim verisinde Ridge ile zaten aşılmış (eğitim PPL ≈ 167–176; 175 hedefi tohum 2\'de η≈0,00008 ile, diğerlerinde η=0 ile): denklem `L0 ≤ hedef` verir, η=0. Bu, eğitim CE\'sinin genelleme için iyimser olduğunu gösteriyor (Ridge eğitim–test CE farkı ≈ 0,22, tohum 1).')
P('- [Kesin] Yerel ikinci derece model **geçersiz**: eğitim eğriliği negatif (b≈−3000; parabol aşağı açık, minimumu yok), gerçek eğri ise η≈0,02\'de dönüp yükseliyor. Model, uzak köklerde CE\'yi sistematik olarak fazla iyimser tahmin ediyor (hedef 100 için tahmin–gerçek −0,15…−0,20 nat, 3 tohum). Doğrulama eğriliği tohuma ve h\'ye göre işaret/büyüklük değiştiriyor (ör. −142…+411): parabol değil.')
P('- [Kesin] Sayısal çözüm sorun değil: `‖AD−FᵀG‖/‖FᵀG‖ ≈ 3e-7`; analitik `a` ile sonlu fark `a` h→0\'da örtüşüyor (−88,3 vs −87,6, h=0,0005). Engel sayısal değil.')
P('- [Kesin] Tek doğrultuda doğrulama/test CE\'sinin alt sınırı belli: tohum 1\'de sabit α0,T0 ile doğrulama CE minimumu ≈ 5,367 (PPL 214,2; η≈0,006), test CE minimumu ≈ 5,3125 (PPL ≈ 203,1; η≈0,004–0,005); ölçüm yalnız tohum 1 için tablolandı (bölüm 5), diğer tohumlarda sonuçlar aynı yönde (bölüm 2). Hiçbir η test PPL\'ini 190\'a, 137\'ye, 100\'e götüremez; bunun için yeni bilgi/yön gerekir (V20\'nin kalibrasyonlu en iyi sonucu ≈ 199–200 PPL, aynı sınıf).')
P('- [Muhtemel] Engelin ana kaynağı **aşırı uyum / genelleme açığı**: eğitim PPL\'i tohum 1\'de 94\'e kadar iniyor (η=0,02), doğrulama/test aynı anda yükseliyor. Özellik kapasitesi eğitim verisini ezberlemeye yetiyor (H=1024 rastgele özellik, 39 840 örnek, 2001 sınıf); eksik olan, **yeni bağlam bilgisi** (trigram 137 PPL sayımlardan geliyor).')
P('- [Kesin] Aynı `D` yönünü kullanan tüm hedefler yalnız farklı bir η seçimidir; yeni öğrenme bilgisi üretmezler. Hedef değeri bir "öğrenme girdisi" değil, tek boyutlu bir ayar noktasıdır.\n')
P('**Ana soruya cevap:** İstenen PPL\'i doğrudan denkleme koymak, **eğitim CE\'si** için kısmen çalışır (η hesaplanır, eğitim PPL\'i hedefe yaklaşır; hata modelin eğrilik hatasından gelir), fakat **test PPL\'ini** hedefe götürmez. Engel hesaplama değil: (1) hedef eğitim kaybında tanımlı, test kaybında değil; (2) tek doğrultu `−D` boyunca test CE\'sinin minimumu ≈ 5,31 (PPL ≈ 203) ve bu doğrultuda aşırı uyum başlar; (3) ikinci derece model eğrilik değiştiği için uzak köklerde yanlış.\n')
# section 1
P('## 1. Katsayılar ve sayısal doğrulama\n')
P('| tohum | L0 eğitim / doğr. / test (PPL) | a (analitik) | a (FD h=0,0005 / 0,001 / 0,0025) | b eğitim (h=0,0005 / 0,001 / 0,0025) | b doğrulama | çözüm artığı | Ridge s | CE düzeltme s |'); P('|---|---|---|---|---|---|---|---|---|')
for s in (1,2,3):
    d=D[s]; sf=[x for x in d['sonlu'] if x[0]=='egitim']; sv=[x for x in d['sonlu'] if x[0]=='dogrulama']
    P(f'| {s} | {d["L0"][1]} / {d["L0"][3]} / {d["L0"][5]} | {d["L0"][6]} | '+' / '.join(f'{fl(x[2]):.1f}' for x in sf)+' | '+' / '.join(f'{fl(x[3]):.0f}' for x in sf)+' | '+' / '.join(f'{fl(x[3]):.0f}' for x in sv)+f' | {fl(d["ridge"][2]):.1e} | {d["ridge"][0]} | {d["ridge"][1]} |')
P('')
P('## 2. Deney A/B — görev denklemi (eğitim CE\'si) — her tohum ayrı\n')
P('Sütunlar: η_hesaplanan; ikinci derecenin öngördüğü eğitim CE; gerçek eğitim CE/PPL; (tahmin−gerçek); doğrulama PPL; test CE/PPL/Top-1 (sabit α0,T0); doğrulama-kalibre (α,T) test PPL.\n')
P('| tohum | hedef | η | tahmin CE | gerçek eğitim CE (PPL) | tahmin−gerçek | doğr. PPL | test CE | **test PPL** | test Top-1 | kalibre (α,T) → test PPL / Top-1 |'); P('|---|---|---|---|---|---|---|---|---|---|---|')
for s in (1,2,3):
    for t in T:
        x=D[s]['app'][('EGITIM_DENKLEMI',t)]; h=D[s]['hed'][('EGITIM_DENKLEMI',t)]
        P(f'| {s} | {t} | {fl(x["eta"]):.5f} | {x["pred"]} | {x["tr"][0]} ({x["tr"][1]}) | {fl(x["err"]):+.4f} | {x["va"][1]} | {x["te"][0]} | **{x["te"][1]}** | {100*fl(x["te"][2]):.1f}% | α={x["cal"][0]},T={x["cal"][1]} → {x["cal"][3]} / {100*fl(x["cal"][4]):.1f}% |')
P('')
P('## 3. Modelden bağımsız kontrol: gerçek eğitim eğrisinin hedefi kestiği η (yalnız değerlendirme)\n')
P('İkinci derece modelin hatasını ayırmak için aynı tek doğrultu boyunca gerçek eğitim CE eğrisi 17 η noktasında hesaplandı; hedefi kestiği η doğrusal enterpolasyonla bulundu ve orada gerçek değerler ölçüldü (yeni ağırlık = yine `W0−ηD`, tek ağırlık hesabı; η için yalnız ileri değerlendirme kullanıldı, gradyan adımı yok).\n')
P('| tohum | hedef | eğri minimumu (eğitim PPL, η) | kesen η | eğitim PPL | doğr. PPL | test PPL | test Top-1 |'); P('|---|---|---|---|---|---|---|---|')
for s in (1,2,3):
    for t in T:
        k=D[s]['kes'][('egitim',t)]; g=D[s]['gk'].get(t)
        if g: P(f'| {s} | {t} | {k[0].split("(")[1].split(")")[0].replace("PPL ","")} | {fl(g["eta"]):.5f} | {g["tr"][1]} | {g["va"][1]} | {g["te"][1]} | {100*fl(g["te"][2]):.1f}% |')
        else: P(f'| {s} | {t} | {k[0].split("(")[1].split(")")[0].replace("PPL ","")} | '+('η=0 (zaten sağlandı)' if k[1]=='0' else 'ULAŞILAMAZ (eğri minimumu hedefin üstünde)')+' | – | – | – | – |')
P('')
P('## 4. Doğrulama denklemi (tanı; doğrulama a,b,L0 için kullanıldı; test bağımsız)\n')
P('Aynı denklem doğrulama CE\'sine yazıldı: hedeflere "genelleme" açısından bakmak için. Doğrulama burada kontrol/tanı amaçlıdır, test hiçbir kararda yok.\n')
P('| tohum | hedef | durum | η | öngörülen min CE (PPL) | gerçek doğr. PPL | tahmin−gerçek | test PPL | test Top-1 |'); P('|---|---|---|---|---|---|---|---|---|')
for s in (1,2,3):
    for t in T:
        h=D[s]['hed'][('DOGRULAMA_DENKLEMI',t)]; x=D[s]['app'][('DOGRULAMA_DENKLEMI',t)]
        P(f'| {s} | {t} | {h["durum"]} | {fl(x["eta"]):.5f} | {h["ikinci_derece_min_CE"]} | {x["va"][1]} | {fl(x["err"]):+.3f} | {x["te"][1]} | {100*fl(x["te"][2]):.1f}% |')
P('')
P('## 5. Gerçek CE eğrisi (tohum 1; sabit α0,T0; ikinci derece tahmin)\n')
P('| η | eğitim CE | doğrulama CE | test CE | ikinci derece tahmin (eğitim) |'); P('|---|---|---|---|---|')
for r in D[1]['cur']: P('| '+' | '.join(r)+' |')
P('')
P('## 6. Karşılaştırma (H=1024, test PPL)\n')
P('| yöntem | PPL | not |'); P('|---|---|---|')
P('| V17 Ridge (eğitim+doğrulama modeli, kalibre) | 206,8 | referans |'); P('| V20 CE düzeltmesi (A / B) | 198,9 / 199,8 | α,T doğrulamayla seçildi |'); P('| V21 | ≈198,9–199,3 | maske, Top-1 dengeleri |')
P(f'| V22, hedef 100 (eğitim denklemi), sabit α0,T0 | {avg([fl(D[s]["app"][("EGITIM_DENKLEMI","100")]["te"][1]) for s in (1,2,3)]):.1f} | hedefe test\'te ulaşılmadı; eğitim-yalnız model (Ridge burada {avg([fl(x[5]) for x in a0]):.1f}) |')
P('| trigram (ölçülmüş referans) | 137 | sayım tabanlı, bu yöntem değil |\n')
P('## 7. Sınırlar ve ne değişmeli\n')
P('- Tek metin, tek bölme; test n=4 980. Tohumlar arası farklar küçük ama sonuçlar yalnız bu veri için geçerli.')
P('- Denklem tek boyutlu: yalnız `−D` doğrultusu araştırıldı; tüm hedefler aynı yön, yalnız farklı η. Yeni öğrenme bilgisi içermez. [Kesin]')
P('- b, yerel sonlu farktan; hedefe uzak köklerde model güvenilmez (negatif eğrilik → minimum yok). Daha iyi modelleme (3. derece / gerçek 1-B eğriyi doğrudan çözme) hedefi **eğitimde** daha iyi tutturur ama test sorununu çözmez.')
P('- Hedef eğitim CE\'sinde tanımlı → genelleme açığını (eğitim–test CE farkı Ridge\'de ≈0,22, η arttıkça ≈0,7\'ye çıkar) hesaba katmıyor. [Kesin]')
P('- [Tahmin] Değişmesi gerekenler: (1) hedef denklemi **dışarıda-bırakılmış** (leave-one-out / çapraz uydurma) CE üzerinde yazmak — Ridge için LOO artığı kapalı formda `e_i/(1−h_ii)` ve mevcut Cholesky ile hesaplanabilir; bu denenmedi; (2) tek doğrultu yerine hedefe göre **yeni bilgi** ekleyen özellikler (kelime kimliği / n-gram sayım özellikleri; V10–V11\'de kısmen denendi); (3) test için gerçekçi hedef: bu aile ≈199 PPL civarında doyuyor.')
P('- Süre: tohum başı ≈ 50 s, RAM ≈ 1,56 GB (eğitim/doğr./test logitleri bellekte).\n')
P('| komut | wall s | user CPU s | RAM MB |'); P('|---|---|---|---|')
for m in man:
    kv=dict(x.split('=') for x in m[2:]); P(f'| `{m[0]}` | {kv["wall_s"]} | {kv["user_cpu_s"]} | {kv["maxrss_MB"]} |')
open(base+'TEK_HAMLE_V22_RAPOR.md','w').write('\n'.join(o)+'\n')
