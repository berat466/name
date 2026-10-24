# TEK HAMLE V48 — Bağlam hafızasıyla tek hamle öğrenme (SONUÇ: İYİLEŞME YOK)

Kod: `cpp/tek_hamle_v48.cpp` (V46 kopyası; `V48=1`; V46 ve öncesi dokunulmadı). Ham çıktılar: `v48_ham/` (`L2_real`, `L4_real`, `L8_real`, `L4_shuf`). Sistem V46 ile aynı: tek F×512, bağlantılı havuz β=0.9, C0 kelime+çift kimliği, intercept, gruplu Ridge, λ_C=85, λ_F=1000, tek geçiş; tohum 1. Doğrulama/test yalnız eğitim hafızasına erişir; seçim yalnız doğrulama CE.

## Yöntem
Hedef: `Y* = (1−a−b)Y + a·Q_havuz + b·Q_bağlam` (a,b≥0, toplam ≤1); ağırlıklar tek çözümde `W=(1−a−b)W_Y + a·W_Qhavuz + b·W_Qbağlam` (Ridge doğrusal; Zᵀ Y* biriktirilir, n×V hedef matrisi saklanmaz, 2048'lik parçalarla akıtılır).
**Q_bağlam (bağlam hafızası):** eğitim token dizisinden (`ids_train.npy`; satır eşleşmesi python ile 3 bölmede doğrulandı) her satır için önceki L kelime alınır. Hafıza: her mesafe k=1..L için `T_k[w][y]` (kelime w, k kelime sonra y geldi) sayımları. `Q_i = Σ_k β_k·P_k(y | w_{t−k})`, β_k ∝ 1/k, **LOO**: satırın kendi (w_k, y_i) sayımı çıkarılır; komşu/benzerlik yalnız giriş bağlamı kelimeleri üzerinden (hedef kullanılmaz); doğrulama/test yalnız eğitim tablosunu kullanır. L ∈ {2,4,8}. Bu, "k mesafedeki ortak kelimeyi paylaşan eğitim bağlamlarının devamı" olarak yumuşak bir bağlam-kNN'dir; **X/F benzerliğine dayalı komşu seçimi ve dizi (n-gram) eşleşmeli kNN bu sürede yapılmadı.** Not: k=1 terimi V46'daki kendi-kelime LOO dağılımıdır (V46'da tek başına etkisizdi).
**Kontroller:** V46 aynı hatta yeniden ölçüldü; gerçek bağlam vs karıştırılmış bağlam (sorgu bağlamı rastgele başka satırdan, LOO yalnız kelime eşleşirse); yalnız havuz / yalnız bağlam / birleşik ayrı.

## Sonuçlar (seçim doğrulama CE ile; a∈{0,0.15,0.25,0.35}, b∈{0,0.1,0.2,0.3,0.4})
| Sistem | seçilen (a,b) | doğr. CE (PPL) | **test PPL** | Top-1 / Top-5 |
|---|---|---|---|---|
| Referans (a=b=0, V44) | 0, 0 | 4.8675 (130.0) | 124.27 | 0.1637 / 0.3691 |
| **V46 mevcut sistem (yeniden ölçüldü, yalnız havuz)** | 0.25, 0 | **4.8474 (127.4)** | **121.32** | 0.1606 / 0.3681 |
| Yalnız bağlam L=2 | 0, 0.1 | 4.8670 | 124.25 | 0.1631 / 0.3681 |
| Yalnız bağlam L=4 | 0, 0.1 | 4.8676 | 124.34 | 0.1629 / 0.3675 |
| Yalnız bağlam L=8 | 0, 0.1 | 4.8685 | 124.45 | 0.1627 / 0.3673 |
| Havuz+bağlam L=2 | 0.25, 0.1 | 4.8483 | 121.49 | 0.1608 / 0.3679 |
| Havuz+bağlam L=4 | 0.25, 0.1 | 4.8492 | 121.64 | 0.1610 / 0.3681 |
| Havuz+bağlam L=8 | 0.25, 0.1 | 4.8502 | 121.80 | 0.1612 / 0.3675 |
| Karıştırılmış bağlam L=4, yalnız bağlam | 0, 0.1 | 4.8707 | 124.75 | 0.1629 / 0.3673 |
| Karıştırılmış bağlam L=4, havuz+bağlam | 0.25, 0.1 | 4.8538 | 122.33 | 0.1614 / 0.3673 |
Ayrıntı (L=4, doğr. CE): yalnız bağlam b=0/0.1/0.2/0.3/0.4 → 4.8675/4.8676/4.8681/4.8707/4.8748; a=0.25 ile b=0/0.1/0.2/0.3/0.4 → 4.8474/4.8492/4.8527/4.8589/4.8680. Karışık bağlamda b arttıkça çok daha hızlı kötüleşiyor (a=0.25: b=0.2 → 4.8691, b=0.4 → 4.9293).

## Süre ve bellek (CPU, 4 vCPU; gürültülü)
- V46 havuz sinyali (Q, Y0, U): 0.65–2.1 s. Bağlam hafızası: tablo + satır başına Q + FᵀQ **1.9–2.6 s** (yalnız komşu/Q hesabı 1.25–2.05 s; L=2: 1.25 s, L=4: 1.3–1.6 s, L=8: 2.05 s) + Y0,U 0.5–0.7 s. Yani bağlam sinyali tek başına ≈2.5–3 s ek maliyet (V46 toplamı ≈2.3–2.6 s → ≈5 s). Bu, ham n×V Q satırlarını akıtan basit gerçekleme; optimize edilmedi.
- Hafıza tabloları: 32/64/128 MB (L=2/4/8, V=2001, float). Tepe RSS 1.45–1.54 GB (V46 + V48; kaynak: `getrusage`).
- Komşu bulma ayrı bir aşama değil (mesafe-k tablo erişimi); ayrı süresi yukarıdaki "komşu/Q hesabı".

## Değerlendirme (dürüst)
- **Başarı yok:** V46 (121.32) aynı hatta yeniden üretildi (doğr. CE 4.8474). Bağlam sinyali hiçbir L'de (2/4/8) V46'yı geçmedi; en iyi b=0.1'de bile yalnız-havuza göre doğrulama CE'si kötüleşiyor (4.8492 vs 4.8474), test +0.17…+0.48 PPL kötü. **Yalnız bağlam**, referanstan (124.27) ölçülebilir fark vermiyor (124.25–124.45; doğr. CE 4.8670–4.8685): kazanç yok.
- **Gerçek vs karışık bağlam:** gerçek bağlam karışıktan daha az zararlı (L=4, havuz+bağlam b=0.1: 121.64 vs 122.33; b=0.4'te 4.8680 vs 4.9293), yani hafızadaki mesafe-k ilişkileri bir miktar gerçek bilgi taşıyor; ama bu bilgi, havuz sinyaline ya da ağırlıklara faydalı biçimde eklenemeyecek kadar zayıf/zaten C bloğundaki kelime+çift kimliği tarafından kapsanmış [Muhtemel]. L büyüdükçe (2→8) bir miktar daha kötü: uzun bağlamı ağırlıklı ortalamaya koymak sinyali seyreltiyor.
- **Neden olabilir:** bag-of-context (mesafe-k marjinalleri) sıralı bilgiyi (hangi kelimeler hangi sırayla) taşımaz; en bilgilendirici k=1,2 terimleri zaten kimlik + çift kimliğiyle Ridge'de var. Dizi eşleşmesi (4-gram/8-gram tam eşleşme) ve X/F benzerlikli komşu seçimi **denenmedi**; bu sonuç onlar için olumsuz kanıt değildir.
- **V46 dolaylı sızıntı kontrolü:** yapılmadı (zaman). Not: V46'nın kazancı doğrulama/test ölçümlerinde görüldüğü için gerçek hold-out kazançtır (eğitim hedefindeki olası ikinci-dereceden bağımlılık yalnız eğitim kalitesini etkileyebilir). Temiz çapraz-uydurma (grafı/P_v'yi diğer yarıdan al) denemesi önerilir.
- **Tahmin sırasında kNN olasılığı ekleme kontrolü:** yapılmadı.

## Sonuç
Geçmiş kelime dizilerinin mesafe-bazlı ilişkilerini ortak ağırlık formülüne eklemek (bu biçimiyle) mevcut kelime havuzundan **daha güçlü öğrenme sağlamadı**; V46 (121.32) en iyi sistem olarak kalıyor. Başarı iddia edilmiyor.
