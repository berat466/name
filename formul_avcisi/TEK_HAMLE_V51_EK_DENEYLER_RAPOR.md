# TEK HAMLE V51 — Eksik deneylerin tamamlanması (keşif niteliğinde)

Kod: `cpp/tek_hamle_v51b.cpp` (V51 + OOF havuz grafı + eşleme tohumu + sabit-α çıktısı), `cpp/tek_hamle_v51c.cpp` (V51b + yalnız-kapı karıştırması). V50/V51 kodları dokunulmadı. Ham: `v51_ek_deneyler_ham/` (`ozet_agg.txt`, `ozet_sizinti.txt`, `agg.py`, `t.py`, `L_*`, `G_*`, `M_*`, `R_*`, `S_*`, `SG_*`). Hepsi: model tohumu 1 (tek), tek bölünme; **eşleme tohumu yalnız bölüm 3'te değişir**. Seçimler yalnız doğrulama CE ile; test yalnız seçilen/rapor edilen ayarlarda okundu. Çok sayıda ayarın aynı doğrulama bölünmesinde seçilmesi nedeniyle bu sonuçlar **keşif niteliğindedir**.

## 1) Sızıntı denetimi ve TEMİZ referans
- **ρ=0 olduğunda eski öğrenme sinyali kapalı:** ρ=0 ile `RIDGEK` açık/kapalı koşularında B-satırı birebir aynı (4.8316 / 119.30) → V50'nin 119,30'u ρ-sinyalinden etkilenmez.
- **V50'de kalan olası sızıntı: havuz grafı.** Eğitim satırlarının F özellikleri, grafın komşuluk ağırlıklarıyla karıştırılıyor ve graf **tüm eğitim hedef sayımlarından** (satırın kendi etiketi dahil) kurulmuştu. Temiz kontrol (`V51_OOFPOOL=1`): eğitim özellikleri için 5 belge-bazlı katta, satırın katı sayımları ve grafı dışarıda bırakılarak kurulan graf; doğrulama/test tam eğitim grafıyla.
- **Sonuç (V50 sistemi, ρ=0, α doğrulamayla 0.2):**
| | doğr. CE | test PPL | α=0 doğr. CE |
|---|---|---|---|
| Eski (sızıntılı graf) V50 | 4.8316 | 119.30 | 4.8655 |
| **Temiz (OOF graf) V50 — yeni referans** | **4.8318** | **119.71** | 4.8749 |
Doğrulama CE hemen hemen aynı (+0.0002), test +0.41 PPL; α=0 modelinde (son karışımsız) etki +0.0094 CE. Yani graf sızıntısı V50'de sınırlı ama sıfır değil. **Temiz referans PPL = 119.71** (119.30 yeniden üretilmeye çalışılmadı; 119.30 yalnız sızıntılı graf koşusunda var). Bundan sonraki karşılaştırmalar temiz tabana göredir.
- **Eski V46/V49 ρ=−0.25 sinyali (α=0):** eski (LOO + tam graf, V49) 4.8400 / 120.43; OOF çekirdek sinyali + sızıntılı havuz: 4.8376 / 120.19; **tamamen temiz (OOF çekirdek + OOF havuz): 4.8425 / 120.94** (+0.51 PPL eskiye göre). ρ-sinyalinin sızıntı payı ≈0.5 PPL.

## 2) Karıştırılmış çekirdek kontrolleri (temiz havuz tabanı; kapı katsayıları sabit)
Üç koşul: **gerçek**, **kapı-karışık** (yalnız aşama 1–3 kapıları ve ρ-sinyali karışık çekirdekten; son karışım gerçek Q), **tam-karışık** (son karışım dahil karışık). Hücre: α(doğrulamayla) / doğr. CE / test PPL.
| Deney (katsayılar) | gerçek | kapı-karışık | tam-karışık |
|---|---|---|---|
| A — temiz V50 (kapı yok) | 0.2 / 4.8318 / 119.71 | – | α=0 / 4.8749 / 125.43 |
| B — F kapısı g_F=0.15 | 0.2 / **4.8242** / **118.91** | 0.2 / 4.8319 / 119.63 | α=0 / 4.8701 / 124.71 |
| C — kuvvet g_P=0.05 | 0.2 / 4.8319 / 119.76 | 0.2 / 4.8319 / 119.73 | α=0 / 4.8750 / 125.46 |
| D — g_F=g_P=0.05 | 0.2 / 4.8289 / 119.29 | 0.2 / 4.8314 / 119.61 | α=0 / 4.8732 / 125.14 |
| E — tüm aşamalar g=0.15 + ρ=−0.25 (K) | 0.1 / **4.8221** / **118.98** | 0.3 / 4.8907 / 126.91 | α=0 / 4.9518 / 135.85 |
- **Karışık kapılar kazancı yok ediyor:** B için gerçek 4.8242 vs kapı-karışık 4.8319 (≈ kapısız 4.8318); D 4.8289 vs 4.8314; E 4.8221 vs 4.8907 (karışık ρ sinyali ağır zarar). C (g_P) gerçekte de kapısızla aynı (4.8319 vs 4.8318): **kuvvet kapısının etkisi yok**.
- Tam-karışık sütunu, karışık son karışımın da kapatılması nedeniyle ayrı bir durumdur (α=0 seçildi; sabit α=0.2'de 137–152 PPL — yalnız karışık Q'nun zarar verdiğini gösterir).
- Her karşılaştırmada katsayılar sabit tutuldu (tablodaki değerler); karışık kontrol kendi α seçimini yaptı (kapı-karışık: α=0.2 / E için 0.3, tam-karışık: α=0).

