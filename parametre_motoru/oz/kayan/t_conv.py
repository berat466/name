import sf, ref, mikro, ops, sys, random, re
from sf import F128, F80, F64
fm = {'f128': F128, 'f80': F80, 'f64': F64}
M64 = (1 << 64) - 1
RND = {}
for l in open('/tmp/gor.txt'):
    m = re.match(r'^sem RND_(F32|F16|BF16) (.*)$', l.strip())
    if m: RND[m.group(1).lower()] = m.group(2)
nbad = 0
def run(g, args, nout=2, a=None):
    R = list(args) + [0] * 8; base = len(args)
    mikro.calistir(g.o, R, a=base, b=0, c=(2 if len(args) >= 4 else 1))
    return tuple(R[base:base + nout])
def check(title, g, cases, nout=2, wmask=None):
    global nbad
    bad = 0
    for args, exp in cases:
        got = run(g, args, nout)
        if nout == 1: exp = (exp if isinstance(exp, int) else exp[0],)
        if tuple(got) != tuple(exp):
            bad += 1
            if bad <= 4: print('  FARK', title, [hex(x) for x in args], '->', [hex(x) for x in got], 'beklenen', [hex(x) for x in exp])
    print('%-14s %5d durum %d fark  (%d mikro-islem, %d kazima)' % (title, len(cases), bad, len(g.o), g.n)); nbad += bad
def dvals(n=70, seed=3):
    r = random.Random(seed); out = []
    for lo in ref.rand_vals('f128', 20, seed=2): pass
    # double ham desenleri
    sp = [0, 1 << 63, 0x7FF0000000000000, 0xFFF0000000000000, 0x7FF8000000000000, 0xFFF0000000000001, 0x7FF0000000000001, 1, (1 << 63) | 1, 0x000FFFFFFFFFFFFF, 0x0010000000000000, 0x7FEFFFFFFFFFFFFF, 0x3FF0000000000000, 0xBFF8000000000000]
    out += sp
    while len(out) < n: out.append(r.getrandbits(64))
    return out
for name in ('f128', 'f80'):
    f = fm[name]; vals = ref.rand_vals(name, 90, seed=31) + [x for x, y in ref.near_pairs(name, 60, seed=5)]
    # d2w
    check('d2%s' % name, ops.d2w(f), [((d,), ref.op_w2w('f64', name, (d, 0))) for d in dvals()])
    # w2d
    check('%s2d' % name, ops.w2w(f, F64, final='one'), [(v, ref.op_w2w(name, 'f64', v)[0]) for v in vals], nout=1)
    # w2w
    other = 'f80' if name == 'f128' else 'f128'
    check('%s2%s' % (name, other), ops.w2w(f, fm[other]), [(v, ref.op_w2w(name, other, v)) for v in vals])
    # w2n
    for nar in ('f32', 'f16', 'bf16'):
        check('%s2%s' % (name, nar), ops.w2n(f, nar, RND[nar]), [(v, ref.op_w2n(name, nar, v)[0]) for v in vals], nout=1)
    # cmp
    pr = [(x, y) for x in vals[:30] for y in vals[:30]][:600] + ref.near_pairs(name, 120, seed=17)
    for k in ('eq', 'ne', 'lt', 'le'):
        check('%s_%s' % (k, name), ops.cmp_(f, k), [((x[0], x[1], y[0], y[1]), ref.op_cmp(name, k, x, y)) for x, y in pr], nout=1)
    check('tobool_' + name, ops.tobool(f), [(v, int(ref.decode(name, *v)[0] != 'zero')) for v in vals], nout=1)
    check('neg_' + name, ops.neg_abs(f, 'neg'), [(v, (v[0], v[1] ^ (1 << (15 if f['ex'] else 63)))) for v in vals])
    check('abs_' + name, ops.neg_abs(f, 'abs'), [(v, (v[0], v[1] & ~(1 << (15 if f['ex'] else 63)))) for v in vals])
    # tamsayi -> wide
    r = random.Random(4)
    iv = [0, 1, -1, 2, 3, (1 << 53) + 1, (1 << 63) - 1, -(1 << 63), (1 << 62) + 3, 1 << 62] + [r.getrandbits(r.randrange(1, 64)) * r.choice([1, -1]) for _ in range(120)]
    iv = [x for x in iv if -(1 << 63) <= x < (1 << 63)]
    check('i2f_' + name, ops.i2w(f, True), [((x & M64,), ref.op_i2w(name, x & M64, True, 64)) for x in iv])
    uv = [0, 1, M64, M64 - 1, 1 << 63, (1 << 63) + 1, (1 << 53) + 1] + [r.getrandbits(r.randrange(1, 65)) for _ in range(120)]
    check('u2f_' + name, ops.i2w(f, False), [((x,), ref.op_i2w(name, x, False, 64)) for x in uv])
    xs = [0, 1, (1 << 128) - 1, 1 << 127, (1 << 127) - 1, (1 << 113) + 1, (1 << 114) + 3, (1 << 64) + 1, (1 << 64)] + [r.getrandbits(r.randrange(1, 129)) for _ in range(120)]
    check('i128_' + name, ops.i128_2w(f, True), [((x & M64, x >> 64), ref.op_i2w(name, x, True, 128)) for x in xs])
    check('u128_' + name, ops.i128_2w(f, False), [((x & M64, x >> 64), ref.op_i2w(name, x, False, 128)) for x in xs])
    # wide -> tamsayi
    fv = list(vals)
    for k in range(60):
        e = f['bias'] + r.choice([-2, -1, 0, 1, 5, 31, 62, 63, 64, 65, 100, 126, 127, 128]); m = r.getrandbits(f['P'] - 1) | (1 << (f['P'] - 1)); fv.append(ref.pack(name, r.randrange(2), e, m))
    for e in (62, 63, 64, 127, 128):   # tam 2^e
        for sg in (0, 1): fv.append(ref.pack(name, sg, f['bias'] + e, 1 << (f['P'] - 1)))
    for bits, sg in ((64, True), (64, False), (128, True), (128, False)):
        ex = []
        for v in fv:
            R = ref.op_w2i(name, v, bits, sg)
            ex.append((v, (R,) if bits == 64 else (R & M64, R >> 64)))
        check('f2%s%d_%s' % ('i' if sg else 'u', bits, name), ops.w2i(f, bits, sg), [(a, e) for a, e in ex], nout=1 if bits == 64 else 2)
    for nb in (8, 16, 32):
        for sg in (True, False):
            ex = []
            for v in fv:
                R = ref.op_w2i(name, v, 64, sg)
                R &= (1 << nb) - 1
                if sg and R >> (nb - 1): R |= M64 ^ ((1 << nb) - 1)
                ex.append((v, (R,)))
            check('f2%s%d_%s' % ('i' if sg else 'u', nb, name), ops.w2i(f, 64, sg, narrow=(nb, sg)), ex, nout=1)
print('TOPLAM FARK', nbad)
sys.exit(1 if nbad else 0)
