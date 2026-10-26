# Tek Hamle AI — V81: ağırlık üretiminde sonraki darboğaz

Dal `claude/formul-avcisi` · taban V80 (`cpp/tek_hamle_v80.cpp`, dokunulmadı) · kod `cpp/tek_hamle_v81.cpp` · betik/ham `v81_agirlik_hesap_sadelestirme/` · süre ≈22 dk / 30 dk.
Etiketler: **[Kesin]** ölçülen/koddan okunan, **[Muhtemel]** çıkarım, **[Tahmin]**.

## 0. Dürüst sonuç
* **Matematiksel (çarpma/toplama) işlem azaltımı bulunamadı: net 0.** FᵀF, F1 gemm, T, U, Schur, K-grafı Gram'ı bit-özdeşlik kısıtı altında zorunlu kaldı (§2). Soru 10'un cevabı: **V81 matematiksel kazanım sağlamadı.**
* Sağlanan: "üretip hemen üzerine yazma / gereksiz yapı" kaldırıldı — aritmetik değil, **gereksiz bellek yazması ve veri-yapısı maliyeti**: ağırlıklar V80 ile **bit-özdeş**.
* Aynı oturum A/B (10.001, 4 iş parçacığı, varsayılan yol): model hazırlama **V80 4.32 → V81 3.99 s** (5 çift), önceki 6 çiftlik tur (REUSE açıkken) **4.52 → 3.90 s**; tam program **7.63 → 7.46 s** (5 çift) ve **8.06 → 7.62 s** (6 çift). İki turda toplam 11 çiftin 9'unda model süresi düştü; son turda 2 çiftte işaret ters, yani kazanç gürültü bandına (~±0.3 s) yakın. Hedef "model <4 s": V81 çoğu çalıştırmada 3.9–4.0 s; **hedef değil sonuç olarak yazılmıyor**, kesin değil.

## 1. V80 model hazırlama ayrıştırması (10.001, `ham/p80b.txt`, FAZ toplamı ≈4.4 s) — soru 1
| aşama | süre | gerçek aritmetik | not |
|---|---|---|---|
| veri okuma + `expand_s` | 0.16 + 0.09 | kopya | bellek |
| F1: tahsis+sıfırlama / gemm / tanh | 0.21 / 0.25 / 0.20 | **2.90e10 madd**, 1.5e8 tanh | |
| K tabloları+kapı (V80) | 0.76 | seyrek Gram 2.7e8 fma + s(K) 8.4e7 | |
| V39 ortalama+karışım | 0.19 | 1.9e9 madd | |
| V49 (μ, σ, cm/cs, apply) | 0.49 | ~5e8 | |
| FᵀF | 0.40 | **3.89e10 madd** | en pahalı aritmetik |
| FᵀY | 0.135 | 1.5e8 ekleme | |
| slot sayımı (ACF/RC/NK) | 0.50 (g3 0.42) | 3.0e8 ekleme | bellek-sınırlı (satır gather) |
| RC/Y0/X (kapalı form) | 0.15 | ~4e7 | |
| T / U | 0.16 / 0.13 | 1.12e10 / 1.4e8 | |
| Schur+W_F | 0.16 | 4.5e7 + 2.6e9 | |
| Q | 0.12 | O(nnz) | |
| okuma/hazırlık serisi (`buildrank` 0.16, id 0.05, …) | ~0.3 | tamsayı | **seri** |
**En pahalı iki aşama:** (aritmetik) FᵀF ve F1 gemm; (duvar) F1 bloğu (1.17 s) ve K bloğu (0.76 s).

## 2. Matematiksel sadeleştirme araştırması (kanıtlı elemeler)
* **FᵀF (3.89e10 madd):** F = [1, A_j, ŝ(d_j²), ŝ(d_j³)] (j<170) + 2 kopya sütun. Sütunlar arasında doğrusal bağımlılık yok (d², d³ elemanlı doğrusal olmayan fonksiyonlar; affin standartlaştırma cm/cs, rmsA her sütunda farklı). Gram'ı A'nın çapraz momentlerinden (Σ A_i^a A_j^b, a,b ∈ {1,2,3}) üretmek de 510²/2 hücre × 9 derece çifti = aynı sayıda çarpma; üstelik float yuvarlama (her F hücresi `float`) cebirle yeniden üretilemez ⇒ bit-özdeş değil. **Azaltım yok.** Gram'ın her hücresi Schur sistemine giriyor (tam 513×513). FᵀY ile ortak hesap: FᵀY çarpma içermiyor (one-hot toplam), ortaklaştırılacak çarpma yok.
* **rmsA/340 sütun:** V79'daki kanıt geçerli (tanh→kapı→karışım→float zinciri); yeni bir cebirsel ilişki bulunamadı, tekrar denenmedi.
* **T/U/Schur:** T üst üçgeni V77/V79'da 2.001'de W_F karmasını değiştirdi ⇒ açılmadı. U yalnız 279 500 gerçek sıfır-olmayan × 513. Schur'da V sağ taraf sütunları bağımsız ve hepsi W_F için gerekli; daha az işlemli tam eşdeğer gösterim bulunamadı.
* **Slot toplamları:** kelime slotu = Σ çift slotları + kalan geçerli ama ACF float toplam sırasını değiştirir (bit farkı) ⇒ uygulanmadı.
* **K-grafı:** V80'de s(K) doğrusal türetildi; √'li benzerlik zorunlu.

