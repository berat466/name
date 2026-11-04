# TEK HAMLE V41 — F + ortak havuz + Ridge çözümünü tek matematiksel yapıda ele alma

Kod: `cpp/tek_hamle_v41.cpp` (V40 kopyası; yeni yol `V41_GRP=1` ile açılır, kapalıyken V40 ile aynı; V40 ve öncesi dokunulmadı). Referans yapı: V40 tek F×512 + bağlantılı havuz, β=0.9, λ_C=100, λ_F=3000, kimlik+çift tabanı, intercept (aynı veri, tohum, bağlantılar, λ'lar). Ham çıktılar: `v41_ham/` (`ref_r*.txt`, `grp_r*.txt`, `tablo41.txt`).

## 1) Matematiksel analiz: ne birleştirilebilir, ne birleştirilemez
**Havuz = F üzerinde doğrusal dönüşüm.** H_i = (1−β)F_i + β·Q_{w(i)}, Q = G·F̄ (G: V×V kNN ağırlıkları, F̄_u: u kelimesiyle biten eğitim satırlarının F ortalaması). Gram açılımı: HᵀH = (1−β)²FᵀF + β(1−β)(F̄ᵀdiag(n)Q + Qᵀdiag(n)F̄) + β²QᵀNQ (N=diag(n_u)); RHS: HᵀY = (1−β)FᵀY + βQᵀC (C: kelime→hedef sayım matrisi).
- Bu, H'yi (n×513) hiç üretmeden Gram/RHS'yi vermek mümkün; **ancak** baskın terim FᵀF (n·513²) yine gerekiyor (F doğrusal-olmayan, çıktısı satır başına hesaplanmak zorunda). Kazanç yalnız H'nin yazılıp okunması (n×513 float) ve havuz karışım geçişi (~0.1–0.2 s). RHS'te QᵀC (513×2001×2001 ≈ 2 GFLOP) mevcut seyrek toplamadan **daha pahalı**. Net hesap kazancı küçük, bu yüzden **uygulanmadı**; eşdeğerlik formülü yazıldı ama hız iddiası yok.
- Ölçülen aşama süreleri bunu doğruluyor: F+havuz+Gram ≈0.83–0.95 s (havuzun kendisi ≈0.2 s) — toplamın küçük kısmı. Asıl maliyet C-bloğu çözümü idi.

**Gerçek tasarruf: C bloğunun yapısı.** C0 tabanında (ID(w_{t−1}) + en sık 2000 (w_{t−2},w_{t−1}) çifti) her çift tek bir son kelimeye bağlı. A_CC = CᵀC + λI bu yüzden son kelimeye göre **blok-köşegen** (2001 grup, en büyüğü 88 satır). Yoğun 4001×4001 Cholesky + 2514 sağ tarafla (Y0: 2001, X: 513) üçgen çözüm yerine grup grup küçük Cholesky+çözüm tam eşdeğerdir. Havuz ve F bu yapıyı bozmaz (C bloğu F'den bağımsız).
- Kod yapı denetimi yapıyor: blok dışı sıfır olmayan eleman sayısı 0 olmazsa eski yoğun yola düşer (denetim 0.008 s; bu çalışmada "EVET, blok dışı=0").
- İşlem tasarrufu (yaklaşık): yoğun Cholesky mc³/3 ≈ 21 GFLOP + çözümler 2·mc²·2514 ≈ 80 GFLOP; grup yolu üst sınırı: Σs_g² ≤ 88·4001 → ≲ 2 GFLOP çözüm + ~0.01 GFLOP Cholesky (≈50× az işlem). Yoğun A (128 MB) kopyası/çarpanı artık üretilmiyor; ancak yoğun ACC (sayım matrisi) hâlâ oluşturuluyor, yani **bellek tasarrufu sınırlı** (ACC kaldı).
- Kaldırılan ara matrisler/işlemler: A_CC+λI yoğun kopyası, 4001² yoğun Cholesky, iki yoğun çok-sağ-taraflı üçgen çözüm (Y0, X). Kaldırılmayanlar: FᵀF Gram, T=ACFᵀX, U=ACFᵀY0 (dgemm, ≈0.07–0.1 s), 513'lük Schur çözümü.

## 2) Eşdeğerlik doğrulaması (önce)
Aynı koşulda V40 yolu ve V41 grup yolu ağırlık dökümü:
- W_C: maks mutlak fark **4.7e-10**, bağıl Frobenius fark **5.4e-10** (||W_C||=2.16); W_F: **1.4e-17 / 6.1e-17**.
- Test PPL, Top-1, Top-5 **aynı**: 124.92 / 0.1610 / 0.3673 (doğrulama CE 4.8753; kalan sistem rezidüsü iki yolda da 1.882e-2, aynı). Yani bu bir eşdeğer çözümdür, öğrenme sonucu değişmedi.

## 3) Süre (9 tekrar, her biri ayrı süreç, sırayla/alternatif; makine gürültülü: aynı yol 2.35–5.86 s arası oynadı)
Toplam = F+havuz+Gram hazırlığı + C-bloğu Cholesky/Y0/X/Schur hazırlığı + λ_F Schur çözümü (M6); tek λ_C, tek λ_F; değerlendirme/ID-only logit hariç değil: "SURE" satırı Y0 doğrulama logitlerini de içerir (iki yolda eşit).
| Yol | F+havuz+Gram | Cholesky+Y0+X+Schur | λ_F çözümü | **Toplam min / medyan** |
|---|---|---|---|---|
| V40 (yoğun) | 0.83–0.95 | 1.35–3.03 | 0.12–1.18 | **2.35 / 3.38 s** |
| V41 grup yolu | 0.84–1.01 | 0.36–2.64 | 0.11–0.84 | **1.33 / 2.64 s** |
Saf Cholesky+Y0+X çözüm aşaması (TT M2+M3+M4): V40 min 1.07 / medyan 1.64 s; V41 min **0.135** / medyan 0.62 s (≈8× min, ≈2.6× medyan). Gürültü nedeniyle toplamdaki kazanç medyanda ≈0.7 s, minimumda ≈1.0 s; en temiz tek karşılaştırma (art arda, gürültüsüz iki koşu: V40 2.35 s, V41 1.33 s) ≈1.8×. Kesin hızlanma katsayısı olarak tek sayı vermiyorum; aralık ≈1.3–1.8×, işlem sayısı azalması çok daha büyük ama dgemm/Gram/Schur sabit maliyetleri kalıyor ve grup yolundaki çözümler basit çok-iş-parçacıklı kodla (BLAS değil) yapıldığı için sapması büyük.

## Sınırlar / dürüstlük
- Kazanç **havuzdan değil, kimlik bloğunun blok-köşegen yapısından**; yani "F+havuz+Ridge tek yapıda" ifadesi bu kazancı açıklamıyor. Havuz+F+Gram tarafında somut tasarruf bulamadım (formül mevcut, uygulanmadı, hesap gereği ihmal edilebilir).
- Yalnız C0 tabanı (son kelimeye bağlı çiftler) için geçerli; kelime ID + önceki kelime ID (V31 tabanı) veya hash çift kimliği gibi yapılarda blok-köşegenlik bozulur ve kod otomatik eski yola düşer.
- Süre gürültülü; 3 değil 9 tekrar yaptım, ama yine de ±1 s oynama var.
- `V41_GRP` varsayılan **kapalı** bırakıldı; eşdeğerlik ve kazanç kanıtlı olduğundan C0 tabanında açmak önerilir, ancak "varsayılan yapma" kararını size bırakıyorum.

## Ana soru
Havuz+F+Ridge'i tek yapıda ele almak (Gram açılımı) ek işlem tasarrufu sağlamadı. Ama ortak yapı analizi (C bloğunun blok-köşegenliği) aynı ağırlıkları (fark ≈5e-10) yaklaşık 50× daha az işlemle ve C-çözüm aşamasında ≈2.6–8× daha hızlı üretmeyi sağladı; toplam ağırlık üretimi ≈3.4 s → ≈2.6 s (medyan), en iyi koşuda 2.35 → 1.33 s.
