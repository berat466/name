import re,os
exec(open('/tmp/v27gen.py').read().split("P('PLACEHOLDER')")[0])
rep=rd(base+'TEK_HAMLE_V27_RAPOR.md')
out=[];P=out.append
def pick(H,nm_prefix,tg,lab_prefix=None):
    es=[]
    for s in (1,2,3):
        if (H,s) in R:
            for x in R[(H,s)]['st']:
                if x['name'].startswith(nm_prefix) and x['tg']==tg and (lab_prefix is None or x['lab'].startswith(lab_prefix)): es.append(x)
    return es
def m(es,f): return avg([f(e) for e in es]) if es else float('nan')
P('## 3. Aşama aşama katkı (3 tohum ortalaması): gerçek ek bilgi ve %25 kapasite payı\n')
P('Hedef Q (trigram dağılımı) için her aşama sonrası açıklanan varyans; "ek" = bir önceki aşamaya göre artış. Y hedefi için kalan regresyon hatası `‖Y−Ŷ‖²/n` (test).\n')
P('| H | aşama | eklenen sütun | kümülatif sütun | Q açıklanan varyans (test) | ek açıklanan (test) | Q-zincir test PPL | Y-zincir kalan hata (test) | Y-zincir test PPL | Y-zincir eğitim / doğr. PPL |'); P('|---|---|---|---|---|---|---|---|---|---|')
for H in (512,1024):
    if not any((H,s) in R for s in (1,2,3)): continue
    cumw=0
    for k in range(1,5):
        lab=f'F1..F{k}'
        q=pick(H,'BAGLI','Q',lab); y=pick(H,'BAGLI','Y',lab)
        if not q: continue
        cumw+=q[0]['w']
        P(f'| {H} | F{k} | {q[0]["w"]} | {cumw} | {m(q,lambda e:e["q"][1]):.3f} | {m(q,lambda e:e["q"][3]):+.3f} | {m(q,lambda e:e["te"][1]):.1f} | {m(y,lambda e:e["res"][2]):.4f} | {m(y,lambda e:e["te"][1]):.1f} | {m(y,lambda e:e["trp"]):.0f} / {m(y,lambda e:e["vap"]):.0f} |')
    s1=pick(H,'TEK_F','Q'); sy=pick(H,'TEK_F','Y'); jq=pick(H,'ORTAK','Q'); jy=pick(H,'ORTAK','Y')
    P(f'| {H} | *tek F ({H}+1)* | {H+1} | {H+1} | {m(s1,lambda e:e["q"][1]):.3f} | – | {m(s1,lambda e:e["te"][1]):.1f} | {m(sy,lambda e:e["res"][2]):.4f} | {m(sy,lambda e:e["te"][1]):.1f} | {m(sy,lambda e:e["trp"]):.0f} / {m(sy,lambda e:e["vap"]):.0f} |')
    P(f'| {H} | *ortak ridge 4 F* | {sum(hs for hs in [H//4+1]+[H//4]*3)} | {H+1} | {m(jq,lambda e:e["q"][1]):.3f} | – | {m(jq,lambda e:e["te"][1]):.1f} | {m(jy,lambda e:e["res"][2]):.4f} | {m(jy,lambda e:e["te"][1]):.1f} | {m(jy,lambda e:e["trp"]):.0f} / {m(jy,lambda e:e["vap"]):.0f} |')
P('')
# karar ozeti
P('## 4. Karşılaştırma ve sonuç\n')
for H in (512,1024):
    if not any((H,s) in R for s in (1,2,3)): continue
    n=len([s for s in (1,2,3) if (H,s) in R])
    sy=m(pick(H,'TEK_F','Y'),lambda e:e['te'][1]); sq=m(pick(H,'TEK_F','Q'),lambda e:e['te'][1])
    c1y=m(pick(H,'BAGLI','Y','F1..F1'),lambda e:e['te'][1]); c4y=m(pick(H,'BAGLI','Y','F1..F4'),lambda e:e['te'][1])
    bsty=min(m(pick(H,'BAGLI','Y',f'F1..F{k}'),lambda e:e['te'][1]) for k in range(1,5))
    c4q=m(pick(H,'BAGLI','Q','F1..F4'),lambda e:e['te'][1]); jy=m(pick(H,'ORTAK','Y'),lambda e:e['te'][1]); jq=m(pick(H,'ORTAK','Q'),lambda e:e['te'][1])
    eq1=m(pick(H,'TEK_F','Q'),lambda e:e['q'][1]); eq4=m(pick(H,'BAGLI','Q','F1..F4'),lambda e:e['q'][1]); eqj=m(pick(H,'ORTAK','Q'),lambda e:e['q'][1])
    P(f'- **H={H} ({n} tohum), doğrusal okuma, test PPL:** tek F: Y {sy:.1f}, Q {sq:.1f}; bağlı 4F: Y (F1 / F1..F4 / en iyi aşama) {c1y:.1f} / {c4y:.1f} / {bsty:.1f}, Q (F1..F4) {c4q:.1f}; ortak ridge: Y {jy:.1f}, Q {jq:.1f}. Q açıklanan varyans (test): tek F {eq1:.3f}, bağlı 4F {eq4:.3f}, ortak ridge {eqj:.3f}.')
