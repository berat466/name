# V81_LOCAL_BENCHMARK — V81'i yerelde (Windows) sıfırdan üretip hız ölçme

**Codex için: bu belge tek başına yeterlidir. Ek soru sormadan aşağıdaki adımları sırayla uygula; sonunda "§9 Codex'in bildireceği çıktı" bölümündeki tabloyu doldur.**

## 0. Ne yapıyor?
10.001 kelimelik Tek Hamle AI modelini **sıfırdan** üretir (özellik üretimi → FᵀF/FᵀY → K-grafı → Schur → W_F), ardından validation + test değerlendirmesini yapar ve **tam program süresini** ölçer. V81 ile V80 aynı koşullarda dönüşümlü (A/B) çalıştırılır.
Kurallar (değişmez): yeni AI/öğrenme matematiği yok; eğitim/validation/test örnekleri azaltılmamıştır (294.816 / 36.852 / 36.852); **hazır ağırlık/checkpoint yüklenmez** (hepsi her koşuda hesaplanır); kaynak dosyalar (V80/V81) **değiştirilmemiştir** (SHA-256 aşağıda); sunucuya özel mutlak yol yoktur.

## 1. Paket içeriği (toplam ≈ 4.8 MB açık, 3.2 MB zip)
| yol | ne |
|---|---|
| `src/tek_hamle_v81.cpp`, `src/tek_hamle_v80.cpp`, `src/knn_sparse_v80.h`, `src/tanhf_vec.h` | V81 ve V80 kaynağı + başlıklar (depoda `cpp/`; **depo kopyasıyla bayt-bayt aynı**, §8) |
| `data_src/` | 10.001 kelimelik veri kümesinin **kaynak verisi**: `ids_{train,val,test}.i32` (kelime kimlikleri, blok×500), `E_table.f32` (10001×64 gömme tablosu), `uni_tr.f64`, `uni_trva.f64` (unigram), `meta_c2.txt` |
| `tools/gen_data.cpp` | kaynak veriden `c2_*_X.f32/ctx.i32/y.i32` dosyalarını üretir (yalnız indeks-bakma/kopya; sayı üretmez). Çıktı **sunucudaki `/tmp/v59_10001` ile bayt-bayt aynıdır** (SHA-256 doğrulanır) |
| `compat/` | derleyiciye `-include` ile önceden eklenen uyumluluk katmanı (BLAS adı eşleme, AVX2 taklidi, CPU koruması, Windows kısa devreleri). V81 kaynağını değiştirmez |
| `CMakeLists.txt` | GCC/Clang (MSYS2 UCRT64) için derleme; hem AVX-512 hem AVX2 ikilileri |
| `RUN_ALL.ps1` | Windows tek komut: MSYS2 kur → derle → veri → doğrula → hız |
| `scripts/*.sh`, `scripts/*.py` | `tum_adimlar.sh` (hepsini yapar), `run_v81.sh`, `verify.sh`, `verify_full.sh`, `bench.sh`, `make_data.sh`, `pick_isa.sh`, `env_v81.sh` |
| `reference/` | `expected_v81.json` (beklenen CE/PPL/Top-k/karma), `data_sha256.txt`, `kaynak_sha256.txt`, sunucu referans günlükleri, Linux doğrulama özeti |
| `MANIFEST_SHA256.txt` (yalnız zip içinde) | pakette bulunan her dosyanın SHA-256'sı |

