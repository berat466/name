# V68: gerçek tek işlemle ağırlık üretimi — sonuç: BAŞARISIZ (iptal testi çözülemedi; kurallara uyan hiçbir aday R² ≥ 0,95'e ulaşmadı)

Kod: `v68_tek_islem/deney.py`, ham çıktı `sonuc.json`. V65/V66/V67 dosyaları değişmedi. Talimat gereği iptal testi çözülmediği için **Digits ve Diabetes çalıştırılmadı**; V68 için sahte başarı üretilmedi.

## Ne denendi (hepsi kurallara uygun: çözücü, ters, SGD, örnek belleği yok; aynı iptal verisi ve bölme: y=x₁−x₂, r=0,965, 700/250/250, hedef: test R² ≥ 0,95)
| Aday | Test R² |
|---|---|
| V66 kuralı (cov/var) — referans | −0,016 |
| Veriden bağımsız ikili-kontrast haritası (820 özellik: xⱼ ve tüm xⱼ−xₖ) + aynı birikim kuralı | 0,052 |
| V66 kuralının 2 uygulaması (artık birikimi, sabit 2 geçiş) | 0,116 |
| (yalnız karşılaştırma) Gram çözümlü ridge zinciri | 1,000 |
**Hedef 0,95 sağlanamadı.** Kontrast haritası cevabı (x₁−x₂) aday olarak içermesine rağmen başarısız: köşegen-normalize kural, doğru farkı yüzlerce ilgisiz farktan ayıramadı (ve zaten O(d²) özellik, genel ölçeklenemez).

## Neden — ölçülen engel (yalnız teşhis; yöntem olarak kullanılmadı)
Gram G'nin özdeğerleri 0,026…2,60, **koşul sayısı 98,2**; 20 özdeğer < 0,2. Sinyalin yaklaşık %52'si en küçük 20 özdeğerli yönlerde. Bu yönlerde kazanç ≈ 1/λ ≈ 38–98 gerekir. Tam-batch gradyan adımlarıyla (Landweber) R² 0,95'e **113 adımda** ulaşıldı (R² 0,22 @10, 0,93 @100, 1,0 @1000) — yani gereken şey, G⁻¹c'ye eşdeğer bir kazanç: ya matris tersi/çözümü (yasak) ya ~10²–10³ matris-vektör adımı (iteratif optimizasyon, yasak). Köşegen, satır-toplamı, düşük dereceli polinom (1–2 uygulama) ve sabit kontrast haritası gibi **sınırlı kazançlı, tek-geçişli kurallar küçük özdeğerli yönleri yükseltemez**. Bu bir kanıt değil, bu veri için ölçüm ve gerekçedir; genel imkânsızlık teoremi iddia etmiyorum.

## Sorular (kesin cevaplar)
1. **Gerçek ağırlık ürettin mi, veri mi sakladın?** Yeni bir çalışan ağırlık üretimi **kurulamadı**; veri saklanmadı (V67 örnek belleği kullanılmadı). Aday kurallar sabit boyutlu ağırlık üretti ama doğru değil.
2. **Tek temel işlem:** bulunamadı. Denenen: birikim + öğe-bölme (V66) ve ikili-kontrast + aynı kural; ikisi yetersiz.
3. **Klasik zincir kullanıldı mı?** Adaylarda hayır. Referans ve teşhiste (ridge, Landweber) yalnız karşılaştırma/teşhis.
4. **Küçük farkları öğrenebildi mi?** Hayır (R² ≤ 0,12).
5. **Digits / Diabetes:** çalıştırılmadı (iptal testi geçilmeden ilerlenmeyecekti). V67 referansları: %97,2 / 0,292 (V68 için ölçüm yok).
6. **Kaç ms'de kaç ağırlık:** adaylar milisaniye altı (1 s'den kısa toplam), 40 ve 820 ağırlık; ama sonuç yanlış olduğundan başarı sayılmaz.
7. **Eğitim örnekleri silinince çalıştı mı?** Adaylar yalnız ağırlıkla tahmin eder, ancak doğrulukları yetersiz; "gerçek ağırlık üretimi" kontrolünü geçmedi.
8. **V66/V67 problemleri:** çözülmedi. V66 köşegen bağımlılığı ve iptal yapısı hâlâ açık; V67 örnek belleği sorunu kaçınıldı ama çözülmedi (alternatif bulunamadı).
9. **Ölçeklenebilirlik:** kanıtlanmadı; olumlu bulgu yok.

## Net sınır
Özellikler arası **ters kovaryans bilgisi** (küçük özdeğerli yönlerin yükseltilmesi) gerekiyorsa, ağırlık üretimi ya G⁻¹ benzeri bir çözüm ya da yaklaşık koşul sayısı kadar adımlı yineleme gerektiriyor. Kurallar ikisini de yasakladığından, bu veri için kurallara uyan tek-işlem çözümü 10 dakikada bulunamadı. Kuralları gevşetmeden ilerleyecek yön: kazancı sınırlı olmayan bir işlem (ör. ters/çözüm ya da çok adımlı kontrol edilmiş yineleme) kabul edilirse tamamen klasik eğitimle özdeş olur; yani "klasik zincirsiz" hedef bu iptal yapısında sağlanamadı.
