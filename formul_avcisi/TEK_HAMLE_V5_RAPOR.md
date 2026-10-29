# Tek hamle V5 raporu — önceki ağırlıktan devam (RLS) vs baştan çözüm

Branch: `claude/formul-avcisi`. V4 dosyaları değişmedi (V5 yeni dosyada, V4 çekirdeklerini kopyalar).
Kural: gradyan yok, Adam yok, epok yok. Kod: `cpp/tek_hamle_v5.cpp`. Ham loglar, komutlar, ağırlık dökümleri: `v5_ham/`.
Ortam: Intel Xeon 2,80 GHz, 4 çekirdek (AVX-512), 16 GB, g++ 13.3.0 `-O3 -march=native -fopenmp` (`v5_ham/ortam.txt`).

## Ana soru ve kısa cevap
**Eski ağırlıktan devam ederek, baştan çözümle aynı düşük kaybı daha kısa sürede elde edebiliyor muyuz?**
- [Kesin] **Aynı kaybı elde ediyoruz** (test MSE 5 hane aynı, unutma aynı).
- [Kesin] **Daha kısa sürede değil.** Denenen H=256 ve H=512, 3 görev kurulumunda RLS güncellemesi hem V4'ün kesin birikimli çözümünden (≈ 9–11× yavaş) hem de her görevde baştan çözümden (≈ 4,5–6,4× yavaş; özellik hesabı dahil toplamda 1,5–2,6×) daha yavaştı.
- [Muhtemel] Neden: RLS her örnek için `P` matrisini (m×m) güncellemek zorunda (örnek başına ≈ 2m² çarpma-toplama); toplu yöntem `FᵀF`'i simetri kullanarak ≈ m²/2 ile biriktirir ve tek Cholesky (m³/3, m=257'de ihmal edilebilir) atar. RLS'nin çözdüğü sorun (m³ çözümü) bu boyutta zaten ucuz. RLS her görev sonunda ağırlık gerekmeyip örnek örnek akış gerektiğinde anlamlı olur; görev düzeyinde güncellemede değil.

