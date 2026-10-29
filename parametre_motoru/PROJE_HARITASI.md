# Parametre Motoru — proje haritası

Bu belge **neyin nerede olduğunu** anlatır (kalıcı harita). **Ne zaman ne yapıldığı** için `YAPILANLAR.md` (günlük); yol haritası için `CPP_YOL_HARITASI.md`; ayrıntılı tasarım ve sonuçlar için `OZ_BARINDIRMA.md`.

## 1. Bir cümleyle
Uygulamaların, makine kodu üretiminin ve geliştirme araçlarının **sayısal parametre tabloları** olduğu bir motor. Asıl kaynak sayılardır; okunabilir metin (`.pma`, `.pma.in`, görünüm `.pmv`) isteğe bağlıdır ve kayıpsız çevrilir. Eski C++ `pm` **bağımsız doğrulama referansı** olarak kalır.

## 2. Sayısal biçimler (hepsi sayı listesi)
| Biçim | Ne | Nerede |
|---|---|---|
| **PMB1** | ikili program tablosu (komutlar, sabitler, veri); `pm liste` düz sayı listesine döner | `src/ikili.cpp`, çözücü/kodlayıcı `oz/coz.*`, `oz/yaz.*` |
| **PMY1** | yapısal program tablosu (if/while/atama/ifade ağacı) | `src/yapisal.cpp`, dönüştürücü `oz/yap/` |
| **PMK1** | kanonik komut tablosu: komut/host/operand türü, **anlamlar (SEM)**, mikro-işlemler (PRIM), takma komutlar (PSEUDO), yeniden yazma kuralları (REWRITE), x86 şablon simgeleri (TOKEN) ve şablonları (TEMPLATE) | `oz/komut/komutlar.pmk`, belge `oz/komut/KOMUT_TABLOSU.md` |
| **PMO1** | sayısal araç nesnesi (taslak liste + bağıl kayıtlar); bağlayıcı tablodan ve nesneden PMB1 üretir | `oz/nesne/{asm,valid,gen2}.pmo`, `oz/arac/baglayici.*` |
| **PMV** | PMO1'in okunabilir görünümü (düzenlenir, `yaz` ile aynı sayılara döner) | `oz/arac/gorunum.*`, `oz/nesne/duzenle.sh`, belge `oz/nesne/GORUNUM.md` |

## 3. Üretim zinciri (kim kimi üretir)
```
komutlar.pmk ──komut @@tab──► sözlük ──┐
                                         ├─ baglayici ─► PMB1 (asm.pmb, valid.pmb, gen2.pmb)
asm/valid/gen2 .pmo (PMO1) ─────────────┘
PMB1 ──gen2 (x86 şablonları + mikro-işlemler)──► bağımsız Linux x86-64 ELF
PMB1 ──svm (SEM'i çalışma anında okur)──► yürütme
.pmo ──gor──► .pmv ──düzenle──► yaz ──► .pmo        (istege bağlı okunabilir düzenleme)
ELF takımı (oz/bag/*.elf) ──yenile.sh──► yeni ELF takımı   (pm'siz kendini yeniden üretme, A→B→A)
```
Önyükleme `pm`'i bir kez kullanır (`oz/bag/kur.sh`); sonrası ELF araçlarla `pm`'siz.

## 4. Dizin haritası (`parametre_motoru/`)
| Yol | İçerik |
|---|---|
| `src/` | **C++ `pm`** (bağımsız referans): yükleyici, doğrulayıcı, VM (`vm.cpp`, `anlam.h`), native x86 (`native_x64.cpp`), `asm.cpp`, `yapisal.cpp`, `liste.cpp`; `ops_gen.h` komut tablosundan üretilir |
| `tests/` | ana test koşucusu (`run_tests.sh`) ve 28 örnek program (`*.pma`, beklenen çıktı/tuzak) |
| `uygulamalar/` | örnek uygulamalar (hayat, sahne, pong, şablonlar) |
| `oz/komut/` | **PMK1 tablosu** (`komutlar.pmk`), belge, test (`dogrula_komut.sh`), sabotaj, rastgele tablo üreteci, geçiş araçları (`gecis/`) |
| `oz/arac/` | küçük araçlar (PMA kaynak + `.pmb`): `doldur`, `opstab`, `fmtc`, `komut` (tabloyu işler), `baglayici` (PMO1 bağlar), `gorunum` (PMV), `paketle` |
| `oz/asm/` | **assembler** (`asm.pma.in` metin kopyası, `asm.pmb`), kural ve ondalık testleri |
| `oz/dogr/` | **PMB1 doğrulayıcı + analiz** (`valid.*`): kuralları SEM'den türetir |
| `oz/elf/` | ilk ELF üretici (PMY1 olarak) ve OOB testi |
| `oz/elf2/` | **gen2**: çok fonksiyonlu bağımsız ELF üretici (`gen2.pma.in`, `gen2.pmb`), çalışma zamanı kitaplığı `rt.*` (present/poll/frame/putf), testler: elf2, ondalık, grafik, mikro, **çağrı** |
| `oz/yap/` | PMY1 → `.pma` dönüştürücüsü |
| `oz/sayisal/` | **svm**: SEM'i çalışma anında okuyup yürüten sayısal yürütücü |
| `oz/nesne/` | **PMO1 nesneleri**, bağlayıcı/görünüm testleri, `duzenle.sh`, Python referansı (`ref/`) |
| `oz/bag/` | pm'siz zincir: `kur.sh` (önyükleme), `yenile.sh` (kendini yeniden üretme), `*.elf` (git'e girmez), testler |
| `oz/hizli.sh` | **hızlı test sistemi** (değişen dosyaya göre paketler, `--tam`) |
| kökte `*_RAPORU.md`, `SARTNAME.md`, `YAPISAL.md` | önceki aşamaların raporları (bkz. §7) |
| `../formul_avcisi/` | **bu motordan ayrı** önceki çalışma: formül arama ve tek-hamle ağırlık üretimi deneyleri (V1–V81 raporları) |
| `../app_service/`, `../web_service/`, `../siber_savunma/` | motordan ayrı, depoda duran diğer klasörler (içerikleri bu belgede incelenmedi) |

