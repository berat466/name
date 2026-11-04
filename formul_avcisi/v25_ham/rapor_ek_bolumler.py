import re
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
rep=rd(base+'TEK_HAMLE_V25_RAPOR.md')
T1=rd(base+'v25_ham/v25log/v25_c2_H1024_s1_ABCDE.txt')
rows1={}
for l in T1.splitlines():
    if l.startswith('RES|'):
        f=l.split('|'); k=(f[1],f[2],f[3].split('=')[1]); te=re.search(r'test CE=([\d.]+) PPL=([\d.]+)',l); rel=float(re.search(r'rel_hata e/d/t=[\d.]+/[\d.]+/([\d.]+)',l).group(1)); rows1[k]=(float(te.group(2)),1-rel)
rows5={}
for l in rd(base+'v25_ham/v25log/v25_c2_H512_s1_ABCD.txt').splitlines():
    if l.startswith('RES|'):
        f=l.split('|'); k=(f[1],f[2],f[3].split('=')[1]); te=re.search(r'test CE=([\d.]+) PPL=([\d.]+)',l); rel=float(re.search(r'rel_hata e/d/t=[\d.]+/[\d.]+/([\d.]+)',l).group(1)); rows5[k]=(float(te.group(2)),1-rel)
lin=[]
for fn,H in (('v25r_c2_H1024_s1_BDE_linread.txt',1024),('v25r_c2_H512_s1_BCD_linread.txt',512)):
    for l in rd(base+'v25_ham/v25log/'+fn).splitlines():
        if l.startswith('LINREAD|'):
            f=l.split('|'); m=re.search(r'alpha=(\S+) eps=(\S+)\|dogr CE=([\d.]+)\|test CE=([\d.]+) PPL=([\d.]+)',l)
            lin.append((H,f[1],f[2],f[3].split('=')[1],m.group(1),m.group(2),float(m.group(5))))
out=[]
out.append("## 3. Ek tanı: okuma katmanı (kalibre softmax vs doğrusal okuma) — AYNI W, farklı okuma\n")
out.append("Ana protokolde (V9–V24) ridge çıktısı `z` kalibre softmax `P=(1−α)π+α·softmax(z/T)` ile olasılığa çevrilir. Q'ya MSE ile iyi yaklaşan `z` zaten olasılık ölçeğindedir; bu yüzden **aynı ağırlıkları** doğrusal okumayla da değerlendirdim: `P=(1−α)π+α·max(z,ε)/Σmax(z,ε)` (α∈{0,5…1}, ε∈{1e-5,1e-4,1e-3}, **doğrulama CE ile** seçildi; test yalnız son ölçüm). Bu yeni ağırlık üretimi değil, okuma tanısıdır; Top-1/Top-5 bu okumada ölçülmedi.\n")
out.append("| H | sistem | hedef | λ | softmax-kalibre test PPL (ana protokol) | doğrusal okuma test PPL | (α,ε) | Q açıklanan varyans (test) |\n|---|---|---|---|---|---|---|---|")
for (H,nm,tg,lam,a,e,tp) in lin:
    d=rows1 if H==1024 else rows5
    sp,ev=d.get((nm,tg,lam),(float('nan'),float('nan')))
    out.append(f"| {H} | {nm} | W_{tg} | {lam} | {sp:.1f} | **{tp:.1f}** | {a},{e} | {('%.3f'%ev) if tg=='Q' else '–'} |")
