# Yol haritası — sayısal parametre motorunun yetenekleri (C++/Linux yetenekleri dahil)

> **Ana plan artık `PLAN_1399_MADDE.md`'dir** (41 bölüm, 1.399 madde, numaralar sabit). Bu belge aşamaların kısa özeti ve Aşama 0 kanıt kaydıdır. Ana hedef ve öncelikler: `PROJE_HARITASI.md` §1.

**Karar (kullanıcı):** önce Linux x86-64 yetenekleri biter; sonra Windows (x86-64, PE). macOS kapsam dışı (kapalı model, geliştiriciye sınırlı; ayrıca ARM64 arka ucu gerektirir). Bu yetenekler **sayısal motorumuzun** yetenekleridir; C++'ı ana motor yapmak anlamına gelmez.

**Çerçeve [Tasarım ilkesi]:** hedef yeni bir metin dili değil, yeteneklerin sayısal tablolarla sağlanması. Her yetenek: (1) yeni komut/anlam/şablon kaydı ya da (2) sayısal kitaplık (motorun kendi sayısal programları). **Dış diller (C/C++, Python, Rust, Java, C#, JavaScript) yalnızca uyumluluk/aktarım katmanıdır** (plan Bölüm 07, 22, 41: kaynak tanıma ve dönüştürme); kanonik kaynak sayısal parametrelerdir ve bu katmanlar motorun yerine geçmez. Dış dil ön yüzleri ikinci önceliktir; sayısal motorun yetenekleri önce gelir.

## Aşama 0 — çekirdek tamamlama
Gösterim: ✅ tamam (kanıtıyla), ❌ eksik. Her madde: bağımsız referans (C++ `pm run`) + hedefli test + regresyon, kanıt satırı ve commit.
- ✅ `ret` mikro-işlem şablonu (`tpl prim ret`) — kanıt: `oz/elf2/dogrula_cagri.sh` (yalnızca SEM ile tanımlı `RETX s`; fib/özyineleme, yığın taşması, derinlik sınırı, yakıt sayacı, bölme-sıfır tuzağı: ELF = svm = C++ `pm`); sabotaj: şablon silinince ve 3 bozuk şablonda test başarısız
- ✅ `halt` mikro-işlem şablonu (`tpl prim halt`) — kanıt: aynı betik (`STOP`, çağrılan işlevde erken halt dahil); bozuk şablon (`REL_EXITC`) yakalandı
- ✅ `call` mikro-işlem şablonu (`tpl prim call` + 16 alana bağlı işlev simgesi `FREL_Fk/FNR8_Fk/FNP8_Fk/FCNT_Fk`, gen2'de `etok_fn`) — kanıt: `oz/elf2/dogrula_cagri.sh` (yalnızca SEM ile tanımlı `CALLX F,b`: fib özyineleme, yakıt 1..5000, DEPTH/STACK tuzakları, yığın eşiği ve eşik+1 (yerel yazmaç sıfırlama yığın sonunu aşmıyor: guard sözcükleri korunur), zorlanmış `-6 1` yolu); 6 bozuk şablon mutantı yakalandı (yanlış alan, yanlış işlev simgesi, yakıt denetimi eksik, yanlış sayaç, fazla sıfırlama)
- ✅ `host` mikro-işlem şablonu (`tpl prim host : HOSTSP_F0`, simge `HOSTSP_Fk` 440+; gen2'de `hostmap` (host kimliğine göre özel host şablonu ya da çalışma zamanı CALL'u), `hostslot` ile `scanrt` SEM'de host mikro-işlemi olan her komutu tanır) — kanıt: `oz/elf2/dogrula_cagri.sh` 91 denetim (yalnızca SEM ile tanımlı `HOSTX h,b`: getc (stdin, NUL/yüksek bayt, EOF), putf (çalışma zamanı çağrısı), putc/puti, grafik present/poll/frame (sahne, hayat), yakıt; zorlanmış `-6 1` ile tüm grafik denetimi 257/0); `sabotaj_cagri.sh` 22 mutant (ret/halt/call/host şablonları ve gen2 kodu) 22/22 yakalandı
- ❌ Şablon simgelerinin anlamı tabloda (reçete): gen2'nin `val32/target/etok/etok_fn` elle kodu genel bir değerlendiriciye iner; yeni simge = yalnızca tablo kaydı. Kanıt ölçütü: eski ve yeni gen2 ELF'leri tüm korpusta bayt-bayt aynı
- ⛔ Yürütücülerin (svm, C++ `pm`) mikro-işlem uygulamaları elle kalır: bağımsız referans olmaları tasarım gereği (kapsam dışı, bilinçli)

## Aşama 1 — çalışma zamanı ve sistem (Linux)
- Bellek geri verme (`free`) ve genel amaçlı ayırıcı (`new/delete`)
- Dosya host işlevleri: açma, okuma, yazma, kapama, konumlama; dizin; komut satırı argümanları, ortam değişkenleri
- Genel sistem çağrısı arayüzü (host tablosuna kayıt), saat/zamanlayıcı, sinyaller
- Ağ: TCP/UDP soket çağrıları (TLS ayrı ve büyük: şifreleme kitaplığı gerekir)

## Aşama 2 — türler ve sayılar
- 8/16/32 bit tamsayılar, işaretsiz aritmetik, `bool`, `char`; `float` (32 bit)
- `<cmath>` işlevleri (sin, cos, exp, log, pow): sayısal kitaplık, doğruluk testi bağımsız C++ `libm` ile
- Büyük sayı (keyfî hassasiyet), karmaşık sayı

## Aşama 3 — dil özelliklerinin motor karşılıkları (kaynak dili YOK)
Karar (kullanıcı): kod zaten doğrudan sayısal parametreye dönüşüyor (insan ya da AI sayısal tablo üretir); yeni bir kaynak dili ya da PMY1 genişletmesi gerekmez. C++ özellikleri *sözdizimi* olarak değil, motor yeteneği olarak karşılanır:
- Fonksiyon işaretçisi / lambda → dolaylı çağrı komutu (`CALLI`) + kapanış verisi: tabloya komut kaydı (OP + SEM + şablon)
- `switch` → atlama tablosu komutu
- `struct`/`union`/`enum`/bit alanı → bellek yerleşim kuralı (alan ofsetleri) ve erişim komutları; tip denetimi doğrulayıcıya sayısal kural olarak
- İstisna → tuzak yakalama / geri sarma mekanizması (açılım tablosu)
- Sanal fonksiyon / kalıtım / sınıf → sanal tablo yerleşimi + dolaylı çağrı; kurucu/yıkıcı = çağrı kuralı
- Şablon / generic / `constexpr` → sayısal program üretimi (tablodan tablo)
- Kısa devre, `?:`, ifade ağacı → mevcut PMY1/PMB1 ile zaten ifade edilir; ek dil yok

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

## Kapsam notları
- **Yeni zorunlu metinsel dil yok.** Dış dil ön yüzleri (C/C++ vb. kaynak okuma ve sayısal programa dönüştürme) *uyumluluk katmanıdır* ve plan Bölüm 07/22/41'de ele alınır; sırası gelince tam kapsamıyla yapılır.
- Satır içi assembly, macOS, çekirdek/sürücü, GPU/3D ve ses/video işleri de plan bölümlerinde (30, 32, 37) yer alır; sıraları gelene kadar yapılmaz.
- `.so`/`dlopen`: ELF'imiz şu an bilerek DLL'siz; plan Bölüm 07 (07.019–07.020) bunu kapsar.
