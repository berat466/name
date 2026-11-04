# Tek hamle V14 raporu — cross-entropy odaklı tek çözüm `W=(L·FᵀF+λI)⁻¹Fᵀ(T−P₀)`

Branch: `claude/formul-avcisi`. Referans: `TEK_HAMLE_V13_RAPOR.md`. V8–V13 dosyaları değişmedi (V14 yeni dosyada). Süre: 10 dk sınırı içinde. Kural: gradyan/Adam/backprop/RLS/epok/tekrarlı güncelleme yok; tek Cholesky çözümü.
Kod: `cpp/tek_hamle_v14.cpp`, `metin_kalibrasyon_v14.py` (V9 kalibrasyonunun V14 skor dizini kopyası). Ham loglar: `v14_ham/v14log/`.

## Açık cevap
**Hayır — bu CE-odaklı tek çözüm, V12 ridge + V9 kalibrasyonundan çok daha kötü sonuç verdi; unigramı yalnız çok az geçti.** Test CE 5,57–5,59 nat (PPL 263–266) vs V12 ridge+kalibrasyon 5,38 nat (PPL 217) ve trigram 4,92 (PPL 137).

| Model (tohum 1, c=2) | H | test CE (nat) | PPL | top-1 | top-5 | ağırlık üretimi sn |
|---|---|---|---|---|---|---|
| unigram (W=0, yalnız b) | – | 5,639 | 281,2 | 0,0904 | 0,2255 | – |
| trigram (interpolasyonlu) | – | 4,923 | 137,4 | 0,1600 | 0,3616 | – |
| V10/V12 ridge + V9 kalibrasyonu | 256 | 5,423 | 226,6 | 0,1406 | 0,2791 | ≈ 0,3 |
| V10/V12 ridge + V9 kalibrasyonu | 512 | 5,379 | 216,8 | 0,1492 | 0,2900 | ≈ 0,4 |
| **V14 doğrudan CE yaklaşımı** (ham: softmax(FW+b)) | 256 | **5,585** | **266,4** | 0,1054 | 0,2255 | 0,18 |
| **V14** (ham) | 512 | **5,573** | **263,1** | 0,1088 | 0,2255 | 0,37 |
| V14 + V9 kalibrasyonu (adil kontrol) | 256 / 512 | 5,585 / 5,573 | 266,4 / 263,1 | aynı | aynı | – |

(V10/V12 satırları önceki raporlardaki aynı veri/bölme sonuçları: V12 A H=512 tohum 1 ve V10 `fark` H=256 tohum 1; yeniden çalıştırılmadı. Unigram ve trigram V8/V9 raporlarından.)

- [Kesin] V14, **unigramın üstünde ama az**: H=512'de test CE 5,639 → 5,573 (−0,066 nat, PPL −%6,4); top-1 %9,0 → %10,9. top-5 unigramla özdeş (%22,55) → düzeltme, en olası 5 adayı değiştirmeyecek kadar küçük.
- [Kesin] **Adil kalibrasyon kontrolü**: V9 kalibrasyonunu (α, T ızgaraları, yalnız doğrulamada) V14'e de uyguladım: doğrulama, her iki H için α=1, T=1'i (yani formülün kendisini) seçti; kalibrasyon ek kazanç vermedi. V14'ün çıktısı zaten olasılık ölçeğinde; ridge'in aksine kalibrasyona ihtiyacı yok, ama bu onu ridge+kalibrasyonun düzeyine getirmiyor. (C++ içinde de küçük bir kalibrasyon ızgarası (T 0,5–3, α 0,5–1) doğrulamada tarandı: yine α=1, T=1.)
- [Kesin] **Sonucun ne kadarı yalnız unigram bilgisinden?** Unigram (W=0) CE 5,639, PPL 281,2. V14'ün unigrama göre kazancı (−0,066 nat, +1,8 puan top-1) tamamen `W` katkısı; yani W bir miktar bağlam bilgisi taşıyor, ama V12 ridge+kalibrasyonun kazancının (−0,26 nat) yalnız ≈ %25'i. Ham ridge çıktısının (kalibrasyonsuz) PPL'i 521 idi; V14 kalibrasyonsuz 263: ham skor kalitesinde büyük iyileşme, ama kalibre ridge'in gerisinde.

## Yöntem
- Unigram: `P_uni` = eğitim sayımlarından add-1 (V9 ile aynı); `b=log P_uni`; `P₀ = softmax(b) = P_uni` her eğitim satırına uygulanır (`T−P₀` her satırda `onehot − P_uni`).
- `F`: V12'nin `ref` özelliği (yoğun gömme + fark, K=192), `F=tanh(γXW₁+b₁)` bias sütunu **hariç** (H sütun); `W₁` çift-yön yöntemiyle (V3).
- `W = (L·FᵀF + λI)⁻¹ Fᵀ(T−P₀)`, **L=0,5**, Cholesky (H×H, 256 satırlık FP64 birikimli `FᵀF`); `Fᵀ(T−P₀) = FᵀT − (Σ_i f_i)P_uniᵀ` (toplanabilir istatistikten; yoğun `T−P₀` matrisi oluşturulmadı).
- Tahmin: `softmax(FW + b)`. Son model eğitim+doğrulama istatistikleriyle, test için `b` eğitim+doğrulama unigramından (V9 protokolü).
- Seçim yalnız doğrulamada: γ ∈ {1, 4}, λ ∈ {10⁻³, 10⁻², 10⁻¹, 1, 10, 100, 1000}; ölçüt ham doğrulama CE (T=1, α=1) ve ayrıca mini kalibrasyon ızgarasıyla kalibre CE. Seçilen: γ=1, λ=10⁻³ (her iki H için). **Not:** λ ızgarasının alt ucu (10⁻³) seçildi, ama CE 10⁻³ → 10⁻² → 10⁻¹ arasında 5,6319 → 5,6320 → 5,6326 (plato); daha küçük λ anlamlı kazanç vermez.
- Test yalnız sonda; L yalnız 0,5 (başlangıç değeri), ayarlanmadı.

