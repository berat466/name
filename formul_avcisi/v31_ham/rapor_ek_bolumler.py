exec(open('/tmp/v31gen.py').read().split("P('PLACEHOLDER')")[0])
rep=rd(base+'TEK_HAMLE_V31_RAPOR.md')
out=[];P=out.append
# pair-ID referans
pr={}
for f in sorted(glob.glob(L+'v31_pairID_ref_lam*_s1.txt')):
    lam=re.search(r'lam(\d+)_s1',f).group(1)
    for l in rd(f).replace('\\n','\n').splitlines():
        if l.startswith('READ|') and '|DOGRUSAL|' in l and '|Y|' in l:
            va=re.search(r'dogr CE=([\d.]+) PPL=([\d.]+)',l).groups(); te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups(); pr[lam]=(float(va[0]),float(va[1]),[float(x) for x in te])
P('## 4. Referans netleştirmesi: V30\'daki "yalnız kimlik 135,7" hangi özelliklerdi?\n')
P('V25/V26c\'nin `C0` sistemi `[ID(w_{t−1}) (2001), en sık 2000 (w_{t−2},w_{t−1}) **çift** kimliği]` idi; yani **sıralı çift (trigram bağlamı) bilgisi** içeriyordu. V31\'in `C`\'si ise görevdeki gibi yalnız `[ID(w_{t−2}),ID(w_{t−1})]` (çift kimliği yok). İkisi farklı özellik kümeleri; V30 raporunda "yalnız kimlik" etiketi bu farkı belirtmiyordu. Aynı koşulda yeniden ölçüm (tohum 1, v26c `E` modu, doğrusal okuma, λ doğrulamayla):\n')
P('| λ | doğrulama CE (PPL) | test PPL | Top-1 / Top-5 |\n|---|---|---|---|')
for lam in sorted(pr,key=int): P(f'| {lam} | {pr[lam][0]:.4f} ({pr[lam][1]:.1f}) | {pr[lam][2][1]:.1f} | {100*pr[lam][2][2]:.1f}% / {100*pr[lam][2][3]:.1f}% |')
bl=min(pr,key=lambda k:pr[k][0])
P(f'\nDoğrulamayla seçilen λ={bl}: test PPL **{pr[bl][2][1]:.1f}** (çift kimlikli, n-gram tablosu benzeri). Bu, V31\'in `C` (çiftsiz) yalnız-kimlik referansından (bölüm 3) farklıdır; **adil V31 karşılaştırması çiftsiz C ile yapılır.** Çift kimlikli "C+çift" ortak çözüm bu raporda denenmedi (süre).\n')
P('## 5. Kimlik ve F\'nin ayrı katkısı (ortak çözümün tohum 1 logitleri; aynı W)\n')
r1=F[1]
P('| okuma | test PPL | Top-1 / Top-5 | eğitim–doğr. CE farkı |\n|---|---|---|---|')
for tag,nm in ((CB,'tam ortak: C+F'),(PC,'yalnız C kısmı (C·W_C)'),(PF,'yalnız F kısmı (F·W_F)'),(ID,'ayrı yalnız-kimlik çözümü (λ_C=20)')):
    r=r1[tag]; P(f'| {nm} | {r["te"][1]:.1f} | {100*r["te"][2]:.1f}% / {100*r["te"][3]:.1f}% | {r["gap"]:.3f} |')
P('\nParçalar kendi α,ε\'larıyla (doğrulamayla) okundu; C kısmı tek başına yalnız-kimlik çözümünden daha **kötü** (ortak çözümde F, kimlik ağırlıklarını da değiştirir: `W_C` ortak çözümün düzeltilmiş hâli); F kısmı tek başına zayıf. Birlikte daha iyi: F, kimliğin açıklayamadığı kalan bilgiyi tamamlıyor.\n')
P('## 6. F gerçekten yeni bilgi ekliyor mu? Görülmüş / görülmemiş bağlamlar\n')
P('| tohum | yalnız kimlik C test PPL | V31 kimlik+4F test PPL | **F\'nin test kazancı** | bağlam GÖRÜLDÜ: C → C+F (PPL) | yalnız bigram: C → C+F (PPL) |\n|---|---|---|---|---|---|')
for s in sorted(F):
    a,b=F[s][ID],F[s][CB]; P(f'| {s} | {a["te"][1]:.1f} | {b["te"][1]:.1f} | {a["te"][1]-b["te"][1]:+.1f} PPL ({100*(a["te"][1]-b["te"][1])/a["te"][1]:.1f}%) | {a["cs"][0]} → {b["cs"][0]} | {a["cb"][0]} → {b["cb"][0]} |')
