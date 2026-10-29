# Profil Raporu — 1,03 s nereye gidiyor?

Motorun kendisi (`src/`) bu çalışmada **değişmedi**. Deneyler `deney_profil/` altında, kopya kod üzerinde; `deney_profil/calistir.sh` hepsini yeniden üretir, ham çıktı `deney_profil/sonuc.txt`.
Makine: Intel Xeon 2,8 GHz (4 çekirdek, paylaşımlı sandbox), g++ 13, `-O2`, tek iş parçacığı. Her ölçüm 3–5 koşunun **en iyisi**. Koşular arası dalgalanma ≈ ±5%, bazı hücrelerde ±10–15%; bu sınırın altındaki farkları "yok" sayıyorum.

## Yöntem
1. **Kapat/aç (ablation):** `vm.cpp`'nin deney kopyasında her mekanizma ayrı ayrı çıkarıldı: yakıt sayacı, çağrıda yazmaç sıfırlama, yığın/derinlik denetimi, bellek sınır denetimi, `switch` yerine hesaplanmış `goto`.
2. **Dinamik komut histogramı:** hangi komut kaç kez çalıştı.
3. **Yerel C++** (`-O0`, `-O2`) aynı algoritmalar için.
4. **AOT üst sınır deneyi:** aynı program tablosunu C++'a çeviren küçük bir araç (`pm2cpp`). Aynı tablo, ikinci bir yürütme yöntemi.

Beş iş yükü: `fib35` (çağrı ağırlıklı), `loop_sum` (saf tamsayı döngüsü), `sieve10m` (bellek), `floatloop` (ondalık), `memsum` (bellek okuma). Hepsi küçük çekirdek kıyaslamalar; gerçek uygulama (arayüz, oyun) **değil**.

## Sorularına cevap: `fib(35)` için 0,92 s nereye gidiyor?
| Kalem | Ölçülen etki (fib35) | Yorum |
|---|---|---|
| **Yakıt sayacı** | **≈ %0** (0,919 → 0,915 s, gürültü içinde) | Önemsiz. Sonlanma garantisi bedava sayılır. |
| **Yığın/derinlik denetimi** | ≈ %3–4 (gürültü sınırında) | Önemsiz. |
| **Bellek sınır denetimi** (`ld/st`) | ≈ %1–3 (sieve/memsum) | Önemsiz. |
| **Çağrıda yazmaç sıfırlama** | **%14–21** (iki koşuda 0,919 → 0,790 / 0,734 s) | Gerçek maliyet. Yalnızca çağrı ağırlıklı yükte. Spesifikasyon kuralını değiştirerek kaldırılabilir (aşağıda). |
| **Komut dağıtımı** (`switch` → hesaplanmış `goto`) | **%19** fib, %3–12 sieve/memsum, ≈0 loop_sum/floatloop | Çağrı ve bellek yoğun yükte kazanç var, saf döngüde yok. |
| **Gereksiz komutlar** (benim ürettiğim kod) | **dinamik komutların %42'si** (`ldi` %21,1 + `mov` %21,1) | Sabitleri her çağrıda yazmaca yüklemek ve argümanları pencereye `mov` ile kopyalamak. Doğrudan operand (immediate) ve argümanı doğrudan pencereye yazan bir geçişle ~%40 daha az komut. |
| Hepsi kapalı (dağıtım + sıfırlama + denetimler + yakıt yok) | 0,919 → **0,656 s** (%29) | Geriye kalan ≈ %70, yorumlayıcının temel yapısı: komut çözme + dolaylı atlama + bellekteki yazmaç dosyası. |

Kalemler **toplanmaz** (üst üste biniyor). Her biri tek başına çıkarıldığında ölçüldü.

**Basit döngüde komut başına 5,4 çevrim** (`loop_sum`: 4 komut, 7,7 ns/yineleme). İyi bir yazmaç VM'i bunun yarısına inebilir; bu motorda dağıtım yöntemi değiştirince bile `loop_sum` düzelmedi, yani darboğaz orada dağıtımdan çok, bağımlı komutların bellekteki yazmaçlar üzerinden zincirlenmesi olabilir. **Bunu ayrıca ölçmedim**, çıkarım.

## Yerel C++ ile fark (aynı algoritma)
| İş yükü | Motor | Yerel `-O0` | Yerel `-O2` | Motor / `-O0` | Motor / `-O2` |
|---|---|---|---|---|---|
| fib35 | 0,919 | 0,077 | 0,026 | 11,9× | 35× |
| loop_sum | 0,775 | 0,301 | 0,031 | 2,6× | 25× |
| sieve10m | 0,718 | 0,483 | 0,345 | 1,5× | 2,1× |
| floatloop | 0,351 | 0,109 | 0,051 | 3,2× | 6,9× |
| memsum | 0,685 | 0,172 | 0,022 | 4,0× | 32× |

