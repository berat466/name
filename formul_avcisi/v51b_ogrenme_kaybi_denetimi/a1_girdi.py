import re, numpy as np
from collections import Counter
txt=open('/tmp/pg1342.txt',encoding='utf-8').read(); a=txt.index("*** START OF"); b=txt.index("*** END OF"); body=txt[a:b]; start=body.index("It is a truth universally acknowledged"); body=body[start:]
low=body.lower(); toks_all=re.findall(r"[a-z]+(?:'[a-z]+)?",low); toks=toks_all[:50000]
print('== TOKENLESTIRME (metin_hazirla.py: kucuk harf + regex [a-z]+(\'[a-z]+)?)')
print('govde karakter sayisi %d ; kitap toplam token %d ; kullanilan ilk %d (kalan %d token kullanilmiyor = %.1f%% kitap)'%(len(body),len(toks_all),len(toks),len(toks_all)-len(toks),100*(len(toks_all)-len(toks))/len(toks_all)))
used_end=None
# 50000. tokenin bitis konumunu bul
pos=0; k=0
for m in re.finditer(r"[a-z]+(?:'[a-z]+)?",low):
    k+=1
    if k==50000: used_end=m.end(); break
seg=low[:used_end]
dig=len(re.findall(r'[0-9]',seg)); punct=len(re.findall(r'[^\sa-z0-9]',seg)); upper=sum(1 for ch in body[:used_end] if ch.isupper()); apos=len(re.findall(r"'",seg))
nonascii=sum(1 for ch in seg if ord(ch)>127)
print('kullanilan metinde: rakam %d, noktalama/ozel karakter %d (cumle siniri/virgul bilgisi tamamen atiliyor), buyuk harf %d (kucultuluyor -> "The"/"the", "Mr"/"mr" ayni), kesme isareti %d, ascii-disi %d'%(dig,punct,upper,apos,nonascii))
# buyuk/kucuk harf cakisan tur sayisi
raw=re.findall(r"[A-Za-z]+(?:'[A-Za-z]+)?",body[:used_end]); rc=Counter(raw); lc=Counter(w.lower() for w in raw)
coll=sum(1 for w,c in lc.items() if len({r for r in rc if r.lower()==w})>1) if len(lc)<4000 else -1
print('ham tur (buyuk/kucuk ayri) %d -> kucuk harf tur %d ; birlestirilen tur sayisi %d'%(len(rc),len(lc),len(rc)-len(lc)))
# sozluk: bloklara bol (metin_hazirla ile ayni)
BL=500; nb=100; rng=np.random.default_rng(0); perm=rng.permutation(nb); split={'train':sorted(perm[:80]),'val':sorted(perm[80:90]),'test':sorted(perm[90:])}
blk=lambda s:[toks[i*BL:(i+1)*BL] for i in split[s]]
cnt=Counter(w for b_ in blk('train') for w in b_); vocab=['<unk>']+[w for w,_ in cnt.most_common(2000)]; vs=set(vocab)
print('\n== SOZLUK: egitimde %d farkli tur; 2000 en sik tutuluyor + <unk>'%len(cnt))
oov_types=[w for w in cnt if w not in vs]; print('egitimde sozluk disi kalan tur: %d (egitim tokenlarinin %.2f%%); en sik sozluk-disi sayim %d; sozluk son kelime sayimi %d'%(len(oov_types),100*sum(cnt[w] for w in oov_types)/sum(cnt.values()),max(cnt[w] for w in oov_types),cnt[vocab[-1]]))
for s in ('train','val','test'):
    t=[w for b_ in blk(s) for w in b_]; unk=[w for w in t if w not in vs]
    print('%s: token %d ; UNK token %d (%.2f%%) ; UNK olan farkli tur %d ; egitimde hic gorulmeyen tur: %d token (%.2f%%)'%(s,len(t),len(unk),100*len(unk)/len(t),len(set(unk)),sum(1 for w in t if w not in cnt),100*sum(1 for w in t if w not in cnt)/len(t)))
cc=Counter(); 
# 'ayni temsile donusen farkli kelime': UNK sinifi
unk_types=set(w for b_ in blk('train')+blk('val')+blk('test') for w in b_ if w not in vs); print('Uc bolmede UNK sinifina giren toplam farkli tur:',len(unk_types))
tr=np.load('/tmp/v8data/ids_train.npy'); c=np.bincount(tr.ravel(),minlength=2001)
print('sozlukte egitim sayimi <=2: %d kelime, <5: %d, <10: %d, >=100: %d ; kelimelerin %%50 sikligi: en sik %d kelime tokenlarin %%50\'sini kaplar'%((c[1:]<=2).sum(),(c[1:]<5).sum(),(c[1:]<10).sum(),(c[1:]>=100).sum(),int(np.searchsorted(np.cumsum(np.sort(c)[::-1])/c.sum(),0.5))+1))
# baglam
print('\n== BAGLAM: c=2 (w_{t-2},w_{t-1}); her 500 tokenluk blokta ilk 2 token HEDEF OLMAZ (satir yok), blok sinirini asan baglam yok')
for s,n in (('train',80),('val',10),('test',10)): print('%s: token %d -> satir %d ; hedef olarak hic kullanilmayan token %d (%.2f%%)'%(s,n*500,n*498,n*2,100*2/500))
print('daha eski kelimeler (w_{t-3}..): X, C, F ve Q hicbirinde yok (c=2 sabit)')
