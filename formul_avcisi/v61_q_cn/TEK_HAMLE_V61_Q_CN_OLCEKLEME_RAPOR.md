# V61: yoğun Q/Cn hesaplarının kaldırılması — sonuç: BAŞARILI (hızlı yol için); kalan V² tabloları belirtildi

Kod: `cpp/tek_hamle_v61.cpp` (V60 + `v61_counts/v61_graph/v61_stab`; `V61_SPQ=0` eski yoğun Q yolu, `V61_QCHECK=1` yeni/eski karşılaştırma), `cpp/test_q_sparse.cpp` (matematiksel ölçekleme testi). V54/V59/V60 dosyaları değişmedi. V60'ın seyrek kNN'i aynen kullanılıyor (eski yoğun K-grafına dönüş yok). Veri/ayar: V59 ortak veri, hafif ayar.

## Q formülü ve çözüm [Kesin]
Tüketici yalnız s(K)[w] = Σ_y Q[w][y]·E(y) (V×64) hesaplıyor (kapı/projeksiyon bunun üzerinden). Q satırı tam olarak:
q_w = norm( (1−b)·C_w/n_w + b·N_w ) , N_w = komşu satırların ağırlıklı karışımı (≤10 satır, **seyrek**), sonra q ← λ q + (1−λ) π (π = ortak dağılım, **rank-1**), yeniden normalize. Yani q_w = α_w·(seyrek parça) + β_w·π (n_w=0 ve komşusuz satırlarda Ptr, o da ortak vektör). Bu **tam eşdeğer** bir ayrışım (yaklaşım değil): s(K)[w] = (λ·Σ_{y∈destek} q_seyrek[y]E(y) + (1−λ)·(π·E)) / toplam, π·E tablo başına bir kez (O(V·64)). Q tablosu hiç oluşmuyor; Cn de yoğun V×V yerine CSR sayım (yalnız sıfır olmayanlar; fold sayımları örneklerden doğrudan).

