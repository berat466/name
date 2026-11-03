# Yapılanlar günlüğü (Parametre Motoru)

Kronolojik kayıt: **ne yapıldı, hangi commit, hangi testle doğrulandı.** Yeni madde bitince en alta eklenir (kural: `PROJE_HARITASI.md` §6). Konum haritası için `PROJE_HARITASI.md`. Commit özetleri `git log` kaynaklıdır; test sayıları ilgili commit/rapor kaynaklıdır. Rakam yazmadığım yerde sayıyı bilmiyorum.

## Aşama 0 — motor ve ilk uygulamalar (2026-10-27/28)
| Commit | Ne | Kanıt |
|---|---|---|
| 8eb302b | Şartname, yükleyici, ikili kodlama (PMB1), doğrulama, VM, testler | `tests/run_tests.sh` |
| 0a3540b, b7560a1 | Profil deneyi: 1,03 s nereye gidiyor (motor değişmedi) | `PROFIL_RAPORU.md` |
| 0adeac6, f82a2ac | Kod üretimi eniyileme: doğrudan-operand komutlar, kesin atama ile sıfırlama atlama; 55 komut | `KOD_URETIMI_RAPORU.md` |
| 3b884e4 | Anlam tablosu (`anlam.h`), tablodan yorumlayıcı, x86-64 makine kodu yürütücüsü, yakıt noktaları | `NATIVE_RAPORU.md` |
| c053d17, 6a9faf1 | Yazmaç dağıtımı, karşılaştır-ve-atla; ondalık komutlar + SSE2; fuzz'ın bulduğu iki hata düzeltildi | fuzz karşılaştırması |
| 805c388 | Ekran/girdi kapısı (`present/poll/frame`) + üç uygulama (hayat, sahne, pong) | bağımsız awk doğrulaması |
| 78c8880 | Şablon/örnekleme katmanı (`.pmt`), davranış kütüphanesi, beş uygulama | `SABLON_RAPORU.md` |
| 4e09117, 8697187, 7674461, 459a913, 1718025, 7b43451 | AI testleri: görev kehanetleri, doğrudan sayısal parametre testi (6 ajan), yapısal tablo PMY1 (cözücü, özelleştirici, `pm yapisal/tablo`) | `AI_*_RAPORU.md` |
| cd4bc54 … a74d137 | Büyük ölçek testi (26 ajan, kısmi sonuç), rapor ve yarıda kesilme nedeni açıkça yazıldı | `AI_BUYUK_OLCEK_RAPORU.md` |

## Aşama 1 — öz-barındırma: C++ bileşenleri parametre tablosu olarak (2026-10-28)
| Commit | Ne | Kanıt |
|---|---|---|
| a64b63a | Envanter, "oku" ilkesi, paketleyici tablo olarak | bayt-bayt |
| ef903a6 | PMB1 kodlayıcı + çözücü tablo olarak; `pm liste/yazliste` referansı | 4553 otomatik denetim |
| a79bce4, 04f4be8 | PMY1 ile bağımsız ELF üretimi (fizibilite); OOB denetimi | 890 program |
| 5a6387e, 6012ea0 | PMB1 doğrulayıcı + analiz tabloda; `pm listedogrula` referansı | 4882 denetim |
| 0e1a218 | gen2: çok fonksiyonlu ELF üretici (call/ret, alloc, getc, STACK/DEPTH/ALLOC), sabit nokta | pm ile karşılaştırma |
| a7b6af2 | Assembler tabloda (`oz/asm/asm.pmb`) | 2069 denetim, bayt-bayt |
| b16becd | ELF üretici v2 (yakıt), `yenile.sh` ile pm'siz kendini yeniden üretme, A→B→A | tur testi |
| 4519209 | `yap.pmb`, doldur/opstab/fmtc tabloda; pm'siz uçtan uca PMY1→ELF | zincir testi |
| 3d411a7, 29ece4d, 8c363ef, 930406a, 2bdd1d2, 951ae9c | ELF'te `present/poll/frame` (çalışma zamanı kütüphanesi `rt.pma`), grafik testleri, FUEL eşdeğerliği | `dogrula_grafik.sh` |
| 8c476e1, f9ce8ae | Ondalık: SSE2 şablonları, `putf` (doğru yuvarlama), assembler'da ondalık sabit | `dogrula_float/ondalik`, sabotaj |

