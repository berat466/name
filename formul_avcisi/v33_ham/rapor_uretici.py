import re,glob,os,numpy as np
base='/home/user/name/formul_avcisi/'; H=base+'v33_ham/'
def parse(p):
    t=open(p).read(); d={}
    d['okuma']=float(re.search(r'veri dosyalarini okuma=([\d.]+)s',t).group(1)); d['expand']=float(re.search(r'expand_s .*?=([\d.]+)s',t).group(1))
    d['zincir']=float(re.search(r'V31_ZINCIR_OZELLIK\|.*?hazirlik=([\d.]+)s',t).group(1)); d['cmat']=float(re.search(r'V31_HAZIRLIK\|.*?sure=([\d.]+)s',t).group(1))
    for k in ('M1','M2','M3','M4','M5','M6','E1','E2','E3','P2','P5'):
        m=re.search(r'TT\|'+k+r' .*?\|([\d.]+)s',t); d[k]=float(m.group(1)) if m else 0.0
    r=re.search(r'SONUC\|ORTAK_KIMLIK\+4F\(secilen\)\|.*?test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t); d['ppl']=float(r.group(2)); d['ce']=float(r.group(1)); d['t1']=float(r.group(3)); d['t5']=float(r.group(4))
    d['rez']=float(re.search(r'= ([\d.e+-]+) \(float',t).group(1)); d['wall']=float(re.search(r'TOPLAM sure=([\d.]+)s',t).group(1)); d['ram']=float(re.search(r'tepe_RAM=(\d+)MB',t).group(1))
    d['mat']=d['M1']+d['M2']+d['M3']+d['M4']+d['M5']+d['M6']; d['hazir']=d['okuma']+d['expand']+d['zincir']+d['cmat']; d['toplam']=d['hazir']+d['mat']
    return d
def runs(pref): return [parse(p) for p in sorted(glob.glob(H+pref+'_r*.txt'))]
E=runs('eski'); N=runs('yeni')
def a(L,k): return np.mean([x[k] for x in L]), (np.std([x[k] for x in L]) if len(L)>1 else 0.0)
def f(L,k): m,s=a(L,k); return f'{m:.2f} ± {s:.2f}' if len(L)>1 else f'{m:.2f}'
o=[];P=o.append
P('# TEK HAMLE V33 — C++ hız optimizasyonu (aynı matematik, optimize doğrusal cebir)\n')
P('''Dal `claude/formul-avcisi` · kod `cpp/tek_hamle_v33.cpp` (V31+zamanlayıcı kopyası; V31/V31_sure dosyaları değişmedi) · ham çıktılar `v33_ham/`. Makine: 4 çekirdek Xeon 2,8 GHz. **Tek tohum (1), H=512, sabit hiperparametre: λ_C=100, λ_F=3000, 3 tekrar.** Formül, F zinciri, γ, λ, bölünme, okuma (V26 doğrusal) değişmedi.

**Ne değişti:** `V33_BLAS=1` ile elle yazılmış çekirdekler yerine sunucuda zaten bulunan **OpenBLAS** (scipy wheel'i ile gelen `libscipy_openblas-5f890258.so`; indirme yok; sistemin `libblas.so.3`'ü referans BLAS olduğundan kullanılmadı) kullanılır: `sgemm` (tüm float matris çarpımları, `gemm_f`), `dpotrf` (Cholesky, 4002² ve 513²), `dtrsm` ×2 (çoklu sağ taraf çözümü; `dpotrs` yerine satır-ana düzende doğrudan), `dgemm` (Schur parçaları T, U). Ek olarak tek-λ_C koşusunda `A_CC` kopyası taşıma ile atlanır, Y₀/X çözümleri çift duyarlıkta tutulup T,U'da kullanılır. **Eski çekirdekler silinmedi**: `V33_BLAS=0` (varsayılan) eski yolu çalıştırır.
''')
P('## 1. Süre kapsamı\n')
P('"Tek-hamle toplam" = veri okuma + `expand_s` + F₁–F₄ üretimi + F Gram/FᵀY + C bloğu (A_CC, A_CF, R_C) kurulumu + [A_CC kopya, Cholesky 4002², Y₀ çözümü, X çözümü, Schur parçaları, Schur sistemi]. **Dışarıda:** hiperparametre taraması, yalnız-kimlik λ_C adayları, doğrulama okuma seçimi (E1/E2), eğitim logitleri ve rapor tanıları (E3), test tahmini (P2/P5). V32 denetiminde 14,6–14,8 s’lik "eski" süre aynı kapsamdaydı (hazırlık 2,7–3,8 + matematik 11,1–12,0); bu raporda eski süre aynı program, aynı koşulda yeniden ölçüldü.\n')
P(f'## 2. Hız testi ({len(E)} tekrar, ort. ± std, saniye)\n')
P('| Ölçüm | Eski C++ | Optimize C++ (OpenBLAS, 4 iplik) | Hızlanma |\n|---|---|---|---|')
rows=[('Veri okuma + `expand_s`',lambda L:[x['okuma']+x['expand'] for x in L]),('F₁–F₄ üretimi + F Gram/FᵀY (tek kalem; ayrı ölçülmedi)',lambda L:[x['zincir'] for x in L]),('Matris oluşturma (A_CC, A_CF, R_C; seyrek sayım)',lambda L:[x['cmat'] for x in L]),
('A_CC kopya + λ_C (M1)',lambda L:[x['M1'] for x in L]),('Cholesky 4002² (M2)',lambda L:[x['M2'] for x in L]),('Çoklu sağ taraf çözümü: Y₀ (2001 sağ taraf, M3) + X (513, M4)',lambda L:[x['M3']+x['M4'] for x in L]),('Schur işlemleri: T,U (M5) + 513² sistem ve W_C (M6)',lambda L:[x['M5']+x['M6'] for x in L]),
('**Tek-hamle toplam**',lambda L:[x['toplam'] for x in L])]
for nm,fn in rows:
    e=fn(E); n=fn(N); sp=(f'{np.mean(e)/np.mean(n):.1f}×' if np.mean(n)>0.01 else 'kopya atlandı')
    P(f'| {nm} | {np.mean(e):.2f} ± {np.std(e):.2f} | {np.mean(n):.2f} ± {np.std(n):.2f} | {sp} |')
P(f'| Matematiksel çözüm (M1–M6) | {f(E,"mat")} | {f(N,"mat")} | {a(E,"mat")[0]/a(N,"mat")[0]:.1f}× |')
P(f'| (kapsam dışı) program toplam wall süresi: tarama yok ama tanılar dahil | {f(E,"wall")} | {f(N,"wall")} | {a(E,"wall")[0]/a(N,"wall")[0]:.1f}× |')
P(f'| Test PPL (tohum 1) | {np.mean([x["ppl"] for x in E]):.2f} | {np.mean([x["ppl"] for x in N]):.2f} | – |')
P('')
P('Eski 14,6 s ile yeni sonuç aynı kapsamda: **evet** (aynı satırlar toplanıyor; bu raporun eski koşuları da ≈13–15 s verdi, V32 denetimiyle tutarlı). Tohum 1 için tek konfigürasyon; 3 tekrar donanım gürültüsünü gösterir, tohum varyansını değil.\n')
# esdegerlik
P('## 3. Sayısal eşdeğerlik (aynı girdi, eski vs yeni)\n')
import numpy as np
dd={}
for nm in ('WC','WF'):
    try:
        x=np.fromfile(f'/tmp/v33_old_{nm}.bin',dtype=np.float32); y=np.fromfile(f'/tmp/v33_new_{nm}.bin',dtype=np.float32); dd[nm]=(float(np.abs(x-y).max()),float(np.abs(x).max()),float(np.linalg.norm(x-y)/np.linalg.norm(x)),x.size)
    except Exception as e: dd[nm]=None
P('| Büyüklük | W_C (4002×2001, kimlik) | W_F (513×2001, F) |\n|---|---|---|')
if dd.get('WC') and dd.get('WF'):
    P(f'| maks. mutlak fark | {dd["WC"][0]:.2e} (maks \\|W\\|={dd["WC"][1]:.3f}) | {dd["WF"][0]:.2e} (maks \\|W\\|={dd["WF"][1]:.4f}) |')
    P(f'| göreli Frobenius fark | {dd["WC"][2]:.2e} | {dd["WF"][2]:.2e} |')
P(f'| denklem rezidüsü ‖Zᵀ(ZW−Y)+ΛW‖/‖ZᵀY‖ | eski {np.mean([x["rez"] for x in E]):.2e} → yeni {np.mean([x["rez"] for x in N]):.2e} | (aynı ortak sistem) |')
P(f'| Test CE / PPL / Top-1 / Top-5 | eski {E[0]["ce"]:.4f} / {E[0]["ppl"]:.2f} / {100*E[0]["t1"]:.2f}% / {100*E[0]["t5"]:.2f}% | yeni {N[0]["ce"]:.4f} / {N[0]["ppl"]:.2f} / {100*N[0]["t1"]:.2f}% / {100*N[0]["t5"]:.2f}% |')
P('\nOlasılık normalizasyonu: her iki yolda `|Σp−1|` ≤ 1e-13 (logda `olasilik_toplam_sapma`). Fark kaynağı: eski yol Y₀/X çözümlerini float’a yuvarlayıp T,U’yu float Y₀/X ile hesaplıyordu; yeni yol çift duyarlık kullanır → yeni rezidü daha küçük (**yeni çözüm eskiden daha doğru**), ağırlık farkı ~1e-7…1e-5 göreli, PPL ve Top-k **özdeş** (iki tohum-1 koşusunda aynı 139,78).\n')
# thread
P('## 4. Çekirdek sayısı etkisi (tek tekrar)\n')
P('| yapılandırma | tek-hamle toplam (s) | matematik M1–M6 (s) |\n|---|---|---|')
for nm,p in (('Eski, 1 iplik (OMP=1)','eski_iplik1'),('Eski, 4 iplik (ort.)','__e'),('OpenBLAS, 1 iplik','yeni_iplik1'),('OpenBLAS, 2 iplik','yeni_iplik2'),('OpenBLAS, 4 iplik (ort.)','__n')):
    if p=='__e': P(f'| {nm} | {a(E,"toplam")[0]:.2f} | {a(E,"mat")[0]:.2f} |'); continue
    if p=='__n': P(f'| {nm} | {a(N,"toplam")[0]:.2f} | {a(N,"mat")[0]:.2f} |'); continue
    fp=H+p+'.txt'
    if os.path.exists(fp): x=parse(fp); P(f'| {nm} | {x["toplam"]:.2f} | {x["mat"]:.2f} |')
    else: P(f'| {nm} | ölçülmedi | ölçülmedi |')
P('')
P("""## 5. Sorular

1. **Eski süre → yeni süre.** Aynı kapsamda, bu raporun 3 tekrarında: **12,97 ± 0,37 s → 3,75 ± 0,19 s (3,5×)**. V32 denetiminde raporlanan ≈14,6 s'ye göre (o ölçüm ısınma/dalgalanma payı taşıyordu; bu oturumun eski koşuları 12,6–13,4 s verdi) hızlanma ≈ **3,9×**. Yalnız matematiksel çözüm (M1–M6): **10,61 → 1,67 s (6,4×)**. Tek tohum (1), H=512, tek konfigürasyon.
2. **En büyük darboğaz:** elle yazılmış Cholesky (4002², 4,0 s) ve çoklu sağ taraf çözümü (Y₀, 2001 sağ taraf; 4,9 s) — ikisi toplam matematiksel sürenin ≈%84'ü. OpenBLAS ile Cholesky 4,0→0,20 s (19,7×), sağ taraf çözümleri 4,9→0,88 s (5,5×).
3. **Ağırlık farkı:** W_C göreli Frobenius 2,3e-7 (maks mutlak 4,5e-8), W_F göreli 8,2e-6 (maks mutlak 9,4e-8). Denklem rezidüsü eskiden yeniye 2,7e-7 → 6,1e-8 (yeni daha doğru: çift duyarlık Y₀/X). Farklar float duyarlığı mertebesinde ve eski yolun yuvarlama kaynaklı.
4. **PPL aynı mı?** Evet: test CE 4,9401, PPL **139,78 (fark 0,00)**, Top-1 %15,60, Top-5 %35,68 — eski ve yeni özdeş (tohum 1; 3 tohum ortalaması 140,3 daha önceki V31 raporunda; bu denetimde tohum 2/3 yeniden koşturulmadı).
5. **Klasik MLP'nin 7 s eşiği:** sabit-hiperparametreli ağırlık üretimimiz artık **≈3,75 s** (+ α,ε okuma seçimi ≈0,45 s, test tahmini ≈0,43 s) → klasik MLP'nin doğrulama PPL ≈147'ye ulaştığı ≈7 s'nin **altında**, tek-koşu süresinin (21,5 s) çok altında. **Kalite farkı korunuyor ve hız kazancı bunu kapatmıyor:** MLP 3 epok sonra devam edip test PPL **122,9**'a iniyor (en iyi doğrulama ≈12 s'de), bizim yöntem 140,3'te sabit. Yani artık "aynı kalite düzeyinde daha hızlı" ama "daha iyi kalite" değil. Uyarı: klasik süre `import torch` (13,4 s) ve hiperparametre seçimi (≈216 s) hariç; bizim tarafta hiperparametre taraması (V31: ≈45–50 s hesap) hariç — OpenBLAS ile λ_C adayı başına maliyet ≈10,6 → ≈1,2 s (M1–M5 bileşenlerinden hesap, doğrudan ölçülmedi) olacağından seçim maliyeti de benzer oranda düşmeli [tahmin].
6. **Sonraki gerçek darboğaz:** F₁–F₄ üretimi + F Gram/FᵀY **1,36 s (toplamın %36'sı)** — elle yazılmış `syrk`/çapraz Gram ve yön üretimi, hâlâ OpenBLAS dışı; ardından Y₀+X çözümü 0,88 s, Schur 0,58 s, C bloğu kurulumu 0,65 s (seri sayım döngüleri). OpenBLAS ile iplik ölçeklemesi: 1 iplik 5,86 s, 2 iplik 4,35 s, 4 iplik 3,75 s (verimsiz ölçekleme: seri kısımlar ve küçük işler baskın).

## 6. Ek kontrol: V32 süre betiği (checkpoint hatası)

`v32_sure_ham/mlp_sure_olcum.py` test öncesi **en iyi doğrulama checkpoint'ini geri yüklemiyordu** (132,5 PPL buradan). Düzeltme eklendi (`best_state` saklanır, test öncesi `load_state_dict(best_state)`; V32 `klasik_baz_v32.py`'deki davranışla aynı). **Düzeltilmiş betik YENİDEN ÇALIŞTIRILMADI**: görev klasik MLP'yi yeniden eğitmeyi yasakladığı için yalnız kodu düzelttim, sözdizimini doğruladım; bu yüzden düzeltmenin PPL'i (beklenti: V32'nin 122,9'una yakın) bu raporda **ölçülmüş değil**. Resmî 122,9 PPL (V32, `klasik_sonuclar.jsonl`) değişmedi.

## 7. Sınırlamalar

- Tek tohum (1) ve tek konfigürasyon (λ_C=100, λ_F=3000); H=512. H=1024 ve diğer tohumlar yeniden zamanlanmadı.
- OpenBLAS, scipy wheel'inin gömülü kütüphanesi (`/usr/local/lib/python3.13/dist-packages/scipy.libs/libscipy_openblas-5f890258.so`); başka makinede yol değişir (rpath). Sistemin `libblas.so.3`'ü referans BLAS olduğu için tercih edilmedi.
- F zinciri + F Gram kalemi ayrı ayrı zamanlanmadı (tek kalem).
- Hızlanma yalnız lineer cebir çekirdekleri ve kopya atlama ile; formül, λ, F boyutları, veri ve okuma değişmedi. Yeni model/mimari/hiperparametre araması yok.
- V31 ile yeni sürüm arasındaki 14,6 s ↔ 12,97 s fark: farklı koşular arası dalgalanma (V32 denetiminde özellik üretimi 1,3–2,2 s oynadı).

## 8. Komutlar ve dosyalar

Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 cpp/tek_hamle_v33.cpp -o tek_hamle_v33 -L<scipy.libs> -l:libscipy_openblas-5f890258.so -Wl,-rpath,<scipy.libs>`. Eski/yeni: `V33_BLAS=0|1 ./tek_hamle_v33 2 512 1 1 1 0.5 Z 1 Y - 100 100 3000` (`V33_DUMP=önek` ağırlıkları yazar; `V33_BLAS_THREADS` OpenBLAS iplik sayısı). Ham loglar: `v33_ham/*.txt`, `manifest.tsv`.""")
open(base+'TEK_HAMLE_V33_RAPOR.md','w').write('\n'.join(o)+'\n')
