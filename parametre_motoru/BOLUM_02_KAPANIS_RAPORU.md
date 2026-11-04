# Bölüm 02 — Kapanış Raporu: İfadeler, Operatörler ve Sabitler

Dal: `claude/formul-avcisi` · Linux x86-64 · 2 Kasım 2026

## 1. Sonuç
**02.001–02.034: 34/34 madde sayısal yolla doğrulandı.** PLAN: 133 🟢 / 42 🔵 / 1224 ❌ (toplam 1399). Bölüm 03'e geçilmedi.
Bu rapor önceki "KAPANMADI" raporunun yerine geçer; önceki geri alma gerekçeleri aşağıda madde madde kapatıldı (§4), **motorda var olup ifade derleyicisine bağlanmamış yetenekler §5'te açık listelidir**.

**PLAN sayımı (düzeltme):** Bölüm 02'nin ilk (geri alınan) sürümünden kalan **17 madde zaten 🟢** idi (001–011, 014, 026–030) ve toplam 116 🟢'nın içindeydi. Bu bölümde **17 madde daha** (012, 013, 015–025, 031–034) ❌→🟢 yapıldı: 116 + 17 = **133**. Yani 133, "eski 133'e eklenen 17" değil, "116'ya eklenen 17"dir; önceki raporlarda 133'ün ortaya çıkış yolu açıklanmamıştı. Sayım doğrudan tablodan alındı: `grep -E '^[0-9]{2}\.[0-9]{3}  ' PLAN_1399_MADDE.md` → 133 🟢 + 42 🔵 + 1224 ❌ = 1399; Bölüm 01 = 33 🟢, Bölüm 02 = 34 🟢 (BOLUM 01–02 toplamı 67; diğer bölümlerden 66 🟢).