## Aşama 2 — komut bilgisi sayıya taşındı (2026-10-29)
| Commit | Ne | Kanıt |
|---|---|---|
| 89aa1f7, 4bb1fa5, d884fe0 | PMK1: komut/host tanımlarının tek kaynağı `komutlar.pmk`; `coz/yaz` tablodan | `dogrula_komut.sh` |
| 3ae7504, a8b691f, 1ba1e1d | Belge 3h/3i; son koşu sayıları | elf2 2787/0, yenile 53/0 |
| 5687c04 | PRIM + SEM bölümleri, `komut` aracı desteği, **svm** sayısal yürütücü | `dogrula_svm.sh` |
| 410195a, 7320e60 | SEM'den yerel üretim (gen2 mikro-işlem şablonları, `-6 1`); FMIN yalnız OP+SEM ile uçtan uca (oracle C++) | `dogrula_mikro/sabotaj_mikro` |
| 8ef00db, 7156766, 2ce1cc2 | Bellek/alloc/genişletilmiş tablo mikro testleri, svm sınır testi | |
| bf52527, 9f0b4fd, 468aa44, 95a4d42 | PSEUDO (6) + REWRITE (7): takma komutlar ve yeniden yazma kuralları sayıda; testler ve sabotaj | `dogrula_kural.sh` 352 |
| 8ab1e60 | Doğrulayıcı kuralları SEM'den türetilir (OPFLOW/OPTGT/…), XORI + kural + GOTO genişletme testi | `dogrula_komut` bölüm 9 |
| ffd223b, 9f3490b, 26f81b4, 7698be5 | x86 şablonları sayı: TOKEN (8) + TEMPLATE (9), `sablonc` kaldırıldı, br/brz/brnz + geri atlama yakıt varyantları | `dogrula_mikro` |
| a2418b2, 3200b37, 9207764, 5d4851b | **PMO1**: araçlar sayısal nesne, bağlayıcı; asm/valid/gen2 metinsiz üretilir; yalnız sayılarla genişletme testi | `dogrula_nesne` 37/0, `yenile` 59/0 |
| d69054f | Rastgele test üreteçleri tekrarlanabilir (kendi LCG) | |

