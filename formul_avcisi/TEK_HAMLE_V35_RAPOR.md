# TEK HAMLE V35 — İç ağırlıkları hedef bilgisiyle hesapla (SONUÇ: İYİLEŞME YOK)

Kod: `cpp/tek_hamle_v35.cpp` (V34 yolu aynen duruyor; `V35_TGT=1` ile yeni yol açılır, `V35_YSHUF=1` etiket-karıştırma kontrolü, `V35_RANK=1` rank tanısı). Ham çıktılar: `v35_ham/`. Aynı bölünme, aynı protokol (c=2, H=4×128, tohum 1, λ_C=100, ID ve 4002 blok aynı, son çözüm aynı birleşik Ridge). Test hiçbir seçimde kullanılmadı.

## Ne yapıldı
- Her F aşaması için girdi (X, seyrek ID/çift-ID, F2–F4'te önceki F'ler + X) **merkezlendi**, `M = (I−μ)ᵀR` kuruldu (R = gerçek sonraki-kelime one-hot'u; F2–F4'te R = Y − önceki aşamaların kümülatif Ridge tahmini). M'nin ilk 128 sol tekil vektörü iç yön olarak alındı, her yön eğitim projeksiyonunun std'sine bölündü (beyazlatma), bias = −ortalama/std, `tanh(γ·(I·W+b))`.
- SVD: M (≈8300×2001) için tam SVD yerine **randomize alt-uzay yinelemesi** (sketch l=138, 3 güç adımı, MGS, küçük `dsyev`). Sketch'teki rastgele matris yalnız SVD'yi hesaplamak için; yönler M'nin tekil vektörleri, rastgele doldurma yok. Gradyan/Adam/geri yayılım/döngüsel güncelleme yok. Aşama Ridge'leri (λ=1000) tek çözüm, artık hesabı için.
- Rank sınırı: M'nin satır sayısı 8290–8674 ve V=2001 → üst sınır 2001 ≥ 128; SVD ranki F genişliğini hiçbir aşamada kısıtlamadı.

## Sonuçlar (test; seçim yalnız doğrulamada)
| Yöntem | doğr. PPL | test PPL | Top-1 | Top-5 |
|---|---|---|---|---|
| V34 4F (rastgele çift yönleri)+ID, λ_F=3000 | 147.0 | **139.78** | 0.1560 | 0.3568 |
| V35 hedef-SVD, γ=1/0.5, λ_F=3000 | 158.7 | 151.31 | 0.1600 | 0.3546 |
| V35 γ=0.25/0.25, λ_F=30000 | 149.2 | 141.32 | 0.1618 | 0.3608 |
| **V35 seçilen (γ=0.1/0.1, λ_F=10000; doğr. CE en düşük)** | 148.9 | **140.91** | 0.1580 | 0.3606 |
| Kontrol: aynı ayar, etiketler KARIŞIK ile iç yönler | 148.4 | 140.43 | 0.1602 | 0.3606 |
| (referans) yalnız ID, F kapalı | 153.2 | 144.49 | 0.1554 | 0.3556 |
(Not: önceki "ID+çift ID 133.2" tablosu farklı bir hazırlık/protokoldü; bu koşuda kendi ID-only ölçümüm 144.49. 133.2 ile doğrudan aynı koşul olarak karşılaştırmadım.)

γ grid'i (0.1, 0.25, 0.5, 1) ve λ_F (100…300000) doğrulamada tarandı; en iyi doğrulama CE γ=0.1 uçta (grid kenarı, daha da küçük γ denenmedi).

## Süre (tek hamlede ağırlık üretimi, 4 çekirdek, sıcak)
- V34: F zinciri+Gram ≈ 1.1–1.2 s; Cholesky/Schur hazırlığı ≈ 1.3–2.5 s (gürültülü).
- V35: F üretimi ≈ **8.0–9.3 s** (M kurma çift duyarlık 8300×2001, randomize SVD, naif MGS, aşama Ridge'leri); Cholesky/Schur aynı ≈ 2.1–3.1 s.
- Sonuç: V35 ≈ 3–4× **daha yavaş**; optimize edilmedi (yakın zamanda M'yi float yapmak/MGS yerine BLAS-QR ile kısaltılabilir, bunu denemedim).

## Rank / fark tanısı (V35_RANK, γ=1 koşusu; merkezli F Gram'ı)
| | F1 | F2 | F3 | F4 |
|---|---|---|---|---|
| V34 sayısal rank (>1e-6·max) | 128 | 128 | 125 | 117 |
| V34 efektif rank (PR) | 4.9 | 2.0 | 1.4 | 1.2 |
| V35 sayısal rank | 128 | 128 | 128 | 128 |
| V35 efektif rank (PR) | 80.0 | 59.9 | 48.9 | 53.3 |
V34'te F2–F4 neredeyse tek boyuta çökerken (PR 1.2–2.0) V35 özellikleri çok daha çeşitli. Ama bu çeşitlilik kaliteye dönüşmedi. (γ=0.1 için rank ölçülmedi.)

## Y bilgisi gerçekten kullanıldı mı?
- **Mekanik olarak evet**: M = (I−μ)ᵀ(Y−Zcum) yalnız eğitim etiketlerinden; doğrulama/test etiketi hiçbir yerde girmedi (R eğitim satırlarında, seçim doğrulama CE'si ile).
- **Faydalı olarak ölçülemedi**: etiketleri karıştırıp aynı işlemi yapınca sonuç aynı veya biraz iyi (γ=0.1'de test 140.43 vs 140.91; doğr. 148.4 vs 148.9; γ=1'de 153.1 vs 151.3, fark 1.8 PPL). Yani V35'in seçilen konfigürasyonundaki iç yönlerin *hedefe duyarlı olmasından* ölçülebilir bir kazanç yok [Kesin, tek tohum, tek karıştırma]. Gözlenen şey, girdi uzayında yüksek enerjili yönler + düşük γ ile yarı-lineer özelliklerdir [Tahmin].

## Dürüst sonuç
- Kalite: V34'ü geçmedi (139.78 → 140.91 test, doğrulamada da kötü: 147.0 → 148.9). Top-1 ve Top-5 biraz yüksek (0.1580/0.3606 vs 0.1560/0.3568) ama PPL kaybı var; seçim kriteri doğrulama CE olduğundan sonucu "iyileşti" diye sunmuyorum.
- Hız: daha yavaş (~4×), optimizasyonsuz.
- Açık sorular/kusurlar: (1) F1'de M'nin sayısal ranki tam 128 çıktı (sketch'in 138 yönünden 10'unun özdeğeri ~0); bunun gerçek mi, sketch/MGS yapıntısı mı olduğunu **doğrulamadım**. (2) Tek tohum, tek karıştırma; (3) γ grid kenarı; (4) aşama Ridge λ=1000 ayarlanmadı; (5) artık R'ye ID belleği (C bloğu) dahil edilmedi — yönler ID'nin zaten açıkladığı şeyi tekrar edebilir; (6) "ID+çift ID 133.2" ile aynı koşul karşılaştırması yapılmadı.
- Sonuç: TopSVD(XᵀY) ilk adayı bu hâliyle işe yaramadı; V34 varsayılan kalıyor.
