# Tek Hamle AI — V78: ilerleyen hesaplama akışı ve sonuç seçimi

Dal `claude/formul-avcisi` · taban: V77 (`c0a4cdd`, dokunulmadı) · yeni kod: `cpp/tek_hamle_v78.cpp`, deneyler: `cpp/ilerleyen_deney.cpp`, `cpp/ilerleyen_deney2.cpp` · betik/ham çıktı: `v78_ilerleyen_hesap/`

Etiketler: **[Kesin]** bu oturumda ölçülen/koddan okunan, **[Muhtemel]** güçlü çıkarım, **[Tahmin]** boşluk doldurma.

---

## 0. Kısa sonuç

* **Fikir iki farklı şey söylüyor ve ölçüm ikisini ayırıyor.** (a) "Sonucu bir öncekine ekleyerek türet" (toplayarak ilerleme): çarpmaya karşı **kazanç yalnız** tam sayı, vektör, önbellekte durumda; bellek-sınırlı, rastgele, ondalıklı ya da bağımsız işlemde kazanç yok ya da zarar. (b) "Cebirsel ilişkiyle bir sonucu diğerinden türet" (dağılma, doğrusallık): **gerçek** kazanç; V77'de bunun **küçük** bir yeri bulundu.
* V77 → V78 tam program, 10.001, 7 dönüşümlü çift, medyan: **9.83 s → 9.24 s (−0.59 s, −%6.0)**; 7 çiftin 7'sinde V78 hızlı (oran 0.88–0.98). Test 1.87 → 1.58 s, val 2.49 → 2.16 s. Model hazırlama **değişmedi** (5.24 / 5.43 s: aynı kod, fark gürültü). Bu oturumda V77 referansı 9.83 s ölçüldü (görevdeki 9.41 s farklı makine durumu).
* Aynı ağırlıklar: tüm model ara büyüklüklerinin karmaları (Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F) V77 ile **bit-bit aynı**. Val/test logit satır-karmaları V77 ile aynı (`54dbb30ae95dea9d`, `ab76d69b092a3d11`; `ham/zh78.txt`). 8 α için val Top-1/Top-5, test Top-1/Top-5, seçilen (α₀, ε₀, α_Q), test PPL 262.930 aynı. CE'de yalnız toplama-sırası farkı: val CE ≤2.0e-14 mutlak (≤3.8e-15 bağıl, ≤23 ULP), test CE ≤1.1e-14 (12 ULP).
* **Dürüst sınır:** V78'in aritmetik azaltımı küçük: ≈ −3.4e9 madd-eşdeğeri (V77 toplamının ≈%0.7'si). Test fuse'taki hızlanmanın çoğu işlem sayısından değil, gecikme zincirini kırmaktan ve rank'i v üzerinde vektörleştirmekten geliyor (bu, görevin kendi ölçütüne göre matematiksel azaltım değil). V77'nin model hazırlamasında (F1, K-grafı, FᵀF, slot, Schur) "önceki sonuçtan devam" edebilen ve ölçümde kazandıran bir hesap **bulamadım**.

---

## 1. Önce fikri C++ ile kanıtla

`cpp/ilerleyen_deney.cpp`, `ilerleyen_deney2.cpp` (`-O3 -march=native`, AVX-512). Tüm girdiler çalışma zamanından (taban `b` ve çarpanlar `volatile`/argv), sonuç dizileri doğrulanır (hata olursa program çıkar), her ölçüm 15 tekrarın medyanı, çıktı `ham/deney1_int.txt`, `ham/deney2.txt`.

