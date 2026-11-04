# TEK HAMLE V51 — Tek merkezi öğrenme çekirdeği (SONUÇ: güvenilir kazanç yok; V50 korunuyor)

Kod: `cpp/tek_hamle_v51.cpp` (V50 kopyası; `V51=1`; V49/V50 değişmedi). Ham çıktılar: `v51_ham/` (`A_ref`, `B_gF*`, `C_gP*`, `D_*`, `E_all_g*`, `Es_shuf_*`, `seed*`, `env.sh`, `tablo51.py`, `seed_tablo51.txt`). Referans sistem V50: tek F×512, V49 kuvvetleri (A,(A−μ)²,(A−μ)³), kelime+çift kimliği, havuz β=0.9, gruplu Ridge, λ_C=85, λ_F=3000, V50 son aşama karışımı. **A: V50'nin 119,30 PPL'i yeniden üretildi** (doğrulama CE 4.8316, α=0.2); tüm kapılar kapalıyken birebir aynı. Tohum 1; tohum 2 ve 3 tekrarlandı.

## Merkezi çekirdek (bir kez hesaplanır, tüm aşamalar okur)
`K(x)=Q_havuz(·|w_{t−1})`: `K(w)=(1−b)P_w + b·Σ_v a_wv P_v` (b=0.9, kNN=10, havuz kosinüs ağırlıkları, unigram yedeği) tablosu, **bir kez** kurulan 6 tablo: eğitim satırı için 5 belge-bazlı (dizi/500-token belgesi, `(i/498)%5`) out-of-fold tablo — sayımlar **ve graf** satırın katı dışında kurulur — ve doğrulama/test için tam eğitim tablosu. Doğrulama/test etiketi çekirdeğe girmez; bilinmeyen doğru kelime hiçbir aşamada çekirdeğe girmez. Çekirdek ayrıca dört kopya hesaplanmadı: tablolar tek yerde, aşamalar buradan okur (F kapısı ve kuvvet kapısı aynı `U` matrisini paylaşır).
- **Aşama 1 (F kapısı g_F):** `s(K)=Σ_y K_y E(y)` (E: mevcut 64D kelime tablosu), eğitimden standardizasyon, **sabit** rastgele (eğitilmeyen, tohum 777) 64→512 eşleme → `U` (eğitim std≈1). `F_j' = tanh(atanh(F_j) + g_F·U_j)`.
- **Aşama 2 (kuvvet kapısı g_P):** A²/A³ taban merkezi satıra göre kaydırılır: `(h_j − μ_j − g_P·σ_j·U_j)^p` (aynı `U`). Yeni sütun yok, 512 korunur.
- **Aşama 3 (Ridge, ρ):** V49 sinyalindeki Q, çekirdeğin OOF dağılımı olur (`K(w_i)`, satırın kat tablosu), ρ=−0.25; satır parçalarıyla akıtılır.
- **Aşama 4 (son, α):** `P_son=(1−α)P_model+α·K(x)`, tam eğitim tablosu (V50 ile aynı).
Hiçbir eğitilebilir projeksiyon/ağ/optimizer yok; tüm kurallar sabit ve deterministik.

## Sonuçlar (doğrulama CE ile seçilen α ve kapı; tohum 1)
Kapı değerleri ∈ {0, 0.05, 0.15}. "Son" satırı: α ∈ {0,0.025,0.05,0.1,0.2,0.3,0.4,0.6}. Her hücre: doğrulama CE / test PPL.
| Deney | F | Kuvvet | Ridge | Son | seçilen kapı, α | doğr. CE | test PPL |
|---|---|---|---|---|---|---|---|
| **A — V50 referansı** | – | – | – | α=0.2 | – | **4.8316** | **119.30** |
| B — F + son | g_F | – | – | açık | g_F=0.15, α=0.2 (0.05: 4.8323/119.44) | 4.8319 | 119.73 |
| C — kuvvet + son | – | g_P | – | açık | g_P=0.05, α=0.2 (0.15: 4.8325/119.35) | 4.8321 | 119.31 |
| D — F + kuvvet + son | g_F | g_P | – | açık | g=0.05, α=0.2 (0.15: 4.8333/119.86) | 4.8325 | 119.44 |
| **E — tüm aşamalar** | g_F | g_P | ρ=−0.25 (K) | açık | **g=0.15, α=0.05** (g=0.05: α=0.1, 4.8296/119.22) | **4.8272** | 119.49 |
| F — tüm aşamalar, son kapalı | g_F | g_P | ρ=−0.25 (K) | α=0 | g=0.15 (g=0.05: 4.8340/119.80) | 4.8290 | 119.76 |
Referans dışı: V49 (ρ=−0.25, α=0): 4.8400 / 120.43.

## Tohum tekrarı (E, g=0.15 vs A; doğrulama CE / test PPL; α doğrulamayla)
| tohum | A (V50) | E (tüm aşamalar) | fark (doğr. CE / test) |
|---|---|---|---|
| 1 | 4.8316 / 119.30 | 4.8272 / 119.49 | −0.0044 / **+0.19** |
| 2 | 4.8345 / 119.84 | 4.8298 / 119.54 | −0.0047 / −0.30 |
| 3 | 4.8302 / 119.68 | 4.8317 / 119.52 | **+0.0015** / −0.16 |
Ortalama: doğrulama CE −0.0025, test −0.09 PPL; yön tohumlar arasında tutarsız (tohum 1 testte kötü, tohum 3 doğrulamada kötü).