### Verinin kökeni (soru 2)
`v59_genisleyebilir/hazirla_v59.py` (depoda): Project Gutenberg (kamu malı) pg1342 + pg11 + pg2701 metinleri, 500-token bloklar, `numpy.random.default_rng(0)` permütasyonu, %80/%10/%10 bölme (592/74/74 blok), kelime kimlikleri **eğitimden** frekansa göre (id≥V → 0 = `<unk>`), V=10001, gömme = eğitimden PPMI-SVD(64) (`svds`, `v0=ones`), tüm V boyutlarında aynı. Ham metinler pakete **konmadı**; bunun yerine `hazirla_v59.py`'nin ürettiği nihai veri (ids + gömme + unigram, 4.1 MB) verilir ve `gen_data` ile eğitim/val/test dosyaları **bit-özdeş** yeniden üretilir. Sözlük dizgileri (kelime metinleri) programa gerekmez (program yalnızca tamsayı kimlikleri okur). Bölmeler = ids dosyalarının kendisi; tohumlar = `rng(0)` (veri hazırlama), `V=10001`, `BL=500`.

## 2. Gereksinimler
* Windows 10/11 x64, ≥ **8 GB boş RAM** (normal koşu tepe ≈ 1.6 GB; `verify_full` tam karma modu ≈ 6.2 GB), ≥ 2 GB disk (derleme + `data/` ≈ 190 MB), **internet** (MSYS2 + paketler için bir kez).
* Derleyici/kütüphaneler (MSYS2 UCRT64 ile otomatik kurulur): GCC (C++17, GNU uzantıları), CMake ≥ 3.16, Ninja, **OpenBLAS (standart paket, LAPACK dahil)**, OpenMP (libgomp), Python 3.
* **MSVC desteklenmiyor** (kaynak GCC'ye özgü: `bits/stdc++.h`, `__attribute__`, `__builtin_*`; OpenMP 2.0 yetersiz). CMake bunu açıkça reddeder.
* Sunucuda kullanılan: Linux x86-64, glibc 2.39, g++ 13.3.0 `-O3 -march=native -fopenmp -std=c++17`, scipy'nin ön-ekli OpenBLAS'ı (`scipy_openblas`, SkylakeX çekirdekleri), 4 CPU.

## 3. Tek komutla çalıştırma (Windows)
```powershell
# 1) paketi bosluksuz bir yola ac:  C:\V81_LOCAL_BENCHMARK
cd C:\V81_LOCAL_BENCHMARK
Set-ExecutionPolicy -Scope Process Bypass
.\RUN_ALL.ps1                     # 5 cift, 4 is parcacigi, ISA otomatik
# secenekler:  .\RUN_ALL.ps1 -Pairs 7 -Threads 8 -Isa avx2    |   .\RUN_ALL.ps1 -SkipSetup
```
`RUN_ALL.ps1` şunları yapar: MSYS2 yoksa `winget` ile kurar → `pacman` ile gcc/cmake/ninja/openblas/python → `scripts/tum_adimlar.sh` (CMake derleme → `th_gen_data` ile veri üretimi + **SHA-256 doğrulaması** → `verify.sh` → `bench.sh`).
**Bu PowerShell betiği ve Windows'ta çalıştırma bu pakette HİÇ çalıştırılmadı** (hazırlama makinesi Linux; PowerShell yok). Hata olursa §4'teki elle yol (aynı bash betikleri) kullanılır — bash betikleri Linux'ta uçtan uca test edildi.

## 4. Elle yol (RUN_ALL.ps1 hata verirse; MSYS2 "UCRT64" terminalinde)
```bash
pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-openblas mingw-w64-ucrt-x86_64-python
cd /c/V81_LOCAL_BENCHMARK
THREADS=4 bash scripts/tum_adimlar.sh 5
```
Adım adım: `cmake -S . -B build -G Ninja -DTH_ISA=auto && cmake --build build` → `scripts/make_data.sh build/bin data/v59_10001` → `scripts/verify.sh build/bin data/v59_10001 <avx512|avx2>` → `scripts/bench.sh build/bin data/v59_10001 <avx512|avx2> 5`. WSL2/Linux'ta: `sudo apt install g++ cmake ninja-build libopenblas-dev python3` sonra `bash scripts/tum_adimlar.sh`.

## 5. Kesin çalıştırma komutu (V81 varsayılan hızlı yol)
`scripts/run_v81.sh` bunu yapar (sunucuyla **aynı** ortam; `scripts/env_v81.sh`):
```
V33_BLAS=1 V36_C0=1 V37_BIAS=1 V39=1 V39_BETA=0.9 V40_STAGES=1 V40_W=512 V46=1 V46_ONLY0=1 V46_RHOS=-0.25 V49=2 V50=1 V51=1 V50_RHO=-0.25
V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15 V53_PMIN=2 V53_LAMP=50 V53_QKAP=3 V53_QPI=1
V75_STREAM=1 V63_ARROW=2 V64_IMPL=1 V64_SPQ=1 V64_COMPACT=1 V72_UCSC=1 V73_RHS=1 V73_SYRK=1 V53H_LEAN=1 V53H_FAST=1 V53H_TEST=1
OMP_NUM_THREADS=4 OPENBLAS_NUM_THREADS=4
tek_hamle_v81_<isa> 2 512 1 1 1 0.5 Z 1 Y <veri_dizini> 85 85 9000
```
Programın kendi varsayılanları (V76–V81) ayrıca açılır (AVX-512/8-alfa, spekülatif tek geçiş, bölmesiz rank, sıfırlamasız tahsis, doğrusal s(K), tam−fold sayım vb.); çıktıdaki `V81_YOL|…`, `V80_YOL|…`, `V78_YOL|…` satırları bunları gösterir. Zamanlama için `V78_AUTO` varsayılan (açık) bırakılır (sunucu ölçümüyle aynı); **doğrulama** koşusu `V78_AUTO=0` kullanır (zamanlamaya bağlı fuse yolu seçimi son hane CE farkı üretebildiği için).
Çıktıda aranacaklar: `V53H_MODEL_HAZIR|<s>` = model (ağırlık) hazırlama süresi; `TH_WALL_SECONDS=` = tam program süresi (betik ölçer); `V53H_SONUC|VAL…`, `V53H_SONUC|TEST…`.

## 6. ISA (AVX-512 / AVX2) ve güvenlik
* CMake, derleme makinesinde `__builtin_cpu_supports` ile AVX-512 (F/CD/BW/DQ/VL) sınar (`ISA_SECILEN.txt`). Ürettiği ikililer: `tek_hamle_v8{0,1}_avx512` (`-mavx512f -mavx512cd -mavx512bw -mavx512dq -mavx512vl -mavx2 -mfma -mbmi2`) ve `tek_hamle_v8{0,1}_avx2` (`-mavx2 -mfma -mbmi2`). `pick_isa.sh` desteklenen sürümü seçer.
* **Her ikili başlangıçta CPU'yu kendisi sınar** (`compat/th_compat.h`): AVX-512 ikilisi destekli olmayan CPU'da **çalışmaz**, `TH_HATA` yazıp 97 koduyla çıkar. AVX2 ikilisi AVX2+FMA ister.
* AVX2 yolunda kaynaktaki `_mm512_*_pd` komutları **skaler, aynı-işlemli taklitle** (`compat/th_avx2_emu.h`; `fmadd`→`std::fma`, `max`/karşılaştırma Intel anlamı, `reduce_add` GCC sırası) derlenir; ayrıca AVX-512 elle yazılmış blok çözücüsü (`h_solve_avx512`) kaynağın kendi `#ifdef __AVX512F__` koşuluyla AVX2'de devre dışıdır (skaler `h_solve_fast` kullanılır).
* **Matematiksel fark (Linux'ta ölçüldü, bu makine AVX-512'li):** AVX-512 ve AVX2 ikilileri **aynı** karmaları üretir (Fc, AFF, RF, ACF, RC, Y0f, X, U, **W_F**, s(K) ve graf karmaları, CE, PPL, Top-k) — hız farkı var: AVX2 ≈ %20–25 daha yavaş (§7).
* Başka CPU (ör. ARM) desteklenmez.

## 7. Beklenen sonuçlar (referans)
### 7.1 Doğruluk (bu değerler **KATI** karşılaştırılır; `reference/expected_v81.json`)
| değer | beklenen |
|---|---|
| n_eğitim / n_val / n_test | 294816 / 36852 / 36852 |
| Validation CE / PPL | **5.441941** / 230.890 (Top-1 0.1394, Top-5 0.3172) |
| Test CE / **PPL** | 5.571887 / **262.930** |
| Test Top-1 / Top-5 | **0.1370 / 0.3112** |
| seçilen okuma α / ε / α_Q | 0.85 / 1e-05 / 0.3 |
| W_F karması (`V78_WFHASH`) | `d54c462f219c825b` |
| s(K) tablo karmaları (6) | `734c4478eb4aabd3, dd99574892a97762, dd37148d67343820, 4880fe3ac6a23ddc, 836cac92ede66d52, 95f5a3bfe61d9c9e` |
| graf (nb/wt/bw) karmaları | `reference/expected_v81.json` içinde |
| tam karma modu (`verify_full.sh`, RAM 6.2 GB) | Fc `4efe8a827d220399`, AFF `26401df5ea0b1dea`, RF `18b4555f8bd71339`, ACF `8ea5f941b27af912`, RC `2092be167205cbbb`, Y0f `75427dcfc6ac872c`, X `e2858fa1ad353f51`, U `bc55ec1d32364d8d`, W_F `d54c462f219c825b` (T `5eb9247a5661ec43` sunucu BLAS'ına bağlı; Debian OpenBLAS 0.3.26'da `ebc5f198a97d4e1b` — **W_F aynı kaldı**) |
`check_ref.py` durumları: **KATI ESIT** (hepsi birebir) / **SAYISAL-YAKIN** (karmalar farklı ama test PPL bağıl fark < 2e-3 ve val CE mutlak fark < 5e-3: platform libm/BLAS toplama sırası) / **HATA**.
**Windows için beklenti [Tahmin, ölçülmedi]:** `tanhf` için kaynağın kendi öz-denetimi (`tv_selftest`) glibc'ye karşı yazılmış; UCRT'de başarısız olabilir → libm `tanhf`'e düşer (`V54_TANH|oz-denetim BASARISIZ` satırı) ve OpenBLAS sürümü farklıdır. Bu durumda karmaların farklı, ancak CE/PPL'nin (6 hane) aynı veya çok yakın çıkması beklenir; bu **hata değil**, rapora yazılacak bir bulgudur.

### 7.2 Hız (sunucu = yalnız bağlam; yerel ölçümle doğrudan kıyaslama!)
Sunucu (4 CPU Xeon VM, AVX-512), aynı oturum A/B medyanları: model hazırlama V80 **4.32 s** → V81 **3.99 s**; tam program V80 **7.63 s** → V81 **7.46 s** (V81, V80'e göre ~0.2–0.4 s hızlı; gürültü ±0.3 s). Peak RSS ≈ 1.53 GB.
Bu paketle Linux'ta (Debian OpenBLAS 0.3.26, aynı VM) ölçüm, 5 çift: **AVX-512:** tam program V80 7.12 s → V81 6.91 s, model 4.16 → 3.99 s; **AVX2 (taklit):** tam program V80 8.73 s → V81 8.51 s, model 4.84 → 4.60 s (`reference/`, `results/`). Test PPL her koşuda 262.930.
Modelin ürettiği hesap (aritmetik) V80 ve V81'de **aynıdır**; V81'in farkı gereksiz bellek yazması/veri yapısı kaldırmasıdır (sıfırlamasız tahsis, `buildrank`).

## 8. SHA-256 ve kaynak commit (soru 9)
* Kaynak commit: V81 `cpp/tek_hamle_v81.cpp` son değişiklik **`4ca93fae893bf254385c8bf9bd32ef23660366a7`**; V80/başlıklar (`tek_hamle_v80.cpp`, `knn_sparse_v80.h`, `tanhf_vec.h`) son değişiklik `bb81d50b1971ed13f1de226e0b36aa7ce9c16b53`. Paketin kendi commit'i: `git log -1 -- V81_LOCAL_BENCHMARK`.
* Kaynak SHA-256 (`reference/kaynak_sha256.txt`):
  * `9761461e849427538e0720aee118eb0ba985cf8dbd5d182a294b3638dee95aa6` tek_hamle_v81.cpp
  * `d565c0c35ce2a314a554c50fcdbc0d9ad5ff2bc3703a8842b2426a4a2230d42d` tek_hamle_v80.cpp
  * `fe9c78aea2845c9f3af6884cec87510e39c5639408a7973809c54bf2f2618276` knn_sparse_v80.h
  * `c0aea585357c90d84123d02cc7ae4ea570a92912d0e21b67af7702378cdc29d5` tanhf_vec.h
* Üretilen veri SHA-256 (`reference/data_sha256.txt`; `make_data.sh` otomatik doğrular): c2_train_X.f32 `d96df2ed…7dda0`, c2_val_X.f32 `c86327a7…4fb7`, c2_test_X.f32 `0bd1c0fe…3bdf` vb. — tam liste dosyada. Kaynak veri (`data_src/`) ve tüm paket dosyaları: zip içindeki `MANIFEST_SHA256.txt`.
* Bağımlılıklar (paketlenmedi, sistem kütüphanesi): OpenBLAS (MSYS2 `mingw-w64-ucrt-x86_64-openblas`), libgomp (GCC), libstdc++ — sürümleri `results/verify_*.log` başına yazılmaz; `pacman -Q mingw-w64-ucrt-x86_64-{gcc,openblas,cmake}` çıktısını rapora ekle.

## 9. Codex'in bildireceği çıktı
`results/verify_<isa>.log`, `results/verify_ozet.txt`, `results/bench_<isa>_ozet.md` dosyalarını okuyup şu tabloyu doldur:
| alan | değer |
|---|---|
| CPU modeli, çekirdek/iş parçacığı, RAM, Windows sürümü | |
| seçilen ISA (avx512/avx2), derleyici (`gcc --version`), OpenBLAS (`pacman -Q`) | |
| veri SHA-256 doğrulaması (TAMAM/HATA) | |
| verify DURUM (KATI ESIT / SAYISAL-YAKIN / HATA) + farklı alanlar | |
| `tanh_selftest` (geçti / libm'e düştü) | |
| V80 ve V81: tam program medyanı [min–maks], model hazırlama medyanı [min–maks] (5 çift) | |
| test PPL / val CE (V80 ve V81) | |
| tepe RAM (`tepe RSS=` satırı) | |
Ek (isteğe bağlı): `-Threads <fiziksel çekirdek sayısı>` ile bir tur daha; `-Isa avx2` ile AVX2 turu (AVX-512'li makinede).

## 10. Sorun giderme
* `TH_HATA ... AVX-512 desteklemiyor`: `-Isa avx2` veya `TH_ISA=avx2` kullan.
* CMake "OpenBLAS bulunamadı": MSYS2'de `pacman -S mingw-w64-ucrt-x86_64-openblas`; elle yol: `-DTH_BLAS_LIB=/ucrt64/lib/libopenblas.dll.a`.
* Çalışırken DLL bulunamadı: komutları **UCRT64** kabuğunda çalıştır (PATH'te `/ucrt64/bin` olmalı); `RUN_ALL.ps1` bunu `MSYSTEM=UCRT64` ile yapar.
* Bellek hatası: tam karma modu (6.2 GB) yerine `verify.sh` (hafif) kullan.
* `openblas_set_num_threads_local` yoksa uyumluluk katmanı boş taklit kullanır (sonucu değil, yalnız iç-içe BLAS iş parçacığı sayısını etkiler; performans biraz farklı olabilir).
* Süre ölçümü sırasında başka ağır iş çalıştırma; ilk çalıştırmada antivirüs taraması süreyi şişirebilir (ilk çifti at).