## Aşama 3 — hızlı test ve sayısal düzenleme (2026-10-29)
| Commit | Ne | Kanıt |
|---|---|---|
| f7f3aef | **Hızlı test sistemi** `oz/hizli.sh` (değişen dosyadan paketler, paralel, `--tam`, kritik denetimler) | `oz/dogrula_hizli.sh` 51; bulgu: `dogrula_ondalik` bozuk pm'de boş geçiyordu, düzeltildi |
| b481909 | **PMO1 görünüm/düzenleme**: `gorunum.pma` (gor/yaz/adlar, ELF'e çevrilir), `duzenle.sh`, bağımsız Python referansı; uçtan uca düzenleme (valid kuralı 99, gen2 varsayılanı, asm yorum karakteri, yeni tablo komutu); zincire eklendi | gor→yaz kimlik; 782 rastgele + 150 bozulmuş görünümde fark 0; sabotaj 60/60; tam koşu 21 paket 0 hata (953 sn) |

## Aşama 4 — C++ (Linux) yetenek eşdeğerliği (yol haritası: `CPP_YOL_HARITASI.md`)
| Commit | Madde | Ne eklendi | Kanıt |
|---|---|---|---|
| 7e737bb, fcd5233 | — | Yol haritası; Windows sonra, macOS kapsam dışı; kaynak dili yok | |
| 66066ab | ✅ `ret`, ✅ `halt` mikro şablonu | `tpl prim ret/halt` | `dogrula_cagri.sh` (STOP/RETX yalnız SEM ile): ELF = svm = C++ pm; şablon silme + 3 bozuk şablon yakalandı |
| 1ad2e19 | ✅ `call` mikro şablonu | `tpl prim call` + 16 işlev simgesi `FREL/FNR8/FNP8/FCNT_Fk`, gen2 `etok_fn` | `dogrula_cagri.sh` 64 (CALLX: fib, yakıt 1..5000, DEPTH/STACK, yığın eşiği ve +1); 6 bozuk şablon yakalandı |
| ba126b7 | ✅ `host` mikro şablonu | `tpl prim host` + `HOSTSP_Fk`, gen2 `hostmap/hostslot` (scanrt SEM'den) | `dogrula_cagri.sh` 91; `sabotaj_cagri.sh` 22/22 |

## Plan: 1.399 madde / 41 bölüm (2026-10-31)
Mimari düzeltme: ana ürün sayısal motor, dış diller uyumluluk katmanı (`PROJE_HARITASI.md` §1); plan `PLAN_1399_MADDE.md`. Bölümler sırayla işlenir.

## Bölüm 01 — temel sayı ve değer türleri (31 Ekim – 1 Kasım 2026; durum: aşağıdaki "Son durum" satırı)
| Konu | Ne eklendi | Kanıt |
|---|---|---|
| Tamsayı aileleri (01.013–019, 021, 022, 027) | PRIM 48–58 + 51 OP (sext/zext, add/sub/mul/div/mod/neg/shl/not _i8.._u32, divu modu ltu leu, u2f f2u, takma gtu geu); yalnızca OP+SEM, x86 şablonu yok | `oz/elf2/dogrula_tur.sh` (ELF = svm = C++ pm = bağımsız C++ oracle), `sabotaj_tur.sh` |
| TYPES bölümü (PMK1 bölüm 10) | 29 tür: sınıf, bit, işaret, boyut, hiza, bellek sözcüğü, bayt sırası, limitler (analitik), **52** C/C++ tür adı için LP64 ABI eşlemesi (`_Float32/64/128/32x/64x`, `std::float*_t` dahil), dönüşüm matrisi; `komut @@tur` | `oz/komut/dogrula_turler.sh` (derleyici gerçekleriyle sizeof/alignof/numeric_limits + **dönüşüm kapsamı: her sayısal sıralı çift için yol**), `dogrula_komut.sh`, `sabotaj_komut.sh` |
| Program düzeyi tür sistemi (01.020, 01.030) | PMT1 tür notu, OPSIG/HOSTSIG, akış duyarlı denetleyici `oz/tip/tipdogr.pma` (sayısal PMA aracı; ELF'i de üretilir); geniş değer = baş tür + DEVAM sözcükleri; yanlış tür/geçersiz dönüşüm/yanlış genişlik kesin hata koduyla reddedilir | `oz/tip/dogrula_tip.sh` (131 durum, pm = ELF), `sabotaj_tip.sh` |
| Genel çoklu yazmaç operand (01.026) | KIND `q Q o O` (2/4 sözcük), `rdw/wrw/mulhu`, `lpb/lpe` döngü, `spld/spst` kazıma alanı (256 sözcük), OPW/MARK/RELM/SCR simgeleri; valid/asm/gen2/svm/C++ genişlik denetimi | `oz/tur128/dogrula_i128.sh` (ELF = svm = pm = `__int128` oracle), `sabotaj_i128.sh` |
| 128 bit tamsayı (01.026) | i128/u128: aritmetik, mantık, kaydırma, karşılaştırma, bölme/kalan (128 yinelemeli), bswap128, dönüşümler (tamsayı, **f16/bf16/f32/f64 tek yuvarlama**, f80/f128) | `dogrula_i128.sh`, `dogrula_kayan.sh` ("i128/u128 <-> dar kayan") |
| long double / float128 (01.024, 01.025) | f80 ve f128 yazılım kayan nokta: add sub mul div sqrt neg abs eq ne lt le tobool, ham erişim, 36 dönüşüm komutu/biçim; yalnızca OP+SEM (mikro-programlar `oz/kayan/{sf,ops}.py` üreteçlerinden); anlam `oz/kayan/KAYAN_NOKTA.md` | `oz/kayan/dogrula_kayan.sh` (ELF = svm = C++ pm `long double`/`__float128` = bağımsız oracle), üreteç aşamasında `t_*.py` tam rasyonel referans, `sabotaj_kayan.sh` |
| Karmaşık sayı (01.029) | c64/c128/c160: add sub mul div neg conj mov eq ne tobool mk real imag, 6 karmaşık↔karmaşık, gerçek→karmaşık dönüşümü (CX_LIMITED_RANGE) | `dogrula_kayan.sh` (oracle: derleyicinin `_Complex` türleri, `-fcx-limited-range`), `dogrula_tip.sh` |
| Komut kodu ≥ 255 | PMB1 kaçış kodlaması `0xFF`+LEB128; tablo 343 komut; C++ `Op/Ins` 16 bit; `ikili.cpp`, `asm.pma.in`, `coz/yaz.sayi.in` | `dogrula_kayan.sh` (sayısal assembler = C++ assembler bayt-bayt, `liste` gidiş-dönüş), `dogrula_ikili.sh`, `dogrula_asm.sh` |
| Sınırlar | svm geçici dizisi 32, mikro-işlem belleği 80 000, komut başına en çok 8191 mikro-işlem, assembler ad tabloları 4096 | `dogrula_komut.sh` |
| Altyapı | `oz/komut/yenile_tablo.sh`; üreteç betikleri `gecis/tur*_ekle.py` (`--guncelle`: SEM yeniden üret); v1 üretici için dondurulmuş 55 komutluk tablo | `oz/hizli.sh --tam --bag --sabotaj` |
**Son durum (2 Kasım 2026):** 01.001–01.033 tamamlandı (Linux x86-64; **Windows ABI yapılmadı, kapsam dışı**). Son kod sürümünde SOĞUK (önbelleksiz, `HIZLI_YENIDEN=1`) tam koşu `oz/hizli.sh --tam --bag --sabotaj`: **47 paket, 0 başarısız**, duvar 45 dk 29 sn, toplam CPU 9144 sn (4 çekirdek). Normal paketlerde 25 066 kontrol geçti, 0 kaldı. Mutasyon: 645 mutant, **610 yakalandı, 35 kaçtı — kaçanların tümü (`sabotaj_kayan`) `oz/kayan/sabotaj_kayan_denk.txt`'te gerekçeli denk mutant, beklenmeyen kaçan 0**; `sabotaj_komut` 72/72 yakalandı. Atlanan testler: svm 0; pmy2elf'te 44 (pm'in varsayılan yakıtını tüketen ya da çok uzun dönen rastgele modeller; gerekçe `ATLANAN` günlüğünde). Geriye uyumluluk (`dogrula_uyum.sh`) 189/189.
**Test hızlandırma (2 Kasım):** önceki ölçülen tam koşu 13 402 sn (3 s 43 dk; kullanıcı gözlemi 55–60 dk) → **45 dk 29 sn**. Yapılanlar: sabotajda ilk-hatada-dur (`oz/ilkhata.sh`), sabotaj mutantlarının hepsi paralel ve iki aşamalı (az örnekli/ilgili grup ilk aşama, kaçan mutant TAM kosuyla yeniden sinanır), tüm paketler ve mutantlar tek global FIFO yuva havuzunda (`oz/slot.sh`, bariyer yok), pmy2elf 4 parçada paralel (452→303 sn duvar), şeffaf g++ derleme önbelleği (`oz/onbellek_bin/g++`, içerik anahtarlı), paket sonuç önbelleği (anahtar = tüm depo ağacının içerik özeti; yalnız geçenler saklanır; değişmemiş ağaçta tam koşu 4 sn, `HIZLI_YENIDEN=1` ile yok sayılır). **İki kademe (ölçüldü):** `oz/hizli.sh --tam --bag` (tüm normal paketler, 30 paket, 0 başarısız) soğuk 12 dk 59 sn (CPU 2240 sn); mutasyon kademesi (`--sabotaj`, +17 paket, 645 mutant) ek ~32 dk. **Tek komutla 20 dk hedefi TUTMADI:** koşu CPU-bağımlı (9144 CPU-sn / 4 çekirdek = 38 dk alt sınır). En büyük CPU: sabotaj_kayan 2038, sabotaj_komut 1484, sabotaj_gorunum 831, sabotaj_donusum 651, pmy2elf 452, sabotaj_mikro 405, svm 249. 20 dk için ≤ 4800 CPU-sn gerekir; bunu test/mutant silmeden sağlamak ancak daha çok çekirdek ya da komut aracının (her çağrıda 510 KB tablo ayrıştırılıyor, ~0,2 sn) hızlandırılmasıyla mümkün.

## Bölüm 02 — ifadeler, operatörler ve sabitler (2 Kasım 2026; DURUM: TAMAMLANDI 34/34 — sayısal derleyici ifx.pmb; ilk sürümün geri alınan iddiası "Bölüm 02 düzeltme ve kapanış" bölümünde düzeltildi)
Mimari [Kesin]: yeni zorunlu metin dili yok. `pm ifade kaynak.cx` bir **C++ alt kümesi uyumluluk ön yüzüdür** (dış dil katmanı; motorun ürünü değil, `src/ifade.cpp`, C++ referans/geliştirme aracı): metin → lexer → operatör-tablosu güdümlü öncelik ayrıştırıcısı → AST → **PMX1** (kanonik SAYISAL AST: sayı listesi; `--pmx1` yazar, `--pmx1-oku` okur; derleme HER ZAMAN sayı listesinden çalışır, yapısal doğrulama + döngüsüzlük + derinlik sınırı + fuzz) → anlam/üretim → PMA → PMB1 → mevcut yürütücüler (C++ `pm`, native ELF/gen2, svm). Operatör öncelik/birleşim tablosu **PMK1 bölüm 11**'dedir (`komutlar.pmk`, isteğe bağlı bölüm; `oz/komut/gecis/ifade_ops_ekle.py` + `yenile_ifade.sh`), C++ tarafı `src/ifade_ops_gen.h`'i bu tablodan okur; tablo ↔ derlenmiş tablo (`pm ifade --ops`) ↔ üretilmiş başlık tutarlılığı testle sınanır.
| Konu | Ne eklendi | Kanıt |
|---|---|---|
| Sabitler 02.026–030 | karakter/kaçış (`\n \t \\ \' \" \? \a\b\f\r\v \ooo \xhh`), ham dize `R"d(...)d"`, bitişik dize birleştirme, `0b 0 0x`, `'` ayırıcı, `u l ul ll ull f l` sonekleri ve [lex.icon] tür seçimi, ondalık/onaltılık ondalık, UDL sonekleri | `testler/sabitler.cx`, `red/lex_*.cx` |
| İfadeler 02.004–011 | tablo güdümlü öncelik ayrıştırıcısı, parantez, `&& ||` kısa devre (sabit katlama dahil), `?:` (tür birleştirme, lvalue/sınıf kolları), `=` ve 10 bileşik atama (C++17 sağ-önce), `++ --` ön/son, virgül, tamsayı terfisi + olağan aritmetik dönüşümler, sabit katlama | `testler/{oncelik,kisa_devre,kosul,atama,artir,virgul}.cx`, rastgele ifade üreteci (150 test/koşu, UBSan elemeli) |
| Erişim/boyut 02.012–016 | `.` `->` (taban sınıf, iç içe, dizi üye), `[]` (dizi/işaretçi/`i[a]`, sabit sınır reddi, değişken indekste çalışma zamanı denetimi), `sizeof/alignof` Itanium ABI dizilimiyle (vptr, taban, EBO, kuyruk dolgusu; dizi sınırları sabit ifade) | `testler/{uye,dizi,sizeof}.cx` |
| noexcept/typeid 02.017–018 | `noexcept(ifade)` (işlev bildirimi, `noexcept(bool)`, çağrı/dynamic_cast<T&>/typeid analizi, değerlendirilmeyen işlenen), `typeid(tür/ifade)` (statik/dinamik, `==`, `!=`, `.name()` Itanium adları) | `testler/{noexcept,rtti,typeid_ad}.cx` |
| Dönüşümler 02.019–024 | `static_cast` (sayısal, işaretçi yukarı/aşağı, `T&&`=std::move), `dynamic_cast` (işaretçi/başvuru/`void*`, tür kimliği + `isa` işlevleri, sanal gönderim `vd*`), `const_cast`, `reinterpret_cast` (işaretçi↔tamsayı, aynı boyutlu başvuru), örtük dönüşüm sıralaması (tam/terfi/dönüşüm/kullanıcı) ve aşırı yükleme çözümü, kullanıcı dönüşüm işleçleri (`explicit`, bağlamsal bool, belirsizlik) | `testler/{donusum_acik,donusum_ortuk,kullanici_donusum,rtti}.cx` |
| UDL 02.025 | `operator""_x` (`unsigned long long`, `long double`, `char`, `(const char*, size_t)`) | `testler/udl.cx`, `red/udl_*.cx` |
| `<=>` 02.031–032 | yerleşik `<=>` (strong/partial ordering), kullanıcı `<=>`, `< > <= >=` yeniden yazımı (ters aday dahil), `== !=`, `= default` `<=>` ve `==` (taban, iç içe, üye kategorilerinin ortak türü) | `testler/{uzay,varsayilan_karsilastirma}.cx` |
| Kategoriler/sıra 02.033–034 | lvalue/xvalue/prvalue (aşırı yükleme `T&`, `const T&`, `T&&`, `const T&&` ile gözlenir; ~70 ifade türü), geçici nesne gerçekleştirme, C++17 değerlendirme sırası, **sıralanmamış değiştirme tanılaması** (`E_UNSEQ`) | `testler/{kategori,sira}.cx`, `red/unseq_*.cx` |
| Bellek/işaretçi güvenliği | null işaretçi dolaylı erişimi `TRAP: OOB` (motorda adres 0 geçerli sözcük olduğundan derleyici denetler), sabit sınır dışı indeks derleme hatası, değişken indekste çalışma zamanı tuzağı, `dynamic_cast<T&>` başarısızlığı tuzak, sıfıra bölme `DIV0`, yığın taşması denetimi | `trap/*.cx` (pm = ELF = svm tuzak adı + stdout) |
| Reddedilenler | 102 olumsuz test (kesin hata kodu; çoğu g++ ile de reddedilir) | `red/*.cx` |
| Doğrulama | `oz/ifade/dogrula_ifade.sh` (hızlı 20, tam 150 rastgele); `oz/hizli.sh` paketi `ifade`; sabotaj `oz/ifade/sabotaj_ifade.sh` | bkz. Son durum ve `BOLUM_02_KAPANIS_RAPORU.md` |
**Sınırlar (dürüst):** şablon, ad alanı (yalnız `std::` yerleşik adları), lambda, `new/delete`, kurucu/yok edici çağrısı (yalnız boş yok edici sözdizimi), istisna (C++ referans ön yüzde yok; sayısal derleyicide gerçek: bkz. düzeltme ve kapanış), çoklu/sanal kalıtım, statik üye, `enum`, işlev işaretçisi, `int (*p)[3]` bildirici (takma ad kullanılır), `long double` f64 ile temsil (UDL parametresi dahil), erişim denetimi (`private`) yok, `wchar_t/char8_t/char16_t` ve dize ön ekleri yok. Motor sözcük adresli olduğundan `reinterpret_cast` bayt görünümü vermez (eşit boyutlu skalerler / işaretçi↔`unsigned long` = sözcük adresi×8); işlev argümanları SOLDAN SAĞA değerlendirilir (g++ sağdan sola; standart belirsiz bırakır, testler buna bağlı değildir). Üretilen programlar genel 64 bit komutlar + açık normalleştirme (`sext/zext/rnd_f32`) kullanır; **PMT1 tür notu üretilip `tipdogr` ile doğrulanmaz** (kayıtlar ifade başına yeniden kullanılır; kapsam dışı kaldı).
**[GERİ ALINDI — bkz. "Bölüm 02 düzeltme"]** İlk sürüm iddiası: 02.001–02.034 tamamlandı. C++ referans ön yüzü için `ifade` paketi 924 kontrol, 0 başarısız (57 sn). Tam normal koşu `HIZLI_YENIDEN=1 oz/hizli.sh --tam --bag`: **31 paket, 0 başarısız, duvar 762 sn (12 dk 42 sn), CPU 2374 sn** (≤ 20 dk). Sabotaj: 58 mutant, 56 yakalandı, 2 gerekçeli denk, 0 yakalanmadı (239 sn). Bölüm 01 sabotaj kümeleri bu bölümde yeniden koşulmadı (kaynakları değişmedi). Ayrıntı: `BOLUM_02_KAPANIS_RAPORU.md`.
### Bölüm 02 düzeltme ve kapanış (2 Kasım 2026) — kurallar sayısal parametre, derleyici sayısal program; 34/34
İlk sürümde ifade derleme/anlam kararlarının TAMAMI `src/ifade.cpp` (C++) içindeydi; PMX1 yalnız sayı listesiydi. "34/34" iddiası geri alındı, mimari düzeltildi ve bölüm bu kez **sayısal derleyiciyle** kapatıldı. Ayrıntı ve dürüst sınırlar: `BOLUM_02_KAPANIS_RAPORU.md`.
| Parça | Ne | Kanıt |
|---|---|---|
| **ifx.pmb** (sayısal PMX1 derleyicisi) | PMX1 sayı listesini okuyup PMA + PMT1 tür notu üretir (`oz/ifade/num/ifx.pmb`; `ifx.cx` geliştirme görünümü, tazelik testli). Sınıf, başvuru, `auto`, aşırı yükleme, sanal gönderim, RTTI, `<=>`, UDL, değer kategorileri, f80, istisna, bayt görünümü dahil. pm'siz zincir: `ifx.elf` → `asm.elf` → `coz.elf` → `valid.elf` → `tipdogr.elf` → `gen2.elf` | `oz/ifade/num/dogrula_ifx.sh` |
| **Kural tablosu** | `komutlar.pmk` bölüm 12 (tür özellikleri, operatör→komut, karşılaştırma, dönüşüm adları, yazmaç türü→TYPES kimliği) + OPSIG/HOSTSIG imzaları çalışma anında okunur | tablo değişikliği testi (`+`→sub, `*`→add) |
| **PMT1/OPSIG tür duyarlı üretim** | her işlev için tür notu; sabit havuzu yazmaçları yerine `mov`, çağrı penceresi `dyn`, f80 çiftleri/`devam`; `tipdogr` her test programını onaylar, bozuk notlar reddedilir | `dogrula_ifx.sh` (tipdogr + 69 bozuk not) |
| **f80** | `long double` değer işlemleri Bölüm 01 f80 komutlarıyla; sessiz f64 indirgemesi yok | `testler/f80_*.cx` |
| **İstisna** | `throw/try/catch`, `catch(...)`, `throw;`, `std::exception/bad_cast/bad_typeid`; yakalanmayan/`noexcept`/`main` kaçışı = TRAP; `dynamic_cast<T&>`/`typeid(*null)` gerçek istisna | `testler/istisna_*.cx`, `num/trap/exc_*`, `num/red/catch_*` |
| **Bayt görünümü** | `reinterpret_cast<(unsigned/signed) char*>(&tamsayı/double)` bayt okuma/yazma/aritmetik, geri dönüş; dar tamsayı normalizasyonu; sızıntı = derleme hatası | `testler/bayt_gorunum.cx`, `bellek_gorunum.cx`, `num/red/bayt_*` |
| **f128** | `__float128` (`1.5Q`, `pq`): 2 sözcüklü değer, f80 ile aynı yazmaç-çifti yolu; adlar/tür/eşleme tablodan (bölüm 12 f128 sütunları); f80↔f128 açık dönüşüm; `{}` daraltma sırası float<double<f80<f128 | `testler/f128_*.cx`, `num/red/f128_*` |
| **asm.elf sınırı** | sayısal assembler girdi sınırı 1 MiB → 4 MiB (tablolar birlikte büyütüldü; `oz/asm/asm.pma.in` → `metinden.sh` → `asm.pmo/asm.pmb`) | öz-barındırma testi 1,7 MB PMA'yı pm'siz işler |
| **Öz-barındırma** | ifx.cx kendini derler; PMA1 = PMA2 (sabit nokta), `ifx2.elf` native | `dogrula_ifx.sh` (4. adım) |
| **Bozuk PMX1** | ifx.elf çökmez/takılmaz; kabul ettiği çıktı asm→coz→valid→tipdogr'dan geçer | `pmx_fuzz_ifx.py` |
| **Sabotaj** | `num/sabotaj_ifx.sh`: tablo ve kaynak mutantları (istisna, bayt görünümü, tür notu, öz-barındırma dahil) | bkz. kapanış raporu |
**Durum:** 34/34 madde 🟢. Açık sınırlar (kapanış raporu §5; motor yeteneği bağlantı denetimi): i128/u128, f16/bf16, karmaşık, bswap motorda var ama ifade derleyicisine bağlı DEĞİL; bayt görünümü kapsamı (tamsayı/double↔1 bayt), null/terminate tuzağı derleyici üretimli (`TRAP OOB`), referans C++ ön yüz istisna/bayt görünümü üretmez. İşlev argüman sırası: C++ standardı belirsiz bırakır; soldan sağa seçilmiştir, garanti değildir.

## Bölüm 03 — denetim akışı (3 Kasım 2026; DURUM: TAMAMLANDI 19/19, 2. tur düzeltmeyle eksiksiz — ayrıntı: `BOLUM_03_KAPANIS_RAPORU.md`)
Mimari [Kesin]: kural ve işlem anlamı sayısal parametre; kaynak düzeyi akış ifx.pmb'de (PMX1 düğümleri), motor düzeyi yeni komut yalnız tabloda (JTAB), iyileştirici sayısal program (cfgopt.pmb, ifx ile derli).
| Konu | Ne eklendi | Kanıt |
|---|---|---|
| switch/case/default 03.008 | PMX1 `N_SWITCH/N_CASE/N_DEFAULT`; seyrek → ikili arama, yoğun → `jtab`; aralık (`case a ... b`), düşme, ortada default, işaretsiz/64 bit, iç içe, ilklendirici, Duff cihazı; yinelenen/çakışan/sığmayan/ters case reddi | `num/testler/ca_switch.cx`, `num/red/sw_*` |
| Atlama tablosu 03.009 | `JTAB r,n` + `BRX` mikro-işlemi YALNIZ tabloda (OP+SEM+PRIM+x86 şablonu+OPSIG; `oz/komut/gecis/denetim_ekle.py`); C++ `anlam.h` makroları, `svm.pma` kolu, ELF şablonu (sınırlı dizin + 5 bayt ileri `jmp` merdiveni) | `cfg/akis/tablo.pma` (pm = ELF = svm), `ca_switch` yoğun switch'ler |
| range-for / do-while 03.010–011 | `N_RANGEFOR`: dizi, çok boyutlu dizi, üye/serbest `begin/end` yineleyici sınıfı, korutin; sentetik PMX1 düğümleriyle açılır | `ca_rangefor.cx`, `ca_dongu.cx` |
| break/continue 03.012 + liveness/scope 03.018 | tek denetim yığını; `goto`/etiket; erişilebilirlik (`E_FLOW` değer döndüren işlevin sonu), `E_JUMPINIT`, `E_GOTO`, `E_LABEL`, `E_CASE` | 30 olumsuz test |
| desen eşleştirme 03.013 | `__match` (sabit/aralık/joker/bağlama/seçenek/koruma/yapı) | `ca_match.cx` (+`.out`) |
| korutin 03.014 | `std::generator<T>`, `co_yield/co_await/co_return`, yığın çerçevesi, `corun` dağıtıcı | `ca_korutin.cx` (+`.out`) |
| istisna akışı / if constexpr / kısa devre 03.015–017 | try içinden break/continue/return/goto; `if constexpr`; `cj` doğrudan koşul atlaması | `ca_istisna_akis.cx`, `ca_constexpr.cx`, `ca_kosul.cx` |
| CFG iyileştirici 03.019 | `oz/cfg/cfgopt.pmb`: atlama zinciri, sabit dal katlama, dal tersleme, ulaşılamaz/ölü kod; tablodan çalışma anında komut özellikleri | `oz/cfg/dogrula_cfg.sh`, `sabotaj_cfg.sh` |


**2. tur — eksiksiz kapanış düzeltmesi (3 Kasım 2026):**
| Konu | Ne yapıldı | Kanıt |
|---|---|---|
| Native JTAB | `src/native_x64.cpp` `MK_BRX`: işaretsiz sıkıştırma + 32 bit göreli adres tablosu, yakıt harcamaz; `--native-strict` JTAB'ı doğrudan çalıştırır | `tests/jtab_*.pma` (yorumlayıcı = native-strict, 3 promote kipi, çıktı + çıkış + tuzak + kalan yakıt); yakıt süpürmesi 1..130 pm = native = ELF; `oz/cfg/sabotaj_native.sh` 6 mutant |
| jtab akış/merdiven kuralı | `OPFLOW=4` (komut.pmb türetir), `valid.pmb` + C++ `validate_program/analyze_program`: n≥0, varsayılan kod içinde, girdiler ileri `jmp`; akış kenarları ip+1…ip+1+n (başlatılmamış okuma artık yakalanır) | `dogrula_valid.sh` 11 elle jtab listesi; `sabotaj_valid.sh` 5 mutant; `oz/cfg/red/jtab_*.pma`; `dogrula_asm.sh`: anlamsal ret valid.pmb'ye bırakılır (10 program) |
| gen2 `isbackop` | OPFLOW 4 geri atlama sayılmıyor (`jtab r,1` DIV0 reddini giderdi) | `jtab_*` ELF karşılaştırmaları |
| cfgopt | geçersiz akış güvenle reddedilir (DIV0), yakıt tekdüzeliği + meta-test, native-strict karşılaştırması | `dogrula_cfg.sh` 598 kontrol, `sabotaj_cfg.py` 25 mutant |
| Korutin + akış | goto/__match/kısa devre/switch/try-catch/iç içe/örnek yalıtımı/resume | `testler/ca_korutin2.cx` (+`.out`) |
