# TEK HAMLE V42 — Gruplu çözümü doğrula ve güvenli şekilde aktif et

Kod: `cpp/tek_hamle_v42.cpp` (V41 kopyası; V40/V41 değişmedi). Ham çıktılar: `v42_ham/` (`log_*`, `time_*`, `karsilastirma42.txt`, `karsilastir42.py`). Koşul: V40 tek F×512 + bağlantılı havuz (β=0.9), C0 tabanı, intercept açık (`V37_BIAS=1`). Ridge formülü, F, havuz, PPL hesabı değişmedi.

## 1) Intercept nerede, gruplu çözüm onu koruyor mu?
- **Sabit-1 sütunu C bloğunda değil, F bloğunun 0. sütununda** (F1'in ilk sütunu). Yani `A_CC` (gruplu çözülen matris) intercept'i içermez.
- İntercept ile kelime grupları arasındaki bağ `A_CF`'nin 0. sütunudur: `ACF[a][0]` = kimlik a'yı içeren eğitim satırı sayısı. Bu sütun tüm gruplarda doludur, yani intercept farklı kelime gruplarını **birbirine bağlar**. Bu bağ atlanmıyor: gruplu yol `X = A_CC⁻¹ A_CF` hesabını `A_CF`'nin **tüm** sütunlarıyla (0. sütun dahil) grup grup yapar, grupları birbirine bağlayan terim `A_FF − A_FC A_CC⁻¹ A_CF` Schur tamamlayıcısı (513×513, yoğun) olarak aynen çözülür. Yalnız `A_CC⁻¹` blok-köşegen sayıldığı için blok dışı hesap yapılmaz; bu bir yaklaşım değil, `A_CC`'nin gerçek yapısıdır.
- Kanıt: (i) kodda blok dışı sıfır olmayan eleman sayısı 0 (V41_GRUP denetimi), (ii) her λ_C için grup çözümünün bağıl rezidüsü ~1e-15 (X'in 0. = sabit sütunu dahil kontrol edildi), (iii) yoğun yolla birebir aynı ağırlıklar (aşağıda).
- **Gruplu çözümün geçerlilik koşulu:** `A_CC = CᵀC+λI` bloklarının **grup içi** olması: her satır/sütun tek bir "son kelime" grubuna ait ve gruplar arası `CᵀC` girdisi sıfır. C0 tabanında (ID(w_{t−1}) + o kelimeyle biten sık çiftler) bu doğrudur. Bozulursa (ör. ID(w_{t−2}) ayrı sütun, hash çift kimliği, farklı baz) denetim ≠ 0 bulur.
- **Otomatik geri dönüş (yeni, V42):** (a) yapı denetimi sıfır değilse yoğun Cholesky; (b) çözümden sonra rezidü denetimi (Y0'da 6, X'te 4 sütun; X'te 0. sütun dahil) bağıl rezidü ≥ 1e-8 ise yoğun Cholesky'e dönülür; (c) taban C0 değilse (`V36_C0` kapalı) grup yolu hiç devreye girmez. Eski yoğun yol silinmedi (`V41_GRP=0` ile zorlanır).

## 2) Eşdeğerlik testleri (yoğun V40 yolu vs gruplu yol; aynı veri/aynı sistem)
3×3 (λ_C ∈ {10,20,100} × λ_F ∈ {300,3000,30000}) tohum 1'de tam; tohum 2'de 3 temsilî kombinasyon (tohum bağımsız değişkeni F yönlerini değiştirir; veri bölünmesi aynı). Tam tablo: `karsilastirma42.txt`.
| | sonuç |
|---|---|
| W_C bağıl Frobenius farkı | **1.5e-10 … 2.7e-9** (maks mutlak 1.5e-8; ||W_C||≈2) |
| W_F bağıl Frobenius farkı | **0 … 5e-12** |
| Doğrulama CE / test PPL / Top-1 / Top-5 | **12 kombinasyonun hepsinde birebir aynı** (ör. λ_C=100, λ_F=3000: 4.8753 / 124.92 / 0.1610 / 0.3673) |
| Tam sistem denklem rezidüsü ‖ZᵀZW+ΛW−ZᵀY‖/‖ZᵀY‖ | iki yolda da **7e-8 … 1.3e-7** (aynı mertebe, float gemm sınırı) |
| Grup çözümü iç rezidüsü | tümü 1e-8'in çok altında (örn. 1.9e-15), 12/12 "KABUL" |
Ayrıca rezidü hesabındaki bir **raporlama hatası düzeltildi**: önceki sürümlerde (V37–V41, intercept açıkken) `COZUM_REZIDUSU` sabit sütuna da λ_F uyguluyordu, oysa çözücü sabit sütuna λ_C uygular; bu yüzden 1.9e-2 gibi yanlış büyük değer görünüyordu. Doğru Λ ile rezidü 2.7e-7 … 1.3e-7. Çözücü değişmedi, yalnız ölçüm düzeltildi (V42'de).

## 3) Süre (aynı sunucu, 3 tekrar, λ_C=100, λ_F=3000; F+havuz+Gram + C-bloğu Cholesky/Y0/X/Schur hazırlığı + λ_F çözümü)
| Yol | tekrarlar (toplam) | medyan | min |
|---|---|---|---|
| Yoğun (V40) | 3.23, 3.00, 3.54 s | **3.23 s** | 3.00 s |
| Gruplu (V42) | 2.38, 1.83, 2.22 s | **2.22 s** | 1.83 s |
Aşama bazında Cholesky+Y0+X+Schur: yoğun 1.31–1.65 s → gruplu 0.42–0.83 s. F+havuz+Gram (0.74–0.91 s) ve λ_F çözümü değişmedi. Hızlanma ≈1.45× (medyan), ≈1.6× (min). V41'in 3.38→2.64 s medyanı ile tutarlı; sunucu gürültüsü ±0.5 s.

## Varsayılan
Doğruluk kaybı yok (ağırlıklar ≤3e-9 bağıl fark, PPL/Top-k birebir) ve hız kazancı var → **C0 tabanında gruplu çözüm artık varsayılan** (V42; `V41_GRP=0` yoğun yol). Güvenlik: yapı denetimi + rezidü denetimi + C0 dışı tabanda devre dışı. Sınır: yalnız bu baz için geçerli; yeni baz eklenirse yapı denetimi geçmedikçe aktif olmaz.
