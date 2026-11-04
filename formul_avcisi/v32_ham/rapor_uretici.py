import json,numpy as np
base='/home/user/name/formul_avcisi/'
R=[json.loads(l) for l in open(base+'v32_ham/klasik_sonuclar.jsonl')]
sel={r['model']:r for r in R if r['asama']=='secilen'}; res={}
for r in R:
    if r['asama']=='sonuc': res.setdefault(r['model'],[]).append(r)
grid={}
for r in R:
    if r['asama']=='secim': grid.setdefault(r['model'],[]).append(r)
def ms(k,f): v=[f(r) for r in res[k]]; return np.mean(v),np.std(v)
NM={'id':'MLP · sıra duyarlı 2 kelime kimliği (öğrenilen gömme)','X':'MLP · sabit PPMI-SVD X (bizim girdi, kimlik yok)','id_X':'MLP · kimlik + X (V31 ile aynı bilgi)','lstm':'LSTM · blok içi tam geçmiş (daha fazla bağlam)'}
o=[];P=o.append
P('# TEK HAMLE V32 — Klasik (gradyanla eğitilmiş) yapay zekâ ile karşılaştırma\n')
P('Kod `klasik_baz_v32.py` (PyTorch, CPU, 4 çekirdek), ham sonuçlar/log `v32_ham/`. Bu çalışma **bizim yöntemlerimiz dışında**, klasik gradyan eğitimi (AdamW, erken durdurma) ile aynı veri üzerinde baz çizgisi çıkarır. Önceki sürümlerdeki "Adam yok" kuralı bizim ağırlık üretim algoritmamız içindi; bu raporun amacı tam olarak o kuralı kullanmayan klasik yöntemle ölçmektir.\n')
P('''## 1. Protokol (adil karşılaştırma koşulları)

- **Aynı veri:** Pride and Prejudice ilk 50 000 token, V=2001, 80/10/10 blok bölünmesi (seed 0), değerlendirme konumları: her 500 tokenlık blokta t≥2 → eğitim 39 840 / doğrulama 4 980 / test 4 980 örnek. Betik, `c2_*_ctx/y` dosyalarının blok konumlarıyla **birebir aynı** olduğunu doğrular (`HIZALAMA` satırı, assert).
- **Ölçüt:** PPL = exp(ortalama CE), saf softmax (kalibrasyon yok; klasik modelin kendi olasılığı). Hiperparametre ve erken durdurma yalnız **doğrulama CE** ile; test yalnız seçilen modelde, her tohumda bir kez.
- **Tohum:** 3 (ağırlık başlangıcı/mini-batch sırası); seçim tohum 0 doğrulamasında.
- **Modeller:** (1) *MLP-id*: iki önceki kelimenin kimlikleri (sıra duyarlı) → öğrenilen gömme → gizli katman; (2) *MLP-X*: bizim sabit PPMI-SVD X girdisi (192-d), kimlik yok; (3) *MLP-id+X*: V31/V30 ile aynı bilgi; (4) *LSTM*: aynı bloklarda tüm geçmişi görür (**bağlam 2 kelimeyle sınırlı değil**; MLP\'lerden ve bizim yöntemlerden daha fazla bilgi).
- Izgara küçük (MLP: 6 konfigürasyon, LSTM: 4); AdamW, lr 1e-3…3e-3, dropout 0,3–0,5, weight decay 0,01, patience 6.
''')
P('## 2. Klasik baz çizgileri (test; 3 tohum ort. ± std)\n')
P('| model | seçilen yapı (doğrulamayla) | test PPL | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim süresi (s) | parametre |\n|---|---|---|---|---|---|---|---|---|')
for k in ('id','X','id_X','lstm'):
    c=sel[k]['cfg']; a,s=ms(k,lambda r:r['test_PPL'])
    desc=f"emb={c['emb']}, hid={c['hid']}, drop={c['drop']}" if k!='X' else f"hid={c['hid']}, drop={c['drop']}"
    P(f"| {NM[k]} | {desc} | **{a:.1f} ± {s:.1f}** | {100*ms(k,lambda r:r['top1'])[0]:.1f}% | {100*ms(k,lambda r:r['top5'])[0]:.1f}% | {ms(k,lambda r:r['egitim_PPL'])[0]:.0f} | {ms(k,lambda r:r['dogr_PPL'])[0]:.0f} | {ms(k,lambda r:r['egitim_suresi_s'])[0]:.0f} | {res[k][0]['parametre']:,} |")
