# Formul Avcisi

Matris carpimi icin **az carpmali ve kayipsiz (tam esit)** algoritmalar arar.
Cikan formulleri insan okur ve yorumlar; sistem hicbir sonucu "bulgu" saymaz,
tam kanitlayamadigi surece.

## Calistirma
```bash
python search.py --n 2 --rank 7 --restarts 600 --out r7_bulunan.json   # Strassen'i bulur
python search.py --n 2 --rank 6 --restarts 400                          # imkansiz: 0 dogrulama beklenir
```

## Nasil calisir
1. n x n matris carpimi bir 3-boyutlu tensor T'dir.
2. T'yi R "tek carpma" terimine ayirmak = R carpmali algoritma. ALS ile sayisal arar,
   degerleri {-1,0,1}'e iter.
3. Hakem: tamsayi tensor, T'ye **birebir esit** mi (kanit). Degilse atilir.

## Sonuclar (2x2, bu ortamda)
- rank 8 (klasik): arama zayif kaldi, 0/40 (bu yontem sureksiz aile cozumlerini sevmiyor).
- rank 7: 7/600 deneme kanitlandi; en iyisi **7 carpma + 18 toplama = Strassen ile ayni**.
- rank 6: 0/400 (bilinen sonuc: 2x2 icin 7 en iyisi, yani beklenen).

## Sinirlar
- Yeni bir sey bulmadi: bilinen sonucu yeniden buldu. Bu, yontemin calistiginin kaniti, yeniligin degil.
- Az carpma = hizli GPU degildir: Strassen sayisal olarak daha kararsiz, GPU'da yogun matmul
  (Tensor Core) ve bellek hizi cogu zaman belirleyicidir. Hakem simdilik yalnizca carpma/toplama sayar.
- 3x3 (en iyi bilinen 23) ve buyuk n icin bu basit arama yetmeyebilir.

## Optimizer formul simulatoru (`optim_search.py`)
Semboller (`g m v w Δg`, `sign tanh sqrt ...`) rastgele birlestirilir; her formul mini sinir agi
egitiminde puanlanir (ogrenme hizi + yeni gorevden sonra eskiyi unutma). Referanslar: SGD, Momentum,
RMSprop, Adam, AdamW, Lion-benzeri; hepsi ayni ogrenme orani izgarasinda taranir.

**Bulgu (dogrulama + kontrol testleriyle):** Arama `Adam + sign(1.1 - 2w)` gibi bir formul buldu.
Dar testte Adam'dan %15-20 iyi gorundu, ama (1) Adam'in ogrenme orani izgarasi alt uctan eksikti;
izgara genisletilince uzun spiral gorevinde avantaj kayboldu, (2) kazanc yalnizca "daire" gorevinde
tutarli, (3) unutma 3 testin 2'sinde kotulesti. Sonuc: genel bir optimizer kesfi DEGIL, gorev
ozel bir etki. Ders: tek gorevde arama, bench'i somuruyor; baseline'lar adil ayarlanmali.
Betikler: `stres_testi.py`, `kontrol_testi.py`.

## Cok gorevli simulasyon (`optim_suite.py`) - ilk dayanikli sonuc
200 gorunmemis rastgele gorev (8 tur: spiral, daire, ay, kume, xor, 3 regresyon; farkli gurultu, genislik,
uzunluk; yarisinda ardisik gorev), her formul icin TEK ortak ogrenme orani (0.003..10 izgarasi), her nesil
farkli gorevler. Sonuc (`sonuc_suite.md`, `kontrol_suite.py`):
- Cikan formullerin cogu Adam'in epsilon'u oynatilmis kopyasi (+%1-3, yazi-tura duzeyi): gurultu.
- Tek tutarli aday: `tanh(m / (sqrt(v) + v))` -> Adam'a gore **+%16 daha az kalan kayip**, 200 gorevin %64'unde
  daha iyi. Parca parca: yalniz tanh +%5.5, yalniz ek payda `+v` +%9.9, ikisi +%16 (toplamsal).
  Yorum: buyuk gradyanlarda adimi kucultup (payda) adimi sinirlayan (tanh) bir Adam; gradyan-patlama
  korumali Adam gibi. Yeni ilke degil; bu testte islediginin kaniti.
- SINIRLAR: kucuk tanh MLP'ler, yapay gorevler, <=500 adim, olculen sey EGITIM kaybi (genelleme degil).
  Gercek veri ve buyuk ag icin DOGRULANMADI.

