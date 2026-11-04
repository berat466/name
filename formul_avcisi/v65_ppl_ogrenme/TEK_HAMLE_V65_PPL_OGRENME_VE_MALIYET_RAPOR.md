# V65: PPL, gerçek öğrenme ve hesaplama maliyeti

Kod: `cpp/tek_hamle_v65.cpp` (V64 + `V65_EVAL=1` akış tabanlı değerlendirme, `V65_CHECK=1` açık/örtük eşdeğerlik), `v65_ppl_ogrenme/ozet.py`, ham çıktılar `ham/`. V61/V63/V64 dosyaları değişmedi. Veri: V59 ortak veri (Gutenberg: P&P+Alice+Moby Dick, ~297 k eğitim tokenı, blok-bölme %80/10/10, sözlük ve gömme yalnız eğitimden; sızıntı yok). Hiperparametreler (λ, γ, α_okuma, ε, α_Q) V64'tekiyle aynı; α/ε/α_Q **yalnız val** ile seçildi, test seçimde kullanılmadı. V64 koşuları: `V63_ARROW=2 V64_IMPL=1 V64_SPQ=1 V64_COMPACT=1` (açıkça etkin). A = V63 yolu (açık W_C, yoğun Q; `tek_hamle_v65` bayraklar kapalı). PPL=exp(CE), CE her yerde tüm test tokenları (n=36 852, <unk> hedefleri dahil) ve tüm V sınıfı üzerinden normalize (`Σp`) hesaplandı.

## Ana tablo (V64 yolu; tutarlı değerler, farklı sözlüklerin ham PPL'leri doğrudan karşılaştırılamaz)
| Ölçüm | 2.001 | 5.001 | 10.001 |
|---|---|---|---|
| Eğitim PPL (her 10. eğitim örneği, n=29 482, örnek-içi) | 35,32 | 45,64 | 54,23 |
| Validation PPL (CE) | 90,35 (4,5037) | 160,68 (5,0794) | 230,89 (5,4419) |
| **Test PPL (CE)** | **88,89 (4,4874)** | **173,83 (5,1581)** | **262,93 (5,5719)** |
| Test Top-1 / Top-5 | 21,51 % / 42,58 % | 16,41 % / 35,46 % | 13,70 % / 31,12 % |
| Test Top-1, <unk> hedefleri hariç | 9,04 % | 11,74 % | 12,77 % |
| Unigram test PPL (CE) | 205,26 (5,3243) | 415,74 (6,0301) | 628,23 (6,4429) |
| Test PPL / unigram PPL | 0,433 | 0,418 | 0,419 |
| Eğitim→test CE farkı | +0,92 | +1,34 | +1,58 |
| Test hedefi <unk> oranı / bağlamda <unk> oranı | 16,8 % / 30,6 % | 9,4 % / 17,6 % | 5,4 % / 10,3 % |
| Model hazırlama süresi A (V63 yolu) → B (V64) | 6,5 → 5,9–6,9 s | 14,5 → 9,1 s | 28,2 → 10,8–11,5 s |
| Tepe RAM A → B (test akışla) | 1,74 → 1,42 GB | 3,79 → 2,51 GB† | 7,83 → **2,81 GB** |
| V63/V64 fark: maks \|Δlogit\| / CE / Top-1 uyumu / rezidu | 4,8e-7 / özdeş (4,487430) / 1,000000 / 1,9e-14 | 4,2e-7 / özdeş (5,158058) / 1,000000 / 3,2e-14 | 3,6e-7 / özdeş (5,571887) / 1,000000 / 3,0e-14 |
| Değerlendirme süresi (val+test+eğitim altkümesi+4 kontrol, akış) | 6,9–7,4 s | 14,7 s | 22,3–22,6 s |
† 5.001 B: test eski yolla saklanıyordu (akış öncesi sürüm); 2.001/10.001'de akış (V65) kullanıldı.
VM gürültülü: modeller-hazır süreleri tekrarlı ama tek çift; mutlak değerlere ±%20 güvenin.

