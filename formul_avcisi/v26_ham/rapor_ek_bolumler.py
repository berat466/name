import re,os,sys
sys.path.insert(0,'/tmp')
base='/home/user/name/formul_avcisi/'
def rd(p): return open(p).read()
exec(open('/tmp/v26gen.py').read().split("P('PLACEHOLDER')")[0].split("o=[];P=o.append")[0])   # D, agg, avg, fl, MN, RN
rep=rd(base+'TEK_HAMLE_V26_RAPOR.md')
out=[];P=out.append
P('## 4. Okuma kazancı ve öğrenme kazancı ayrı (3 tohum ort. test PPL)\n')
P('| H | V17 softmax-ince | V17 doğrusal | **okuma kazancı** (aynı W) | V20 softmax-ince | V20 doğrusal | V20 W\'nin öğrenme kazancı (softmax okumada) | V20 W\'nin öğrenme kazancı (doğrusal okumada) | en iyi (W, okuma) |'); P('|---|---|---|---|---|---|---|---|---|')
for H in (1024,512):
    g=lambda mn,rn: avg(agg(H,mn,rn,'te',1))
    v17s,v17l=g(MN[0],RN[1]),g(MN[0],RN[2]); v20s,v20l=g(MN[1],RN[1]),g(MN[1],RN[2])
    cands={'V17+doğrusal':v17l,'V20+softmax-ince':v20s,'V17+softmax-ince':v17s,'V20+doğrusal':v20l}; b=min(cands,key=cands.get)
    P(f'| {H} | {v17s:.1f} | {v17l:.1f} | {v17l-v17s:+.1f} | {v20s:.1f} | {v20l:.1f} | {v20s-v17s:+.1f} | {v20l-v17l:+.1f} | {b} ({cands[b]:.1f}) |')
P('')
P('- [Kesin] Okuma kazancı (aynı W): H=1024\'te V17 ağırlıkları −13,4 PPL, H=512\'de −30,4 PPL (ince softmax\'a göre; kaba V17 izgarasına göre −16,7 / −32,3).')
P('- [Kesin] Softmax ızgarasını 4,4× genişletmek (kaba→ince) tek başına −0,8…−3,3 PPL kazandırdı: V9–V24 kalibrasyonu hafif eksik bütçeliydi, ama asıl fark ızgaradan değil, okumanın biçiminden geliyor (doğrusal okuma 160 noktalı ızgarayla ince softmax\'ın (480 nokta) 13–30 PPL altında).')
P('- [Kesin] **V20\'nin tek CE düzeltmesinin kazancı okumaya özgü**: softmax okumada V17→V20 −5,7 (H=1024) / −8,2 (H=512) PPL; doğrusal okumada **+9,2 / +9,1 PPL (kötüleşme)**. V20 gradyanı softmax-kalibre CE için hesaplanmıştı; aynı ağırlıklar doğrusal okumada daha kötü.')
P('- [Kesin] Bu protokolde (yalnız eğitim ağırlıkları, λ softmax okumasıyla seçilmiş) en iyi sonuç **V17 Ridge ağırlıkları + doğrusal okuma**: H=1024 192,2; H=512 **182,5** (H=512 H=1024\'ten iyi: doğrusal okumada aşırı uyum, eğitim–doğrulama farkı 0,92–1,06 nat; eğitim PPL ≈ 71).')
P('- [Muhtemel] λ, γ yalnız softmax okuması için seçilmişti; doğrusal okuma için ayrıca tarama yapılmadı (V25\'te H=512 λ=100 doğrusal 172,9 görüldü), dolayısıyla doğrusal sonuçlar muhtemelen alt sınırdır değil, iyileştirilebilir.')
P('- Top-1/Top-5 (bölüm 2): doğrusal okuma Top-5\'i belirgin artırıyor (H=1024 29,9→33,7%; H=512 29,3→33,4%), Top-1 hafif düşüyor (15,7→15,3%; 15,0→14,8%). PPL kazancı sıralama doğruluğundan çok olasılık kütlesinin dağılımından geliyor.\n')
# C
P('## 5. V25 bulgusunun doğrulanması (tohum 1; aynı W\'ya softmax ve doğrusal okuma)\n')
P('Okuma parametreleri doğrulamayla (softmax-kaba V17 izgarası = V25 ana protokolü, softmax-ince 22T×20α, doğrusal 8ε×20α). V25 ham değerleri karşılaştırma için son sütunda.\n')
P('| H | sistem | hedef | λ | softmax-kaba test PPL | softmax-ince | **doğrusal** (Top-1 / Top-5) | V25\'te doğrusal (18 noktalı ızgara) | okuma sınırı |'); P('|---|---|---|---|---|---|---|---|---|')
v25={('B_F(H=1024)','Y','10'):194.63,('B_F(H=1024)','Q','10'):175.10,('C2_F+w_kimligi+sik_cift(P=4001)','Y','10'):174.99,('C2_F+w_kimligi+sik_cift(P=4001)','Q','10'):152.79,('C0_yalniz_kimlik_(F_yok)(P=4001)','Y','10'):136.84,('C0_yalniz_kimlik_(F_yok)(P=4001)','Q','10'):137.51,
('B_F(H=512)','Y','1'):182.25,('B_F(H=512)','Q','1'):169.56,('B_F(H=1024)','Y','100'):189.51,('B_F(H=1024)','Q','100'):172.48,('B_F(H=512)','Y','100'):172.95,('B_F(H=512)','Q','100'):175.76,('C1_F+w_kimligi(P=2001)','Y','1'):163.76,('C1_F+w_kimligi(P=2001)','Q','1'):146.51,('C1_F+w_kimligi(P=2001)','Y','100'):150.13,('C1_F+w_kimligi(P=2001)','Q','100'):152.21,('C2_F+w_kimligi+sik_cift(P=4001)','Y','1'):166.26,('C2_F+w_kimligi+sik_cift(P=4001)','Q','1'):144.81}
for fn,H in (('v26c_c2_H1024_s1_BDE.txt',1024),('v26c_c2_H512_s1_BCD.txt',512)):
    T=rd(base+'v26_ham/v26log/'+fn).replace('\\n','\n'); res={}
    for l in T.splitlines():
        if l.startswith('RES|'):
            f=l.split('|'); res[(f[1],f[2],f[3].split('=')[1])]=float(re.search(r'test CE=[\d.]+ PPL=([\d.]+)',l).group(1))
    rows={}
    for l in T.splitlines():
        if l.startswith('READ|'):
            f=l.split('|'); k=(f[1],f[2],f[3].split('=')[1]); 
            te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups(); rows.setdefault(k,{})[f[4]]=(float(te[1]),float(te[2]),float(te[3]),l.split('sinir:')[1].strip())
    for k,v in rows.items():
        s=v['SOFTMAX_INCE'];n=v['DOGRUSAL']; ref=v25.get(k)
        P(f'| {H} | {k[0]} | W_{k[1]} | {k[2]} | {res.get(k,float("nan")):.1f} | {s[0]:.1f} | **{n[0]:.1f}** ({100*n[1]:.1f}% / {100*n[2]:.1f}%) | {("%.1f"%ref) if ref else "–"} | {n[3]} |')