## Capa (eski deger formule verilir) - unutmaya karsi ilk saglam sonuc
Fikir (kullanici): guncellerken eski degeri de formule ver. `a` = gorev baslamadan onceki agirlik.
Kural: `w <- w - lr * ( Adam(m,v) + 0.1 * (w - a) )`  (capa yalniz gorev DEGISIMINDEN sonra aktif).
200 gorunmemis gorev (93 ardisik), tek ortak lr (`capa_ilk_faz_testi.py`, `plastisite_kontrol.py`):
- Ardisik gorevlerde kalan kayip Adam'dan **%53 dusuk**; eski gorev kaybi 0.87 -> 0.46, YENI gorev ogrenmesi bozulmadi (0.087 -> 0.063).
- Tek-gorevli ornekte capa=0 oldugundan Adam ile ayni; "tum gorevler" kazanci (%30) ardisik gorevlerden gelir.
- Evrimsel arama bu insan yapimi capali baseline'i GECEMEDI (en iyi kesif: Adam+m, %30 ardisik).
- SINIRLAR: gorev siniri BILINIYOR varsayildi; kucuk tanh MLP; yapay gorevler; tekrar (replay) ve gercek EWC ile
  karsilastirilmadi. SGD eskiyi daha iyi tutar (0.35) ama yeniyi kotu ogrenir (0.21).

## Yerel kural + eski degisim hafizasi (`lokal_kural.py`)
Kural: `D_t = ort(giris * hata) + mu * D_{t-1}`; `w <- w - lr*D`. Gizli katman hatasi: `bp` (geri yayilim),
`fa` (sabit rastgele geri baglanti), `none` (gizli katman ogrenmez). 200 gorunmemis gorev; referans Adam(bp)
tum=0.0507, ardisik=0.5523 (kalan kayip, dusuk iyi).
| gizli yol | eski degisim | tum | ardisik |
|---|---|---|---|
| bp | yok | 0.279 | 0.318 |
| bp | 0.9 | 0.114 | 0.728 |
| fa | yok | 0.239 | 0.295 |
| fa | 0.9 | 0.135 | 0.329 |
| none | yok / 0.9 | 0.367 / 0.298 | 0.395 / 0.396 |
- Eski degisim hafizasi tum gorevlerde buyuk kazanc (momentum etkisi) ama ardisikte daha cok unutuyor (bp: 0.32 -> 0.73).
- Rastgele sabit geri baglanti (geri yayilimsiz kredi atama) bu kucuk aglarda sadece ~%18 kotu (0.135 vs 0.114).
- En iyi yerel kural Adam'dan ~2.7x kotu: eksik parca "buyukluk hafizasi" (v).
- UYARI: feedback alignment'in geri matrisi carpimi de geri yayilimla ayni maliyette; hesap tasarrufu yok, kazanc bellek/agirlik tasima.

