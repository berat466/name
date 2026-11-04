# TEK HAMLE V51b — Sunucuda devam ve bilgi kaybı (kayıpsız fonksiyon) araştırması

Kod: `cpp/tek_hamle_v52.cpp` (V51b kopyası + yalnız ölçüm/döküm kancası `V52_DUMP` + deneysel `V52_PW`; V51b ve öncesi dokunulmadı). Ham: `v52_ham/` (`ref_B.txt`, `ref_plainF.txt`, `analiz1.txt`, `analiz2_B.txt`, `base_lamgrid.txt`, `pw_p0.1_t1e-4.txt`, `proxy.py`).

## 1) Bu sunucuda bulunanlar / eksikler
- **Bulundu (sürüm/hash, sha256 ilk 16):** `cpp/tek_hamle_v51b.cpp` 7f10dc17…, `v51c` 45e93959…, `v51g` (GPU shim) 8f185283…, `v50` c12cbbb3…; Git dalı `claude/formul-avcisi` (HEAD 7e0d170). Veri: `/tmp/v8data` (2001 kelime; `c2_train_X.f32` d733e315…, `c2_train_y.i32` 28b624c7…). Train 39 840 / val 4 980 / test 4 980.
- **Eksik:** 5 000 kelimelik veri ve kodu (yok; yalnız 2001'lik veri var → 5000'lik sonuçlar bu sunucuda **yeniden üretilemez**). Görevde anılan "kelime sırası" ve "parçalı hesaplama (V2/V3/V4)" deney kaynakları bu sunucuda yok; buradaki `tek_hamle_v2/v3/v4.cpp` eski MNIST sürümleri, o deneylerle ilgisiz. Önceki 5000-kelime sonuçları (174,5401 vb.) bu rapora **dahil edilmedi**; "174,5401'in altına inme" hedefi bu veri için geçerli değil.
- **Referans bu sunucuda yeniden üretildi (2001 kelime, `V51_OOFPOOL=1 V51_GF=0.15 V49=2`):** doğrulama CE 4,8242, test CE 4,7783, **test PPL 118,91**, Top-1 %16,12, Top-5 %37,03 — görevdeki 4,82422173 / 4,77825708 / 118,8969 / %16,1245 / %37,0281 ile (yazdırma hassasiyetinde) uyumlu. Ağırlık üretimi (sıcak) ≈1,1–2,0 s; değerlendirme/SSD okuma hariç.

