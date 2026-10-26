# V74: otomatik çalışma ↔ Claude test akışı süre karşılaştırması

Aynı ikili (`tek_hamle_v74bin`, `tek_hamle_v73.cpp`'den bir kez derlendi; **derleme 35,4 s**, ayrıca, hiçbir çalışma süresine karıştırılmadı; kaynak sha256 `231906a66f151c9e`, ikili `51ed6b4b7b6c41c2`), aynı veri (10.001, V59 ortak), aynı betik `kos.sh` ve parametreler (V73 bayrakları: ARROW=2 IMPL SPQ COMPACT UCSC RHS SYRK; EVAL+TEST), aynı CPU/4 iş parçacığı. Sıra A1→B1→A2→B2→A3→B3, hiçbiri eşzamanlı değil. Kod ve hesap değişmedi. Dosyalar: `derle.sh`, `kos.sh`, `cagri.log`, `ozet.json`, `ham/`.
- **A (otomatik):** tek komut (`./kos.sh A_k`); betik başlangıç/bitişi kendi `date` ile ve `calistir.py` rusage ile kaydeder; çalışırken Claude hiçbir şey yapmaz (araç çağrısı bloklu bekler); sonuç özeti betik tarafından üretilir.
- **B (Claude testi):** iki araç çağrısı: (1) komutu çalıştır, (2) sonuç JSON'unu oku. Her çağrının başlangıç/bitişi zaman damgalıdır; **çağrılar arası boşluk = Claude'un düşünme/üretme + harness gecikmesi** (doğrudan ölçülebilen tek Claude süresi). Gerçek V73 oturumunda daha çok çağrı vardı (derleme, hash karşılaştırma, rapor yazma); burada yalnız çalıştır-ve-oku akışı ölçüldü.

## Sonuç tablosu (3 tekrar; medyan, [tekrarlar])
| Ölçüm | A: Otomatik | B: Claude testi |
|---|---|---|
| Program başlangıç → bitiş (yükleme+model+değerlendirme) | **51,3 s** [52,9; 51,3; 49,8] | **49,1 s** [49,8; 49,1; 49,0] |
| Model hazır süresi (`MODEL_HAZIR`) | 9,23 s [11,95; 9,23; 8,71] | 9,60 s [9,71; 8,12; 9,60] |
| Toplam iş akışı süresi (komut başı → sonuç elde) | **51,4 s** [53,0; 51,4; 49,8] | **59,3 s** [58,7; 59,3; 62,0] |
| Claude müdahale süresi (iş akışı − program) | 0 (çağrı içinde bekleme) | 8,8 / 10,2 / 13,1 s → medyan **10,2 s** (tamamı çağrılar arası boşluk: 8,8 / 10,1 / 13,0 s) |
| Tepe RAM (MAXRSS) | 2,80 GB | 2,80 GB |
| CPU kullanımı (user+sys / duvar) | 2,82 çekirdek | 2,88 çekirdek |
| Test PPL | 262,93 (3/3) | 262,93 (3/3) |
| Sonuç hash (VAL+TEST metrik satırları sha256) | `1024734e4731537c` (3/3) | `1024734e4731537c` (3/3) |
Val CE 5,441941 altı koşunun hepsinde aynı. **Ağırlık/logit karmaları bu görevde yeniden hesaplanmadı** (HASH bayrağı süreleri şişirirdi); V73'te aynı ikili/bayraklarla U, W_F, Zval karmaları V72 ile aynı bulunmuştu. Otomatik modun hiçbir hesabı atlamadığına kanıt: aynı betik, aynı ikili, aynı metrik satırı ve model-hazır çıktıları (her iki yolda `MODEL_HAZIR`, `V65|TEST` satırı üretildi).
Önbellek/ilk çalıştırma: ilk koşu (A1) en yavaş (52,9 s; hazır 11,95 s), sonrakiler 49–51 s; veri dosyaları 1. koşudan önce de önbellekteydi (önceki oturumlar), yine de A1'in +2–3 s'si dosya/CPU önbelleği ve VM dalgalanması. VM dalgalanması: program süresi aralığı 49,0–52,9 s (±4 %), `MODEL_HAZIR` 8,1–12,0 s (±20 %).

## Kesin cevaplar
1. **Claude müdahalesi olmadan model üretimi:** program 51,3 s, bunun içinde `MODEL_HAZIR` 9,2 s (kalan ≈ 42 s: veri okuma, değerlendirme, PPL).
2. **Claude testinde:** program 49,1 s (model hazır 9,6 s); toplam iş akışı 59,3 s.
3. **Toplam iş akışı farkı:** B − A = **+7,9 s** (59,3 − 51,4); medyanlar arası fark; tekrar bazında +5,7 / +7,9 / +12,2 s.
4. **Sonuçlar aynı mı?** Evet: test PPL 262,93, val CE 5,441941, metrik hash'i altı koşuda aynı (ağırlık karması ölçülmedi).
5. **Fark kaynağı:** Program süreleri farklılığı **−2,2 s** (B, A'dan daha *hızlı*: VM dalgalanması/sıra etkisi, anlamlı değil, aralıklar örtüşüyor). Toplam farkın tamamı (+7,9 s ≈ 10,2 s Claude boşluğu − 2,2 s program) **Claude'un iki çağrı arasındaki gecikmesinden** geliyor.
6. **Otomatik daha hızlı mı?** Hesap bakımından **hayır**: aynı ikili, program süresi eşdeğer (51,3 ↔ 49,1 s, VM dalgalanması içinde). Fark yalnızca yönetim/gecikme süresi (≈ 9–13 s çağrılar arası, bu minimal akışta). Sınırlar: Claude'un düşünme süresi tek başına ayrıştırılamadı (boşluk = düşünme + üretim + harness, ayrıştırılamaz); gerçek bir görevde (çok çağrı, hata ayıklama) fark çok daha büyüktür ve burada ölçülmedi; araç-sınıflandırıcı hataları (iki kez "no verdict", yeniden deneme) ölçüm dışı bırakıldı ama gerçek akışa ek gecikme getirir. Program süresinden Claude'un toplam maliyeti *tahmin edilmedi*.