P(f'| **ort.** | {avg([F[s][ID]["te"][1] for s in F]):.1f} | {avg([F[s][CB]["te"][1] for s in F]):.1f} | {avg([F[s][ID]["te"][1]-F[s][CB]["te"][1] for s in F]):+.1f} PPL | | |')
P('')
P(f'- [Kesin] Ortak çözümde F, yalnız-kimliğe göre ortalama {avg([F[s][ID]["te"][1]-F[s][CB]["te"][1] for s in F]):.1f} PPL kazandırıyor; kazanç hem trigram bağlamı görülen hem görülmeyen (yalnız bigram) satırlarda var. Sonuç **136\'nın altına inmedi**: ortalama {avg([F[s][CB]["te"][1] for s in F]):.1f}. (136 referansı çift kimlikli farklı bir yalnız-kimlik setiydi, bölüm 4; çiftsiz C ile referans {avg([F[s][ID]["te"][1] for s in F]):.1f}.)')
P('- [Kesin] V30\'a göre (153,4; kimlik F\'lerin içinde, seyrek ağırlıkla) V31 ayrı düzenlileştirmeyle belirgin iyileşti: kimliği F içinde seyreltmek yerine doğrudan korumak ve F\'yi güçlü düzenlileştirmek (λ_F≈3000 ≫ λ_C=100) kazandırdı. F neredeyse "küçük düzeltme" olarak kullanılıyor; λ_F=1e9 (F kapalı) ve λ_F=1 (F serbest) ikisi de daha kötü.')
P('- [Kesin] Eğitim PPL yalnız-kimlikte ≈39 → ortak ≈52: ortak çözüm eğitimde daha az ezberliyor, doğrulama/test daha iyi. Kimlik n-gram sayımlarına dayanır; bu **yeni bağımsız genelleme olarak sunulmuyor**.\n')
P('## 7. Sayısal doğruluk, süre, bellek\n')
for l in rd(L+'v31_sweep2_H512_s1.txt').splitlines():
    if l.startswith('COZUM_REZIDUSU'): P('- Çözüm rezidüsü (seçilen ortak çözüm, tohum 1): '+l.replace('COZUM_REZIDUSU|||','').strip()+'. Tüm okumalarda olasılık toplamı sapması ≤ 1e-13.')
sch=re.findall(r'SURE\|lamC=(\S+) Cholesky\(C blogu\)\+Schur hazirlik=([\d.]+)s',rd(L+'v31_sweep_H512_s1.txt'))
P('- Süre (tohum 1, tek süreç değil: bölüm 4 kontrolü ile ardışık): C-blok Cholesky (4002²) + Schur hazırlığı ≈ '+', '.join(f'λ_C={a}: {b}s' for a,b in sch)+'; her λ_F için 513×513 çözüm + `W_C` yeniden yapımı ≈ 0,4–1,0 s. F zinciri üretimi + Gram ≈ 1,3 s.')
man=[l.split('\t') for l in rd(L+'manifest.tsv').splitlines()]
P('\n| komut | wall s | user CPU s | RAM MB |\n|---|---|---|---|')
for mm in man:
    kv=dict(x.split('=') for x in mm[2:]); P(f"| `{mm[0]}` | {kv['wall_s']} | {kv['user_cpu_s']} | {kv['maxrss_MB']} |")
P('''
## 8. Sınırlamalar

- Seçim yalnız tohum 1 doğrulamasında; tohum 2–3 yalnız λ_C=100, λ_F=3000 (ve yalnız-kimlik için kendi doğrulamasıyla λ_C∈{10,20,100}) ile koşuldu. H=1024 yapılmadı. Ortak sistemde C+çift (hash/sık çift) yok. γ₁, γ₂, F zinciri değişmedi.
- F zinciri girdisinde kimlik yok (görev tanımı gereği); V30\'daki "F girdisine kimlik" ile bu çalışmada F\'nin kimlikten bağımsız olması karşılaştırılmadı (aynı program içinde).
- Q hedefi/açıklanan varyans ölçülmedi (yalnız Y). Yalnız-kimlik ve ortak çözüm n-gram sayımlarına dayalıdır.
- Yeni yapı ana mimariye eklenmedi.
''')
summ = '''## 0. Özet (H=512, doğrusal okuma, test)

- [Kesin] **Çiftsiz kimlik C=[ID(w_{t−2}),ID(w_{t−1})] yalnız ridge: 144,1 PPL** (tohumdan bağımsız, çünkü rastgele özellik yok; λ_C=20 doğrulamayla). Ortak çözüm **kimlik + dört F: 140,3 PPL** (3 tohum: 139,8 / 141,2 / 139,9; λ_C=100, λ_F=3000). F\'nin eklediği gerçek test kazancı ≈ 3,8 PPL (%2,6), hem trigram bağlamı görülen hem görülmeyen satırlarda.
- [Kesin] V30\'a (153,4) göre büyük iyileşme; **136\'nın altına inilemedi** (140,3). V30\'da anılan "yalnız kimlik 135,7" referansı **çift (trigram bağlamı) kimliği** içeren farklı bir özellik kümesiydi; aynı koşulda yeniden ölçüm (bölüm 4): 133,2 (λ=20). Yani V31\'in çiftsiz C+4F\'si, çift kimlikli yalnız-kimlik tablosunun gerisinde: trigram-bağlamı kimliği, 4 F\'nin matematiksel bilgisinden daha değerli çıktı.
- [Kesin] Kontrol: λ_F→∞ (F kapalı) yalnız-kimlik sonucuna aynen dönüyor (doğrulama CE eşit); ayrı düzenlileştirme ve Schur çözümü doğru çalışıyor (rezidü 4e-7).
- Kimlik n-gram sayımlarına dayanır; bu sonuçlar bağımsız genelleme başarısı olarak sunulmuyor.

'''
rep=rep.replace('PLACEHOLDER','\n'.join(out)).replace('## 1. Formül ve hesap', summ+'## 1. Formül ve hesap',1)
open(base+'TEK_HAMLE_V31_RAPOR.md','w').write(rep+'\n')
