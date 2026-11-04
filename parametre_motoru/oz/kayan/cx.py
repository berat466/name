#!/usr/bin/env python3
"""Karmasik sayi komutlari mikro-program ureticileri (GELISTIRME araci; cikti: komut tablosu SEM). Temsil (yazmac): c64 = 2 sozcuk (gercek, sanal; her biri f32 degeri double kabinda),
c128 = 2 sozcuk (iki double), c160 = 4 sozcuk (gercek: f80 lo,hi ; sanal: f80 lo,hi). Anlam: ISO C 'CX_LIMITED_RANGE' (duz formuller, NaN/sonsuz kurtarma YOK):
  carpma  (a+bi)(c+di) = (ac - bd) + (ad + bc)i
  bolme   (a+bi)/(c+di) = ((ac + bd) / (cc + dd)) + ((bc - ad) / (cc + dd))i
  eq: iki bilesen esit ; ne: eq degil ; tobool: gercek != 0 ya da sanal != 0 ; neg: iki bilesenin isareti ; conj: sanalin isareti.
Her bilesen islemi kendi gercek turunun yuvarlamasiyla yapilir (f32: her islem sonrasi f32'ye; f64: duz; f80: yazilim kayan nokta alt islemleriyle)."""
import ops, sf
from sf import G, F80, F64
def sub(g, fn, b=None, c=None, a=None):
    """alt islem (ops.py uretecleri) : operand yuvalari kazima degiskenlerine eslenir, hedef degiskene yazilir"""
    G.cur = g; g.sm = {k: v for k, v in (('b', b), ('c', c)) if v}; g.dm = a
    try: fn()
    finally: G.cur = None; g.sm = {}; g.dm = None
def load_slot(g, slot, var, nw):
    for k in range(nw): g.e('rdw t1 %s %d' % (slot, k)); g.st(var, 't1', k)
def store_out(g, var, nw):
    for k in range(nw): g.ld('t1', var, k); g.e('wrw a %d t1' % k)
# ---------------- f32 bilesen islemleri (double kabi) : islem + f32'ye yuvarla ----------------
RND32 = None
def real32(g, op, d, x, y):
    """d = x op y (f32 yuvarlamali): x,y,d 1 sozcuk kazima degiskenleri (double kabinda f32 degeri)"""
    g.ld('t1', x); g.ld('t2', y); g.e('%s t3 t1 t2' % op, 'alu_or t1 t3 t3')
    g.e(*RND32[1:-1]); g.st(d, 't3')
def rnd32(g, d, x):
    g.ld('t1', x); g.e(*RND32[1:-1]); g.st(d, 't3')
def comp_double(op, d, g, x, y): g.ld('t1', x); g.ld('t2', y); g.e('%s t3 t1 t2' % op); g.st(d, 't3')
# ---------------- genel karmasik islemler : bilesen islem arayuzu ----------------
class Real:
    """bilesen turu: sozcuk sayisi nw ; islem(g, 'add'|'sub'|'mul'|'div', d, x, y) ; ld/st ; flip(g, var) isaret cevir ; eq(g, d, x, y) (d: 1 sozcuk 0/1) ; nz(g, d, x) (x != 0)"""
    def __init__(self, kind): self.kind = kind; self.nw = 2 if kind == 'f80' else 1
    def name(self, base): return ('V_cx' + base) if self.nw == 2 else ('cx_' + base)
    def op(self, g, o, d, x, y):
        if self.kind == 'f32': real32(g, {'add': 'fadd', 'sub': 'fsub', 'mul': 'fmul', 'div': 'fdiv'}[o], d, x, y)
        elif self.kind == 'f64': comp_double({'add': 'fadd', 'sub': 'fsub', 'mul': 'fmul', 'div': 'fdiv'}[o], d, g, x, y)
        else: sub(g, {'add': lambda: ops.addsub(F80, False), 'sub': lambda: ops.addsub(F80, True), 'mul': lambda: ops.mul(F80), 'div': lambda: ops.div(F80)}[o], b=x, c=y, a=d)
    def flip(self, g, v):   # isaret biti cevir
        if self.nw == 1: g.ld('t1', v); g.e('lit t2 1', 'lit t3 63', 'alu_shl t2 t2 t3', 'alu_xor t1 t1 t2'); g.st(v, 't1')
        else: g.ld('t1', v, 1); g.e('lit t2 32768', 'alu_xor t1 t1 t2'); g.st(v, 't1', 1)
    def eq(self, g, d, x, y):
        if self.nw == 1: g.ld('t1', x); g.ld('t2', y); g.e('feq t3 t1 t2'); g.st(d, 't3')
        else: sub(g, lambda: ops.cmp_(F80, 'eq'), b=x, c=y, a=d)
    def nz(self, g, d, x):   # NaN -> 1
        if self.nw == 1: g.ld('t1', x); g.e('lit t2 0', 'feq t3 t1 t2', 'lit t4 1', 'alu_xor t3 t3 t4'); g.st(d, 't3')
        else: sub(g, lambda: ops.tobool(F80), b=x, a=d)
