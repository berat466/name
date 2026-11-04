# V71: sonuç-odaklı bağımlılık denetimi — sonuç: KISMİ (bir ek kaldırma uygulandı ve doğrulandı; net kazanç ölçülemedi, ana sisteme bağlanmadı)

Kod: `cpp/tek_hamle_v71.cpp` (V65 + `V71_SYRK=1`, `V71_CHECK=1`; varsayılan KAPALI), ham çıktılar `v71_dogrudan_sonuc/ham/`. V65–V70 değişmedi. Yeni öğrenme algoritması yok; OpenClaw/harici ajan/GPU yok; gizli iterasyon yok.

## 1) Nihai çıktıların bağımlılık zinciri (V65, 10.001, hafif ayar)
Çıktı: z(x) = F̃(x)·W_F + Σ_slot Y0_slot (V64 örtük W_C). Gerekenler:
W_F ← S⁻¹(R_F − U), S = A_FF+λI − T; T=A_CFᵀ(A_CC+Λ)⁻¹A_CF; U=A_CFᵀ(A_CC+Λ)⁻¹R_C; X, Y0 ← ok-başı kapalı form (A_CC'nin yalnız slot sayıları n_k); F̃ için X (yalnız değerlendirilen bağlam slotları), Y0 seyrek satırlar; Q_havuz satırı q_w (seyrek+π).
Bu zincirin **zorunlu** kısmı: F üretimi, Fᵀ F (A_FF), Fᵀ Y (R_F), slot toplamları (A_CF, R_C, n_k), kapalı-form X/Y0, T, U, Schur Cholesky (513³/3), W_F çözümü, örtük okuma.

## 2) V54→V65 arasında kaldırılan (zincirden çıkan) hesaplar — 10.001 için işlem/bellek (ölçülen veya kapalı-form sayım)
| Kaldırılan | Eski maliyet | Yeni | Kaynak sürüm |
|---|---|---|---|
| K-graf S=NnNnᵀ (6 tablo, yoğun V³) | 1,2e13 madd, 40,7 s | 0,27 G madd, ~1 s | V60 |
| 6× yoğun Q (V×V), Cfull, Qf32 | 4,8+ GB, kq2 14 s | 0 (s(K) seyrek+π) | V61 |
| Grup Cholesky + üçgen çözümler | 1,9e10 madd | 1,2e8 madd (ok-başı) | V63 |
| Yoğun Y0d + dgemm ile U | 2,2e11 madd | 8,5e9 madd (XᵀR_C seyrek) | V57/V63 |
| W_C=Y0−XW_F (mc×V) | 2,2e11 madd, 1,7 GB | 0 (örtük okuma) | V64 |
| Pn, Qt (V×V), ACCB, seen, posy | ~2,6 GB | 0 | V64 |
| Test/val logit matrisi n×V saklama | 1,5 GB | blok akış | V65 |
Toplam kaldırılan: yaklaşık **1,2e13 + 4,5e11 madd** ve **≥ 7 GB** ara bellek (6,32→2,67–2,84 GB tepe RSS ölçüldü, V63→V64). Model hazır: 26–29 s → ~10–11 s (V63→V64/65).

## 3) V71'de yeni kaldırma: T için X'siz, yarı-işlem
Özdeşlik (ok-başı): T = ZᵀZ, Z satırları: çift slot a_p/√d_p, kelime slotu u_w/√s_w (u_w = x_w·s_w). `dsyrk` yalnız üst üçgeni hesaplar: **T madd'i yarıya iner** (10.001: 1,12e10 → 5,62e9; 2.001: 7,70e9 → 3,86e9).
Doğrulama (V71_CHECK, aynı süreçte dgemm(ACFᵀX) ile): maks bağıl fark **9,6e-16 (2k) / 1,1e-15 (10k)**; test PPL aynı (88,893 ve 262,930), val CE 5,441941 aynı.
Ölçüm (10.001, g8 = T süresi): V65 0,17 / 0,33 s → V71 0,15 / 0,19 s (VM gürültülü); model hazır 14,5 / 10,0 → 10,1 / 9,9 s. **Net hızlanma gürültüden ayrılamadı**; ayrıca Z tamponu +170 MB (tepe RSS 2,67→2,84 GB). Sonuç: işlem sayısı gerçekten yarıya indi, ama RAM artışı ve belirsiz süre kazancı nedeniyle **ana sisteme bağlanmadı** (varsayılan kapalı).

## 4) Hâlâ kalan pahalı bağımlılıklar (10.001; ölçülen duvar süresi)
U=XᵀR_C 2,65 s (8,5e9 madd; CPU/duvar ≈1,8: bellek-bağlı/dengesiz → asıl aday), K-tablo+kapı f2 2,7 s (s(K)+standardizasyon k4 1,3 s, kapı atanh/tanh k8 1,1 s), V49 dönüşümü 0,84 s, özellik üretimi 0,71 s, FᵀF 0,67 s (n·513²/2=3,9e10 madd), FᵀY 0,50 s, slot sayımı 0,43–0,53 s. Önerilen sıra: (1) U'yu bellek-dostu/bloklu planla (aritmetik değil, veri hareketi), (2) kapıda ortak atanh/tanh önbelleği (anahtar sayısı ≪ n; V54'te yapıldı), (3) FᵀY ile ACF/RC saçılımını tek veri geçişinde birleştirme (F satırı bir kez okunur; toplama sırası değişirse bit-eşitlik bozulur — uygulanmadı).

## Sorular
1. **Doğrudan hesaplama düzeni:** ok-başı kapalı form → U=XᵀR_C → Schur → örtük okuma + seyrek s(K)/q_w; V71'de T=ZᵀZ eklendi (opsiyonel).
2. **Kaldırılanlar:** bölüm 2 tablosu + T için dgemm→dsyrk (yarı).
3. **İşlem tasarrufu:** ≈1,2e13+4,5e11 madd (V54 yoğun düzenine göre, ölçülen/kapalı-form); V71 ek: 5,6e9 madd (10k).
4. **RAM:** V63→V64: 6,32→2,67 GB (ölçüldü); V71 syrk +0,17 GB.
5. **Ağırlıklar/PPL:** korundu (T farkı 1e-15, test PPL 262,930 ve 88,893 aynı).
6. **Klasik eğitim/gizli çözücü:** yok; Schur Cholesky (513³/3) ve ok-başı kapalı form kalan zorunlu doğrusal cebir (V65 matematiği, değişmedi).
7. **Tek girdiden doğru çıktı:** yöntem gereği tek girdinin çıktısı W_F, bağlam slotlarının X/Y0 satırları ve q_w ile hesaplanır (örtük okuma akışta toplu doğrulandı); **tek-örnek süresi ayrıca ölçülmedi**. Tek girdiden *ağırlık* üretilemez: ağırlık için tüm eğitim istatistikleri (A_FF, R_F, T, U) zorunlu.
8. **50k–100k:** bölüm 2'deki kaldırmalar V'ye bağlı yoğun matrisleri geri getirmez (V61–V65 kod taraması + sentetik kNN testi); T için Z tamponu mc×513 doğrusal. Gerçek 50k+ model koşusu yapılmadı.
9. **Kalan zorunlu hesap:** FᵀF (n·mf²/2), F üretimi (n·d·H), slot toplamları (n·mf), U (2·mf·nnz R_C), Schur Cholesky 513³/3 ve W_F çözümü (mf²·V), örtük okuma (n_test·mf·V).
