# Tek hamlede ağırlık üretme — algoritma, formüller ve raporlar

Kapsam: bu klasörde (`formul_avcisi/`) yapılan tek-hamle çalışmaları. Delta projesi (kullanıcının bilgisayarındaki)
bu rapora KARIŞTIRILMADI; Delta'nın kendi raporları ayrı (`DELTA_TASKS`).

Güven etiketleri: [Kesin] ölçüldü, [Muhtemel] güçlü çıkarım, [Tahmin] doldurma.

---

## 1. Algoritma (ELM / ridge, tek hamle)

Ağ: giriş → 1 gizli katman (tanh) → çıkış. İki ağırlık matrisi var.

1. **Birinci matris rastgele üretilir, eğitilmez** (tek seferlik):
   `W1 ~ N(0, 1/d)`, `b1 ~ N(0, 0.5²)`  (d = giriş boyu)
2. **Özellikler:** `F = [ tanh(X·W1 + b1) , 1 ]`   (n × (H+1))
3. **İkinci matris tek çözümle üretilir (döngü yok):**

   **`W2 = (FᵀF + λI)⁻¹ · FᵀT`**

   T = one-hot hedefler. Çözüm Cholesky ile: `S·W2 = R`, `S = FᵀF + λI`, `R = FᵀT`.
4. **Tahmin:** `argmax( [tanh(X·W1 + b1), 1] · W2 )`
5. λ doğrulama kümesinden seçilir (`{1, 10, 100, 1000}`).

