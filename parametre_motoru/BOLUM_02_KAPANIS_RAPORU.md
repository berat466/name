# Bölüm 02 — Kapanış Raporu: İfadeler, Operatörler ve Sabitler

Dal: `claude/formul-avcisi` · Kapsam: Linux x86-64 (LP64) · 2 Kasım 2026

## 1. Sonuç
Bölüm 02 (02.001–02.034) tamamlandı. Son commit bu raporu içeren commit'tir. PLAN sayımı: **133 🟢 / 42 🔵 / 1224 ❌**. 02.001–003 önceden 🟢'ydi ve bu bölümün testleriyle yeniden uygulanıyor (makine aritmetiği, karşılaştırma, sabit havuzu `pm ifade` çıktısında kullanılır); 02.004–034 bu çalışmada gerçekleştirildi ve doğrulandı.

## 2. Mimari (sayısal motor açısından) [Kesin]
- Yeni zorunlu metin dili **yoktur**. `pm ifade kaynak.cx`, C++ alt kümesini okuyan bir **dış dil uyumluluk ön yüzüdür** (`src/ifade.cpp`; C++ referans/geliştirme aracı — motorun kendisi gibi C++ ile yazılmıştır, sayısal PMA aracı olarak yeniden yazılmadı).
- Akış: metin → lexer → ayrıştırıcı → AST → **PMX1** (kanonik sayısal AST, sayı listesi) → anlam + kod üretimi → PMA → PMB1. **Derleme her zaman PMX1 sayı listesinden çalışır** (`--pmx1` yazar, `--pmx1-oku` okur; aynı PMA çıkar). PMX1 yapısal doğrulanır (tür aralığı, tür başına çocuk sayısı, çocuk türü, döngüsüzlük, derinlik) ve bozma/fuzz ile sınanır.
- **Operatör öncelik/birleşim tablosu PMK1 bölüm 11'dedir** (`oz/komut/komutlar.pmk`, isteğe bağlı bölüm; eski araçlar olduğu gibi korur). `oz/komut/yenile_ifade.sh` → `src/ifade_ops_gen.h`; ayrıştırıcı bu tablodan beslenir; tablo ↔ derlenmiş tablo (`pm ifade --ops`) ↔ üretilmiş başlık tutarlılığı her koşuda sınanır.
- Üretilen program, Bölüm 01'in sayısal komutlarını kullanır (`sext/zext8/16/32`, `ltu/leu`, `divu/modu`, `u2f/f2u`, `rnd_f32`, 64 bit aritmetik + açık normalleştirme) ve mevcut yürütücülerde (C++ `pm`, native ELF/gen2, svm) birebir aynı çalışır. Yığın çerçeveleri yazılımsal yığında (`__sp/__lim`), nesneler sözcük düzeninde; `sizeof/alignof` ABI dizilimiyle (Itanium) bildirilir.

## 3. Maddeler → gerçekleştirim → kanıt
| Madde | Gerçekleştirim | Kanıt (`oz/ifade/`) |
|---|---|---|
| 004 005 | tablo güdümlü öncelik ayrıştırıcısı, parantez | `testler/oncelik.cx` + rastgele ifadeler |
| 006 | `&&` `||` kısa devre (kontrol akışı + sabit katlama) | `kisa_devre.cx` |
| 007 | `?:` tür birleştirme, lvalue/sınıf kolları | `kosul.cx` |
| 008 009 | `=`, 10 bileşik atama, C++17 sağ-önce sırası, daraltma | `atama.cx` |
| 010 011 | ön/son `++ --`, virgül | `artir.cx`, `virgul.cx` |
| 012 013 014 | `.`, `->`, `[]` (dizi/işaretçi/`i[a]`), tür ve sınır denetimi | `uye.cx`, `dizi.cx`, `trap/idx_oob.cx` |
| 015 016 | `sizeof`, `alignof` | `sizeof.cx` |
| 017 | `noexcept(ifade)` | `noexcept.cx` |
| 018 | `typeid` (statik/dinamik, `==`, `name()`) | `rtti.cx`, `typeid_ad.cx` |
| 019 020 021 022 | `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast` | `donusum_acik.cx`, `rtti.cx`, `red/*cast*` |
| 023 024 | örtük dönüşümler (sıralama), kullanıcı dönüşüm operatörleri | `donusum_ortuk.cx`, `kullanici_donusum.cx` |
| 025 | UDL | `udl.cx` |
| 026–030 | karakter/kaçış, ham dize, bitişik dize, ikili/sekizli/onaltılı, ayırıcı, sonekler | `sabitler.cx`, `red/lex_*` |
| 031 032 | `<=>`, `= default` karşılaştırma, yeniden yazım | `uzay.cx`, `varsayilan_karsilastirma.cx`, `ters_aday.cx` |
| 033 | lvalue/xvalue/prvalue | `kategori.cx` |
| 034 | değerlendirme sırası, yan etkiler, sıralanmamış değiştirme tanılaması | `sira.cx`, `red/unseq_*` |
Bileşik uygulamalar: `uyg_agac.cx` (sanal gönderim + dynamic_cast + typeid + özyineleme), `uyg_vektor.cx` (operatör aşırı yükleme, dönüşüm, `<=>`, UDL), `uyg_metin.cx`.