out.append("")
out.append("""## 4. Karar soruları

**A — X→F bilgi aktarımını artırıyor mu?** [Kesin, ölçülen] Evet bu kapasitede: Q açıklanan varyans (test) X: 0,30 → F: 0,42; gerçek etiket PPL (kalibre softmax) 228,8 → 208,5. [Muhtemel] Kapasite karışık (193 vs 1025 yoğun sütun), artışın tamamı "tanh dönüşümü" değildir; eşit boyutlu doğrusal özellik kontrolü yapılmadı. F yine de kelime-çifti kimliğini çözmüyor.

**B — Bağlam kimliği açıklanan öğretici bilgisini %43'ün üzerine çıkarıyor mu?** [Kesin] Evet: F+w kimliği (H=1024, λ=10) %75, H=512 λ=1'de %96 (test, W_Q); sık çiftleri eklemek yalnız +%0,7. Sebep: V24'ün EM ağırlıklarına göre Q'nun %95'i bigram+unigram (trigram bileşeni %4,6), yani `w` kimliği Q'nun neredeyse tamamını doğrusal üretir. Yalnız kimlik (F yok, C0) da %72 açıklıyor → **bilgi kaybının yeri: F'nin kelime kimliği/çift bilgisini taşıyamaması** [Kesin: kimlik eklenince %42 → %75–96].

**C — Açıklanan bilgi artınca test PPL de düşüyor mu?** Evet ama kalibre-softmax okumada az: W_Y 208,5 → 200,5 → 198,1 (F → F+w → F+w+çift; H=1024); W_Q 223,9 → 212,2 → 210,8; Q'yu %96 açıklayan H=512 C1 bile 213. **Q'ya MSE yaklaşması ≠ PPL başarısı** [Kesin]. Doğrusal okumada aynı ağırlıklarla PPL çok daha düşük (bölüm 3): B W_Q 223,9 → 175,1; C2 W_Q 152,8; H=512 C1/C2 W_Q 146,5 / 144,8; yalnız-kimlik C0 136,8–137,5.

**D — Kimlikli kapalı form çözüm trigramın 138,9'una yaklaşabilir mi?** [Kesin] Evet: yalnız-kimlik ridge + doğrusal okuma test PPL 136,8 (W_Y) / 137,5 (W_Q), trigram 138,9, bigram 144,9. Fakat bu **klasik n-gram tablosunun ridge ile yeniden ifadesidir** (kimlik sütunları = sayım tablosu); yeni ağırlık-üretim başarısı olarak sunulmuyor. F eklenince doğrusal okumada kötüleşiyor (H=1024 C2 W_Q 152,8 vs C0 137,5) [Muhtemel: rastgele özelliklerin gürültüsü, aynı λ]. Ana mimariye eklenmedi.

**E — Eksik bilgi sayısal gösterilebilir mi?** [Kesin] Merdiven (W_Q, test açıklanan varyans): X 0,30 → F 0,42 → F+w kimliği 0,75 (λ=10) / 0,96 (H=512, λ=1) → +sık çift 0,76; yalnız kimlik 0,72. Eksik bilgi: kelime kimliği/çift bilgisi (F'de yok, X'te doğrusal olarak yok). İkinci, **bağımsız** kayıp okuma katmanında: aynı W için kalibre softmax → doğrusal okuma PPL farkı C0 W_Q'da 75 PPL (212,2 → 137,5), B W_Q'da 49 PPL (223,9 → 175,1).

**V24 düzeltmesi:** [Kesin] V24'ün "bilgi aktarımı ≈%43, kayıp F temsilinde" sonucu açıklanan varyans için doğru; ama "öğretici başarısı öğrenciye geçmiyor (PPL 222)" kısmı büyük ölçüde **softmax-kalibrasyon okumasının** sonucuydu: aynı F ve W_Q doğrusal okumada 175 PPL (H=512: 170) veriyor. V9–V24'teki tüm "Ridge" PPL değerleri bu okumaya bağlıdır; bu bulgu V17/V20 karşılaştırmalarını (206,8 / 198,9) etkiler ve doğrulanması gerekir (tek tohum). [Muhtemel] Darboğaz iki katlı: (1) F kimlik bilgisini taşımıyor, (2) softmax(z/T) kalibrasyonu olasılık-ölçekli regresyon çıktısı için kayıplı okuma.
""")
out.append("""## 5. Sınırlamalar

- Tek tohum (1); H=1024 ana; H=512 yalnız B/C1/C2 (A ve C0 yok). 3 tohum yapılmadı.
- İlk koşuda Gram matrisini yalnız bir üçgenle kopyalama hatası vardı (B ≈ 274 PPL); düzeltildi, B V24 sonuçlarını birebir yeniden üretti (208,53 / 223,87). Hatalı koşunun logu saklanmadı.
- Doğrusal okuma yalnız CE/PPL ile ölçüldü (Top-k yok); (α,ε) 18 noktalık ızgara, doğrulamayla. Seyrek kimlik λ'sı F ile aynı; ayrı λ denenmedi. Sık çift sayısı 2000 (19 646 çiftten).
- Q eğitim satırlarında leave-one-out; kimlik özellikleri yalnız geçmiş bağlam kelimelerinden (hedef sızıntısı yok). Kimlikli W_Y sistemlerinde yüksek eğitim–doğrulama farkı (0,70–0,85 nat) aşırı uyumu gösterir.
- C0/C1/C2 n-gram sayımının doğrusal yeniden ifadesidir; V11/V12 kimlik denemeleriyle fark: burada kimlikler Q'ya karşı ve doğrusal okumayla ölçüldü. V11/V12 sayıları yeniden koşturulmadı.
- Kapasite farkı (A: 193, B: 1025, C: 1025 + 2001/4001 sütun) sonuçları yalnız "temsil kalitesi"ne bağlamayı engeller.

## 6. Öneri (uygulanmadı)

1. V9–V24'ün kalibre-softmax okumasını doğrusal olasılık okumasıyla yeniden ölçmek (3 tohum, V17/V20 protokolü): Ridge'in gerçek PPL'ini belirlemek. Bu, hedef-koşullu ağırlık üretimini değerlendirme ölçüsü bakımından değiştirir.
2. Kimlik bloğu ve yoğun F için ayrı λ; Q hedefinin olasılık uzayında kalması; F'yi yalnız kimliğin açıklayamadığı artık üzerinde kullanmak. Hedefi V24'teki ulaşılabilirlik sertifikasıyla birlikte yazmak.
3. ≈137 PPL değerleri n-gram tablosunun yeniden ifadesidir; "öğrenme kazancı" olarak raporlanmamalı. Gerçek ölçüt: F+kimliğin kimlik-yalnız modele göre ek kazancı (burada doğrusal okumada negatif).

## 7. Süre ve bellek

""")
man=[l.split('\t') for l in rd(base+'v25_ham/v25log/manifest.tsv').splitlines()]
out.append("| komut | wall s | user CPU s | RAM MB |\n|---|---|---|---|")
for m in man:
    kv=dict(x.split('=') for x in m[2:]); out.append(f"| `{m[0]}` | {kv['wall_s']} | {kv['user_cpu_s']} | {kv['maxrss_MB']} |")
rep=rep.replace('PLACEHOLDER','\n'.join(out))
open(base+'TEK_HAMLE_V25_RAPOR.md','w').write(rep+'\n')