## Kurulum
- Sınıf-artımlı MNIST, ilk 3 görev: {0,1}, {2,3}, {4,5}; 10 çıkış, one-hot hedef. Eğitim = 55k fit satırı (görev başına ≈ 11k), test = 10k.
- Sabit özellik matrisi (V4 sabit-W1): W1 yalnız görev-1 verisinden, çift-yön (`cyon_rbias`), γ=0,125, λ=1, tek tohum (1). Büyütme yok.
- RLS (tam formül, double): `K = P Fᵀ (I + F P Fᵀ)⁻¹`, `W ← W + K (T − F W)`, `P ← P − K F P`; başlangıç `W=0`, `P=I/λ`; `(I+FPFᵀ)` b×b Cholesky ile çözülür (ters alınmadı). Eski görevlerin verisi yeniden işlenmez; durum = (W, P). Blok boyutları b ∈ {1, 16, 64, 256, 1024}.
- Karşılaştırmalar: (a) **V4 kesin**: S,R birikimli (yalnız yeni görev işlenir) + Cholesky; (b) **baştan**: her görevde görülen TÜM veri yeniden işlenip S,R sıfırdan kurulup çözülür; (c) **ref64**: (a) ile aynı ama S,R tam double toplamla (ağırlık doğruluğu referansı).
- Satırlar her görevde aynı karışık sırayla (tohum 99+1) işlendi. Süreler 3 tekrarın **medyanı** (aynı komut, aynı tohum). Süre = duvar saati; "özellik" = yeni satırlar için `tanh(γ(XW1+b1))` hesabı (tüm yöntemlerde ortak iş), "güncelleme+çözüm" = istatistik/RLS güncellemesi + (varsa) Cholesky.
- Kayıp = MSE (ortalama `(z−t)²`, örnek × 10 çıkış); eski/yeni/toplam = önceki görevlerin / son görevin / görülen tüm sınıfların test örnekleri. Unutma = (görev 3 sonrası eski görev i test MSE) − (görev i'yi öğrendikten hemen sonraki test MSE), i=1,2 ortalaması.
- Çalışma limiti 10 dk: ana deney 260 sn'de (bütçe 540 sn) bitti, ek ref64 + 5 görev koşuları ~10 sn; toplam limit içinde, hiçbir koşu atlanmadı (`atlanan.txt` yok).

## Sonuçlar — 3 görev (görev 3 sonrası, γ=0,125, λ=1, tohum 1)

#### H=256 (özellik), m=257

| yöntem | test MSE eski | yeni | toplam | doğr. toplam % | unutma (MSE) | özellik sn (3 görev) | güncelleme+çözüm sn (3 görev) | görev başına güncelleme+çözüm sn (t=1/2/3) | toplam sn | kesin-V4'e göre güncelleme yavaşlığı | W farkı vs ref64 (göreli) | duvar sn (süreç) | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| V4 sabit-W1 kesin (yalnız yeni görev istatistiği + Cholesky) | 0.01307 | 0.01586 | 0.01394 | 96.58 | 0.00788 | 0.209 | 0.083 | 0.030 / 0.027 / 0.026 | 0.292 | 1.0× | 2.2e-03 | 0.73 | 226 |
| baştan çözüm (görülen TÜM veri her görevde yeniden işlenir) | 0.01307 | 0.01586 | 0.01394 | 96.58 | 0.00788 | 0.471 | 0.174 | 0.032 / 0.060 / 0.082 | 0.646 | 2.1× | 2.2e-03 | 1.08 | 227 |
| RLS, blok=1 | 0.01307 | 0.01586 | 0.01394 | 96.58 | 0.00788 | 0.234 | 1.258 | 0.461 / 0.419 / 0.378 | 1.492 | 15.2× | 8.4e-11 | 1.98 | 227 |
| RLS, blok=16 | 0.01307 | 0.01586 | 0.01394 | 96.58 | 0.00788 | 0.194 | 0.781 | 0.281 / 0.274 / 0.227 | 0.975 | 9.4× | 8.3e-11 | 1.35 | 227 |
| RLS, blok=64 | 0.01307 | 0.01586 | 0.01394 | 96.58 | 0.00788 | 0.199 | 1.290 | 0.474 / 0.419 / 0.397 | 1.489 | 15.6× | 8.5e-11 | 1.87 | 227 |
| RLS, blok=256 | 0.01307 | 0.01586 | 0.01394 | 96.58 | 0.00788 | 0.208 | 3.652 | 1.335 / 1.217 / 1.100 | 3.860 | 44.1× | 8.6e-11 | 4.26 | 228 |
| RLS, blok=1024 | 0.01307 | 0.01586 | 0.01394 | 96.58 | 0.00788 | 0.205 | 14.948 | 5.256 / 5.003 / 4.689 | 15.153 | 180.5× | 1.0e-10 | 15.53 | 239 |

#### H=512 (özellik), m=513

| yöntem | test MSE eski | yeni | toplam | doğr. toplam % | unutma (MSE) | özellik sn (3 görev) | güncelleme+çözüm sn (3 görev) | görev başına güncelleme+çözüm sn (t=1/2/3) | toplam sn | kesin-V4'e göre güncelleme yavaşlığı | W farkı vs ref64 (göreli) | duvar sn (süreç) | tepe RAM MB |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| V4 sabit-W1 kesin (yalnız yeni görev istatistiği + Cholesky) | 0.00999 | 0.01228 | 0.01070 | 97.61 | 0.00601 | 0.427 | 0.268 | 0.097 / 0.089 / 0.082 | 0.695 | 1.0× | 5.6e-03 | 1.27 | 234 |
| baştan çözüm (görülen TÜM veri her görevde yeniden işlenir) | 0.00999 | 0.01229 | 0.01070 | 97.61 | 0.00601 | 0.801 | 0.450 | 0.089 / 0.151 / 0.209 | 1.251 | 1.7× | 5.0e-03 | 1.85 | 234 |
| RLS, blok=1 | 0.00999 | 0.01228 | 0.01070 | 97.61 | 0.00601 | 0.457 | 5.708 | 1.984 / 1.959 / 1.765 | 6.165 | 21.3× | 2.0e-10 | 6.91 | 236 |
| RLS, blok=16 | 0.00999 | 0.01228 | 0.01070 | 97.61 | 0.00601 | 0.405 | 2.888 | 1.062 / 0.962 / 0.864 | 3.293 | 10.8× | 2.0e-10 | 3.89 | 235 |
| RLS, blok=64 | 0.00999 | 0.01228 | 0.01070 | 97.61 | 0.00601 | 0.401 | 3.402 | 1.239 / 1.109 / 1.054 | 3.804 | 12.7× | 2.0e-10 | 4.36 | 236 |
| RLS, blok=256 | 0.00999 | 0.01228 | 0.01070 | 97.61 | 0.00601 | 0.453 | 8.376 | 2.998 / 2.810 / 2.568 | 8.829 | 31.3× | 2.0e-10 | 9.47 | 237 |
| RLS, blok=1024 | 0.00999 | 0.01228 | 0.01070 | 97.61 | 0.00601 | 0.442 | 30.121 | 10.475 / 10.178 / 9.469 | 30.563 | 112.6× | 2.2e-10 | 31.20 | 252 |

(Tablo `v5_ham/analiz.py` mantığıyla ham loglardan üretildi; "yavaşlık" = güncelleme+çözüm süresi / V4 kesin güncelleme+çözüm süresi.)

## Okuma
1. [Kesin] **Kayıp aynı.** H=256: test MSE eski 0,01307 / yeni 0,01586 / toplam 0,01394, doğruluk %96,58, unutma 0,00788 — kesin, baştan ve tüm RLS blok boyutlarında aynı. H=512: 0,00999 / 0,01228 / 0,01070, %97,61, unutma 0,00601 (baştan çözümde yeni görev MSE 0,01229, beşinci hanede fark). RLS, `P₀=I/λ` ile başlayınca matematiksel olarak ridge çözümüne eşit olduğu için beklenen sonuç.
2. [Kesin] **RLS daha hızlı değil.** En iyi RLS (blok 16): güncelleme+çözüm H=256'da 0,781 sn (kesin 0,083 sn, baştan 0,174 sn), H=512'de 2,888 sn (kesin 0,268 sn, baştan 0,450 sn). Özellik hesabı dahil toplam: H=256 RLS 0,98 sn, kesin 0,29 sn, baştan 0,65 sn; H=512 RLS 3,29 sn, kesin 0,70 sn, baştan 1,25 sn.
3. [Kesin] Blok boyutu etkisi (H=256 güncelleme+çözüm, 3 görev): b=1 → 1,26 sn, b=16 → 0,78 sn, b=64 → 1,29 sn, b=256 → 3,65 sn, b=1024 → 14,9 sn. Blok büyüdükçe b×b matris işlemleri ve çarpımlar baskın oluyor; b=1'de ise örnek başına ayrı güncelleme maliyeti. Hiçbir blok boyutu toplu yöntemi geçmedi.
4. [Kesin] **Baştan çözümün maliyeti görev sayısıyla büyüyor, RLS'ninki sabit**: H=256'da baştan güncelleme+çözüm görev başına 0,032 → 0,060 → 0,082 sn (artıyor), RLS(b=16) 0,281 → 0,274 → 0,227 sn (sabit); V4 kesin 0,030 → 0,027 → 0,026 sn (sabit). V4 kesin zaten "önceki durumdan devam" ediyor (S,R korunuyor).
5. [Kesin] **Ağırlık farkı: RLS en doğru, V4'ün kesin çözümü referanstan sapıyor.** Tam double toplamlı referansa (ref64) göre göreli Frobenius farkı (görev 3): RLS tüm blok boyutlarında H=256'da ≈ 8–10×10⁻¹¹, H=512'de ≈ 2×10⁻¹⁰ (yani sayısal olarak aynı çözüm); V4 kesin H=256'da 2,2×10⁻³, H=512'de 5,6×10⁻³; baştan çözüm 2,2×10⁻³ / 5,0×10⁻³. Bu fark V4'ün `syrk_add` kodunun 2048'lik parçalar içinde float toplam yapmasından geliyor, RLS'den değil (RLS double). Kayba etkisi görülmedi (MSE aynı), ağırlıklara etkisi var. V4 dosyalarına dokunmadım; bilgi olarak raporda.
6. [Kesin] **RAM aynı düzeyde**: süreç tepe RAM'i H=256'da kesin 226, baştan 227, RLS 227–239 MB; H=512'de kesin 233–234, RLS 235–252 MB. Ölçülen fark yüklenen veri kümesinin (≈ 226 MB) üstüne ≤ 18 MB; durum RLS için (W, P) ≈ `m²` double (257² × 8 B ≈ 0,5 MB; 513² × 8 B ≈ 2,1 MB), toplu yöntem için (S, R) aynı büyüklükte. P'nin simetri sapması: göreli ≈ 4×10⁻¹⁶ (b=1) … 2×10⁻¹⁴ (b≥16): biriken sayısal sapma ihmal edilebilir.
7. [Kesin] **5 görev ek koşusu (H=256, tek tekrar, b=16)**, görev başına güncelleme+çözüm sn (`v5_ham/v5log2/T5_*`):
   - V4 kesin: 0,029 / 0,024 / 0,021 / 0,025 / 0,022 (sabit);
   - baştan: 0,031 / 0,051 / 0,069 / 0,093 / 0,104 (≈ lineer artış);
   - RLS(b=16): 0,254 / 0,255 / 0,230 / 0,262 / 0,239 (sabit).
   Beş görevde RLS hâlâ baştan çözümden yavaş (görev 5'te 0,24 vs 0,10 sn). [Tahmin] Baştan çözümün eğrisi RLS'ye yaklaşık 12. görev civarında yetişir; MNIST'te yalnız 5 sınıf-çifti görevi olduğu için bu ölçülmedi.

## Sınırlar
- Tek tohum (1), H=256 ve 512; süreler 3 tekrarın medyanı. Süreler küçük (milisaniye-saniye) olduğu için sabit gecikmeler ve çok iş parçacıklı (4 çekirdek) çalışma etkisi ölçüme karışır; sıralama farklı boyutlarda (H=256, 512) ve tekrarlarda aynı kaldı.
- RLS tek iş parçacıklı mı çok iş parçacıklı mı: blok işlemleri `b·m² > 4·10⁵` olduğunda OpenMP ile paralel (4 çekirdek), küçük bloklarda seri. Toplu yöntemin `syrk` ve Cholesky çekirdekleri paralel. Her iki taraf aynı makinede.
- RLS çekirdeği benim yazdığım basit ikili-çevrim çarpımlarıdır (BLAS yok); toplu yöntemin `syrk` çekirdeği V4'ten, daha optimize. Daha optimize bir RLS çekirdeği (örn. simetriyi kullanan P güncellemesi) farkı küçültebilir; 9–11×'i kapatıp kapatmayacağı ölçülmedi ([Tahmin]: kapatması beklenmez, çünkü örnek başına P güncellemesi toplu `syrk`'ten ≈ 4× fazla işlem).
- Unutma ve kayıp RLS'de matematiksel olarak ridge ile aynı: RLS bu kurulumda yeni bir öğrenme davranışı getirmiyor; yalnız hesaplama biçimini değiştiriyor.
- Büyüyen özellik uzayı bu deneyde yok (V4'ün sorunu ayrı).

## İyileştirme fikirleri (ölçülmedi)
- RLS'yi yalnız çok küçük, sürekli akışlı güncellemelerde (görev ortasında anlık ağırlık gerekirse) kullanmak.
- P yerine yalnız S,R tutmak (V4) ve çözümü Cholesky'yi bir önceki Cholesky faktörüne rank-b güncelleme olarak yapmak (faktör güncellemesi: m² yerine m²b/…); kazanç m büyükken ancak.
- V4 `syrk_add` toplamını double yapıp (ya da Kahan/pairwise) ağırlık sapmasını gidermek.

## Doğrulama / hatalar
- Hata: yok (ilk çalıştırmalarda derleme dışında düzeltme gerekmedi). Not: ana koşular `ref64` modu eklenmeden önce derlenen ikili ile alındı; `ref64` ek bir dal olarak eklendi, `kesin/bastan/rls` yolları değişmedi. `ref64`, 5 görev ek koşuları ve ağırlık karşılaştırması sonradan derlenen ikili ile alındı (ağırlık dökümleri aynı etiketlerle yeniden yazıldı; aynı tohum/tek karıştırma sırası olduğundan 1–3. görev değerleri aynıdır).
- Tekrarlanabilirlik: aynı komut 3 kez koşuldu; test MSE/doğruluk üç tekrarda aynı (ham loglar `v5_ham/v5log/*_rep{1,2,3}.txt`).

## Komutlar (gerçek, `v5_ham/v5log/manifest.tsv`)
Biçim: `tek_hamle_v5 MOD H TOHUM γ λ BLOK [GÖREV]`.

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v5 kesin 256 1 0.125 1 0` | 1.24 | 2.07 | 226 |
| `./tek_hamle_v5 bastan 256 1 0.125 1 0` | 1.17 | 3.34 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 1` | 2.04 | 3.47 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 16` | 1.46 | 4.87 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 64` | 1.91 | 6.70 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 256` | 4.26 | 8.40 | 228 |
| `./tek_hamle_v5 rls 256 1 0.125 1 1024` | 16.14 | 24.43 | 239 |
| `./tek_hamle_v5 kesin 512 1 0.125 1 0` | 1.39 | 4.51 | 233 |
| `./tek_hamle_v5 bastan 512 1 0.125 1 0` | 2.07 | 7.28 | 234 |
| `./tek_hamle_v5 rls 512 1 0.125 1 1` | 6.91 | 9.43 | 235 |
| `./tek_hamle_v5 rls 512 1 0.125 1 16` | 3.99 | 14.76 | 235 |
| `./tek_hamle_v5 rls 512 1 0.125 1 64` | 4.34 | 16.37 | 236 |
| `./tek_hamle_v5 rls 512 1 0.125 1 256` | 9.20 | 18.08 | 237 |
| `./tek_hamle_v5 rls 512 1 0.125 1 1024` | 31.20 | 44.12 | 251 |
| `./tek_hamle_v5 kesin 256 1 0.125 1 0` | 0.73 | 2.07 | 226 |
| `./tek_hamle_v5 bastan 256 1 0.125 1 0` | 1.08 | 3.59 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 1` | 1.98 | 3.06 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 16` | 1.30 | 4.48 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 64` | 1.87 | 6.71 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 256` | 4.22 | 8.41 | 228 |
| `./tek_hamle_v5 rls 256 1 0.125 1 1024` | 15.53 | 23.56 | 239 |
| `./tek_hamle_v5 kesin 512 1 0.125 1 0` | 1.25 | 4.11 | 234 |
| `./tek_hamle_v5 bastan 512 1 0.125 1 0` | 1.85 | 6.44 | 234 |
| `./tek_hamle_v5 rls 512 1 0.125 1 1` | 6.97 | 9.56 | 235 |
| `./tek_hamle_v5 rls 512 1 0.125 1 16` | 3.89 | 14.59 | 235 |
| `./tek_hamle_v5 rls 512 1 0.125 1 64` | 4.36 | 16.43 | 236 |
| `./tek_hamle_v5 rls 512 1 0.125 1 256` | 9.47 | 18.64 | 237 |
| `./tek_hamle_v5 rls 512 1 0.125 1 1024` | 31.56 | 44.47 | 252 |
| `./tek_hamle_v5 kesin 256 1 0.125 1 0` | 0.69 | 2.01 | 226 |
| `./tek_hamle_v5 bastan 256 1 0.125 1 0` | 1.02 | 3.34 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 1` | 1.91 | 3.33 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 16` | 1.35 | 4.62 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 64` | 1.85 | 6.67 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 256` | 4.30 | 8.64 | 228 |
| `./tek_hamle_v5 rls 256 1 0.125 1 1024` | 15.38 | 23.32 | 239 |
| `./tek_hamle_v5 kesin 512 1 0.125 1 0` | 1.27 | 4.15 | 234 |
| `./tek_hamle_v5 bastan 512 1 0.125 1 0` | 1.81 | 6.34 | 234 |
| `./tek_hamle_v5 rls 512 1 0.125 1 1` | 6.72 | 9.21 | 236 |
| `./tek_hamle_v5 rls 512 1 0.125 1 16` | 3.84 | 14.25 | 235 |
| `./tek_hamle_v5 rls 512 1 0.125 1 64` | 4.49 | 16.75 | 236 |
| `./tek_hamle_v5 rls 512 1 0.125 1 256` | 9.50 | 18.75 | 237 |
| `./tek_hamle_v5 rls 512 1 0.125 1 1024` | 31.05 | 43.74 | 252 |

Ek koşular (`v5_ham/v5log2/manifest.tsv`):

| komut | duvar sn | kullanıcı CPU sn | tepe RAM MB |
|---|---|---|---|
| `./tek_hamle_v5 ref64 256 1 0.125 1 0` | 0.67 | 1.89 | 226 |
| `./tek_hamle_v5 ref64 512 1 0.125 1 0` | 1.53 | 4.79 | 233 |
| `./tek_hamle_v5 kesin 256 1 0.125 1 0 5` | 1.00 | 3.25 | 226 |
| `./tek_hamle_v5 bastan 256 1 0.125 1 0 5` | 2.03 | 7.25 | 227 |
| `./tek_hamle_v5 rls 256 1 0.125 1 16 5` | 2.12 | 7.70 | 227 |

## Ham terminal çıktısı örneği (`./tek_hamle_v5 rls 256 1 0.125 1 16`, tekrar 1)
```
V5 mod=rls H=256 tohum=1 gamma=0.125 lambda=1 blok=16 gorev=3 (4 cekirdek)
ADIM|rls|16|1|0.125|1|256|1|257|-1.000000|0.000284|0.000284|-1.0000|0.9991|0.9991|0.0738|0.3266|0.0000
GOREV|rls|16|1|1|1|0.000284|0.9991
ADIM|rls|16|1|0.125|1|256|2|257|0.003805|0.010325|0.007008|0.9910|0.9770|0.9841|0.0648|0.2769|0.0000
GOREV|rls|16|1|2|1|0.003805|0.9910
GOREV|rls|16|1|2|2|0.010325|0.9770
ADIM|rls|16|1|0.125|1|256|3|257|0.013072|0.015858|0.013938|0.9670|0.9632|0.9658|0.0565|0.2346|0.0000
GOREV|rls|16|1|3|1|0.006840|0.9858
GOREV|rls|16|1|3|2|0.019527|0.9476
GOREV|rls|16|1|3|3|0.015858|0.9632
OZET_ISLEM|rls|b=16|H=256|toplam_sn=1.27|tepe_RAM_MB=227|P_asimetri_goreli=1.934e-14|cholesky_yeniden=0
```
(Biçim: `ADIM|mod|blok|tohum|γ|λ|H|görev|m|test_MSE_eski|yeni|toplam|doğr_eski|yeni|toplam|özellik_sn|güncelleme_sn|çözüm_sn`; `GOREV|mod|blok|tohum|t|i|test_MSE_i|doğr_i`.)
