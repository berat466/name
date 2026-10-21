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
