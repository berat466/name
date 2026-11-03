# Bölüm 04 — Kapanış Raporu: Fonksiyonlar ve Çağrı Modeli

Dal: `claude/formul-avcisi` · Linux x86-64 · 3 Kasım 2026 · başlangıç commit'i 3e71083

## 1. Sonuç
**04.001–04.033: 33/33 madde gerçek sayısal motorda uygulandı ve doğrulandı.** PLAN: **173 🟢 / 38 🔵 / 1188 ❌** (toplam 1399). Bölüm 05'e geçilmedi.
Sayım tablodan alındı: Bölüm 04 öncesi 146 🟢 / 41 🔵 / 1212 ❌; Bölüm 04'ün 6 🟢'si (001–006) zaten sayılıydı, 3 madde 🔵→🟢 (007–009), 24 madde ❌→🟢 (010–033) → 🟢 146+3+24 = 173; 🔵 41−3 = 38; ❌ 1212−24 = 1188.

## 2. Mimari (kanonik yapı sayısal parametre) [Kesin]
- **Motor düzeyi — iki yeni komut, yalnız tablo kaydıyla:** `CALLI` (dolaylı çağrı, OP 344) ve `TCALL` (kuyruk çağrısı, OP 345). Her ikisi de **yalnız** PMK1'e eklendi (`oz/komut/gecis/cagri_ekle.py`: OP + SEM + PRIM 67/68 + x86 şablonları `tpl op CALLI/TCALL`, `tpl prim calli/tcall`, `tpl sp calli_stub` + OPSIG + doğrulayıcı özellikleri). `komut.pmb`, SEM'deki `call/calli/tcall` ilkellerinden `OPCALL/OPCS0/OPCS1/OPCS2/OPFLOW` türetir; `valid.pmb`, `tipdogr.pmb`, `asm.pmb`, `gen2.pmb`, `svm.pmb`, C++ `pm` ve yerel JIT (`src/native_x64.cpp`) bu özellikleri okur. Ayrıntı: `oz/komut/KOMUT_TABLOSU.md` (Bölüm 04 bölümü).
- **Kaynak düzeyi (ifx.pmb, PMX1):** yeni düğümler `N_TFN/N_LAMBDA/N_NAMEARG/N_VASTART/N_VAARG/N_VAEND/N_TYID/N_STATASSERT/N_ALLOCA/N_ABICLASS` (sayısal AST, doğrulamalı). Anlam ifx'tedir (kural tablosu yine PMK1 bölüm 12).
- **İşlev değerleri = işlev dizini.** Her (imza, sret, this) bileşimi için bir prototip işlev `fpN` üretilir; `calli` imza/arity denetimini bununla yapar. 0. işlev `fpnul` (20 parametre) boş işaretçi çağrısını tuzağa çevirir.
- **Sabit değerlendirici (`cx`):** ifx içinde tür-duyarlı bir AST yorumlayıcısı (tamsayı/kayan nokta/dizi/döngü/özyineleme; işaretli taşma ve sıfıra bölme = sabit ifade değil). `constexpr`, `consteval`, `constinit`, `static_assert`, dizi boyutları ve `case` etiketleri hep bunu kullanır.
- **Satır içi açılım (inline):** `inline` + tek `return ifade;` gövdeli + skaler + özyinelemesiz işlevler çağrı noktasında açılır (parametreler yeni yuvalar, çağıran yerelleri `var_lo` engeliyle gizlenir); aksi halde normal çağrı.
- **Kuyruk çağrısı:** `return f(...)` konumundaki çağrılar `tcall`'a çevrilir (aynı dönüş türü, işaretçi/başvuru/sınıf argümanı yok, try/işlenen-istisna yok, alloca yok, noexcept uyumlu); aksi halde normal çağrı.
- **Korutinler:** `std::generator<T>` korutini korundu; ek olarak **`promise_type`'lı kullanıcı sınıfları**: iç içe sınıf (`Task::promise_type`) ayrıştırıcıda hoist edilir; `std::coroutine_handle<>` / `<P>` ayrıştırıcıda örneklenen gerçek sınıflardır (`resume/done/destroy/address/promise/from_promise/from_address`, `__co_*` iç işlevleri); `std::suspend_always/never` önsözde gerçek sınıflar. Çerçeve düzeni: [0] durum, [1] yield değeri, [2] işlev kimliği, [3] son-askıya-alma bayrağı, [8..] promise, ardından parametreler/yerel değişkenler. Gövde, ifx'te sentezlenen AST ile sarılır: `co_await initial_suspend(); try { gövde; return_void() } catch(...) { unhandled_exception() } __cofinal: __co_mark(); co_await final_suspend();`. `co_await` tam awaiter protokolüdür (`await_ready`, `await_suspend(tutamaç)` void/bool/**tutamaç (simetrik aktarım)**, `await_resume`); deyim/bildirim içinde değer olarak da kullanılır (askıdan dönüşte yazmaç geçerli olmadığı için sonuç gizli yerel değişkene hoist edilir).
- **ABI:** SysV x86-64 sınıflandırıcısı (`__abi_class(T)`: NO_CLASS/INTEGER/SSE/SSEUP/X87/X87UP/MEMORY, sekizli birleştirme kuralı, >16 bayt = MEMORY, polimorfik = Itanium "çağrı için önemsiz değil" → görünmez başvuru bayrağı +64); parametre sınırı 8 → **16** (yığınla geçirilen fazladan argümanlar).
- **Bağlama:** `extern "C"` (C bağlantısında aşırı yükleme yok), `static` (iç bağlantı), `extern` bildirim/tanım birleştirme, tür/bağlantı/noexcept tutarsızlığı derleme hatasıdır; tanımsız `extern` global kullanımı ve tanımsız işlev `E_UNDEF`.
- **Yığın nesnelerinin dönüşte temizlenmesi:** `alloca(n)` `__sp`'nin üstüne dinamik alan ayırır; işlev dönüşü (`emit_ret`: `__sp = fp`) onu kendiliğinden serbest bırakır — istisna yayılımı ve kuyruk çağrısı dahil.

## 3. Madde durumu ve kanıt
| Madde | Durum | Gerçek kanıt |
|---|---|---|
| 04.001 Fonksiyon tanımı | 🟢 | tüm `ca_*` testleri; `func fN` + PMT1 notu (`tipdogr`) |
| 04.002 Doğrudan çağrı | 🟢 | `call` komutu; `ca_ovl_defarg`, `ca_abi_args` (g++ oracle) |
| 04.003 Dönüş | 🟢 | `ret`; `ca_abi_args` (struct/double/long double dönüşleri) |
| 04.004 Özyineleme | 🟢 | `tests/calli_fib.pma`, `ca_tailcall.cx` (fact/fib/ikili özyineleme), `ca_inline.cx` (özyinelemeli inline) |
| 04.005 Çok fonksiyonlu bağımsız ELF | 🟢 | `dogrula_ifx.sh`: her testte ifx.elf/gen2.elf ELF = pm = svm = g++ ; oz-barındırma (ifx kendini derler, ELF, sabit nokta) |
| 04.006 Çağrı derinliği sınırı ve tuzak | 🟢 | `trap/fn_ozyineleme_derin.cx`, `trap/fn_alloca_tasma.cx`, `tests/calli_derin.pma` (DEPTH/STACK tuzakları, pm = ELF) |
| 04.007–009 SEM ret/call/halt x86 şablonu | 🟢 | `tpl prim ret/call/halt` (tablo); `oz/elf2/dogrula_cagri.sh` (yalnız SEM ile tanımlı yeni dönüş/çağrı/durma komutları ELF üretir) |
| 04.010 İşlev işaretçisi + CALLI | 🟢 | `CALLI` (tablo); `tests/calli_*.pma` (pm = native-strict = ELF = svm, OOB, imza uyumsuzluğu, yakıt süpürmesi), `ca_fnptr.cx`, `trap/fn_null_cagri.cx` |
| 04.011 İşlev işaretçisi tabloları | 🟢 | `tests/calli_tablo.pma`, `ca_fnptr2.cx` (işaretçi dizileri, tablo yönlendirme) |
| 04.012 Aşırı yükleme / 04.013 varsayılan argüman | 🟢 | `ca_ovl_defarg.cx` (dönüşüm sıralaması, belirsizlik, varsayılan argüman çözümü) |
| 04.014 Varargs | 🟢 | `ca_vararg.cx` (`va_list/va_start/va_arg/va_end`, `main`/ara işlev) |
| 04.015 İsimli parametreler | 🟢 | `ca_named.cx` (+`.out`: C++'ta yok, bağımsız beklenen çıktı) |
| 04.016 ABI'ye uygun parametre geçirme / sınıflandırma | 🟢 | `ca_abi_args.cx` (12–13 parametre, karışık tür, yapı değeri/dönüşü, referans; g++ oracle), `ca_abi_class.cx` (+`.out`, psABI 3.2.3 beklenen sınıflar), `lim` 16 |
| 04.017 Lambda + capture / 04.018 generic lambda / 04.019 mutable + closure | 🟢 | `ca_lambda.cx`, `ca_lambda2.cx` (değer/başvuru/init/this yakalama, iç içe, mutable, generic, işlev işaretçisine dönüşüm) |
| 04.020 Function object | 🟢 | `ca_functor.cx` (operator(), const/non-const aşırı yükleme, durum) |
| 04.021 `std::function` / 04.022 `std::move_only_function` | 🟢 | `ca_stdfunction.cx`, `ca_moveonly.cx`, `trap/fn_std_function_bos.cx` (boş çağrı tuzağı) |
| 04.023 Üye işlev işaretçisi | 🟢 | `ca_memptr.cx` (`.*`, `->*`, sanal) |
| 04.024 noexcept sözleşmesi | 🟢 | `ca_noexcept.cx` (operatör, işlev işaretçisi türü, lambda; g++ oracle), `trap/exc_noexcept.cx`, `trap/fn_noexcept_atar.cx` (ihlal = terminate/tuzak), `red/lk_noexcept_tutarsiz.cx` |
| 04.025 Inline semantiği | 🟢 | `ca_inline.cx` (+ `dogrula_ifx.sh` 3d: `inline` çağrı sayısını düşürür) |
| 04.026 constexpr / 04.027 consteval / 04.028 constinit | 🟢 | `ca_constexpr2.cx`, `ca_constexpr3_static.cx` (döngü/özyineleme/dizi/double/u64, dizi boyutu, `case`), 7 olumsuz test `red/cx_*.cx` |
| 04.029 extern "C" / 04.031 görünürlük ve bağlama | 🟢 | `ca_linkage.cx`, 8 olumsuz test `red/lk_*.cx`; `static` yerel (`ca_constexpr3_static.cx`) |
| 04.030 Genel kuyruk çağrısı | 🟢 | `ca_tailcall.cx` (sum(100000), karşılıklı özyineleme, double, collatz), `tests/tcall_*.pma` (yakıt, döngü) |
| 04.032 Değişken ömürlü yığın nesneleri | 🟢 | `ca_alloca.cx` (20000×alloca(100) dönüşte serbest, istisna yayılımı, özyineleme), `trap/fn_alloca_*.cx`, `red/alloca_tur.cx` |
| 04.033 Korutin promise/awaiter/resume/destroy | 🟢 | `ca_coro_promise1/2.cx`, `ca_coro_awaiter.cx` (g++ oracle: promise, yield_value, return_value, simetrik aktarım, bool await_suspend, istisna→unhandled_exception, iç içe görev), `ca_gen_destroy.cx`, `trap/coro_*.cx`, 5 olumsuz test `red/coro_*.cx` |

## 4. Sınırlar (dürüst) [Kesin]
- `__int128` / `std::generator` dışı türler ve iç içe `promise_type` dışındaki kullanıcı şablonları yoktur (alt kümede şablon yok; `std::function<R(A...)>` ve `std::coroutine_handle<P>` ayrıştırıcıda adla örneklenir). `std::function<int(int)> f(sq);` (doğrudan ilkleme) desteklenmez, `= sq` ve atama desteklenir.
- Promise'li korutinlerde parametreler tek sözcüklü skaler olmalıdır; çerçeve belleği motorun alloc-only modelinde serbest bırakılmaz (`destroy()` çerçeveyi geçersiz kılar, ikinci `destroy`/`resume` tuzaktır). `co_await` yalnız ifade deyimi, bildirim ilklendiricisi ve `co_yield/co_return` içinde (koşul/döngü başlığında `E_COROUTINE`). Sınıf yok edicileri bu alt kümede çalışmaz (yok edici gövdesi boş olmalı), bu yüzden "yığın nesnesi temizliği" çerçeve geri alımıdır (yok edici çağrısı değil).
- `alloca` bellek modeli sözcük adreslidir: `n` bayt için `n` sözcük ayrılır (tür başına bir sözcük); VLA (`int a[n]`) bilerek reddedilir (`red/const_expr.cx`: standart C++ değildir).
- Satır içi açılım yalnız tek-`return` skaler işlevlerde; kuyruk çağrısında sınıf/işaretçi/başvuru argümanı varsa normal çağrıya düşülür (çerçeve geri alınınca yerel adres sarkabilir).
- ABI sınıflandırıcısı bir **derleme zamanı sorgusudur** (`__abi_class`); iç çağrı kuralı pencereli yazmaçlardır (sınıf argümanlar kopya ile görünmez başvuru gibi geçer). Gerçek dış (C kütüphanesi) çağrısı yoktur; `extern "C"` bağlantı kuralları (aşırı yükleme yok, ad çakışması) uygulanır.
- Referans C++ ön yüz (`pm ifade`) bu bölümün yeni yapılarını üretmez; bağımsız referans olarak g++ oracle (ve elle hesaplı `.out`) kullanılır.

## 5. Çalışma sırasında bulunan gerçek hatalar
- **Kuru/gerçek geçiş tutarsızlığı:** lambda kapanışı gibi tembel üretilen durum, kuru geçişte yazmaç türlerini kaydırıyordu (gerçek geçişte `r5U` kullanılırken bildirim `r5I`); kuru geçiş yeni sınıf/işlev ürettiyse tekrarlanıyor. `note_win` yalnız gerçekten üretilen çağrıları sayıyor (`noexcept(f(x))` gibi değerlendirilmeyen işlenenler pencereyi büyütüyordu).
- Sabit değerlendirici ilk sürümde tamsayı ikili işlemleri `binop`'a bırakıyordu (o yalnız karşılaştırmayı katlar): hepsi yerel katlamaya taşındı; işaretli taşma/sıfıra bölme artık "sabit ifade değil".
- `valid.pmb` (524288 → 2097152 sayı) ve `gen2.pmb` (belirteç/komut tamponları) sınırları, ifx kendini derleyince 673 bin sayıya ulaştığı için büyütüldü (hem sayısal nesne hem metin kaynağı; `metinden.sh` ile bayt-bayt aynı); `tipdogr` durum dizisi 4 M → 12 M.
- Rastgele ifade üretecinde tanımsız davranış sızıntısı: g++ değişkensiz `(unsigned char)(-2.0)` ifadesini derleme zamanında katlayıp 0 veriyordu (UBSan görmüyor); üreteç artık değişkensiz kayan nokta→tamsayı dönüşümü üretmiyor (Bölüm 03 commit'inde de aynı davranış, yani bu bölümden önce var olan bir bulgu).

## 6. Testler [Kesin — aşağıdaki sayılar bu turda çalıştırılan koşulardan]
- **Son kodun tam koşusu** `HIZLI_YENIDEN=1 oz/hizli.sh --tam --bag`: **33 paket, 0 başarısız, duvar 1159 sn (19 dk 19 sn), toplam CPU 3340 sn** (hedef ≤ 20 dk). Başlıca paketler: `ifx` **990** kontrol (g++ oracle = ifx.elf = ifx.pmb → asm.elf → coz.elf → valid.elf → tipdogr → gen2.elf ELF = pm = svm; olumsuz + tuzak testleri; öz-barındırma sabit noktası; bozuk PMX1 fuzz), `ifade` 927, `asm` 2291, `valid` 8475, `tip` 141, `komut` 399, `cfg` 814, `uyum` **189** (eski PMB1 geriye uyumluluk), `bag` 1818, `pmy2elf` 372, `yenile` 77, `gorunum` 963, `nesne` 37, `elf2` 2952, `cagri` 91, `svm` 420, `testler` KATI EŞİT (350 program, native karşılaştırma 129).
- **Bölüm 04 için eklenen testler:** 24 yeni `ca_*` olumlu program (g++ oracle ya da elle hesaplı `.out`), 21 olumsuz derleme testi (`red/cx_*`, `lk_*`, `coro_*`, `alloca_tur`), 9 tuzak testi (`trap/fn_*`, `trap/coro_*`), `tests/calli_*`, `tests/tcall_*` + `tests/ek/` (70 000 dolaylı çağrı sayacı, tcall yığın sınırı, tcall yerel sıfırlama), valid elle listeleri (calli/tcall 12), tipdogr (calli/tcall 5), komut tablosu türetme (OPCALL/OPFLOW 3), dogrula_ifx 3d (inline açılım çağrı sayısı).
- **Mutasyon (test-testi):** ifx **139 mutant (19 tablo + 120 kaynak) → 139 yakalandı** (Bölüm 04 için 29 yeni: inline kapsamı/özyineleme, tcall koşulları, constexpr taşma/sıfıra bölme/static_assert/consteval/constinit, yerel static, bağlantı kuralları, alloca, awaiter protokolü, bitmiş resume/çift destroy tuzakları, promise konumu, ABI birleştirme/MEMORY eşiği/görünmez başvuru/X87, kuru geçiş tekrarı); native CALLI/TCALL JIT **10/10** (`oz/cfg/sabotaj_native_cagri.sh`: indeks sınırı, arity, stub boyu, pencere ileri/geri, derinlik sayacı, tcall yığın sınırı/argüman taşıma/yerel sıfırlama/unwind); doğrulayıcı calli **3/3**; tipdogr calli/tcall **2/2**; komut tablosu OPCALL/OPFLOW türetmesi **2/2**.
- **Native ELF / SVM / referans eşdeğerliği:** her `ca_*` testi için ELF = pm = svm = (g++ ya da elle hesaplı beklenen); motor düzeyinde `tests/calli_*`, `tcall_*` pm = `--native-strict` = ELF = svm (3 promote kipi, yakıt süpürmesi dahil).
- **ABI/yığın:** `ca_abi_args.cx` (13 parametreli çağrılar, karışık tür, yapı değeri/dönüşü; g++ oracle), `trap/fn_alloca_tasma`, `trap/fn_ozyineleme_derin`, `ca_alloca.cx` (20 000 dönüşte serbest bırakma).
- **Öz-barındırma:** ifx.cx kendini derler; PMA1 = PMA2 (187 bin satır), `ifx2.elf` (gen2) native; `valid.elf`/`tipdogr` kabul; sabit nokta `dogrula_ifx.sh` 4. adım.

## 7. Çalışma sırasında bulunan ve kapatılan altyapı bulguları [Kesin]
- **Rastgele ifade üreteci:** değişkensiz sabit kayan nokta → tamsayı dönüşümü (g++ derleme zamanında katlıyor, UBSan görmüyor) yaklaşık her 3 koşuda bir sahte fark üretiyordu (Bölüm 03 commit'inde de aynı); üreteç düzeltildi (30 koşu × 150 program = 4500 programda 0 fark).
- **`pm` bellek ayrılamazsa:** yakalanmayan `std::bad_alloc` ile `abort` (rc 134) ediyordu; artık temiz hata (`HATA: bellek ayrılamadı`, rc 1). `bag`/`pmy2elf` bu durumu "kaynak sınırı" olarak atlar (eskiden rc 134'ü atlıyordu).
- **`asm.pmo` tabloya bağlandı:** `OP_CALLI/OP_TCALL` komut kimliği sabitleri yerine `OPCALL/OPFLOW` dizilerinden (`isxcalli/isxtcall`); böylece motor-yenileme zinciri (donmuş v1 tablosu) yeniden bağlanır (`yenile` paketi).
- **cfgopt.pmb** ifx çıktısı değiştiği için yeniden üretildi (`oz/cfg/uret.sh`).
- **Bilinen sınırlama (önceden de vardı, Bölüm 03 araçlarıyla yeniden üretildi):** `host putf/present/poll/frame` çalışma-zamanı işlevleri ELF'te pencere tabanından başlayan çerçevede koşar; pencere canlı yazmaçların altında seçilirse (yalnız mutasyonla üretilmiş PMB1'de) ELF o yazmaçları ezer, `pm` ezmez. Derleyici (ifx/asm) pencereyi her zaman çerçevenin sonuna koyar, bu yüzden gerçek programlar etkilenmez. Bunun görünür olmaması için `bag` mutasyon külliyatı kararlı tutuldu: yeni motor testleri `tests/ek/` altına kondu (`run_tests.sh` çalıştırır, `bag` külliyatına girmez).
