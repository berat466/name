import sf, ref, mikro, ops, cx, sys, random, struct, math, re
from sf import F80
RND = {}
for l in open('/tmp/gor.txt'):
    m = re.match(r'^sem RND_F32 (.*)$', l.strip())
    if m: cx.RND32 = m.group(1).split(' ; ')
M64 = (1 << 64) - 1
def fb(x): return struct.unpack('<Q', struct.pack('<d', x))[0]
def bf(b): return struct.unpack('<d', struct.pack('<Q', b & M64))[0]
def f32(x):
    try: return struct.unpack('<f', struct.pack('<f', x))[0]
    except OverflowError: return math.copysign(math.inf, x)
nbad = 0
def check(title, g, cases, nin, nout, cslot=None):
    global nbad
    bad = 0
    for args, exp in cases:
        R = list(args) + [0] * (nout + 2)
        a = len(args); mikro.calistir(g.o, R, a=a, b=0, c=(cslot if cslot is not None else (nin if len(args) > nin else 0)))
        got = tuple(R[a:a + nout])
        if got != tuple(exp):
            bad += 1
            if bad <= 3: print('  FARK', title, [hex(x) for x in args], '->', [hex(x) for x in got], 'beklenen', [hex(x) for x in exp])
    print('%-16s %4d durum %d fark (%d mikro-islem, %d kazima)' % (title, len(cases), bad, len(g.o), g.n)); nbad += bad
r = random.Random(5)
def fvals(kind, n):
    out = [0.0, -0.0, 1.0, -1.5, 2.5e-300, 1e300, -3.0, 0.1, 1e-310, math.inf, -math.inf, 123456.789]
    while len(out) < n: out.append(r.choice([1, -1]) * r.random() * 10 ** r.randint(-30, 30))
    if kind == 'f32': out = [f32(x) for x in out]
    return out
for cname, kind in (('c128', 'f64'), ('c64', 'f32')):
    vals = fvals(kind, 14)
    rnd = (lambda x: x) if kind == 'f64' else f32
    cs = [(a, b) for a in vals[:7] for b in vals[3:10]][:60] + [(r.choice(vals), r.choice(vals)) for _ in range(40)]
    pairs = [((a, b), (c, d)) for (a, b), (c, d) in zip(cs, cs[::-1]) if not any(math.isinf(x) for x in (a, b, c, d))] + [((a, b), (c, d)) for (a, b), (c, d) in zip(cs[:25], cs[3:28])]
    def w(c): return [fb(c[0]), fb(c[1])]
    def mul(a, b, c, d): return (rnd(rnd(a * c) - rnd(b * d)), rnd(rnd(a * d) + rnd(b * c)))
    def div(a, b, c, d):
        try:
            dn = rnd(rnd(c * c) + rnd(d * d)); nr = rnd(rnd(a * c) + rnd(b * d)); ni = rnd(rnd(b * c) - rnd(a * d))
            return (rnd(nr / dn), rnd(ni / dn))
        except ZeroDivisionError: return None
    for k in ('add', 'sub', 'mul', 'div'):
        cases = []
        for (a, b), (c, d) in pairs:
            if k == 'add': e = (rnd(a + c), rnd(b + d))
            elif k == 'sub': e = (rnd(a - c), rnd(b - d))
            elif k == 'mul':
                if any(math.isinf(x) or math.isnan(x) for x in (a, b, c, d)): continue
                e = mul(a, b, c, d)
            else:
                if any(math.isinf(x) or math.isnan(x) for x in (a, b, c, d)): continue
                e = div(a, b, c, d)
                if e is None or any(math.isnan(x) for x in e): continue
            if any(math.isnan(x) for x in e): continue
            cases.append((w((a, b)) + w((c, d)), [fb(e[0]), fb(e[1])]))
        check('%s_%s' % (k, cname), cx.build(cname, k), cases, 2, 2)
    cases = []
    for (a, b), (c, d) in pairs: cases.append((w((a, b)), [fb(-a), fb(-b)]))
    check('neg_' + cname, cx.build(cname, 'neg'), cases, 2, 2)
    check('conj_' + cname, cx.build(cname, 'conj'), [(w((a, b)), [fb(a), fb(-b)]) for (a, b), _ in pairs], 2, 2)
    check('eq_' + cname, cx.build(cname, 'eq'), [(w((a, b)) + w((c, d)), [int(a == c and b == d)]) for (a, b), (c, d) in pairs], 2, 1)
    check('ne_' + cname, cx.build(cname, 'ne'), [(w((a, b)) + w((c, d)), [int(not (a == c and b == d))]) for (a, b), (c, d) in pairs], 2, 1)
    check('tobool_' + cname, cx.build(cname, 'tobool'), [(w((a, b)), [int(not (a == 0 and b == 0))]) for (a, b), _ in pairs], 2, 1)
    check('mk_' + cname, cx.build(cname, 'mk'), [((fb(a), fb(b)), [fb(rnd(a)), fb(rnd(b))]) for (a, b), _ in pairs], 1, 2)
    check('real_' + cname, cx.build(cname, 'real'), [(w((a, b)), [fb(a)]) for (a, b), _ in pairs], 2, 1)
    check('imag_' + cname, cx.build(cname, 'imag'), [(w((a, b)), [fb(b)]) for (a, b), _ in pairs], 2, 1)