P('')
P('Konfigürasyon taraması (tohum 0, doğrulama PPL):\n')
for k in ('id','X','id_X','lstm'):
    P(f"- {NM[k]}: "+'; '.join(f"{g['cfg'].get('emb','-')}/{g['cfg']['hid']}/dr{g['cfg']['drop']}/lr{g['cfg']['lr']} → {g['dogr_PPL']:.1f}" for g in grid[k]))
P('')
P('## 3. Bizim yöntemlerimizle yan yana (test PPL; hepsi aynı bölünme)\n')
ours=[('Trigram n-gram (EM enterpolasyon)','138,9','sayım tabanlı, V24'),('V31: kimlik + 4F ortak çözüm (H=512)','140,3','3 tohum, gradyansız, bu oturum'),('V31: yalnız kimlik C (çiftsiz) ridge','144,1','tohumdan bağımsız'),('V30: 4F + kimlik (F içinde)','153,4','3 tohum'),('V28: 4F (X bağlantılı), kimliksiz, ortak ridge','176,5','3 tohum'),('V26: tek F (H=512), doğrusal okuma','182,5','3 tohum'),('V26: tek F (H=1024)','192,2','3 tohum')]
rows=[(n,v,f) for n,v,f in ours]
for k in ('lstm','id_X','X','id'):
    a,s=ms(k,lambda r:r['test_PPL']); rows.append((NM[k]+' (klasik, AdamW)',f'{a:.1f}'.replace('.',','),'3 tohum, bu rapor'))