| durum | A doğrudan | B ilerleyen | sonuç |
|---|---|---|---|
| ardışık 1..100 000, int64, L2 içinde | 0.266 ns/eleman (derleyici vektörleştirir) | AVX-512 `v += 8b`, 1 zincir 0.219 / 4 zincir **0.207** | B ≈ %22 hızlı |
| boyut taraması (int64, 4 zincir AVX-512) | N=256: 0.174 / 2 048: 0.175 / 16 384: 0.237 / 131 072: 0.253 / 1 048 576: 0.376 / 16.7M: 1.126 / 100M: 1.096 | 0.137 / **0.104** / 0.202 / 0.208 / 0.378 / 1.113 / 1.092 | B önbellekte 0.59–0.85×; **bellek-sınırlıda (≥1M) 1.00×** — fark yok |
| ardışık 1..100 000, **double** | 0.236 ns | `s += b` skaler **1.236 ns** (gecikme zinciri, 5.2× yavaş) | A kazanır; ayrıca ondalık adımda **13 491 ULP** kayma (b=0.1; b=1/3'te 13 107 ULP), b tamsayı ise 0 |
| rastgele istenen m sayı | m=100: 0.05 µs · m=10⁴: 1.96 µs · m=10⁵: 40.7 µs | sıralama 0.35 · 89.5 · **1823 µs** + farklardan ilerle 0.06 · 14.9 · 298 µs (farklar yine **çarpılır**) | A ≈50× hızlı (m=10⁵: 40.7 ↔ 2 121 µs) — ilerleyen hesap burada yararsız |
| ortak çarpan: Σ b·xᵢ, m=10⁵ | doğrudan 52.6 µs | b·(Σ xᵢ) **11.5 µs** | **4.6× — gerçek cebirsel ilişki** (dağılma) |
| bağımsız aᵢ·cᵢ | 0.708 ns/eleman | türetilecek önceki sonuç yok | uygulanamaz |
| bir sonuç çok kullanımda (P=b·c, 10⁵ kullanım) | her seferinde 61.5 µs | bir kez + taşı 37.7 µs | 1.6× (ortak alt ifadeyi dışarı alma; derleyici sabitle bunu zaten yapar) |
| **14×2 → 14×6 → 14×8** (m=4M farklı taban, çarpanlar çalışma zamanı) | 3 çarpma: **3.28 ns/b** | (1 çarpma + ×3 çarpma + 1 toplama) **3.03**; (1 çarpma + 3 toplama) **3.27** | fark ≤%8 (yazma-bant sınırlı). Derleme-zamanı sabit çarpanda: **3.36** (derleyici zaten kaydırma/`lea` ile ilişkiyi kullanıyor) |
| aynı, bağımlı zincir | `imul` **1.36 ns/adım** | `add` **0.72 ns/adım** | gecikme 1.9× — ama işlem hattı dolu iken verim aynı |

**Cevap 1 (14×2 → 14×6 → 14×8):** toplam işlem hattında **pratikte eşit** (3.28 ↔ 3.03 ↔ 3.27 ns). Toplama yalnız *bağımlı tek zincirde* çarpmadan hızlı (0.72 ↔ 1.36 ns). Derleyici sabit çarpanlarda zaten ilişkiyi kullanıyor.

**Cevap 2 (100 000 işlem):** ilerleyen hesap **yalnız** şu koşulda kazandırıyor: tam sayı (ya da tam gösterilebilen), AVX-512 şeritlerinde birden çok bağımsız zincir, veri L2'ye sığıyor (N ≲ 131 k): %15–40 kazanç. Bellek-sınırlıda (N ≥ 1M) fark yok. double + ondalık adımda hem yavaş hem hatalı. Rastgele/seyrek istek → doğrudan. Gerçek kazanç **cebirsel** ilişkide (dağılma: 4.6×), "toplayarak ilerleme"de değil.

**C yöntemi (akıllı seçim)** deneyde: seyrek/rastgele → A; ardışık int64 + önbellek → B; ortak çarpan → dağılma. V78 motorunda aynı ilke, ölçülmüş maliyetle çalışır (§6).

---

## 2. V77'nin gerçek matematiğine uygulama — hesap hesap

Her satır için sorular: (1) önceki sonuç ne taşıyor, (2) aynı katkı tekrar mı üretiliyor, (3) bir sonuçtan birkaç farklı sonuç türetilebilir mi, (4) gereksiz çıktı üretiliyor mu, (5) eski durum ne zaman kapatılır, (6) devam gerçekten daha az aritmetik mi.

| hesap | ilişki var mı? | karar (ölçümlü) |
|---|---|---|
| **FᵀF** (AFF) | Sütunlar [1, A, d², d³]: Gram girdileri lineer ilişkili değil (d², d³ eleman-bazlı doğrusal olmayan fonksiyonlar). 256-satırlık blok toplama zaten tek geçiş ilerleyen birikim. Simetrik yarı V73'ten beri. | yeni türetme **yok** |
| **FᵀY** (RF) | One-hot toplamı; V77 sınıf-sıralı. Σ_y RF[:,y] = F'nin sütun toplamı (= slot toplamlarının toplamı) ama tüm sınıflar gerekli. | yeni türetme yok |
| **slot toplamları** (ACF) | Kelime slotu = Σ(o kelimenin çift slotları) + (çift slotu olmayan örnekler). Ok-başı yapıdan çıkan **geçerli** özdeşlik. ≈6.5e7 ekleme (ACF eklemelerinin ~%23'ü, ≈0.05 s), toplama sırası değişir. | geçerli, **uygulanmadı** (kazanç gürültü altı) [Tahmin: çift-slotu kapsama oranı ölçülmedi] |
| **K-grafı, Q** | OOF tablosu f = tam − fold f sayımları **doğrusal**; ama benzerlik **√sayım** üzerinde: Gram(√(C−D)) tam Gram'dan doğrusal türetilemez (karekök). Fold f'de değişmeyen satırlar: bu oturumda numpy ile ölçüldü, değişen satır oranı %63–65 (fold başına 6 273–6 476 / 10 001 kelime); değişenler sık kelimeler olduğundan Gram işinin büyük kısmı bunlarda [Tahmin: iş payı ayrıca ölçülmedi] → yeniden kullanılabilir iş küçük. s(K) lineer kısmı (Σ C E) türetilebilir ama baskın değil. | **türetilemez**; ölçümle gösterildi |
| **U, T** | T = Σ_p aₚaₚᵀ/dₚ + Σ_g s_g x_w x_wᵀ (ok-başı kapalı form) = Z'Z → **dsyrk, madd 1.122e10 → 5.62e9**. Ölçüm: syrk **0.169 s**, dgemm 0.175 s (kazanç yok); T farkı 1.1e-15 bağıl ama **W_F karması değişti**. | doğrulandı, **hızlandırmıyor + bit değişiyor** → varsayılan kapalı (`V78_TZ=1`) |
| **Schur çözümü, W_F** | Tek çözüm; V sağ taraf birbirinden bağımsız sütunlar. | türetme yok |
| **val/test logitleri** | Satırlar bağımsız: zᵢ = f̃ᵢ W_F; zᵢ'yi zⱼ'den türetmek f̃ᵢ−f̃ⱼ yoğun olduğundan 513×V madd'e eşit. | türetme yok |
| **val seçimi (α₀,ε₀)** | z_y ve S_e skalerleri yeterli; V77 zaten S_e'yi saklıyor (S2 yeniden kullanımı) ve doğrulama logit geçişini tek geçişe indirdi (spekülatif). | V77'de uygulanmış |
| **fuse: sm paydası** | **Gerçek ilişki:** p_α(v) = (1−α)pm_v + α q_v **α'da doğrusal** ⇒ sm_α = (1−α)·Σpm + α·Σq; Σpm, pm oluşurken toplanır; Σq_w satır başına **bir kez** Q yapısından (Σval + coef·Σtaban) ⇒ V'lik sm geçişi kalkar (tam softmax paydası korunur, doğrusallıkla elde edilir). | **uygulandı** (`V78_LIN`) |
| **fuse: q dizisi** | q = coef·taban + seyrek: yoğun kısım rank döngüsünde satır-içi, seyrek girdiler sonra düzeltilir ⇒ V uzunluğunda q dizisi yazılmaz/okunmaz. | uygulandı |
| **fuse: pm'siz doğrusal** | p_v = A·Ptr_v + B·g_v + α·q_v (pm dizisi ve bölme yok). | uygulandı, **ölçümde kaybetti** (§3) → kapalı, otomatik seçici ölçüp eliyor |

---

## 3. Ne gerçekleştirildi (V78)

Hepsi V77'nin tek ana motorunda; yeni model/formül yok; V77 ve eski kaynaklar değişmedi.

1. **`V78_LIN` — dosyalanmış doğrusallıktan sm ve q dizisinin kalkması.** Her val (8 α şeridi) ve test (tek α) örneğinde `sm` geçişi (8 şerit × V × (çarpma+fma+toplama)) ve V uzunluğunda q yazma geçişi yok. Rank sayımı artık bölmesiz ve doğrudan `p_v > p_y'`; test için rank **v üzerinde** vektörleştirilir (8 v/komut). Q satırı toplamları `SQ_w` bir kez O(nnz = 2 131 264) ile hazırlanır.
2. **`V78_S1V` — aşama 1 `S_e` (8 ε) toplamı 4 bağımsız zincirde** (gecikme gizlenir; toplama sırası değişir ⇒ ULP).
3. Fuse döngüleri `schedule(dynamic,16)` (satır başına maliyet kelime desteğine göre 1:500 değişiyor).
4. **Otomatik yöntem seçimi (`V78_AUTO`, §6).**
5. **Kapalı bırakılanlar (ölçülmüş olumsuz):** `V78_FZ=1` (pm'siz doğrusal, gather'lı seyrek düzeltme: dizi başına seyrek ≈2 100 girdi × gather maliyeti kazancı yedi; probe: 4.2 ms ↔ 5.1 ms / 64 satır), `V78_TZ=1` (T=Z'Z, kazanç yok + bit), `V78_MALLOPT=1` (hata sayısı 757 k → 496 k, ama RSS +400 MB ve SYS ↑, duvar kazancı yok).

### İşlem muhasebesi (soru 5–6) [Muhtemel: koddan sayım; ayrıca profil sayaçları `ham/pr3.txt`]

| kalem | V77 | V78 | net |
|---|---|---|---|
| val sm geçişi (8 şerit×V×(mul+fma+add)) | 36 852×8×10 001×4 = 1.18e10 flop | 0 | −1.18e10 |
| val SPM toplamı (pm oluşurken) | 0 | 3.7e8 | +3.7e8 |
| val seyrek düzeltme (ort. ≈2 170 girdi × 2 değerlendirme × 8 şerit) | 0 | ≈ +5.1e9 | +5.1e9 |
| test sm geçişi | 1.5e9 | 0 | −1.5e9 |
| test SPM + seyrek düzeltme (2 099 girdi) | 0 | 3.7e8 + 6.2e8 | +1.0e9 |
| q yoğun yazma geçişi (V mul+store, val+test) | 7.4e8 mul + 7.4e8 store | mul satır-içi (aynı sayı), **store yok** | −7.4e8 bellek yazma |
| SQ_w hazırlığı | 0 | 2.1e6 | +2e6 |
| **net aritmetik** | | | **≈ −6.8e9 flop ≈ −3.4e9 madd** (V77 toplamının ≈%0.7'si) |

Yani gerçekten **kaldırılan çarpma/toplama** küçük. Test fuse'ta ölçülen 0.84 → 0.35 s düşüşün asıl kaynağı: sm toplamının 4 çevrimlik bağımlı zinciri (`sm += p`, V adım) ve rank geçişinin skalerden v-üzerinde 8'li vektöre geçmesi; aritmetik sayısı neredeyse aynı. Eklenen durum yönetimi maliyeti: seyrek düzeltme (yukarıda), blok başına probe (≈9 ms, bir kez), `SQ` (V×8 B = 80 KB), `SvAll` V77'den.

---

## 4. Matematiksel doğruluk

`dogrula.sh`, `olcek.sh`, `fark.py`. V77 ↔ V78 aynı veri/bayrak, `V76_DEBUG=1 V53H_HASH=1`.

| büyüklük | 2.001 | 5.001 | 10.001 | 20.001 (karmasız) |
|---|---|---|---|---|
| Fc, AFF, RF, ACF, RC, Y0f, X, T, U, W_F karmaları | aynı | aynı | aynı | — (RAM) |
| 8 α val Top-1/Top-5, test Top-1/Top-5 | aynı | aynı | aynı | aynı |
| seçilen α₀, ε₀, α_Q | aynı | aynı | aynı | aynı |
| 8 α val CE (15 hane) maks | 9.8e-15 mutlak (2.2e-15 bağıl, 11 ULP) | 2.0e-14 (3.8e-15, 22) | 2.0e-14 (3.8e-15, 23) | 3.0e-14 (5.4e-15, 34) |
| test CE | 0 | 0 | 1.07e-14 (12 ULP) | 0 |
| `lin_table` (seçim) maks | 7.0e-14 (79 ULP) | 8.0e-14 (90) | 2.9e-13 (327) | 6.7e-13 (754) |
| test PPL | 88.893 | 173.827 | **262.930** | 252.531 |

* İlk farklı ara büyüklük: **CE toplamı ve `lin_table`** (toplama sırası: sm doğrusallıkla, S_e 4 zincirle). Model ara büyüklüğü: **yok**. W_F aynı.
* `lin_table` yalnız `argmin` için; boşluk 1.3e-4 … 1.0e-3 ≫ 7e-13 ⇒ seçim aynı (V77 `V77_SECIM` satırı).
* Rank: V77 `fl(p/sm) > fl(p_y/sm)`; V78 `p > p_y'` (bölmesiz). İkisi yalnız p ile p_y' arasında <1 ulp fark varsa ayrışır. Ölçüm: 4 veri kümesinde × (8 α val + test) tüm Top-k sayıları **birebir aynı**.
* Mantık: sm = Σp doğrusal ilişkiden ⇒ analitik olarak aynı sayı; yuvarlama farkı 1e-14 bağıl'ın altında.

---

## 5. Gerçek kazanç — V77 ↔ V78

`ab.sh` N=7 dönüşümlü (A77 sonra B78), aynı veri/ayar, 4 iş parçacığı; `ab_ozet.py`, ham: `ham/A77_*`, `ham/B78_*`.

| çift | V77 | V78 | fark | oran |
|---|---|---|---|---|
| 1 | 9.12 | 8.88 | 0.24 | 0.97 |
| 2 | 9.83 | 9.60 | 0.23 | 0.98 |
| 3 | 9.84 | 8.78 | 1.07 | 0.89 |
| 4 | 9.62 | 8.93 | 0.69 | 0.93 |
| 5 | 10.28 | 9.58 | 0.71 | 0.93 |
| 6 | 9.54 | 9.35 | 0.19 | 0.98 |
| 7 | 10.47 | 9.24 | 1.23 | 0.88 |
| **medyan** | **9.83** | **9.24** | **0.69** | **0.93** |

| ölçüm | V77 medyan [min–maks] | V78 medyan [min–maks] |
|---|---|---|
| model hazırlama | 5.24 [4.96–5.68] | 5.43 [4.99–5.56] (aynı kod: gürültü) |
| validation | 2.49 [2.31–2.68] | **2.16** [1.96–2.45] |
| test | 1.87 [1.73–2.15] | **1.58** [1.34–1.70] |
| tam program | 9.83 [9.12–10.47] | **9.24** [8.78–9.60] |
| kullanıcı CPU | 32.4 s | 30.8 s |
| tepe RSS | 1530 MB | 1530 MB |
| test PPL | 262.930 | 262.930 |

**Ablasyon** (aynı oturum, 3 tur dönüşümlü, tam program medyanı): V78 taban 8.74 s; `V78_LIN=0` 9.75 (+1.01); `V78_S1V=0` 8.94 (+0.20); `V78_FZ=1` 8.73 (0); `V78_TZ=1` 8.72 (0); V77 9.28. Gürültü ±0.5 s; LIN'in katkısı dönüşümlü çiftlerle tutarlı (test fuse 0.84→0.35, val fuse 0.66→0.40).

`V78_AUTO`'nun tek maliyeti ≈9 ms (64 satır × 2 yol × 2 tekrar).

---

## 6. Otomatik yöntem seçimi

Motor, val akışının ilk bloğunda, aynı satırlar üzerinde **doğrulanmış-eşdeğer iki fuse yolunu** (pm-dizili ↔ pm'siz doğrusal/gather) 64 satır × 2 tekrarla zamanlar; yalnız %10'dan fazla ucuzsa ikinciyi seçer. Bu veride ölçüm: 4.22 ms ↔ 5.09 ms ⇒ pm-dizili (`V78_SECIM` satırı). Seçim *ölçülmüş maliyete* dayanıyor, varsayıma değil; seçilmeyen yol yan etkisizdir. Doğrudan/ilerleyen/seyrek/örtük hesap kararı V77'de zaten yapısal: örtük W_C (V64, eval), seyrek U (V72), kapalı-form ok-başı (V63), spekülatif tek geçiş + kesin yedek (V77; 20.001'de yeniden-hesap %36 olsa da ölçüm yedekten ucuz: φ < gemm/(gemm+fuse) ≈ 0.7).

---

## 7. 20.001 ve daha büyük sözlükler

20.001, tek çift (hash'siz, `ham/s20k_*`): V77 70.5 s → V78 **54.8 s**; RSS 6.44 GB; test PPL 252.531 her ikisinde. Eval kalemleri: test fuse 7.17 → 2.78 s, val fuse 4.73 → 2.69 s, toplam val+test 40.2 → 30.0 s; model hazırlama 30.0 → 24.4 s **aynı kod ile** (gürültü: tek çift, iki ayrı makine durumu) — bu yüzden 20.001 için kanıtlanan kazanç **fuse kalemleri**: fuse maliyeti n·V ile büyür ve V78'in kazancı da onunla büyür (10.001'de −0.8 s, 20.001'de ≈ −6.4 s fuse). 50k/100k: gerçek veri yok; fuse payı n_val·V ile ölçeklendiği için yapısal olarak faydalı olmalı [Tahmin], ölçüm yok.

---

## 8. Soruların cevabı

1. **14×2 → 14×6 → 14×8:** pratikte eşit (3.28 ↔ 3.03 ↔ 3.27 ns/b); toplama yalnız bağımlı zincirde hızlı (0.72 ↔ 1.36 ns); derleyici sabitlerde zaten kullanıyor.
2. **100 000 işlemde ilerleyen hesap ne zaman kazandırır:** tam sayı + AVX-512 çok zincirli + veri L2'de (%15–40); bellek-sınırlıda fark yok; double/ondalık/rastgele/bağımsızda kazanç yok; cebirsel ilişkide (dağılma) 4.6×.
3. **V77'de aynı bilgi yeniden üretilen yerler:** her örnekte V uzunluğunda sm toplamı (α'da doğrusal), V uzunluğunda q dizisi (yapıdan türetilebilir), val logit geçişi (V77'de kaldırıldı), S2 (V77'de kaldırıldı); geçerli ama uygulanmayan: kelime slotu ACF'si = Σ çift slotları.
4. **Önceki sonuçtan devam edebilen hesaplar:** fuse paydası (sm), q dizisi; T=Z'Z doğrulandı ama hızlandırmıyor. Edemeyenler: FᵀF, FᵀY, K-grafı (√sayım), Schur, logitler.
5. **Kaldırılan gerçek işlem:** ≈ −6.8e9 flop (≈ −3.4e9 madd) net; brüt kaldırılan 1.33e10 flop (sm geçişleri); V77'nin toplamının ≈%0.7'si.
6. **Eklenen işlem maliyeti:** seyrek düzeltme ≈ +5.7e9 flop (val 5.1e9 + test 0.6e9), SPM toplamı +7.4e8, SQ hazırlığı 2e6, probe ≈9 ms (toplam ≈ +6.5e9 flop).
7. **Model hazırlama:** ~5.2–5.4 s (V77 ile aynı kod; azalmadı).
8. **Bütün program:** 9.83 → **9.24 s** (medyan, 7 çift); 20.001'de 70.5 → 54.8 s (tek çift).
9. **Ağırlıklar/öğrenme korundu mu:** evet — model ara büyüklüklerinin tümü bit-özdeş, Top-k ve seçimler aynı, CE farkı ≤3e-14 (ULP seviyesi), PPL aynı.
10. **20.001+ yararlı mı:** fuse kazancı n·V ile büyüdüğü için evet (fuse −6.4 s); asıl darboğaz sgemm (≈19 s) ve model hazırlama değişmedi.

---

## 9. Başarısız / uygulanmayan (nedenleriyle)

* **T = Z'Z (dsyrk):** madd yarıya iniyor ama süre aynı (0.169 ↔ 0.175 s) ve W_F karması değişiyor ⇒ ilerleyen/rank-1 yapı var, **kazanç yok**.
* **pm'siz doğrusal rank (FZ):** işlem sayısı azalıyor ama seyrek düzeltme ≈2 100 gather → daha yavaş; otomatik seçici eliyor.
* **K-grafı OOF türetme:** √sayım doğrusal değil; yeniden kullanılabilir iş <%5.
* **ACF kelime slotu roll-up:** geçerli; ≈0.05 s ve bit değişimi → uygulanmadı.
* **MALLOPT:** sayfa hatası −%34, ama RSS +400 MB, SYS ↑, duvar eşit.
* **Küçük fuse blokları (L3'e sığsın diye):** blok 512 → 13.98 s, 1024 → 11.46, 2048 → 10.44, 8000 → 9.09 (`ham/ch*`): sgemm paketleme + eşzamanlama kaybı büyük.
* **Model hazırlamada ilerleyen akışın ölçülebilir kazancı:** bulunamadı.

## 10. Yeniden üretim

```
cd cpp && g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v78 tek_hamle_v78.cpp -L$SL -l:libscipy_openblas*.so -Wl,-rpath,$SL
cd v78_ilerleyen_hesap && ./dogrula.sh && N=7 ./ab.sh && python3 ab_ozet.py 7 && ./olcek.sh s2k /tmp/v59_2001 1 && ./ablasyon.sh
g++ -O3 -march=native -std=c++17 -o ilerleyen_deney ../cpp/ilerleyen_deney.cpp && ./ilerleyen_deney 14 100000
```
Bayraklar: `V78_LIN` (1), `V78_S1V` (1), `V78_AUTO` (1), `V78_FZ` (0), `V78_TZ` (0), `V78_MALLOPT` (0), `V78_PROF=1`, `V78_WFHASH=1`, `V78_TCHECK=1`. `V78_LIN=0 V78_S1V=0` V77 fuse yoluna döner (V77 bayrakları aynen çalışır).