## 4. Doğrulama sonuçları
- **Paket `ifade`** (`oz/ifade/dogrula_ifade.sh`): 25 olumlu program × (PMX1 gidiş-dönüş + C++ `pm` + native ELF + svm; beklenen çıktı **g++ oracle**'dan), 150 rastgele UB'siz ifade programı (UBSan ile elenir, tür izlemeli üreteç), 102 olumsuz test (kesin hata kodu; çoğu g++ ile de reddedilir), 11 çalışma zamanı tuzağı (pm = ELF = svm: tuzak adı + önceki stdout), PMX1 fuzz (120 bozma + döngü) ve operatör tablosu tutarlılığı: **924 kontrol, 0 başarısız** (57 sn duvar).
- **Tam normal koşu** `HIZLI_YENIDEN=1 oz/hizli.sh --tam --bag`: **31 paket, 0 başarısız, duvar 762 sn (12 dk 42 sn), toplam CPU 2374 sn** (4 çekirdek; ≤ 20 dk hedefi karşılandı). Bölüm 01 paketleri (komut, uyum, arac, turler, tip, tur, svm, elf2, yenile, bag …) tablo değişikliğine (bölüm 11) rağmen geçti: geriye uyumluluk (189/189) ve öz-barındırma zinciri (`yenile` 60/0, `bag`, `pmy2elf`) korunmuştur.
- **Sabotaj (mutasyon)** `oz/ifade/sabotaj_ifade.sh`: **58 mutant, 56 yakalandı, 2 gerekçeli denk, 0 yakalanmadı** (234–239 sn; ilk koşuda 17 mutant kaçmıştı: gerçek test boşlukları — ushort terfisi, sağ kaydırma işareti, ters aday, aşırı yükleme sırası, sınır/derinlik/dizi sınırı, tuzak çıktısı, PMX1 döngüsü, vb. — testler eklenerek kapatıldı; ardından tam yeniden koşuldu). Gerekçeli denk iki mutant: (a) işaretçi karşılaştırmasında işaretli/işaretsiz sıralama — adresler 2^27 sözcükten küçük pozitiftir, davranış aynı; (b) olağan aritmetik dönüşümde eşit derece dalı — sonraki dal aynı işaretsiz türü verir. Mutantlar `src/ifade.cpp` ve `src/ifade_ops_gen.h` içindeki anlamsal kararları (kısa devre, normalleştirme, ABI dizilimi, sıra, RTTI, null denetimi, PMX1 doğrulama, öncelik tablosu) bozar.
- Önceki bölümlerin sabotaj kümeleri (Bölüm 01'deki 645 mutant) bu çalışmada **yeniden koşulmadı**: ilgili kaynaklar değişmedi; yalnız `komutlar.pmk`'ya isteğe bağlı bölüm 11 eklendi ve `komut/uyum/arac/turler/tip` paketleri bunu geçti.

## 5. Sınırlar (dürüst)
Şablon, ad alanı (yalnız `std::` yerleşik adları), lambda, `new/delete`, kurucu/yok edici çağrıları (yalnız boş yok edici sözdizimi), istisna (başarısız `dynamic_cast<T&>` ve `typeid(*null)` **tuzak** üretir), çoklu/sanal kalıtım, statik üye, `enum`, işlev işaretçisi, `int (*p)[3]` bildirici, `wchar_t/char8_t/char16_t` ve dize ön ekleri, erişim denetimi desteklenmiyor. `long double` f64 ile temsil edilir (UDL parametresi dahil). Motor sözcük adresli: `reinterpret_cast` bayt görünümü vermez (eşit boyutlu skalerler; işaretçi↔`unsigned long` = sözcük adresi × 8). İşlev argümanları soldan sağa değerlendirilir (standart belirsiz bırakır; g++ sağdan sola — testler buna bağlı değil). Null işaretçi dolaylı erişimi derleyici tarafından `TRAP: OOB` yapılır (motorda adres 0 geçerli sözcük); null olmayan geçersiz işaretçiler yalnız bellek sınırı dışındaysa yakalanır. Sıralanmamış değiştirme tanılaması muhafazakârdır (yalnız tam adlı nesneler). **PMT1 tür notu üretilip `tipdogr` ile doğrulanmaz** (kayıtlar ifade başına yeniden kullanıldığı için). Ön yüz C++ ile yazılmıştır, kendi kendini sayısal PMA olarak derleyemez.

## 6. Dosyalar
`src/ifade.cpp`, `src/ifade_ops_gen.h`, `oz/komut/gecis/ifade_ops_{ekle,h}.py`, `oz/komut/yenile_ifade.sh`, `oz/ifade/*` (testler, red, trap, rastgele üreteç, fuzz, sabotaj), `oz/hizli.sh` (paket `ifade`, sabotaj `ifade`), `YAPILANLAR.md`, `PROJE_HARITASI.md`, `PLAN_1399_MADDE.md`.
