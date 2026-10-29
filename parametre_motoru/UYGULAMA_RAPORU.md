# Ekran/Girdi Kapısı ve Üç Uygulama — Rapor

Soru: aynı çekirdek, **yalnızca parametre dosyaları değişerek** farklı türde uygulamalar çalıştırabiliyor mu?

## Yapılanlar
- **Host kapısı (3 yeni işlev):** `present(ptr, w, h)` kare sunar, `poll()` girdi olayı verir, `frame()` kare sayacıdır (şartname bölüm 4). Yorumlayıcı ve makine kodu aynı `src/host.h` kodunu kullanır; geçersiz argümanda çökmez, −1 döner.
- **Sanal çerçeve tamponu:** pencere yok (sandbox'ta ekran yok). Kare PPM'e, ASCII'ye ya da bir hash zincirine dökülür; girdi, kare numaralı bir betikten gelir. Her şey deterministik.
- **Üç uygulama** (`uygulamalar/`), her biri tek bir parametre dosyası (528, 633 ve 504 bayt; yazma biçiminde 2,2–2,9 KB):
  | Uygulama | Ne yapıyor | Veri/davranış nerede |
  |---|---|---|
  | `hayat.pma` | Yaşam Oyunu, 32×24 simit, 4× ölçekli 128×96 kare, 60 nesil | başlangıç kalıbı veri tablosunda |
  | `sahne.pma` | 160×120 sahne: degrade gökyüzü, ev, ağaç, güneş, bulutlar | **13 şekillik bir tablo** (tür, a, b, c, d, renk0, renk1); çizici kodu tabloyu okur |
  | `pong.pma` | raketli Pong, 400 kare, tuş girdisiyle | girdi `poll()` ile ana bilgisayardan gelir |
- Hepsi **aynı `pm` ikilisiyle** çalışır; hiçbirinde uygulamaya özgü C++ yok.

## Doğruluk (üç bağımsız kanıt)
1. **Yorumlayıcı = makine kodu**, üç yazmaç kipinde: stdout, kare sayısı, **kare zinciri hash'i**, son kare hash'i ve harcanan **yakıt** bire bir aynı (60 + 1 + 400 kare). `.pmb` (kaynaksız parametre dosyası) `.pma` ile aynı.
2. **Bağımsız awk doğrulaması** (ayrı yazılmış, aynı kurallar):
   - Yaşam Oyunu: 60. nesil nüfusu (44) ve en son sunulan karenin 32×24 haritası bire bir aynı.
   - Sahne: PPM'in **57.600 baytının tamamı** awk çiziciyle bire bir aynı (degrade, daire, kırpma dahil).
   - Pong: betikli girdiyle son durum (skor, raket, top) aynı (`-5 33 59 29`).
3. **Farklılık fuzz'ı**, uygulama tohumlarıyla (ekran çağrıları ve kare zinciri karşılaştırmalı): 6 seri × 30.000 deneme, ~15.000 geçerli program, **0 fark, 0 çökme**.
- Toplam: `make test` = 203 kontrol + 20 uygulama denetimi, normal ve ASan/UBSan derlemesinde geçti.

## Hız
| Uygulama | Kare | Yorumlayıcı | Makine kodu | Kare/s (makine kodu) |
|---|---|---|---|---|
| hayat | 60 | 34 ms | 16 ms | ≈ 3.800 |
| sahne | 1 | 13 ms | 8 ms | (tek kare) |
| pong | 400 | 56 ms | 19 ms | ≈ 20.000 |
60 kare/s hedefinin çok üstünde; ama kareler küçük (80×60 – 160×120) ve çizim yazılım düzeyinde.

## Dürüst değerlendirme
- **"Kodsuz" hâlâ kısmen doğru.** Dağıtılan şey parametre dosyası ve motor onu çalıştırıyor. Ama uygulamaların *mantığı* (çizici, Yaşam Oyunu kuralı, Pong fiziği) hâlâ kod; ben `.pma` olarak **yazdım**. Gerçekten veri olan kısım: Yaşam Oyunu'nda başlangıç kalıbı, sahnede şekil tablosu. Sahne uygulaması bu fikrin en açık örneği: tabloyu değiştirirsen sahne değişir, çizici koduna dokunmazsın. Ama çizici hâlâ kod.
- **Şablon katmanı** (bölüm 9) yok: sahnedeki "13 şekil" bir veri tablosu, motor düzeyinde bir şablon/örnekleme mekanizması değil.
- **Gerçek pencere yok.** Kareler PPM/ASCII/hash olarak alınıyor; SDL/X11 arka ucu yazılmadı ve denenmedi. Ses, fare, dosya, ağ yok.
- AI'nın doğrudan parametre üretmesi (hedefin 4. maddesi) denenmedi; bu uygulamaları ben yazdım, bağımsız ölçüm değil.
- Kıyas yok: aynı uygulamaların yerel C++ karşılıklarıyla boyut/hız karşılaştırması yapılmadı.
