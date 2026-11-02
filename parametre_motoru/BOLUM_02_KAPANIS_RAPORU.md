# Bölüm 02 — Durum Raporu: İfadeler, Operatörler ve Sabitler (**KAPANMADI**)

Dal: `claude/formul-avcisi` · Linux x86-64 · 2 Kasım 2026

## 1. Gerçek durum
İlk sürümde "02.001–034 tamamlandı" demiştim; bu **doğru değildi**: ifade anlamının tamamı `src/ifade.cpp` (C++) içindeydi, PMX1 yalnız sayı listesiydi. Bu sürümde mimari düzeltildi ve işaretler geri alındı. **PLAN: 116 🟢 / 42 🔵 / 1241 ❌. Bölüm 02: 17 🟢 / 17 ❌ → bölüm AÇIK.** Bölüm 03'e geçilmedi.

## 2. Sayısal motor olarak artık kaynaktan bağımsız çalışanlar [Kesin]
- **`oz/ifade/num/ifx.pmb`** — PMX1 sayı listesini okuyup PMA üreten **sayısal program** (kanonik ürün). `ifx.cx` yalnız geliştirme görünümüdür (asm.pma.in gibi); `ifx.pmb`'nin tazeliği testle sınanır. Çalışma anında C++/Python kaynağı ya da derleyicisi gerekmez.
- **Kural tablosu — `komutlar.pmk` bölüm 12 (IFADE KURAL TABLOSU)** (+ bölüm 11 operatör önceliği): tür özellikleri (bit, işaret, derece, normalleştirme komutu), ikili operatör → komut eşlemesi, karşılaştırma eşlemesi (işaretsizde takas, kayanda terslemeler), dönüşüm komut adları. `ifx.pmb` tabloyu **çalışma anında** okur. Kanıt: tablo sayıları değiştirilince (`+`→`sub`, `*`→`add`) hiçbir kaynak/ikili yeniden üretilmeden çıktı değişir.
- **pm'siz uçtan uca zincir:** `ifx.elf` (gen2 ile native) → `asm.elf` → `coz.elf` → `valid.elf` (sayısal doğrulayıcı kabul eder) → `gen2.elf` → çalışan ELF. Çıktı = g++ = `ifx.pmb` (pm) PMA'sı bayt-bayt aynı = pm = svm. (pm yalnız bootstrap `kur.sh` ve bağımsız referans olarak kullanılır.)
- Kapsam (çekirdek): skaler/işaretçi/dizi, tüm tekli/ikili/atama/bileşik atama/`?:` (lvalue dahil)/virgül/`++ --`/`&& ||`, `static_cast` ve C tipi dönüşüm, `sizeof/alignof`, dize sabiti, global değişken, işlev çağrısı/özyineleme, if/while/do/for/break/continue/return, çalışma zamanı sınır ve null denetimi.
- **Dürüst ret:** sınıf, referans, `auto`, `long double`, `T()`/`T{}`, aşırı yükleme, UDL, typeid, dynamic/const/reinterpret_cast … `@@HATA desteklenmiyor` ile AYRIK ret edilir; sessizce yanlış kod üretilmez (`num/red/*`).
- **long double:** önceki sürümdeki sessiz f64'e indirgeme **kaldırıldı** (hem C++ referans ön yüzü hem ifx): değer işlemi reddedilir, yalnız tür düzeyi (`sizeof/alignof = 16`) kalır. Gerçek f80 işlem üretimi (Bölüm 01'in f80 komutlarıyla) **yapılmadı**.

## 3. Madde durumu (kanıt: `oz/ifade/num/testler`, `oz/ifade/testler`)
| Madde | Durum | Not |
|---|---|---|
| 001 002 003 | 🟢 | motor aritmetiği/karşılaştırma/sabit havuzu; ifx testleri bunları gerçek C++ ifadeleriyle sürer |
| 004 005 | 🟢 | ifade ağacı semantiği (öncelik/parantez) PMX1'den sayısal olarak işlenir; metin ayrıştırıcı isteğe bağlı C++ uyumluluk ön yüzüdür (öncelik tablosu bölüm 11) |
| 006–011 | 🟢 | kısa devre, `?:`, atama, bileşik atamalar, `++/--`, virgül: sayısal derleyici + 3 yürütücü |
| 014 | 🟢 | indeksleme, işaretçi aritmetiği, tür ve sınır denetimi (çekirdek) |
| 026–030 | 🟢 | sabitlerin değer ve türü PMX1 düğümlerinde, sayısal olarak işlenir (kaçış, ham/bitişik dize, ikili/sekizli/onaltılı, ayırıcı, sonekler); lexer isteğe bağlı ön yüzdedir |
| 012 013 | ❌ | üye erişimi: sınıf yalnız C++ referans ön yüzünde |
| 015 016 | ❌ | skaler/dizi sayısal; sınıf ABI dizilimi yalnız C++ yolunda |
| 017 018 020 021 022 | ❌ | noexcept, typeid, dynamic/const/reinterpret_cast: yalnız C++ yolunda; `reinterpret_cast` bayt temsili yok (motor sözcük adresli) |
| 019 023 | ❌ | skaler `static_cast`/örtük dönüşüm sayısal; sınıf/işaretçi-hiyerarşi dönüşümleri yalnız C++ yolunda |
| 024 025 031 032 033 034 | ❌ | kullanıcı dönüşümleri, UDL, `<=>`, varsayılan karşılaştırma, xvalue/başvuru kategorileri, sıralanmamış-değiştirme tanılaması: yalnız C++ yolunda |

