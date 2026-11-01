#!/usr/bin/env python3
"""Yazilim kayan nokta (f80 = x87 genisletilmis, f128 = IEEE binary128) MIKRO-PROGRAM URETECI (GELISTIRME araci; motorun parcasi uretilen mikro-programlardir).
Tum islemler komut tablosunda yalnizca OP+SEM'dir: rdw/wrw (genis operand), alu/cmp/sel, mulhu, divu (tuzak), lpb/lpe (dongu), spld/spst (kazima alani).
Gosterim (acilmis): isaret s, ussu E (1.xxx * 2^E), mantis M = 128 bit, en anlamli bit (127) = 1 (sifir degilse), sinif cls: 0 sifir 1 sonlu 2 sonsuz 3 NaN.
Yuvarlama en yakina-cifte. Gecersiz islem: varsayilan NaN (isaret 1, sessiz, yuk 0). NaN islenen: ilk NaN islenen sessizlestirilerek yayilir.
Gecici ayrimi: t1..t12 yardimci/ara, t13 dongu sayaci, t14..t16 sabit uretimi."""
M64 = (1 << 64) - 1
F128 = dict(name='f128', P=113, bias=16383, ex=False)
F80 = dict(name='f80', P=64, bias=16383, ex=True)
class G:
    def __init__(self): self.o = []; self.n = 0; self.v = {}
    def var(self, name, w=1):
        if name.endswith(('_m', '_q')) or name in ('W', 'Z') or name.startswith('V_'): w = 2   # cok sozcuklu degiskenler (dusuk, yuksek)
        if name not in self.v:
            self.v[name] = self.n; self.n += w
            assert self.n <= 256, 'kazima alani tasti: ' + name
        return self.v[name]
    def e(self, *s): self.o += list(s)
    def text(self): return ' ; '.join(self.o)
    def ld(self, t, name, w=0): self.e('spld %s %d' % (t, self.var(name) + w))
    def st(self, name, t, w=0): self.e('spst %d %s' % (self.var(name) + w, t))
    def c(self, t, v):   # 64 bit isaretsiz sabit -> t (t14, t15, t16 ara)
        v &= M64; sv = v - (1 << 64) if v >> 63 else v
        if -(1 << 31) <= sv < (1 << 31): self.e('lit %s %d' % (t, sv)); return
        hi, lo = v >> 32, v & 0xFFFFFFFF
        shi = hi - (1 << 32) if hi >> 31 else hi; slo = lo - (1 << 32) if lo >> 31 else lo
        self.e('lit %s %d' % (t, shi), 'lit t14 32', 'alu_shl %s %s t14' % (t, t), 'lit t15 %d' % slo, 'lit t16 -1', 'alu_shr t16 t16 t14', 'alu_and t15 t15 t16', 'alu_or %s %s t15' % (t, t))
    def c2(self, name, lo, hi): self.c('t1', lo); self.st(name, 't1', 0); self.c('t1', hi); self.st(name, 't1', 1)
    def copy2(self, d, s): self.ld('t1', s, 0); self.ld('t2', s, 1); self.st(d, 't1', 0); self.st(d, 't2', 1)
    def add2(self, d, a, b):
        self.ld('t1', a, 0); self.ld('t2', a, 1); self.ld('t3', b, 0); self.ld('t4', b, 1)
        self.e('alu_add t5 t1 t3', 'cmp_ltu t6 t5 t1', 'alu_add t7 t2 t4', 'alu_add t7 t7 t6'); self.st(d, 't5', 0); self.st(d, 't7', 1)
    def sub2(self, d, a, b):
        self.ld('t1', a, 0); self.ld('t2', a, 1); self.ld('t3', b, 0); self.ld('t4', b, 1)
        self.e('cmp_ltu t6 t1 t3', 'alu_sub t5 t1 t3', 'alu_sub t7 t2 t4', 'alu_sub t7 t7 t6'); self.st(d, 't5', 0); self.st(d, 't7', 1)
    def shl1(self, v):
        self.ld('t1', v, 0); self.ld('t2', v, 1)
        self.e('lit t3 63', 'alu_shr t4 t1 t3', 'lit t5 1', 'alu_shl t2 t2 t5', 'alu_or t2 t2 t4', 'alu_shl t1 t1 t5'); self.st(v, 't1', 0); self.st(v, 't2', 1)
    def shr1(self, v):
        self.ld('t1', v, 0); self.ld('t2', v, 1)
        self.e('lit t3 63', 'lit t5 1', 'alu_shl t4 t2 t3', 'alu_shr t1 t1 t5', 'alu_or t1 t1 t4', 'alu_shr t2 t2 t5'); self.st(v, 't1', 0); self.st(v, 't2', 1)
    def ltu2(self, t, a, b):   # t = (a < b) isaretsiz 128 ; t1..t8 kullanir
        self.ld('t1', a, 0); self.ld('t2', a, 1); self.ld('t3', b, 0); self.ld('t4', b, 1)
        self.e('cmp_ltu %s t2 t4' % t, 'cmp_eq t5 t2 t4', 'cmp_ltu t6 t1 t3', 'alu_and t5 t5 t6', 'alu_or %s %s t5' % (t, t))
    def nz2(self, t, a):
        self.ld('t1', a, 0); self.ld('t2', a, 1); self.e('alu_or %s t1 t2' % t, 'lit t3 0', 'cmp_ne %s %s t3' % (t, t))
    def selv(self, d, cond_t, a, b):   # d = cond ? a : b (1 sozcuk degisken)
        self.ld('t1', a); self.ld('t2', b); self.e('sel t3 %s t1 t2' % cond_t); self.st(d, 't3')
    def sel2(self, d, cond_t, a, b):   # 2 sozcuk
        self.ld('t1', a, 0); self.ld('t2', b, 0); self.e('sel t3 %s t1 t2' % cond_t); self.st(d, 't3', 0)
        self.ld('t1', a, 1); self.ld('t2', b, 1); self.e('sel t3 %s t1 t2' % cond_t); self.st(d, 't3', 1)
    # ---- dongu: sayac t13, govde f(g)
    def loop(self, n, body):
        self.e('lit t13 %d' % n, 'lpb'); body(); self.e('lit t12 1', 'alu_sub t13 t13 t12', 'lpe t13')
    def normalize(self, X):   # M msb 1 olana dek sola kaydir (M = 0 degilse), E azalt
        def body():
            self.ld('t1', X + '_m', 0); self.ld('t2', X + '_m', 1)
            self.e('lit t3 63', 'alu_shr t4 t2 t3', 'alu_or t5 t1 t2', 'lit t6 0', 'cmp_ne t5 t5 t6', 'lit t7 1', 'alu_xor t4 t4 t7', 'alu_and t4 t4 t5')   # t4 = yap
            self.e('alu_shr t8 t1 t3', 'alu_shl t9 t2 t7', 'alu_or t9 t9 t8', 'alu_shl t10 t1 t7')
            self.e('sel t2 t4 t9 t2', 'sel t1 t4 t10 t1'); self.st(X + '_m', 't1', 0); self.st(X + '_m', 't2', 1)
            self.ld('t8', X + '_E'); self.e('alu_sub t8 t8 t4'); self.st(X + '_E', 't8')
        self.loop(128, body)
    # ---- ACMA (unpack): operand yuvasi 'b'/'c' -> X_s X_E X_c X_m(2) X_rl X_rh
    def unpack(self, X, fmt, slot):
        self.e('rdw t1 %s 0' % slot, 'rdw t2 %s 1' % slot); self.st(X + '_rl', 't1'); self.st(X + '_rh', 't2')
        bias = fmt['bias']
        if not fmt['ex']:
            self.e('lit t3 63', 'alu_shr t4 t2 t3'); self.st(X + '_s', 't4')
            self.e('lit t3 48', 'alu_shr t5 t2 t3', 'lit t6 32767', 'alu_and t5 t5 t6')
            self.c('t6', 0xFFFFFFFFFFFF); self.e('alu_and t6 t2 t6')
            self.e('alu_or t7 t6 t1', 'lit t8 0', 'cmp_ne t7 t7 t8')
            self.e('cmp_ne t9 t5 t8', 'lit t3 48', 'alu_shl t9 t9 t3', 'alu_or t9 t9 t6', 'lit t3 15', 'alu_shl t9 t9 t3', 'lit t3 49', 'alu_shr t10 t1 t3', 'alu_or t9 t9 t10')
            self.e('lit t3 15', 'alu_shl t10 t1 t3'); self.st(X + '_m', 't10', 0); self.st(X + '_m', 't9', 1)
            unn = False
        else:
            self.e('lit t3 15', 'alu_shr t4 t2 t3', 'lit t6 1', 'alu_and t4 t4 t6'); self.st(X + '_s', 't4')
            self.e('lit t6 32767', 'alu_and t5 t2 t6')
            self.e('lit t3 1', 'alu_shl t7 t1 t3', 'lit t8 0', 'cmp_ne t7 t7 t8')
            self.st(X + '_m', 't8', 0); self.st(X + '_m', 't1', 1)
            unn = True
        # sinif: t5 ef, t7 fnz, t8 = 0
        self.e('cmp_eq t3 t5 t8', 'lit t4 32767', 'cmp_eq t4 t5 t4', 'lit t6 1', 'alu_xor t9 t7 t6', 'alu_and t10 t3 t9', 'alu_and t11 t4 t9', 'alu_and t12 t4 t7')
        if unn:   # f80: ef != 0, ef != max, tam sayi biti 0 -> gecersiz (NaN)
            self.e('lit t6 63', 'alu_shr t8 t1 t6', 'lit t6 1', 'alu_xor t8 t8 t6', 'alu_xor t6 t3 t6', 'alu_and t8 t8 t6', 'lit t6 1', 'alu_xor t6 t4 t6', 'alu_and t8 t8 t6')   # t8 = unnormal
            self.e('alu_or t12 t12 t8', 'lit t8 0')
        self.e('lit t6 1', 'lit t9 0', 'sel t6 t10 t9 t6', 'lit t9 2', 'sel t6 t11 t9 t6', 'lit t9 3', 'sel t6 t12 t9 t6'); self.st(X + '_c', 't6')
        self.c('t9', -bias); self.e('alu_add t9 t5 t9'); self.c('t10', 1 - bias); self.e('sel t9 t3 t10 t9'); self.st(X + '_E', 't9')
        self.normalize(X)
    # ---- PAKLA (pack): R_s R_E R_m R_st -> W_lo W_hi  (sonlu, sifir olmayan deger; sifir/sonsuz/NaN bu yoldan gecmez)
    def pack(self, fmt, R, W='W'):
        P, bias = fmt['P'], fmt['bias']; emin = 1 - bias
        self.ld('t1', R + '_E'); self.c('t2', emin)
        self.e('alu_sub t3 t2 t1', 'lit t4 0', 'cmp_gt t5 t3 t4', 'sel t3 t5 t3 t4', 'lit t6 140', 'cmp_gt t5 t3 t6', 'sel t3 t5 t6 t3')   # t3 = alt-normal ek kaydirma
        self.e('cmp_ge t7 t1 t2'); self.st(W + '_nm', 't7')
        self.e('lit t4 %d' % (128 - P), 'alu_add t3 t3 t4'); self.st(W + '_d', 't3')
        self.copy2(W + '_q', R + '_m'); self.e('lit t1 0'); self.st(W + '_g', 't1'); self.ld('t1', R + '_st'); self.st(W + '_k', 't1')
        self.ld('t13', W + '_d'); self.e('lpb')
        self.ld('t1', W + '_k'); self.ld('t2', W + '_g'); self.e('alu_or t1 t1 t2'); self.st(W + '_k', 't1')
        self.ld('t3', W + '_q', 0); self.e('lit t4 1', 'alu_and t2 t3 t4'); self.st(W + '_g', 't2'); self.shr1(W + '_q')
        self.e('lit t12 1', 'alu_sub t13 t13 t12', 'lpe t13')
        # en yakina-cifte: inc = g & (k | q&1)
        self.ld('t1', W + '_q', 0); self.e('lit t2 1', 'alu_and t3 t1 t2'); self.ld('t4', W + '_k'); self.e('alu_or t3 t3 t4'); self.ld('t5', W + '_g'); self.e('alu_and t3 t3 t5')
        self.ld('t1', W + '_q', 0); self.ld('t2', W + '_q', 1); self.e('alu_add t4 t1 t3', 'cmp_ltu t5 t4 t1', 'alu_add t2 t2 t5'); self.st(W + '_q', 't4', 0); self.st(W + '_q', 't2', 1)
        # tasma: q >= 2^P (yalniz normal)
        self.ld('t1', W + '_q', 1); self.e('lit t2 %d' % (P - 64) if P > 64 else 'lit t2 0', 'alu_shr t3 t1 t2', 'lit t4 0', 'cmp_ne t3 t3 t4'); self.ld('t5', W + '_nm'); self.e('alu_and t3 t3 t5')   # t3 = ov
        self.ld('t1', W + '_q', 0); self.ld('t2', W + '_q', 1); self.e('lit t8 0')
        if P > 64: self.e('lit t6 1', 'lit t7 %d' % (P - 65), 'alu_shl t6 t6 t7', 'sel t1 t3 t8 t1', 'sel t2 t3 t6 t2')
        else: self.c('t6', 1 << 63); self.e('sel t1 t3 t6 t1', 'sel t2 t3 t8 t2')
        self.st(W + '_q', 't1', 0); self.st(W + '_q', 't2', 1)
        self.ld('t4', R + '_E'); self.e('alu_add t4 t4 t3'); self.st(R + '_E2', 't4')
        # bicim
        self.ld('t1', W + '_nm'); self.ld('t2', R + '_s'); self.ld('t3', R + '_E2'); self.ld('t4', W + '_q', 0); self.ld('t5', W + '_q', 1)
        if not fmt['ex']:
            self.c('t6', bias - 1); self.e('alu_add t6 t6 t3', 'lit t7 0', 'sel t6 t1 t6 t7', 'lit t7 48', 'alu_shl t6 t6 t7', 'alu_add t5 t5 t6')
            self.e('lit t7 48', 'alu_shr t6 t5 t7', 'lit t8 32767', 'cmp_ge t8 t6 t8')
            self.c('t9', 0x7FFF << 48); self.e('lit t10 0', 'sel t5 t8 t9 t5', 'sel t4 t8 t10 t4')
            self.e('lit t7 63', 'alu_shl t2 t2 t7', 'alu_or t5 t5 t2')
        else:
            self.c('t6', bias); self.e('alu_add t6 t6 t3', 'lit t7 63', 'alu_shr t8 t4 t7', 'sel t6 t1 t6 t8')
            self.e('lit t8 32767', 'cmp_ge t8 t6 t8'); self.c('t9', 1 << 63); self.e('sel t4 t8 t9 t4', 'lit t10 32767', 'sel t6 t8 t10 t6')
            self.e('lit t7 15', 'alu_shl t2 t2 t7', 'alu_or t5 t6 t2')
        self.st(W, 't4', 0); self.st(W, 't5', 1)
    # ---- ozel degerler
    def mk_defnan(self, name, fmt):
        if not fmt['ex']: self.c('t1', 0); self.st(name, 't1', 0); self.c('t1', 0xFFFF800000000000); self.st(name, 't1', 1)
        else: self.c('t1', 0xC000000000000000); self.st(name, 't1', 0); self.c('t1', 0xFFFF); self.st(name, 't1', 1)
    def mk_inf(self, name, fmt, svar):
        self.ld('t2', svar)
        if not fmt['ex']:
            self.c('t1', 0); self.st(name, 't1', 0); self.e('lit t3 63', 'alu_shl t2 t2 t3'); self.c('t4', 0x7FFF << 48); self.e('alu_or t2 t2 t4'); self.st(name, 't2', 1)
        else:
            self.c('t1', 1 << 63); self.st(name, 't1', 0); self.e('lit t3 15', 'alu_shl t2 t2 t3', 'lit t4 32767', 'alu_or t2 t2 t4'); self.st(name, 't2', 1)
    def mk_zero(self, name, fmt, svar):
        self.ld('t2', svar); self.c('t1', 0); self.st(name, 't1', 0)
        self.e('lit t3 %d' % (63 if not fmt['ex'] else 15), 'alu_shl t2 t2 t3'); self.st(name, 't2', 1)
    def mk_quiet(self, name, fmt, X):   # X_rl/X_rh -> name_lo/name_hi sessiz
        self.ld('t1', X + '_rl'); self.ld('t2', X + '_rh')
        if not fmt['ex']: self.e('lit t3 1', 'lit t4 47', 'alu_shl t3 t3 t4', 'alu_or t2 t2 t3')
        else: self.e('lit t3 1', 'lit t4 62', 'alu_shl t3 t3 t4', 'alu_or t1 t1 t3')
        self.st(name, 't1', 0); self.st(name, 't2', 1)
    def mk_raw(self, name, X): self.ld('t1', X + '_rl'); self.st(name, 't1', 0); self.ld('t2', X + '_rh'); self.st(name, 't2', 1)
    def flip_raw(self, name, fmt, X, flipvar=None):   # ham sozcuklerde isaret biti cevir (flipvar 0/1)
        self.ld('t1', X + '_rl'); self.ld('t2', X + '_rh')
        self.ld('t3', flipvar) if flipvar else self.e('lit t3 1')
        self.e('lit t4 %d' % (63 if not fmt['ex'] else 15), 'alu_shl t3 t3 t4', 'alu_xor t2 t2 t3'); self.st(name, 't1', 0); self.st(name, 't2', 1)
    def flag(self, name, X, k):   # name = (X_c == k)
        self.ld('t1', X + '_c'); self.e('lit t2 %d' % k, 'cmp_eq t1 t1 t2'); self.st(name, 't1')
    def andf(self, d, a, b): self.ld('t1', a); self.ld('t2', b); self.e('alu_and t1 t1 t2'); self.st(d, 't1')
    def orf(self, d, a, b): self.ld('t1', a); self.ld('t2', b); self.e('alu_or t1 t1 t2'); self.st(d, 't1')
    def chain(self, base, order):   # base: sonuc degisken adi; order: [(kosul_degiskeni, aday_adi)] dusukten yuksege oncelik; sonuc W2_lo/W2_hi
        self.copy2('Z', base)
        for cond, cand in order:
            self.ld('t9', cond); self.sel2('Z', 't9', cand, 'Z')
    def emit_wide(self, src='Z'):
        self.ld('t1', src, 0); self.ld('t2', src, 1); self.e('wrw a 0 t1', 'wrw a 1 t2')