P('''
**Okuma (yalnız yukarıdaki ölçümler):**
- [Kesin] Aynı toplam sütun sayısında, bağlı 4 F tek F\'den **daha az** Q bilgisi açıklıyor ve test PPL\'i düşürmüyor. Başarı hedefi (V26 referansı 182,5\'in altı) Y hedefiyle sağlanmadı; aşağıdaki tablo ve seed ortalamaları bunu gösteriyor.
- [Kesin] En iyi bağlı-F Y sonucu H=512'de 186,4 (yalnız F₁), H=1024'te 189,2 (yalnız F₁): 182,5 referansının altına inilemedi. H=1024'te F₁ (257 sütun) tek F'den (1025 sütun, 192,2) iyi çıkması zincirden değil, daha küçük modelin doğrusal okumada daha az aşırı uyum yapmasından geliyor (H=512 tek F 182,5 hâlâ daha iyi) [Muhtemel].
- [Kesin] Q-hedefli zincirde F₂ test PPL'ini 183,9→178,9 (H=512) ve 180,6→177,8 (H=1024) düşürüyor, F₃/F₄ düz kalıyor: ek aşama yalnız küçük bir kazanç sağlıyor, tek F'nin (170,3 / 172,3) gerisinde.
- [Kesin] F₂, F₃, F₄ eklediği açıklanan Q varyansı çok küçük ve hızla sönüyor (ek ≈ +0,02 → +0,005 → +0,002); %25 sütun payı %25 benzersiz bilgi taşımıyor: toplam açıklanan varyans (≈0,29–0,32) tek F\'nin (≈0,40–0,42) altında, yani toplam "%100\'e yaklaşma" ölçütünde zincir tek F\'nin gerisinde.
- [Kesin] Y hedefli zincirde F₂..F₄ eğitim PPL\'ini düşürüyor ama doğrulama/test PPL\'ini kötüleştiriyor: ek aşamalar eğitime uyuyor, genelleme kazandırmıyor (eğitim–doğrulama farkı büyüyor).
- [Muhtemel] Neden: (a) F₁ yalnız h/4 yön alıyor, ham girdi X\'ten yön sayısı azalıyor (kapasite bölünüyor); (b) F₂..F₄ girdisi tanh çıktıları olduğundan yeni bilgi eklemiyor, F₁\'in doğrusal olmayan yeniden karışımı; ham X ve kelime kimliği bilgisi (V25: eksik bilginin yeri) hiçbir aşamaya girmiyor. Bu açıklama ölçülmedi, doğrulanması için aşama başına X\'in de verildiği (skip) varyant gerekir.
- [Kesin] Ortak tek ridge, aşamalı (ardışık artık) çözümden Q açıklanan varyansı biraz artırıyor (çapraz ilişkiler) ama tek F düzeyine ulaşmıyor.
''')
P('''## 5. Sınırlamalar

- γ₂=1 ve λ (V17\'den) F₂..F₄ için ayrıca seçilmedi; aşama başına λ/γ taraması yapılırsa sonuç değişebilir. Yön sayısı h/4 sabit (tek F ile aynı toplam sütun).
- Tohum sayısı tablo başlıklarındaki n; test n=4 980. Q hedefi leave-one-out trigram öğreticidir (V25); Q-zincirin test PPL\'i etiketle ölçülür, ama Q hedefi gerçek öğrenme değil öğretici bilgisinin aktarımı ölçümüdür.
- Süre/RAM ölçümleri aşağıda; çekirdekler başka iş çalıştırmadı (tek süreç).
- Zincir bir "yeni mimari" değildir; ana algoritmaya eklenmedi.
''')
P('## 6. Süre ve bellek\n')
P('| komut | wall s | user CPU s | RAM MB |\n|---|---|---|---|')
for mm in man:
    kv=dict(x.split('=') for x in mm[2:]); P(f"| `{mm[0]}` | {kv['wall_s']} | {kv['user_cpu_s']} | {kv['maxrss_MB']} |")
ts=[(H,s,R[(H,s)]['time']) for (H,s) in sorted(R)]
P('\nZincir başına süre (özellik üretimi + Gram + RHS, sonra aşama çözümleri+değerlendirme), tohum 1: ' + '; '.join(f'H={H}: '+', '.join(f"{k.split('(')[0]}={v if isinstance(v,float) else v}" for k,v in t.items()) for H,s,t in ts if s==1))
rep=rep.replace('PLACEHOLDER','\n'.join(out))
open(base+'TEK_HAMLE_V27_RAPOR.md','w').write(rep+'\n')