# c160: bilesenler f80
vals = ref.rand_vals('f80', 30, seed=3)
def okv(v): return ref.decode('f80', *v)[0] in ('fin', 'zero')
vals = [v for v in vals if okv(v)]
cs = [(r.choice(vals), r.choice(vals)) for _ in range(90)]
def w160(c): return [c[0][0], c[0][1], c[1][0], c[1][1]]
A = lambda x, y: ref.op_addsub('f80', x, y, False); S = lambda x, y: ref.op_addsub('f80', x, y, True); Mu = lambda x, y: ref.op_mul('f80', x, y); Dv = lambda x, y: ref.op_div('f80', x, y)
pairs = [(cs[i], cs[(i * 7 + 3) % len(cs)]) for i in range(len(cs))]
def isnan80(v): return ref.decode('f80', *v)[0] == 'nan'
for k in ('add', 'sub', 'mul', 'div'):
    cases = []
    for (a, b), (c, d) in pairs[:40 if k in ('mul', 'div') else 80]:
        if k == 'add': e = (A(a, c), A(b, d))
        elif k == 'sub': e = (S(a, c), S(b, d))
        elif k == 'mul': e = (S(Mu(a, c), Mu(b, d)), A(Mu(a, d), Mu(b, c)))
        else:
            dn = A(Mu(c, c), Mu(d, d)); nr = A(Mu(a, c), Mu(b, d)); ni = S(Mu(b, c), Mu(a, d)); e = (Dv(nr, dn), Dv(ni, dn))
        cases.append((w160(((a, b), (c, d)) and (a, b)) + w160((c, d)), list(e[0]) + list(e[1])))
    check('%s_c160' % k, cx.build('c160', k), cases, 4, 4)
neg = lambda v: (v[0], v[1] ^ 0x8000)
check('neg_c160', cx.build('c160', 'neg'), [(w160(p[0]), list(neg(p[0][0])) + list(neg(p[0][1]))) for p in pairs], 4, 4)
check('conj_c160', cx.build('c160', 'conj'), [(w160(p[0]), list(p[0][0]) + list(neg(p[0][1]))) for p in pairs], 4, 4)
eqf = lambda x, y: ref.op_cmp('f80', 'eq', x, y)
check('eq_c160', cx.build('c160', 'eq'), [(w160(p[0]) + w160(p[1]), [int(eqf(p[0][0], p[1][0]) and eqf(p[0][1], p[1][1]))]) for p in pairs] + [(w160(p[0]) + w160(p[0]), [1]) for p in pairs[:20]], 4, 1)
check('ne_c160', cx.build('c160', 'ne'), [(w160(p[0]) + w160(p[1]), [int(not (eqf(p[0][0], p[1][0]) and eqf(p[0][1], p[1][1])))]) for p in pairs] + [(w160(p[0]) + w160(p[0]), [0]) for p in pairs[:20]], 4, 1)
nz = lambda v: int(ref.decode('f80', *v)[0] != 'zero')
check('tobool_c160', cx.build('c160', 'tobool'), [(w160(p[0]), [nz(p[0][0]) | nz(p[0][1])]) for p in pairs] + [(w160(((0, 0), (0, 0))), [0]), (w160(((0, 0x8000), (0, 0))), [0])], 4, 1)
check('mk_c160', cx.build('c160', 'mk'), [(list(p[0][0]) + list(p[0][1]), list(p[0][0]) + list(p[0][1])) for p in pairs], 4, 4, cslot=2)
check('real_c160', cx.build('c160', 'real'), [(w160(p[0]), list(p[0][0])) for p in pairs], 4, 2)
check('imag_c160', cx.build('c160', 'imag'), [(w160(p[0]), list(p[0][1])) for p in pairs], 4, 2)
print('TOPLAM FARK', nbad)
sys.exit(1 if nbad else 0)
