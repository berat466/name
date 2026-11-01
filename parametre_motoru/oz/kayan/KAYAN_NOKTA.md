# Genişletilmiş kayan nokta (f80, f128) ve karmaşık sayılar (c64, c128, c160) — Plan Bölüm 01

**Kapsam [Kesin]:** Linux x86-64 (LP64, System V). Windows (LLP64; `long double` = 64 bit) ABI'si **bu çalışmada yapılmadı ve tamamlandı sayılmaz**; `TYPES` ABI kayıtları yalnız ABI 1 (Linux x86-64) içindir.

## Mimari
Tüm komutlar komut tablosunda (`oz/komut/komutlar.pmk`) yalnızca **OP + SEM** kaydıdır; x86 şablonu, C++ anlam kodu gerekmez:
- `svm.pmb` tabloyu çalışma anında okuyup yürütür, `gen2.pmb` aynı mikro-programları x86-64'e indirger (ELF), C++ `pm` ise **bağımsız referans**tır (`src/anlam_wfp.h`, `src/anlam_wcx.h`; derleyici türleri `long double`, `__float128`+libquadmath, `float/double/long double`). Üçü aynı sonucu vermek zorundadır (`dogrula_kayan.sh`).
- Mikro-program araçları: dönüşüm döngüsü `lpb`/`lpe` (tek seviye, sayaç geçicisi), kazıma alanı `spld`/`spst` (komut örneği boyunca yaşayan **256 sözcük**; ELF'te `rsp-144-8n`, svm'de ayrı dizi), geniş operand `rdw`/`wrw` (KIND `q Q o O`), `mulhu`. Geçici sayısı 16 (`t1..t16`).
- Üretim araçları (GELİŞTİRME aracı, motorun parçası değil; çıktısı tablodaki sayılardır): `sf.py` (açma/paketleme/yuvarlama), `ops.py` (işlemler), `cx.py` (karmaşık), `oz/komut/gecis/tur11..13_ekle.py`. Python yorumlayıcı `mikro.py` yalnız üretecin hızlı sınanması içindir.
- **Komut kodu ≥ 255 (PMB1):** kod < 255 tek bayt; ≥ 255 ise `0xFF` + LEB128(kod−255). Eski kodlar aynen kalır; eski araç yeni kodu `bilinmeyen komut kodu` diye reddeder. `ikili.cpp`, `asm.pma.in` (`asm.pmb`), `yaz.sayi.in`, `coz.sayi.in` birlikte güncellendi; gen2/valid/svm sayı listesi (`pm liste`) okur, etkilenmez.

## Temsil
| Tür | Yazmaç | Not |
|---|---|---|
| f80 | 2 sözcük: ham mantis (açık tam sayı biti) / `(işaret<<15)\|üs` | `pack_f80 Q,r,r`, `lo64_f80`, `hi64_f80` ham erişim |
| f128 | 2 sözcük: kesir düşük 64 / `(işaret<<63)\|(üs<<48)\|kesir yüksek 48` | |
| c64 | 2 sözcük: gerçek, sanal (her biri **f32 değeri double kabında**) | f32 gibi |
| c128 | 2 sözcük: iki double | |
| c160 | 4 sözcük: f80 gerçek (2) + f80 sanal (2) | KIND `o`/`O` |