## Geri yayilimsiz yerel kural aramasi (`lokal_arama.py`)
Gizli katmana hata SABIT rastgele geri baglantiyla tasinir (feedback alignment). 120 gorunmemis gorev (57 ardisik).
Referans klasik Adam(bp): tum 0.046, ardisik 0.439. Sonuclar: `sonuc_lokal_indep.md`, `sonuc_lokal_full.md`.
- Adam-FA 0.081 (%74 kotu); **Adam-FA + capa 0.069 (%48 kotu) ama ardisikte 0.297 (klasikten %32 iyi)**.
- Tam rastgele baslangicli bagimsiz arama (Adam'in `m` degiskeni yok): klasik NLMS'i (`g/(giris gucu*c)`) yeniden buldu;
  buyukluk hafizasi (`q`) ve eski degisim (`u`) kullanan bir sey bulamadi (kucuk butce).
- Tohumlu arama: Adam-FA'nin epsilon kopyalari; insan yapimi capali baseline'i gecemedi.
- Sinirlar: tek gizli katman, yapay gorevler; hata tasima matmul'u hala var (islem tasarrufu yok).

## Ekleme-yalniz ag (append-only / progressive) - `ekleme_agi.py`, `ekleme_kontrol.py`
Fikir (kullanici): eski hesap/agirlik korunur, yeni islem+agirlik zincire EKLENIR; yeni islem eski ciktilari kullanir.
Uygulama: A gorevi ogrenilir, dondurulur; B icin yeni gizli blok + A'nin gizli ozelliklerinden yanal baglanti.
93 ardisik gorev (200 gorunmemisten), kalan kayip orani (dusuk iyi), tek ortak lr:
| yontem | ESKI gorev | YENI gorev | ortalama |
|---|---|---|---|
| Adam ince ayar | 0.386 | 0.117 | 0.287 |
| Adam + capa(0.1) | 0.366 | 0.066 | 0.244 |
| **EKLEME, gorev kimligi biliniyor** | **0.013** | 0.031 | **0.030** |
| EKLEME, merkez-uzakligi yonlendirici | 0.028 | 0.081 | 0.070 |
| EKLEME, kimliksiz toplam | 0.396 | 0.075 | 0.267 |
- Kapasite aciklamiyor: ~11x genis (esit parametreli) ince ayar ag da 0.32-0.33 (ekleme agi 0.030).
- Kimlik/yonlendirme olmadan (toplam) unutma geri geliyor: 0.267 ~ ince ayar.
- BEDEL: parametre ORTALAMA 11.4x (yanal baglanti H*H) TEK ek gorev icin; her gorevde buyur.
- Sinir: regresyon fazlari birbirine benzer fonksiyonlar oldugu icin yonlendirici testi sorunu hafife aliyor.

### Buyumeyi kucultme (`ekleme_kucuk.py`, kimlik biliniyor, 93 ardisik gorev)
| ekleme bicimi | ESKI | YENI | ortalama | toplam param (temel=1) |
|---|---|---|---|---|
| tam (Hn=H, yanal tam) | 0.013 | 0.031 | 0.030 | 8.7x |
| Hn=H/2, yanal rutbe 4 | 0.027 | 0.033 | 0.038 | 2.7x |
| Hn=H/4, yanal rutbe 2 | 0.013 | 0.061 | 0.047 | 1.8x |
| Hn=H/4, yanal YOK | 0.013 | 0.088 | 0.070 | 1.3x |
Referans Adam+capa 0.244. Kazancin cogu yanal baglantidan degil, eskiyi dondurup YENI blok eklemekten geliyor.
Bedel: her yeni gorevde +%27 (H/4) buyume, kimlik/yonlendirme sart.

## Anahtarla ust uste bindirme (superposition) - `anahtar_ust_uste.py`
Fikir (kullanici, guvenlikteki tuzlamadan): ayni agirlik, her gorevde farkli rastgele +-1 anahtar: `W_etkin = W (.) K_gorev`.
Parametre BUYUMEZ; gorev kimligi (anahtar) biliniyor. 93 ardisik gorev, kalan kayip orani (dusuk iyi), tek ortak lr:
| genislik | anahtarsiz ince ayar | anahtarsiz+capa | ANAHTARLI | ANAHTARLI+capa |
|---|---|---|---|---|
| x1 (kucuk ag) | 0.287 | 0.244 | 0.308 | 0.296 |
| x4 | 0.297 | 0.293 | 0.222 | 0.208 |
| x8 | 0.323 | 0.318 | 0.180 | 0.176 |
- Kucuk aglarda fayda yok (kapasite yok); ag genisledikce anahtar unutmayi kesiyor: eski gorev 0.44 -> 0.145 (x8).
- x8'de anahtarli ~%45 daha az kalan kayip; lr izgarasinin alt ucu genisletilince degismedi.
- Yine de ekleme zincirinin (0.030, ama +%27..+%770 parametre) gerisinde; anahtar parametre buyumeden geliyor.
- Sinirlar: kimlik biliniyor; kucuk-orta tanh MLP; yapay gorevler; cok sayida gorev (>2) test edilmedi.

### Anahtar: 2 / 5 / 10 ardisik gorev (`anahtar_cok_gorev.py`, genislik x8, 30 dizi, lr alt ucu genis)
| gorev sayisi | anahtarsiz | ANAHTARLI | anahtarli kazanc |
|---|---|---|---|
| 2 | 0.302 | 0.226 | %25 |
| 5 | 0.495 | 0.359 | %28 |
| 10 | 0.442 | 0.424 | %4 (yok) |
- Anahtarli yontem ~5 gorev mertebesine kadar fayda veriyor, 10 gorevde kazanc kayboluyor (gurultu birikiyor).
- Capa, 2'den fazla gorevde neredeyse fark yaratmiyor (yalniz bir onceki fazin basina capalar).
- DERS: lr izgarasinin alt ucu eksik oldugunda anahtarsiz referans %24'e kadar haksiz kotu gorundu. Izgara artik 0.0003'ten basliyor.

## Hafiza gibi / tek-hamle ogrenme (`hafiza_ogrenme.py`, `hafiza_digits.py`, `hafiza_ardisik.py`)
Fikir (kullanici): agirligi dongusuz, tek seferde uret; ogrenmek = depolamak; cevap = hafizadan bulmak.
Uygulama: ELM (rastgele ozellik + halka cozum, tek hamle) ve kNN (saklama). Hep TEST verisinde.
- **Oyuncak gorevler (107 tek-fazli):** ELM Adam kalitesinde (+%3.7), ogrenme islemi benzer; kNN ogrenmesi bedava ama cok daha kotu.
- **GERCEK veri, el yazisi rakamlar (5 bolme, dogrulamayla secilen hiperparametre):**
  | yontem | test dogruluk | ogrenme islemi |
  |---|---|---|
  | Adam MLP | %96.6 | 1.1e10 |
  | ELM tek hamle (256) | %96.1 | 1.3e8 (~85x az) |
  | ELM tek hamle (1024) | %97.9 | 1.8e9 (~6x az) |
  | kNN (saklama) | %96.8 | ~0 (sorgu maliyeti n*64) |
- **Ardisik gorevler (93, TEST):** istatistik biriktirme `S += F^T F, R += F^T T` ESKI 0.092 / YENI 0.065 / ortalama **0.127**;
  Adam ince ayar (ayricalikli lr) 0.217; yalniz son gorevle ELM 0.319. Unutmayi cozen sey toplanabilir istatistik.
- SINIRLAR: kucuk/kolay veri; ozellikler ogrenilmiyor (rastgele) -> karmasik algida zayiflar; ELM maliyeti Hf^2..Hf^3;
  biriktirilen ozet Hf^2 sayi (agin parametresinden cok daha buyuk); RLS/analitik surekli ogrenme ailesine yakin.
