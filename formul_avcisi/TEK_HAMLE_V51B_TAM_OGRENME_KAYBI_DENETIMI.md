# TEK HAMLE V51b — Tam öğrenme sınırları ve bilgi kaybı denetimi

**Kapsam/durum.** Teşhis ve ölçüm; hiçbir iyileştirme ana sürüme uygulanmadı. Ana referans `cpp/tek_hamle_v51b.cpp` (sha256 7f10dc17…, DEĞİŞTİRİLMEDİ). Ölçüm kancası `cpp/tek_hamle_v52.cpp` (V51b + `V52_DUMP` döküm, `V52_KNN`; model matematiği aynı). Ham ölçümler ve araçlar: `v51b_ogrenme_kaybi_denetimi/` (komutlar OKU_BENI.md'de). Veri: 2001 kelime, train 39 840 / val 4 980 / test 4 980; 5000 kelimelik veri **sunucuda yok**, o sonuçlar bu denetime girmedi. Referans yeniden üretildi: val CE 4,8242, test CE 4,7783, **test PPL 118,91**, Top-1 %16,12, Top-5 %37,03 (görevdeki 118,8969 ile aynı).
**Doğrulama:** V51b'nin analitik çözümünün Python kopyası C++ ile tam örtüşüyor (λ_F=1000/3000/10000 → 4,8569/4,8527/4,8673; α=0,2 son karışım val 4,8242, test 4,7783). Bütün blok/PC/sütun ablation'ları bu doğrulanmış kopyada yapıldı. Ayarlar (ölçüm konfigürasyonu): tek F×512, V49 (170×{A,A²,A³}+2A), kapı g_F=0,15, OOF havuz grafı, ρ=0, λ_C=85, λ_F=3000, okuma a=0,85 ε=1e-5, son karışım α=0,2. Tek tohum (model tohumu 1), tek bölünme — her sonuç bu sınırla geçerlidir. Validation seçim/teşhis için, test yalnız raporlama için kullanıldı.

## 0) ÖZET TABLO — kayıp mekanizmaları (önem sırası: val CE etkisi ve geri kazanılabilirlik potansiyeline göre)
Sınıf: A matematiksel, B sayısal, C öğrenme kapasitesi, D bilinçli güvenlik/genelleme kısıtı.
| Sıra | Kayıp mekanizması | Etkilenen bilgi | Ölçülen kayıp | Öğrenmeye etkisi | Geri kazanılabilir mi? |
|---|---|---|---|---|---|
| 1 | **Veri miktarı/seyreklik** (C/D): 50 000 tokenin ilk bölümü kullanılıyor, kitabın %59,6'sı (73 625 token) kullanılmıyor; kelime sayımı medyan 4 | kelime/çift kimliklerinin istatistiği | kimlik ağırlıkları gürültülü: λ_C=85'te kelime slotlarının %73,9'u ve çift slotlarının %77,0'ı faktör <0,1'e büzülüyor | train CE 3,79 vs val 4,85 (genelleme açığı 1,06) | veriyle evet (matematik dışı); matematikle kısmen |
| 2 | **C bloğunda ridge büzülmesi** (D, aşırıysa C): λ_C=85 tek değer | kelime (2001) ve çift (2000) kimlik ağırlıkları | etkin df: kelime 205,7/2001, çift 173,6/2000 (%10,3 / %8,7); çift slotu olan satırların %78,1'inde faktör <0,3 | C tek başına val CE 4,9552, F ile 4,8527; kimlik bilgisi büyük kısmı ridge dışında Q ile (son karışım) geri alınıyor: −0,0285 CE | kısmen: Q yolu zaten kullanıyor; sayıma duyarlı büzülme ölçülmedi |
| 3 | **Çıkış kırpması** `max(z,ε)` (A, tersinmez) | negatif/küçük skorlar | z<ε: %43,6 (z<0: %29,8); hedef z_y<ε: val %4,3 / test %4,0 / train %0,0 satır | bu satırlar val CE'nin %8,9'u (ort. CE 10,1 vs 4,6) | kısmen (okuma tanımı değişirse); ε=1e-5 val-seçilmiş |
| 4 | **Bağlam uzunluğu c=2 + çift kapsamı** (A/C): w_{t−3}… yok; çift slotu yalnız top-2000 çift | sıra/uzun bağlam; nadir çiftler | çift slotu olan satır: train %46,2 / val %43,4 / test %45,4; val çiftlerinin %33,3'ü, test %31,9'u eğitimde hiç yok | %53,8 satırda C yalnız kelime+sabit görüyor (w_{t−2} yalnız X/F'te) | slot kapsamı artırılabilir; ridge λ_C ile birlikte düşünülmeli |
| 5 | **Q tablosunda sıfır kütle / pürüzsüzleştirme yok** (C) | son aşamadaki havuz olasılığı | Q[w][y]=0: val %14,9 / test %14,4 / OOF-train %19,5 satır | α=0,2 karışım yine de −0,0285 CE verir; Q tek başına kullanılamaz (CE=∞) | evet (bilgi var, Q kısmen kullanılamaz) |
| 6 | **F'in kuyruk yönleri ezilmesi** (D) | Schur PC>100 sinyali (OLS enerjisinin %52,8'i) | df(3000)=30,7/512; 496/512 yön λ'dan küçük | **kayıp değil**: train–val sinyal kosinüsü PC 100–200: 0,095; 200–300: 0,048; 300–512: 0,039 (gürültü); kuyruğu tamamen kapatmak val 4,8528 (taban 4,8527) | gerek yok; açmak zararlı (λ×0,1: 4,8705) |
| 7 | **F ile C'nin örtüşmesi** (C: temsil çakışması) | F bilgisi | F enerjisinin %47,7'si C+sabit ile açıklanıyor; F varyansının %84,1'i son kelimeyle (hold-out %64,7) | F'in "yeni" bilgisi ilk ~35 PC'de; PR(Schur)=12,2 | örtüşme havuzun doğası (β=0,9) |
| 8 | **Top-k komşuluk (K=10)** (A/D) | graf: 948 aday kelime, 20 010 bağlantı | 1 053 kelime (%52,6; token payı %6,9) hiç komşu olamıyor; top-10 benzerlik kütlesinin %10,8'i; 70,5% kelimede 11. komşu ≥%98×10. | K=3: val 4,8576 (+0,034), K=5: 4,8311, K=10: 4,8242, K=20: 4,8232, K=40: 4,8243 | K 10–40 plato; sınır pratik olarak keyfi, kayıp küçük |
| 9 | **Tokenizasyon** (A, istemeden/bilinçli): egri apostrof sözcüğü bölüyor; noktalama atılıyor | 733 egri apostrof, 10 820 noktalama karakteri | 265 parça token (%0,53: `s` 253…); cümle sınırı bilgisi yok; 232 tür büyük/küçük birleşti; 79 rakam atıldı | küçük | evet (apostrof düzeltmesi), etki küçük |
| 10 | **UNK** (A/D bilinçli sözlük sınırı) | 2 000 sözlük dışı türler tek sınıfa | UNK token: train %4,86, val %8,10, test %9,04; 2 304 farklı tür tek sınıf | UNK satırları CE payı %4,6–5,3 (ort. CE 2,76 vs 5,01): ölçülen "PPL" kelime-düzeyi açık-sözlük PPL'den düşük | ölçü tanımı; sözlük büyütmekle |
| 11 | **V49 yön seçimi** (A/C) | 342 taban yön atılıyor (170 taban×3 sütun) | düz 512 yön val 4,8483 vs V49 4,8527 (Δ −0,0044 düz lehine); son aşamalı val 4,8200 vs 4,8242, test 119,00 vs 118,91 | V49 üstünlüğü ölçülemedi: kapı varken güçler bir şey eklemiyor | V49 yerine düz 512 de aynı test PPL |
| 12 | **OOF** (D) | kat-dışı sayımlar | sayımların %20'si kullanılamaz; OOF aday sayısı 948→813; son kelimesi OOF'ta hiç görülmeyen satır %0,99 | bilinçli güvenlik; kayıp değil | korunmalı |
| 13 | **Float32 / sayısal** (B) | Gram, ağırlık | Gram göreli hata 4–6e-8; ağırlık farkı 1,6–2,5e-7; κ(S+3000I)=20,8; κ(A_CC+85I)=475; doygunluk yok (max|F|=0,92 düz; V49'da tanh dışı ağır kuyruk %0,12) | ihmal edilebilir | gerek yok |

## 1) Tam hesaplama grafiği (kaynaktan; `tek_hamle_v51b.cpp`)
Ham metin (`/tmp/pg1342.txt`) → `metin_hazirla.py`: küçük harf, `[a-z]+('[a-z]+)?`, ilk 50 000 token, 100 blok×500, rastgele 80/10/10 bölme (bloklar bağlam taşırmaz), sözlük = eğitimin en sık 2000'i + `<unk>`, PPMI-SVD 64D gömme (yalnız eğitim) → `/tmp/v8data` (ids, X [e_{t−2},e_{t−1}], y).
V51b akışı (satır no `cpp/tek_hamle_v51b.cpp`):
1. `expand_s` (193): X=[e_{t−2}, e_{t−1}, D=e_{t−1}−e_{t−2}] → 192D float32; **D lineer bağımlı: X'in sayısal ranki 128**.
2. `make_w1` (148) + `features` (167): F1=tanh(γ·(X·W+b)); W sütunu = eğitimden rastgele seçilen, hedefi farklı iki satırın normalize farkı; b orta nokta + N(0,σ²) → 39 840×513 (sabit sütun 0).
3. V51 çekirdek bloğu (785): 5 OOF kat + tam eğitim için Hellinger-kosinüs grafı (`v51_graph_cn`, 507), Q tabloları (`v51_qtab`, 521), s(K)=ΣK_yE(y), sabit 64→512 eşleme, F kapısı `tanh(atanh(F)+g_F·U)`.
4. V39 havuz (`v39_graph` 498, `v39_mix` 530): H=(1−β)F+βΣa_wvF̄_v, β=0,9, kNN=10; eğitim satırları OOF grafla.
5. V49 dönüşümü (810): merkez/ölçek eğitimden; 170 taban×{A,(A−μ)²,(A−μ)³}+2A.
6. `v31solve` (553): ACC/ACF/RC/RF sayımları; gruplu Cholesky (`grp_ok`, 593–): (A_CC+λ_C I)⁻¹; Schur `S=A_FF−A_FC A_CC⁻¹ A_CF`; `(S+Λ_F)W_F=RHS_F−A_FC Y0`; `W_C=Y0−X W_F`. Gram `cross` (833) float32 sgemm 4096'lık parça→double.
7. `lin_table`/`pickLin` (308/578): P=(1−a)π+a·max(z,ε)/Σmax(z,ε), (a,ε) val ile seçilir; V50 bloğu (676): `buildQtab` (679), `fuse` (690): P_son=(1−α)P_model+αQ(w_{t−1}).
Her aşamanın girdi/çıktı/tip/tersinirlik özeti `v51b_ogrenme_kaybi_denetimi/` çıktılarında ve yukarıdaki tabloda.

## 2) Girdide kaybolan/reddedilen bilgiler (`a1_girdi.txt`)
- **Kullanılmayan metin:** kitap 123 625 token; yalnız ilk 50 000 (kitabın %59,6'sı kullanılmıyor).
- **Noktalama/özel karakter:** 10 820 karakter atılıyor → virgül/nokta/cümle sınırı bilgisi yok; sınır tokenları birbirine yapışıyor. Büyük harf 5 839, rakam 79; 232 tür büyük/küçük birleşiyor. **Egri apostrof (’) ascii olmadığından regex sözcüğü böler:** 733 apostrof, `s` 253, `t` 6, `m` 5, `re` 1 → 265 parça token (%0,53) — istemeden (A).
- **Sözlük/UNK:** eğitimde 3 848 tür; 2 000'i tutuluyor, 1 848 tür (eğitim tokenlarının %4,86'sı) UNK; sözlük son kelime sayımı 2; sayım ≤2: 556 kelime, <5: 1 052 (%52,6), <10: 1 478; yalnız 67 kelime ≥100; en sık 44 kelime tokenların %50'si. Val UNK %8,10 (378 tür), test %9,04 (400 tür); val'in %4,54'ü, test'in %5,68'i eğitimde hiç görülmemiş tür.
- **Bağlam:** c=2 sabit; w_{t−3} ve eski kelimeler X/C/F/Q'da hiçbir yerde yok; her bloğun ilk 2 tokenı hedef olamıyor (%0,40); blok sınırı bağlamı kesiyor. Sıra X'te korunuyor ([e_{t−2},e_{t−1}]); C'de sıra yalnız çift slotunda (w_{t−2},w_{t−1}) var, kelime slotu yalnız w_{t−1}. Q ve havuz yalnız w_{t−1}'e bağlı (sıra/w_{t−2} yok).
- **Çakışma:** gömme tablosunda normu 0 veya kosinüsü >0,95 olan kelime çifti yok → farklı kelimeler aynı gömmeye inmiyor. C temsilinde ise çift slotu olmayan %53,8 satır yalnız (w_{t−1}, sabit) ile temsil edilir (aynı C vektörüne inen farklı w_{t−2}'li satırlar).
- **Eğitimde hiç görülmeyen çift:** val %33,3, test %31,9 (eğitimde 19 646 farklı çift/39 840 satır).

## 3) Özellik üretimindeki sınırlar (`a5_ozellik.txt`, `a9_v49_atilan.txt`)
- **X rank 128/192** (D bağımlı) — F'in doğrusal kısmı ≤128 boyutlu alt uzayda.
- **Etkin rank:** düz F kovaryansı PR=2,2 (V52); V49 sonrası PR=11,3; Schur (C-sonrası) PR=12,2, rank(>1e-3·maks)=174/512. Yedeklilik: ölü sütun 0, |r|>0,99 çifti 0, |r|>0,9: 13 çift, medyan maks|r|=0,771.
- **Tür bazında (val CE, λ=3000):** yalnız A (172 sütun) 4,8616; A+A² 4,8557; A+A³ 4,8556; A²+A³ 4,8711; A² 4,8919; A³ 4,8837; hepsi 4,8527. Rastgele %75/%50/%25 sütun: +0,0023/+0,0081/+0,0246 (±0,001). Yani 3× sütun yalnız −0,0089 CE.
- **Sütun bazında marjinal katkı** (512 yeniden çözüm): çıkarılınca CE **iyileşen 164 sütun**, etkisiz (|Δ|<1e-5) 74, zararlı 274 (>1e-4: 62, >1e-3: 0); max tek sütun katkısı 3,1e-4, toplam 0,0086 → hiçbir sütun kritik değil; ~⅓ sütun hafif zararlı (gürültü).
- **V49 atılan yönler:** düz F+kapı, 512 ayrı yön (powersız): val 4,8483 (V49 4,8527); V49'un attığı 340 yön tek başına 4,8516 (ilk 172 yön 4,8616). Son aşamalı tam hat: düz 512 → val 4,8200, test **119,00** vs V49 4,8242 / 118,91 → **atılan yönlerde ve güç sütunlarında net bir kayıp/kazanç yok**.
- **F = kelime kimliği fonksiyonu:** F sütunlarının w_{t−1} ile açıklanan varyansı η²=0,841 (hold-out 0,647). Ağırlık katkısı yoğunluğu: ilk 128 sütun %34,5, son 256 sütun %38,9 (dağınık). Kapı (g_F=0,15) F'e Q-tabanlı bilgi ekler: düz F val 4,8323→4,8200 (son aşamalı).
- **Saturasyon:** düz F'te max|F|=0,92 (doygunluk yok); V49'da %0,12 değer >0,999 (ağır kuyruk, max 22; tanh doygunluğu değil).

## 4) Top-k ve komşuluk (`a6_q_topk_oof.txt`, `cpp_ham/K*.txt`)
Aday = eğitim sayımı ≥5: 948 kelime; 1 053 kelime (%52,6; eğitim token payı %6,9) hiç komşu olamaz. Benzerlik: 1. komşu medyan 0,596; 10.: 0,441; 11. (ilk atılan): 0,433; 30.: 0,362; top-10 tüm aday benzerlik kütlesinin %10,8'i (token-ağırlıklı %5,4). 10.–11. fark medyan 0,0033; kelimelerin %70,5'inde 11. komşu 10.'nun %98'inden büyük (sınır keyfi). Farklı komşu listesi 1 670/2 001; en büyük aynı-liste grubu 57 kelime; 434 kelime aynı listeyi paylaşıyor. Komşu olarak 935/948 aday kullanılıyor. Top-k dışındaki ilişkiler başka bir yerde kullanılmıyor (graf yalnız top-K saklar).
K etkisi (son aşama, Python; α=0,2 val CE): K=1 4,8536, 3 4,8303, 5 4,8234, 10 4,8242, 20 4,8240, 40 4,8257, **tüm 948 aday 4,8573**. Tam C++ hat (havuz+son aşama birlikte): K=3 4,8576/test 122,03; 5 4,8311/121,39; 10 4,8242/118,91; 20 4,8232/119,22; 40 4,8243/120,51. **Top-k bilgi kaybettirmiyor, gürültü azaltıyor** (tüm adaylar kötü). Maliyet: graf 2·V³≈16 GFLOP + sıralama ≈0,2–0,8 s; kayıp küçük.

## 5) OOF ve Q (`a6`)
5 kat (blok index mod 5; kat başına 7 968 satır): her eğitim satırı için sayımların %20'si ve graf adayları (948→813 ort.) kat dışında; son kelimesi OOF'ta hiç görülmeyen satır %0,99. Q'da pürüzsüzleştirme yok: Q[w][y]=0 olan satır OOF-train %19,5, val %14,9, test %14,4 (Q tek başına CE=∞; fusion modelle çalışıyor). OOF bilinçli güvenlik (D); kayıp olarak sınıflandırılmadı.

## 6) Ortak havuz ve blok katkıları (`a4_cozum.txt`)
| Model | train CE | val CE | test CE |
|---|---|---|---|
| yalnız C (kelime+çift+sabit) | 3,8532 | 4,9552 | 4,8971 |
| yalnız F+sabit (λ_F=3000) | 4,8085 | 5,0225 | 4,9737 |
| **C+F (λ_F=3000)** | 3,7918 | **4,8527** | 4,8104 |
| C+F λ_F=300 / 1000 / 10000 / 30000 | 3,8380/3,7889/3,7611/3,7752 | 4,8836/4,8569/4,8673/4,8942 | 4,8414/4,8146/4,8252/4,8510 |
| C+F cezasız (λ_F=1e-3) | 4,0506 | 5,1198 | 5,0716 |
F|C katkısı −0,1025 CE, C|F katkısı −0,1698. F enerjisinin %47,7'si C+sabit ile açıklanıyor (iz(S)/iz(A_FF)=0,5232).

## 7) Analitik çözümün sınırları (`a4`)
Schur spektrumu: λmax 5,9e4, medyan 25,2, min 0,17; df(300)=101,2, df(3000)=30,7, df(30000)=7,0; ezilen (λ_i<λ_F) yön sayısı 496/512. **Önceki V52'deki "kuyrukta %26 sinyal = kayıp" yorumu düzeltildi:** OLS sinyal enerjisi ilk 35 PC %34,8, ilk 100 %47,2, ilk 300 %71,2; ancak eğitim–doğrulama sinyal vektörü kosinüsü PC 0–10: 0,887; 10–35: 0,373; 35–100: 0,267; 100–200: 0,095; 200–300: 0,048; 300–512: 0,039. Kuyruğun hedef ilişkisi **genellemiyor**; kuyruğu tamamen kapatmak (ilk 100/200 PC) val CE'yi değiştirmiyor (4,8528/4,8525 vs 4,8527), ilk 35 PC +0,005; kuyruk λ çarpanları ×0,3/0,1/0,03/0,01 (ilk 100 PC sabit): 4,8542/4,8705/4,9261/4,9964. Yani **ridge'in ezdiği yönler çoğunlukla gürültü**, bastırma genelleme için yararlı (D).
F ağırlıkları: |W|<1e-3·maks olan öğe %28,1; satır normu medyan/maks 0,0063/0,0101. C ağırlıkları: kelime slotu satır normu medyan 0,0229, çift slotu 0,0266.

## 8) Çıkış (`a8_cikis_baglam.txt`)
| | train | val | test |
|---|---|---|---|
| z<0 / z<ε oranı | %30,3 / %43,1 | %29,8 / %43,6 | %30,1 / %44,1 |
| hedef z_y<ε satır | %0,0 | %4,3 | %4,0 |
| kırpmayla eklenen kütle (ΣmaxΣ−Σz) / negatif kütle | 0,030 / 0,023 | 0,027 / 0,019 | 0,027 / 0,019 |
| CE (okuma) | 3,7918 | 4,8527 | 4,8104 |
| Top-1 / Top-5 (ham z) | 0,1650/0,4372 | 0,1506/0,3554 | 0,1627/0,3697 |
| Top-1 / Top-5 (okuma P) | 0,1640/0,4240 | 0,1516/0,3560 | 0,1635/0,3701 |
Kırpma+unigram karışımı argmax'ı %2,2–2,6 satırda, top-5 kümesini ~%22 satırda değiştiriyor ama doğruluğu düşürmüyor. z_y<ε satırları val CE'nin %8,9'u (satır payı %4,3). Okuma duyarlılığı: a=1,0 (unigramsız) 4,8992 (+0,046); ε=1e-3: 5,38; ε=1e-7, a=0,85: 4,8580. **Son karışım P_son (α=0,2): val 4,8242 (Top-1 0,1510, Top-5 0,3558), test 4,7783 (0,1612/0,3703); argmax %10,1–10,7 satırda P_model'den değişiyor.** UNK hedefli satır val %8,1: CE 2,76 (diğer 5,01), CE payı %4,6, UNK Top-1 %46,2. P1/P2<1,05 (ayırt edilemeyen) %8,5; val'de top-5'te hiç görünmeyen sınıf 1 695/2 001 (%84,7). Kırpma öncesi/sonrası CE: kırpmasız tanım yok (negatif olasılık); duyarlılık yukarıdaki taramadır, mevcut değerlendirme tanımı değiştirilmedi.

## 9) Soruların cevapları
- **Model hangi bilgiyi hiç kabul etmiyor?** w_{t−3}…, noktalama/cümle sınırı, kitabın %59,6'sı, sözlük dışı 2 304 tür (UNK), eğitimde görülmeyen çiftlerin (%32–33) kimliği, çift slotu olmayan %53,8 satırda w_{t−2}'nin C'de kimliği.
- **Hesaplayıp sonradan silen?** 342 taban yön (V49; net kayıp yok), Top-k dışı aday ilişkileri (K=10; kayıp küçük/yararlı), kırpma (z≤ε), PC>100 yönleri (gürültü).
- **Koruyup kullanamayan?** C'de kelime/çift kimlikleri (df %10), Q'daki seyrek P_w (sıfır kütle %14,9); F'in C ile örtüşen %47,7'si.
- **Regularization neyi bastırıyor?** Düşük sayımlı kimlik ağırlıkları ve F kuyruğu; ikincisi gerçekten gürültü, ilkinin ne kadarının sinyal olduğu **ölçülemedi** (Q yolu kısmen geri alıyor).
- **Gereksiz/tekrarlı:** sütunların ~⅓'ü hafif zararlı (164/512), A²/A³ kapılı düzende üstünlük sağlamıyor, F'in %84'ü kelime kimliği fonksiyonu (C ile örtüşüyor).
- **Öğrenmeyi gerçekten kötüleştirenler:** çıkış kırpması (z_y<ε satırları CE'nin %8,9'u), Q'da sıfır kütle, veri azlığı, tokenizasyon hataları (küçük).
- **Kaldırılması yalnız gürültü artırır:** F kuyruk ridge'i, OOF, Top-k'nın tamamını açmak, λ_F düşürmek.
- **En yüksek potansiyelli üç matematiksel iyileştirme (uygulanmadı; ölçüm destekli sıra):** (1) **Q yolunda seyreklik/sıfır kütle** — Q[w][y]=0 val satırlarının %14,9'unda ve son karışım zaten −0,0285 CE verirken Q pürüzsüz değil; geri kazanılabilir bilgi burada. (2) **Kimlik (C) bloğunda sayıma duyarlı büzülme** — tek λ_C=85 ile df %10 kalıyor, rare kimliklerin bir kısmı sinyal olabilir (kısmen Q ile telafi); Schur/Ridge yolunda test edilmeli. (3) **Çift/bağlam kapsamı** — %53,8 satırın çift slotu yok, val çiftlerinin %33'ü görülmemiş; kapsamın genişletilmesi (düşük sayımlı çiftlerin Q/n-gram tipi geri düşüşle kullanılması) potansiyel taşıyor. Matematik dışı ama en büyük kaldıraç: **kullanılmayan %59,6 metin** (veri 2,5×).

## 10) Sınırlar / ölçülemeyenler
Tek veri (2001 kelime), tek tohum, tek bölünme; 5000 kelime yok. Okuma kırpmasının kırpmasız kontrafaktüeli tanımsız (negatif olasılık). Kimlik büzülmesinin gerçek sinyal payı (C bloğunda per-slot genelleme) ölçülmedi. Python kopyası kapı/pool dahil F'i C++ dökümünden aldı; F üretimi (`make_w1`) için özel ablation yapılmadı. Test yalnız raporlandı, hiçbir seçimde kullanılmadı.