"Onlarca kat yavaş" kaygısı **çağrı ağırlıklı ve saf hesap** yüklerinde doğru (12–35×). **Bellek bağımlı** işte fark küçük (1,5–2×).
`-O2` sütunlarına dikkat: derleyici bu küçük döngüleri vektörleştirip açıyor; gerçek uygulamada oran küçülür. Bu kıyaslar bunu göstermiyor.

## AOT deneyi: aynı tablo, ikinci bir yürütme yöntemi
Tabloyu C++'a çeviren araç (yazmaçlar bellekte `R[]` olarak kaldı, anlam VM ile aynı; derleme `g++ -O2`). **Beş programın çıktısı VM ile bire bir aynı** (`cikti-ayni: evet`).
| İş yükü | VM | AOT | AOT + yakıt | VM / AOT+yakıt |
|---|---|---|---|---|
| fib35 | 0,876 | 0,143 | 0,135 | **6,5×** |
| loop_sum | 0,784 | 0,005 | 0,074 | 10,7× (yakıtsız 160×: derleyici döngüyü kapalı forma çevirdi, geçersiz kıyas) |
| sieve10m | 0,714 | 0,343 | 0,341 | 2,1× (≈ yerel `-O2`: 0,345) |
| floatloop | 0,366 | 0,037 | 0,043 | 8,5× |
| memsum | 0,710 | 0,095 | 0,062 | 11,4× |

Yakıt, AOT'ta **yalnızca işlev girişinde ve geri atlamada** (döngü başı) azaltıldı; komut bazlı değil. Bu sonlanma garantisini korur (VM'deki komut başına sayaçtan farklı hassasiyet).
AOT süreleri **süreç başlatma + bellek ayırmayı içeriyor**, VM süreleri yalnızca yürütme; yani kıyas AOT aleyhine hafif eğik.

## Sonuçlar
1. **Parametre = kalıcı biçim, yürütme yöntemi ayrı** fikri çalışıyor. Aynı tablo iki farklı yürütme yönteminde aynı çıktıyı verdi. Ama bu **beş küçük programda** gösterildi; tuzak konumu, yakıt hassasiyeti ve yığın taşması davranışı AOT'ta eşdeğer değil.
2. **Yorumlayıcıyı ayarlamak sınırlı kazandırır.** Hesaplanmış `goto` (%3–19), sıfırlamanın kaldırılması (%14–21, yalnızca çağrılarda), daha az komut üreten kod (~%40 daha az komut) birlikte çağrı ağırlıklı yükte **en iyi ihtimalle ~2×** verir **[Tahmin: etkiler üst üste binmediği için ölçülmedi]**. Yerel `-O2` ile fark yine ≥ 10×.
3. **Büyük kazanç farklı bir yürütme yönteminden geliyor:** AOT bu yüklerde 2–11× (bellek bağımlıda ≈ yerel hızda). Bedeli: hedefte derleyici ya da JIT gerekir; ikincisi bu motorun "küçük ve bağımsız" hedefiyle çakışır.
4. **Önemsiz çıkanlar:** yakıt sayacı, yığın/derinlik denetimi, bellek sınırları. Güvenlik ve sonlanma için bunlar korunabilir.

## Şartnameye çıkan somut öneriler (henüz uygulanmadı)
- **Başlatılmamış yazmaç kuralı:** "çağrıda sıfırlanır" yerine "okunmadan önce yazılmış olduğu doğrulayıcıyla kanıtlanır (kesin atama)". Böylece hem VM hem AOT sıfırlamayı atlar. Maliyet: doğrulayıcıya veri akışı analizi.
- **Yakıt hassasiyeti:** "komut başına" yerine "çağrı ve geri atlama başına". Backend'ler arası tutarlılık için tek tanım.
- **Çok yürütücü zorunlu test:** her yeni yürütme yöntemi, mevcut tüm testlerde referans yorumlayıcıyla bayt eşit çıktı vermek zorunda (burada elle yapıldı, otomatik test değil).
- **Kod üretimi geçişi:** immediate operandlar ve argümanları doğrudan pencereye yazan bir peephole geçişi (fib35'te komutların ~%40'ı).

## Sınırlar
- 5 küçük kıyas, tek makine, paylaşımlı sandbox gürültüsü (±5–15%).
- Çağrı etkisi (sıfırlama) iki koşuda %14 ve %21 çıktı; "~%14–21" aralığı olarak yazdım.
- Hesaplanmış `goto` etkisi bazı hücrelerde tersine döndü (`loop_sum` +4%); gürültü ya da gerçek, ayıramadım.
- AOT aracı: yığın taşması, derinlik ve tuzak konumunu kopyalamıyor; yalnızca assembler'ın ürettiği programlarda doğrulandı (pencere alanının çağırana ait adlı yazmaçları ezmediği varsayımı).
- Bu ölçümler "uygulama hızı" değil, motorun komut işleme maliyetidir.