## Karıştırılmış çekirdek kontrolü (g=0.15, tohum 1, tüm kapılar + ridge)
Tüm aşamalarda bozulmuş (permüte komşulu) çekirdek: son aşama için α=0 seçildi; E-eşdeğeri doğr. CE **4.9404, test 134.18** (gerçek çekirdek: 4.8272 / 119.49); ρ=0 satırı: 4.8642 / 123.62 (V50'nin α=0 satırı 4.8655 / 123.85). Yani çekirdeğin bilgisi gerçek (karışıkta ağır zarar), ama zarar/fayda dengesi küçük marjla.

## Kapıların bilgiyi tekrarlayıp tekrarlamadığı (anahtar bulgu)
Son aşama kapalıyken (α=0, ρ=0) doğrulama CE: A 4.8655 → g_F=0.05: 4.8613, g_F=0.15: **4.8535** (F kapısı tek başına modeli belirgin iyileştiriyor: test 123.85→122.86). Ama **son aşama açılınca** (α=0.2) aynı modeller 4.8316 → 4.8323/4.8319'a gelir: kazanç silinir. Aynı bilgi (kelime-havuzu dağılımı) F'de verilince son karışımın ihtiyacı azalıyor; α_opt 0.2 → 0.05'e düşüyor (E). Kuvvet kapısı tek başına neredeyse etkisiz (α=0: 4.8660/4.8663 ≈ 4.8655). Yani **bilgiyi birden fazla aşamada vermek ek öğrenme üretmiyor; aşamalar aynı bilgiyi paylaşıyor** (erişim ≠ kullanım: kapılar bilgiyi erişilebilir yaptı ama etkisi son aşamada zaten alınıyor).
Aşamalar ayrı ayrı: F kapısı (V49'a göre ρ=−0.25 ile): ρ-sinyaline ek 4.8400→4.8322 (g_F=0.15) ve çekirdek-ridge ile 4.8290 (F satırı) — son aşama kapalıyken bile tüm-aşama çekirdeği V49'u 4.8400→4.8290 (test 120.43→119.76) geçiyor, ancak V50'nin son-aşama yöntemi (4.8316 / 119.30) bunu zaten sağlıyor.

## Süre (CPU, 4 vCPU, gürültülü)
Çekirdek kurulumu (6 tablo = 6 graf V³ + 6 Q tablosu, özet ve U): **2.0–4.2 s** (kapılar kapalı olsa bile bu kodda hesaplandı; yalnızca kapı açıksa gerekli olurdu). F/kuvvet kapıları ayrıca ≈0.1–0.3 s (tanh/atanh ve U geçişi). Ridge için K-akıtma (E) V50/V46 pool-Q sinyali yerine geçti: ≈1–1.5 s. F+havuz+kuvvet+Gram hazırlığı kapılar kapalı 6.3 s (ilk koşu, kurulum dahil) / 3.8–5.3 s sonraki koşular; V50'nin ≈1.4–2.0 s hazırlığına göre çekirdek ≈+2–3 s ekliyor. Bellek: tablolar 6×32 MB + U (n×512×4 B ≈ 80 MB) ek.

## Yorum
- **Başarı iddiası yok:** E'nin doğrulama kazancı (−0.0044) ilk tohumda var, ama testte +0.19 PPL kötü; üç tohumda ortalama kazanç −0.0025 CE / −0.09 PPL ve tohum 3'te doğrulama da kötü. Şart olan tutarlı doğrulama kazancı sağlanmadı. V50 (119.30) en iyi güvenilir sistem olarak kalıyor.
- Soru "tek merkezi çekirdeğe bütün aşamaların erişmesi, yalnız son aşamada kullanmaktan daha iyi mi?": Hayır. Çekirdek yalnız F'de iyi (son aşama kapalıyken), yalnız sonda iyi (V50), ikisinde birlikte aynı seviyede (E ≈ A); bilgi tekrar ediliyor, kapılar birbirinin yerine geçiyor.
- Bilginin hangi aşamada kullanılacağı: ucuz ve etkili olan son aşama (α); F kapısı son aşama yoksa kullanışlı (123.85→122.86, 120.43→119.76 yolu), ama son aşamayı tamamlayıcı değil.
- Sızıntı: çekirdek eğitim satırları için belge-bazlı OOF + graf fold dışı; doğrulama/test etiketi yok. V50'nin havuz grafının (F havuzlamasında kullanılan, tüm eğitim sayımlı graf) dolaylı etiket bağımlılığı bu deneyde de **giderilmedi** (V50'den miras); yeni kapılar bunu kullanmaz.
- Sınırlar: kapı ızgarası {0,0.05,0.15}; tek bölünme, 3 tohum; birleşik kapı (g_F=g_P) dışında karışımlar denenmedi; U projeksiyonu sabit rastgele (başka eşleme denenmedi); karışık kontrol yalnız E için tam, B/C/D için çalıştırılmadı.