## V63 ↔ V64 eşdeğerliği
Aynı süreçte örtük (V64) ve açık W_C (aynı X, Y0, W_F) test logitleri karşılaştırıldı (`V65_CHECK`, 36 852 örnek × V): maks \|Δz\|=4,8e-7 / 4,2e-7 / 3,6e-7 (float yuvarlama; maks \|z\|≈1), Test CE 6 hanede özdeş, doğru/yanlış sınıflaması %100 uyumlu. Ayrı V63 koşusunun (A) test CE'si V64'ünkiyle aynı (4,4874 / 5,1581 / 5,5719). Küçük kayan nokta farkları öğrenme ölçümlerini değiştirmedi. (Check modunda açık W_C de kurulduğundan o koşuların süre/RAM'i tabloya alınmadı.)

## Öğrenme gerçekten var mı?
- **Test PPL unigramın 0,42–0,43 katı** (2,3–2,4× kazanç) her üç boyutta. Bağlam bilgisi kullanılıyor.
- **Bağlam kontrolü (eğitim ayarı değişmedi; yalnız değerlendirme girdisi)**: (i) bağlam–hedef eşleşmesi karıştırılmış test (girdi dağılımı korunur, ilişki bozulur): PPL 401,9 / 777,5 / 1 204,8 — unigramdan **kötü**, Top-1 (unk hariç) 1,4 / 2,3 / 2,9 % → model yanlış bağlamda ceza alıyor, yani bağlama gerçekten dayanıyor; (ii) C slotları (kelime/çift kimlikleri) kapalı, Q yok (F-only): 138,0 / 279,5 / 419,7 — dağılım kayması var (W_F, C ile birlikte çözüldü), yine de unigramdan iyi; (iii) Q_havuz kaldırıldı: 92,18 / 180,97 / 275,10 (Q kazancı %3,6 / 3,9 / 4,4 PPL).
- **Ezberleme/genelleme:** eğitim PPL 35→54, test PPL 89→263; CE açığı 0,92→1,58 nat ve sözlükle büyüyor → ciddi örnek-içi uyum; test yine de unigramı geçiyor (genelleme var ama zayıflayan, veri küçük: ~297 k token).
- **Sık / nadir (hedef kimliği: 1–100 sık, 101–1000 orta, >1000 nadir; id=frekans sırası):** 10.001'de test Top-1: sık %21,9, orta %2,7, nadir %0,09; CE: 3,84 / 6,66 / 9,81 (ln V=9,21 → nadirlerde tekdüze dağılımdan bile kötü). 2.001: nadir Top-1 %0,04. **Nadir kelimeler öğrenilemiyor**; eğitimde de nadir CE 5,3 nat (örnek-içi öğreniyor, genellemiyor).
- Top-1 <unk> sınıfıyla şişiyor (2.001'de %83'ü <unk> hedeflerinde); <unk> hariç Top-1 %9–13.

## Maliyet (V64, 10.001; s) ve darboğaz
Model hazır 10,8–11,5 s: özellik+K+Gram (f0) 6,5 (K-tablo/kapı f2 3,0 içinde), U=XᵀR_C 2,7 (2.001: 0,9; 5.001: 2,1), grup çözümü 0,8, Q_havuz 0,2. Değerlendirme (akış): 22,3–22,6 s, val seçimi (e1a, lin_table+yoğun val logit) 11,2 s + α_Q seçimi 4,5 s. Akış: test logitleri artık n×V saklanmadan (2048 satırlık bloklar, tüm V üzerinden softmax/rank) → 10.001'de RSS 4,17→2,81 GB, CE değişmedi. Val logiti seçim için hâlâ yoğun (n×V).
**Bir sonraki darboğaz:** (1) özellik+Gram+K-grafı/kapı hazırlığı (f0, ~6,5 s), (2) val seçimi (n×V yoğun val logit + lin_table, ~16 s), (3) U (~2,7 s). T için dsyrk ve Q satırı yeniden kurulumu bu görevde incelenmedi.

## Sorular
1. **Ne kadar öğreniyor?** Test PPL unigramın ~0,42 katı; zayıf genelleme farkı (CE +0,9…1,6), <unk> hariç Top-1 %9–13, nadir kelime ≈0.
2. **10.001 test PPL:** 262,93 (CE 5,5719; Top-1 %13,70; Top-5 %31,12).
3. **V64 hızlanması öğrenmeyi korudu mu?** Evet: logit farkı ≤4,8e-7, CE 6 hanede özdeş, sınıflama %100 uyumlu.
4. **Ezberleme mi genelleme mi?** İkisi: eğitim PPL 54 / test 263 (ezberleme belirgin), ama test unigramdan 2,4× iyi (gerçek bağlam ilişkisi; karıştırma testi PPL'i 1 205'e çıkarıyor).
5. **Unigram karşısında:** 0,43 / 0,42 / 0,42 oran (2,3–2,4× iyi).
6. **Nadir kelimeler:** Hayır (Top-1 ≈ %0,1, CE ≥ ln V).
7. **Süre:** ağırlık hazırlama 5,9–6,9 / 9,1 / 10,8–11,5 s (2k/5k/10k), değerlendirme 7 / 15 / 22 s.
8. **Sonraki darboğaz:** özellik+Gram+K-graf hazırlığı (f0) ve val seçimindeki n×V yoğun logit.

## Yapılmadı / sınırlar
Eğitim metrikleri her 10. örnekte (n=29 482) ve örnek-içi; 5.001 B koşusu akışlı test sürümüyle yeniden koşulmadı; yeni dsyrk/Q-satırı optimizasyonu yok; V64 bayrakları varsayılan kapalı (ana sürüme bağlanmadı); klasik LM karşılaştırması yok; tek tohum, güven aralığı yok.