def ctype(name): return {'c64': Real('f32'), 'c128': Real('f64'), 'c160': Real('f80')}[name]
def build(cname, kind):
    """karmasik komut: kind: add sub mul div neg conj mov eq ne tobool mk real imag"""
    R = ctype(cname); nw = R.nw; g = G.newG(); N = lambda b: R.name(b)
    ar, ai, br, bi = N('ar'), N('ai'), N('br'), N('bi')
    if kind in ('mk',):
        for k in range(nw): g.rdw('t1', 'b', k) if nw == 2 else g.rd1('t1', 'b'); g.st(ar, 't1', k)
        for k in range(nw): g.rdw('t1', 'c', k) if nw == 2 else g.rd1('t1', 'c'); g.st(ai, 't1', k)
        if cname == 'c64': rnd32(g, ar, ar); rnd32(g, ai, ai)
        store_pair(g, ar, ai, nw); return g
    # operandlari kazima degiskenlerine al
    src = lambda slot, rv, iv: (load_slot(g, slot, rv, nw), None) and None
    def ld_c(slot, rv, iv):
        for k in range(nw): g.rdw('t1', slot, k); g.st(rv, 't1', k)
        for k in range(nw): g.rdw('t1', slot, nw + k); g.st(iv, 't1', k)
    if kind in ('real', 'imag'):
        base = 0 if kind == 'real' else nw
        if nw == 1: g.rdw('t1', 'b', base); g.out1('t1')
        else: g.rdw('t1', 'b', base); g.rdw('t2', 'b', base + 1); g.out2('t1', 't2')
        return g
    ld_c('b', ar, ai)
    if kind in ('add', 'sub', 'mul', 'div', 'eq', 'ne'): ld_c('c', br, bi)
    if kind in ('add', 'sub'):
        R.op(g, kind, N('rr'), ar, br); R.op(g, kind, N('ri'), ai, bi); store_pair(g, N('rr'), N('ri'), nw)
    elif kind == 'mul':
        R.op(g, 'mul', N('p1'), ar, br); R.op(g, 'mul', N('p2'), ai, bi); R.op(g, 'sub', N('rr'), N('p1'), N('p2'))
        R.op(g, 'mul', N('p3'), ar, bi); R.op(g, 'mul', N('p4'), ai, br); R.op(g, 'add', N('ri'), N('p3'), N('p4')); store_pair(g, N('rr'), N('ri'), nw)
    elif kind == 'div':
        R.op(g, 'mul', N('p1'), br, br); R.op(g, 'mul', N('p2'), bi, bi); R.op(g, 'add', N('dn'), N('p1'), N('p2'))
        R.op(g, 'mul', N('p3'), ar, br); R.op(g, 'mul', N('p4'), ai, bi); R.op(g, 'add', N('nr'), N('p3'), N('p4'))
        R.op(g, 'mul', N('p5'), ai, br); R.op(g, 'mul', N('p6'), ar, bi); R.op(g, 'sub', N('ni'), N('p5'), N('p6'))
        R.op(g, 'div', N('rr'), N('nr'), N('dn')); R.op(g, 'div', N('ri'), N('ni'), N('dn')); store_pair(g, N('rr'), N('ri'), nw)
    elif kind == 'neg': R.flip(g, ar); R.flip(g, ai); store_pair(g, ar, ai, nw)
    elif kind == 'conj': R.flip(g, ai); store_pair(g, ar, ai, nw)
    elif kind == 'mov': store_pair(g, ar, ai, nw)
    elif kind in ('eq', 'ne'):
        R.eq(g, 'cx_q1', ar, br); R.eq(g, 'cx_q2', ai, bi); g.ld('t1', 'cx_q1'); g.ld('t2', 'cx_q2'); g.e('alu_and t1 t1 t2')
        if kind == 'ne': g.e('lit t2 1', 'alu_xor t1 t1 t2')
        g.out1('t1')
    elif kind == 'tobool':
        R.nz(g, 'cx_q1', ar); R.nz(g, 'cx_q2', ai); g.ld('t1', 'cx_q1'); g.ld('t2', 'cx_q2'); g.e('alu_or t1 t1 t2'); g.out1('t1')
    else: raise Exception(kind)
    return g
