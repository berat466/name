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
