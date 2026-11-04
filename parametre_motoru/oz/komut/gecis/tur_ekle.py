#!/usr/bin/env python3
# Plan Bolum 01 (temel sayi ve deger turleri): komut tablosuna (PMK1 okunabilir gorunumu, stdin -> stdout) TAMSAYI AILELERI ekler:
#   mikro-islemler: sx8 sx16 sx32 zx8 zx16 zx32 (isaret/sifir genisletme), cmp_ltu cmp_leu (isaretsiz karsilastirma), divu modu (isaretsiz bolme/kalan), lit (sabit sayi)
#   komutlar: sext*/zext* (donusum), add/sub/mul/div/mod/neg/shl _i8/_i16/_i32, add/sub/mul/neg/not/shl _u8/_u16/_u32, divu modu ltu leu, u2f f2u; takma gtu geu
# Bu bir TEK SEFERLIK uretecdir; kanonik kaynak, ciktisi olan oz/komut/komutlar.pmk'dir (sayilar). Yeniden calistirmak mevcut girdiyi iki kez genisletmemelidir: onceden eklenmisse hata verir.
import sys, re
L = sys.stdin.read().split('\n')
if any(l.startswith('prim ') and ' sx8 ' in l for l in L): sys.exit('tur_ekle: tablo zaten genisletilmis')
def last(prefix): return max(i for i, l in enumerate(L) if l.startswith(prefix))
prim_last = last('prim ')
pid = int(L[prim_last].split()[1])
newprim = []
def prim(name, fields, cls=0):
    global pid
    pid += 1; newprim.append('prim %d %s %s ext 1=%d' % (pid, name, fields, cls))
for n in ('sx8', 'sx16', 'sx32', 'zx8', 'zx16', 'zx32'): prim(n, 'to,ti')
prim('cmp_ltu', 'to,ti,ti'); prim('cmp_leu', 'to,ti,ti')
prim('divu', 'to,ti,ti', 1); prim('modu', 'to,ti,ti', 1)
prim('lit', 'to,n')
op_last = last('op ')
oid = int(L[op_last].split()[1])
newop = []; newsem = []
ids = {}
def op(name, mn, fmt, sem):
    global oid
    oid += 1; ids[name] = oid
    newop.append('op %d %s %s %s' % (oid, name.upper(), mn, fmt)); newsem.append('sem %s %s' % (name.upper(), sem))
for w in (8, 16, 32):
    op('sext%d' % w, 'sext%d' % w, 'd,r', 'rd t1 b ; sx%d t2 t1 ; wr a t2' % w)
for w in (8, 16, 32):
    op('zext%d' % w, 'zext%d' % w, 'd,r', 'rd t1 b ; zx%d t2 t1 ; wr a t2' % w)
bin2 = {'add': 'alu_add', 'sub': 'alu_sub', 'mul': 'alu_mul', 'shl': 'alu_shl'}
for w in (8, 16, 32):
    for k, p in bin2.items():
        op('%s_i%d' % (k, w), '%s_i%d' % (k, w), 'd,r,r', 'rd t1 b ; rd t2 c ; %s t3 t1 t2 ; sx%d t4 t3 ; wr a t4' % (p, w))
    op('div_i%d' % w, 'div_i%d' % w, 'd,r,r', 'rd t1 b ; rd t2 c ; divs t3 t1 t2 ; sx%d t4 t3 ; wr a t4' % w)
    op('mod_i%d' % w, 'mod_i%d' % w, 'd,r,r', 'rd t1 b ; rd t2 c ; mods t3 t1 t2 ; sx%d t4 t3 ; wr a t4' % w)
    op('neg_i%d' % w, 'neg_i%d' % w, 'd,r', 'rd t1 b ; un_neg t2 t1 ; sx%d t3 t2 ; wr a t3' % w)
for w in (8, 16, 32):
    for k, p in bin2.items():
        op('%s_u%d' % (k, w), '%s_u%d' % (k, w), 'd,r,r', 'rd t1 b ; rd t2 c ; %s t3 t1 t2 ; zx%d t4 t3 ; wr a t4' % (p, w))
    op('neg_u%d' % w, 'neg_u%d' % w, 'd,r', 'rd t1 b ; un_neg t2 t1 ; zx%d t3 t2 ; wr a t3' % w)
    op('not_u%d' % w, 'not_u%d' % w, 'd,r', 'rd t1 b ; un_not t2 t1 ; zx%d t3 t2 ; wr a t3' % w)
