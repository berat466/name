import re,glob,os,numpy as np
base='/home/user/name/formul_avcisi/'; H=base+'v34_ham/'
def parse(p):
    t=open(p).read(); d={}
    g=lambda pat:float(re.search(pat,t).group(1))
    d['okuma']=g(r'veri dosyalarini okuma=([\d.]+)s')+g(r'expand_s .*?=([\d.]+)s'); d['zincir']=g(r'V31_ZINCIR_OZELLIK\|.*?hazirlik=([\d.]+)s'); d['cmat']=g(r'V31_HAZIRLIK\|.*?sure=([\d.]+)s')
    for k in ('M1','M2','M3','M4','M5','M6'): d[k]=g(r'TT\|'+k+r' .*?\|([\d.]+)s')
    d['mat']=sum(d[k] for k in ('M1','M2','M3','M4','M5','M6')); d['toplam']=d['okuma']+d['zincir']+d['cmat']+d['mat']
    d['ppl']=g(r'SONUC\|ORTAK_KIMLIK\+4F\(secilen\)\|.*?test CE=([\d.]+) PPL=([\d.]+)') if False else float(re.search(r'SONUC\|ORTAK_KIMLIK\+4F\(secilen\)\|.*?test CE=[\d.]+ PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t).group(1))
    m=re.search(r'SONUC\|ORTAK_KIMLIK\+4F\(secilen\)\|.*?test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t); d['ce']=float(m.group(1)); d['t1']=float(m.group(3)); d['t5']=float(m.group(4))
    d['rez']=g(r'= ([\d.e+-]+) \(float'); d['dev']=re.search(r'olasilik_toplam_sapma=(\S+)',t).group(1)
    return d
names=['ref','fast','fast_chol2','fast_chol4','fast_chol6','fast_rhs2','fast_rhs4','fast_rhs6']
R={n:[parse(p) for p in sorted(glob.glob(H+f't_{n}_r[0-9].txt')) if 'TOPLAM' in open(p).read()] for n in names}
def ms(n,k): v=[x[k] for x in R[n]]; return np.mean(v),np.std(v)
LAB={'ref':'V33 OpenBLAS, tek çözüm (referans)','fast':'V34 FAST (paralel kopya/Gram/C kurulumu), tek çözüm','fast_chol2':'FAST + 2 blok Cholesky','fast_chol4':'FAST + 4 blok Cholesky','fast_chol6':'FAST + 6 blok Cholesky','fast_rhs2':'FAST + 2 sağ taraf grubu','fast_rhs4':'FAST + 4 sağ taraf grubu','fast_rhs6':'FAST + 6 sağ taraf grubu'}
o=[];P=o.append
n_rep=len(R['ref'])
P('# TEK HAMLE V34 — Hesabı küçük parçalara bölerek hızlandırma\n')
P(f'''Dal `claude/formul-avcisi` · referans V33 (`5b06884`) · kod `cpp/tek_hamle_v34.cpp` (V33 kopyası; V33 dosyaları değişmedi, OpenBLAS yolu korundu) · ham çıktılar `v34_ham/`. Tohum 1, H=512, λ_C=100, λ_F=3000, 4 çekirdek; **{n_rep} tekrar, ardışık/serpiştirilmiş sırayla** (her turda tüm yapılandırmalar), ortalama ± std. Süre kapsamı V33 ile aynı: veri okuma + `expand_s` + F₁–F₄ üretimi + F Gram/FᵀY + C bloğu kurulumu + A_CC kopya, Cholesky 4002², çoklu sağ taraf çözümü, Schur; **hiperparametre taraması ve rapor tanıları hariç**. Formül `W=(ZᵀZ+Λ)⁻¹ZᵀY` ve tüm hiperparametreler değişmedi.

Eklenen seçenekler (hepsi ortam değişkeni, varsayılan davranış aşağıda belirtilen): 
- `V34_CHOL_BLOCKS=k`: 4002²'lik Cholesky'nin **k bloklu sağdan-bakan blok Cholesky** karşılığı (diyagonal blok `dpotrf`, panel `dtrsm`, artık `dsyrk`; **çapraz terimler korunur**, bağımsız ridge değil).
- `V34_RHS_GROUPS=g`: Y₀ ve X çoklu sağ taraf çözümünde sağ tarafı g sütun grubuna bölüp gruplar OpenMP ile paralel, her grup 1 BLAS ipliğiyle `dtrsm` (kopyasız: ofsetli işaretçi + `ldb`).
- `V34_FAST=1`: matematiksel olarak aynı, yalnız uygulama: F Gram'ında elle transpoz+`gemm_f` yerine `sgemm(Trans)`; kat/kopya (`cat`, `build`) paralel; C bloğu (A_CC, A_CF, R_C) sayımı kimlik sahipliğiyle (`id % nt`) çakışmasız paralel.
''')
P('## 1. Süre karşılaştırması\n')
P('| Yöntem | Toplam süre (s) | Hızlanma (V33 ref.) | F₁–F₄+Gram | C kurulumu | Cholesky | Sağ taraf çözümü (Y₀+X) | Schur (T,U,513²) | Test PPL |\n|---|---|---|---|---|---|---|---|---|')
base_t=ms('ref','toplam')[0]
for n in names:
    if not R[n]: continue
    t,s=ms(n,'toplam'); P(f'| {LAB[n]} | **{t:.2f} ± {s:.2f}** | {base_t/t:.2f}× | {ms(n,"zincir")[0]:.2f} | {ms(n,"cmat")[0]:.2f} | {ms(n,"M2")[0]:.2f} | {np.mean([x["M3"]+x["M4"] for x in R[n]]):.2f} | {np.mean([x["M5"]+x["M6"] for x in R[n]]):.2f} | {np.mean([x["ppl"] for x in R[n]]):.2f} |')
P('')
best=min([n for n in names if R[n]],key=lambda n:ms(n,'toplam')[0])
P(f'**En hızlı toplam:** `{best}` — {ms(best,"toplam")[0]:.2f} ± {ms(best,"toplam")[1]:.2f} s (V33 referansı bu oturumda {base_t:.2f} s; V33 raporundaki 3,75 s).\n')
P('**Isınma düzeltmesi:** referansın 1. turu soğuk (4,68 s). Yalnız 2. ve 3. turlar alınırsa: ref 3,46 s, FAST 3,34 s (1,04×), FAST+4 grup 3,21 s (**1,08×**), FAST+6 blok Cholesky 3,23 s (1,07×). Yani **dürüst hızlanma ≈ 1,04–1,08× (≈ 0,1–0,25 s)**, üstteki 1,14–1,19× soğuk ilk koşunun şişirdiği değerdir.\n')
P('Not: ilk geçişte (tek tekrar, her yapılandırma bir kez) ref 5,78 s ölçüldü; aynı yapılandırmanın tekrarlar arası dalgalanması bu makinede büyük (özellikle F zinciri/Gram ve bellek ayırma kalemleri). Bu yüzden yukarıdaki tablo tekrarlı ortalamadır; tek geçiş sayıları `v34_ham/ref.txt`, `fast.txt`… dosyalarında.\n')
vs=parse(H+'varsayilan_dogrulama.txt')
P("""## 2. Doğruluk (V33 referansına karşı, aynı girdi)

| Yöntem | Ağırlıklar (W_C, W_F; float) | Denklem rezidüsü | Test CE / PPL | Top-1 / Top-5 | \\|Σp−1\\| maks |
|---|---|---|---|---|---|""")
r=parse(H+'ref.txt')
for n,lab in (('ref','V33 referans'),('fast','FAST'),('rhs2','2 grup sağ taraf'),('rhs4','4 grup'),('rhs6','6 grup'),('chol2','2 blok Cholesky'),('chol4','4 blok'),('chol6','6 blok')):
    q=parse(H+n+'.txt'); P(f'| {lab} | {"referans" if n=="ref" else "bit-özdeş (cmp, maks. mutlak fark 0)"} | {q["rez"]:.2e} | {q["ce"]:.4f} / {q["ppl"]:.2f} | {100*q["t1"]:.2f}% / {100*q["t5"]:.2f}% | {q["dev"]} |')
P(f'| FAST + 4 grup, FAST + 4 blok Cholesky + 4 grup (birleşik) | bit-özdeş (cmp) | 6,05e-08 | 4,9401 / 139,78 | 15,60% / 35,68% | ≤1e-13 |')
P(f'| **Varsayılan (FAST açık)** | **bit-özdeş** | {vs["rez"]:.2e} | {vs["ce"]:.4f} / {vs["ppl"]:.2f} | {100*vs["t1"]:.2f}% / {100*vs["t5"]:.2f}% | {vs["dev"]} |')
P("""
Ağırlıklar float çıktı düzeyinde **bit-özdeş**: bloklama/gruplama yalnız çift duyarlıkta ~1e-16 göreli değişiklik yaratır, float'a yuvarlama sonrası farklar görünmez (cmp: fark yok). PPL 139,78 değişmedi.

## 3. Sorular

1. **2, 4 ve 6 blok gerçekten çalıştı mı?** Evet: hem blok Cholesky (4002², `V34_CHOL_BLOCKS=2/4/6`, dpotrf+dtrsm+dsyrk, çapraz terimler korunuyor) hem sağ taraf grupları (`V34_RHS_GROUPS=2/4/6`) çalıştı; loglarda `V34_MOD|... CHOL_BLOKLARI=k` / `RHS_GRUPLARI=g`; ağırlıklar bit-özdeş, PPL 139,78. Not: sağ taraf gruplarında OpenBLAS `Detect OpenMP Loop and this application may hang` uyarısı verdi (OpenMP bölgesinde BLAS çağrısı); takılma olmadı ama bu yol riskli sayıldı.
2. **Hangisi daha hızlı?** Ölçüm gürültüsü (±0,1–0,6 s) bloklamanın farkından büyük. Serpiştirilmiş 3 turun ortalaması: V33 ref 3,86 ± 0,58 (ilk tur soğuk koşu 4,68; ısınmış turlar 3,56 / 3,36), FAST 3,38 ± 0,15, FAST+4 blok Cholesky 3,40 ± 0,11, FAST+6 blok 3,34 ± 0,20, FAST+4 grup 3,25 ± 0,08, FAST+2 grup 3,84, FAST+6 grup 3,64. Eşleşmiş farklar (ref − yöntem) ortalama 0,03–0,61 s, std 0,3–0,5 s: **hiçbir yöntem ısınmış referansa göre güvenilir şekilde ayrışmıyor**; en tutarlı küçük kazanç FAST'ın C kurulumunda (0,80 → 0,50 s) ve 4 grupta (0,3–0,6 s, ama tek turda 2 grup yavaşlatıyor).
3. **Neden?** (a) 4002² Cholesky zaten 0,2 s, çoklu sağ taraf çözümü ≈ 90 GFLOP/s ile makinenin tepe hızına yakın (OpenBLAS iç bloklaması + 4 iplik): ek bloklama aynı flop'u yapar, yalnız kopya/senkronizasyon ekler; blok Cholesky'nin M2 süresi 0,19–0,27 s (iyileşme yok). (b) Sağ taraf gruplaması her grup 1 iplik kullanınca toplam iplik sayısı değişmez, ama OpenBLAS'ın tek çağrıda zaten paralel olduğu yerde küçük kazanç/kayıp ± gürültü; 2 ve 6 grup 4 çekirdeğe dengesiz bölünür (2 grup: 2 çekirdek boşta; 6 grup: 4+2 dalgalar). (c) Gerçek kazanç kopya/seri döngü kalemlerinde: C kurulumu 0,80 → 0,50 s (kimlik sahipliğiyle çakışmasız paralel sayım), F Gram'da transpoz kopyalarının atılması.
4. **Ağırlıklar ve PPL değişti mi?** Hayır: float düzeyinde bit-özdeş, PPL 139,78, Top-1 %15,60, Top-5 %35,68, rezidü 6,05e-8, |Σp−1|≤1e-13.
5. **En hızlı toplam süre:** 3 tur ortalaması **3,25 ± 0,08 s** (FAST + 4 sağ taraf grubu); tekil en iyi koşu 3,07 s (FAST+6 blok, tur 2), ilk geçişte FAST tek koşu 2,54 s. Gürültü payı nedeniyle sağlam iddia: **V33 yolu ısınmış koşuda ≈3,4–3,6 s, FAST ≈3,2–3,5 s; fark ≈ 0,1–0,3 s (3–9 %)**. V33 raporundaki 3,75 s'nin altında hepsi ölçüldü, ama referansın kendisi de bu oturumda ısınınca 3,4–3,6 s'ye indi.
6. **OpenBLAS'ın kendi bloklaması yeterli mi?** Evet, matematiksel çözüm kalemleri için (Cholesky + çözümler ≈ 1,2 s toplam, %35). Ek bloklama/gruplama ölçülebilir fayda sağlamadı. Kalan süre BLAS dışı kalemlerde: F₁–F₄ üretimi + Gram ≈ 1,1 s (%33), C kurulumu ≈ 0,3–0,5 s, Y₀+X çözümü ≈ 1,0 s.

## 4. Varsayılan ve deneysel yollar

- **Varsayılan yapıldı:** `V34_FAST=1` (paralel kopya, `sgemm(Trans)` Gram, çakışmasız C kurulumu): ağırlık bit-özdeş, hiçbir yapılandırmada yavaşlatmadı, C kurulumunu kısalttı. `V34_FAST=0` V33 yolunu verir; `V33_BLAS=0` eski elle yazılmış çekirdekler.
- **Deneysel (kapalı):** `V34_CHOL_BLOCKS` (blok Cholesky; fayda yok), `V34_RHS_GROUPS` (sütun grupları; fayda belirsiz, OpenBLAS uyarısı).

## 5. Sınırlamalar

Tek makine, tek tohum/konfigürasyon, 3 tur (yapılandırma başına 3 koşu); ölçüm gürültüsü (±0,1–0,6 s) iyileştirme büyüklüğüyle aynı mertebede, bu yüzden hızlanma katsayıları ±%10 belirsizdir. İlk koşu soğuk (ref'in 4,68 s'si) ve tek geçiş tabloları (`ref.txt`, `fast.txt`…; ref 5,78 s) ısınma/eşzamanlılık gürültüsü içerir; ana sonuçlar 3-turlu tablodur. F zinciri + Gram hâlâ BLAS dışı elle yazılmış kalemler içerir (syrk, rhs_add, yön üretimi) ve ayrıca bölünmedi.

## 6. Komutlar

`V33_BLAS=1 [V34_FAST=0|1] [V34_CHOL_BLOCKS=k] [V34_RHS_GROUPS=g] [V33_DUMP=önek] ./tek_hamle_v34 2 512 1 1 1 0.5 Z 1 Y - 100 100 3000`. Derleme V33 ile aynı (+ `scipy.libs` OpenBLAS). Ham çıktılar: `v34_ham/`.""")
open(base+'TEK_HAMLE_V34_RAPOR.md','w').write('\n'.join(o)+'\n')
print(best, {n:(round(ms(n,'toplam')[0],2),round(ms(n,'toplam')[1],2)) for n in names if R[n]})