## 3. Uygulananlar (hepsi bit-özdeş; hiçbiri aritmetik azaltım DEĞİL)
| # | değişiklik | ne "üretilip atılıyordu" | doğrulama |
|---|---|---|---|
| 1 | `V81_UNINIT`: `vector::resize/ctor` yerine sıfırlamasız tahsis (F 605 MB + val/test F, `expand_s` X2 300 MB, `readbin` 450 MB) | her biri n eleman **seri sıfırlanıp** hemen üzerine yazılıyordu (tüm elemanlar sonradan yazılıyor; kullanılmayan F1 sütunları hiç okunmuyor) | Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F karmaları aynı |
| 2 | `V81_RANK`: `buildrank` 295k `std::map` düğüm eklemesi yerine sıralama + koşu-uzunluğu | düğüm tahsisi/log n eklemeleri | aynı (sayım, anahtar) kümesi ⇒ aynı rütbeler (karma aynı) |
| 3 | `V81_REUSE` (**varsayılan kapalı**): Y0c/Xd/Xf (395 MB) Fc'nin dokunulmuş belleğinde | ilk-dokunuş sayfa hataları | g45 0.126→0.066 s ama model/duvar ölçümde fark yok (3.87↔3.97, 7.47↔7.39) ⇒ kanıtlanmadı, kapalı |
Ölçülen bileşenler (`ham/B81_3` ↔ `A80_3`): `buildrank` 0.114→0.034 s, `expand_s` 0.119→0.057 s, F tahsis 0.187→0.114 s, veri okuma 0.134→0.098 s (gürültülü; tek koşu).

### İşlem tablosu (zorunlu istenen)
| ölçüm | V80 | V81 |
|---|---|---|
| çarpma / toplama / madd | referans | **aynı (net kaldırılan 0)** |
| yeni eklenen aritmetik | — | 0 (buildrank: sıralama karşılaştırması ≈ 295k·18, eski map ≈ aynı mertebe; tamsayı) |
| bellek yazması | F, X2, readbin için tam sıfırlama (≈1.4 GB yazma) | sıfırlama yok (−~1.4 GB sıralı yazma) |
| ilgili süre | f1b1 ≈0.19–0.21 s, buildrank 0.11–0.16 s, expand 0.09–0.12 s | 0.11 s, 0.03 s, 0.06 s |
| RSS | 1531 MB | 1532 MB |
| ağırlık karması (W_F) | `d54c462f219c825b` | aynı |
**SIMD/paralelleştirme/bellek kazancı matematiksel işlem azaltımı olarak sunulmuyor.**

## 4. Doğruluk
`dogrula.sh` (V80↔V81; `V78_AUTO=0` ile): 2.001, 5.001, 10.001'de Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F karmaları, s(K) tablo karmaları, 8 α val CE (15 hane), test CE, Top-1/Top-5, α/ε/α_Q **aynı**; 20.001'de CE/Top-k/seçimler aynı (tek çift, `ham/s20k_*`), test PPL 262.930 (10.001), 252.531 (20.001). Not: `V78_AUTO` (V78'in fuse yolu zamanlama probu) zamanlamaya bağlı olarak son hane (≤6e-14) CE farkı üretebiliyor; bu V81'den bağımsız, `V78_AUTO=0` ile giderilir (karşılaştırma bu bayrakla yapıldı).

## 5. 20.001 (tek çift; V80 ve V81 ardışık)
Model hazırlama 29.8 → 18.9 s, tam program 59.6 → 50.2 s. VM gürültüsü çok büyük (V80 çalıştırmasında SYS 44 s); **bu fark güvenilir kazanç iddiası değildir.** Yapısal beklenti: sıfırlama/tahsis maliyeti n·513 ile ve okuma n ile doğrusal büyür ⇒ 20k'da (n=1.28M) mutlak kazanç ~4× büyük olmalı [Tahmin; ölçüm gürültülü].

## 6. Soruların cevabı
1. En pahalı aşama: aritmetik FᵀF (3.89e10 madd); duvar F1 bloğu 1.17 s, ardından K 0.76 s.
2. Gereksiz matematiksel işlem: **bulunamadı**. Gereksiz yazma/yapı: sıfırlamalar, `std::map` ile buildrank.
3. Kaldırılan çarpma-toplama: **0**.
4. Eklenen işlem: 0.
5. Net aritmetik tasarruf: **0**.
6. Model hazırlama: **≈3.9–4.0 s** (V80 4.3–4.5 s, aynı oturum).
7. Tam program: **≈7.5–7.6 s** (V80 7.6–8.1 s).
8. Ağırlıklar V80 ile aynı mı: **evet, bit-özdeş**.
9. PPL/hiperparametreler: **korundu** (262.930; α/ε/α_Q aynı).
10. Daha büyük sözlük: sıfırlama/tahsis ∝ n; V (sözlük) ile değil; 20k gürültülü, yapısal beklenti yukarıda.
11. **V81 gerçek matematiksel kazanım sağladı mı? Hayır.** Yalnızca gereksiz bellek yazması/veri yapısı kaldırıldı.

## 7. Yeniden üretim
`cd v81_agirlik_hesap_sadelestirme && ./dogrula.sh y10k /tmp/v59_10001`; bayraklar `V81_UNINIT`(1) `V81_RANK`(1) `V81_REUSE`(0).