op('divu', 'divu', 'd,r,r', 'rd t1 b ; rd t2 c ; divu t3 t1 t2 ; wr a t3')
op('modu', 'modu', 'd,r,r', 'rd t1 b ; rd t2 c ; modu t3 t1 t2 ; wr a t3')
op('ltu', 'ltu', 'd,r,r', 'rd t1 b ; rd t2 c ; cmp_ltu t3 t1 t2 ; wr a t3')
op('leu', 'leu', 'd,r,r', 'rd t1 b ; rd t2 c ; cmp_leu t3 t1 t2 ; wr a t3')
# u2f: x<0 (>=2^63) ise (x>>1 | x&1) cevrilip 2 ile carpilir; sel ile secilir.   t1 x  t2 1  t3 x>>1  t4 x&1  t5 or  t6 f(t5)  t7 2*t6  t3 f(x)  t4 0  t2 (x<0)  t5 sonuc
op('u2f', 'u2f', 'd,r', 'rd t1 b ; lit t2 1 ; alu_shr t3 t1 t2 ; alu_and t4 t1 t2 ; alu_or t5 t3 t4 ; i2f t6 t5 ; fadd t7 t6 t6 ; i2f t3 t1 ; lit t4 0 ; cmp_lt t2 t1 t4 ; sel t5 t2 t7 t3 ; wr a t5')
# f2u: f>=2^63 ise f2i(f-2^63) xor 2^63 (>=2^64 doyar: hepsi 1); degilse f2i(f), negatif ise 0. NaN -> 0.
#   t1 f  t2 1086  t3 52  t4 2^63(bit deseni)  t5 (2^63<=f)  t6 f-2^63  t7 f2i(t6)  t8 1  t3 63  t8 MIN  t7 hi  t2 f2i(f)  t3 0  t4 neg  t2 clamp  t6 sonuc
op('f2u', 'f2u', 'd,r', 'rd t1 b ; lit t2 1086 ; lit t3 52 ; alu_shl t4 t2 t3 ; fle t5 t4 t1 ; fsub t6 t1 t4 ; f2i t7 t6 ; lit t8 1 ; lit t3 63 ; alu_shl t8 t8 t3 ; alu_xor t7 t7 t8 ; f2i t2 t1 ; lit t3 0 ; cmp_lt t4 t2 t3 ; sel t2 t4 t3 t2 ; sel t6 t5 t7 t2 ; wr a t6')
# --- takma komutlar ve kurallar: gtu geu (b,c takasli LTU/LEU)
sec6 = [i for i, l in enumerate(L) if l.startswith('section 6 ')][0]
sec7 = [i for i, l in enumerate(L) if l.startswith('section 7 ')][0]
t = L[sec6].split(); cnt = int(t[4])
def pseudo(i, sym, nm):
    f = [i, len(sym)] + [ord(c) for c in sym] + [len(nm)] + [ord(c) for c in nm] + [3, 0]
    return [len(f)] + f
add6 = pseudo(cnt, 'GTU', 'gtu') + pseudo(cnt + 1, 'GEU', 'geu')
L[sec6] = ' '.join(t[:4] + [str(cnt + 2)] + t[5:] + [str(x) for x in add6])
t = L[sec7].split(); rc = int(t[4])
def rule(src, target):
    f = [1, src, 1, 3, 0, 0, 2, 2, 0, 1, target, 0]
    return [len(f)] + f
add7 = rule(cnt, ids['ltu']) + rule(cnt + 1, ids['leu'])
L[sec7] = ' '.join(t[:4] + [str(rc + 2)] + t[5:] + [str(x) for x in add7])
tpl_last = last('tpl prim ')
newtpl = [
 'tpl prim sx8 : 48 8B 44 24 TMP_F1 48 0F BE C0 48 89 44 24 TMP_F0',
 'tpl prim sx16 : 48 8B 44 24 TMP_F1 48 0F BF C0 48 89 44 24 TMP_F0',
 'tpl prim sx32 : 48 8B 44 24 TMP_F1 48 63 C0 48 89 44 24 TMP_F0',
 'tpl prim zx8 : 48 8B 44 24 TMP_F1 48 0F B6 C0 48 89 44 24 TMP_F0',
 'tpl prim zx16 : 48 8B 44 24 TMP_F1 48 0F B7 C0 48 89 44 24 TMP_F0',
 'tpl prim zx32 : 48 8B 44 24 TMP_F1 89 C0 48 89 44 24 TMP_F0',
 'tpl prim cmp_ltu : 48 8B 44 24 TMP_F1 48 8B 4C 24 TMP_F2 48 39 C8 0F 92 C0 0F B6 C0 48 89 44 24 TMP_F0',
 'tpl prim cmp_leu : 48 8B 44 24 TMP_F1 48 8B 4C 24 TMP_F2 48 39 C8 0F 96 C0 0F B6 C0 48 89 44 24 TMP_F0',
 'tpl prim divu : 48 8B 44 24 TMP_F1 48 8B 4C 24 TMP_F2 48 85 C9 0F 84 REL_DIV0 31 D2 48 F7 F1 48 89 44 24 TMP_F0',
 'tpl prim modu : 48 8B 44 24 TMP_F1 48 8B 4C 24 TMP_F2 48 85 C9 0F 84 REL_DIV0 31 D2 48 F7 F1 48 89 54 24 TMP_F0',
 'tpl prim lit : 48 C7 C0 LIT_F1 48 89 44 24 TMP_F0',
]
out = []
for i, l in enumerate(L):
    out.append(l)
    if i == prim_last: out += newprim
    if i == op_last: out += newop
    if i == tpl_last: out += newtpl
    # sem satirlari: son "sem" satirindan sonra
sem_last = max(i for i, l in enumerate(out) if l.startswith('sem '))
out[sem_last + 1:sem_last + 1] = newsem
sys.stdout.write('\n'.join(out))