P('')
P('- [Kesin] V25 bulgusu tekrarlandı: aynı W, doğrusal okuma → B (F) W_Q ≈ 174–175 (V25: 175,1), C2 W_Q 152,8 (V25: 152,8), yalnız-kimlik 135,7 (W_Y) / 137,5 (W_Q) (V25: 136,8 / 137,5). Sayılar V25 ile ± birkaç PPL içinde (ızgara farkı: 160 vs 18 nokta).')
P('- [Kesin] Softmax okuması W_Q için yetersiz: aynı W_Q yüksek açıklanan varyansa rağmen softmax-ince 208–221 PPL verir; doğrusal okuma 137–175.')
P('- **Yalnız kimlikle trigram/bigram tablosunu ridge ile yeniden ifade etmek yeni öğrenme başarısı değildir** (V25\'te belirtildi); yukarıdaki kimlikli satırlar kimlik bloğunun n-gram tablosu gibi davranmasının ölçümüdür.\n')
# time
P('## 6. Süre ve bellek\n')
P('| H | tohum | Ridge (gram+Cholesky+çözüm) s | V20 CE düzeltmesi (gradyan+D) s | kalibrasyon tabloları (5 η): kaba / ince softmax / doğrusal s | toplam s | tepe RAM MB |'); P('|---|---|---|---|---|---|---|')
for (H,s),d in sorted(D.items(),reverse=True):
    t=rd(f'{base}v26_ham/v26log/v26_c2_H{H}_s{s}.txt'); cal=re.search(r'softmax-kaba=([\d.]+)s softmax-ince=([\d.]+)s dogrusal=([\d.]+)s',t).groups()
    P(f'| {H} | {s} | {d["sure"][0]:.2f} | {d["sure"][1]:.2f} | {cal[0]} / {cal[1]} / {cal[2]} | {d["tot"][0]} | {d["tot"][1]} |')
