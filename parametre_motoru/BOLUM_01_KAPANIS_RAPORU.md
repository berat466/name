# Bölüm 01 — Kapanış Raporu (tüm raporlar bir arada)

Dal: `claude/formul-avcisi` · Son commit: `039aa69` (bu rapor sonrası commit ayrıca) · Kapsam: Linux x86-64 (LP64). **Windows ABI yapılmadı, kapsam dışı.**

## 1. Sonuç
Bölüm 01 (temel sayı ve değer türleri) **kapandı**: 01.001–01.033 son kod sürümünde, soğuk (önbelleksiz) tam koşuyla doğrulandı. PLAN sayımı: 102 🟢 / 42 🔵 / 1255 ❌. Bölüm 02'ye geçilmedi.

## 2. Yapılan gerçek entegrasyon
- **Sayısal tür sistemi:** PMT1 tür notu + TYPES/OPSIG/HOSTSIG + akış duyarlı denetleyici `oz/tip/tipdogr.pma` (sayısal PMA aracı; ELF'i de üretilir). Yanlış tür, geçersiz dönüşüm, yanlış genişlik kesin hata koduyla reddedilir.
- **Genel çoklu yazmaç operand:** KIND `q Q o O`, `rdw/wrw/mulhu`, `lpb/lpe` döngü, `spld/spst` kazıma alanı (256 sözcük); valid/asm/gen2/svm/C++ tutarlı.
- **128 bit tamsayı:** i128/u128 aritmetik, mantık, kaydırma, karşılaştırma, bölme/kalan, `bswap128`, dönüşümler (tamsayı, f16/bf16/f32/f64 tek yuvarlama, f80/f128).
- **long double / float128:** f80 ve f128 yazılım kayan nokta (add sub mul div sqrt neg abs eq ne lt le tobool, ham erişim, 36 dönüşüm komutu/biçim); yalnızca OP+SEM.
- **Karmaşık sayılar:** c64, c128, c160 (add sub mul div neg conj mov eq ne tobool mk real imag, karmaşık↔karmaşık, gerçek→karmaşık; ISO C CX_LIMITED_RANGE).
- **TYPES:** 29 tür, 52 C/C++ tür adı için ABI eşlemesi (`_Float32/64/128/32x/64x`, `std::float*_t` dahil), limitler, dönüşüm matrisi; her sayısal sıralı çift için dönüşüm *yolu* var (karmaşık→gerçek `real_*/imag_*` ile alınır; `nullptr_t` yalnız bool/u64).
- **PMB1 komut kodu ≥ 255:** kaçış kodlaması `0xFF` + LEB128(kod−255); tablo 343 komut; eski programlar bayt-bayt aynı (`dogrula_uyum.sh` 189/189).
- **Bağımsız referans:** C++ `pm` (`src/anlam_wfp.h`, `anlam_wcx.h`: long double, __float128+libquadmath, _Complex) ve derleyici türleriyle yazılmış oracle (`oz/kayan/kayan_oracle.cpp`); ELF = svm = C++ pm = oracle.
- **Native üretim bağımsızlığı:** gen2, svm, asm, valid, coz/yaz, tipdogr sayısal araçlar; C++ yalnız geliştirme/karşılaştırma. `yenile` (pm'siz kendini yeniden üretme) 60/0, `bag` zinciri 1272/0.

## 3. Son bütünleşik doğrulama (soğuk, son kodda: `HIZLI_YENIDEN=1 oz/hizli.sh --tam --bag --sabotaj`)
**47 paket, 0 başarısız.** Duvar 45 dk 29 sn, toplam CPU 9144 sn (4 çekirdek). Normal paketlerde 25 066 kontrol geçti, 0 kaldı.

### 3a. Normal paketler (süre sn, CPU sn)
| Paket | Durum | Süre | CPU | Özet |
|---|---|---|---|---|
| pmy2elf | geçti | 303 | 452 | uctan uca (PMY1 -> ELF, yalniz ELF araclar): gecen 372, kalan 0 (ELF uretilen 160, reddedilen 22, zaman asimi  |
| svm | geçti | 290 | 249 | sayisal anlam yurutucusu: 380 gecti, 0 basarisiz (zaman asimi nedeniyle atlanan: 0) |
| yap | geçti | 229 | 210 | yapisal (parametre): gecen 2366, kalan 0 (zincir kabul 394, ret 1537) |
| valid | geçti | 280 | 198 | PMB1 dogrulayici+analiz (parametre): gecen 5662, kalan 0 (kabul 764, bunlardan zero_free=0: 184, ret 4813) |
| gorunum | geçti | 261 | 188 | PMO1 gorunum ve duzenleme: 928 gecti, 0 basarisiz (atlanan referans karsilastirmasi: 0) |
| asm | geçti | 214 | 181 | assembler (parametre): gecen 2253, kalan 0 (C++ ile ayni uretim: 795, ayni ret: 1418) |
| bag | geçti | 143 | 140 | pm'siz zincir: gecen 1272, kalan 0 (uretilen 164, reddedilen 953) |
| elf2 | geçti | 131 | 127 | ELF uretici gen2: gecen 2787, kalan 0 (karsilastirilan program: 495, tuzakli: 183, uretici reddetti: 0, yakita |
| komut | geçti | 94 | 92 | komut tablosu: 397 gecti, 0 basarisiz |
| donusum | geçti | 126 | 81 | donusum: 797 gecti, 0 basarisiz |
| ikili | geçti | 69 | 61 | PMB1 kodlayici+cozucu (parametre): gecen 3757, kalan 0 |
| oob | geçti | 188 | 42 | OOB denetimi: gecen 1640, kalan 0 (calistirilan program 890: tuzakli 750, tuzaksiz 140; uretici reddetti 0) |
| cagri | geçti | 67 | 37 | call/ret/halt/host mikro-islem sablonlari: 91 gecti, 0 basarisiz |
| uyum | geçti | 39 | 29 | geriye uyumluluk: 189 gecti, 0 basarisiz |
| arac | geçti | 45 | 26 | araclar (doldur/opstab/fmtc): gecen 608, kalan 0 (ayni ret: 262) |
| kural | geçti | 52 | 26 | yeniden yazma kurallari: 352 gecti, 0 basarisiz |
| yenile | geçti | 22 | 24 | motor yenileme (pm'siz): gecen 60, kalan 0 |
| grafik | geçti | 28 | 23 | grafik: 257 gecti, 0 basarisiz (160 karsilastirma) |
| nesne | geçti | 23 | 21 | sayisal nesne ve baglayici: 37 gecti, 0 basarisiz |
| elf | geçti | 33 | 15 | ELF uretici: gecen 92, kalan 0 (karsilastirilan program: 68, ureticinin reddettigi/desteklenmeyen: 19, yakit-t |
| tip | geçti | 28 | 15 | tip denetleyicisi: 131 gecti, 0 basarisiz |
| dar | geçti | 88 | 12 | dar kayan nokta: 82 gecti, 0 basarisiz |
| mikro | geçti | 14 | 12 | mikro-islem yerel uretimi: 214 gecti, 0 basarisiz |
| kayan | geçti | 27 | 11 | genis kayan nokta: 90 gecti, 0 basarisiz |
| ondalik | geçti | 15 | 8 | ondalik sabit: 60 gecti, 0 basarisiz (ikisi de reddeden: 0) |
| tur | geçti | 26 | 5 | tamsayi turleri: 56 gecti, 0 basarisiz |
| i128 | geçti | 16 | 3 | 128 bit tamsayi: 19 gecti, 0 basarisiz |
| testler | geçti | 6 | 2 | KATI ESIT: tum testler gecti |
| float | geçti | 4 | 1 | ondalik: 11 gecti, 0 basarisiz |
| turler | geçti | 17 | 1 | turler: 106 gecti, 0 basarisiz |

### 3b. Mutasyon (sabotaj) paketleri
| Paket | Durum | Süre | CPU | Özet |
|---|---|---|---|---|
| sabotaj_kayan | geçti | 2122 | 2038 | sabotaj(kayan): 133 yakalandi, 35 kacti (35 gerekceli denk, 0 beklenmeyen), 0 uygulanamadi/uretilemedi |
| sabotaj_komut | geçti | 1069 | 1484 | sabotaj(komut): 72 yakalandi, 0 kacti |
| sabotaj_gorunum | geçti | 698 | 831 | sabotaj(gorunum): 60 yakalandi, 0 kacti, 0 uygulanamadi/derlenemedi |
| sabotaj_donusum | geçti | 1423 | 651 | sabotaj(donusum): 22 yakalandi, 0 kacti, 0 uygulanamadi/uretilemedi |
| sabotaj_mikro | geçti | 461 | 405 | sabotaj(mikro-islem sablonlari): 48 yakalandi, 0 kacti |
| sabotaj_tip | geçti | 1360 | 203 | sabotaj(tip): 32 yakalandi, 0 kacti, 0 uygulanamadi/derlenemedi |
| sabotaj_svm | geçti | 338 | 159 | sabotaj(svm): 26 yakalandi, 0 kacti |
| sabotaj_float | geçti | 326 | 157 | sabotaj(ondalik): 24 yakalandi, 0 kacti |
| sabotaj_grafik | geçti | 166 | 157 | sabotaj: 20 yakalandi, 0 kacti |
| sabotaj_nesne | geçti | 268 | 153 | sabotaj(nesne/baglayici): 14 yakalandi, 0 kacti |
| sabotaj_cagri | geçti | 1339 | 134 | sabotaj(cagri): 22 yakalandi, 0 kacti, 0 uygulanamadi/uretilemedi |
| sabotaj_dar | geçti | 1387 | 120 | sabotaj(dar): 27 yakalandi, 0 kacti, 0 uygulanamadi/uretilemedi |
| sabotaj_kural | geçti | 211 | 95 | sabotaj(yeniden yazma kurallari): 19 yakalandi, 0 kacti |
| sabotaj_tur | geçti | 1327 | 86 | sabotaj(tur): 22 yakalandi, 0 kacti, 0 uygulanamadi/uretilemedi |
| sabotaj_i128 | geçti | 1393 | 69 | sabotaj(i128): 39 yakalandi, 0 kacti, 0 uygulanamadi/uretilemedi |
| sabotaj_ondalik | geçti | 235 | 56 | sabotaj(asm ondalik): 17 yakalandi, 0 kacti |
| sabotaj_valid | geçti | 112 | 54 | sabotaj(dogrulayici): 13 yakalandi, 0 kacti |

**Toplam: 645 mutant — 610 yakalandı, 35 kaçtı; kaçanların tümü gerekçeli denk, beklenmeyen kaçan 0.**

### 3c. Denk mutantlar (35, hepsi `sabotaj_kayan`)
Gerekçeler `oz/kayan/sabotaj_kayan_denk.txt`: ölü kod (ör. `ne` komutu `lt` bacağını kullanmaz), sonucu etkilemeyen sabitler (ör. `lit -1→-2` ardından mantıksal kaydırma aynı değeri verir), tam dönüşümde yuvarlama artışının hiç uygulanmaması, NaN yükü/sessiz bit sonraki aşamada zaten konması, f80 sabitlerinde alt 32 bitin hep 0 olması. Her biri elle incelendi; ayrıca rastgele girdili eşdeğerlik taramasında (binlerce girdi) fark çıkmadı. Betik, listede olmayan her kaçanı başarısızlık sayar.

### 3d. Kapatılan gerçek test açıkları
karekök yoğun test; rastgele mantisli dönüşümler; sıfır karmaşık değerler; tie (eşit uzaklık) operandları; f32 alt normal ve aynı büyüklükte bileşenler (c64); tek bit farklı mantisler; hedef biçim sınır üsleri; unnormal f80 değerlerinin küçük N'de de bulunması; `sabotaj_komut`: seyrek kimlik testi ("0 53") ve `types_tag` mutant deseni.

## 4. Atlanan testler
- svm: **0** (varsayılan yakıtı biten programlar artık 3 000 000 adımlık sınırlı yakıtla pm ile svm arasında karşılaştırılıyor).
- pmy2elf: 44 — pm'in varsayılan yakıtını tüketen (ELF yakıtsız üretilir; yakıt tuzağı `dogrula_elf2.sh`/`dogrula_svm.sh`'de sınırlı yakıtla sınanır) ya da çok uzun dönen rastgele modeller (ör. ~7,5e11 yineleme). Gerekçe `ATLANAN` günlüğüne yazılır.

## 5. Önceki hatalar
- `yenile` ve `bag`: bu çalışmada yeşil (60/0, 1272/0). Önceki kırmızı koşu eski kod durumuna aitti.
- `sabotaj_komut` 11 kaçan (eski) → giderildi; son: 72/72.

## 6. Test süresi
| | Süre |
|---|---|
| Önceki ölçülen tam koşu | 13 402 sn (3 sa 43 dk); kullanıcı gözlemi 55–60 dk |
| Yeni soğuk tam koşu (mutasyon dahil) | 45 dk 29 sn |
| Yeni normal kademe (`--tam --bag`) | 12 dk 59 sn, 30 paket, 0 başarısız (CPU 2240 sn) |
| Mutasyon kademesi (`--sabotaj`) | ek ≈ 32 dk |
| Değişmemiş ağaçta tekrar koşu | 4 sn (sonuç önbelleği) |

Yapılan hızlandırma: sabotajda ilk-hatada-dur (`oz/ilkhata.sh`); tüm sabotaj mutantları paralel ve iki aşamalı (ilgili/az örnekli grup ilk aşama, kaçan mutant TAM kosuyla yeniden sinanır); tüm paketler ve mutantlar tek global FIFO yuva havuzunda (`oz/slot.sh`, paketler arası bariyer yok); pmy2elf 4 parça paralel; şeffaf g++ derleme önbelleği (`oz/onbellek_bin/g++`); paket sonuç önbelleği (anahtar = tüm depo ağacının içerik özeti, yalnız geçenler saklanır, `HIZLI_YENIDEN=1` yok sayar); paket başına CPU raporu.

**20 dk (ya da 15 dk) hedefi tek komutla TUTMADI.** Koşu CPU-bağımlı: 9144 CPU-sn / 4 çekirdek = 38 dk alt sınır; 15 dk için ≤ 3600 CPU-sn gerekir. Test/mutant silmeden bu ancak daha çok çekirdek ya da `komut` aracının hızlandırılmasıyla (her çağrıda 510 KB tablo ayrıştırılıyor, ~0,2 sn; testler yüzlerce kez çağırıyor) mümkün.

## 7. Bölüm 01 maddeleri (01.001–01.033)
Tamamı 🟢. Kanıt eşlemesi: 001–010 `tests/`, `dogrula_float`, `dogrula_elf2`; 011 `dogrula_float` (özel değerler × tüm ikili işlemler), FMIN 41×41, `dogrula_kayan`; 012 `dogrula_ondalik` (60/0) + `sabotaj_ondalik`; 013–019, 021, 022, 027, 028 `dogrula_tur`; 020 `dogrula_tur` (bool) + `dogrula_tip` (131/0); 023 `dogrula_dar`; 024, 025, 029 `dogrula_kayan` (90/0); 026 `dogrula_i128` + `dogrula_kayan`; 030 `dogrula_donusum`, `dogrula_turler` (kapsam), `dogrula_tip`; 031 `dogrula_turler` (limitler, C++ derleyici gerçekleriyle); 032 `dogrula_turler` (ABI); 033 `dogrula_turler` (bayt sırası) + `bswap16/32/64/128` testleri.

## 8. Commit'ler
`1ad82c5`, `2072690`, `fbc04b9`, `aaec132`, `940796f`, `ebfba00`, `86cb524`, `039aa69` (+ bu rapor).

## 9. Açık/ kalan
Windows ABI (kapsam dışı); kanonik olmayan f80 kodlamaları (sahte sonsuz/alt normal) tanımsız ama tuzaksız; test süresi hedefi (yukarıda).
