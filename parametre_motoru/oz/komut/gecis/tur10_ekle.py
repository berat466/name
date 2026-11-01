#!/usr/bin/env python3
# 128 bit bolme/kalan (div_i128 mod_i128 div_u128 mod_u128, Q,q,q): bit-seri geri-yuklemeli bolme, mikro-program dongusu (lpb/lpe) ile 128 yineleme.
#   t1,t2 = bolunen (bolum birikir) t3,t4 = kalan t5,t6 = bolen t7 = sayac t9.. gecici ; sifira bolme: divu ile tuzak (DIV0). Isaretli: buyukluk uzerinden, bolum isareti = isaretlerin XOR'u, kalan isareti = bolunenin isareti.
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' DIV_I128 ' in l for l in L): sys.exit('tur10_ekle: tablo zaten genisletilmis')
ol = max(i for i, l in enumerate(L) if l.startswith('op ')); oid = int(L[ol].split()[1])
sl = max(i for i, l in enumerate(L) if l.startswith('sem '))
nop, nsem = [], []
def neg128(lo, hi, m, t, c):   # (lo,hi) = (lo,hi) ^ m, sonra +(m&1) ; m = 0 ya da -1 ; t, c gecici
    return ['alu_xor %s %s %s' % (lo, lo, m), 'alu_xor %s %s %s' % (hi, hi, m), 'lit %s 1' % t, 'alu_and %s %s %s' % (t, m, t),
            'alu_add %s %s %s' % (lo, lo, t), 'cmp_ltu %s %s %s' % (c, lo, t), 'alu_add %s %s %s' % (hi, hi, c)]
def build(signed, want_rem):
    s = ['rdw t1 b 0', 'rdw t2 b 1', 'rdw t5 c 0', 'rdw t6 c 1', 'alu_or t9 t5 t6', 'divu t9 t9 t9']   # bolen 0 ise divu tuzak verir
    if signed:
        s += ['lit t14 63', 'alu_shr t10 t2 t14', 'un_neg t10 t10',          # t10 = bolunen isaret maskesi (0/-1)
              'alu_shr t11 t6 t14', 'un_neg t11 t11']                          # t11 = bolen isaret maskesi
        s += neg128('t1', 't2', 't10', 't12', 't13') + neg128('t5', 't6', 't11', 't12', 't13')
    s += ['lit t3 0', 'lit t4 0', 'lit t7 128', 'lit t14 63', 'lit t15 1']
    body = ['alu_shr t9 t2 t14',
            'alu_shl t4 t4 t15', 'alu_shr t12 t3 t14', 'alu_or t4 t4 t12', 'alu_shl t3 t3 t15', 'alu_or t3 t3 t9',
            'alu_shl t2 t2 t15', 'alu_shr t12 t1 t14', 'alu_or t2 t2 t12', 'alu_shl t1 t1 t15',
            'cmp_ltu t8 t4 t6', 'cmp_eq t12 t4 t6', 'cmp_ltu t13 t3 t5', 'alu_and t12 t12 t13', 'alu_or t8 t8 t12', 'lit t12 0', 'cmp_eq t8 t8 t12',
            'un_neg t9 t8', 'alu_and t12 t5 t9', 'alu_and t13 t6 t9',
            'cmp_ltu t9 t3 t12', 'alu_sub t3 t3 t12', 'alu_sub t4 t4 t13', 'alu_sub t4 t4 t9',
            'alu_or t1 t1 t8', 'alu_sub t7 t7 t15']
    s += ['lpb'] + body + ['lpe t7']
    if signed:
        if want_rem: s += neg128('t3', 't4', 't10', 't12', 't13')               # kalan: bolunenin isareti
        else:
            s += ['alu_xor t10 t10 t11'] + neg128('t1', 't2', 't10', 't12', 't13')  # bolum: isaretlerin xor'u
    res = ('t3', 't4') if want_rem else ('t1', 't2')
    s += ['wrw a 0 %s' % res[0], 'wrw a 1 %s' % res[1]]
    return ' ; '.join(s)
def op(name, sem):
    global oid
    oid += 1; nop.append('op %d %s %s Q,q,q' % (oid, name.upper(), name)); nsem.append('sem %s %s' % (name.upper(), sem))
op('div_i128', build(True, False)); op('mod_i128', build(True, True)); op('div_u128', build(False, False)); op('mod_u128', build(False, True))
ins = {ol: nop, sl: nsem}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