P('\n**Uyarı:** Bu süreler tutarsız: V25-doğrulama koşusu (`v26c`) aynı anda çalıştığı için 4 çekirdek paylaşıldı; V26 Ridge süreleri normalde ≈1,7 s (H=1024), V20 düzeltmesi ≈6 s (V22/V25 temiz ölçümleri) iken burada 2–4× büyük görünüyor. Süre karşılaştırması için yalnız oranlar anlamlı: doğrusal okuma kalibrasyon tablosu softmax-kabadan ≈7–9× hızlı (üstel hesap yok), ince softmax ızgarası kabadan ≈1,5× yavaş. Okuma (tek değerlendirme) 0,3–0,8 s, doğrusal en hızlısı. [Kesin: oranlar loglardan; mutlak süreler güvenilmez]\n')
P('''## 7. Sonuç

**Ana soru: formülle ürettiğimiz ağırlıklar yetersiz mi, yoksa softmax yüzünden bilgiyi yanlış mı okuyorduk?**

- [Kesin] Softmax kalibrasyonu **ciddi bir okuma kaybı** yaratıyordu: aynı V17 Ridge ağırlıkları doğrusal okumada H=1024'te 205,6 → 192,2, H=512'de 212,9 → 182,5 PPL (3 tohum, yalnız-eğitim protokolü, parametreler yalnız doğrulamada). Bu fark softmax ızgarasını genişletmekle kapanmıyor.
- [Kesin] Bu iyileşme "okuma kazancıdır"; ağırlık değişmedi. Öğrenme kazancı olarak sunulamaz. Aynı zamanda ağırlıkların kalitesi softmax okuma altında olduğundan kötü görünüyordu.
- [Kesin] **V20'nin CE düzeltmesi bu protokolde doğrusal okumayla en iyi sonucu vermiyor**; V17 + doğrusal okuma V20'yi (her iki okumayla) geçiyor. V20'nin PPL kazancı softmax'a özgüymüş. Dolayısıyla V19–V21'in "CE düzeltmesi" iyileştirmeleri okuma katmanına bağımlı bir bulgudur.
- [Kesin] Doğrusal okuma Top-5'i artırıyor (+3,8 pp), Top-1'i hafif düşürüyor (−0,4 pp); eğitim–doğrulama farkı çok büyüyor (≈1,0 nat; eğitim PPL ≈ 71 vs test 192) → bu okumayla eğitim CE\'si güvenilir bir ölçüt değil; seçim yalnız doğrulamayla yapılmalı.
- [Muhtemel] 100 PPL'e yaklaşmak için bu okuma tek başına yetmez (en iyi 3-tohum ≈ 182,5); ama V25'te kimlik+doğrusal okumayla ≈ 137–150 görüldü ki bu n-gram tablosunun yeniden ifadesidir.
- Doğrusal okuma V26 **okuma seçeneği** olarak uygulandı (`tek_hamle_v26.cpp`), ana algoritmaya birleştirilmedi.

## 8. Sınırlamalar

- 3 tohum, tek metin/bölme; test n=4 980. λ/γ doğrusal okuma için yeniden seçilmedi. Doğrusal okumada ε=1e-7 sıkça ızgara kenarında seçildi (etkisi, α karışımı nedeniyle küçük; ε→0 sınırı) ve α=0,5–0,8 aralığında.
- V20 burada yalnız-eğitim protokolüyle (η doğrulamayla, V20 raporu: eğitim+doğrulama modeli): "eski PPL" sütunu referanstır, birebir yeniden ölçüm değildir.
- Süreler çekirdek paylaşımı nedeniyle güvenilmez (bölüm 6). Sayısal doğruluk: tüm okumalarda |Σp−1| ≤ 1e-13 ✓; negatif/sıfır z oranı ≈ %48 (V17/V20, doğrusal ve softmax aynı z), doğrusal okumada gerçek sınıfın kırpılma oranı %17–20.
- V25 doğrulaması tek tohum; ızgaralar farklı olduğundan tam eşitlik beklenmez.
''')
P('## 9. Çalıştırma\n\nDerleme: `g++ -O3 -march=native -fopenmp -std=c++17 cpp/tek_hamle_v26.cpp -o tek_hamle_v26`; komut: `./tek_hamle_v26 2 H TOHUM GAMMA LAMBDA`. V25 doğrulaması: `./tek_hamle_v26c 2 H 1 GAMMA LAMBDA MOD` (`BDE` / `BCD`).\n')
man=[l.split('\t') for l in rd(base+'v26_ham/v26log/manifest.tsv').splitlines()]
P('| komut | wall s | user CPU s | RAM MB |'); P('|---|---|---|---|')
for m in man:
    kv=dict(x.split('=') for x in m[2:]); P(f"| `{m[0]}` | {kv['wall_s']} | {kv['user_cpu_s']} | {kv['maxrss_MB']} |")
rep=rep.replace('PLACEHOLDER','\n'.join(out))
open(base+'TEK_HAMLE_V26_RAPOR.md','w').write(rep+'\n')
