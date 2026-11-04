exec(open('/tmp/v30gen.py').read().split("P('PLACEHOLDER')")[0])
rep=rd(base+'TEK_HAMLE_V30_RAPOR.md')
out=[];P=out.append
# id-only referans (v26c, mod E, lam=1, seed 1)
idl={}
for l in rd(L+'v30_C_idonly_ref_H512_s1.txt').replace('\\n','\n').splitlines():
    if l.startswith('READ|') and '|DOGRUSAL|' in l:
        f=l.split('|'); te=re.search(r'test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',l).groups(); idl[f[2]]=[float(x) for x in te]
REF=asama(L+'v30_REF_V29ayar_sID1_lam1_H512_s1.txt')
refA=[r for r in REF if r['name'].startswith('ORTAK_RIDGE_V28') and r['tg']=='Y'][0]; refB=[r for r in REF if r['name'].startswith('ORTAK_RIDGE_V29') and r['tg']=='Y'][0]
f1=[f for f in F[1] if f['tg']=='Y'][0]
P('## 4. Adil mimari karşılaştırması (H=512 toplam özellik boyutu, doğrusal okuma)\n')
P('| sistem | test PPL | Top-1 / Top-5 | kaynak / protokol |'); P('|---|---|---|---|')
P('| Tek F (512+1) | 182,5 | 14,8% / 33,3% | V26–V28 3 tohum ortalaması (bu koşuda yeniden ölçülmedi) |')
P(f'| 4 F, kimliksiz, ortak ridge | 176,5 (3 tohum) / **{refA["te"][1]:.1f}** (tohum 1, bu programda yeniden ölçüldü) | {100*refA["te"][2]:.1f}% / {100*refA["te"][3]:.1f}% (tohum 1) | V28/V29 + yeniden ölçüm |')
P(f'| 4 F + sıralı kimlik (V29 ayarı s_ID=1, λ=1) | 166,3 (3 tohum) / **{refB["te"][1]:.1f}** (tohum 1, yeniden ölçüldü) | {100*refB["te"][2]:.1f}% / {100*refB["te"][3]:.1f}% (tohum 1) | V29 + yeniden ölçüm |')
P(f'| **V30 optimize (s_ID=8, λ=20), 3 tohum** | **{FINAL:.1f}** | {100*avg([f["te"][2] for f in fy]):.1f}% / {100*avg([f["te"][3] for f in fy]):.1f}% | bu rapor, bölüm 3 |')
P(f'| Yalnız kimlik (F yok, ridge, λ=1, tohum 1) | {idl.get("Y",[0,float("nan")])[1]:.1f} (W_Y) / {idl.get("Q",[0,float("nan")])[1]:.1f} (W_Q) | {100*idl.get("Y",[0,0,0,0])[2]:.1f}% / {100*idl.get("Y",[0,0,0,0])[3]:.1f}% | V26c `E` modu yeniden koşuldu; **n-gram tablosu benzeri, genelleme başarısı sayılmaz** |')
P('| Trigram (n-gram) | 138,9 | 16,0% / 35,8% | V24/V25 |')
P('')
P('| Yalnız kimlik (F yok, ridge, **λ=10**, tohum 1) | 135,7 (W_Y) / 137,5 (W_Q) | 16,3% / 35,4% (W_Y) | V26c `E` modu, H=1024 koşusunun C0 satırı (kimlik-yalnız modelde F yok, H\'den bağımsız; aynı doğrusal okuma, doğrulamayla). **n-gram tablosu benzeri** |')
P('\n**Önemli:** kimlik-yalnız ridge λ\'ya duyarlı (λ=1: 152,2; λ=10: 135,7). Ayarlanmış kimlik-yalnız model (135,7) V30\'dan (153,4) **18 PPL daha iyi** ve trigrama (138,9) eşit/yakın. Yani dört F\'nin matematiksel özellikleri, kimlik bilgisini seyrelterek bu n-gram benzeri tabanın **altında** kalıyor; V30\'un F\'li yapı olarak kazancı V29\'a göre büyük, ama ham kimlik tablosunu geçmiyor. [Kesin: tohum 1 karşılaştırması]\n')
# kategori
K=kat(L+'v30_REF_V29ayar_sID1_lam1_H512_s1.txt')+kat(L+'v30_FINAL_sID8_lam20_H512_s1.txt')
P('## 5. Görülmüş / görülmemiş bağlamlar (test; tohum 1; Y hedefi, ortak ridge)\n')
P('Test satırlarının %68,1\'inde (t−2,t−1) trigram bağlamı eğitimde görülmüş (3 392 satır), %31,9\'unda yalnız bigram bağlamı görülmüş (1 588 satır). Trigram n-gram: görülen 4,928 / yalnız bigram 4,945 nat (V24).\n')
P('| model | trigram bağlamı GÖRÜLDÜ: CE (PPL) / Top-1 | yalnız bigram görüldü (trigram bağlamı görülmedi): CE (PPL) / Top-1 |'); P('|---|---|---|')
for nm,key in (('4F kimliksiz (V28 ayarı)','V28'),('4F + kimlik, V29 ayarı (s_ID=1, λ=1)','V29'),('V30 (s_ID=8, λ=20)','FINAL')):
    if key=='FINAL': ks=[k for k in K if k['name'].startswith('ORTAK') and k['tg']=='Y' and 'KIMLIK' in k['name'] and k['n'] and False]
    pass