## 2. Mimari (ana ilke korunuyor) [Kesin]
- **`oz/ifade/num/ifx.pmb`** = sayısal PMX1 → PMA derleyicisi (kanonik ürün, sayısal program). Kural/işlem anlamı **tabloda**: `komutlar.pmk` bölüm 12 (IFADE KURAL TABLOSU: tür özellikleri, operatör→komut eşlemesi, karşılaştırma eşlemesi, dönüşüm adları, yazmaç türü→TYPES kimliği) + bölüm 11 (öncelik) + OPSIG/HOSTSIG imzaları (`komut.elf` dökümü, çalışma anında okunur). Tablo sayıları değişince hiçbir şey yeniden üretilmeden davranış değişir (test: `+`→sub, `*`→add).
- **Yürütme zinciri pm'siz:** `ifx.elf` → `asm.elf` → `coz.elf` → `valid.elf` → `tipdogr.elf` → `gen2.elf` → ELF. C++ ön yüz (`pm ifade`, metin → PMX1) yalnız **isteğe bağlı uyumluluk aracıdır**; PMX1 sayı listesini sayısal derleyici işler.
- **Tür duyarlı üretim (PMT1/OPSIG):** ifx her işlev için `;;PMT1` tür notu üretir (yazmaç türleri sabit; çağrı penceresi `dyn`, geniş değerin ikinci sözcüğü `devam`); `tipdogr` OPSIG/HOSTSIG ile **her test programını** doğrular (kasıtlı bozulan tür notları reddedilir: 69 bozuk notun ≥5'i reddedilmek zorunda).
- **f80** (`long double`) ve **f128** (`__float128`): ifx'te sınıf değeri gibi 2 sözcüklü bellek değeri (Q/q yazmaç çifti, sret, değer-parametre, dizi/üye/global); işlemler Bölüm 01 komutlarıyla (`add_f128 … cv_i64_f128 … cv_f80_f128`); komut adları, yazmaç türü (TYPES 26) ve operatör eşlemesi **tablodan** (`komutlar.pmk` bölüm 12: IKILI/KARSILASTIR kayıtlarına f128 sütunu, AD kaydına 21 yeni ad, YAZMAC kaydına f128 türü). Sabitler bit-tam (`1.5Q`, `1e4000Q`: 128 bit = `i` + `d` ham bitleri; `strtoflt128`). Ortak tür kuralı: f128 > f80 > double > float; daraltma `{}` içinde yasak. **f64'e sessiz indirgeme yok**, f80↔f128 yalnız açık `cv_f80_f128/cv_f128_f80` komutlarıyla. Not: f128 de f80 gibi **2 sözcüklü** (128 bit) bir değerdir; 4 sözcüklü operand yalnız karmaşık c160 içindir.
- **Sayısal assembler sınırı:** `asm.elf` 1 MiB girdi sınırı **kaldırıldı** (4 MiB; token/deyim/işlenen tabloları ve kodlanmış sınırlar `oz/asm/asm.pma.in` içinde birlikte büyütüldü, `oz/nesne/metinden.sh` ile `asm.pmo/asm.pmb` yeniden üretildi, numune: 1,7 MB PMA, `pm asm` ile bayt-bayt aynı çıktı). Öz-barındırmada artık **yalnız sayısal zincir** kullanılır; `pm asm` yalnız bağımsız çapraz denetimdir (çıktı `asm.elf` ile bayt-bayt aynı olmalı).
- **İstisna mekanizması (gerçek çalışma zamanı):** `throw`/`try`/`catch` (tür eşleşmesi: skaler tür kimliği, sınıf için `isa` + türetilmiş→taban; `catch(...)`, `throw;`, değer/başvuru yakalama), `std::exception`/`std::bad_cast`/`std::bad_typeid` (ön söz sınıfları; `what()` sanal). Yazılımsal yayılım: `__ex/__exo` hücreleri, her çağrı sonrası denetim, çerçeve kaydı geri yüklenerek dönüş; yakalanmayan istisna, `noexcept` işlevden kaçan istisna ve `main`'den kaçan istisna = `std::terminate` = TRAP. `dynamic_cast<T&>` başarısızlığı `std::bad_cast`, `typeid(*null)` `std::bad_typeid` fırlatır.
- **Bayt düzeyi bellek görünümü:** motor sözcük adreslidir (adres 0 geçerli sözcük). `reinterpret_cast<unsigned char*/char*>(&nesne)` için **bayt adresli işaretçi** (taint `bp` = asıl nesne boyutu S; bayt adresi = sözcük × S + bayt): okuma `(sözcük >> 8k) & 255`, yazma sözcük oku-değiştir-yaz, aritmetik/karşılaştırma/fark, asıl türe geri dönüş (`P / S`). Dar tamsayılar `reinterpret_cast` bulunan programlarda her okumada kendi genişliğine normalize edilir (işaretli↔işaretsiz aynı boyut punları doğru). Sızıntı güvenli: bayt adresli işaretçi işlev argümanı, `return`, üye/dizi ataması, `void*`, referansa bağlama, adres alma, bileşik atama, `++/--` ile **derleme hatası** (sessiz yanlış okuma yok).
- **Öz-barındırma (sabit nokta):** `ifx.cx`'in PMX1'i → `ifx.elf` → PMA1 → (`asm.elf`→`coz.elf`→`valid.elf`→`tipdogr`→`gen2.elf`; `pm` yok) → `ifx2.elf`; `ifx2.elf` aynı girdiyi derleyince PMA2 = PMA1 (95 877 satır, bayt-bayt aynı). Bu çalışma gerçek bir hatayı (işlev adı `op…` ile başlayınca operatör sanılıyordu) ve ifx'in sabit tablo sınırlarını ortaya çıkardı, düzeltildi.

## 3. Madde durumu ve kanıt
Her testin yolu: g++ oracle = `ifx.elf` (native) = `ifx.pmb` (pm) PMA'sı bayt-bayt aynı; PMA → `asm.elf` → `coz.elf` → `valid.elf` kabul → `tipdogr` kabul → `gen2.elf` ELF; ELF = pm = svm = g++ çıktısı; ayrıca C++ referans yolu aynı çıktıyı verir (f80/f128, istisna, bayt görünümü testleri hariç: referans yolunda tanımsız, bkz. §5).

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
| f128 | 🟢 | `testler/f128_aritmetik.cx` (4 işlem, karşılaştırma, tamsayı/float/double/long double dönüşümleri, taşma/alt taşma/NaN/∞, dizi/üye/global/parametre/dönüş, `?:`, `+= ++`, sizeof/alignof), `f128_f80_karisik.cx`; 6 olumsuz test (`num/red/f128_*`, `ld_daraltma`) |
| İstisna (D) | 🟢 | `istisna_temel.cx`, `istisna_sinif.cx`, 6 trap testi (`num/trap/exc_*`), 6 olumsuz test (`num/red/catch_*`, `throw_void`), bozuk PMX1 fuzz |

## 4. Önceki raporda açık bırakılanların durumu
1. Metin → PMX1 yalnız C++ ön yüzde → **değişmedi, bilinçli**: PMX1 kanonik sayısal girdi; ön yüz isteğe bağlı uyumluluk aracı (sayısal metin ayrıştırıcı yazılmadı).
2. Sınıf/RTTI/aşırı yükleme/`<=>`/UDL/kategori/noexcept/ABI için sayısal derleyici yok → **kapandı** (ifx.pmb hepsini üretir).
3. PMT1/OPSIG/`tipdogr` entegrasyonu yok → **kapandı** (her program tür notu üretir ve `tipdogr` onaylar; sabit havuzu yazmaçları kaçınıldı, çağrı penceresi/`dyn`, f80 çiftleri, devam).
4. Null erişim motorda tuzak değil → **değişmedi**: adres 0 motorda geçerli sözcük; denetim derleyici üretimli (`ld _trp, -1` → `TRAP OOB`) ve hem ifx hem referansta aynı. Motor düzeyi null tuzağı pm/gen2/svm/valid değişikliği ister; yapılmadı.
5. İstisna altyapısı yok → **kapandı** (§2).
6. Değerlendirme sırası: standart garantileri (`&& || ?: ,`, atamada sağ-önce, `<< >>` soldan sağa, C++17) uygulanır; **belirsiz olanlar (işlev argümanları) soldan sağa seçilmiştir ve garanti olarak sunulmaz**.
7. `ifx.cx` → `ifx.pmb` üretimi `pm ifade` ile (kaynak/üretim ilişkisi); **kendi kendini üretme artık sınanıyor** (sabit nokta, §2).

## 5. Motor yeteneği ↔ ifade derleyicisi bağlantı denetimi ve kalan sınırlar (dürüst)
Motorda (Bölüm 01) bulunan sayısal yeteneklerin ifade derleyicisine bağlantı durumu:

| Motor yeteneği | Bağlı mı | Not |
|---|---|---|
| bool, 8/16/32/64 bit işaretli-işaretsiz tamsayı, `char` aileleri | ✅ | normalleştirme komutları tablodan |
| f32, f64 | ✅ | |
| **f80 `long double`** | ✅ | testler `f80_*` |
| **f128 `__float128`** | ✅ (bu tur) | testler `f128_*`; 2 sözcük; f80↔f128 açık dönüşüm |
| i128 / u128 (`__int128`) | ❌ bağlı değil | motor komutları var (`add_i128 …`), ifx/ön yüz tanımıyor (tür hiç tanınmaz, sessiz indirgeme yok) |
| f16 / bf16 (`_Float16`, `__bf16`) | ❌ bağlı değil | motor komutları var (`add_f16 …`, `rnd_f16`) |
| karmaşık c64 / c128 / c160 (`_Complex`) | ❌ bağlı değil | motor komutları var (`add_c64 …`) |
| bswap16/32/64/128 (`__builtin_bswap*`) | ❌ bağlı değil | motor komutları var |
| `alloc` | ✅ | istisna nesneleri |

Bunlar 02.001–034 maddelerinin ifade/operatör/sabit kapsamını bozmaz (her maddenin testi vardır) ama "motorda var olan yeteneğin tam bağlantısı" ölçütüne göre **eksiktir**; sonraki çalışmalarda bu tablo Bölüm 02 ile ilişkilendirilerek ele alınmalıdır. Diğer sınırlar:
- **Bayt görünümü kapsamı:** tamsayı ve `double` nesneleri 1 baytlik tamsayı türleriyle (okuma, `=` yazma, aritmetik); sınıf nesnesi, `float`, `long double`, `__float128`, `bool` bit/bayt görünümü ve bayt işaretçisinin işlev sınırını geçmesi **derleme hatasıdır** (olumsuz testler). Motorda bayt-adresli `ld/st` komutu yok; görünüm derleyici üretimli sözcük aritmetiğidir.
- **Null/bounds tuzağı** derleyici üretimli (§4/4); istisna için `std::terminate` motorda ayrı tuzak adı değil, `TRAP OOB` olarak görünür.
- Referans C++ ön yüz (`pm ifade`) istisna, `__float128`, `long double` değer işlemi ve bayt görünümü üretmez (`E_UNSUPPORTED`); yalnız isteğe bağlı eski uyumluluk aracıdır.
- **`ifx.pmb` depoya eklendi** (`.gitignore` `*.pmb` kuralı nedeniyle önceki commitlerde hiç izlenmiyordu; tazelik testi onu `ifx.cx`'ten yeniden üretip karşılaştırır).

## 6. Testler
- **Tam normal koşu** `HIZLI_YENIDEN=1 oz/hizli.sh --tam --bag` (f128 + asm sınırı değişikliklerinden sonra): **32 paket, 0 başarısız, duvar 783 sn (13 dk 03 sn), toplam CPU 2574 sn** (hedef ≤ 20 dk; hiçbir test atlanmadı). Paketler: `ifx` **628 kontrol**, `ifade` **927**, `asm` 2253/2253 (C++ `pm asm` ile aynı üretim 795, aynı ret 1418), `bag` 1272/1272 (pm'siz zincir), `kural` 352, uyum 189/189, yenile 60/60, valid 5662/5662, yap 2366/2366 … → geriye uyumluluk, Bölüm 01 öz-barındırması ve eski programlar korunuyor.
- `ifx` 628 kontrolün içeriği: 45 program için g++ = ifx.elf = ifx.pmb(pm) PMA'sı bayt-bayt aynı → asm.elf → coz.elf → valid.elf kabul → tipdogr kabul → ELF = pm = svm = g++ çıktısı; kural tablosu değişikliği (3 senaryo); 69 bozuk tür notu; olumsuz 29 + 105 test (kesin hata kodu); 17 çalışma zamanı tuzağı; iki taban programdan 200 bozuk PMX1; **pm'siz öz-barındırma sabit noktası** (1,7 MB PMA'yı `asm.elf` doğrudan işler; `pm asm` çıktısıyla bayt-bayt aynılığı çapraz denetlenir).
- **Sabotaj (test-testi):** ifx **84 mutant** (19 kural-tablosu + 65 kaynak: f128 adları/yazmaç türü/ortak tür/daraltma, istisna, bayt görünümü, tür notu, öz-barındırma sınırları, çekirdek anlam) → **84 yakalandı, 0 yakalanmadı**; C++ referans `ifade` 58 mutant → 56 yakalandı, 2 gerekçeli denk, 0 yakalanmadı; **değişen assembler için** `asm` kural sabotajı 19/19, ondalık 17/17, nesne/bağlayıcı 14/14, görünüm 60/60 yakalandı.
- **Test altyapısı notları:** rastgele ifade testleri g++ oracle'ı `-frounding-math` (g++ sabit katlama `-0` hatası) ve `-fext-numeric-literals` (`1.5Q`) ile derler; `kural_degistir.py` IKILI kaydının altı dizgeli yeni biçimine uyarlandı.
- Çalışma sırasında bulunup düzeltilen **gerçek hatalar:** `op…` ile başlayan işlev adları operatör sanılıyordu; ifx sabit tablo sınırları büyütüldü; `{}` daraltma denetimi `long double`/f128'i hiç kapsamıyordu (düzeltildi: float < double < f80 < f128 sırası); `ifx.pmb` `.gitignore` yüzünden izlenmiyordu.

## 7. Dosyalar
`oz/ifade/num/{ifx.pmb,ifx.cx,ifx.sh,dogrula_ifx.sh,pmx_fuzz_ifx.py,sabotaj_ifx.sh,sabotaj_ifx.py,kural_degistir.py,testler/,red/,trap/}`, `oz/komut/gecis/ifade_kural_ekle.py`, `komutlar.pmk` (bölüm 11, 12; f128 sütunları), `oz/asm/asm.pma.in` + `oz/nesne/asm.pmo`/`oz/asm/asm.pmb` (4 MiB girdi), `src/ifade.cpp` (`__float128`, `q` soneki, `pq`) (isteğe bağlı ön yüz: `try/catch/throw` düğümleri N_THROW/N_TRY/N_CATCH, istisna ön sözü), `oz/hizli.sh`, belgeler.
