CLAUDE GELİŞTİRME GÖREVİ — SAYISAL PARAMETRE MOTORU
1.399 KONTROL / 41 BÖLÜM — SIRAYLA, BÖLÜMÜ TAMAMLADIKTAN SONRA DUR
=======================================================================

GÖREV KURALLARI — BU DOSYAYI GÖREV TALİMATI OLARAK KULLAN

01) Aşağıdaki 41 bölümü NUMARASINA GÖRE işle. Her çalışmada YALNIZCA sıradaki tamamlanmamış bölümü ele al. Bu bölüm tamamen bitmeden bir sonrakine geçme. Bölüm bitince DUR; kullanıcı yeni talimat verince sıradaki bölüme geç.
02) Hedef yeni bir C++ metin dili yapmak değildir. Program, komut semantiği, bağlantılar ve mümkün olan araç tanımları için KANONİK KAYNAK sayısal parametre mimarisidir. Mevcut C++/Python/Rust kaynakları için dönüşüm ve uyumluluk hedefi korunacak.
03) Bölümün bütün maddelerini birlikte ele al; tek madde, oyuncak demo, küçük PoC, sahte/placeholder uygulama veya "önce küçük örnek" ile bölüm tamamlandı deme. Tam modülü / tam işlevi mevcut motorun içine bütünleşik biçimde uygula. Varsa gerçek mevcut bileşenleri kullan; gereksiz paralel sistem kurma.
04) Sadece test çalıştırmak için ayrı bir mini motor, demo uygulaması, gösteri aracı veya geçici çalışma ortamı geliştirme. Test, teslimatın kendisi değildir. BÖLÜMÜN TAM ENTEGRASYONUNDAN SONRA mevcut test/derleme araçlarıyla gerçek işlevi, regresyonu ve öz-barındırmayı doğrula. Gereken doğrulama atlanamaz.
05) "Bir kısmını yaptım, büyüğüne zaman yetmez" gerekçesiyle o bölümü TAMAMLANDI işaretleme. Yalnızca tam uygulama + gerçek doğrulama varsa tamamlandı say; herhangi bir zorunlu madde eksikse bölümü AÇIK bırak ve engeli doğru belirt.
06) Daha önce yapılmış işlevleri tekrar yazmadan GERÇEK depo durumunu kontrol et. 🟢/🔵 eski raporlarda bildirilmiş durumdur; otomatik olarak kanıt sayılmaz. Hatalı/eski işaretleri düzelt.
07) Tamamlanan her maddenin depodaki ilgili dosyası/işlevi ve bölüm sonunda doğrulama çıktısı bulunmalı. Gerekliyse commit oluştur; PROJE_HARITASI.md, YAPILANLAR.md ve envanteri GERÇEK duruma göre güncelle. Belgeleri koddan önce tamamlandı göstermek için değiştirme.
08) İşi bölümü tamamlamaya yönlendir. Bölüme ait bütün özelliklerin birbiriyle çalışması, hata davranışı, veri/bellek güvenliği ve Linux native çıktı eşdeğerliği sağlanmadan bitmiş sayma. Önceden çalışan yetenekleri bozma.
09) Bölüm tamamlanınca yalnızca şu kapanış raporunu sun: bölüm numarası ve adı; tüm kontrol maddelerinin son durumu; yapılan gerçek entegrasyon; mevcut kaynaklardan bağımsız native üretim durumu; son bütünleşik doğrulama; commit/kanıtlar; varsa açık kalan engeller. Ardından DUR.
10) Bu listedeki 1.399 öğe karşılaştırma kontrolüdür; bazıları aynı altyapıyı ortaklaşa kullanır. Yeni ürün geliştirme ve gereksiz bağımsız alt sistemler çıkarma. İlgili bölümün işlev kapsamı tamamlanmalıdır. Kaynak kod/kütüphane dönüştürme henüz genel olarak tamamlanmış sayılmayacak.

UYARI: Bölüm 01 tamamlanmadan bölüm 02'ye geçme; aynı kural bölüm 41'e kadar geçerlidir. Kullanıcı yeni görev vermedikçe bir sonraki bölümü başlatma.

DURUM İŞARETLERİ
🟢 Önceki rapora göre tamamlandı; depoda doğrula.
🔵 Daha yeni rapora göre tamamlandı; depoda doğrula.
❌ Yapılmadı veya tam işlevi kanıtlanmadı.

SAYIM: 41 bölüm / 1399 kontrol / 79 🟢 / 44 🔵 / 1276 ❌ (30 Ekim 2026 belge durumudur).

=======================================================================
TAM BÖLÜM TABLOSU — EKSİKSİZ
=======================================================================

BÖLÜM ÖZETİ
01. Temel sayı ve değer türleri: 33 kontrol | 🟢10 🔵2 ❌21
02. İfadeler, operatörler ve sabitler: 34 kontrol | 🟢3 🔵0 ❌31
03. Denetim akışı: 19 kontrol | 🟢6 🔵1 ❌12
04. Fonksiyonlar ve çağrı modeli: 33 kontrol | 🟢6 🔵3 ❌24
05. Yapılar, sınıflar, nesneler: 39 kontrol | 🟢1 🔵0 ❌38
06. Tür sistemi, generic ve metaprogramlama: 40 kontrol | 🟢0 🔵0 ❌40
07. Önişlemci, bildirim ve derleme modeli: 24 kontrol | 🟢1 🔵1 ❌22
08. Bellek adresleme ve ayırma: 41 kontrol | 🟢9 🔵0 ❌32
09. Yaşam döngüsü, hata, istisna ve sözleşme: 26 kontrol | 🟢5 🔵0 ❌21
10. STL: sıralı ve ilişkisel kaplar: 34 kontrol | 🟢1 🔵0 ❌33
11. STL: değer taşıyıcıları ve yardımcılar: 31 kontrol | 🟢0 🔵0 ❌31
12. Metin ve Unicode: 31 kontrol | 🟢2 🔵0 ❌29
13. Iterator, ranges ve görünüm adaptörleri: 31 kontrol | 🟢0 🔵0 ❌31
14. Standart algoritmalar: 43 kontrol | 🟢0 🔵0 ❌43
15. Matematik: skaler ve özel fonksiyonlar: 36 kontrol | 🟢2 🔵1 ❌33
16. Lineer cebir, sayısal analiz, SIMD: 34 kontrol | 🟢0 🔵0 ❌34
17. Rastgele sayılar, saat, tarih: 26 kontrol | 🟢0 🔵0 ❌26
18. Girdi/çıktı, formatlama, akışlar: 33 kontrol | 🟢4 🔵2 ❌27
19. Dosya sistemi: 32 kontrol | 🟢0 🔵0 ❌32
20. Eşzamanlılık ve thread: 35 kontrol | 🟢0 🔵0 ❌35
21. Atomic ve lock-free programlama: 21 kontrol | 🟢0 🔵0 ❌21
22. C++ ön yüzü ve kaynak tanıma: 26 kontrol | 🟢2 🔵1 ❌23
23. Ara biçimler, derleyici, bağlayıcı: 34 kontrol | 🟢6 🔵4 ❌24
24. Derleyici optimizasyonları: 41 kontrol | 🟢1 🔵1 ❌39
25. Debug, analiz, doğrulama, araçlar: 34 kontrol | 🟢3 🔵3 ❌28
26. Linux süreç, zamanlayıcı ve terminal: 31 kontrol | 🟢2 🔵0 ❌29
27. Linux bellek, IPC ve yüksek performanslı I/O: 35 kontrol | 🟢0 🔵0 ❌35
28. Linux ağ, socket ve internet: 45 kontrol | 🟢0 🔵0 ❌45
29. Linux güvenlik, izolasyon, yetkilendirme: 42 kontrol | 🟢2 🔵1 ❌39
30. 2D/3D grafik, pencere ve kullanıcı arayüzü: 42 kontrol | 🟢6 🔵1 ❌35
31. Oyun motoru, fizik, simülasyon: 38 kontrol | 🟢2 🔵0 ❌36
32. Ses, video, görüntü ve medya: 36 kontrol | 🟢0 🔵0 ❌36
33. Veritabanı, depolama ve veri biçimleri: 37 kontrol | 🟢0 🔵0 ❌37
34. Yapay zekâ, makine öğrenmesi ve bilim: 43 kontrol | 🟢1 🔵1 ❌41
35. Sunucu altyapısı, bulut ve dağıtık sistemler: 35 kontrol | 🟢0 🔵0 ❌35
36. Kriptografi ve veri güvenliği: 34 kontrol | 🟢0 🔵0 ❌34
37. Cihazlar, işletim sistemi çekirdeği, gömülü: 31 kontrol | 🟢0 🔵0 ❌31
38. Paketleme, yapı sistemi ve yayınlama: 33 kontrol | 🟢0 🔵2 ❌31
39. Otomasyon, uygulama entegrasyonu ve belge üretimi: 30 kontrol | 🟢0 🔵0 ❌30
40. Motorumuzun C++'tan ayrı, özgün mimari hedefleri: 40 kontrol | 🟢4 🔵20 ❌16
41. Dışarıdan kaynak kod ve kütüphane içeri aktarma (sayısal dönüşüm hedefi): 36 kontrol | 🟢0 🔵0 ❌36

===========================================================================
TÜM KONTROL MADDELERİ
===========================================================================

01. TEMEL SAYI VE DEĞER TÜRLERI
------------------------------------------------------------------------
01.001  🟢  64-bit işaretli tam sayı işlemleri (int64)
01.002  🟢  64-bit artı, eksi, çarpma
01.003  🟢  64-bit işaretli bölme ve kalan
01.004  🟢  Sayı karşılaştırmaları
01.005  🟢  64-bit VE/VEYA/XOR
01.006  🟢  Bit kaydırma ve aritmetik sağ kaydırma
01.007  🟢  Doğrudan sabitli (immediate) aritmetik
01.008  🟢  IEEE-754 double (64-bit) toplama/çıkarma/çarpma/bölme
01.009  🟢  Double karekök, negatifini alma, karşılaştırma
01.010  🟢  Tamsayı ↔ double dönüşümü
01.011  🔵  NaN, sonsuzluk, negatif sıfır, alt-normal bit davranışı için hedefli çapraz testler
01.012  🔵  Sayı olarak ondalık sabitleri ayrıştırma ve yazma
01.013  ❌  Ayrı int8 türü ve taşma kuralları
01.014  ❌  Ayrı int16 türü ve taşma kuralları
01.015  ❌  Ayrı int32 türü ve taşma kuralları
01.016  ❌  İşaretsiz uint8 türü
01.017  ❌  İşaretsiz uint16 türü
01.018  ❌  İşaretsiz uint32 türü
01.019  ❌  Genel unsigned uint64 türü (sabit havuzu yeterli değil)
01.020  ❌  bool türü ve tür denetimi
01.021  ❌  char/signed char/unsigned char türü
01.022  ❌  wchar_t, char8_t, char16_t, char32_t türleri
01.023  ❌  float32 hesabı ve türü
01.024  ❌  long double türü
01.025  ❌  Standart sabit genişlikli kayan nokta türleri (float16, float128 vb.)
01.026  ❌  128-bit tam sayı / derleyici genişletmeleri
01.027  ❌  std::byte türü
01.028  ❌  nullptr_t ve tür düzeyinde boş gösterici
01.029  ❌  Karmaşık sayı türü
01.030  ❌  Sayısal türler arasında genel dönüşüm kuralları
01.031  ❌  Tür bazında sayısal limitler ve taşma davranışı
01.032  ❌  Ana makine/ABI veri modeli ve platforma göre tür genişliği
01.033  ❌  Endianness ve temsil seçimi için genel tür altyapısı

