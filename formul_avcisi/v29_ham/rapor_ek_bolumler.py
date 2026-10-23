exec(open('/tmp/v29gen.py').read().split("P('PLACEHOLDER')")[0])
rep=rd(base+'TEK_HAMLE_V29_RAPOR.md')
out=[];P=out.append
A=pick('ORTAK_RIDGE_V28_KIMLIK_YOK(4x128)','Y','F1..F4',True); B=pick('ORTAK_RIDGE_V29_KIMLIK_SIRALI(4x128)','Y','F1..F4',True); C=pick('ORTAK_RIDGE_V29_KIMLIK_SIRALI+CIFT(4x128)','Y','F1..F4',True)
Aq=pick('ORTAK_RIDGE_V28_KIMLIK_YOK(4x128)','Q','F1..F4',True); Bq=pick('ORTAK_RIDGE_V29_KIMLIK_SIRALI(4x128)','Q','F1..F4',True)
cA=pick('V28_KIMLIK_YOK(4x128)','Y','F1..F4',True); cB=pick('V29_KIMLIK_SIRALI(4x128)','Y','F1..F4',True)
P('## 5. Ana soruya cevap\n')
P(f'- **Ana sonuç (n={N} tohum):** kimlik + X + dört bağlantılı F, ortak ridge ile test PPL **{m(B,lambda e:e["te"][1]):.1f}** (kimliksiz aynı program {m(A,lambda e:e["te"][1]):.1f}; V28 raporu 176,5). Ardışık çözümle {m(cB,lambda e:e["te"][1]):.1f} (kimliksiz {m(cA,lambda e:e["te"][1]):.1f}). Çift hash kimliği eklemek ({m(C,lambda e:e["te"][1]):.1f}) sıralı kimlikten daha iyi değil. Top-5 {100*m(B,lambda e:e["te"][3]):.1f}%, Top-1 {100*m(B,lambda e:e["te"][2]):.1f}%.')
P(f'- [Kesin] Q hedefiyle açıklanan trigram varyansı (test) ortak ridge B {m(Bq,lambda e:e["q"][1]):.3f} (A {m(Aq,lambda e:e["q"][1]):.3f}; V25\'te tek F 0,40–0,43): kimlik girişi %40–43 sınırını aştı.')
P('- [Kesin] Kimlik her F aşamasının girdisine katıldı (F₁ girdisinde ölçek eşitlemesi, F₂..F₄\'te blok RMS=1); yalnız son çıktıya eklenmedi. Dört F ve ağırlık hesabı tek C++ programı içinde, kapalı form.')
P('- **Kimlik = n-gram belleği uyarısı:** kimlik one-hot\'ları (t−1, t−2) kelime ve bigram sayım bilgisini doğrudan taşır (V25: kimlik-yalnız ridge doğrusal okumayla ≈137 PPL, yani n-gram tablosunun yeniden ifadesi). Buradaki kazanç, kimliğin ezber/sayım bilgisini F\'lere taşımasıdır; **bağımsız genelleme kazancı olarak sunulmuyor**. Eğitim PPL\'inin düşmesine rağmen doğrulama/test de düştüğü için bu bilgi görülmemiş metinde de işe yarıyor, ama bu bilgi n-gram istatistiğidir.')
P('- [Muhtemel] Kimlik-yalnız ridge (V25) ≈137 PPL verirken burada ≈166: F\'lerin tanh ve λ ile kimlik bilgisini seyreltmesi, ayrıca λ\'nın doğrusal okuma için ayarlanmamış olması. Bu ayrıştırma yapılmadı.')
P('- [Kesin] Eğitim PPL belirgin düşük (≈70 vs 80), eğitim–doğrulama farkı büyük (bölüm 2); doğrulama ve test yine iyileşiyor.')
P('''
## 6. Sınırlamalar

- Yalnız H=512 (4×128); H=1024 yapılmadı. Süre sınırı 12 dk.
- λ, γ₁, γ₂ kimlik için yeniden seçilmedi (V17/V28'den); kimlik bloğunun ölçeği tek bir kural (enerji eşitleme). Alternatif ölçekler ve kimlik için ayrı λ denenmedi. V25'te tek F + kimlik λ=100 gibi büyük λ\'ların doğrusal okumada daha iyi olduğu görülmüştü; bu çalışma bunu taramadı.
- A (kimlik yok) bu programda yeniden ölçüldü ve V28 raporundaki 176,5 / 180,7 ile örtüştü (aynı γ₂=0,5, tohum ve protokol).
- Hash çift kimliği (4096 kova) çarpışmalı; en sık çiftleri kimliğe çeviren V25 yöntemi kullanılmadı.
- Q sonuçları öğretici aktarımıdır, bağımsız öğrenme değil. Test n=4 980.
- Yeni yapı ana mimariye eklenmedi.
''')
P('## 7. Süre ve bellek\n')
P('| komut | wall s | user CPU s | RAM MB |\n|---|---|---|---|')
for mm in man:
    kv=dict(x.split('=') for x in mm[2:]); P(f"| `{mm[0]}` | {kv['wall_s']} | {kv['user_cpu_s']} | {kv['maxrss_MB']} |")
tt=[R[s]['tot'] for s in R]
P('\nZincir başına toplam süre (özellik+Gram+RHS+iki hedef+değerlendirme; tohum ort.): '+'; '.join(f"{k.split('(')[0]}={avg([t[k] for t in tt]):.1f}s" for k in tt[0]))
P('\nSeyrek kimlik bloğu yoğun açılmadı: seyrek ağırlık (8098×128 float) ≈ 4 MB / aşama; tepe RAM tabloda (V28 H=512 ≈ 890 MB).')
rep=rep.replace('PLACEHOLDER','\n'.join(out))
open(base+'TEK_HAMLE_V29_RAPOR.md','w').write(rep+'\n')