## 2) Mevcut formülün gerçek akışı (kaynak koddan) ve bilgi kaybı haritası
| Aşama | İşlem (V51b) | Boyut / tip | Tersinir mi? | Sayısal hassasiyet | Olası kayıp |
|---|---|---|---|---|---|
| X | iki kelime gömmesi (2×64) + fark bloğu → X (192) | float32 | kelime kimliği → 64D: **kayıplı** (ama C bloğunda tam kimlik var) | – | kimlik bilgisi C bloğunda korunuyor |
| F üretim | `tanh(γ·XA + b)`, A = eğitimden seçilen sınıf-çifti yönleri (192→512) | 39 840×512 f32 | tanh tersinir (atanh); doğrusal kısım rank ≤192 | doygunluk yok: düz F'te max‖F‖=0,92 | etkin rank çok düşük: kovaryans PR=**2,2**, sayısal rank 228/512 |
| Havuz | `H=(1−β)F+β·ΣaF̄_v`, β=0,9 | 512 | kelime biliniyorsa tersinir (`F=(H−βQ_w)/0,1`); ağırlık dengesi değişir | – | satırın kendi F'inin payı %10'a iner |
| V49 kuvvet | 170 taban × {A,(A−μ)²,(A−μ)³} + 2 A | 512 | A↔(A−μ)³ tersinir; **342 taban yön atılıyor (kayıplı seçim)** | kuyruk: max 22, %0,12 değer >0,999 (doygunluk değil, ağır kuyruk) | seçim kaybı |
| Kapı | `tanh(atanh(F)+g_F·U)` | 512 | tersinir (U biliniyorsa) | – | – |
| Gram/RHS | FᵀF (float32 sgemm 4096'lık parçalar → double toplam) | 513×513 | – | göreli hata **4–6·10⁻⁸** | önemsiz |
| Ridge | `(ZᵀZ+Λ)W=ZᵀY`, Schur, double Cholesky, W float | 4001×2001 + 513×2001 | **tersinmez: büzülme** | ağırlık farkı (G32↔G64) **1,6–2,5·10⁻⁷** | ana kayıp: bkz. aşağı |
| Okuma | `P=(1−α)π+α·max(z,ε)/Σmax(z,ε)` | 2001 | **tersinmez (kırpma+normalizasyon)** | – | z≤ε değerlerinin işareti/büyüklüğü atılır (bu görevde ölçülmedi) |
| Son karışım | `P_son=(1−α)P_model+αQ_havuz` | 2001 | karışım, bilgi ekler | – | – |

## 3) Üç türde kayıp, ölçümlerle (ayrı ayrı)
1. **Matematiksel tersinmezlik:** (a) Ridge büzülmesi — F bloğunun etkin serbestlik derecesi λ_F=3000'de yalnız **35,8/512** (λ=300: 109, λ=30000: 9,3); OLS sinyal enerjisinin **%25,7'si son 284 ana bileşende** ve ridge bu yönleri ezer. (b) Okuma kırpması. (c) V49'da 342 taban yön seçimi.
2. **Sayısal hassasiyet:** ihmal edilebilir. Float32-parçalı Gram, float64 Gram'dan 4–6·10⁻⁸ ve ridge ağırlıkları 1,6–2,5·10⁻⁷ farklı; FP64 Gram yatırımının PPL'e etkisi bu mertebede (≪ gürültü). tanh doygunluğu yok.
3. **Özellik yetersizliği/birleştirme:** F'in etkin rankı çok düşük (PR 2,2 / V49 sonrası 11,3) — bilgi birkaç baskın yönde; kuyruk yönleri küçük varyanslı (ev≈10⁻⁶) ama sinyal taşıyor.

## 4) Aday kayıpsız dönüşümler
| Yöntem | Gerçekten tersinir? | Sonlu hassasiyette | Boyut | Havuz/Ridge uyumu | PPL için gerçek neden? | Sonuç |
|---|---|---|---|---|---|---|
| Rastgele ortogonal, **Hadamard (512)**, PCA rotasyonu, ortonormal Haar dalgacık | Evet (QᵀQ=I) | ~1e-7 kayıp (float32) | 512 sabit | uyumlu (F→FQ) | **YOK:** izotropik ridge Q-değişmezdir: `W'=QᵀW`, tahminler özdeş | **ölçüldü: 3 dönüşümle CE farkı ≤ 9·10⁻¹⁶** (F-only proxy, aynı λ); tam hatta ACF/ACC kuplajı da kovaryant → özdeş (matematiksel) |
| Tersinir ama ortogonal olmayan: **kısmi beyazlatma** `(F−μ)U diag((ev+τev₁)^−p)` | Evet (koşul sayısı 2,5) | iyi | 512 | uyumlu; **etkin düzenleyiciyi değiştirir** | küçük varyanslı sinyalli yönleri ridge'den kurtarır | proxy'de **−0,0071 CE** (p=0,1,τ=1e-4), ama tam hatta **olumsuz** (aşağıda) |
| Bilgi koruyan birleştirme (A↔(A−μ)ᵏ) | kısmen | – | 512 | uyumlu | V49: tutarlı küçük kazanç | mevcut |
| FP64 / Kahan Gram | evet | 4e-8 → ~1e-16 | – | uyumlu | **yok** (kayıp zaten önemsiz) | önerilmez |

**Anahtar sonuç:** Kayıpsız/ortogonal dönüşümler (Hadamard, dalgacık dahil) bu formülde izotropik ridge ile **kanıtlanmış biçimde PPL'i değiştirmez**; tersinir olmak yeni yararlı bilgi üretmek demek değildir. Fark ancak **düzenleyiciyi** (Λ'yı) dönüşümle birlikte değiştirmekle (ör. banda bağlı λ, beyazlatma) oluşur — bu mevcut formülün değiştirilmesidir.