def getk(path,prefix):
    ks=[k for k in kat(path) if k['name'].startswith(prefix) and k['tg']=='Y']
    s=[k for k in ks if k['cat'].endswith('GORULDU')][0]; u=[k for k in ks if 'gorulmedi' in k['cat']][0]; return s,u
for nm,path,prefix in (('4F kimliksiz (V28 ayarı)',L+'v30_REF_V29ayar_sID1_lam1_H512_s1.txt','ORTAK_RIDGE_V28'),('4F + kimlik, V29 ayarı (s_ID=1, λ=1)',L+'v30_REF_V29ayar_sID1_lam1_H512_s1.txt','ORTAK_RIDGE_V29'),('**V30** (s_ID=8, λ=20)',L+'v30_FINAL_sID8_lam20_H512_s1.txt','ORTAK_RIDGE_V29')):
    s,u=getk(path,prefix); P(f'| {nm} | {s["ce"]:.4f} ({s["ppl"]:.1f}) / {100*s["t1"]:.1f}% | {u["ce"]:.4f} ({u["ppl"]:.1f}) / {100*u["t1"]:.1f}% |')
P('')
# H1024
P('## 6. H=1024 (4×256), tohum 1 (zaman kaldığı için tek koşu; γ₁=4, s_ID=8, λ=20 — H=1024 için ayrıca seçilmedi)\n')
if os.path.exists(L+'v30_FINAL_sID8_lam20_H1024_s1.txt'):
    h=asama(L+'v30_FINAL_sID8_lam20_H1024_s1.txt'); hy=[x for x in h if x['tg']=='Y'][0]; hq=[x for x in h if x['tg']=='Q'][0]
    P(f'Test PPL (Y) **{hy["te"][1]:.1f}** (Top-1 {100*hy["te"][2]:.1f}% / Top-5 {100*hy["te"][3]:.1f}%), eğitim / doğr. PPL {hy["tr"]:.0f} / {hy["va"]:.0f}, eğitim–doğr. CE farkı {hy["gap"]:.3f}; Q açıklanan varyans (test) {hq["qe"]:.3f}. Karşılaştırma: V28 H=1024 ortak ridge 183,4 (3 tohum), tek F 192,2. Bu H=1024 sonucu seçilmiş hiperparametre içermez ve tek tohumdur.\n')
