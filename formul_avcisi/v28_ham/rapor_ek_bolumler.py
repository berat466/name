exec(open('/tmp/v28gen.py').read().split("P('PLACEHOLDER')")[0])
rep=rd(base+'TEK_HAMLE_V28_RAPOR.md')
out=[];P=out.append
P('## 6. Kritik sorular\n')
for H in (512,1024):
    ns=len([1 for (h,s) in sel if h==H])
    if not ns: continue
    sy=m(pick(H,'TEK_F','Y','F1..F1'),lambda e:e['te'][1]); sq=m(pick(H,'TEK_F','Q','F1..F1'),lambda e:e['te'][1]); eq1=m(pick(H,'TEK_F','Q','F1..F1'),lambda e:e['q'][1])
    v27y=m(pick(H,'V27_X_YOK','Y','F1..F4'),lambda e:e['te'][1]); v28y=m(pick(H,'V28_X_BAG','Y','F1..F4'),lambda e:e['te'][1]); v28y1=m(pick(H,'V28_X_BAG','Y','F1..F1'),lambda e:e['te'][1])
    v27q=m(pick(H,'V27_X_YOK','Q','F1..F4'),lambda e:e['q'][1]); v28q=m(pick(H,'V28_X_BAG','Q','F1..F4'),lambda e:e['q'][1])
    jy=m(pick(H,'ORTAK','Y'),lambda e:e['te'][1]); jq=m(pick(H,'ORTAK','Q'),lambda e:e['q'][1]); jqp=m(pick(H,'ORTAK','Q'),lambda e:e['te'][1])
    ek=[m(pick(H,'V28_X_BAG','Q',f'F1..F{k}'),lambda e:e['q'][3]) for k in range(1,5)]
    tr_s=m(pick(H,'TEK_F','Y'),lambda e:e['trp']); tr_c=m(pick(H,'V28_X_BAG','Y','F1..F4'),lambda e:e['trp'])
    v_s=m(pick(H,'TEK_F','Y'),lambda e:e['vap']); v_c=m(pick(H,'V28_X_BAG','Y','F1..F4'),lambda e:e['vap'])
    P(f'**H={H} (n={ns} tohum)**\n')
    P(f'1. *X\'i yeniden vermek V27 kaybını azaltıyor mu?* Y hedefi, F₁..F₄ test PPL: V27 {v27y:.1f} → V28 {v28y:.1f} (tek F {sy:.1f}; yalnız F₁ {v28y1:.1f}). Q açıklanan varyans (test): V27 {v27q:.3f} → V28 {v28q:.3f} (tek F {eq1:.3f}).')
    P(f'2. *F₃, F₄ yeni bilgi açıklıyor mu?* V28 Q-zincirde ek açıklanan varyans (test) F₂ {ek[1]:+.3f}, F₃ {ek[2]:+.3f}, F₄ {ek[3]:+.3f} (F₁ {ek[0]:.3f}).')
    P(f'3. *%40–43 sınırı aşılıyor mu?* En yüksek açıklanan varyans (test): bağlı zincir {v28q:.3f}, ortak ridge {jq:.3f}, tek F {eq1:.3f}. ' + ('Sınır aşılmadı.' if max(v28q,jq)<=max(eq1,0.43) else 'Tek F\'yi aştı.'))
    P(f'4. *Eğitim iyileşmesi testte de görülüyor mu?* Y hedefi eğitim PPL: tek F {tr_s:.0f}, V28 zinciri {tr_c:.0f}; doğrulama: {v_s:.0f} vs {v_c:.0f}; test: {sy:.1f} vs {v28y:.1f}.')
    tt=[sel[(h,s)]['tot'] for (h,s) in sel if h==H]
    P(f'5. *Tek F\'den başarılı ve hızlı mı?* Ortak ridge (E) Y test PPL {jy:.1f} (tek F {sy:.1f}). Süre (zincir başına toplam, hazırlık+iki hedef+değerlendirme; tohum ort.): tek F {avg([t["TEK_F(H=%d)"%H] for t in tt]):.1f} s, V28 zinciri {avg([t["V28_X_BAGLANTILI_4F(4x%d)"%(H//4)] for t in tt]):.1f} s, V27 {avg([t["V27_X_YOK_4F(4x%d)"%(H//4)] for t in tt]):.1f} s, ortak ridge {avg([t["ORTAK_RIDGE_X_BAGLANTILI_4F(4x%d)"%(H//4)] for t in tt]):.1f} s (değerlendirme ağırlıklı; zincir 4 aşama değerlendirdiği için daha uzun).\n')
