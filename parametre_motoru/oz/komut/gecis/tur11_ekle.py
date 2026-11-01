#!/usr/bin/env python3
"""Plan Bolum 01 (f80 = x87 genisletilmis, f128 = IEEE binary128): yazilim kayan nokta komutlari. Her komut OP + SEM (mevcut mikro-islemler + dongu/kazima alani).
Mikro-programlar ../../kayan/{sf,ops}.py ureteclerinden gelir (GELISTIRME araci; motorun parcasi tabloya yazilan sayilardir). Girdi: okunabilir gorunum (stdin) -> stdout.
Komutlar (F = f80 | f128): add_F sub_F mul_F div_F (Q,q,q) sqrt_F neg_F abs_F mov_F (Q,q) pack_F (Q,r,r) lo64_F hi64_F (d,q) eq_F ne_F lt_F le_F (d,q,q) tobool_F (d,q)
  cv_f64_F (Q,f) cv_F_f64 (d,q) cv_f80_f128 cv_f128_f80 (Q,q) cv_F_f32 cv_F_f16 cv_F_bf16 (d,q) cv_i64_F cv_u64_F (Q,r) cv_i128_F cv_u128_F (Q,q)
  cv_F_i64 cv_F_u64 (d,q) cv_F_i128 cv_F_u128 (Q,q) cv_F_i8 cv_F_i16 cv_F_i32 cv_F_u8 cv_F_u16 cv_F_u32 (d,q)"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'kayan'))
import sf, ops
from sf import F80, F128, F64, G
L = sys.stdin.read().split('\n')
GUNCELLE = '--guncelle' in sys.argv
if not GUNCELLE and any(l.startswith('op ') and ' ADD_F80 ' in l for l in L): sys.exit('tur11_ekle: tablo zaten genisletilmis (--guncelle: mevcut komutlarin SEM kayitlarini yeniden uret)')
ol = max(i for i, l in enumerate(L) if l.startswith('op ')); oid = int(L[ol].split()[1])
sl = max(i for i, l in enumerate(L) if l.startswith('sem '))
RND = {}
for l in L:
    t = l.split(' ', 2)
    if t[0] == 'sem' and t[1] in ('RND_F32', 'RND_F16', 'RND_BF16'): RND[t[1][4:].lower()] = t[2]
assert len(RND) == 3
nop, nsem, cxx = [], [], []
def op(name, sig, g, w=None):
    global oid
    oid += 1; nop.append('op %d %s %s %s' % (oid, name.upper(), name, sig)); nsem.append('sem %s %s' % (name.upper(), g.text()))
    cxx.append((name.upper(), w))
def raw_move(src_lo=True):
    pass
FM = {'f80': F80, 'f128': F128}
for fi, (n, f) in enumerate(FM.items()):
    W = lambda code, p2=0, fmt=fi: ('WFP_' + code, fmt, p2)
    op('add_' + n, 'Q,q,q', ops.addsub(f, False), W('ADD')); op('sub_' + n, 'Q,q,q', ops.addsub(f, True), W('SUB'))
    op('mul_' + n, 'Q,q,q', ops.mul(f), W('MUL')); op('div_' + n, 'Q,q,q', ops.div(f), W('DIV')); op('sqrt_' + n, 'Q,q', ops.sqrt(f), W('SQRT'))
    op('neg_' + n, 'Q,q', ops.neg_abs(f, 'neg'), W('NEG')); op('abs_' + n, 'Q,q', ops.neg_abs(f, 'abs'), W('ABS'))
    g = G(); g.e('rdw t1 b 0', 'rdw t2 b 1', 'wrw a 0 t1', 'wrw a 1 t2'); op('mov_' + n, 'Q,q', g, W('MOV'))
    g = G(); g.e('rd t1 b', 'rd t2 c')
    if f['ex']: g.e('lit t3 65535', 'alu_and t2 t2 t3')
    g.e('wrw a 0 t1', 'wrw a 1 t2'); op('pack_' + n, 'Q,r,r', g, W('PACK'))
    g = G(); g.e('rdw t1 b 0', 'wr a t1'); op('lo64_' + n, 'd,q', g, W('LO'))
    g = G(); g.e('rdw t1 b 1')
    if f['ex']: g.e('lit t3 65535', 'alu_and t1 t1 t3')
    g.e('wr a t1'); op('hi64_' + n, 'd,q', g, W('HI'))
    for k in ('eq', 'ne', 'lt', 'le'): op('%s_%s' % (k, n), 'd,q,q', ops.cmp_(f, k), W(k.upper()))
    op('tobool_' + n, 'd,q', ops.tobool(f), W('TOBOOL'))
    op('cv_f64_' + n, 'Q,f', ops.w2w(F64, f), W('FROM_F64')); op('cv_%s_f64' % n, 'd,q', ops.w2w(f, F64, final='one'), W('TO_F64'))
    for ni, nar in enumerate(('f32', 'f16', 'bf16')): op('cv_%s_%s' % (n, nar), 'd,q', ops.w2n(f, nar, RND[nar]), W('TO_NARROW', ni))
    op('cv_i64_' + n, 'Q,r', ops.i2w(f, True), W('FROM_I64')); op('cv_u64_' + n, 'Q,r', ops.i2w(f, False), W('FROM_U64'))
    op('cv_i128_' + n, 'Q,q', ops.i128_2w(f, True), W('FROM_I128')); op('cv_u128_' + n, 'Q,q', ops.i128_2w(f, False), W('FROM_U128'))
    op('cv_%s_i64' % n, 'd,q', ops.w2i(f, 64, True), W('TO_INT_S', 64)); op('cv_%s_u64' % n, 'd,q', ops.w2i(f, 64, False), W('TO_INT_U', 64))
    op('cv_%s_i128' % n, 'Q,q', ops.w2i(f, 128, True), W('TO_INT_S', 128)); op('cv_%s_u128' % n, 'Q,q', ops.w2i(f, 128, False), W('TO_INT_U', 128))
    for nb in (8, 16, 32):
        for sg in (True, False): op('cv_%s_%s%d' % (n, 'i' if sg else 'u', nb), 'd,q', ops.w2i(f, 64, sg, narrow=(nb, sg)), W('TO_INT_S' if sg else 'TO_INT_U', nb))
op('cv_f80_f128', 'Q,q', ops.w2w(F80, F128), ('WFP_TO_OTHER', 0, 0)); op('cv_f128_f80', 'Q,q', ops.w2w(F128, F80), ('WFP_TO_OTHER', 1, 0))
# C++ referans icin makro basligi (src/anlam_wfp_gen.h)
hdr = ['// ' + 'GENIS KAYAN NOKTA komutlarinin C++ anlam makrolari -- oz/komut/gecis/tur11_ekle.py ile uretilir (tablodaki OP adlariyla bire bir).']
for nm, w in cxx: hdr.append('#define SEM_%s(M) M##_WFP(%s,%d,%d)' % (nm, w[0], w[1], w[2]))
hdr.append('#define PM_SEM_OPS_WFP(X) \\\n  ' + ' '.join('X(%s)' % nm for nm, w in cxx))
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'src', 'anlam_wfp_gen.h'), 'w').write('\n'.join(hdr) + '\n')
if GUNCELLE:   # mevcut komutlarin SEM satirlarini yeniden uretilenlerle degistir (OP/TYPES degismez)
    yeni = {l.split(' ', 2)[1]: l for l in nsem}; out = []
    for l in L:
        t = l.split(' ', 2)
        out.append(yeni[t[1]] if t[0] == 'sem' and len(t) > 1 and t[1] in yeni else l)
    print('\n'.join(out), end=''); sys.exit(0)
ins = {ol: nop, sl: nsem}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
