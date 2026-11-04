# V60: ölçeklenebilir hesaplama — sonuç: KISMİ (K-grafı kökten düzeltildi; Q-tablosu V² darboğazı açıkta)

Kod: `cpp/knn_sparse.h` (genel seyrek kNN çekirdeği), `cpp/tek_hamle_v60.cpp` (V54 + çekirdek; `V60_SPK=0` eski yoğun yol, `V60_KCHECK=1` iki yolu karşılaştırır), `cpp/test_knn_sparse.cpp` (matematiksel ölçekleme testi). V54/V59 kaynakları değişmedi. Veri: V59'un ortak veri kümesi (aynı kimlikler/ayarlar), hafif ayar, HASH modu açık, her çift art arda koşturuldu. **VM çok gürültülü** (aynı kod f2'de 12,9–33 s verdi): toplam süreler güvenilir değil, K-grafı süreleri güvenilir (10–40× farkla).

| Ölçüm | 2.000 | 5.000 | 10.000 | 50.000 matematik testi (sentetik) |
|---|---|---|---|---|
| K-grafı süresi, eski yoğun (6 tablo) | 0,54 s | 4,70 s | 40,66 s | — (yoğun 250 000 GF/graf) |
| K-grafı süresi, yeni seyrek | 0,18 s | 0,54 s | 0,98 s | 8,2 s (tek graf; 100k: 29,4 s) |
| kg2 (S hesabı) eski → yeni | 0,41→0,16 s | 4,26→0,43 s | 35,8→0,61 s | |
| S için işlem sayısı (6 tablo) yoğun → seyrek | 48 → 0,20 GF | 750 → 0,40 GF | 12 000 → 0,54 GF | 250 000 → 8,6 GF (100k: 2 000 000 → 30 GF, tek graf) |
| K-tablo+kapı (f2) eski → yeni | 1,38→0,89 s | 7,6→3,4 s | 52,4→12,9 s (sonraki koşularda 22–33 s: VM) | |
| Tam model hazır (gürültülü) eski / yeni | 15,2 / 20,0 s | 29,9 / 38,3 s | 102,6 / 46,9 s | Uygulanmaz |
| Tepe RAM | 1,9 / 1,8 GB | 4,3 / 4,3 GB | 9,0 / 9,0 GB (HASH açık; HASH'siz 6,5 GB) | seyrek kNN: 74 MB (50k), 143 MB (100k) |
| Val CE eski / yeni | 4,503665 / 4,503665 | 5,079437 / 5,079437 | 5,441946 / 5,441941 | Uygulanmaz |
| Matematiksel eşitlik | komşu listeleri 6/6 tabloda **aynı**, ağırlıklar ≤5e-8; WC ≤1,0e-7, WF ≤2,0e-7 fark (karma farklı) | WC ≤4,5e-8, WF ≤3,7e-8 | karmalar farklı, CE 5e-6 | çekirdek, aynı toplama sırasıyla yoğun naif hesapla **bit-eşit** (V=1500, 0/1500 fark) |
| Algoritmik yol | CSR + CSC seyrek çarpım, top-k (deger,indeks) | aynı | aynı | aynı (tek kod yolu) |

## Cevaplar
1. **Kaldırılan gereksiz hesaplar:** V×V×V yoğun sgemm (Nn·Nnᵀ; Nn %0,4–2 dolu), V×V yoğun S matrisinin tahsisi/yazımı/taraması (top-k her u için V eleman tarıyordu), V×V Nn matrisi. Yerine yalnız ortak sütunu olan (u,w) çiftleri toplanıyor; ortak sütunu olmayan çiftlerin S değeri tam 0 olduğundan top-k'da (değer, indeks) sırasıyla en küçük uygun indeksler dolgu oluyor → aynı küme/sıra. Yaklaşık komşuluk yok, top-k sırası değişmedi.
2. **Karmaşıklık:** eski O(V³) FLOP + O(V²) bellek (×6 tablo); yeni O(Σₖ nnz(sütun k)²) FLOP (ölçülen: 10k'da yoğunun %0,005'i), O(nnz) bellek + işçi başına O(V) tampon. Seyrek çarpım sonucu yoğunlaşabilir: sentetik testte dokunulan çiftler V²'nin %76–91'i, yine de maliyet katsayısı küçük (top-k dokunulan çifti tarar: O(dokunulan)); en kötü durumda bile yoğun O(V³)'e dönmez, çünkü iş Σ nnz² ile sınırlı.
3. **10.000'de 50,4 s → ?** K-grafı 40,7 s → 0,98 s; f2 (K-tablo+kapı) 52,4 → 12,9 s (aynı koşu çifti). Toplam model süresi ölçümü VM gürültüsüyle belirsiz (102,6 → 46,9 s; temiz kıyas yapılamadı).
4. **50.000/100.000:** Aynı çekirdek, aynı kod yolu, yoğun yola dönüş yok; **yalnız çekirdek matematik testi** (sentetik Zipf+küme bigram sayımı, nnz/satır ≈37): 50k tek graf 8,2 s/8,6 GF, 100k 29,4 s/30 GF, RAM <150 MB. Bu model eğitimi değildir. Tam model 50k+ için ayrıca V² yoğun Cn ve Qt (aşağıda) engeli var.
5. **Önceki hesapların yeniden kullanımı:** Bu görevde **uygulanmadı** (süre). Zemin: 6 OOF grafı aynı Cfull'dan bir fold çıkarılarak türüyor → komşu listeleri artımlı güncellenebilir; sözlük genişlemesinde yalnız yeni kimlikli satırlar/sütunlar değişir (V59: %13–22 örnek). Kaydı yok, doğrulaması yok.
6. **Hâlâ V ile büyüyenler:** (a) yoğun Cn (V×V float) ve Qt (V×V double) ×6 tablo: 10k'da ~4,8 GB, 50k'da ~120 GB → asıl engel; (b) "kq2 Qt tahsis" 14 s (6 tablo, sıfır-başlatma + sayfa hatası, tek iş parçacığı); (c) U yoğun Y0d+dgemm (3 s @10k, ∝mc·V); (d) çıkış W_C (mc×V); (e) Q/OOF n×V; (f) kNN'de Σ nnz² (veri yoğunluğuna bağlı). Gram/Schur/Cholesky F-tarafı (513) V'den bağımsız.
7. **Ağırlıklar ve kalite:** Bit-eşitlik **sağlanmadı**: sgemm'in dahili toplama sırası taklit edilemedi (Q=128…100000 denendi, hiçbiri sıfır fark vermedi; çok iş parçacıklı kuyruk sütunları). Komşu listeleri (index ve sıra) 2.001'de 6 tablonun tümünde aynı; ağırlık farkı ≤5e-8; Val CE 2.000/5.000'de 6 hane aynı, 10.000'de 4,5e-6 fark. Dolayısıyla "sayısal eşdeğer, bit-eşit değil". V60 ikilisinde seyrek yol **varsayılan açık** (V54 dosyası değişmedi); katı bit-eşitlik gerekirse `V60_SPK=0`.
8. **En büyük kalan darboğaz:** V×V yoğun Qt/Cn tabloları (altı kopya): bellek ve sıfır-başlatma. Çözüm yönü: Q satırı = seyrek parça + rank-1 (π) terimi olarak saklamak (q = a·seyrek + b·π); tüketici (kapı/proj) buna göre yeniden yazılmalı. 10k'da f2'nin ~14 s'si bu.

## Yetişmeyenler
Seyrek/rank-1 Qt, yoğun Cn'nin CSR'ye çevrilmesi, artımlı genişletme (2k→5k→10k kesintisiz devam), seyrek U'nun otomatik bağlanması (doğrulanmamış, açılmadı), temiz (gürültüsüz) toplam süre kıyası.