def store_pair(g, rv, iv, nw):
    """sonuc (gercek, sanal) -> hedef yuva a: nw sozcuk + nw sozcuk"""
    for k in range(nw): g.ld('t1', rv, k); g.e('wrw a %d t1' % k)
    for k in range(nw): g.ld('t1', iv, k); g.e('wrw a %d t1' % (nw + k))
def conv(src, dst, rnd32_sem=None):
    """karmasik -> karmasik donusum (bilesen bilesen)"""
    g = G.newG(); Rs, Rd = ctype(src), ctype(dst); ns, nd = Rs.nw, Rd.nw
    ar, ai = Rs.name('ar'), Rs.name('ai')
    for k in range(ns): g.rdw('t1', 'b', k); g.st(ar, 't1', k)
    for k in range(ns): g.rdw('t1', 'b', ns + k); g.st(ai, 't1', k)
    kinds = (src, dst)
    def cvt(d_, s_):
        a, b = ('f32', 'f64', 'f80')[('c64', 'c128', 'c160').index(src)], ('f32', 'f64', 'f80')[('c64', 'c128', 'c160').index(dst)]
        if a == b or (a == 'f32' and b == 'f64'): return [('copy',)]
        if a == 'f64' and b == 'f32': return [('rnd32',)]
        if b == 'f80': return [('d2w',)]            # f32/f64 -> f80
        if a == 'f80' and b == 'f64': return [('w2d',)]
        if a == 'f80' and b == 'f32': return [('w2n',)]
        raise Exception((a, b))
    step = cvt(None, None)[0][0]
    for sv, dv in ((ar, Rd.name('rr')), (ai, Rd.name('ri'))):
        if step == 'copy':
            for k in range(ns): g.ld('t1', sv, k); g.st(dv, 't1', k)
        elif step == 'rnd32': rnd32(g, dv, sv)
        elif step == 'd2w': sub(g, lambda: ops.w2w(F64, F80), b=sv, a=dv)
        elif step == 'w2d': sub(g, lambda: ops.w2w(F80, F64, final='one'), b=sv, a=dv)
        elif step == 'w2n': sub(g, lambda: ops.w2n(F80, 'f32', ' ; '.join(RND32)), b=sv, a=dv)
    store_pair(g, Rd.name('rr'), Rd.name('ri'), nd); return g
def from_real(real, dst, rnd32_sem=None):
    """gercek (f64 kabi, f80 genis) -> karmasik (sanal 0)"""
    g = G.newG(); Rd = ctype(dst); nd = Rd.nw
    if real == 'f64':
        g.rd1('t1', 'b'); g.st('cx_x', 't1')
        if dst == 'c64': rnd32(g, 'cx_rr', 'cx_x')
        elif dst == 'c128': g.ld('t1', 'cx_x'); g.st('cx_rr', 't1')
        else: sub(g, lambda: ops.w2w(F64, F80), b='cx_x', a='V_cxrr')
    else:   # f80 (2 sozcuk)
        g.rdw('t1', 'b', 0); g.rdw('t2', 'b', 1); g.st('V_cxx', 't1', 0); g.st('V_cxx', 't2', 1)
        if dst == 'c160': g.copy2('V_cxrr', 'V_cxx')
        elif dst == 'c128': sub(g, lambda: ops.w2w(F80, F64, final='one'), b='V_cxx', a='cx_rr')
        else: sub(g, lambda: ops.w2n(F80, 'f32', ' ; '.join(RND32)), b='V_cxx', a='cx_rr')
    rv = 'V_cxrr' if nd == 2 else 'cx_rr'; iv = 'V_cxri' if nd == 2 else 'cx_ri'
    g.e('lit t1 0')
    for k in range(nd): g.st(iv, 't1', k)
    store_pair(g, rv, iv, nd); return g