## 5. Test haritası (hangi test neyi korur)
Hızlı çalıştırma: `oz/hizli.sh` (ilgili paketler + kritik denetimler), tam: `oz/hizli.sh --tam [--bag] [--sabotaj]`. Listeler: `oz/hizli.sh --alanlar`.
| Paket | Betik | Korur |
|---|---|---|
| testler | `tests/run_tests.sh` | ana programlar: yorumlayıcı ↔ ikili ↔ native, çıktı/çıkış kodu/tuzak/yakıt |
| asm, kural, ondalik | `oz/asm/dogrula_*.sh` | assembler = C++ `pm asm` (bayt-bayt), yeniden yazma kuralları, ondalık sabit |
| valid | `oz/dogr/dogrula_valid.sh` | doğrulayıcı = C++ `listedogrula` (güvenlik kapısı) |
| ikili | `oz/dogrula_ikili.sh` | PMB1 kodlayıcı/çözücü = C++ `liste/yazliste` |
| yap | `oz/yap/dogrula_yap.sh` | PMY1 dönüştürücü |
| elf, oob | `oz/elf/dogrula_*.sh` | ilk ELF üretici, bellek sınırı |
| elf2 | `oz/elf2/dogrula_elf2.sh` | gen2 ELF = `pm run` (stdout, çıkış kodu, tuzak, yakıt), bağımsızlık, sabit nokta |
| float, grafik, mikro, cagri | `oz/elf2/dogrula_*.sh` | SSE2 ondalık, ekran/girdi, SEM'den yerel üretim (zorlanmış), `call/ret/halt/host` mikro şablonları |
| svm | `oz/sayisal/dogrula_svm.sh` | SEM yürütücüsü = `pm` |
| komut | `oz/komut/dogrula_komut.sh` | PMK1 biçimi, kayıpsız çevirim, genişletme (yeni komutlar yalnız sayılarla) |
| nesne, gorunum | `oz/nesne/dogrula_{nesne,gorunum}.sh` | bağlayıcı = metin yolu; PMV gor↔yaz kimliği, uçtan uca düzenleme; bağımsız Python referansı |
| arac | `oz/arac/dogrula_arac.sh` | araçların ve ikililerin tazeliği |
| bag, pmy2elf, yenile | `oz/bag/dogrula_*.sh` | pm'siz zincir, PMY1→ELF, kendini yeniden üretme |
| hizli | `oz/dogrula_hizli.sh` | hızlı sistemin kendisi |
Sabotaj (testlerin gerçekten yakaladığı): `oz/*/sabotaj_*.sh` (çağrı şablonları: `oz/elf2/sabotaj_cagri.sh`) (mutant üretir, testin başarısız olmasını bekler).

## 6. Kurallar (çalışma biçimi)
1. ✅ yapılanları yeniden yapma; mevcut testlerle koru. 2. İlk ❌ maddeden başla. 3. Bağımsız referans + hedefli test + regresyonla doğrula. 4. Başarılıysa yol haritasında ✅ yap, kanıtı belgeye yaz, **`YAPILANLAR.md`'ye ekle**, commit/push. 5. Kanıtsız ✅ yok. 6. Yeni kaynak dili/ayrı motor/mimari değişikliği yok.
Tasarım ilkeleri: sayısal asıl kaynak; güvenlik ayrı, sürümlü katmanlar (henüz yok); genişletilebilirlik (yeni komut = tablo kaydı); doğrulayıcı kapısı kapatılamaz.

## 7. Belge indeksi
| Belge | Konu |
|---|---|
| `SARTNAME.md` | motorun şartnamesi (komutlar, biçimler) |
| `RAPOR.md`, `PROFIL_RAPORU.md`, `KOD_URETIMI_RAPORU.md`, `NATIVE_RAPORU.md`, `UYGULAMA_RAPORU.md`, `SABLON_RAPORU.md` | ilk aşamalar: motor, profil, kod üretimi, makine kodu, uygulamalar, şablon katmanı |
| `AI_TESTI_RAPORU.md`, `AI_SAYISAL_RAPORU.md`, `AI_YAPISAL_RAPORU.md`, `AI_BUYUK_OLCEK_RAPORU.md`, `YAPISAL.md` | AI'nın tabloyu doğrudan üretebilmesi deneyleri, PMY1 |
| `OZ_BARINDIRMA.md` | öz-barındırma: bölüm 3–3n (PMB1 → ELF → kendini üretme → PMK1 → PMO1 → görünüm) ve dürüst sınırlar |
| `oz/komut/KOMUT_TABLOSU.md` | PMK1 ayrıntısı |
| `oz/nesne/GORUNUM.md` | PMV biçimi ve hata kodları |
| `CPP_YOL_HARITASI.md` | C++ (Linux) yetenek eşdeğerliği — madde madde ✅/❌ |
| `YAPILANLAR.md` | günlük: ne, ne zaman, hangi commit, hangi test |
