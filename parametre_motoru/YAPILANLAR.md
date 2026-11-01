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
**Test hızlandırma (2 Kasım):** önceki ölçülen tam koşu 13 402 sn (3 s 43 dk; kullanıcı gözlemi 55–60 dk) → **45 dk 29 sn**. Yapılanlar: sabotajda ilk-hatada-dur (`oz/ilkhata.sh`), sabotaj mutantlarının hepsi paralel ve iki aşamalı (az örnekli/ilgili grup ilk aşama, kaçan mutant TAM kosuyla yeniden sinanır), tüm paketler ve mutantlar tek global FIFO yuva havuzunda (`oz/slot.sh`, bariyer yok), pmy2elf 4 parçada paralel (452→303 sn duvar), şeffaf g++ derleme önbelleği (`oz/onbellek_bin/g++`, içerik anahtarlı), paket sonuç önbelleği (anahtar = tüm depo ağacının içerik özeti; yalnız geçenler saklanır; değişmemiş ağaçta tam koşu 4 sn, `HIZLI_YENIDEN=1` ile yok sayılır). **20 dk hedefi TUTMADI:** koşu CPU-bağımlı (9144 CPU-sn / 4 çekirdek = 38 dk alt sınır). En büyük CPU: sabotaj_kayan 2038, sabotaj_komut 1484, sabotaj_gorunum 831, sabotaj_donusum 651, pmy2elf 452, sabotaj_mikro 405, svm 249. 20 dk için ≤ 4800 CPU-sn gerekir; bunu test/mutant silmeden sağlamak ancak daha çok çekirdek ya da komut aracının (her çağrıda 510 KB tablo ayrıştırılıyor, ~0,2 sn) hızlandırılmasıyla mümkün.
