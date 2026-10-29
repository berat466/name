# C++ (Linux) yetenek eşdeğerliği yol haritası

**Karar (kullanıcı):** önce C++'ın Linux karşılığı biter; sonra Windows (x86-64, PE). macOS kapsam dışı (kapalı model, geliştiriciye sınırlı; ayrıca ARM64 arka ucu gerektirir). Önceki sıra korunur: `call/ret/halt/host` mikro-işlem şablonları → güvenlik katmanları; bu harita onların etrafına dizilir.

**Çerçeve [Tasarım ilkesi]:** hedef C++ *sözdizimini* derlemek değil, C++'ın *yeteneklerini* sayısal tablolarla sağlamak. Her yetenek: (1) yeni komut/anlam/şablon kaydı ya da (2) motorun kendi dilinde (PMY1 genişletmesi) yazılmış sayısal kitaplık. C++ kaynak dilinin tamamını ayrıştırmak ve `#include/#define` önişlemcisi bilerek kapsam dışıdır (metin tabanlı; sayısal-önce ilkesiyle çelişir).

## Aşama 0 — çekirdek tamamlama (sırada)
- `call/ret/halt/host` mikro-işlem şablonları (şu an yalnızca sabit şablon)
- Mikro-işlemlerin en alt uygulamalarının sayısallaşması (kalan elle kod)

## Aşama 1 — çalışma zamanı ve sistem (Linux)
- Bellek geri verme (`free`) ve genel amaçlı ayırıcı (`new/delete`)
- Dosya host işlevleri: açma, okuma, yazma, kapama, konumlama; dizin; komut satırı argümanları, ortam değişkenleri
- Genel sistem çağrısı arayüzü (host tablosuna kayıt), saat/zamanlayıcı, sinyaller
- Ağ: TCP/UDP soket çağrıları (TLS ayrı ve büyük: şifreleme kitaplığı gerekir)

## Aşama 2 — türler ve sayılar
- 8/16/32 bit tamsayılar, işaretsiz aritmetik, `bool`, `char`; `float` (32 bit)
- `<cmath>` işlevleri (sin, cos, exp, log, pow): sayısal kitaplık, doğruluk testi bağımsız C++ `libm` ile
- Büyük sayı (keyfî hassasiyet), karmaşık sayı

## Aşama 3 — dil katmanı (PMY1 genişletmesi) **[ön karar gerekli]**
- İfade ayrıştırıcı (`a + b * c`, `?:`, kısa devre), `switch`
- Dolaylı çağrı (fonksiyon işaretçisi; yeni `CALLI` komutu), lambda (kapanış)
- `struct`/`union`/`enum`/bit alanı, tür takma adı, tipli işaretçi ve referans, statik tür denetimi, `auto`
- İstisnalar (`try/catch/throw`: açılım tablosu)
- Sınıf, kalıtım, sanal fonksiyon (sanal tablo), operatör aşırı yükleme, kurucu/yıkıcı (RAII), akıllı işaretçiler
- Şablonlar/generic, `constexpr`, `namespace`/modül

## Aşama 4 — kitaplık
- `string`, `vector`, `array`, `list`, `deque`, `map/set`, `unordered_*`
- `<algorithm>`, `<numeric>`, `<functional>`, `<optional>/<variant>/<tuple>`, `<random>`, `<chrono>`, `<bitset>`, `<regex>`
- `iostream`/biçimli çıktı, `filesystem`, Unicode/yerelleştirme

## Aşama 5 — eşzamanlılık
- İş parçacığı (`clone`), `mutex/condition_variable` (`futex`), `atomic` ve bellek modeli, semaphore, paylaşımlı bellek

## Aşama 6 — derleyici kalitesi
- Yazmaç ataması, sabit katlama, ölü kod eleme, satır içi açma, döngü eniyilemeleri (`-O2` benzeri)
- Hata ayıklama bilgisi, profiler, sanitizer benzeri denetimler, dinamik olmayan kitaplık bağlama (ayrı derleme birimi)

## Aşama 7 — güvenlik katmanları (ayrı, sürümlü)
- İmza/bütünlük, yetkilendirme, sandbox/izolasyon, kullanıcıya özel parametreler, güvenli güncelleme/kurtarma
- Doğrulayıcı sürümü: tek kanonik sürüm mü, imzalı çoklu sürüm mü (açık karar)

## Sonra — Windows x86-64 (PE)
- PE kapsayıcısı + içe aktarma tablosu, Windows host şablonları (`WriteFile` vb.), test için Wine ya da gerçek Windows

## Bilinçli kapsam dışı
C++ kaynak dilinin tamamı, önişlemci, satır içi assembly, `.so`/`dlopen` (ELF'imiz bilerek DLL'siz), 3D/GPU/ses/masaüstü pencere sistemi, macOS.
