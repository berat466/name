# AI Testi Raporu — .pmt ile yeni uygulama üretimi

## Düzen
- Üç yeni uygulama: etkileşimli oyun (Meteor Kaçışı), grafik (Dalga Karoları), veri işleme (Sensör Analizi). Mevcut örneklerin kopyası değil.
- Her görev için bağlamsız ayrı bir AI ajanı; elinde yalnızca `DIL.md`, `std.pmt`, iki örnek ve `GOREV.md` vardı. İlk deneme hiçbir komut çalıştırılmadan yazıldı.
- Doğruluk ölçütü: gizli bağımsız awk kehanetleri (`ai_testi/kehanet/`). Çıktı sayıları ve/veya kare baytları birebir tutmalı. Derlenmek yetmez.
- Test sırasında motora veya uygulamaya özel C++/.pma kodu eklenmedi. Yalnızca `.pmt` yazıldı.
- Referans çözümler (`ai_testi/referans/`) görevlerin dilde ifade edilebildiğini kanıtlamak için benim yazdığım, ajanlara gösterilmeyen çözümlerdir.

## Sonuç
| Görev | İlk deneme derleme | İlk deneme davranış | Hata sayısı | Düzeltme turu | .pmb bayt | Tanım sözcük / satır / bayt | Yorumlayıcı / native ms |
|---|---|---|---|---|---|---|---|
| Sensör Analizi (veri) | ok | TUTTU | 0 | 0 | 914 | 154 / 28 / 774 (referans; ajan çözümü aynı büyüklükte) | 6.4 / 5.2 |
| Meteor Kaçışı (oyun) | ok | TUTTU | 0 | 0 | 1105 (ref. 1097) | 181 / 38 / 771 (ref. 177 / 38 / 769) | 125 / 39 |
| Dalga Karoları (grafik) | ok | FARKLI | 1 | 1 | 848 (ref. 699) | 155 / 30 / 634 (ref. 135 / 25 / 539) | 72 / 15.5 |

Özet: 3 görevde ilk denemede derleme 3/3, doğru davranış 2/3. Üçü de en fazla 1 turda doğruya geldi. Toplam 1 davranış hatası.

## Hatanın kaynağı
- Dalga: ajan `kural v = v * kare` yazdı; `kare` sayacı 0 tabanlı ama DIL.md bunu söylemiyordu. Hata AI'nın değil **dil belgesinin** eksiği. Düzeltme: `kural t = kare + 1` ek kuralı (+2 satır).
- Meteor ve Sensör'de ajanlar belgede şu boşlukları bildirdi: kural sırası/üzerine yazma, `girdi` önceliği, etkileşim koşulunun çift başına değerlendirilmesi, `topla` boş küme davranışı, negatif sabit. Davranışları doğru tahmin ettiler; belge sonradan bu noktalarla tamamlandı.
- Meteor ajanının "ifadede `-8` yazılamıyor" gözlemi yanlış çıktı: negatif sabitler her yerde geçerli (denendi). Yani ajanın belirsizlik algısı da bir hata kaynağı olabilir.

## Çözüm kalitesi
- Meteor: ajan çözümü referansla aynı boyutta. Ajan ayrıca aynı oyunu etkileşimsiz, tamamen farklı biçimde ikinci kez yazıp kare zinciri özetini karşılaştırmış (aynı çıktı, 378 kare).
- Dalga: referanstan %20-45 büyük (ara alan zinciri), çalışma süresi aynı mertebe. Doğru.
- Native hızlanma: oyun 3.2x, grafik 4.7x, veri 1.2x (veri uygulaması kısa; süre başlatma ağırlıklı).

## Dürüstlük notları (sonucu zayıflatan noktalar)
- [Kesin] Yalıtım namus esasına dayalıydı: ajanlar aynı dosya sisteminde çalıştı, ben bakmadan kehanet dosyalarına erişmedikleri varsayıldı. Ajan raporlarında erişim izi yok, ama teknik olarak engellenmedi.
- [Kesin] Sensör görevi verilen veri-örneğine yakın; bu görevin başarısı en az bilgilendirici olandır.
- [Kesin] Ajanlar aynı model ailesi; benim tasarladığım dil ve görevlerle çalıştılar. Bu, "bağımsız kullanıcı" testi değil, "belgeyle yeni uygulama yazabilme" testidir.
- [Kesin] Dalga ajanı kendi doğrulaması için Python kullanmış (proje kuralına aykırı; teslim edilen dosya yalnızca `.pmt`, ölçümü etkilemez ama kural ihlali).
- [Muhtemel] 3 görev istatistik için az; "2/3 ilk deneme" kesin bir oran değildir.
- [Kesin] Bu test .pmt dilini ölçtü. AI'nın doğrudan sayısal parametre ürettiği test henüz yapılmadı (kullanıcı ayrı test olarak istedi).
- [Kesin] "Kodsuz" kısmi: .pmt bir dil; AI bu dilde metin yazıyor, tablo değil. Specializer ve makrolar motorun parçasıdır.

## Sonraki adımlar (onay bekliyor)
1. Doğrudan sayısal parametre üretimi testi.
2. Parametre gösteriminin küçültülmesi ve çalışma hızı (kullanıcı kararı: şimdilik ertelendi).