else: P('Tamamlanmadı.\n')
P('## 7. Sonuç\n')
P(f'- **3 tohum, H=512, doğrulamayla seçilmiş (s_ID=8, λ=20): test PPL {FINAL:.1f}** (V29: 166,3; V28: 176,5; tek F: 182,5). Kazanç 166,3\'e göre {166.3-FINAL:.1f} PPL; bunun kaynağı öncelikle s_ID (λ tek başına: bölüm 1, sID=1 iken λ=1→10 test 163,8→160,7 yani ≈3 PPL, s_ID=1→8: ≈8 PPL, tohum 1).')
P('- [Kesin] λ optimumu ızgara içinde (10–20); s_ID optimumu ızgara dışına (8) uzandı — kimlik katkısının V29 ölçeğinde (1) **hafife alındığı** görülüyor. Daha büyük s_ID doğrulamada tekrar kötüleşiyor (16).')
P('- [Kesin] Hedef "trigram 138,9\'a yaklaşma" **sağlanmadı**: V30 ≈ 15 PPL üstünde; yalnız-kimlik ridge (F yok) V25\'te ≈137 verdi, yani F\'li model kimlik bilgisini seyreltiyor (tanh, ortak λ).')
P('- [Kesin] V30\'un Q açıklanan varyansı (0,313) V29\'dan (0,433) **düştü**, çünkü seçim Y hedefine göre yapıldı (λ=20, s_ID=8); Y için iyi olan ayar trigram dağılımını daha az doğrusal olarak yeniden üretiyor. Q-hedefli PPL ise 156,7. Açıklanan varyans ile PPL burada da birlikte hareket etmiyor.')
P('- **Kimlik n-gram belleğidir:** s_ID büyüdükçe model kimlik (sayım) bilgisine daha çok yaslanıyor ve Top-1 düşüyor (bölüm 2: s_ID 1→8, λ=10: Top-1 15,1→14,5%) ama PPL düşüyor; bu ezber/sayım bilgisinin daha iyi kullanılmasıdır, bağımsız genelleme değildir.')
P('''
## 8. Sınırlamalar

- Seçimler (λ, s_ID) yalnız H=512, tohum 1 doğrulamasında; diğer tohumlar ve H=1024 aynı değerlerle koşuldu. γ₁, γ₂ değişmedi (V29: γ₁ V17, γ₂=0,5).
- Üç hedefe (Y/Q) ayrı seçim yapılmadı; seçim Y\'ye göre. Q sonuçları öğretici aktarımıdır, bağımsız öğrenme değil.
- Kimlik ölçeği yalnız tek çarpan; F₁ ve F₂..F₄ girdisi için ayrı çarpanlar, X bloğu için ayrı ölçek denenmedi. Kimlik çiftleri (hash) V29\'da yardım etmedi, bu çalışmada tekrarlanmadı.
- Süre: λ-sweep aynı özellikler üzerinde çok λ için tek Gram ile yapıldı (λ başına ≈ 3 s çözüm + değerlendirme). Ağırlık üretim süresi/RAM aşağıda.
- V30 ana mimariye eklenmedi.
''')
P('## 9. Süre ve bellek\n')
man=[l.split('\t') for l in rd(L+'manifest.tsv').splitlines()]
P('| komut | wall s | user CPU s | RAM MB |\n|---|---|---|---|')
for mm in man:
    kv=dict(x.split('=') for x in mm[2:]); P(f"| `{mm[0]}` | {kv['wall_s']} | {kv['user_cpu_s']} | {kv['maxrss_MB']} |")
chol=re.findall(r'CHOL_COZUM\|lam=(\S+)\|cholesky\+cozum suresi=([\d.]+)s',rd(L+'v30_FINAL_sID8_lam20_H512_s1.txt'))
P(f'\nCholesky + çözüm süresi (513×513 sistem, V=2001 sağ taraf; H=512 tohum 1): {chol[0][1]} s; kimlik dahil F üretimi + Gram + RHS: `ZINCIR` satırlarında (v30_ham).')
rep=rep.replace('PLACEHOLDER','\n'.join(out))
open(base+'TEK_HAMLE_V30_RAPOR.md','w').write(rep+'\n')
