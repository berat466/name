# TEK HAMLE V53 — V51b öğrenme sınırlarını giderme ve modeli geliştirme

Dal: `claude/formul-avcisi` · Kaynak: `cpp/tek_hamle_v53.cpp` (V51b/V52'den kopya; **`cpp/tek_hamle_v51b.cpp` değişmedi**, sha256 `7f10dc17…`) · Veri üretici: `v53_ogrenme_gelistirme/metin_hazirla_v53.py` · Ham çıktılar: `v53_ogrenme_gelistirme/ham/` · Yeniden üretim: `v53_ogrenme_gelistirme/run_final.sh`

Güven etiketleri: **[Kesin]** ölçüldü/doğrulandı, **[Muhtemel]** güçlü çıkarım, **[Tahmin]** boşluk doldurma. Bu raporun büyük çoğunluğu [Kesin]; tahminler ayrıca işaretli.

## 0. Sonuç (tek bakışta)

| | V51b referans | V53 nihai | Fark |
|---|---|---|---|
| Validation CE | 4.8242 | **4.6090** | −0.2152 |
| Test CE | 4.7783 | **4.5859** | −0.1924 |
| **Test PPL** | 118.91 | **98.09** | −17.5 % |
| Test Top-1 / Top-5 | 16.12 % / 37.03 % | **18.21 % / 39.68 %** | +2.09 / +2.65 puan |
| Model hazır süresi (okuma+özellik+Gram+çözüm) | 2.8–3.1 s | 21.7–25.2 s | ≈ 8× |
| Tepe RAM | 1.44 GB | 7.3 GB | ≈ 5× |

Hedefler (val CE < 4.8242, test PPL < 118.8969) **aşıldı** [Kesin]. Kazancın büyük kısmı veriden (A, −0.187 val CE) ve çift-slot kapsamından (D, −0.022); B ve E işe yaramadı, C küçük kazandı.

Tek çekirdek, tek ortak havuz, tek analitik ağırlık üretimi korundu: C bloğu (kelime + çift slotları + sabit) ve F bloğu (512 rastgele-yön özelliği, V39 havuzu, V49 güç kolonları, V51 merkezi çekirdek) **hâlâ tek ortak ridge**, tek Schur/Cholesky çözümü; backprop, iteratif güncelleme, replay, ayrı model/uzman yok [Kesin].

## 1. Aşama A — veri kapsamı (59.6 % neden kullanılmıyordu?)

**Kök neden [Kesin]:** `metin_hazirla.py` satır 14: `toks = re.findall(...)[:50000]` + `assert len(toks) == 50000`. Kasıtlı bir üst sınırdı; kitabın 123,625 tokenından 73,625'i (**%59.6**) hiçbir yere ayrılmamıştı (ne eğitim, ne val, ne test). Veri kaybı ne bölme hatası ne sızıntı önleme değil, V8'deki "ilk 50k token" tasarım kararının mirası.

**Çözüm:** aynı tokenizasyon, aynı `rng(0).permutation(100)` bölmesi, **aynı 2001 kelimelik sözlük** (eski 80 eğitim bloğundan; val/test id'leri bit-bit aynı). Kalan **147 tam 500-token bloğu yalnız TRAIN'e** eklendi (eski 80 blok önde ve aynı sırada → doc-fold atamaları eski satırlar için aynı). 125 token'lık kısmi son blok kullanılmadı (%0.1).

| | V51b | V53 |
|---|---|---|
| Eğitim token / satır | 40,000 / 39,840 | **113,500 / 113,046** (2.84×) |
| Kitabın eğitime giden payı | %32.4 | **%91.8** |
| Val / test token | 5,000 / 5,000 | 5,000 / 5,000 (**aynı**) |
| Eğitim UNK oranı | %4.86 | %7.50 (sözlük sabit; val %8.1 test %9.0) |
| Val satırında (w₂,w₁) çifti eğitimde görüldü | %66.7 | %78.2 |
| Val satırında 3-gram / 4-gram bağlam görüldü | %22.2 / %4.1 | %33.6 / %8.2 |

**Sızıntı denetimi [Kesin]:** val/test X, y, ctx dosyaları eski sürümle **birebir aynı** (karşılaştırıldı: `True`); 8-gram örtüşmesi eski eğitimle val 2/4930, test 1/4930 → genişletilmiş eğitimle **aynı** (genişleme yeni örtüşme eklemedi; kalanlar kitabın doğal tekrar eden deyimleri). Unigram (`uni_tr`) ve PPMI-SVD gömmeler yalnız eğitim bloklarından.

**Yalnız-veri kazancı (aynı hiperparametre, eski gömme dosyadan birebir geri alındı):** val CE 4.8242 → **4.6370** (−0.1872) [Kesin]. Gömmeleri genişletilmiş eğitimden yeniden hesaplamak −0.0023 daha (4.6347); λ_F'yi 3000→9000 val ile yeniden seçmek −0.0013 daha (4.6334). **Not:** gömme farkı (0.0023) tek tohumda gürültü sınırında [Muhtemel]; ilke olarak "gömme de eğitim verisinden" seçildi.

λ_C×λ_F ızgarası (val): λ_C ∈ {25,50,85,170} × λ_F ∈ {1000,3000,9000}: en iyi (85, 9000) 4.6334; λ_C=85 veri 2.84× büyüse de hâlâ en iyi; g_F ∈ {0.10,0.15,0.20,0.25} düz (4.6334–4.6336) [Kesin].

## 2. Aşama B — sayıma duyarlı C-blok ridge

λ_i = λ₀ · (1 + nᵢ/κ)^(−α) (nᵢ = slotun eğitim satırı sayısı; α>0: sık slot daha az büzülür, α=0 sabit λ). 120 konfigürasyon (λ₀ ∈ {85…1360}, α ∈ {0.25…1}, κ ∈ {20,100}, λ_F ∈ {3000,9000,27000}), seçim yalnız val.

| Yapılandırma (val, birleşik model) | CE | Sabit λ=85'e fark (eşli) |
|---|---|---|
| Sabit λ=85 | 4.6333 | — |
| **α=+0.25 (varsayım: sık slot az büzülsün)** | 4.6342 | **+0.0009 ± 0.0006 (daha kötü)** |
| α=+0.5 / +0.75 / +1.0 (en iyi λ₀, κ=100) | 4.6390 / 4.6396 / 4.6406 | daha kötü (+0.006…+0.007) |
| α=−0.25, λ₀=85 (ters yön: sık slot daha çok büzülür) | 4.6320 | −0.0013 ± 0.0008 (z=−1.7) |
| α=−0.25, λ₀=60 | 4.6312 | −0.0021 ± 0.0007 (z=−2.8, ~20 konfigürasyon arasından seçildi) |

Etkin serbestlik derecesi df = tr(A_CC(A_CC+Λ)⁻¹) (kelime / çift / sabit slot): sabit **371.4 / 367.9 / 1.0 (toplam 740.3)**; α=+0.25: 390.1 / 384.0 / 1.0 (**775.2**); α=−0.25: 351.4 / 351.8 / 1.0 (704.2).

**Karar: B reddedildi [Kesin].** Hipotez edilen yön (sık slotlar az büzülsün) val'ı kötüleştirdi; ters yön yalnız ≈0.002 kazandırıyor ve seçim yanlılığı sınırında. Çift slotları çoğaldıktan sonra (aşama D rejimi) aynı deney tekrarlandı: α>0 yine kötü (4.6140–4.6204 vs 4.6108), α<0 −0.003 (4.6079–4.6081) — tutarlı ama küçük; "zorla birleştirme" yapılmadı, nihai modelde sabit λ var. [Muhtemel] açıklama: F bloğu ve geç-Q karışımı sık slotların sinyalini zaten taşıyor; ridge büzülmesinin asıl işlevi nadir slotların gürültüsünü bastırmak.

## 3. Aşama C — Q sıfır-kütle düzeltmesi

Q(w) = (1−b)P_w + b·Σa_wv P_v tablosunda hedefe sıfır olasılık: ham P_w'de val hedefinin (w,y) çifti eğitimde görülmeyen satır oranı **%21.8**; kNN karışımından sonra Q'da hedefe sıfır kütle: V51b verisinde **%14.9**, V53 verisinde **%7.8** (satır içi sıfır sütun oranı %63.9).

Düzeltme (epsilon eklemek **değil**): q ← λ_w·q + (1−λ_w)·π, **λ_w = n_w/(n_w+κ)** (frekansa duyarlı Dirichlet geri çekilme), **π = Kneser-Ney devam eden unigram** (kaç farklı w₁'den sonra geldiği; “+1”li). Tablo hem geç-karışım Q_havuz'unda hem 6 K-tablosunda (5 OOF + tam) uygulanır; OOF'ta π fold sayımından hesaplanır → sızıntı yok; κ yalnız val.

| Varyant | val CE |
|---|---|
| Düzeltme yok (A sonrası) | 4.6334 |
| κ=3, KN-π | **4.6305** |
| κ=10, KN-π | 4.6314 |
| κ=3, eğitim unigramı | 4.6311 |
| κ=30, KN-π | 4.6359 (kötü) |

Düzeltme sonrası hedefe sıfır kütle **%0** [Kesin]. Kazanç küçük (−0.0029) ama yönü hem Python kopyasında (yalnız geç-karışım −0.0012) hem C++'ta (kapı dahil −0.0029) tutarlı → **kabul** (Top-1 16.29→16.24 val, önemsiz). Model-sonu okumada zaten (1−a)π tabanı olduğu için sıfır-kütle etkisi yapısal olarak sınırlıydı [Muhtemel].

## 4. Aşama D — bağlam genişletme (2/3/4 kelime)

Python kopyasında ölçüm (F sabit; C bloğu slotları değişir; üst-N / sayım≥3 satırlarında λ_çift=85, sayım≥2 / ≥1 satırlarında 50), seçilenler C++'ta uygulandı (aşağıda). Kapsam = val satırında slotun var olma oranı. val CE = okuma + Q karışımı sonrası.

| C bloğu | Slot | Kapsam | val CE |
|---|---|---|---|
| Kelime + top-2000 çift (V51b yapısı, A+C sonrası) | 4,001 | %45.7 | 4.6322 |
| + w_{t−2} tek-kelime slotu | 6,003 | %100 | **4.6531 (kötü)** |
| top-5000 çift | 7,002 | %57.0 | 4.6252 |
| top-10000 çift | 12,002 | %65.2 | 4.6191 |
| tüm çiftler sayım ≥3 | 9,502 | %61.8 | 4.6213 |
| tüm çiftler sayım ≥2 (λ_çift=50) | 15,166 | %68.0 | 4.6142 |
| **tüm çiftler sayım ≥1 (λ_çift=50)** | **39,947** | **%78.2** | **4.6108** (Python) / **4.6090** (C++, gerçek kapı+Q) |
| + 3'lü (w₃,w₂,w₁) top-5000, sayım≥2 | +5,000 | %17.5 | 4.6080 (−0.003) |
| + 3'lü top-20000 | +11,312 | — | 4.6080 |
| + 3'lü 3000 + 4'lü 3000 (taban 2000 çiftle) | 10,002 | 3-gram %15.3, 4-gram %3.2 | 4.6285 vs 4.6322 (−0.004) |

λ_çift taraması (Python, sayım≥1, λ_F=9000): 25→4.6165, 35→4.6101, **50→4.6108**, 170→4.6260, 340→4.6394 (sayım≥2 için de 15→4.6274, 25→4.6210, 35→4.6141, 50→4.6142, 170→4.6278); λ_kelime=85 en iyi (60/120/170 daha kötü); C++ doğrulaması: λ_çift 35/50/70 → 4.6097/4.6094/4.6115 (λ_F=9000), λ_F=3000 ile 4.6090 (seçilen), 27000 ile 4.6153.

**Kabul edilen D = tüm çift slotları (sayım ≥1), λ_çift=50, λ_kelime=85, λ_F=3000** (val en iyisi; hepsi tek ortak ridge). 3'lü/4'lü slotlar yalnız ≈0.003–0.004 verdi, bellek ve süre artırıyor, "daha uzun bağlam daha iyi" varsayımı **desteklenmedi**: kazancın büyük kısmı 2-kelimelik bağlamın *kapsamından* (%45.7→%78.2) geliyor [Kesin]; 4-gram bağlam val satırlarının yalnızca %8.2'sinde görülmüş [Kesin]. İkinci kelime tek başına slot olarak (w_{t−2}) **zarar verdi** (+0.021) [Kesin].

Uygulama (`cpp/tek_hamle_v53.cpp`): `V53_PMIN=1` (çift alt sınırı), `V53_LAMP=50`. Önceki yoğun `A_CC` (mc×mc, mc=40k için 12.8 GB) yerine **yalnız son-kelime grup blokları** saklanıyor (55 MB); A_CC yapısı gereği blok-köşegen olduğundan grup Cholesky matematiksel olarak aynı çözüm. En büyük grup 882 slot. Regresyon: eski konfigürasyon yeni kodla **4.8242 / 118.91** üretiyor (bit-aynı), ayrıca daha hızlı (12.7 s vs 18.5 s toplam) çünkü yoğun Gram yok [Kesin].

**Bedel [Kesin]:** tüm çiftler = C blok ağırlığı 4,001×2001 → 39,946×2001 (≈32 MB → ≈320 MB float32); tepe RAM 7.3 GB. Hafif alternatif: **sayım ≥2** → 15,165 slot, val 4.6129 (+0.0035), model hazır 9.2 s, RAM 3.6 GB; sayım ≥3 → 9,501 slot, 4.6173, 7.7 s, 3.3 GB. Nihai seçim en iyi val (sayım ≥1).

## 5. Aşama E — çıkış dönüşümü

Mevcut: P=(1−a)π + a·max(z,ε)/Σmax(z,ε). Ölçüm: val'de z≤1e-5 sütun payı **%51.2**, z<0 payı %35.2, **hedef sütunu kırpılan satır %4.2**. Tüm alternatifler negatif olmayan, satır toplamı 1 (sapma <1e-15), NaN/Inf yok (assert); (a, α_Q, parametre) val ile seçildi.

| Dönüşüm | val CE | Top-1 / Top-5 (val) |
|---|---|---|
| max(z,ε) [mevcut] | 4.6079 | 16.49 / 37.95 |
| softplus(z/t)·t | 4.6111 | 16.59 / 38.05 |
| max(z,ε)^p (ε=1e-5, p∈{.5…1.5}) | 4.6079 (p=1) | — |
| max(z,1e-4)^p, p=1.25 | 4.6076 | 16.69 / 38.19 |
| softmax(z/T) | 5.0092 | 16.71 / 33.17 |
| max(z,0)+ε·exp(min(z,0)/ε) | 4.6102 | 16.49 / 37.95 |
| (relu(z)+ε)^0.75 | 4.7055 | — |

**Karar: E reddedildi [Kesin]** — mevcut kırpma CE'de en iyi/eşit; en iyi alternatif (p=1.25) CE'de −0.0003 (gürültü), Top-1'de +0.2 puan. Kırpma "%4.2 hedef satırını zarar verir" gözlemi (V51b denetimi) doğru ama kırpılan değerler zaten ridge kuyruğunun genelleşmeyen kısmı; yumuşak alternatifler bu sinyali geri getirmiyor. (Ayrı not: tablo Python kopyasının α_Q ızgarası biraz daha geniş olduğu için 4.6079, C++ 4.6090.)

## 6. Aşama tablosu

Test PPL yalnız referans ve nihai satırda (ara aşamalarda test kullanılmadı; bkz. 8. bölüm uyarı). Top-1/Top-5 ara satırlarda **val**, referans/nihai satırda test (+val parantezde).

| Aşama | Validation CE | Test PPL | Top-1 | Top-5 | Model hazır süresi | Durum |
|---|---|---|---|---|---|---|
| 0 V51b referans | 4.8242 | 118.91 | 16.12 % (val 15.10) | 37.03 % (val 35.58) | 2.8–3.1 s | referans (yeniden üretildi, bit-aynı) |
| A1 +veri (eski gömme, aynı hp) | 4.6370 | — | 16.37 % (val) | 37.55 % | ~5–8 s* | kabul (yalnız-veri −0.1872) |
| A2 +gömme eğitimden | 4.6347 | — | 16.35 % | 37.41 % | ~5 s* | kabul |
| A3 +λ_F=9000 (val) | 4.6334 | — | 16.29 % | 37.45 % | ~9 s* | kabul |
| B sayıma duyarlı λ (α>0) | 4.6342 (en iyi α>0) | — | — | — | — | **reddedildi** (A3'ten kötü) |
| C Q Dirichlet+KN (κ=3) | 4.6305 | — | 16.24 % | 37.47 % | ~6 s* | kabul |
| D tüm çiftler sayım≥1, λ_çift=50, λ_F=3000 | **4.6090** | **98.09** | **18.21 %** (val 16.63) | **39.68 %** (val 37.99) | **21.7–25.2 s** | **kabul — nihai** |
| E çıkış dönüşümü | 4.6079–4.6111 (Python) | — | — | — | — | **reddedildi** (kazanç yok) |

\* Ara aşama süreleri yoğun-A_CC ikilisiyle ve soğuk/ısınmış önbellekle ölçüldü, gürültülü; yalnız referans ve nihai satırı temiz tekrarlı ölçüm (aşağıda).

## 7. Süre dökümü (temiz, ardışık, başka iş yokken; 2 tekrar)

| Kalem | V51b ref (39,840 satır) | V53 nihai (113,046 satır) |
|---|---|---|
| Veri okuma (sayfa önbelleği sıcak) | 0.02 s | 0.05–0.06 s |
| Özellik (V51 K-tabloları + V49 + V39 havuz + F) | 2.3–2.5 s (K-tablo 1.5–1.7) | 4.5–4.6 s (K-tablo 2.3–2.6) |
| Q/OOF (K tabloları 5 fold + tam, KN-π dahil) | yukarıya dahil | yukarıya dahil |
| Gram/RHS (C^TC blokları, C^TF, C^TY, F^TF) | 0.10 s | 4.2–4.5 s |
| Cholesky(C blokları) + Y0 + X + Schur hazırlığı | 0.25–0.26 s | 11.5–13.6 s |
| Saf ağırlık üretimi (Schur çözümü + W_C=Y0−X W_F) | 0.12–0.14 s | 1.1–2.9 s |
| **Model hazır (toplam)** | **2.8–3.1 s** | **21.7–25.2 s** |
| Val+test değerlendirme (logit + PPL) | ~0.1–0.3 s | ~0.2 s |
| Program toplamı (rapor amaçlı eğitim logitleri vs. dahil) | 10.3 s | 68–73 s |

**Süre artışı ≈ 8×**: veri 2.84× + çift slotları 10× → C bloğu işleri (Gram, grup Cholesky, Y0 = 40k×2001 sağ taraf, U=A_FC·Y0) baskın. Aşama D yerine "A+C" ile kalınırsa ≈5–9 s. [Tahmin] Y0'ın yalnız seyrek satırlar için hesaplanması ve C^TY'nin seyrek tutulması bu süreyi yarıdan aza indirebilir (denenmedi).

## 8. Dürüst sınırlar

- **[Kesin] Test'in görünmesi:** C++ programı her koşuda test sütununu da yazdırıyor; seçim kararlarında kullanılmadı (tüm seçimler `V50_SECIM … dogr CE=` val sütunundan veya Python'da yalnız val'dan), fakat ham log çıktılarının bir kısmında test değerleri de görüldü. Nihai test sayısı tek yapılandırma için (kabul edilen D) raporlandı; aday arama test'e göre yapılmadı.
- **[Kesin] Tek kitap, tek bölme, tek tohum.** Val yalnız 4,980 satır; eşli farkın standart hatası ≈0.0006–0.0008 CE. Dolayısıyla < ~0.002'lik farklar (gömme yeniden hesabı, λ_F 3000↔9000, B'nin ters yönü, E) **gürültü sınırında**; A (−0.187), D çift kapsamı (−0.022) ve C (−0.003, tutarlı iki ortamda) dışındakiler güçlü kanıt sayılmamalı.
- **[Kesin]** Val; aynı zamanda (a, ε, α_Q, λ'lar, κ, λ_çift, sayım alt sınırı) seçiminde kullanıldı → val CE iyimser yanlı, test PPL (98.09) bağımsız tahmin; yine de test yalnız 4,980 satır.
- **[Muhtemel] Sözlük sabit** (eski eğitimden 2001 kelime); genişletilmiş eğitimde UNK %7.5 → sözlüğü yeniden seçmek ayrı bir deney olurdu (val/test id'leri değişeceği için "val/test aynı" kuralıyla çakıştığından yapılmadı).
- **[Kesin]** Model boyutu ve RAM arttı (bölüm 4); "tüm çiftler" ile hafif sayım≥2 arası tercih val'a göre yapıldı.
- Yan etki: bir başlangıç kazasında `V52_DUMP=` boş değişkeniyle yazılan ≈100 MB'lık dump dosyaları (`/Fc.f32`, `/Fcv.f32`, `/Fce.f32`, `/ytr.i32` …) sunucunun kök dizinine düştü; silme komutu güvenlik kontrolünden geçmedi, dokunulmadı. Proje etkilenmedi.

## 9. Yeniden üretim

```
bash v53_ogrenme_gelistirme/run_final.sh      # veri (yeniemb) + derleme + nihai koşu
```
Ortam değişkenleri: `V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15` (+ `v51_ek_deneyler_ham/env.sh`), `V53_PMIN=1 V53_LAMP=50 V53_QKAP=3 V53_QPI=1`; komut: `tek_hamle_v53 2 512 1 1 1 0.5 Z 1 Y /tmp/v53A_yeni 85 85 3000`. Referansı yeniden üretmek için aynı komut `V53_*` olmadan ve `/tmp/v8data`, `… 85 85 3000` ile 4.8242/118.91 verir.

Analiz betikleri: `b_countaware.py b2.py b3.py` (B), `c_qsmooth.py` (C ön ölçüm), `d_context.py d2_pairs.py d3_grid.py d4.py` (D), `e_cikis.py` (E), `model53.py` (V51b analitik çözümün Python kopyası; C++ ile CE 4.6333↔4.6334 eşleşti).

## 10. Sorular

1. **Hangi sorunlar çözüldü?** Kullanılmayan %59.6 veri (kasıtlı `[:50000]` sınırı) eğitime alındı; çift bağlamı kapsama sorunu (%45.7→%78.2) yoğun A_CC yerine grup blokları sayesinde 40k slota çıkarıldı; Q sıfır-kütlesi %7.8→%0. Sayıma duyarlı λ ve çıkış dönüşümü ise *sorun olmadığı* ya da hipotez yönünün yanlış olduğu gösterildi.
2. **Kullanılabilir eğitim verisi artışı?** 40,000 → 113,500 token (2.84×; 39,840 → 113,046 satır; kitap payı %32.4 → %91.8). Val/test aynı.
3. **En çok işe yarayan değişiklik?** Veri (A): val CE −0.1872 tek başına; sonra çift slot kapsamı (D): −0.022.
4. **Başarısız olanlar?** B (sayıma duyarlı λ, α>0: +0.0009 kötü), E (çıkış dönüşümleri: kazanç yok, softmax/softplus kötü), D'de w_{t−2} slotu (+0.021 kötü), 3'lü/4'lü slotlar (≈ −0.003, maliyetine değmedi).
5. **Final val CE ve test PPL?** val CE **4.6090**, test PPL **98.09** (test CE 4.5859, Top-1 18.21 %, Top-5 39.68 %).
6. **Model hazırlık süresi?** 2.8–3.1 s → 21.7–25.2 s (≈8×); hafif seçenek (sayım≥2) 9.2 s ile val 4.6129.
7. **Tek çekirdek ve ortak havuz korundu mu?** Evet: tek ortak ridge (C+F birlikte), tek Schur/Cholesky analitik çözüm; backprop/iterasyon/replay/ayrı uzman yok.
8. **Git commit/push?** Bkz. commit mesajı ve son mesaj.