def key(r): return float(r[1].replace(',','.'))
P('| sıra | model | test PPL | not |\n|---|---|---|---|')
for i,(n,v,f) in enumerate(sorted(rows,key=key),1): P(f'| {i} | {n} | **{v}** | {f} |')
P('')
a_id=ms('id_X',lambda r:r['test_PPL'])[0]; a_X=ms('X',lambda r:r['test_PPL'])[0]; a_l=ms('lstm',lambda r:r['test_PPL'])[0]; a_i=ms('id',lambda r:r['test_PPL'])[0]
P('## 4. Bulgular\n')
P(f'- [Kesin] **Aynı bilgiyle klasik eğitim bizim yöntemimizden belirgin iyi.** Kimlik+X (iki kelime bağlamı): klasik MLP **{a_id:.1f}** vs V31 140,3 (**%{100*(140.3-a_id)/140.3:.0f} daha düşük PPL**). Yalnız X (kimliksiz): klasik MLP **{a_X:.1f}** vs bizim en iyi kimliksiz sistemimiz 176,5 (4F ortak ridge) — **%{100*(176.5-a_X)/176.5:.0f} daha düşük**. Yani "rastgele özellik + ridge" yaklaşımı, aynı girdi bilgisinden gradyanla eğitilen küçük bir MLP kadar bilgi çıkaramıyor.')
P(f'- [Kesin] **Klasik MLP (yalnız 2 kelime kimliği) {a_i:.1f}**: trigram n-gram (138,9) ve V31 (140,3) ile aynı seviye. Dolayısıyla V31\'in 140\'ı klasik 2-kelime-bağlam sınıfının **alt ucunda**; ona eşit değil, üstünde değil.')
P(f'- [Kesin] **Daha fazla bağlam (LSTM, tam blok geçmişi): {a_l:.1f}** — en iyi sonuç; bizim en iyisinden %{100*(140.3-a_l)/140.3:.0f} daha düşük. Bu satır 2 kelimeyle sınırlı değil; adil karşılaştırma için MLP satırlarına bakılmalı, LSTM ise "klasik yöntemle ulaşılabilecek" tavanı gösterir.')
P('- [Kesin] Top-1/Top-5: LSTM %16,9 / %37,0; MLP\'ler %14–15 / %34–35; bizim V31 %15,6 / %35,8. Sıralama doğruluğunda fark küçük, fark PPL (olasılık kalitesi) tarafında.')
P('- [Kesin] **Hız:** klasik eğitim bu veride zaten kısa (MLP 13–25 s, LSTM ≈65–70 s, CPU). Bizim V31 hattı: F zinciri+Gram ≈1,3 s, C-blok Cholesky ≈10 s, λ taraması dahil tek tohum ≈40–60 s. **Gradyansız yöntem burada belirgin bir hız avantajı sağlamıyor.**')
P('- [Muhtemel] Aşırı uyum her iki tarafta var (eğitim PPL 45–70 vs test 108–137); klasik modeller erken durdurma+dropout ile daha iyi genelliyor. Bizim yöntemde λ/ölçek seçimi benzer işi yapıyor ama sonuç daha zayıf.')
P('- [Tahmin] Daha iyi ayarlanmış klasik modeller (daha geniş ızgara, ağırlık bağlama, daha büyük bağlam) bu sayıları daha da düşürür; bu rapordaki klasik değerler **alt sınırdır (kolay baz çizgisi)**, üst sınır değil.\n')
P('## 5. Puanlama (1–100; klasik, ayarlanmış gradyan eğitimine göre)\n')
P('Önceki sohbetteki tahminim (≈30/100 kalite) bu ölçümle **aşağı** revize edilir: o tahmin klasik sinir ağının n-gram seviyesinde kalacağını varsayıyordu; ölçüm klasik MLP\'nin aynı girdilerle %12–30 daha iyi, LSTM\'in ≈%23 daha iyi olduğunu gösterdi.\n')
P(f'''| Boyut | Puan | Gerekçe (ölçüm) |
|---|---|---|
| Nihai tahmin kalitesi (PPL) | **≈ 20** | En iyi V31 140,3; aynı bilgiyle klasik MLP {a_id:.0f}, LSTM {a_l:.0f}. Skor oranı: en iyi klasik/ bizim ≈ {a_l/140.3:.2f} |
| Aynı bilgiden bilgi çıkarma (X, kimliksiz) | **≈ 15** | Klasik MLP-X {a_X:.0f} vs bizim 176,5 (≈%{100*(176.5-a_X)/176.5:.0f} fark) |
| Verimlilik (gradyansız, tek hamle) | **≈ 45** | Epok/Adam yok ve çözüm doğrudan; ancak bu küçük veride klasik eğitim de saniyeler sürüyor, hız farkı yok |
| Bilimsel titizlik / şeffaflık | **≈ 65** | Doğrulama/test ayrımı, tohumlar, hata ve etiket düzeltmeleri raporlandı; ama baz çizgisi çok geç ölçüldü |
| **Genel** | **≈ 25** | "Klasik n-gram seviyesine ulaşan, ancak aynı bilgiyle gradyanla eğitilmiş küçük bir sinir ağından belirgin geri kalan, gradyansız bir yöntem" |
''')
P('- Bu puanlar benim değerlendirmemdir ([Tahmin] niteliğinde); dayanak ölçümler tablo 2–3\'tür.\n')
P('## 6. Sınırlamalar\n')
P('''- Küçük hiperparametre ızgarası (MLP 6, LSTM 4 yapı); tek veri seti; test n=4 980. Tohum varyansı yalnız başlangıç/mini-batch sırası (MLP std ≈ 1,5–2 PPL).
- LSTM, MLP ve bizim yöntemlerden daha uzun bağlam görüyor; "aynı bilgi" karşılaştırması yalnız MLP-id, MLP-X, MLP-id+X satırları için geçerli.
- Klasik modellerde PPL saf softmax ile; bizim yöntemlerde V26 doğrusal olasılık okuması (doğrulamayla ayarlı) kullanıldı. Bu, bizim lehimize bir serbestlik; yine de klasik tarafı geçemedik.
- Bizim yöntemlerin sayıları önceki raporlardan alındı (aynı bölünme/protokol); bu oturumda yeniden koşturulmadı.
- Eski sürümlerin kodlarına dokunulmadı; yeni yapı ana mimariye eklenmedi.
''')
P('## 7. Çalıştırma\n\n`python3 klasik_baz_v32.py id X id_X lstm` (PyTorch 2.14 CPU). Ham sonuçlar: `v32_ham/klasik_sonuclar.jsonl`, log: `v32_ham/klasik_log.txt`.')
open(base+'TEK_HAMLE_V32_RAPOR.md','w').write('\n'.join(o)+'\n')