02. İFADELER, OPERATÖRLER VE SABITLER
------------------------------------------------------------------------
02.001  🟢  Makine komutlarıyla aritmetik hesaplama
02.002  🟢  Makine komutlarıyla karşılaştırma
02.003  🟢  Sabit havuzu ve anlık sayısal operand
02.004  ❌  Öncelik kurallı genel ifade ayrıştırıcısı (a+b*c)
02.005  ❌  Parantezli ifade grameri
02.006  ❌  Kısa devre mantığı (&&, //)
02.007  ❌  Üçlü koşul operatörü (?:)
02.008  ❌  Genel atama ifadesi
02.009  ❌  Bileşik atamalar (+=, -= vb.) için genel ifade katmanı
02.010  ❌  Ön/son artırma ve azaltma ifadeleri
02.011  ❌  Virgül operatörü
02.012  ❌  Üye erişim ifadesi (.)
02.013  ❌  Gösterici üye erişimi (->)
02.014  ❌  Dizi indeksleme ifadeleri ve tür kontrolü
02.015  ❌  Genel sizeof kuralları
02.016  ❌  Genel alignof kuralları
02.017  ❌  noexcept operatörü
02.018  ❌  typeid operatörü
02.019  ❌  static_cast
02.020  ❌  dynamic_cast
02.021  ❌  const_cast
02.022  ❌  reinterpret_cast
02.023  ❌  Örtük dönüşüm kuralları
02.024  ❌  Kullanıcı tanımlı dönüşüm operatörleri
02.025  ❌  Kullanıcı tanımlı sabitler (UDL)
02.026  ❌  Karakter ve kaçış dizisi sabitleri
02.027  ❌  Ham dizge (raw string) sabitleri
02.028  ❌  Bitişik dizge sabitlerinin birleştirilmesi
02.029  ❌  İkili, sekizli, onaltılı kaynak sabit sözdizimi
02.030  ❌  Basamak ayırıcıları ve tür sonekleri
02.031  ❌  Üç yönlü karşılaştırma (<=>)
02.032  ❌  Karşılaştırmaların otomatik türetilmesi
02.033  ❌  C++ değer kategorileri (lvalue, xvalue, prvalue)
02.034  ❌  İfade değerlendirme sırası ve yan etki semantiği

03. DENETIM AKIŞI
------------------------------------------------------------------------
03.001  🟢  Koşullu dallanma
03.002  🟢  Döngüler
03.003  🟢  İç içe döngüler
03.004  🟢  Koşulsuz atlama (jmp)
03.005  🟢  Fonksiyondan dönüş
03.006  🟢  PMY1 ile temel yapısal koşul/döngü tanımı
03.007  🔵  Yeni GOTO komutunun yalnızca sayısal SEM tanımı ve native üretimi
03.008  ❌  Genel switch/case/default semantiği
03.009  ❌  Atlama tablosuyla switch optimizasyonu
03.010  ❌  Kaynak seviyesinde range-for
03.011  ❌  Kaynak seviyesinde do-while
03.012  ❌  break/continue için yüksek seviyeli yapısal soyutlama
03.013  ❌  Genel pattern matching (C++ standardında yaygın sabit özellik değil)
03.014  ❌  Korutin askıya alma/sürdürme
03.015  ❌  İstisna yayılımıyla denetim akışı
03.016  ❌  Derleme zamanı koşullu ifade seçimi (if constexpr)
03.017  ❌  Genel kısa devre kontrol akışı
03.018  ❌  Kaynak seviyesinde zorunlu liveness/scope kuralları
03.019  ❌  Denetim akışı grafiği (CFG) merkezli genel optimizer

04. FONKSIYONLAR VE ÇAĞRI MODELI
------------------------------------------------------------------------
04.001  🟢  Fonksiyon tanımı
04.002  🟢  Doğrudan fonksiyon çağrısı
04.003  🟢  Fonksiyondan geri dönüş
04.004  🟢  Özyinelemeli fonksiyon çağrısı
04.005  🟢  Çok fonksiyonlu bağımsız ELF
04.006  🟢  Çağrı derinliği sınırı ve tuzak
04.007  🔵  SEM ret mikro-işlemini x86 şablonuyla üretme
04.008  🔵  SEM call mikro-işlemini x86 şablonuyla üretme (Claude günlüğüne göre)
04.009  🔵  SEM halt mikro-işlemini x86 şablonuyla üretme
04.010  ❌  Fonksiyon işaretçisi ve dolaylı çağrı (CALLI)
04.011  ❌  Fonksiyon işaretçisi tabloları
04.012  ❌  Fonksiyon aşırı yüklemesi
04.013  ❌  Varsayılan argümanlar
04.014  ❌  Değişken sayıda argüman (varargs)
04.015  ❌  İsimli parametreler (C++'ta yerleşik değil)
04.016  ❌  C++ ABI'sine uygun parametre geçirme ve tür sınıflandırması
04.017  ❌  Genel lambda ve capture
04.018  ❌  Generic lambda
04.019  ❌  Mutable lambda ve closure nesnesi
04.020  ❌  Fonksiyon çağrılabilir nesneleri (function object)
04.021  ❌  std::function benzeri tür silme
04.022  ❌  std::move_only_function
04.023  ❌  Üye fonksiyon işaretçisi
04.024  ❌  noexcept fonksiyon sözleşmesi
04.025  ❌  Inline fonksiyon semantiği
04.026  ❌  constexpr fonksiyon
04.027  ❌  consteval fonksiyon
04.028  ❌  constinit değişken
04.029  ❌  Özel bağlama türleri (extern C)
04.030  ❌  Genel tail-call optimizasyonu
04.031  ❌  Fonksiyon görünürlüğü ve linkage
04.032  ❌  Değişken ömürlü yığın nesnelerinin dönüşte temizlenmesi
04.033  ❌  Coroutines: promise, awaiter, resume/destroy

05. YAPILAR, SINIFLAR, NESNELER
------------------------------------------------------------------------
05.001  🟢  PMY1'de yapısal fonksiyon ve şablon-benzeri davranış tanımı (C++ class karşılığı değil)
05.002  ❌  Genel struct bellek yerleşim ve alan erişimi
05.003  ❌  class veri modeli
05.004  ❌  union veri modeli
05.005  ❌  Enum ve enum class veri modeli
05.006  ❌  Bit alanları (bit-fields)
05.007  ❌  Üye alanlarına offset ile güvenli erişim
05.008  ❌  Nesne içi hizalama ve padding kuralları
05.009  ❌  İç içe sınıflar
05.010  ❌  Statik veri üyeleri
05.011  ❌  Statik üye fonksiyonlar
05.012  ❌  this göstericisi
05.013  ❌  public/private/protected erişim denetimi
05.014  ❌  friend erişim kuralı
05.015  ❌  Kurucular (constructors)
05.016  ❌  Yıkıcılar (destructors)
05.017  ❌  Kopya kurucusu
05.018  ❌  Taşıma kurucusu
05.019  ❌  Kopya atama
05.020  ❌  Taşıma atama
05.021  ❌  Varsayılan özel üye fonksiyonlar
05.022  ❌  deleted/defaulted üye fonksiyonlar
05.023  ❌  Üye başlatıcı listeleri
05.024  ❌  Nesne ömür ve sahiplik semantiği
05.025  ❌  Tekli kalıtım
05.026  ❌  Çoklu kalıtım
05.027  ❌  Sanal kalıtım
05.028  ❌  Sanal fonksiyonlar
05.029  ❌  Sanal tablo (vtable) / dinamik dispatch
05.030  ❌  Saf sanal fonksiyon/soyut sınıf
05.031  ❌  override/final kuralları
05.032  ❌  Run-time polymorphism
05.033  ❌  RTTI/dynamic_cast ve typeid
05.034  ❌  Operatör aşırı yükleme
05.035  ❌  Dönüştürücü kurucu ve explicit
05.036  ❌  Delegating/inheriting constructors
05.037  ❌  Empty base optimization ve [[no_unique_address]]
05.038  ❌  Nesne dilimleri ve çoklu kalıtım pointer ayarları
05.039  ❌  Genel object layout ABI

06. TÜR SISTEMI, GENERIC VE METAPROGRAMLAMA
------------------------------------------------------------------------
06.001  ❌  Statik tür denetimi ve tür eşitliği
06.002  ❌  Tür çıkarımı (auto)
06.003  ❌  decltype
06.004  ❌  decltype(auto)
06.005  ❌  Referans türleri (&, &&)
06.006  ❌  Tüm türlü gösterici türleri
06.007  ❌  const doğruluğu ve const üye fonksiyon
06.008  ❌  volatile niteliği
06.009  ❌  typedef ve using type aliases
06.010  ❌  Namespaces ve namespace aliases
06.011  ❌  C++20 modules/import/export
06.012  ❌  One Definition Rule ve isim çözümleme
06.013  ❌  Argument-dependent lookup (ADL)
06.014  ❌  Class template
06.015  ❌  Function template
06.016  ❌  Variable template
06.017  ❌  Member template
06.018  ❌  Template specialization
06.019  ❌  Partial specialization
06.020  ❌  Template template parameter
06.021  ❌  Non-type template parameter
06.022  ❌  Parameter packs
06.023  ❌  Variadic templates
06.024  ❌  Fold expressions
06.025  ❌  Pack indexing (C++26 taslağı)
06.026  ❌  CTAD (class template argument deduction)
06.027  ❌  SFINAE
06.028  ❌  Substitution/deduction failure kuralları
06.029  ❌  Concepts
06.030  ❌  Requires expressions
06.031  ❌  Constraint satisfaction / subsumption
06.032  ❌  if constexpr
06.033  ❌  type_traits
06.034  ❌  Metafunctions ve integral_constant
06.035  ❌  Compile-time reflection / std::meta (C++26 taslağı)
06.036  ❌  Template for (C++26 taslağı)
06.037  ❌  Source-level code generation reflection
06.038  ❌  static_assert
06.039  ❌  Attributes ([[nodiscard]], [[maybe_unused]] vb.)
06.040  ❌  Contracts (C++26 taslağı)

07. ÖNIŞLEMCI, BILDIRIM VE DERLEME MODELI
------------------------------------------------------------------------
07.001  🟢  Programı bağımsız ELF'e dönüştürme
07.002  🔵  PMO1: motor araçları için ayrı sayısal nesne ve bağlayıcı
07.003  ❌  C++ kaynak önişlemcisi
07.004  ❌  #include
07.005  ❌  #define makroları
07.006  ❌  Fonksiyon benzeri makrolar
07.007  ❌  #if/#ifdef/#ifndef koşullu derleme
07.008  ❌  #elifdef/#elifndef (C++23)
07.009  ❌  #line/#error/#warning/#pragma
07.010  ❌  Makro token birleştirme (##)
07.011  ❌  Makro stringleştirme (#)
07.012  ❌  Include guard / pragma once benzeri modül denetimi
07.013  ❌  C++ translation-unit kavramının tam desteği
07.014  ❌  Genel static/extern değişken bağlama kuralları
07.015  ❌  C++ header-only kitaplık modeli
07.016  ❌  C++ module interface/implementation birimleri
07.017  ❌  Ayrı derlenmiş keyfî uygulama nesnelerini bağlama
07.018  ❌  Genel statik .a arşivi bağlayıcısı
07.019  ❌  ELF dinamik .so bağlantısı
07.020  ❌  dlopen/dlsym/dlclose
07.021  ❌  ELF relocation/linker script/visibility genel desteği
07.022  ❌  Program giriş/başlatma ABI'si ve C runtime uyumu
07.023  ❌  C++ kaynak derleme ve C++ uyumluluk testi
07.024  ❌  Genel derleme sistemi (Make/CMake/Ninja eşdeğeri)

08. BELLEK ADRESLEME VE AYIRMA
------------------------------------------------------------------------
08.001  🟢  64-bit sözcüklere sayısal adresle erişim
08.002  🟢  Bellekten yükleme
08.003  🟢  Belleğe yazma
08.004  🟢  Sınır dışı erişimi tuzakla durdurma
08.005  🟢  Bump/ardışık alloc
08.006  🟢  Çağrı yığını
08.007  🟢  Yığın taşması ve çağrı derinliği sınırı
08.008  🟢  Global veri bölgesi
08.009  🟢  Sıfırlanan bss / space veri bölgesi
08.010  ❌  Türe göre pointer aritmetiği
08.011  ❌  Pointer-to-pointer
08.012  ❌  Pointer alias kuralları
08.013  ❌  Strict aliasing semantiği
08.014  ❌  Hizalı bellek ayırma
08.015  ❌  Yerleştirmeli new (placement new)
08.016  ❌  İsteğe bağlı belirli adres aralığı ayırma
08.017  ❌  free ile belleği geri verme
08.018  ❌  Genel malloc/calloc/realloc/free
08.019  ❌  Genel new/delete
08.020  ❌  new[]/delete[]
08.021  ❌  Arena allocator tam sistem
08.022  ❌  Slab allocator
08.023  ❌  Buddy allocator
08.024  ❌  Object pool
08.025  ❌  Fragmentasyon yönetimi
08.026  ❌  Bellek yeniden kullanımı
08.027  ❌  Ownership/borrowing semantiği
08.028  ❌  RAII
08.029  ❌  std::unique_ptr
08.030  ❌  std::shared_ptr
08.031  ❌  std::weak_ptr
08.032  ❌  std::make_unique/std::make_shared
08.033  ❌  std::allocator / allocator_traits
08.034  ❌  std::pmr::memory_resource
08.035  ❌  std::pmr monotonic/pool kaynakları
08.036  ❌  std::scoped_allocator
08.037  ❌  std::launder/lifetime-start semantiği
08.038  ❌  Uninitialized memory algorithms
08.039  ❌  Reference-counting ve cycle stratejisi
08.040  ❌  Garbage collection (C++ standart zorunluluğu değil)
08.041  ❌  Bellek sanitizer / use-after-free algılayıcısı

09. YAŞAM DÖNGÜSÜ, HATA, ISTISNA VE SÖZLEŞME
------------------------------------------------------------------------
09.001  🟢  DIV0 tuzağı
09.002  🟢  OOB tuzağı
09.003  🟢  STACK/DEPTH tuzakları
09.004  🟢  FUEL sınırı
09.005  🟢  Geçersiz programı reddeden doğrulayıcı
09.006  ❌  try/catch/throw
09.007  ❌  C++ exception unwinding
09.008  ❌  Exception safety guarantee: basic/strong/no-throw
09.009  ❌  Uncaught exception -> terminate semantiği
09.010  ❌  Custom exception türleri
09.011  ❌  std::exception_ptr
09.012  ❌  std::nested_exception
09.013  ❌  noexcept propagation
09.014  ❌  std::error_code / error_condition
09.015  ❌  std::system_error
09.016  ❌  std::expected<T,E> genel yapısı
09.017  ❌  std::optional hata/yokluk modeli
09.018  ❌  std::variant tabanlı tagged error
09.019  ❌  assert ve contract kütüphanesi
09.020  ❌  Kaynak/stack trace yakalama
09.021  ❌  Thread-safe exception propagation
09.022  ❌  Core language undefined behavior denetimi
09.023  ❌  Dangling reference algılama
09.024  ❌  Uninitialized read algılama
09.025  ❌  Integer overflow trap seçeneği
09.026  ❌  UBSan eşdeğer genel analiz

10. STL: SIRALI VE ILIŞKISEL KAPLAR
------------------------------------------------------------------------
10.001  🟢  Düşük düzeyli sözcük dizileri
10.002  ❌  std::array
10.003  ❌  std::vector
10.004  ❌  std::deque
10.005  ❌  std::list
10.006  ❌  std::forward_list
10.007  ❌  std::set
10.008  ❌  std::multiset
10.009  ❌  std::map
10.010  ❌  std::multimap
10.011  ❌  std::unordered_set
10.012  ❌  std::unordered_multiset
10.013  ❌  std::unordered_map
10.014  ❌  std::unordered_multimap
10.015  ❌  std::stack
10.016  ❌  std::queue
10.017  ❌  std::priority_queue
10.018  ❌  std::flat_map (C++23)
10.019  ❌  std::flat_multimap (C++23)
10.020  ❌  std::flat_set (C++23)
10.021  ❌  std::flat_multiset (C++23)
10.022  ❌  std::hive (C++26 taslağı)
10.023  ❌  std::inplace_vector (C++26 taslağı)
10.024  ❌  std::span
10.025  ❌  std::mdspan
10.026  ❌  Dizi büyütme ve kapasite yönetimi
10.027  ❌  Iterator invalidation kuralları
10.028  ❌  Custom comparator / allocator
10.029  ❌  Heterogeneous lookup
10.030  ❌  Transparent hashing/comparison
10.031  ❌  Node handle extract/merge
10.032  ❌  Container emplacement
10.033  ❌  Container swap/splice
10.034  ❌  C++ container exception-safety guarantees

11. STL: DEĞER TAŞIYICILARI VE YARDIMCILAR
------------------------------------------------------------------------
11.001  ❌  std::pair
11.002  ❌  std::tuple
11.003  ❌  std::optional
11.004  ❌  std::variant
11.005  ❌  std::any
11.006  ❌  std::expected
11.007  ❌  std::reference_wrapper
11.008  ❌  std::function
11.009  ❌  std::move_only_function
11.010  ❌  std::bind
11.011  ❌  std::invoke
11.012  ❌  std::apply
11.013  ❌  std::visit
11.014  ❌  std::get/std::get_if
11.015  ❌  std::exchange
11.016  ❌  std::swap
11.017  ❌  std::move
11.018  ❌  std::forward
11.019  ❌  std::forward_like
11.020  ❌  std::integer_sequence
11.021  ❌  std::index_sequence
11.022  ❌  std::bitset
11.023  ❌  std::byte yardımcıları
11.024  ❌  std::source_location
11.025  ❌  std::type_index
11.026  ❌  std::ratio
11.027  ❌  std::initializer_list
11.028  ❌  std::hash genel altyapısı
11.029  ❌  std::less / std::greater
11.030  ❌  std::identity
11.031  ❌  std::scoped_lock

12. METIN VE UNICODE
------------------------------------------------------------------------
12.001  🟢  ASCII/sayı dizileri üzerinden temel karakter çıktısı
12.002  🟢  Standart girdiden tek bayt okuma
12.003  ❌  std::string
12.004  ❌  std::wstring
12.005  ❌  std::u8string
12.006  ❌  std::u16string
12.007  ❌  std::u32string
12.008  ❌  std::string_view
12.009  ❌  Dize boyutlandırma / append / insert / erase
12.010  ❌  Dize karşılaştırma / find / substr
12.011  ❌  Dize parçalama / join
12.012  ❌  std::to_string / stoi / stod
12.013  ❌  std::from_chars / to_chars genel API
12.014  ❌  UTF-8 doğrulama
12.015  ❌  UTF-16 dönüşümü
12.016  ❌  UTF-32 dönüşümü
12.017  ❌  Kod noktası / grapheme cluster ayrımı
12.018  ❌  Unicode normalizasyon (NFC/NFD vb.; harici)
12.019  ❌  Unicode case folding (harici)
12.020  ❌  Unicode kategori veritabanı (harici)
12.021  ❌  Yerelleştirilmiş karşılaştırma/collation
12.022  ❌  Çok baytlı karakter işleme
12.023  ❌  regex
12.024  ❌  regex capture ve replacement
12.025  ❌  std::format
12.026  ❌  std::print (C++23)
12.027  ❌  std::locale / facets
12.028  ❌  std::text_encoding (C++26 taslağı)
12.029  ❌  JSON string escaping / unescaping
12.030  ❌  XML entity işleme
12.031  ❌  Tarafsız binary/text kodlama biçimi API

13. ITERATOR, RANGES VE GÖRÜNÜM ADAPTÖRLERI
------------------------------------------------------------------------
13.001  ❌  Iterator kategorileri
13.002  ❌  Contiguous iterator
13.003  ❌  Random-access iterator
13.004  ❌  Bidirectional iterator
13.005  ❌  Forward iterator
13.006  ❌  Input/output iterator
13.007  ❌  Iterator traits
13.008  ❌  std::begin/end / cbegin/cend
13.009  ❌  std::rbegin/rend
13.010  ❌  std::advance/distance/next/prev
13.011  ❌  std::back_inserter/front_inserter/inserter
13.012  ❌  std::reverse_iterator
13.013  ❌  std::move_iterator
13.014  ❌  std::counted_iterator
13.015  ❌  std::common_iterator
13.016  ❌  Ranges concepts
13.017  ❌  std::ranges::range / view
13.018  ❌  std::views::filter
13.019  ❌  std::views::transform
13.020  ❌  std::views::take / drop
13.021  ❌  std::views::split / join
13.022  ❌  std::views::reverse
13.023  ❌  std::views::iota
13.024  ❌  std::views::zip (C++23)
13.025  ❌  std::views::chunk (C++23)
13.026  ❌  std::views::slide (C++23)
13.027  ❌  std::views::adjacent (C++23)
13.028  ❌  std::views::enumerate (C++23)
13.029  ❌  std::ranges::to (C++23)
13.030  ❌  std::generator (C++23)
13.031  ❌  Lazy evaluation ve pipelining

14. STANDART ALGORITMALAR
------------------------------------------------------------------------
14.001  ❌  std::sort
14.002  ❌  std::stable_sort
14.003  ❌  std::partial_sort
14.004  ❌  std::nth_element
14.005  ❌  std::is_sorted
14.006  ❌  std::partition
14.007  ❌  std::stable_partition
14.008  ❌  std::find/find_if
14.009  ❌  std::search
14.010  ❌  std::find_end
14.011  ❌  std::find_first_of
14.012  ❌  std::adjacent_find
14.013  ❌  std::binary_search
14.014  ❌  std::lower_bound/upper_bound
14.015  ❌  std::equal_range
14.016  ❌  std::merge/inplace_merge
14.017  ❌  std::set_union/intersection/difference
14.018  ❌  std::heap (make/push/pop/sort)
14.019  ❌  std::min/max/minmax
14.020  ❌  std::min_element/max_element
14.021  ❌  std::clamp
14.022  ❌  std::copy/copy_if/copy_n
14.023  ❌  std::move/backward
14.024  ❌  std::fill/generate
14.025  ❌  std::transform
14.026  ❌  std::replace/replace_if
14.027  ❌  std::remove/remove_if
14.028  ❌  std::unique
14.029  ❌  std::reverse/rotate/shuffle
14.030  ❌  std::next_permutation/prev_permutation
14.031  ❌  std::count/count_if
14.032  ❌  std::all_of/any_of/none_of
14.033  ❌  std::equal/mismatch
14.034  ❌  std::lexicographical_compare
14.035  ❌  std::for_each
14.036  ❌  std::reduce / transform_reduce
14.037  ❌  std::accumulate
14.038  ❌  std::inner_product
14.039  ❌  std::partial_sum
14.040  ❌  std::inclusive_scan/exclusive_scan
14.041  ❌  std::iota
14.042  ❌  std::gcd/std::lcm
14.043  ❌  Paralel execution policy ile algoritmalar

15. MATEMATIK: SKALER VE ÖZEL FONKSIYONLAR
------------------------------------------------------------------------
15.001  🟢  64-bit işaretli tamsayı temel aritmetiği
15.002  🟢  Double aritmetiği ve karekök
15.003  🔵  FMIN'in yalnızca SEM kaydı ile eklenmesi
15.004  ❌  sin / cos / tan
15.005  ❌  asin / acos / atan / atan2
15.006  ❌  sinh / cosh / tanh
15.007  ❌  asinh / acosh / atanh
15.008  ❌  exp / exp2 / expm1
15.009  ❌  log / log2 / log10 / log1p
15.010  ❌  pow / cbrt / hypot
15.011  ❌  erf / erfc
15.012  ❌  tgamma / lgamma
15.013  ❌  floor / ceil / trunc
15.014  ❌  round / nearbyint / rint
15.015  ❌  fmod / remainder / remquo
15.016  ❌  copysign / nextafter / nexttoward
15.017  ❌  frexp / ldexp / scalbn
15.018  ❌  ilogb / logb
15.019  ❌  fpclassify / isfinite / isnan / isinf
15.020  ❌  fma fused multiply-add
15.021  ❌  fabs ve genel tür bazlı mutlak değer
15.022  ❌  Bessel fonksiyonları
15.023  ❌  Legendre polinomları
15.024  ❌  Hermite polinomları
15.025  ❌  Beta fonksiyonu
15.026  ❌  Zeta fonksiyonu
15.027  ❌  Elliptic integrals
15.028  ❌  Matematik sabitleri (std::numbers)
15.029  ❌  Kompleks sayı aritmetiği / std::complex
15.030  ❌  std::valarray
15.031  ❌  Rasyonel sayı aritmetiği
15.032  ❌  Keyfî hassasiyetli int/float
15.033  ❌  Interval arithmetic
15.034  ❌  Sabit nokta (fixed-point) kitaplığı
15.035  ❌  Birimli miktar (units) kitaplığı
15.036  ❌  Olasılık dağılımlarının matematik fonksiyonları

16. LINEER CEBIR, SAYISAL ANALIZ, SIMD
------------------------------------------------------------------------
16.001  ❌  Dense vector/matrix türleri
16.002  ❌  Matrix çarpımı
16.003  ❌  Matrix transpoz
16.004  ❌  Matrix inverse
16.005  ❌  LU ayrışımı
16.006  ❌  QR ayrışımı
16.007  ❌  Cholesky ayrışımı
16.008  ❌  SVD ayrışımı
16.009  ❌  Eigenvalues/eigenvectors
16.010  ❌  Sparse matrix formatları (CSR/CSC)
16.011  ❌  Iterative linear solvers (CG/GMRES)
16.012  ❌  FFT / inverse FFT
16.013  ❌  DFT / DCT
16.014  ❌  Konvolüsyon
16.015  ❌  İntegrasyon (numerical quadrature)
16.016  ❌  İnterpolasyon / spline
16.017  ❌  Nonlinear root solving
16.018  ❌  Numerical ODE solvers
16.019  ❌  Optimization (gradient / nonlinear)
16.020  ❌  Büyük doğrusal sistem çözücü
16.021  ❌  std::linalg (C++26 taslağı)
16.022  ❌  std::simd / data-parallel types (C++26 taslağı)
16.023  ❌  SSE SIMD veri-paralel vektör API'si (double skaler SSE2 yeterli değil)
16.024  ❌  AVX/AVX2 vektör hesaplama
16.025  ❌  AVX-512
16.026  ❌  SIMD maskeleme ve gather/scatter
16.027  ❌  CPU feature detection
16.028  ❌  Dinamik CPU dispatch
16.029  ❌  Floating-point environment ve rounding modes
16.030  ❌  Floating-point exceptions
16.031  ❌  Saturating arithmetic
16.032  ❌  Checked integer arithmetic (C++26 taslağı)
16.033  ❌  Bit manipülasyon yardımcıları (popcount, rotl, bit_ceil vb.)
16.034  ❌  Kısa vektör optimizasyonu

17. RASTGELE SAYILAR, SAAT, TARIH
------------------------------------------------------------------------
17.001  ❌  std::mt19937
17.002  ❌  std::mt19937_64
17.003  ❌  std::random_device
17.004  ❌  std::uniform_int_distribution
17.005  ❌  std::uniform_real_distribution
17.006  ❌  std::normal_distribution
17.007  ❌  std::exponential_distribution
17.008  ❌  std::poisson_distribution
17.009  ❌  std::bernoulli_distribution
17.010  ❌  std::discrete_distribution
17.011  ❌  std::shuffle
17.012  ❌  Kriptografik güvenli rastgele sayı API'si (ekosistem/OS)
17.013  ❌  std::chrono::duration
17.014  ❌  std::chrono::time_point
17.015  ❌  std::chrono::steady_clock
17.016  ❌  std::chrono::system_clock
17.017  ❌  std::chrono::high_resolution_clock
17.018  ❌  std::chrono::year_month_day
17.019  ❌  std::chrono::zoned_time
17.020  ❌  Saat dilimi veritabanı
17.021  ❌  Monotonic zamanlayıcı
17.022  ❌  Timer callback/interval
17.023  ❌  CPU cycle counter
17.024  ❌  Yüksek hassasiyetli profil zamanı
17.025  ❌  sleep_for/sleep_until
17.026  ❌  Takvim aritmetiği

18. GIRDI/ÇIKTI, FORMATLAMA, AKIŞLAR
------------------------------------------------------------------------
18.001  🟢  stdin getc
18.002  🟢  stdout putc
18.003  🟢  stdout puti
18.004  🔵  stdout putf, negatif sıfır/NaN dahil hedefli eşleşme
18.005  🟢  Program exit code
18.006  🔵  SEM host mikro-işlemiyle desteklenen host işlemleri
18.007  ❌  Genel std::cin/std::cout/std::cerr
18.008  ❌  std::istream/std::ostream
18.009  ❌  std::iostream
18.010  ❌  std::ifstream/std::ofstream/std::fstream
18.011  ❌  std::stringstream
18.012  ❌  std::istringstream/std::ostringstream
18.013  ❌  std::spanstream (C++23)
18.014  ❌  std::streambuf ve buffering
18.015  ❌  std::syncstream
18.016  ❌  std::ios formatting flags
18.017  ❌  std::setw/std::setprecision/std::fixed
18.018  ❌  std::format
18.019  ❌  std::vformat
18.020  ❌  std::print/std::println
18.021  ❌  C printf/scanf family genel biçimlendirme
18.022  ❌  Binary input/output encoding
18.023  ❌  Seek/tell (rastgele erişim)
18.024  ❌  Buffered I/O API
18.025  ❌  Nonblocking I/O API
18.026  ❌  Asynchronous I/O API
18.027  ❌  Dosya tanıtıcılarını seçme/çoğaltma
18.028  ❌  stdin/stdout redirection API
18.029  ❌  Pipe stream API
18.030  ❌  UTF-8 terminal output handling
18.031  ❌  Terminal raw/canonical mode
18.032  ❌  TTY boyutu ve renk/ANSI kontrolü
18.033  ❌  Çoklu kodlama dönüştürücü

19. DOSYA SISTEMI
------------------------------------------------------------------------
19.001  ❌  open / close
19.002  ❌  read / write genel dosya tanıtıcısı
19.003  ❌  pread / pwrite
19.004  ❌  lseek / seek
19.005  ❌  Dosya create / truncate
19.006  ❌  Dosya rename / unlink
19.007  ❌  Dosya copy / move
19.008  ❌  Dosya / klasör var mı kontrolü
19.009  ❌  Mkdir / rmdir
19.010  ❌  Recursive directory traversal
19.011  ❌  std::filesystem::path
19.012  ❌  std::filesystem::directory_iterator
19.013  ❌  std::filesystem::recursive_directory_iterator
19.014  ❌  std::filesystem::permissions
19.015  ❌  Dosya metadata/stat
19.016  ❌  Dosya sahipliği/chown
19.017  ❌  Hard link / symlink
19.018  ❌  File descriptor flags
19.019  ❌  File locking (fcntl/flock)
19.020  ❌  Memory-mapped file (mmap)
19.021  ❌  Sparse file operations
19.022  ❌  File timestamps
19.023  ❌  fsync/fdatasync
19.024  ❌  Atomic rename/replace strategy
19.025  ❌  File watches (inotify)
19.026  ❌  Extended attributes (xattr)
19.027  ❌  Access control lists (ACL)
19.028  ❌  Filesystem mount bilgisi
19.029  ❌  Temporary files/directories
19.030  ❌  Dosya sistemi hata kodları
19.031  ❌  Directory handle (openat) temelli güvenli erişim
19.032  ❌  Dosya ve dizin dolaşımında sandbox

20. EŞZAMANLILIK VE THREAD
------------------------------------------------------------------------
20.001  ❌  std::thread
20.002  ❌  std::jthread
20.003  ❌  pthread_create/join
20.004  ❌  Thread-local storage
20.005  ❌  std::mutex
20.006  ❌  std::recursive_mutex
20.007  ❌  std::shared_mutex
20.008  ❌  std::timed_mutex
20.009  ❌  std::lock_guard
20.010  ❌  std::unique_lock
20.011  ❌  std::shared_lock
20.012  ❌  std::scoped_lock
20.013  ❌  std::condition_variable
20.014  ❌  condition_variable_any
20.015  ❌  std::semaphore
20.016  ❌  std::latch
20.017  ❌  std::barrier
20.018  ❌  std::future
20.019  ❌  std::promise
20.020  ❌  std::packaged_task
20.021  ❌  std::async
20.022  ❌  Thread pool
20.023  ❌  Work stealing scheduler
20.024  ❌  Task graph runtime
20.025  ❌  Thread cancellation / stop_token
20.026  ❌  Thread priority/affinity
20.027  ❌  Thread synchronization / join policy
20.028  ❌  Thread-safe static initialization
20.029  ❌  Data race detection
20.030  ❌  Lock-order deadlock detection
20.031  ❌  ThreadSanitizer eşdeğerliği
20.032  ❌  Per-thread exception propagation
20.033  ❌  Concurrent queues
20.034  ❌  Parallel algorithms
20.035  ❌  Executors/senders/receivers (C++26 taslağı)

21. ATOMIC VE LOCK-FREE PROGRAMLAMA
------------------------------------------------------------------------
21.001  ❌  std::atomic<T>
21.002  ❌  std::atomic_flag
21.003  ❌  std::atomic_ref
21.004  ❌  Atomik load/store/exchange
21.005  ❌  Compare-exchange weak/strong
21.006  ❌  fetch_add/fetch_sub/fetch_or/fetch_and/fetch_xor
21.007  ❌  C++ memory_order_relaxed
21.008  ❌  memory_order_acquire/release
21.009  ❌  memory_order_acq_rel
21.010  ❌  memory_order_seq_cst
21.011  ❌  Atomic fences
21.012  ❌  atomic wait/notify
21.013  ❌  Mutex-free concurrent stack
21.014  ❌  Lock-free queue
21.015  ❌  ABA sorunu ve çözümü
21.016  ❌  Hazard pointers (C++26 taslağı)
21.017  ❌  Read-copy-update RCU (C++26 taslağı)
21.018  ❌  Epoch-based reclamation
21.019  ❌  Memory reclamation güvenliği
21.020  ❌  Cache-line padding / false-sharing denetimi
21.021  ❌  NUMA-aware memory allocation

22. C++ ÖN YÜZÜ VE KAYNAK TANIMA
------------------------------------------------------------------------
22.001  🟢  PMB1 sayısal programı çözme
22.002  🟢  PMY1 temel yapısal programlarını işleme
22.003  🔵  PMK1 komut, PSEUDO ve REWRITE tanımları
22.004  ❌  C++ lexer/tokenizer
22.005  ❌  C++ expression parser
22.006  ❌  C++ statement parser
22.007  ❌  C++ declaration parser
22.008  ❌  C++ class parser
22.009  ❌  C++ template parser
22.010  ❌  C++ overload resolution
22.011  ❌  C++ type checker
22.012  ❌  C++ constexpr interpreter
22.013  ❌  C++ exception semantic analysis
22.014  ❌  C++ name lookup ve ADL
22.015  ❌  C++ modules import graph
22.016  ❌  C++ preprocessor
22.017  ❌  C++ AST ve incremental parser
22.018  ❌  Language-server protocol (LSP) server
22.019  ❌  C++ semantic highlighting
22.020  ❌  Code completion / go-to-definition
22.021  ❌  Refactoring tool
22.022  ❌  Compile-time diagnostics ve fix-it
22.023  ❌  Incremental compilation dependency tracking
22.024  ❌  Clang/GCC kaynak uyumluluk testi
22.025  ❌  C++20/C++23 standard conformance suite
22.026  ❌  C++26 taslak özellik uyumu

23. ARA BIÇIMLER, DERLEYICI, BAĞLAYICI
------------------------------------------------------------------------
23.001  🟢  PMY1 → düşük düzeyli program üretme
23.002  🟢  Sayısal PMB1 kodlama/çözme
23.003  🟢  Sayısal assembler
23.004  🟢  Sayısal validator
23.005  🟢  Linux x86-64 bağımsız ELF üretme
23.006  🟢  Native call/ret ve temel host kullanımı
23.007  🔵  PMK1 anlam/şablon/rewriting tablosu
23.008  🔵  PMO1 kendi araç kaynakları ve bağlayıcı
23.009  🔵  ret/halt/call/host mikro-şablonları
23.010  🔵  Tablo değişince eski ELF araçlarıyla yeni ELF takımı üretimi
23.011  ❌  Genel register transfer language / LLVM IR denkliği
23.012  ❌  Genel SSA intermediate representation
23.013  ❌  Dominator/post-dominator analysis
23.014  ❌  Alias analysis
23.015  ❌  Loop analysis
23.016  ❌  Interprocedural analysis
23.017  ❌  Genel relocation/linking for user apps
23.018  ❌  Genel ELF symbol table ve dynamic symbol resolution
23.019  ❌  Position-independent code / PIE genel destek
23.020  ❌  Shared object (.so) üretimi
23.021  ❌  Static archive (.a) üretimi
23.022  ❌  Global constructors/destructors startup
23.023  ❌  libc/libstdc++ ABI uyumu
23.024  ❌  C++ name mangling/demangling
23.025  ❌  GDB/DWARF debug info generation
23.026  ❌  Unwind tables / exception handling ABI
23.027  ❌  Custom linker scripts / memory regions
23.028  ❌  Garbage collection of unused sections
23.029  ❌  ThinLTO/LTO genel optimizasyon
23.030  ❌  Incremental linking
23.031  ❌  Link-time symbol visibility/versioning
23.032  ❌  Precompiled headers / modules cache
23.033  ❌  Reproducible build metadata
23.034  ❌  Binary size optimizasyon pipeline

24. DERLEYICI OPTIMIZASYONLARI
------------------------------------------------------------------------
24.001  🟢  Düşük düzeyli immediate komut biçimleri
24.002  🔵  Sayısal REWRITE kurallarıyla immediate seçimi
24.003  ❌  Genel constant folding
24.004  ❌  Constant propagation
24.005  ❌  Sparse conditional constant propagation
24.006  ❌  Common subexpression elimination
24.007  ❌  Global value numbering
24.008  ❌  Dead code elimination
24.009  ❌  Dead store elimination
24.010  ❌  Copy propagation
24.011  ❌  Strength reduction
24.012  ❌  Loop invariant code motion
24.013  ❌  Loop unrolling
24.014  ❌  Loop vectorization
24.015  ❌  SLP vectorization
24.016  ❌  Loop fusion/fission
24.017  ❌  Loop interchange
24.018  ❌  Loop tiling/blocking
24.019  ❌  Loop unswitching
24.020  ❌  Induction variable optimization
24.021  ❌  Inlining
24.022  ❌  Interprocedural specialization
24.023  ❌  Devirtualization
24.024  ❌  Escape analysis
24.025  ❌  Scalar replacement of aggregates
24.026  ❌  Memory-to-register promotion
24.027  ❌  Global register allocation (graph-coloring/linear-scan)
24.028  ❌  Instruction scheduling
24.029  ❌  Instruction selection with DAG/isel
24.030  ❌  Peephole optimization (genel)
24.031  ❌  Tail call elimination
24.032  ❌  Branch prediction guided layout
24.033  ❌  Profile-guided optimization (PGO)
24.034  ❌  Link-time optimization (LTO)
24.035  ❌  Whole-program analysis
24.036  ❌  CPU architecture tuning / microarchitecture model
24.037  ❌  Optimization level -O0/-O1/-O2/-O3 denkliği
24.038  ❌  Size optimization -Os/-Oz denkliği
24.039  ❌  Autovectorization
24.040  ❌  Undefined-behavior-aware optimization contracts
24.041  ❌  JIT adaptive recompilation

25. DEBUG, ANALIZ, DOĞRULAMA, ARAÇLAR
------------------------------------------------------------------------
25.001  🟢  Bağımsız C++ referansa karşı differential tests
25.002  🟢  Rastgele test girdileri ve malformed mutant denemeleri
25.003  🟢  DIV0/OOB/fuel/stack sınır testleri
25.004  🔵  Sabotaj/mutasyon testleri ve kaçak mutant düzeltmeleri
25.005  🔵  Hızlı test betiği
25.006  🔵  PMO1 sayı ↔ okunabilir görünüm round-trip
25.007  ❌  DWARF line/variable metadata
25.008  ❌  GDB ile sembolik debugging
25.009  ❌  Breakpoint/watchpoint support
25.010  ❌  Single-step execution
25.011  ❌  Stack trace symbolization
25.012  ❌  Heap profiler
25.013  ❌  CPU sampling profiler
25.014  ❌  Flame graph üretim araçları
25.015  ❌  Code coverage collector
25.016  ❌  Branch/condition coverage
25.017  ❌  MC/DC coverage
25.018  ❌  Property-based testing framework genel amaçlı
25.019  ❌  Structured fuzz coverage guidance
25.020  ❌  AddressSanitizer eşdeğer native enstrümantasyon
25.021  ❌  MemorySanitizer eşdeğerliği
25.022  ❌  ThreadSanitizer eşdeğerliği
25.023  ❌  UndefinedBehaviorSanitizer eşdeğerliği
25.024  ❌  LeakSanitizer eşdeğerliği
25.025  ❌  Static analyzer
25.026  ❌  Taint analysis
25.027  ❌  Symbolic execution
25.028  ❌  SMT solver entegrasyonu
25.029  ❌  Formal verification
25.030  ❌  Proof-carrying code
25.031  ❌  Performance regression CI
25.032  ❌  Crash dump analizi
25.033  ❌  Reproducible debug builds
25.034  ❌  Race/deadlock trace

26. LINUX SÜREÇ, ZAMANLAYICI VE TERMINAL
------------------------------------------------------------------------
26.001  🟢  Linux x86-64 kullanıcı sürecinde ELF çalıştırma
26.002  🟢  Temel read/write/exit host işlemleri
26.003  ❌  fork
26.004  ❌  execve
26.005  ❌  posix_spawn
26.006  ❌  wait/waitpid/waitid
26.007  ❌  clone/clone3
26.008  ❌  pidfd_open/pidfd_send_signal
26.009  ❌  getpid/gettid
26.010  ❌  setsid/setpgid
26.011  ❌  Process groups/job control
26.012  ❌  Signals/sigaction/sigprocmask
26.013  ❌  signalfd
26.014  ❌  timerfd
26.015  ❌  eventfd
26.016  ❌  alarm/clock_nanosleep
26.017  ❌  gettimeofday/clock_gettime
26.018  ❌  Process CPU affinity
26.019  ❌  sched_getscheduler / sched_setscheduler
26.020  ❌  Scheduling priorities / nice
26.021  ❌  getrusage/resource limits
26.022  ❌  prlimit / setrlimit
26.023  ❌  Process environment (getenv/setenv)
26.024  ❌  argc/argv genel kabulü ve işlenmesi
26.025  ❌  Unix terminal ioctl/termios
26.026  ❌  PTY allocation
26.027  ❌  Pipelines ve terminal interaktif kontrol
26.028  ❌  Process exit hooks/atexit
26.029  ❌  Core dump generation/configuration
26.030  ❌  Procfs /proc inspection
26.031  ❌  Namespace-aware process startup

27. LINUX BELLEK, IPC VE YÜKSEK PERFORMANSLI I/O
------------------------------------------------------------------------
27.001  ❌  mmap/munmap
27.002  ❌  mprotect
27.003  ❌  mremap
27.004  ❌  madvise
27.005  ❌  mlock/munlock
27.006  ❌  memfd_create
27.007  ❌  shm_open POSIX shared memory
27.008  ❌  System V shared memory
27.009  ❌  pipe/pipe2
27.010  ❌  socketpair
27.011  ❌  Unix domain sockets
27.012  ❌  POSIX message queues
27.013  ❌  System V message queues
27.014  ❌  POSIX semaphores
27.015  ❌  futex
27.016  ❌  epoll
27.017  ❌  select/poll
27.018  ❌  io_uring setup/submit/completion
27.019  ❌  sendfile
27.020  ❌  splice/tee/vmsplice
27.021  ❌  copy_file_range
27.022  ❌  inotify
27.023  ❌  fanotify
27.024  ❌  eventfd notification
27.025  ❌  Timerfd integration
27.026  ❌  File descriptor passing (SCM_RIGHTS)
27.027  ❌  Shared memory synchronization
27.028  ❌  Zero-copy pipeline
27.029  ❌  NUMA policy / mbind
27.030  ❌  Huge pages / transparent huge pages
27.031  ❌  Direct I/O
27.032  ❌  O_DIRECT alignment semantics
27.033  ❌  eBPF syscall integration
27.034  ❌  Userfaultfd
27.035  ❌  Pagemap/mincore/query

28. LINUX AĞ, SOCKET VE INTERNET
------------------------------------------------------------------------
28.001  ❌  Berkeley sockets API
28.002  ❌  TCP client
28.003  ❌  TCP server
28.004  ❌  TCP keepalive
28.005  ❌  TCP tuning (nodelay, buffers)
28.006  ❌  UDP sockets
28.007  ❌  IPv4
28.008  ❌  IPv6
28.009  ❌  Unix domain sockets for networking
28.010  ❌  DNS lookup/getaddrinfo
28.011  ❌  Reverse DNS
28.012  ❌  Nonblocking sockets
28.013  ❌  Multiplexing (epoll)
28.014  ❌  Concurrent connections
28.015  ❌  TLS client
28.016  ❌  TLS server
28.017  ❌  Certificate validation
28.018  ❌  Mutual TLS
28.019  ❌  HTTP/1.1 client
28.020  ❌  HTTP/1.1 server
28.021  ❌  HTTP/2
28.022  ❌  HTTP/3 (QUIC)
28.023  ❌  WebSocket
28.024  ❌  Server-Sent Events
28.025  ❌  REST API
28.026  ❌  gRPC/Protocol Buffers
28.027  ❌  URL parsing
28.028  ❌  HTTP cookie/session
28.029  ❌  HTTP compression
28.030  ❌  Proxy HTTP/SOCKS
28.031  ❌  Load balancing
28.032  ❌  Rate limiting / backpressure
28.033  ❌  Network timeout/retry/circuit breaker
28.034  ❌  Multipart uploads
28.035  ❌  Streaming downloads
28.036  ❌  Web server/router/middleware
28.037  ❌  Connection pooling
28.038  ❌  Network packet capture
28.039  ❌  Raw sockets (yetki gerektirebilir)
28.040  ❌  Multicast
28.041  ❌  mDNS/SSDP
28.042  ❌  NTP client
28.043  ❌  FTP/SFTP client
28.044  ❌  SSH client/server
28.045  ❌  Socket error classification

29. LINUX GÜVENLIK, IZOLASYON, YETKILENDIRME
------------------------------------------------------------------------
29.001  🟢  Geçersiz sayısal programı reddeden doğrulayıcı
29.002  🟢  Bellek sınırı ve yakıt kotası
29.003  🔵  Sayısal semantikten türetilmiş doğrulama kuralları
29.004  ❌  Dosya yetkileri ve erişim kontrolü
29.005  ❌  User/group IDs
29.006  ❌  Linux capabilities
29.007  ❌  seccomp-bpf filtreleri
29.008  ❌  User namespace
29.009  ❌  Mount namespace
29.010  ❌  PID namespace
29.011  ❌  Network namespace
29.012  ❌  IPC namespace
29.013  ❌  cgroups bellek/CPU/pid limitleri
29.014  ❌  chroot/pivot_root
29.015  ❌  AppArmor / SELinux policy entegrasyonu
29.016  ❌  no_new_privs
29.017  ❌  Privilege dropping
29.018  ❌  Privilege separation
29.019  ❌  Process sandbox
29.020  ❌  Namespace + seccomp + filesystem isolation
29.021  ❌  Crypto hash (SHA-256/512)
29.022  ❌  HMAC
29.023  ❌  Digital signatures (Ed25519/RSA/ECDSA)
29.024  ❌  Trusted public key distribution
29.025  ❌  Signed PMK1/PMO1 packages
29.026  ❌  Versioned signed validation policy
29.027  ❌  Anti-rollback
29.028  ❌  Authenticated updates
29.029  ❌  Transactional updates / rollback
29.030  ❌  Audit logs
29.031  ❌  Authorization / RBAC / ACL
29.032  ❌  Secrets management
29.033  ❌  Secure memory zeroization
29.034  ❌  Certificate chain validation
29.035  ❌  Secure boot / TPM attestation entegrasyonu
29.036  ❌  ASLR/PIE/RELRO/stack canary gibi üretim sertleştirmesi
29.037  ❌  W^X/JIT executable memory discipline
29.038  ❌  CFI / shadow stack
29.039  ❌  Network input taint tracking
29.040  ❌  Program-level capabilities
29.041  ❌  Cryptographic random entropy API
29.042  ❌  Crash recovery isolation

30. 2D/3D GRAFIK, PENCERE VE KULLANICI ARAYÜZÜ
------------------------------------------------------------------------
30.001  🟢  Sanal framebuffer / piksel çizimi
30.002  🟢  present/poll/frame host işlemleri (headless)
30.003  🟢  PPM/ASCII render çıktısı
30.004  🟢  Pong örnek programı (eski rapor; tekil yeniden denetlenmedi)
30.005  🟢  Life örnek programı (eski rapor; tekil yeniden denetlenmedi)
30.006  🟢  Parçacık örneği (eski rapor; tekil yeniden denetlenmedi)
30.007  🔵  HOSTX SEM komutuyla desteklenen grafik host eylemleri
30.008  ❌  X11 pencere sistemi
30.009  ❌  Wayland istemcisi
30.010  ❌  SDL2/SDL3 pencere ve input eşdeğeri
30.011  ❌  GLFW pencere/event eşdeğeri
30.012  ❌  Qt Widgets / Qt Quick
30.013  ❌  GTK kullanıcı arayüzü
30.014  ❌  ImGui gibi immediate-mode GUI
30.015  ❌  Pencere oluşturma/kapatma
30.016  ❌  Pencere olay döngüsü
30.017  ❌  Klavye girişi
30.018  ❌  Fare/touch/gesture girişi
30.019  ❌  Gamepad ve joystick
30.020  ❌  Ekran çözünürlüğü / DPI
30.021  ❌  Clipboard/drag-and-drop
30.022  ❌  Font yükleme ve metin render
30.023  ❌  2D antialiasing ve vector graphics
30.024  ❌  SVG işleme
30.025  ❌  OpenGL context
30.026  ❌  Vulkan instance/device
30.027  ❌  Graphics command buffers
30.028  ❌  GPU buffer/texture allocation
30.029  ❌  Shader derleme (GLSL/SPIR-V)
30.030  ❌  Compute shader
30.031  ❌  Texture sampling, filtering, mipmapping
30.032  ❌  3D mesh geometry
30.033  ❌  Perspective camera / projection
30.034  ❌  Depth/stencil tests
30.035  ❌  Lighting/shadows
30.036  ❌  Skeletal animation
30.037  ❌  Material/PBR shading
30.038  ❌  Postprocessing / HDR / tone mapping
30.039  ❌  Ray tracing
30.040  ❌  GPU synchronization / fences
30.041  ❌  Multi-monitor / fullscreen
30.042  ❌  Accessible UI / screen readers

31. OYUN MOTORU, FIZIK, SIMÜLASYON
------------------------------------------------------------------------
31.001  🟢  Basit 2D oyun/simülasyon örnekleri
31.002  🟢  Frame tabanlı çizim
31.003  ❌  Entity-component-system (ECS)
31.004  ❌  Scene graph
31.005  ❌  Prefab/asset instantiation
31.006  ❌  Sprite atlas/animation
31.007  ❌  2D physics collision
31.008  ❌  3D rigid-body dynamics
31.009  ❌  Collision shapes and broadphase/narrowphase
31.010  ❌  Continuous collision detection
31.011  ❌  Constraint/joint solver
31.012  ❌  Character controller
31.013  ❌  Vehicle physics
31.014  ❌  Ragdoll physics
31.015  ❌  Soft bodies / cloth
31.016  ❌  Fluid simulation
31.017  ❌  Particle GPU simulation
31.018  ❌  Navmesh navigation
31.019  ❌  A* / Dijkstra pathfinding library
31.020  ❌  Behavior trees / finite-state AI
31.021  ❌  Game event bus
31.022  ❌  Input action map
31.023  ❌  Scene serialization
31.024  ❌  Level editor
31.025  ❌  Script embedding API
31.026  ❌  Hot asset reload
31.027  ❌  Resource/asset streaming
31.028  ❌  Networking multiplayer state sync
31.029  ❌  Client prediction / server reconciliation
31.030  ❌  Deterministic rollback networking
31.031  ❌  Save/load game state
31.032  ❌  Animation blending
31.033  ❌  Terrain and LOD
31.034  ❌  Occlusion culling / frustum culling
31.035  ❌  Audio spatialization
31.036  ❌  Game profiling/telemetry
31.037  ❌  Cross-platform asset formats
31.038  ❌  Vulkan/OpenGL rendering pipeline

32. SES, VIDEO, GÖRÜNTÜ VE MEDYA
------------------------------------------------------------------------
32.001  ❌  PCM ses tamponları
32.002  ❌  WAV okuma/yazma
32.003  ❌  MP3 decoding/encoding
32.004  ❌  AAC/Opus codecs
32.005  ❌  ALSA ses aygıtı
32.006  ❌  PulseAudio/PipeWire entegrasyonu
32.007  ❌  PortAudio/SDL audio
32.008  ❌  Mikrofon capture
32.009  ❌  Ses oynatma ve mixer
32.010  ❌  Sample-rate conversion
32.011  ❌  Audio resampling
32.012  ❌  FFT tabanlı audio DSP
32.013  ❌  Noise suppression
32.014  ❌  Echo cancellation
32.015  ❌  Spatial 3D audio/HRTF
32.016  ❌  MIDI okuma/yazma
32.017  ❌  PNG görüntü okuma/yazma
32.018  ❌  JPEG okuma/yazma
32.019  ❌  WebP/AVIF görüntü formatları
32.020  ❌  EXR/HDR görüntü
32.021  ❌  GIF/APNG animasyon
32.022  ❌  Video demux/mux
32.023  ❌  H.264/H.265/AV1 decode
32.024  ❌  Video encode
32.025  ❌  FFmpeg/GStreamer pipeline
32.026  ❌  Webcam capture / V4L2
32.027  ❌  Image resize/interpolation
32.028  ❌  Color space conversion
32.029  ❌  Image filters/convolutions
32.030  ❌  Computer vision feature extraction
32.031  ❌  Optical flow
32.032  ❌  OCR
32.033  ❌  Streaming video playback
32.034  ❌  Real-time audio-video sync
32.035  ❌  WebRTC
32.036  ❌  Screen capture

33. VERITABANI, DEPOLAMA VE VERI BIÇIMLERI
------------------------------------------------------------------------
33.001  ❌  SQLite istemci ve sorgu
33.002  ❌  SQLite embedded veritabanı eşdeğeri
33.003  ❌  PostgreSQL client
33.004  ❌  MySQL/MariaDB client
33.005  ❌  Redis client
33.006  ❌  MongoDB client
33.007  ❌  RocksDB/LevelDB embedded KV store
33.008  ❌  SQL lexer/parser/query executor
33.009  ❌  Transactions / ACID
33.010  ❌  Write-ahead logging (WAL)
33.011  ❌  B-tree / B+tree indexes
33.012  ❌  LSM-tree storage
33.013  ❌  Query planning/optimization
33.014  ❌  MVCC
33.015  ❌  Connection pooling
33.016  ❌  Schema migrations
33.017  ❌  ORM/data mapper
33.018  ❌  JSON parser
33.019  ❌  JSON serializer
33.020  ❌  YAML parser
33.021  ❌  TOML parser
33.022  ❌  INI/config parser
33.023  ❌  XML parser
33.024  ❌  CSV/TSV parser
33.025  ❌  Protocol Buffers
33.026  ❌  FlatBuffers
33.027  ❌  Cap'n Proto
33.028  ❌  MessagePack / CBOR
33.029  ❌  Parquet / Arrow
33.030  ❌  Binary schema evolution
33.031  ❌  Compression gzip/zlib
33.032  ❌  Compression zstd/lz4
33.033  ❌  ZIP/tar archive handling
33.034  ❌  Checksums/CRC
33.035  ❌  Content-addressed storage
33.036  ❌  Memory mapped analytics data
33.037  ❌  Dataframe/table library

34. YAPAY ZEKÂ, MAKINE ÖĞRENMESI VE BILIM
------------------------------------------------------------------------
34.001  🟢  AI tarafından düşük düzeyli/yapısal program üretiminin küçük örnek deneyleri
34.002  🔵  Sayısal parametrelerden yeni komut tanımı, AI yazımı için yapı sağlıyor
34.003  ❌  Tensor object model
34.004  ❌  Tensor broadcasting
34.005  ❌  Tensor strides/views
34.006  ❌  Tensor slicing/gather/scatter
34.007  ❌  Automatic differentiation (autograd)
34.008  ❌  Gradient computation
34.009  ❌  Optimizer SGD
34.010  ❌  Optimizer Adam/AdamW
34.011  ❌  Neural network layers
34.012  ❌  Convolution operators
34.013  ❌  Attention / Transformer kernels
34.014  ❌  Embedding layers
34.015  ❌  LayerNorm/RMSNorm
34.016  ❌  Activation functions
34.017  ❌  Cross-entropy / loss functions
34.018  ❌  Backpropagation
34.019  ❌  Forward-mode autodiff
34.020  ❌  Model serialization/checkpoint
34.021  ❌  Mixed precision FP16/BF16
34.022  ❌  Quantization INT8/INT4
34.023  ❌  GPU tensor operators
34.024  ❌  CUDA/HIP/SYCL backend
34.025  ❌  Distributed training
34.026  ❌  Inference runtime
34.027  ❌  ONNX import/export
34.028  ❌  Tokenizers
34.029  ❌  Language model inference
34.030  ❌  Language model training
34.031  ❌  Automatic program synthesis as mature subsystem
34.032  ❌  Automated program repair
34.033  ❌  Auto architecture search
34.034  ❌  Scientific statistics/regression library
34.035  ❌  Probabilistic programming
34.036  ❌  Graph algorithms library
34.037  ❌  Simulation solver framework
34.038  ❌  Physics/numerical PDE solver
34.039  ❌  Finite element method (FEM)
34.040  ❌  Monte Carlo simulation framework
34.041  ❌  Robotics motion planning
34.042  ❌  SLAM and sensor fusion
34.043  ❌  Genomics/bioinformatics packages

35. SUNUCU ALTYAPISI, BULUT VE DAĞITIK SISTEMLER
------------------------------------------------------------------------
35.001  ❌  HTTP application server
35.002  ❌  Reverse proxy
35.003  ❌  Web API router/middleware
35.004  ❌  RPC server
35.005  ❌  GraphQL server
35.006  ❌  Authentication/session service
35.007  ❌  User account service
35.008  ❌  Job/task queue
35.009  ❌  Persistent message broker
35.010  ❌  Publish-subscribe messaging
35.011  ❌  WebSocket pubsub
35.012  ❌  Distributed caching
35.013  ❌  Service discovery
35.014  ❌  Health checks/readiness/liveness
35.015  ❌  Metrics exporter (Prometheus)
35.016  ❌  Structured logs
35.017  ❌  Distributed tracing / OpenTelemetry
35.018  ❌  Circuit breaker/retry strategy
35.019  ❌  Backpressure and flow control
35.020  ❌  Rate limiter
35.021  ❌  Connection pool manager
35.022  ❌  Event streaming (Kafka client)
35.023  ❌  Distributed consensus (Raft)
35.024  ❌  Leader election
35.025  ❌  Service replication
35.026  ❌  Distributed locks
35.027  ❌  Distributed transactions
35.028  ❌  Shard/rebalance
35.029  ❌  Snapshotting and backups
35.030  ❌  Load shedding
35.031  ❌  Container runtime integration
35.032  ❌  Kubernetes API client
35.033  ❌  Service mesh protocol integration
35.034  ❌  Autoscaling telemetry
35.035  ❌  Fault injection / chaos testing

36. KRIPTOGRAFI VE VERI GÜVENLIĞI
------------------------------------------------------------------------
36.001  ❌  SHA-2 hashing
36.002  ❌  SHA-3 hashing
36.003  ❌  BLAKE2/BLAKE3 hashing
36.004  ❌  HMAC
36.005  ❌  HKDF
36.006  ❌  PBKDF2
36.007  ❌  Argon2 password hashing
36.008  ❌  AES-GCM
36.009  ❌  AES-CTR/CBC
36.010  ❌  ChaCha20-Poly1305
36.011  ❌  RSA encryption/signing
36.012  ❌  Ed25519 signatures
36.013  ❌  ECDSA signatures
36.014  ❌  ECDH/X25519 key agreement
36.015  ❌  X.509 certificate parsing
36.016  ❌  TLS protocol implementation/integration
36.017  ❌  Certificate revocation/OCSP
36.018  ❌  Secure random generator
36.019  ❌  Constant-time compare
36.020  ❌  Memory-safe secret handling
36.021  ❌  Key rotation
36.022  ❌  Key derivation lifecycle
36.023  ❌  Certificate pinning
36.024  ❌  Signed ELF artifacts
36.025  ❌  Signed numeric parameter datasets
36.026  ❌  Merkle trees
36.027  ❌  Authenticated serialization
36.028  ❌  Tamper-evident audit trail
36.029  ❌  Reproducible authenticated updates
36.030  ❌  Secrets vault integration
36.031  ❌  FIDO2/WebAuthn protocol
36.032  ❌  Password auth and MFA
36.033  ❌  Secure enclave/TPM integration
36.034  ❌  Crypto agility and algorithm versioning

37. CIHAZLAR, IŞLETIM SISTEMI ÇEKIRDEĞI, GÖMÜLÜ
------------------------------------------------------------------------
37.001  ❌  Character device drivers
37.002  ❌  Block device drivers
37.003  ❌  PCIe device enumeration
37.004  ❌  USB device access / libusb
37.005  ❌  Serial ports (tty)
37.006  ❌  I2C / SPI
37.007  ❌  GPIO control
37.008  ❌  CAN bus
37.009  ❌  Bluetooth / BLE
37.010  ❌  Wi-Fi device management
37.011  ❌  HID joystick/input
37.012  ❌  Video4Linux camera
37.013  ❌  DRM/KMS direct graphics
37.014  ❌  Linux kernel module
37.015  ❌  eBPF programs
37.016  ❌  Userspace I/O (UIO/VFIO)
37.017  ❌  Device DMA buffers
37.018  ❌  Interrupt handling
37.019  ❌  Real-time scheduling
37.020  ❌  Embedded RTOS integration
37.021  ❌  Bootloader development
37.022  ❌  Firmware update protocol
37.023  ❌  Memory-mapped peripheral registers
37.024  ❌  Hardware abstraction layer
37.025  ❌  Sensor sampling/filtering
37.026  ❌  Industrial protocols (Modbus/OPC-UA)
37.027  ❌  Robotics middleware (ROS2)
37.028  ❌  Microcontroller cross-compilation
37.029  ❌  Bare-metal runtime
37.030  ❌  Kernel-mode debugging
37.031  ❌  Power management / thermal data

38. PAKETLEME, YAPI SISTEMI VE YAYINLAMA
------------------------------------------------------------------------
38.001  🔵  PMO1 motor-araç kaynak nesneleri
38.002  🔵  Proje haritası ve commit'li değişiklik günlüğü
38.003  ❌  Genel-purpose package/module dependency manager
38.004  ❌  CMake benzeri yapı sistemi
38.005  ❌  Ninja/Make benzeri dependency scheduler
38.006  ❌  Compile database (compile_commands.json)
38.007  ❌  Code indexing
38.008  ❌  Incremental builds
38.009  ❌  Distributed compilation
38.010  ❌  Build cache
38.011  ❌  Reproducible builds
38.012  ❌  Build graph visualizer
38.013  ❌  SemVer package resolution
38.014  ❌  Package lockfile
38.015  ❌  Static library packaging
38.016  ❌  Shared library packaging
38.017  ❌  Versioned ABI compatibility checks
38.018  ❌  Application installer / Linux package
38.019  ❌  Containerized build environment
38.020  ❌  Continuous integration pipeline
38.021  ❌  Automated release/version bump
38.022  ❌  SBOM üretimi
38.023  ❌  Software artifact signing
38.024  ❌  License compliance scanning
38.025  ❌  Dependency vulnerability scanning
38.026  ❌  Binary diff / delta update
38.027  ❌  Benchmark dashboard
38.028  ❌  Documentation generator
38.029  ❌  API reference publishing
38.030  ❌  Unit/integration/system test runner genel çatı
38.031  ❌  Artifact repository
38.032  ❌  Plugin loading subsystem
38.033  ❌  Backward-compatibility migration tools

39. OTOMASYON, UYGULAMA ENTEGRASYONU VE BELGE ÜRETIMI
------------------------------------------------------------------------
39.001  ❌  Shell command spawn + güvenli parametreleme
39.002  ❌  Child-process stdout/stderr capture
39.003  ❌  Environment management
39.004  ❌  JSON-RPC integration
39.005  ❌  DBus Linux service integration
39.006  ❌  Cron/systemd timer integration
39.007  ❌  systemd service lifecycle
39.008  ❌  GUI automation (X11/Wayland portal)
39.009  ❌  Clipboard integration
39.010  ❌  PDF generation engine
39.011  ❌  DOCX generation engine
39.012  ❌  XLSX generation engine
39.013  ❌  PPTX generation engine
39.014  ❌  HTML rendering engine
39.015  ❌  Markdown parser/render
39.016  ❌  Templating engine
39.017  ❌  Email SMTP sender
39.018  ❌  IMAP email client
39.019  ❌  OAuth2/OpenID Connect client
39.020  ❌  Filesystem watcher automation
39.021  ❌  Workflow executor
39.022  ❌  Distributed task scheduler
39.023  ❌  Rule engine
39.024  ❌  Data serialization bridge
39.025  ❌  Plugin ABI and extension points
39.026  ❌  Remote configuration management
39.027  ❌  Webhooks
39.028  ❌  Event subscriptions
39.029  ❌  Notification daemon integration
39.030  ❌  Localization/i18n catalog

40. MOTORUMUZUN C++'TAN AYRI, ÖZGÜN MIMARI HEDEFLERI
------------------------------------------------------------------------
40.001  🟢  PMB1 sayısal ikili komut biçimi
40.002  🟢  PMY1 yapılandırılmış sayısal program tanımı
40.003  🟢  Sayı programının doğrulanıp ELF'e çevrilmesi
40.004  🟢  Bağımsız ELF araç zincirinin kendisini yeniden üretmesi
40.005  🔵  PMK1 sayısal komut kimliği ve tanım tablosu
40.006  🔵  PRIM 47 mikro-işlem tanımı
40.007  🔵  SEM 55 komut mikro-programı
40.008  🔵  PSEUDO takma komut tanımı
40.009  🔵  REWRITE sayısal dönüştürme kuralları
40.010  🔵  Sayısal doğrulama özellikleri SEM'den türetme
40.011  🔵  TOKEN ve TEMPLATE x86 şablonları
40.012  🔵  FMIN yalnız sayısal komut tanımıyla
40.013  🔵  GOTO yalnız sayısal komut tanımıyla
40.014  🔵  XORI ve xor→xori dönüştürme kuralı
40.015  🔵  PMO1 sayısal assembler/validator/gen2 kaynakları
40.016  🔵  Sayısal nesne bağlayıcı
40.017  🔵  PMO1 kayıpsız metin görünümü/düzenleme
40.018  🔵  ret mikro-işlem şablonu
40.019  🔵  halt mikro-işlem şablonu
40.020  🔵  call mikro-işlem şablonu (günlükte bildirildi)
40.021  🔵  host mikro-işlem şablonu
40.022  🔵  Eski ELF takımı → yeni komutlu ELF takımı deneyi
40.023  🔵  Hızlı regresyon/test sistemi
40.024  🔵  Kural ve şablon sabotaj testleri
40.025  ❌  Bütün araçların yalnızca sayısal kanonik kaynakta tutulması
40.026  ❌  Bütün mikro-işlem primitiflerinin en alt düzeyde tamamen tablosallaştırılması (kapsam tanımı gerekli)
40.027  ❌  Yeni şablon simgesinin gen2 değişmeden yalnız sayısal reçeteyle tanımlanması (sonraki hedef)
40.028  ❌  Her yeni işlemci arka ucunu yalnız tabloyla tanımlama
40.029  ❌  Gerçek genel amaçlı parametre sürümleme/migrasyon
40.030  ❌  Üretim parametreleri için imzalı sürümler
40.031  ❌  Yetkiye dayalı çalışma zamanı
40.032  ❌  Zorunlu işlem kotası ve OS sandbox entegrasyonu
40.033  ❌  Otomatik kod/parametre düzeltme
40.034  ❌  AI'nın karmaşık uygulamaları doğrudan parametre olarak güvenilir üretmesi
40.035  ❌  Gerçek büyük yazılım ölçeğinde üretim kanıtı
40.036  ❌  Çok çekirdekli parametre derleme/yürütme
40.037  ❌  Sayısal programların dil bağımsız kitaplık dağıtımı
40.038  ❌  Kendi optimizasyon kurallarını öğrenip doğrulama
40.039  ❌  Farklı x86 makine kodu varyantları için doğrulanmış çeşitlendirme
40.040  ❌  Platform ABI adapterlarının parametrelerle genişletilmesi

41. DIŞARIDAN KAYNAK KOD VE KÜTÜPHANE IÇERI AKTARMA (SAYISAL DÖNÜŞÜM HEDEFI)
------------------------------------------------------------------------
41.001  ❌  Kullanıcının veya AI'nın verdiği kaynak kodu dosya, arşiv ya da proje olarak içeri alabilme
41.002  ❌  Birden fazla dosya ve klasörlü kaynak proje keşfi
41.003  ❌  Güvenilir kaynak paketi manifesti ve giriş noktası tanımlama
41.004  ❌  C kaynak kodunu sayısal ara biçime çevirme
41.005  ❌  C++ kaynak kodunu sayısal ara biçime çevirme (genel kapsam)
41.006  ❌  Dış kodun kullanıldığı dil ve sürümü güvenilir belirleme
41.007  ❌  C/C++ tokenlaştırma ve sözdizimi ayrıştırma
41.008  ❌  Makro, koşullu derleme ve include işleme
41.009  ❌  C++ şablon örnekleme ve tür çıkarımı için çeviri uyumluluğu
41.010  ❌  C++ sınıf, nesne yaşam döngüsü ve ABI kurallarını çevirme
41.011  ❌  Kaynak koddaki veri türlerini sayısal yerleşime uyarlama
41.012  ❌  Yapı, enum ve union yerleşimlerini koruyabilme
41.013  ❌  İşaretçi, referans, lifetime, aliasing ve bellek semantiği uyarlama
41.014  ❌  İstisna, RTTI ve sanal çağrıları sayısal çalışma modeline uyarlama
41.015  ❌  Fonksiyonların çağrı sözleşmelerini ve argümanlarını uyarlama
41.016  ❌  Gerekli üçüncü taraf kod parçalarını bağımlılıklarıyla birlikte içeri alabilme
41.017  ❌  C++ standart kütüphane başlıklarını kullanan dış kodu işleme
41.018  ❌  Dış kaynak sembollerini çözme ve sayısal modülleri bağlama
41.019  ❌  Başlık dosyası ile derleme birimi arasındaki bağımlılıkları çözme
41.020  ❌  Statik kaynak paketlerini tam sayısal nesnelere dönüştürme
41.021  ❌  Sayısal modüllerde sürüm ve uyumluluk denetimi
41.022  ❌  Kaynak kod ve çıkan sayısal modül arasında izlenebilir haritalama
41.023  ❌  Dönüştürme sırasında derlenemeyen özellikleri açık raporlama
41.024  ❌  Kaynak-dönüşüm eşdeğerliği için bağımsız referans testleri
41.025  ❌  Dış programın girdiler/çıktılar ve hata davranışında eşdeğerlik testleri
41.026  ❌  Çalışma zamanı dış bağımlılıklarını tespit etme ve bildirme
41.027  ❌  İçe aktarılan kod için Linux sistem çağrısı ve yetki arabirimi
41.028  ❌  Kaynak kodu güvenilmeyen veri sayan doğrulama ve izolasyon
41.029  ❌  İçe aktarılan kodun lisans ve dağıtım koşullarını izleme
41.030  ❌  Kaynak kodu olmayan hazır ELF/.so ikilisini ayırma; otomatik kaynak çevirisi saymama
41.031  ❌  Gerekirse ikili kod analizi için ayrı, doğrulanabilir içeri aktarma yöntemi
41.032  ❌  Dış kaynak paketinden, C++/Python'a çalışma zamanı bağımlılığı olmadan native ELF oluşturma
41.033  ❌  İçe aktarma sonrasında yeni sürümü yalnızca sayısal parametrelerden tekrar üretme
41.034  ❌  AI'nın doğrudan sayısal program üretmesi ve gerektiğinde kaynak kodu dönüştürmesi için ortak doğrulama
41.035  ❌  İçeri aktarılan kod üzerinde sayısal optimizasyon ve hata tanısı
41.036  ❌  Büyük ve gerçek C/C++ projeleriyle ölçek / davranış / performans karşılaştırması