## Tablo (hafif ayar, aynı oturum; K-grafı ve Q: HASH'siz ilk çift `_1` ve HASH'li çift)
| Ölçüm | Eski V60 | Yeni V61 |
|---|---|---|
| 10.000 Q belleği | 6 × 0,80 GB = 4,8 GB (yoğun V×V double) | **0** (Q hiç oluşmuyor); tablo başına 64 değer + sıfır-olmayan destek |
| Q oluşturma süresi (10.000) | `kq2 Qt tahsis` 8,3–14,5 s + qtab 2,8 s (6 tablo) | s(K) doğrudan: 0,47–0,56 s |
| K-grafı süresi (10.000) | 1,0–1,3 s | ≈0,8–0,9 s (k0 V61 toplamı 1,2–1,5 s − s(K) 0,5 s; aynı çekirdek) |
| 10.000 Cn bellek | 0,4 GB ×2 (Cfull + fold kopyası) | CSR: ~372 k sıfır-olmayan × 8 B ≈ 3 MB/tablo |
| K-tablo+kapı f2 (10.000) | 23,4 s (HASH'li: 18,8) | **2,06 s** (HASH'li: 1,98) |
| Tam model hazır (10.000, HASH'siz) | 44,6 s | **24,0 s** |
| Tam model hazır 5.000 / 2.000 (HASH'li, dump'lı) | 59,2 / 20,7 s | 24,9 / 21,1 s (2.000'de f2 0,81→0,71; fark VM gürültüsü içinde) |
| Tepe RSS (10.000, HASH'siz) | 6,70 GB | 6,39 GB (tepe artık Q değil, sonraki aşamada: yoğun RC/Y0d mc×V) |
| WC/WF sayısal farkı | — | 2.001: WC ≤5,96e-8, WF ≤2,8e-8; 5.001: WC ≤5,96e-8, WF ≤1,2e-8 (ağırlık ölçeği 0,46 / 0,07 → bağıl ≤2,6e-7); hepsi sonlu; bit-eşit değil (aşağıda) |
| Validation CE | 4,503665 / 5,079437 / 5,441941 | **4,503665 / 5,079437 / 5,441941** (6 hane aynı) |
| Rezidu | 1,2e-13 | 1,2e-13 (KABUL) |
| 50.000 fizibilite | yoğun Q 20 GB/tablo (×6=120 GB), Cn 10 GB: **imkânsız** | sentetik çekirdek testi: kNN + s(K) hesabı 83 MB RAM, 20 s* |
| 100.000 fizibilite | Q 80 GB/tablo (480 GB), Cn 40 GB: imkânsız | 142 MB RAM, 70 s* (kNN 54 s + s(K) 16 s) |

\* Sentetik Zipf+küme bigram sayımı, **matematiksel hesap/bellek testi — model eğitimi değildir**. Zamanlar iki arka plan koşusuyla çakışarak ölçüldü (şişik; çakışmasız V60 testinde 100k kNN 29 s). Test, V×V Q tahsisinin ve eski yoğun yola dönüşün olmadığını doğrular (tepe RAM ≈ nnz ile orantılı).

## Sayısal fark nedeni (bit eşitliği)
K-grafı girdileri aynı: `V61_QCHECK` ile 6 tablonun hepsinde komşu listeleri ve ağırlıkları **bit-eşit** (nb farkı 0, |Δwt|=0). İlk fark s(K)'de: eski yol Q'yu double→float yuvarlayıp float sgemm ile V terimi topluyor; yeni yol double'da topluyor. Fark: maks |Δs| ≤7,7e-7 (bağıl ≤1,5e-6), tahmini hata eski yolun float birikimi kaynaklı; yeni değer daha doğru. Aşağı akış (standardizasyon, projeksiyon, kapı, WC/WF) bunu ≤6e-8'e indiriyor. Öğrenme kalitesi: CE aynı. Test PPL ölçülmedi (V53H_TEST=0).

## Cevaplar
1. **Tamamen kaldırılan yoğun matrisler:** 6× Q (V×V double), Cfull ve fold kopyaları (V×V float), Qf32 kopyaları (V×V float ×6), dolayısıyla "kq2 Qt tahsis" ve sıfır-başlatma.
2. **Hâlâ gerekli/yoğun:** Q_havuz tablosu (q1: V×V, 4,4 s @10k), RC (mc×V double) ve Y0f/Y0d (mc×V), U=ACFᵀY0 (mf×V) ve W_C (mc×V) — hepsi çıkış-sınıf sayısı V ile çarpan çıkış matrisleri; n×V Zc/val-logit (V şişmesi). Y0d kurulumu + dgemm: 8,2 s @10k.
3. **Karmaşıklık:** s(K): O(Σ_w |destek_w|·64 + V·64) zaman, O(nnz + V·64) bellek (eski: O(V²·64) zaman, O(V²) bellek ×6). Cn: O(nnz). kNN: O(Σ_k nnz(sütun_k)²). 
4. **50k/100k:** Aynı kod yolu (Q/Cn/kNN) çalışıyor (sentetik testte kanıtlandı); tam model için çıkış tabloları (madde 2) hâlâ V² / mc·V olduğundan 50k+ uçtan uca model henüz mümkün değil.
5. **V60 K-grafı kazancı korundu:** evet (1,0–1,2 s @10k; aynı `knn_sparse.h`).
6. **Öğrenme sonuçları:** Val CE üç boyutta 6 hane aynı; WC/WF farkı ≤6e-8 mutlak; bit-eşit değil.
7. **Sıradaki en büyük darboğaz:** U yoğun (Y0d + dgemm, ~8 s @10k, mf·V·mc ∝ V²) ve çıkış matrisleri RC/Y0 (mc×V); sonra Q_havuz V×V tablosu. Seyrek U (V57/`V53H_U=2`) bu aşamanın adayı; doğrulanmamış olduğundan açılmadı.

## Yetişmeyenler
Seyrek U / RC'nin V61'e bağlanması, Q_havuz'un seyrek+rank-1 gösterimi, artımlı 2k→5k→10k genişletme, test PPL.