## Anlam (motor tanımı; donanımdan ayrıldığı yerler açıkça)
- Yuvarlama: en yakına, eşitlikte çifte; taşma → ±∞; alt normal tam; `f80` 64 bit, `f128` 113 bit anlamlı.
- **NaN:** NaN işlenen varsa *ilk* NaN işlenenin ham sözcükleri sessizleştirilerek (f80: lo bit 62, f128: hi bit 47) sonuç olur; NaN yoksa ve işlem geçersizse **varsayılan NaN** = negatif sessiz NaN (f80: `0xFFFF`/`0xC000…`, f128: hi `0xFFFF8000…`). x87'nin "büyük yük" seçimi **uygulanmaz**. f80'de üssü 0/tam-1 olmayan ve tam sayı biti 0 olan desen (unnormal) NaN sayılır.
- **Kanonik olmayan f80 kodlamaları** (sahte sonsuz, sahte alt normal): sonuçları *tanımsız ama tuzaksız*; doğrulama bunları kullanmaz [Kesin: `kayan_oracle.cpp` üretmez].
- `neg`/`abs`: yalnız işaret biti (NaN dahil). `eq ne lt le`: sırasız → `eq/lt/le = 0`, `ne = 1`; `tobool`: NaN → 1, ±0 → 0.
- **Dönüşümler:** f64 → geniş: tam; geniş → f64: tek yuvarlama; f80 ↔ f128; geniş → f32/f16/bf16: **tek yuvarlama** (tek'e yuvarlayarak f64 + `RND_*`; çifte yuvarlama olmaz); tamsayı → geniş (i64/u64/i128/u128): en yakına; NaN dönüşümlerinde kesir yükü kaydırılır, sessiz bit konur, işaret korunur.
- **Kayan → tamsayı:** sıfıra doğru kes; geçersiz (NaN/∞/aralık dışı) → işaretli hedefte en küçük değer (`0x80…0`), işaretsizde 0; dar hedefler (i8/i16/i32/u8/u16/u32) önce 64 bit sonra hedef aralığa sarar (mevcut `f2i_*` ile aynı kural). C++'ta aralık dışı tanımsızdır; motor tanımlıdır.
- **i128/u128 ↔ f16/bf16/f32/f64:** tek yuvarlama (`cv_i128_*`, `cv_u128_*`), geri dönüş `cv_f64_i128/u128` (f16/bf16/f32 kaynakları da bu komutu kullanır).
- **Karmaşık:** ISO C `CX_LIMITED_RANGE` (düz formüller, Ek G kurtarması YOK): `(a+bi)(c+di) = (ac−bd)+(ad+bc)i`, bölme `((ac+bd)/(cc+dd)) + ((bc−ad)/(cc+dd))i`. Her bileşen işlemi kendi türünün yuvarlamasıyla (c64: her işlem sonrası f32). `eq`: iki bileşen eşit, `ne = !eq`, `tobool`: gerçek≠0 ya da sanal≠0. Karmaşık → gerçek **dönüşüm değildir**: `real_*`/`imag_*`. Gerçek → karmaşık sanal 0; tamsayı → karmaşık iki kayıtlı dönüşümün (tamsayı→kayan→karmaşık) birleşimidir.

## Komutlar (OP adı = mnemonik)
`add_F sub_F mul_F div_F sqrt_F neg_F abs_F mov_F pack_F lo64_F hi64_F eq_F ne_F lt_F le_F tobool_F` (F = f80, f128); dönüşümler `cv_<kaynak>_<hedef>`:
f64→F, F→f64, f80↔f128, F→f32/f16/bf16, i64/u64/i128/u128→F, F→i64/u64/i128/u128/i8/u8/i16/u16/i32/u32, i128/u128→f64/f32/f16/bf16, f64→i128/u128;
karmaşık `add sub mul div neg conj mov eq ne tobool mk real imag` (C = c64, c128, c160), `cv_c<a>_c<b>` (6), `cv_f64_C`, `cv_f80_C`.

## Doğrulama
`oz/kayan/dogrula_kayan.sh` (ELF = svm = C++ pm = bağımsız oracle; her program ayrıca sayısal assembler = C++ assembler ve `liste` gidiş-dönüşü ile sınanır), `oz/tip/dogrula_tip.sh` (program düzeyi tür denetimi: f80/f128/karmaşık), `oz/komut/dogrula_turler.sh` (TYPES, limitler, ABI, **dönüşüm kapsamı**), `oz/komut/dogrula_komut.sh`; mutasyon: `oz/kayan/sabotaj_kayan.sh`. Geliştirme aşamasında üreteçler tam rasyonel referansa (`ref.py`) karşı sınandı (`t_addsub.py t_mul.py t_div.py t_sqrt.py t_conv.py t_cx.py`).
