# TEK HAMLE V32 — Klasik (gradyanla eğitilmiş) yapay zekâ ile karşılaştırma

Kod `klasik_baz_v32.py` (PyTorch, CPU, 4 çekirdek), ham sonuçlar/log `v32_ham/`. Bu çalışma **bizim yöntemlerimiz dışında**, klasik gradyan eğitimi (AdamW, erken durdurma) ile aynı veri üzerinde baz çizgisi çıkarır. Önceki sürümlerdeki "Adam yok" kuralı bizim ağırlık üretim algoritmamız içindi; bu raporun amacı tam olarak o kuralı kullanmayan klasik yöntemle ölçmektir.

## 1. Protokol (adil karşılaştırma koşulları)

- **Aynı veri:** Pride and Prejudice ilk 50 000 token, V=2001, 80/10/10 blok bölünmesi (seed 0), değerlendirme konumları: her 500 tokenlık blokta t≥2 → eğitim 39 840 / doğrulama 4 980 / test 4 980 örnek. Betik, `c2_*_ctx/y` dosyalarının blok konumlarıyla **birebir aynı** olduğunu doğrular (`HIZALAMA` satırı, assert).
- **Ölçüt:** PPL = exp(ortalama CE), saf softmax (kalibrasyon yok; klasik modelin kendi olasılığı). Hiperparametre ve erken durdurma yalnız **doğrulama CE** ile; test yalnız seçilen modelde, her tohumda bir kez.
- **Tohum:** 3 (ağırlık başlangıcı/mini-batch sırası); seçim tohum 0 doğrulamasında.
- **Modeller:** (1) *MLP-id*: iki önceki kelimenin kimlikleri (sıra duyarlı) → öğrenilen gömme → gizli katman; (2) *MLP-X*: bizim sabit PPMI-SVD X girdisi (192-d), kimlik yok; (3) *MLP-id+X*: V31/V30 ile aynı bilgi; (4) *LSTM*: aynı bloklarda tüm geçmişi görür (**bağlam 2 kelimeyle sınırlı değil**; MLP'lerden ve bizim yöntemlerden daha fazla bilgi).
- Izgara küçük (MLP: 6 konfigürasyon, LSTM: 4); AdamW, lr 1e-3…3e-3, dropout 0,3–0,5, weight decay 0,01, patience 6.

## 2. Klasik baz çizgileri (test; 3 tohum ort. ± std)

| model | seçilen yapı (doğrulamayla) | test PPL | Top-1 | Top-5 | eğitim PPL | doğr. PPL | eğitim süresi (s) | parametre |
|---|---|---|---|---|---|---|---|---|
| MLP · sıra duyarlı 2 kelime kimliği (öğrenilen gömme) | emb=128, hid=256, drop=0.5 | **136.1 ± 1.5** | 14.3% | 34.4% | 62 | 142 | 24 | 836,177 |
| MLP · sabit PPMI-SVD X (bizim girdi, kimlik yok) | hid=256, drop=0.3 | **125.1 ± 0.4** | 14.5% | 34.4% | 69 | 130 | 15 | 547,281 |
| MLP · kimlik + X (V31 ile aynı bilgi) | emb=32, hid=256, drop=0.3 | **122.9 ± 1.4** | 14.9% | 35.0% | 54 | 127 | 17 | 627,697 |
| LSTM · blok içi tam geçmiş (daha fazla bağlam) | emb=128, hid=256, drop=0.3 | **108.4 ± 0.7** | 16.9% | 37.0% | 49 | 114 | 67 | 1,165,649 |

Konfigürasyon taraması (tohum 0, doğrulama PPL):

- MLP · sıra duyarlı 2 kelime kimliği (öğrenilen gömme): 64/256/dr0.3/lr0.002 → 144.8; 64/512/dr0.5/lr0.002 → 164.2; 128/512/dr0.5/lr0.001 → 143.0; 128/256/dr0.5/lr0.002 → 140.8; 64/1024/dr0.5/lr0.001 → 172.9; 32/256/dr0.3/lr0.002 → 152.1
- MLP · sabit PPMI-SVD X (bizim girdi, kimlik yok): 64/256/dr0.3/lr0.002 → 129.7; 64/512/dr0.5/lr0.002 → 134.5; 64/1024/dr0.5/lr0.001 → 132.8
- MLP · kimlik + X (V31 ile aynı bilgi): 64/256/dr0.3/lr0.002 → 129.6; 64/512/dr0.5/lr0.002 → 139.7; 128/512/dr0.5/lr0.001 → 129.9; 128/256/dr0.5/lr0.002 → 130.1; 64/1024/dr0.5/lr0.001 → 141.2; 32/256/dr0.3/lr0.002 → 128.4
- LSTM · blok içi tam geçmiş (daha fazla bağlam): 128/256/dr0.3/lr0.002 → 112.1; 128/256/dr0.5/lr0.002 → 112.6; 64/128/dr0.3/lr0.003 → 115.7; 128/512/dr0.5/lr0.001 → 116.6

## 3. Bizim yöntemlerimizle yan yana (test PPL; hepsi aynı bölünme)

| sıra | model | test PPL | not |
|---|---|---|---|
| 1 | LSTM · blok içi tam geçmiş (daha fazla bağlam) (klasik, AdamW) | **108,4** | 3 tohum, bu rapor |
| 2 | MLP · kimlik + X (V31 ile aynı bilgi) (klasik, AdamW) | **122,9** | 3 tohum, bu rapor |
| 3 | MLP · sabit PPMI-SVD X (bizim girdi, kimlik yok) (klasik, AdamW) | **125,1** | 3 tohum, bu rapor |
| 4 | MLP · sıra duyarlı 2 kelime kimliği (öğrenilen gömme) (klasik, AdamW) | **136,1** | 3 tohum, bu rapor |
| 5 | Trigram n-gram (EM enterpolasyon) | **138,9** | sayım tabanlı, V24 |
| 6 | V31: kimlik + 4F ortak çözüm (H=512) | **140,3** | 3 tohum, gradyansız, bu oturum |
| 7 | V31: yalnız kimlik C (çiftsiz) ridge | **144,1** | tohumdan bağımsız |
| 8 | V30: 4F + kimlik (F içinde) | **153,4** | 3 tohum |
| 9 | V28: 4F (X bağlantılı), kimliksiz, ortak ridge | **176,5** | 3 tohum |
| 10 | V26: tek F (H=512), doğrusal okuma | **182,5** | 3 tohum |
| 11 | V26: tek F (H=1024) | **192,2** | 3 tohum |

## 4. Bulgular

- [Kesin] **Aynı bilgiyle klasik eğitim bizim yöntemimizden belirgin iyi.** Kimlik+X (iki kelime bağlamı): klasik MLP **122.9** vs V31 140,3 (**%12 daha düşük PPL**). Yalnız X (kimliksiz): klasik MLP **125.1** vs bizim en iyi kimliksiz sistemimiz 176,5 (4F ortak ridge) — **%29 daha düşük**. Yani "rastgele özellik + ridge" yaklaşımı, aynı girdi bilgisinden gradyanla eğitilen küçük bir MLP kadar bilgi çıkaramıyor.
- [Kesin] **Klasik MLP (yalnız 2 kelime kimliği) 136.1**: trigram n-gram (138,9) ve V31 (140,3) ile aynı seviye. Dolayısıyla V31'in 140'ı klasik 2-kelime-bağlam sınıfının **alt ucunda**; ona eşit değil, üstünde değil.
- [Kesin] **Daha fazla bağlam (LSTM, tam blok geçmişi): 108.4** — en iyi sonuç; bizim en iyisinden %23 daha düşük. Bu satır 2 kelimeyle sınırlı değil; adil karşılaştırma için MLP satırlarına bakılmalı, LSTM ise "klasik yöntemle ulaşılabilecek" tavanı gösterir.
- [Kesin] Top-1/Top-5: LSTM %16,9 / %37,0; MLP'ler %14–15 / %34–35; bizim V31 %15,6 / %35,8. Sıralama doğruluğunda fark küçük, fark PPL (olasılık kalitesi) tarafında.
- [Kesin] **Hız:** klasik eğitim bu veride zaten kısa (MLP 13–25 s, LSTM ≈65–70 s, CPU). Bizim V31 hattı: F zinciri+Gram ≈1,3 s, C-blok Cholesky ≈10 s, λ taraması dahil tek tohum ≈40–60 s. **Gradyansız yöntem burada belirgin bir hız avantajı sağlamıyor.**
- [Muhtemel] Aşırı uyum her iki tarafta var (eğitim PPL 45–70 vs test 108–137); klasik modeller erken durdurma+dropout ile daha iyi genelliyor. Bizim yöntemde λ/ölçek seçimi benzer işi yapıyor ama sonuç daha zayıf.
- [Tahmin] Daha iyi ayarlanmış klasik modeller (daha geniş ızgara, ağırlık bağlama, daha büyük bağlam) bu sayıları daha da düşürür; bu rapordaki klasik değerler **alt sınırdır (kolay baz çizgisi)**, üst sınır değil.

## 5. Puanlama (1–100; klasik, ayarlanmış gradyan eğitimine göre)

Önceki sohbetteki tahminim (≈30/100 kalite) bu ölçümle **aşağı** revize edilir: o tahmin klasik sinir ağının n-gram seviyesinde kalacağını varsayıyordu; ölçüm klasik MLP'nin aynı girdilerle %12–30 daha iyi, LSTM'in ≈%23 daha iyi olduğunu gösterdi.

| Boyut | Puan | Gerekçe (ölçüm) |
|---|---|---|
| Nihai tahmin kalitesi (PPL) | **≈ 20** | En iyi V31 140,3; aynı bilgiyle klasik MLP 123, LSTM 108. Skor oranı: en iyi klasik/ bizim ≈ 0.77 |
| Aynı bilgiden bilgi çıkarma (X, kimliksiz) | **≈ 15** | Klasik MLP-X 125 vs bizim 176,5 (≈%29 fark) |
| Verimlilik (gradyansız, tek hamle) | **≈ 45** | Epok/Adam yok ve çözüm doğrudan; ancak bu küçük veride klasik eğitim de saniyeler sürüyor, hız farkı yok |
| Bilimsel titizlik / şeffaflık | **≈ 65** | Doğrulama/test ayrımı, tohumlar, hata ve etiket düzeltmeleri raporlandı; ama baz çizgisi çok geç ölçüldü |
| **Genel** | **≈ 25** | "Klasik n-gram seviyesine ulaşan, ancak aynı bilgiyle gradyanla eğitilmiş küçük bir sinir ağından belirgin geri kalan, gradyansız bir yöntem" |

- Bu puanlar benim değerlendirmemdir ([Tahmin] niteliğinde); dayanak ölçümler tablo 2–3'tür.

## 6. Sınırlamalar

- Küçük hiperparametre ızgarası (MLP 6, LSTM 4 yapı); tek veri seti; test n=4 980. Tohum varyansı yalnız başlangıç/mini-batch sırası (MLP std ≈ 1,5–2 PPL).
- LSTM, MLP ve bizim yöntemlerden daha uzun bağlam görüyor; "aynı bilgi" karşılaştırması yalnız MLP-id, MLP-X, MLP-id+X satırları için geçerli.
- Klasik modellerde PPL saf softmax ile; bizim yöntemlerde V26 doğrusal olasılık okuması (doğrulamayla ayarlı) kullanıldı. Bu, bizim lehimize bir serbestlik; yine de klasik tarafı geçemedik.
- Bizim yöntemlerin sayıları önceki raporlardan alındı (aynı bölünme/protokol); bu oturumda yeniden koşturulmadı.
- Eski sürümlerin kodlarına dokunulmadı; yeni yapı ana mimariye eklenmedi.

## 7. Çalıştırma

`python3 klasik_baz_v32.py id X id_X lstm` (PyTorch 2.14 CPU). Ham sonuçlar: `v32_ham/klasik_sonuclar.jsonl`, log: `v32_ham/klasik_log.txt`.
