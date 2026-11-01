#!/usr/bin/env python3
# Plan Bolum 01 (genel coklu yazmac / 128 bit): komut tablosuna (okunabilir gorunum, stdin -> stdout) GENIS OPERAND altyapisini ve i128/u128 komutlarini ekler.
#   KIND: q (2 sozcuk kaynak) Q (2 sozcuk hedef) o (4 sozcuk kaynak) O (4 sozcuk hedef); ext 3 = sozcuk sayisi, ext 4 = taban tur (1 d, 2 r); KFLAGS bit 8.. = sozcuk-1.
#   Mikro-islemler: rdw to,op,n (operand yazmacinin n. sozcugu) ; wrw op,n,ti ; mulhu to,ti,ti (64x64 -> ust 64 bit, isaretsiz)
#   Kural: bir genis operand, adlandirilmis yazmac r icin r, r+1 (.. r+w-1) ARDISIK yazmaclardir (dusuk sozcuk once = kucuk-sonlu temsil).
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('prim ') and ' rdw ' in l for l in L): sys.exit('tur6_ekle: tablo zaten genisletilmis')
def last(prefix): return max(i for i, l in enumerate(L) if l.startswith(prefix))
kl = last('kind ')
newkind = ['kind 11 q 257 ext 3=2 4=2', 'kind 12 Q 258 ext 3=2 4=1', 'kind 13 o 769 ext 3=4 4=2', 'kind 14 O 770 ext 3=4 4=1']
pl = last('prim '); pid = int(L[pl].split()[1])
newprim = ['prim %d rdw to,op,n ext 1=0' % (pid + 1), 'prim %d wrw op,n,ti ext 1=0' % (pid + 2), 'prim %d mulhu to,ti,ti ext 1=0' % (pid + 3)]
ol = last('op '); oid = int(L[ol].split()[1])
newop, newsem = [], []
def op(name, fmt, sem):
    global oid
    oid += 1; newop.append('op %d %s %s %s' % (oid, name.upper(), name, fmt)); newsem.append('sem %s %s' % (name.upper(), sem))
def J(*a): return ' ; '.join(a)
def rd(slot, d0, d1): return ['rdw %s %s 0' % (d0, slot), 'rdw %s %s 1' % (d1, slot)]
def wr(slot, s0, s1): return ['wrw %s 0 %s' % (slot, s0), 'wrw %s 1 %s' % (slot, s1)]
for ty in ('i128', 'u128'):
    op('mov_' + ty, 'Q,q', J(*(rd('b', 't1', 't2') + wr('a', 't1', 't2'))))
    op('ldi_' + ty, 'Q,i', J('im t1 b', 'lit t2 63', 'alu_sar t2 t1 t2', 'wrw a 0 t1', 'wrw a 1 t2'))
    op('add_' + ty, 'Q,q,q', J(*(rd('b', 't1', 't2') + rd('c', 't3', 't4') + ['alu_add t5 t1 t3', 'cmp_ltu t6 t5 t1', 'alu_add t7 t2 t4', 'alu_add t7 t7 t6'] + wr('a', 't5', 't7'))))
    op('sub_' + ty, 'Q,q,q', J(*(rd('b', 't1', 't2') + rd('c', 't3', 't4') + ['cmp_ltu t6 t1 t3', 'alu_sub t5 t1 t3', 'alu_sub t7 t2 t4', 'alu_sub t7 t7 t6'] + wr('a', 't5', 't7'))))
    op('mul_' + ty, 'Q,q,q', J(*(rd('b', 't1', 't2') + rd('c', 't3', 't4') + ['alu_mul t5 t1 t3', 'mulhu t6 t1 t3', 'alu_mul t7 t1 t4', 'alu_add t6 t6 t7', 'alu_mul t7 t2 t3', 'alu_add t6 t6 t7'] + wr('a', 't5', 't6'))))
    op('neg_' + ty, 'Q,q', J(*(rd('b', 't1', 't2') + ['lit t3 0', 'cmp_ne t4 t1 t3', 'un_neg t1 t1', 'un_neg t2 t2', 'alu_sub t2 t2 t4'] + wr('a', 't1', 't2'))))
    op('not_' + ty, 'Q,q', J(*(rd('b', 't1', 't2') + ['un_not t1 t1', 'un_not t2 t2'] + wr('a', 't1', 't2'))))
    for nm, p in (('and', 'alu_and'), ('or', 'alu_or'), ('xor', 'alu_xor')):
        op('%s_%s' % (nm, ty), 'Q,q,q', J(*(rd('b', 't1', 't2') + rd('c', 't3', 't4') + ['%s t1 t1 t3' % p, '%s t2 t2 t4' % p] + wr('a', 't1', 't2'))))
    pre = rd('b', 't1', 't2') + ['rd t3 c', 'lit t5 127', 'alu_and t3 t3 t5', 'lit t5 6', 'alu_shr t4 t3 t5', 'lit t5 63', 'alu_and t3 t3 t5']
    op('shl_' + ty, 'Q,q,r', J(*(pre + [
        'alu_shl t6 t1 t3', 'alu_shl t7 t2 t3', 'lit t5 1', 'alu_shr t8 t1 t5', 'lit t5 63', 'alu_sub t5 t5 t3', 'alu_shr t8 t8 t5', 'alu_or t7 t7 t8',
        'lit t5 0', 'sel t8 t4 t6 t7', 'sel t6 t4 t5 t6'] + wr('a', 't6', 't8'))))
