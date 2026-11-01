#!/usr/bin/env python3
# Plan Bolum 01 (genis degerler icin mikro-program dongusu ve kazima alani): mikro-islemler
#   lpb            : dongu basi (konum isaretlenir)            lpe ti : gecici ti sifir degilse dongu basina don (tek seviye, mikro-program ici)
#   spld to,n      : kazima alani sozcugu n (0..63) -> gecici   spst n,ti : gecici -> kazima alani sozcugu n
# Kazima alani komut ornegi boyunca yasar (komutlar arasi kalici degil); ELF'te yigin isaretcisinin altinda sabit konumlar, svm'de ayri dizi.
# x86: MARK_Fk (konum kaydet), RELM_Fk (kayitli konuma bagil 32 bit), SCR_Fk (kazima sozcugu diski: -(144+8n))
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('prim ') and ' lpb ' in l for l in L): sys.exit('tur9_ekle: tablo zaten genisletilmis')
def last(prefix): return max(i for i, l in enumerate(L) if l.startswith(prefix))
pl = last('prim '); pid = int(L[pl].split()[1])
newprim = ['prim %d lpb - ext 1=0' % (pid + 1), 'prim %d lpe ti ext 1=0' % (pid + 2), 'prim %d spld to,n ext 1=0' % (pid + 3), 'prim %d spst n,ti ext 1=0' % (pid + 4)]
tl = last('token ')
tok = ['token %d MARK_F%d 4' % (448 + k, k) for k in range(4)] + ['token %d RELM_F%d 4' % (452 + k, k) for k in range(4)] + ['token %d SCR_F%d 4' % (456 + k, k) for k in range(4)]
tpl_l = last('tpl prim ')
tpl = ['tpl prim lpb : MARK_F0', 'tpl prim lpe : 48 8B 44 24 TMP_F0 48 85 C0 0F 85 RELM_F0', 'tpl prim spld : 48 8B 84 24 SCR_F1 48 89 44 24 TMP_F0', 'tpl prim spst : 48 8B 44 24 TMP_F1 48 89 84 24 SCR_F0']
ins = {pl: newprim, tl: tok, tpl_l: tpl}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