Maliyet: `n·H²` (S'yi kurmak) + `H³/3` (çözüm). Bellek: `H²` sayı (S matrisi).

### Tek parçada formül
```
W2 = ( Σᵢ fᵢ fᵢᵀ + λI )⁻¹ · Σᵢ fᵢ tᵢᵀ ,   fᵢ = [ tanh(W1ᵀ xᵢ + b1) ; 1 ]
```

## 2. Toplanabilir sürüm (unutmasız ardışık öğrenme)

`S += FᵀF` ve `R += FᵀT` toplamsal. Yeni görev gelince sadece bunlar güncellenir, sonra `W2 = (S+λI)⁻¹R`.
Eski görevin verisi saklanmaz, sadece S ve R.

## 3. Melez sürüm

Birinci matrisi `k` epok klasik (Adam) adımla eğit, sonra `W2`'yi tek hamlede çöz.

---

## 4. Sonuçlar

### 4.1 Oyuncak görevler (107 tek fazlı) — `hafiza_ogrenme.py`
[Kesin] ELM, Adam kalitesinde (+%3,7). Öğrenme işlemi benzer.

### 4.2 Gerçek veri: el yazısı rakamlar (1,8K örnek, 5 bölme, TEST) — `hafiza_digits.py`
| yöntem | test doğruluk | öğrenme işlemi |
|---|---|---|
| Adam MLP | %96,6 | 1,1e10 |
| ELM tek hamle (256) | %96,1 | 1,3e8 (~85x az) |
| ELM tek hamle (1024) | %97,9 | 1,8e9 (~6x az) |
| kNN (saklama) | %96,8 | ~0 |

### 4.3 C++ (`cpp/tek_hamle.cpp`, OpenMP 4 çekirdek, sabit hiperparametre)
| yöntem | doğruluk | süre |
|---|---|---|
| Adam MLP 64-64-10 (300 adım) | %96,1 / 96,3 / 96,7 | ~1,1–1,2 sn |
| ELM Hf=256 | %96,7 / 96,3 / 96,3 | 0,02–0,035 sn (~35–60x hızlı) |
| ELM Hf=1024 | %97,6 / 97,6 / 97,6 | ~0,2 sn (~5–6x hızlı) |

### 4.4 DÜZELTME: MNIST (60k/10k, TEST, numpy, 4 çekirdek) — `mnist_olcek.py`
| yöntem | doğruluk | süre |
|---|---|---|
| Adam MLP 256, 10 epok | %97,46 | 18 sn |
| Adam MLP 1024, 10 epok | %97,82 | 41 sn |
| Tek hamle Hf=1024 / 2048 / 4096 / 8192 | %93,6 / 95,2 / 96,8 / 97,7 | 1,1 / 3,4 / 14,6 / 55 sn |

[Kesin] Rakam verisindeki hız avantajı MNIST'te yok: aynı doğruluk (~%97,5) için ELM 8192 özellik ve 55 sn ister (klasik 18–41 sn).
Melez: 1024 gizli, 2 epok Adam %96,48 @8,2 sn ↔ melez 1 epok %96,28 @8,2 sn. Eşit.

### 4.5 Ardışık görevler (93, TEST) — `hafiza_ardisik.py`
Kalan kayıp oranı (düşük iyi): toplanabilir istatistik ESKİ 0,092 / YENİ 0,065 / ortalama **0,127**;
Adam ince ayar 0,217; yalnız son görevle ELM 0,319.
[Kesin] Tek hamlenin gerçek değeri burada: unutmayı toplanabilir istatistik çözüyor.

### 4.6 İki katmanı da hesaplama denemeleri (MNIST) — `mnist_iki_katman_cozum.py`
2. katman kapalı formda, 1. katman ona göre (gradyansız):
| yöntem | H=256 | H=1024 |
|---|---|---|
| saf hedef yayılımı | ~%87,5, sonra düşer | ~%92, sonra düşer |
| düzeltme sürümü | %88→89, sonra düşer | %93,7, sonra düşer |
| gerçek δ₁ yönü + (XᵀX+λI)⁻¹ ön koşullu adım + kapalı form çıkış | %88,1→93,3 (11 sn) | %93,7→95,6 (43 sn) |
| Adam | %97,5 (18 sn) | %97,8 (41 sn) |

### 4.7 Derinlik testi (H=256, L gizli katman) — `mnist_derin_gradyansiz.py`
| L | rastgele alt + kapalı çıkış | hedef yayılımı en iyi tur | 6. tur | Adam 5 epok |
|---|---|---|---|---|
| 1 | %87,1 | %87,6 | %87,1 | %97,2 |
| 2 | %87,6 | %88,3 | %42,9 | %97,6 |
| 3 | %87,8 | %88,6 | %34,9 | %97,5 |
| 4 | %87,2 | %87,8 | %18,3 | %97,4 |

[Kesin] Hedef yayılımı derinlikte çöküyor.

---

## 5. Dürüst özet

- [Kesin] Tek hamle = 1 gizli katman, 2 matris: 1. rastgele, 2. kapalı form.
- [Kesin] Küçük/kolay veride 5–60x hızlı; MNIST'te eşit doğrulukta klasikten yavaş.
- [Kesin] Asıl değer: toplanabilir istatistikle unutmasız ardışık öğrenme.
- [Kesin] Özellik katmanı (1. matris) öğrenilmediği için sınır ~%87 (H=256) → %93,6 (1024) → %97,7 (8192, 55 sn).
- [Kesin] 1. katmanı gradyansız "hesaplamak" derin ağda çöküyor; gradyanlı yön + kapalı form ön koşullayıcı yavaş ama düzenli iyileşiyor, Adam'ı geçmiyor.
- [Muhtemel] Sınırlar: bellek H², maliyet H³; kolay veri.

## 6. Dosyalar
`mnist_olcek.py`, `hafiza_ogrenme.py`, `hafiza_digits.py`, `hafiza_ardisik.py`, `cpp/tek_hamle.cpp`, `cpp/veri_hazirla.py`,
`mnist_melez.py`, `mnist_iki_katman_cozum.py`, `mnist_derin_gradyansiz.py`. Ayrıntılı log: `README.md`, fikirler: `FIKIRLER.md`.