P('''## 6b. Ana soruya cevap

- [Kesin] **X\'i F₂–F₄\'e yeniden vermek V27 kaybını büyük ölçüde giderdi (Y hedefi):** H=512 V27 191,5 → V28 180,7; H=1024 195,6 → 185,2 (3 tohum). V27\'de ek aşamalar eğitim PPL\'ini düşürüp doğrulama/testi bozuyordu; V28\'de her aşama doğrulama ve testi de iyileştiriyor (H=512 test: 186,4 → 182,9 → 181,6 → 180,7).
- [Kesin] **Aynı toplam sütunla tek F (λ=V17 seçimi) geçildi:** H=512 tek F 182,5 → V28 zinciri 180,7 (−1,8), ortak ridge **176,5** (−6,0); H=1024 192,2 → 185,2 (−7,0), ortak ridge 183,4 (−8,8). Ortak tek ridge, ardışık artık çözümden daha iyi (çapraz ilişkiler önemli); V28 zincirin Top-1/Top-5 değeri tek F\'nin biraz altında (Top-1 13,9 vs 14,8; Top-5 32,3 vs 33,3, H=512).
- [Kesin, **önemli uyarı**] Bu kazancın ne kadarının yapıdan, ne kadarının λ uyumsuzluğundan geldiği **ayrılmadı**: λ ve γ₁ softmax okuması için V17\'de seçilmişti; V25/V26c\'de aynı H=512 tek F, doğrusal okumada λ=100 ile (tohum 1) 171,6 PPL vermişti (bu çalışmadaki E tohum 1: 180,9; ortalama 176,5). Yani **ayarlanmış tek F, V28\'in ortak ridge\'inden daha iyi olabilir**; E ve zincir için λ taranmadı. Bu raporda "yapı tek F\'yi yendi" sonucu yalnız **aynı λ** için geçerlidir.
- [Kesin] **%40–43 sınırı aşılmadı:** Q açıklanan varyans (test) en yüksek H=512 ortak ridge 0,357, H=1024 0,351; tek F 0,397 / 0,430. Bağlı zincir 0,311 / 0,320. F₃, F₄ ek açıklanan varyans: +0,014/+0,010 (H=512), +0,007/+0,006 (H=1024) — küçük ama sıfır değil (V27: +0,008/+0,001).
- [Muhtemel] X erişimi, sonraki aşamaların ham girdideki bilgiyi yeniden kullanmasını sağlıyor; kazancın bir kısmı ayrıca düşük γ₂ (0,5) ile daha az doygun tanh\'tan gelebilir. Bunu ayıran kontrol yapılmadı.
- Hız: zincirler tek F\'den yavaş (tohum ort. toplam 16,6 s vs 9,5 s H=512; çoğu 4 aşamalı değerlendirme), saf ağırlık üretimi 1–4 s/aşama. [Kesin: loglar]
''')
P('''## 7. Sonuç ve sınırlamalar

- [Kesin] Sonuçlar yukarıdaki tablolardadır; hiçbir sistem için "başarı" varsayılmadı. Karşılaştırma tek F ile aynı toplam sütun sayısında yapıldı (4×h/4 + sabit sütun).
- Ölçülen şey yeni veri değil, mevcut bağlam bilgisinin daha iyi temsil edilmesidir: F'ler aynı X'ten ve önceki F'lerden türetilir.
- γ₂ yalnız H=512 tohum 1'de doğrulamayla seçildi (ızgara {0.25,0.5,1,2}) ve diğer tüm koşulara taşındı; seçim ızgara kenarında olabilir (bölüm 4). Aşama başına ayrı λ/γ taraması yapılmadı. Ölçek yöntemi tek (satır-norm RMS); alternatif ölçeklendirme denenmedi.
- Q hedefli sonuçlar öğretici bilgisinin aktarımını ölçer; bağımsız öğrenme başarısı olarak okunmamalıdır.
- Süreler tek süreç içinde, ancak değerlendirme (4 aşamalı okuma + Q açıklanan varyans) dahil; saf ağırlık üretimi süreleri ilgili logların `asama sure` alanlarında (aşama başına 0,6–4 s).
- Yeni yapı ana mimariye eklenmedi.
''')
P('## 8. Çalıştırma ve süre/RAM\n')
P('Derleme: `g++ -O3 -march=native -fopenmp -std=c++17 cpp/tek_hamle_v28.cpp -o tek_hamle_v28`; komut: `./tek_hamle_v28 2 H TOHUM GAMMA1 LAMBDA GAMMA2 SCVJ` (S tek F, C V28 X-bağlantılı zincir, V V27 yeniden üretimi, J ortak ridge).\n')
P('| komut | wall s | user CPU s | RAM MB |\n|---|---|---|---|')
for mm in man:
    kv=dict(x.split('=') for x in mm[2:]); P(f"| `{mm[0]}` | {kv['wall_s']} | {kv['user_cpu_s']} | {kv['maxrss_MB']} |")
rep=rep.replace('PLACEHOLDER','\n'.join(out))
open(base+'TEK_HAMLE_V28_RAPOR.md','w').write(rep+'\n')
