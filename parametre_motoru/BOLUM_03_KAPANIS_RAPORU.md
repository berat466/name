# Bölüm 03 — Kapanış Raporu: Denetim Akışı

Dal: `claude/formul-avcisi` · Linux x86-64 · 3 Kasım 2026

## 1. Sonuç
**03.001–03.019: 19/19 madde gerçek sayısal motorda doğrulandı.** PLAN: 146 🟢 / 41 🔵 / 1212 ❌ (toplam 1399). Bölüm 04'e geçilmedi.
Sayım tablodan alındı: Bölüm 03 öncesi 133 🟢; Bölüm 03'ün 6 🟢'si zaten sayılıydı, 13 madde (007 🔵→🟢, 008–019 ❌→🟢) eklendi → 133 − 6 + 19 = 146; 🔵 42 → 41; ❌ 1224 → 1212.

## 2. Mimari (kanonik yapı sayısal parametre) [Kesin]
- **Motor düzeyi (yeni komut, kod yazmadan):** `JTAB r, n` (atlama tablosu) + `BRX` mikro-işlemi **yalnız tablo kaydıyla** tanımlandı (`oz/komut/gecis/denetim_ekle.py`: OP + SEM + PRIM + x86 şablonu + OPSIG). Üç yürütücü aynı anlamı çalıştırır: C++ `pm` (anlam makroları), `svm.pmb` (mikro-işlemi isimle okur), `gen2` ELF (şablon: sınırlı dizin → `lea/add/jmp rax` ile 5 baytlık ileri `jmp` merdiveni; SEM yoluyla da aynı çıktı). Doğrulayıcı (`valid.elf`) ve tipdogr (OPSIG `INT1`) komutu tablodan tanır. Merdiven kuralı: `jtab` ardından n adet **ileri** `jmp`; aralık dışı dizin merdivenin ardına düşer (= default).
- **Kaynak düzeyi (ifx.pmb, PMX1):** yeni düğümler `N_SWITCH/N_CASE/N_DEFAULT/N_GOTO/N_LABEL/N_RANGEFOR/N_MATCH/N_ARM/N_P*/N_COYIELD/N_COAWAIT/N_CORETURN` PMX1'e eklendi (sayısal AST, doğrulamalı). Anlam ifx'tedir; C++ ön yüz (`pm ifade`) yalnız isteğe bağlı metin→PMX1 aracıdır ve bu yapıları kendi referans anlamında `E_UNSUPPORTED` ile reddeder.
- **Koşul atlaması (kısa devre) `cj`:** `if/while/do/for` koşulları `&&`, `||`, `!` için doğrudan atlamayla üretilir (bool maddeleştirilmez), sabit operandlar katlanır ve değerlendirilmeyen operand yalnız tür denetimi için gezilir; yan etki sayıları g++ ile aynı.
- **CFG iyileştirici `oz/cfg/cfgopt.pmb`:** C alt kümesinde yazılıp **ifx ile derlenmiş sayısal program** (pm'siz zincir: `ifx.elf → asm.elf`). PMB1 listesi üzerinde, komut özellikleri (akış türü, hedef/anlık yuvası, saflık, `ldi/mov`) **PMK1 tablosundan çalışma anında** (KIND/OP/PRIM/SEM) türetilerek çalışır: atlama zinciri kısaltma, blok içi sabit yayılımıyla koşullu dal katlama, dal tersleme, sonraki komuta atlamayı silme, ulaşılamaz kod silme (CFG erişilebilirliği), blok içi ölü yazma silme; `jtab` merdivenleri korunur.
- **Korutinler (`std::generator<T>`):** yığın çerçevesi `alloc` ile yığına alınır (durum, değer, gövde kimliği, parametreler, yerel değişkenler); `co_yield/co_await/co_return` durum numaralı devam noktalarıdır; `next()/resume()/value()/done()` ve range-for; gövdeden kaçan istisna jeneratörü bitirir ve çağırana yayılır.
- **Desen eşleştirme (`__match`):** sabit, aralık, joker, bağlama, seçenek (`|`), koruma (`if`), yapı deseni (sınıf alanları sırayla); ifx bunları sentetik PMX1 düğümleriyle (== / && / bildirim) açar.
- **Zorunlu erişilebilirlik/kapsam kuralları (03.018):** değer döndüren işlevin sonuna kontrol ulaşabiliyorsa derleme hatası (`E_FLOW`; `main` ve korutin hariç; sabit koşullar, `while(true)`, `break`, `switch` default'u hesaba katılır); ilklendirmeyi atlayan `case`/`goto` (`E_JUMPINIT`), `try` içine / `catch` sınırından `goto` (`E_GOTO`), tanımsız/yinelenen etiket (`E_LABEL`), case/default switch dışında, yinelenen/çakışan/sığmayan/ters `case`, `continue` döngü dışında, `if constexpr` sabit değil.

## 3. Madde durumu ve kanıt
| Madde | Durum | Gerçek kanıt |
|---|---|---|
| 03.001 Koşullu dallanma | 🟢 | `cfg/akis/kosul.pma`, `sabit.pma`; `testler/ca_kosul.cx` (g++ oracle) |
| 03.002 Döngüler / 03.003 iç içe | 🟢 | `akis/dongu.pma`; `ca_dongu.cx`, `ca_rangefor.cx` |
| 03.004 Koşulsuz atlama (jmp) | 🟢 | tüm akış testleri; `goto` ile geri/ileri atlama (`ca_dongu.cx`) |
| 03.005 Fonksiyondan dönüş | 🟢 | `akis/donus.pma` (özyineleme), `ca_dongu.cx` (erken dönüş), korutin `co_return` |
| 03.006 PMY1 yapısal koşul/döngü | 🟢 | mevcut `yap` paketi (PMY1 → pma, 2366 kontrol; Bölüm 03 tam koşusunda yeniden geçti) |
| 03.007 GOTO | 🟢 | motor GOTO komutu yalnız SEM'le (`komut` paketi, bölüm 9: asm/valid/svm/ELF) + kaynak `goto`/etiket (ifx) |
| 03.008 switch/case/default | 🟢 | `testler/ca_switch.cx` (seyrek/yoğun/aralık/düşme/ortada default/negatif/işaretsiz/64 bit/iç içe/döngüde break-continue/ilklendirici/Duff cihazı) + 11 olumsuz test (`red/sw_*`) |
| 03.009 Atlama tablosu | 🟢 | `JTAB` komutu (tablo); `akis/tablo.pma` (pm = ELF = svm, aralık dışı/negatif dizin); `ca_switch.cx` yoğun switch'ler `jtab` üretir (runner denetler); tablo içinde geri `continue` |
| 03.010 range-for | 🟢 | `ca_rangefor.cx`: dizi, 2 boyutlu dizi, üye `begin/end` + yineleyici sınıfı, serbest `begin/end`, referansla değiştirme, `const&`, break/continue, geçici aralık, korutin aralığı |
| 03.011 do-while | 🟢 | `ca_dongu.cx` (en az bir kez, `continue` koşula gider, `break`), `akis/dongu.pma` |
| 03.012 break/continue soyutlaması | 🟢 | tek denetim yığını (döngü/switch/range-for/korutin/try-catch) — `ca_switch/ca_dongu/ca_rangefor/ca_istisna_akis`; işlenen-istisna yığını `break/continue/return` ile doğru geri yüklenir |
| 03.013 Desen eşleştirme | 🟢 | `ca_match.cx` (beklenen çıktı bağımsız Python hesabı) + 3 olumsuz test |
| 03.014 Korutin askıya alma/sürdürme | 🟢 | `ca_korutin.cx`: üreteçler, iç içe/özyineli üreteç, `co_await` askı, `co_return`, istisna, `double/char` elemanları, elle `next/resume/value/done` |
| 03.015 İstisna yayılımıyla akış | 🟢 | `ca_istisna_akis.cx` (try içinden break/continue/return/goto, catch içinde akış, switch içinde throw, yeniden fırlatma, derin yayılım) |
| 03.016 `if constexpr` | 🟢 | `ca_constexpr.cx` (seçilmeyen dal denetlenir ama üretilmez) + olumsuz test |
| 03.017 Genel kısa devre akışı | 🟢 | `cj` + `ca_kosul.cx` (yan etki sayıları, sabit operand, döngü koşulları, işaretçi/double/bool) |
| 03.018 Liveness/scope kuralları | 🟢 | 30 olumsuz test (`red/sw_*, goto_*, donus_*, etiket_*, break_*, continue_*, rangefor_*, korutin_*, constexpr_*`) |
| 03.019 CFG merkezli optimizer | 🟢 | `oz/cfg/dogrula_cfg.sh`: 53 olumlu test programında komut sayısı azalır, pm = iyileştirilmiş pm = iyileştirilmiş ELF = svm, valid + tipdogr kabul, idempotent; `asm.pmb/valid.pmb/gen2.pmb` iyileştirilmiş hâlleri aynı çıktıyı üretir |

## 4. Sınırlar (dürüst)
- `jtab` merdiveni yalnız **ileri** `jmp` içerir (5 bayt sabit); `pm run --native` JTAB'lı programı yorumlayıcıya düşürür (`--native-strict` reddeder); `gen2` ELF'i ve `svm` destekler. Bölüm 01'in Linux/native altyapısı yeniden yazılmadı.
- `__match` standart C++ değildir (P1371 benzeri alt küme, deyim biçimi); `std::generator<T>` elemanı yerleşik skaler tür, üye korutin/özel `promise_type`/`awaitable` yok; `co_yield` deyim düzeyinde, `co_await e;` değeri atılan yalın askıdır.
- `if constexpr` şablonsuz bağlamda (alt kümede şablon yok): seçilmeyen dal tam denetlenir.
- Değer döndüren işlevin sonuna ulaşılabilmesi C++'ta tanımsız davranıştır; burada **derleme hatası** yapıldı (zorunlu liveness kuralı). `goto` `catch` sınırını geçemez ve `try` içine girmez.
- İyileştirici komut sayısını değiştirir; bu yüzden yakıt-sayımına bağlı tuzak noktaları orijinal programla aynı olmak zorunda değildir (gözlemlenebilir çıktı/tuzak türü aynıdır).
- Referans C++ ön yüz (`pm ifade`) yeni yapıları üretmez; bağımsız referans olarak yalnız eski kapsamı doğrular.
- Önceki bölümlerden kalan tür bağlantıları (i128, f16/bf16, karmaşık, bswap) bu bölümün kapsamı dışında bırakıldı (Bölüm 02 raporu §5).

## 5. Testler
- **Tam normal koşu** `HIZLI_YENIDEN=1 oz/hizli.sh --tam --bag`: **33 paket, duvar 846 sn (14 dk 06 sn), toplam CPU 2743 sn** (hedef ≤ 20 dk; hiçbir test atlanmadı). 32 paket ilk koşuda geçti; `komut` paketi 1 başarısızlık verdi (C++ SEM dökümü aracı `anlam_dump.cpp` yeni `BRX` mikro-işlemini tanımıyordu — araç düzeltildi, `komut` 397/397 geçti). Paketler: `ifx` **723 kontrol**, `cfg` **487 kontrol**, `ifade` 927 (referans C++ ön yüz, değişmeden), `asm`, `bag`, `valid`, `yap`, `svm`, `uyum`, `kayan`, `i128`, `elf2` … → geriye uyumluluk (eski programlar), Bölüm 01 araç zinciri ve Bölüm 02 öz-barındırma sabit noktası korundu.
- `ifx` 723 kontrol: 53 olumlu program (g++ oracle ya da bağımsız Python beklenen çıktı) için ifx.elf = ifx.pmb PMA'sı, asm→coz→valid→tipdogr, ELF = pm = svm; 8 yeni `ca_*` test; 59 (+105 ortak) olumsuz test; 17 çalışma zamanı tuzağı; bozuk PMX1 fuzz; **pm'siz öz-barındırma sabit noktası** (ifx kendini yeni denetim akışı koduyla yeniden üretir); yoğun switch'in `jtab` ürettiği denetimi.
- **Mutasyon (test-testi):** ifx **110 mutant (19 tablo + 91 kaynak) → 110 yakalandı** (Bölüm 03 için 26 yeni: `cj` yönleri, switch aralık/çakışma/default/continue/tablo, goto/jump-over-init, erişilebilirlik, if constexpr, range-for, match, korutin); `cfg` **18/18**; C++ referans ön yüz 58 mutant → 56 yakalandı, 2 gerekçeli denk, 0 yakalanmayan.
- Çalışma sırasında bulunan gerçek hatalar: `!` koşulunda "hemen sonraki etiket" yanlış eşlenmişti (ifx'in kendini yeniden üretmesi sabit noktasında yakalandı); sabit `const int` değerleri `rval`'de bellekten okunuyordu (sabit koşullar katlanmıyordu); sabit karşılaştırma ve `&&/||` katlanmıyordu; mutasyon testinde sabit yayılımında "bilinmeyen kaynaklı `mov` hedefi geçersiz kılmıyor" ve "tuzaklı `ld` ölü yazma sanılıyor" boşlukları yeni `akis/sabit.pma`, `akis/tuzak_ld.pma` testleriyle kapatıldı.


## 6. Dosyalar
`oz/ifade/num/{ifx.cx,ifx.pmb,testler/ca_*.cx,red/*,dogrula_ifx.sh,sabotaj_ifx.py}`, `oz/cfg/{cfgopt.cx,cfgopt.pmb,uret.sh,dogrula_cfg.sh,sabotaj_cfg.sh,sabotaj_cfg.py,akis/}`, `oz/komut/gecis/denetim_ekle.py`, `oz/komut/komutlar.pmk` (JTAB, BRX, şablonlar, OPSIG), `src/anlam.h` (BRX makroları), `src/ifade.cpp` (yeni PMX1 düğümleri, söz dizimi), `oz/sayisal/svm.pma/.pmb`, `oz/hizli.sh` (paket `cfg`).