## 3) Bağımsız kapı katsayıları (ρ=0, α doğrulamayla; hücre: doğr. CE (α) / test PPL; temiz taban)
| g_F \ g_P | 0 | 0.05 | 0.15 | 0.30 |
|---|---|---|---|---|
| 0 | 4.8318 (0.2) / 119.71 | 4.8319 / 119.76 | 4.8319 / 119.75 | 4.8320 / 119.74 |
| 0.05 | 4.8288 / 119.27 | 4.8289 / 119.29 | 4.8292 / 119.35 | 4.8295 / 119.43 |
| 0.15 | **4.8242 / 118.91** | 4.8243 / 118.95 | 4.8244 / 119.08 | 4.8250 / 119.35 |
| 0.30 | 4.8262 / 119.37 | 4.8264 / 119.40 | 4.8267 / 119.47 | 4.8283 / 119.73 |
**Doğrulamada en iyi: g_F=0.15, g_P=0** (iç nokta). Etki neredeyse tümüyle F kapısından: g_P=0→0.30 ile doğrulama CE hep aynı veya 0.0001–0.0033 kötü. α=0.2 hep seçildi. ρ/α çapraz ızgarası yetişmedi: ρ=−0.25 yalnız E (g=0.15) için ve α∈{0…0.6} ızgarasıyla, ρ×α×g tam çaprazı yapılmadı. Eşdeğerlik: g_F=g_P=0 ⇒ temiz V50 (4.8318) ve kapısız eski kodla bit-eşdeğer koşulları (V51 A-kontrolü) doğrulandı.

## 4) Eşleme tohumları (64→512 sabit rastgele eşleme; model tohumu 1 sabit; E, g=0.15, ρ=−0.25, α doğrulamayla)
| eşleme tohumu | α | doğr. CE | test PPL | Top-1 / Top-5 |
|---|---|---|---|---|
| 1 | 0.1 | 4.8227 | 118.70 | 0.1618 / 0.3685 |
| 2 | 0.1 | 4.8236 | 119.29 | 0.1618 / 0.3677 |
| 3 | 0.1 | 4.8237 | 119.07 | 0.1596 / 0.3675 |
(V51'in 777 eşlemesi: 4.8221 / 118.98.) Üç eşlemede doğrulama CE 4.8227–4.8237 (aralık 0.001) ve temiz referans 4.8318'e göre −0.0081…−0.0091; test 118.70–119.29 (ortalama 119.02, referans 119.71'e göre −0.7). **Kazanç belirli bir rastgele eşlemeye bağlı değil.** Model tohumları 2 ve 3 (veri-bağımlı F yönleri) bu sürede yetişmedi.

## 5) Süre ve bellek (CPU 4 vCPU, gürültülü)
Çekirdek (6 tablo + 6 graf + U) kurulumu **1.5–3.9 s** (ilk koşu 3.9 s, sonrakiler ~1.9–2.5 s); kapı/OOF-pool ek maliyeti ≈0.1–0.3 s. F+çekirdek+kuvvet+Gram hazırlığı 2.9–4.0 s; ağırlık çözümü (ρ) 0.17–0.24 s (ρ=−0.25 ridge-K: 0.2–0.9 s); logit (val+test) 0.12–0.3 s; son karışım+PPL 0.02–0.06 s. Tepe RSS 1.507 GB. Not: bu kodda çekirdek, kapılar kapalıyken de kuruluyor.

## Cevap (kanıtla)
- **V51 çekirdeği temiz tabanı geçiyor mu?** Doğrulama CE'de evet, tutarlı: F kapısı (g_F=0.15) 4.8318 → 4.8242 (−0.0076), E 4.8221 (−0.0097); testte 119.71 → 118.91 / 118.98 (−0.8). Kazanç **gerçek çekirdeğe özgü** (karışık kapılar kazancı sıfırlıyor), üç eşleme tohumunda yinelendi. Kazanç **F kapısından** geliyor; kuvvet kapısı etkisiz; ρ-sinyali eklenince (E) doğrulamada küçük ek kazanç (4.8242→4.8221), testte yok.
- **V50'nin yayımlanmış 119,30'unu geçiyor mu?** Kanıt zayıf: 119,30 sızıntılı grafla alınmıştı; temiz V50 119,71. Temiz karşılaştırmada B 118.91, E 118.70–119.29 (3 eşleme). Sızıntılı 119,30'a göre fark −0.4 (B) / −0.3 (E ortalaması) ve eşleme tohumuna göre belirsiz (118.70–119.29). Bu fark gürültü sınırında; **kesin üstünlük iddia edilmiyor.**
- Seçim riski: ~20 ayar (g ızgarası 16 + eşleme + α) aynı doğrulama bölünmesinde değerlendirildi; model tohumu 2/3 tekrarı yok; temiz referans yeni; α_okuma kapı sonrası yeniden ayarlanmadı.

## Yetişmeyen / eksik kalanlar
1. Model tohumu 2 ve 3 ile tekrar (yalnız tohum 1).
2. ρ × α × (g_F,g_P) tam çapraz ızgarası (yalnız ρ=0 için 4×4 g ızgarası; ρ=−0.25 yalnız E, g=0.15/0.05).
3. B/C/D için "tam-karışık" koşulları ayrı α seçimiyle var; sabit-α=0.2'de gerçek karşılaştırma `V50_SABIT` satırlarında ham veride.
4. V50'deki ρ-sinyalinin eski LOO tasarımının "tam temiz" yeniden kurulumu (yalnız çekirdek OOF'u temiz kabul edildi).
5. F kapısının atanh/tanh biçimi dışında alternatif kural denenmedi (yeni formül yasaktı).
