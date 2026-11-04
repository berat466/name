#!/usr/bin/env python3
"""Plan Bolum 01 (128 bit tamsayi <-> dar kayan nokta donusumleri): cv_i128_F cv_u128_F (d,q) F = f64 f32 f16 bf16 (tek yuvarlama) ; cv_f64_i128 cv_f64_u128 (Q,f) (f16/bf16/f32/f64 kaynaklari ayni komut).
Kayan -> tamsayi: sifira dogru kes; gecersiz (NaN/sonsuz/aralik disi): isaretli -> en kucuk deger, isaretsiz -> 0. Girdi: gorunum (stdin) -> stdout ; src/anlam_wi_gen.h uretilir. --guncelle: SEM'leri yeniden uret."""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'kayan'))
import sf, ops
from sf import F64
L = sys.stdin.read().split('\n')
GUNCELLE = '--guncelle' in sys.argv
VAR = {l.split()[2] for l in L if l.startswith('op ')}   # tabloda zaten olan komutlar: yeniden eklenmez (yalniz eksikler eklenir)
ol = max(i for i, l in enumerate(L) if l.startswith('op ')); oid = int(L[ol].split()[1])
sl = max(i for i, l in enumerate(L) if l.startswith('sem '))
RND = {l.split(' ', 2)[1][4:].lower(): l.split(' ', 2)[2] for l in L if l.startswith('sem RND_') and l.split(' ', 2)[1] in ('RND_F32', 'RND_F16', 'RND_BF16')}
nop, nsem, cxx = [], [], []
def op(name, sig, g, w):
    global oid
    if name.upper() in VAR:
        cxx.append((name.upper(), w)); nsem.append('sem %s %s' % (name.upper(), g.text())) if GUNCELLE else None; return
    oid += 1; nop.append('op %d %s %s %s' % (oid, name.upper(), name, sig)); nsem.append('sem %s %s' % (name.upper(), g.text())); cxx.append((name.upper(), w))
IDX = {'f32': 0, 'f16': 1, 'bf16': 2, 'f64': 3}
for sg, nm in ((True, 'i128'), (False, 'u128')):
    for nar in ('f64', 'f32', 'f16', 'bf16'): op('cv_%s_%s' % (nm, nar), 'd,q', ops.i128_2narrow(sg, nar, RND.get(nar)), ('WFP_I128_TO_NARROW_' + ('S' if sg else 'U'), 0, IDX[nar]))
op('bswap128', 'Q,q', ops.bswap128(), ('W128_BSWAP', 0, 0, 'W128'))
for sg, nm in ((True, 'i128'), (False, 'u128')): op('cv_f64_' + nm, 'Q,f', ops.w2i(F64, 128, sg), ('WFP_F64_TO_I128_' + ('S' if sg else 'U'), 0, 0))
hdr = ['// 128 bit tamsayi <-> dar kayan nokta komutlarinin C++ anlam makrolari -- oz/komut/gecis/tur13_ekle.py ile uretilir.']
for nm, w in cxx: hdr.append('#define SEM_%s(M) M##_%s(%s,%d,%d)' % (nm, w[3] if len(w) > 3 else 'WFP', w[0], w[1], w[2]))
hdr.append('#define PM_SEM_OPS_WI(X) \\\n  ' + ' '.join('X(%s)' % nm for nm, w in cxx))
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'src', 'anlam_wi_gen.h'), 'w').write('\n'.join(hdr) + '\n')
if GUNCELLE:
    yeni = {l.split(' ', 2)[1]: l for l in nsem}; out = []
    for l in L:
        t = l.split(' ', 2); out.append(yeni[t[1]] if t[0] == 'sem' and len(t) > 1 and t[1] in yeni else l)
    print('\n'.join(out), end=''); sys.exit(0)
ins = {ol: nop, sl: nsem}; out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
