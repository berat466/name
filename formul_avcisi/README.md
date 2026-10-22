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
