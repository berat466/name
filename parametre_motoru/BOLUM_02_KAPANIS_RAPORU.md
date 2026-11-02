# Bölüm 02 — Kapanış Raporu: İfadeler, Operatörler ve Sabitler

Dal: `claude/formul-avcisi` · Linux x86-64 · 2 Kasım 2026

## 1. Sonuç
**02.001–02.034: 34/34 madde sayısal yolla doğrulandı.** PLAN: 133 🟢 / 42 🔵 / 1224 ❌. Bölüm 03'e geçilmedi.
Bu rapor önceki "KAPANMADI" raporunun yerine geçer; önceki geri alma gerekçeleri aşağıda madde madde kapatıldı (§4) ve **kapatılamayanlar §5'te açıkça listelidir** (bölümün maddelerini etkilemeyen sınırlar).

## 2. Mimari (ana ilke korunuyor) [Kesin]
- **`oz/ifade/num/ifx.pmb`** = sayısal PMX1 → PMA derleyicisi (kanonik ürün, sayısal program). Kural/işlem anlamı **tabloda**: `komutlar.pmk` bölüm 12 (IFADE KURAL TABLOSU: tür özellikleri, operatör→komut eşlemesi, karşılaştırma eşlemesi, dönüşüm adları, yazmaç türü→TYPES kimliği) + bölüm 11 (öncelik) + OPSIG/HOSTSIG imzaları (`komut.elf` dökümü, çalışma anında okunur). Tablo sayıları değişince hiçbir şey yeniden üretilmeden davranış değişir (test: `+`→sub, `*`→add).
- **Yürütme zinciri pm'siz:** `ifx.elf` → `asm.elf` → `coz.elf` → `valid.elf` → `tipdogr.elf` → `gen2.elf` → ELF. C++ ön yüz (`pm ifade`, metin → PMX1) yalnız **isteğe bağlı uyumluluk aracıdır**; PMX1 sayı listesini sayısal derleyici işler.
- **Tür duyarlı üretim (PMT1/OPSIG):** ifx her işlev için `;;PMT1` tür notu üretir (yazmaç türleri sabit; çağrı penceresi `dyn`, geniş değerin ikinci sözcüğü `devam`); `tipdogr` OPSIG/HOSTSIG ile **her test programını** doğrular (kasıtlı bozulan tür notları reddedilir: 69 bozuk notun ≥5'i reddedilmek zorunda).
- **f80** (`long double`): ifx'te sınıf değeri gibi 2 sözcüklü değer (kayıt çifti, sret, değer-parametre); işlemler Bölüm 01 f80 komutlarıyla; sessiz f64'e indirgeme yok. Değişmez sabitler bit-tam.
- **İstisna mekanizması (gerçek çalışma zamanı):** `throw`/`try`/`catch` (tür eşleşmesi: skaler tür kimliği, sınıf için `isa` + türetilmiş→taban; `catch(...)`, `throw;`, değer/başvuru yakalama), `std::exception`/`std::bad_cast`/`std::bad_typeid` (ön söz sınıfları; `what()` sanal). Yazılımsal yayılım: `__ex/__exo` hücreleri, her çağrı sonrası denetim, çerçeve kaydı geri yüklenerek dönüş; yakalanmayan istisna, `noexcept` işlevden kaçan istisna ve `main`'den kaçan istisna = `std::terminate` = TRAP. `dynamic_cast<T&>` başarısızlığı `std::bad_cast`, `typeid(*null)` `std::bad_typeid` fırlatır.
- **Bayt düzeyi bellek görünümü:** motor sözcük adreslidir (adres 0 geçerli sözcük). `reinterpret_cast<unsigned char*/char*>(&nesne)` için **bayt adresli işaretçi** (taint `bp` = asıl nesne boyutu S; bayt adresi = sözcük × S + bayt): okuma `(sözcük >> 8k) & 255`, yazma sözcük oku-değiştir-yaz, aritmetik/karşılaştırma/fark, asıl türe geri dönüş (`P / S`). Dar tamsayılar `reinterpret_cast` bulunan programlarda her okumada kendi genişliğine normalize edilir (işaretli↔işaretsiz aynı boyut punları doğru). Sızıntı güvenli: bayt adresli işaretçi işlev argümanı, `return`, üye/dizi ataması, `void*`, referansa bağlama, adres alma, bileşik atama, `++/--` ile **derleme hatası** (sessiz yanlış okuma yok).
- **Öz-barındırma (sabit nokta):** `ifx.cx`'in PMX1'i → `ifx.elf` → PMA1 → (asm→coz→valid→tipdogr→gen2) → `ifx2.elf`; `ifx2.elf` aynı girdiyi derleyince PMA2 = PMA1 (95 877 satır, bayt-bayt aynı). Bu çalışma gerçek bir hatayı (işlev adı `op…` ile başlayınca operatör sanılıyordu) ve ifx'in sabit tablo sınırlarını ortaya çıkardı, düzeltildi.

## 3. Madde durumu ve kanıt
Her testin yolu: g++ oracle = `ifx.elf` (native) = `ifx.pmb` (pm) PMA'sı bayt-bayt aynı; PMA → `asm.elf` → `coz.elf` → `valid.elf` kabul → `tipdogr` kabul → `gen2.elf` ELF; ELF = pm = svm = g++ çıktısı; ayrıca C++ referans yolu aynı çıktıyı verir (reinterpret/istisna testleri hariç: referans yolunda yok, bkz. §5).

| Madde | Durum | Gerçek kanıt (test dosyası) |
|---|---|---|
| 001–011, 014, 026–030 | 🟢 | çekirdek/atama/koşul/dizi/dönüşüm/özyineleme/kısa devre/`?:`/virgül/artır + sabit testleri |
| 012 013 | 🟢 | `testler/uye.cx`, `uyg_agac.cx`: `.` `->`, taban, iç içe, dizi üye, null `->` TRAP (`trap/null_member`) |
| 015 016 | 🟢 | `testler/sizeof.cx`, `sizeof_cekirdek.cx`: Itanium ABI dizilimi (vptr, taban, EBO, dolgu), kural tablosundan tür boyutları |
| 017 | 🟢 | `testler/noexcept.cx` + `trap/exc_noexcept` (noexcept'ten kaçan istisna terminate) |
| 018 | 🟢 | `testler/rtti.cx`, `typeid_ad.cx`, `istisna_sinif.cx` (`typeid(*null)` → `std::bad_typeid`) |
| 019 021 | 🟢 | `testler/donusum_acik.cx`; reddedilenler `red/*` |
| 020 | 🟢 | `rtti.cx`; `dynamic_cast<T&>` hatası → `std::bad_cast` yakalanır (`istisna_sinif.cx`), yakalanmazsa TRAP (`trap/exc_badcast_yakalanmaz`) |
| 022 | 🟢 | `testler/bayt_gorunum.cx` (bayt okuma/yazma, dizi, double, short, long long, geri dönüş) + `bellek_gorunum.cx` (aynı boyutlu punlar) + 13 olumsuz test (`num/red/bayt_*`, `f32_bit_gorunum`, `bool_bit_gorunum`) |
| 023 024 025 | 🟢 | `donusum_ortuk.cx`, `kullanici_donusum.cx`, `udl.cx`, `f80_udl.cx` |
| 031 032 | 🟢 | `sira.cx`, `uzay.cx`, `varsayilan_karsilastirma.cx` |
| 033 034 | 🟢 | `kategori.cx`, `oncelik.cx`, `kisa_devre.cx`, `atama.cx`, `virgul.cx`, `artir.cx`; sıralanmamış değiştirme tanılaması `red/*` |
| İstisna (D) | 🟢 | `istisna_temel.cx`, `istisna_sinif.cx`, 6 trap testi (`num/trap/exc_*`), 6 olumsuz test (`num/red/catch_*`, `throw_void`), bozuk PMX1 fuzz |

## 4. Önceki raporda açık bırakılanların durumu
1. Metin → PMX1 yalnız C++ ön yüzde → **değişmedi, bilinçli**: PMX1 kanonik sayısal girdi; ön yüz isteğe bağlı uyumluluk aracı (sayısal metin ayrıştırıcı yazılmadı).
2. Sınıf/RTTI/aşırı yükleme/`<=>`/UDL/kategori/noexcept/ABI için sayısal derleyici yok → **kapandı** (ifx.pmb hepsini üretir).
3. PMT1/OPSIG/`tipdogr` entegrasyonu yok → **kapandı** (her program tür notu üretir ve `tipdogr` onaylar; sabit havuzu yazmaçları kaçınıldı, çağrı penceresi/`dyn`, f80 çiftleri, devam).
4. Null erişim motorda tuzak değil → **değişmedi**: adres 0 motorda geçerli sözcük; denetim derleyici üretimli (`ld _trp, -1` → `TRAP OOB`) ve hem ifx hem referansta aynı. Motor düzeyi null tuzağı pm/gen2/svm/valid değişikliği ister; yapılmadı.
5. İstisna altyapısı yok → **kapandı** (§2).
6. Değerlendirme sırası: standart garantileri (`&& || ?: ,`, atamada sağ-önce, `<< >>` soldan sağa, C++17) uygulanır; **belirsiz olanlar (işlev argümanları) soldan sağa seçilmiştir ve garanti olarak sunulmaz**.
7. `ifx.cx` → `ifx.pmb` üretimi `pm ifade` ile (kaynak/üretim ilişkisi); **kendi kendini üretme artık sınanıyor** (sabit nokta, §2).

## 5. Kalan sınırlar (dürüst) — hiçbiri 02.001–034 maddelerinin kapsamında olmayan ya da reddi açık olan durumlar
- **f128 (`__float128`) C++ ifadelerinde yok**; f128 komutları ve çalışma kitaplığı Bölüm 01'dedir, ifx'e/ön yüze bağlanmadı. `long double` (f80) tam bağlıdır. f128 sessizce f64'e indirgenmez: tür hiç tanınmaz.
- **Bayt görünümü kapsamı:** tamsayı ve `double` nesneleri 1 baytlik tamsayı türleriyle (okuma, `=` yazma, aritmetik); sınıf nesnesi, `float`, `long double`, `bool` bit/bayt görünümü ve bayt işaretçisinin işlev sınırını geçmesi **derleme hatasıdır** (olumsuz testler). Motorda bayt-adresli `ld/st` komutu yok; görünüm derleyici üretimli sözcük aritmetiğidir.
- **`asm.elf` kaynak girdisi 1 MiB ile sınırlı:** ifx2'nin PMA'sı 1,7 MB olduğundan öz-barındırma testinde yalnız bu adımda `pm asm` kullanılır (diğer her adım pm'siz).
- **Null/bounds tuzağı** derleyici üretimli (madde 4); istisna için `std::terminate` motorda ayrı tuzak adı değil, `TRAP OOB` olarak görünür.
- Referans C++ ön yüz (`pm ifade`) istisna üretmez (`E_UNSUPPORTED`), `reinterpret_cast` bayt görünümü ve dar-tamsayı normalizasyonu yoktur; bağımsız referans olarak yalnız eski kapsam için tutulur.

## 6. Testler
__TEST_SONUCLARI__

## 7. Dosyalar
`oz/ifade/num/{ifx.pmb,ifx.cx,ifx.sh,dogrula_ifx.sh,pmx_fuzz_ifx.py,sabotaj_ifx.sh,sabotaj_ifx.py,kural_degistir.py,testler/,red/,trap/}`, `oz/komut/gecis/ifade_kural_ekle.py`, `komutlar.pmk` (bölüm 11, 12), `src/ifade.cpp` (isteğe bağlı ön yüz: `try/catch/throw` düğümleri N_THROW/N_TRY/N_CATCH, istisna ön sözü), `oz/hizli.sh`, belgeler.