## 5) Seçilen tek aday deneyi: kısmi beyazlatma (tersinir, ortogonal değil)
Gerekçe: F-only proxy'de (doğrulama CE) ortogonal dönüşümler özdeş, kısmi beyazlatma p=0,1/τ=1e-4'te taban 4,9964 → **4,9893** (λ_F=1000). p≥0,25 kazanç yok, p=0,5 kötü. Aday proxy'de doğrulamayla seçildi (p,τ), sonra ayrı kopyada **tam hatta** (ortak havuz, tek Ridge/Schur korunarak; yalnız Gram öncesi F'e afin dönüşüm):
| Sistem (B-satırı: ρ=0, α doğrulamayla 0,2, g_F=0,15, V49=2) | en iyi λ_F | doğr. CE | test PPL | Top-1 / Top-5 |
|---|---|---|---|---|
| **Referans (dönüşümsüz)** | 3000 | **4,8242** | **118,91** | 0,1612 / 0,3703 |
| Kısmi beyazlatma p=0,1, τ=1e-4 | 3000 | 4,8287 | 119,30 | 0,1610 / 0,3703 |
(λ_F=300/1000/3000/10000 ızgarası, ikisi için de; dönüşüm doğrulama CE'yi her λ_F'de 0,004–0,035 **kötüleştirdi**.) Dönüşüm süresi ≈0,5 s. Sonuç: **kayıpsız beyazlatma tam hatta kazanç vermedi**; proxy kazancı (C bloğu olmadan) ana modele taşınmadı. Bu bir olumsuz sonuçtur; tek p,τ noktası denendi, geniş tarama yapılmadı.

## 6) Rapor soruları
1. **Bulunan kaynaklar:** yukarıda (V51b/c/g, V50; 2001'lik veri).
2. **Eksik:** 5000'lik veri ve kod; "kelime sırası" ve "parçalı hesap" (V2/V3/V4) deney kaynakları; önceki 5000-kelime logları.
3. **Gerçek akış:** bölüm 2 tablosu.
4. **Tersinmez/kayıplı aşamalar:** ridge büzülmesi (ana), okuma kırpması, V49 taban seçimi, X'te 64D gömme; havuz/kuvvet/kapı tersinir.
5. **Uygun kayıpsız fonksiyonlar:** hepsi tersinir ve havuz-uyumlu ama izotropik ridge'de ortogonal olanlar etkisiz; beyazlatma etkili ama bu veride olumsuz. Sayısal olarak FP64'e gerek yok.
6. **Sonuçlar:** referans 4,8242 / 118,91; kısmi beyazlatma 4,8287 / 119,30 (kötü). RAM tepe ≈1,5 GB (değişmedi).
7. **Tek çekirdek/ortak havuz korundu mu:** evet; dönüşüm Gram öncesi tek F matrisine uygulandı, ayrı model/eğitim yok, iteratif güncelleme yok.
8. **Sonraki en mantıklı geliştirme:** bilgi kaybı ridge'in kuyruk yönlerini ezmesinden geliyor (df 36/512; sinyalin %26'sı kuyrukta) — ama beyazlatma bunu çözmedi. Daha umut verici: (a) λ_F'yi bileşene göre değil, **C bloğu ile birlikte** (Schur) düşünerek ayarlamak; (b) V49 taban seçiminde (342 atılan yön) daha az kayıplı seçim/birleştirme; (c) okuma kırpmasının kaybını ölçmek (bu görevde ölçülmedi). Bunlar formülde değişiklik gerektirir; onay beklenir.

## Sınırlar
Tek veri (2001 kelime), tek tohum (model tohumu 1), tek bölünme; proxy yalnız F bloğu (C bloğu yok); kısmi beyazlatma için tek (p,τ). Okuma kırpma kaybı ve dalgacık/Hadamard'ın tam hatta çalıştırılması yapılmadı (ortogonallik nedeniyle matematiksel özdeşlik kullanıldı, proxy'de sayısal doğrulandı).