## Süre ve RAM
Ağırlık üretimi (W₁ + istatistik + 1 Cholesky): H=256'da ≈ 0,18 sn, H=512'de ≈ 0,37 sn (V12'nin ≈ 0,4 sn'sine eşit/biraz düşük; H×H sistem, bias sütunu yok). Ayar araması dahil toplam: 6,3 sn (H=256) ve 9,1 sn (H=512); tepe RAM 249 MB ve 377 MB (skor dökümleri dahil).

## Neden ridge'den kötü (yorum)
- [Muhtemel] Tek noktadaki (başlangıç `P₀` = unigram'da) ikinci derece üst-sınır, CE'nin eğriliğini `L·I` sabitiyle yaklaşıkladığı için, dağılımın uçlarındaki (nadir token) kaybı hesaba katmıyor; ridge ise skorları sonradan V9 kalibrasyonuyla (unigram karışımı + keskin softmax, T≈0,01) yeniden ölçekliyor, bu da seçici bir "keskinlik" katıyor. V14'te `W` küçük adım gibi davranıyor: logit = `log P_uni + FW` yakın `log P_uni`. Bunu ölçmedim; yalnız gözlem (top-5 unigramla özdeş) ile uyumlu.
- [Muhtemel] L=0,5 ve λ plato değerleri, adım ölçeğini sabitliyor; L'yi büyütmek (daha küçük adım) ya da küçültmek denenmedi.

## Sınırlar
- Tek tohum (1), tek L, iki γ; 3 tohum doğrulaması yapılmadı (10 dk sınırı).
- V10/V12 satırları önceki raporlardan; bu oturumda yeniden koşulmadı.
- V8'in sıcaklık-ızgarası satırı (`V8_alpha1_V8_T_izgarasi`, T≤0,2) V14 logitleri için anlamsız (T=1 ölçeğinde); `kal.txt`'te görünür, yok sayın. Anlamlı olan `V9_*` satırları (T=1 seçildi).
- İki çıktı adı (`ham`/`kal`) aynı konfigürasyonu seçti.

## Komutlar (gerçek)
Derleme: `g++ -O3 -march=native -fopenmp -o tek_hamle_v14 tek_hamle_v14.cpp`. Çalıştırma: `./tek_hamle_v14 2 H 1` (H=256, 512); kalibrasyon: `python3 -I metin_kalibrasyon_v14.py cift_ham 2 H 1`. Ön hazırlık: `uni_tr.f64`, `uni_trva.f64` (unigram olasılıkları, V9 ile aynı sayımlar) küçük bir Python parçasıyla `/tmp/v8data`'ya yazıldı. Ölçüm (`manifest.tsv`):

- `./tek_hamle_v14 2 256 1`: wall_s=6.39, maxrss_MB=249
- `./tek_hamle_v14 2 512 1`: wall_s=9.19, maxrss_MB=377

## Ham çıktı (`v14_c2_H512_s1.txt`, son satırlar)
```
  gamma=1 lambda=100 ham(T=1,a=1) val_CE=5.6578 | mini-kalibre en iyi val_CE=5.6578 (T=1,a=1)
  gamma=1 lambda=1000 ham(T=1,a=1) val_CE=5.6694 | mini-kalibre en iyi val_CE=5.6694 (T=1,a=1)
  gamma=4 lambda=0.001 ham(T=1,a=1) val_CE=5.6388 | mini-kalibre en iyi val_CE=5.6388 (T=1,a=1)
  gamma=4 lambda=0.01 ham(T=1,a=1) val_CE=5.6388 | mini-kalibre en iyi val_CE=5.6388 (T=1,a=1)
  gamma=4 lambda=0.1 ham(T=1,a=1) val_CE=5.6388 | mini-kalibre en iyi val_CE=5.6388 (T=1,a=1)
  gamma=4 lambda=1 ham(T=1,a=1) val_CE=5.6389 | mini-kalibre en iyi val_CE=5.6389 (T=1,a=1)
  gamma=4 lambda=10 ham(T=1,a=1) val_CE=5.6394 | mini-kalibre en iyi val_CE=5.6394 (T=1,a=1)
  gamma=4 lambda=100 ham(T=1,a=1) val_CE=5.6430 | mini-kalibre en iyi val_CE=5.6430 (T=1,a=1)
  gamma=4 lambda=1000 ham(T=1,a=1) val_CE=5.6553 | mini-kalibre en iyi val_CE=5.6553 (T=1,a=1)
SECILEN[ham] gamma=1 lambda=0.001 L=0.5 val_CE=5.6319 (T=1, alpha=1)  agirlik_uretim_sn=0.367 (W1=0.001 istatistik=0.284 cozum=0.083)
SECILEN[kal] gamma=1 lambda=0.001 L=0.5 val_CE=5.6319 (T=1, alpha=1)  agirlik_uretim_sn=0.347 (W1=0.001 istatistik=0.284 cozum=0.062)
TOPLAM sure=9.1s (arama dahil) arama=6.2s tepe_RAM=377MB
```
