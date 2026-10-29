# V58: 10 dakikalık sözlük ölçekleme testi (V54 çekirdeği, tam yapılandırma)

## Veri sınırı [Kesin]
Sunucudaki tek gerçek metinler: Pride and Prejudice (pg1342) + Alice (pg11) = **151 062 token, 7 695 farklı kelime**. Bölme: 500'lük bloklar, %80 eğitim / %10 val / %10 test; sözlük yalnız eğitimden. Eğitimde **6 721 farklı kelime** var → gerçek sözlük tavanı **6 722** (<unk> dahil). 10 000 / 20 000 / 50 000 / 100 000 için gerçek veri yok; sahte kelime üretilmedi, bu basamaklar **çalıştırılmadı**. Daha geniş bir veri kümesi sunucuda bulunamadı (yalnızca bir MNIST arff'si var, metin değil).

## Sonuçlar (aynı eğitim kümesi: 121 000 eğitim tokenı, 120 516 örnek; V54 çekirdeği `tek_hamle_v54`, 4 iş parçacığı)
| Sözlük | Eğitim tokenı | Model hazır süresi | Tepe RAM | Tamamlandı mı? | Val CE / PPL (yalnız kendi sözlüğünde) |
|---|---|---|---|---|---|
| 2.000 (2001) | 121 000 | 9,8 s | 1,72 GB | Evet | 4,619 / 101,3 |
| 5.000 (5001) | 121 000 | 21,8 s | 3,48 GB | Evet | 5,043 / 154,9 |
| 6.721+<unk> (6722) | 121 000 | 33,8 s | 4,54 GB | Evet (gerçek veri tavanı) | 5,327 / 205,8 |
| 10.000 / 20.000 / 50.000 / 100.000 | – | – | – | Çalıştırılmadı: gerçek kelime yok | – |

Doğrulama: her sözlük için ağırlıklar ayrıca dökülüp kontrol edildi — WC ve WF **tümüyle sonlu** (2001: 86,9 M + 1,0 M; 5001: 286,0 M + 2,6 M; 6722: 412,6 M + 3,4 M eleman; max|w| ≤ 0,41), V53H_REZIDU denetimi her koşuda kabul, çıkış kodu 0. Test PPL ölçülmedi (`V53H_TEST=0`). PPL değerleri farklı sözlükler için birbirleriyle karşılaştırılamaz.

## Notlar
- Süreler VM yükü altında: aynı V54 ikilisi referansta 3,63 s, bu oturumda 2001 için 9,8 s; yani mutlak süreler referansla kıyaslanamaz, yalnız bu tablo içinde (aynı yük) kıyaslanır. Dökümlü ikinci koşular (23 / 70 / 50 s) disk I/O ve gürültü nedeniyle daha yavaştı; tablo dökümsüz ilk koşulardan.
- Ölçekleme: model süresi V ile yaklaşık doğrusal-üstü (2001→6722: ×3,4 sözlük, ×3,5 süre, ×2,6 RAM). Kelime haznesi büyüklüğü ile eğitim tokenı ayrı: token sabit 121 000, yalnız sözlük büyüdü; V büyüdükçe çoğu kelime 1–2 kez görüldüğü için PPL kötüleşiyor (veri kıtlığı, algoritma hatası değil — [Muhtemel]).
- Başka görev eşzamanlı çalışmadı; kaynak çakışması yok. Mevcut kaynaklar değiştirilmedi.
- Dosyalar: `hazirla_v58.py` (veri), `kos.sh`/`kos2.sh`, `ham/`.

**10 dakikada başarıyla eğitilen en büyük kelime haznesi: 6 722 kelime (gerçek veri tavanı).**
**Toplam işlenen gerçek eğitim tokenı: 121 000 eğitim tokenı / koşu (3 koşu: 363 000; farklı kelime 6 721).**
**En büyük başarılı modelin hazırlanma süresi: 33,8 saniye.**