pre = rd('b', 't1', 't2') + ['rd t3 c', 'lit t5 127', 'alu_and t3 t3 t5', 'lit t5 6', 'alu_shr t4 t3 t5', 'lit t5 63', 'alu_and t3 t3 t5']
op('shr_u128', 'Q,q,r', J(*(pre + [
    'alu_shr t6 t1 t3', 'lit t5 1', 'alu_shl t7 t2 t5', 'lit t5 63', 'alu_sub t5 t5 t3', 'alu_shl t7 t7 t5', 'alu_or t6 t6 t7',
    'alu_shr t7 t2 t3', 'lit t5 0', 'sel t6 t4 t7 t6', 'sel t7 t4 t5 t7'] + wr('a', 't6', 't7'))))
op('sar_i128', 'Q,q,r', J(*(pre + [
    'alu_shr t6 t1 t3', 'lit t5 1', 'alu_shl t7 t2 t5', 'lit t5 63', 'alu_sub t5 t5 t3', 'alu_shl t7 t7 t5', 'alu_or t6 t6 t7',
    'alu_sar t7 t2 t3', 'lit t5 63', 'alu_sar t8 t2 t5', 'sel t6 t4 t7 t6', 'sel t7 t4 t8 t7'] + wr('a', 't6', 't7'))))
# karsilastirmalar (hedef dar d)
def cmp_lt(hi_cmp, x, y):   # x,y = (lo,hi) gecici cifti ; sonuc t5 (x < y)
    return ['%s t5 %s %s' % (hi_cmp, x[1], y[1]), 'cmp_eq t6 %s %s' % (x[1], y[1]), 'cmp_ltu t7 %s %s' % (x[0], y[0]), 'alu_and t6 t6 t7', 'alu_or t5 t5 t6']
ld2 = rd('b', 't1', 't2') + rd('c', 't3', 't4')
op('eq_128', 'd,q,q', J(*(ld2 + ['alu_xor t5 t1 t3', 'alu_xor t6 t2 t4', 'alu_or t5 t5 t6', 'lit t6 0', 'cmp_eq t5 t5 t6', 'wr a t5'])))
op('ne_128', 'd,q,q', J(*(ld2 + ['alu_xor t5 t1 t3', 'alu_xor t6 t2 t4', 'alu_or t5 t5 t6', 'lit t6 0', 'cmp_ne t5 t5 t6', 'wr a t5'])))
for sg, hc in (('i', 'cmp_lt'), ('u', 'cmp_ltu')):
    ty = sg + '128'
    op('lt_' + ty, 'd,q,q', J(*(ld2 + cmp_lt(hc, ('t1', 't2'), ('t3', 't4')) + ['wr a t5'])))
    op('le_' + ty, 'd,q,q', J(*(ld2 + cmp_lt(hc, ('t3', 't4'), ('t1', 't2')) + ['lit t6 0', 'cmp_eq t5 t5 t6', 'wr a t5'])))
# donusumler (genis <-> dar)
for ty in ('i128', 'u128'):
    op('sext64_' + ty, 'Q,r', J('rd t1 b', 'lit t2 63', 'alu_sar t2 t1 t2', 'wrw a 0 t1', 'wrw a 1 t2'))
    op('zext64_' + ty, 'Q,r', J('rd t1 b', 'lit t2 0', 'wrw a 0 t1', 'wrw a 1 t2'))
    op('tobool_' + ty, 'd,q', J(*(rd('b', 't1', 't2') + ['alu_or t1 t1 t2', 'lit t2 0', 'cmp_ne t1 t1 t2', 'wr a t1'])))
    for tn, w in (('i8', 8), ('i16', 16), ('i32', 32), ('u8', 8), ('u16', 16), ('u32', 32)):
        ex = ('sx%d' if tn[0] == 'i' else 'zx%d') % w
        op('trunc_%s_%s' % (ty, tn), 'd,q', J('rdw t1 b 0', '%s t2 t1' % ex, 'wr a t2'))
    for tn in ('i64', 'u64'):
        op('trunc_%s_%s' % (ty, tn), 'd,q', J('rdw t1 b 0', 'wr a t1'))
op('cast_i128_u128', 'Q,q', J(*(rd('b', 't1', 't2') + wr('a', 't1', 't2'))))
op('cast_u128_i128', 'Q,q', J(*(rd('b', 't1', 't2') + wr('a', 't1', 't2'))))
# ---- tabloya yerlestir
sem_last = last('sem ')
ins = {kl: newkind, pl: newprim, ol: newop, sem_last: newsem}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