## 4. Test sonuçları
- Tam normal koşu `HIZLI_YENIDEN=1 oz/hizli.sh --tam --bag`: **32 paket, 0 başarısız, duvar 783 sn (13 dk 03 sn), CPU 2398 sn** (hedef ≤ 20 dk). Paketler: `ifx` 110 kontrol, `ifade` 927 kontrol, Bölüm 01 paketleri (uyum 189/189, yenile 60/0, bag, pmy2elf, komut, tip, turler …) geçti → geriye uyumluluk ve öz-barındırma korunuyor.
- Sabotaj: `ifx` 35 mutant (15 kural-tablosu + 20 kaynak) **35 yakalandı** (69 sn); C++ referans `ifade` sabotajı 58 mutant, 56 yakalandı, 2 gerekçeli denk, 0 yakalanmadı (225 sn). Bölüm 01 sabotaj kümeleri bu çalışmada yeniden koşulmadı.

## 5. Kalan zorunlu bağımlılıklar (dürüst)
1. **Metin → PMX1** yalnız C++ ön yüzünde (`pm ifade`, isteğe bağlı uyumluluk). Sayısal metin ayrıştırıcı yok.
2. **Sınıf/nesne/RTTI/aşırı yükleme/`<=>`/UDL/kategori/noexcept/ABI düzeni** için sayısal kural ve sayısal derleyici yok; yalnız C++ referansta (bağımsız referans olarak testli, ama "tamamlandı" sayılmaz).
3. **PMT1/OPSIG/`tipdogr` entegrasyonu yapılmadı:** üretilen programa tür notu eklenemedi (sabit havuzu yazmaçlarının türleri, yazmaç yeniden kullanımı, genel `add` ↔ `i64/u64` imzaları nedeniyle tür duyarlı kod üretimi gerekiyor; denendi, tamamlanmadı). Üretilen PMB1 yalnız yapısal doğrulayıcıdan (`valid.elf`) geçiyor.
4. **Null erişim:** motorun `ld/st` yolunda adres 0 geçerli sözcük; denetim derleyici üretimli (ifx ve C++ ön yüz). Motor düzeyi tuzak yapılmadı (pm/gen2/svm/valid değişikliği gerektirir).
5. **İstisna altyapısı yok:** `dynamic_cast<T&>` ve `typeid(*null)` tuzaktır, `std::bad_cast` değil.
6. **Değerlendirme sırası:** standart garantileri (`&& || ?: ,`, atamada sağ-önce, `<< >>` soldan sağa) uygulanır; belirsiz olanlar (işlev argümanları) iki yolda da soldan sağa seçilmiştir ve garanti olarak sunulmaz.
7. `ifx.cx` (geliştirme görünümü) C++ alt kümesiyle yazılıp `pm ifade` ile `ifx.pmb`'ye derlenir; bu, `asm.pma.in` → `asm.pmb` ilişkisi gibi kaynak/üretim ilişkisidir, ama üretimin kendisi hâlâ C++ aracıyla yapılıyor (kendi kendini üretme yalnızca `ifx.elf` düzeyinde).

## 6. Dosyalar
`oz/ifade/num/{ifx.pmb,ifx.cx,ifx.sh,dogrula_ifx.sh,sabotaj_ifx.sh,kural_degistir.py,testler/,red/}`, `oz/komut/gecis/ifade_kural_ekle.py`, `oz/komut/yenile_ifade.sh`, `komutlar.pmk` (bölüm 11, 12), `src/ifade.cpp` (C++ referans/uyumluluk ön yüzü; long double düzeltmesi, `__getc`), `oz/hizli.sh` (paket `ifx`, sabotaj `ifx`), belgeler.
