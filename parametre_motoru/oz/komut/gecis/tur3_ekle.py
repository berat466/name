#!/usr/bin/env python3
# Plan Bolum 01 (float32 / float16 / bfloat16): komut tablosuna (okunabilir gorunum, stdin -> stdout) dar kayan nokta turlerinin KOMUTLARINI ekler. Yeni mikro-islem YOK.
#   Deger gosterimi: dar kayan nokta degerleri double olarak (tam, kayipsiz) saklanir; her aritmetik sonuc ilgili bicimin izgarasina (en yakina, esitlikte cifte) yuvarlanir.
#   Cift yuvarlama guvenli: double (53 bit) >= 2*p+2 (p = 24, 11, 8) oldugundan +,-,*,/,sqrt double'da hesaplanip yuvarlanmasi dogrudan yuvarlamayla ayni sonucu verir.
#   rnd_<f> : double -> bicim izgarasi (Dekker/Veltkamp benzeri sihirli toplama: (|x|+C)-C, C = 1.5*2^(e+52-MB); altnormal icin sabit C; tasma -> sonsuz; NaN -> sessiz NaN, yuk kisaltilir)
#   add_/sub_/mul_/div_/sqrt_<f>, i2f_<f> (isaretli int64 -> bicim, tek yuvarlama: 2^53 ustu icin yapiskan bit), u2f_<f> (isaretsiz)
#   f2i_i8.. f2i_u32 : double -> tamsayi (kes + sar), ftobool : x != 0
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' RND32 ' in l for l in L): sys.exit('tur3_ekle: tablo zaten genisletilmis')
op_last = max(i for i, l in enumerate(L) if l.startswith('op '))
oid = int(L[op_last].split()[1])
newop, newsem = [], []
def op(name, fmt, sem):
    global oid
    oid += 1; newop.append('op %d %s %s %s' % (oid, name.upper(), name, fmt)); newsem.append('sem %s %s' % (name.upper(), sem))
def rnd(MB, EMIN, EMAX):
    S = 52 - MB; EMINB = EMIN + 1023; TB = EMAX + 1 + 1023
    return ' ; '.join([
        'lit t2 63', 'lit t3 1', 'alu_shl t3 t3 t2', 'alu_and t4 t1 t3', 'alu_xor t5 t1 t4',
        'lit t6 %d' % S, 'alu_shr t7 t1 t6', 'alu_shl t7 t7 t6', 'lit t6 1', 'lit t2 51', 'alu_shl t6 t6 t2', 'alu_or t7 t7 t6',
        'lit t1 2047', 'lit t2 52', 'alu_shl t1 t1 t2', 'cmp_gt t2 t5 t1',
        'lit t3 52', 'alu_shr t6 t5 t3', 'lit t8 %d' % EMINB, 'cmp_lt t3 t6 t8', 'sel t6 t3 t8 t6',
        'lit t8 %d' % S, 'alu_add t6 t6 t8', 'lit t8 52', 'alu_shl t6 t6 t8', 'lit t3 1', 'lit t8 51', 'alu_shl t3 t3 t8', 'alu_or t6 t6 t3',
        'fadd t3 t5 t6', 'fsub t3 t3 t6',
        'lit t8 %d' % TB, 'lit t6 52', 'alu_shl t8 t8 t6',
        'cmp_ge t6 t5 t8', 'cmp_ge t5 t3 t8', 'alu_or t6 t6 t5',
        'alu_or t3 t3 t4', 'alu_or t8 t4 t1', 'sel t3 t6 t8 t3', 'sel t3 t2 t7 t3', 'wr a t3'])
def i2f(signed):
    pre = ['rd t1 b', 'lit t2 63', 'alu_sar t3 t1 t2', 'alu_xor t4 t1 t3', 'alu_sub t4 t4 t3'] if signed else ['rd t4 b', 'lit t3 0']
    mid = ['lit t2 11', 'alu_shr t5 t4 t2', 'lit t6 2047', 'alu_and t6 t4 t6', 'lit t7 0', 'cmp_ne t6 t6 t7', 'alu_or t5 t5 t6',
           'i2f t5 t5', 'lit t6 2048', 'i2f t6 t6', 'fmul t5 t5 t6', 'i2f t6 t4', 'lit t7 1', 'lit t2 53', 'alu_shl t7 t7 t2', 'cmp_ltu t7 t4 t7', 'sel t6 t7 t6 t5']
    post = ['lit t2 0', 'cmp_ne t7 t3 t2', 'fneg t5 t6', 'sel t1 t7 t5 t6'] if signed else ['alu_or t1 t6 t6']
    return pre + mid + post
FM = {'f32': (23, -126, 127), 'f16': (10, -14, 15), 'bf16': (7, -126, 127)}
for f, (MB, EMIN, EMAX) in FM.items():
    R = rnd(MB, EMIN, EMAX)
    op('rnd_' + f, 'd,f', 'rd t1 b ; ' + R)
    for k, p in (('add', 'fadd'), ('sub', 'fsub'), ('mul', 'fmul'), ('div', 'fdiv')):
        op('%s_%s' % (k, f), 'd,f,f', 'rd t1 b ; rd t2 c ; %s t1 t1 t2 ; ' % p + R)
    op('sqrt_' + f, 'd,f', 'rd t1 b ; fsqrt t1 t1 ; ' + R)
    op('i2f_' + f, 'd,r', ' ; '.join(i2f(True)) + ' ; ' + R)
    op('u2f_' + f, 'd,r', ' ; '.join(i2f(False)) + ' ; ' + R)
for w in (8, 16, 32):
    op('f2i_i%d' % w, 'd,f', 'rd t1 b ; f2i t2 t1 ; sx%d t3 t2 ; wr a t3' % w)
    op('f2i_u%d' % w, 'd,f', 'rd t1 b ; f2i t2 t1 ; zx%d t3 t2 ; wr a t3' % w)
op('ftobool', 'd,f', 'rd t1 b ; lit t2 0 ; feq t3 t1 t2 ; lit t4 0 ; cmp_eq t3 t3 t4 ; wr a t3')
sem_last = max(i for i, l in enumerate(L) if l.startswith('sem '))
out = L[:op_last + 1] + newop + L[op_last + 1:sem_last + 1] + newsem + L[sem_last + 1:]
print('\n'.join(out), end='')
