import sf, ref, mikro, ops, sys, random
from sf import F128, F80
fm = {'f128': F128, 'f80': F80}
nbad = 0
for name in ('f128', 'f80'):
    g = ops.mul(fm[name]); print(name, 'mul', len(g.o), 'mikro-islem', g.n, 'kazima')
    vals = ref.rand_vals(name, 40, seed=9); r = random.Random(5)
    pairs = [(x, y) for x in vals[:26] for y in vals[:26]] + [(r.choice(vals), r.choice(vals)) for _ in range(150)] + ref.near_pairs(name, 300, seed=21)
    # carpim icin us toplami kucuk/buyuk/alt-normal cifleri
    f = ref.FM[name]; P = f['P']
    for _ in range(300):
        e1 = f['bias'] + r.randrange(-60, 61); e2 = r.choice([-e1 + f['bias'] * 1 + f['bias'] - 1 + r.randrange(-3, 4), f['bias'] + r.randrange(-60, 61), 2 * f['bias'] - e1 + r.randrange(-P - 4, 4)])
        e2 = max(0, min(0x7FFE, e2)); e1 = max(1, min(0x7FFE, e1))
        m1 = r.getrandbits(P - 1) | (1 << (P - 1)); m2 = r.getrandbits(P - 1) | (1 << (P - 1))
        pairs.append((ref.pack(name, r.randrange(2), e1, m1), ref.pack(name, r.randrange(2), e2, m2)))
    bad = n = 0
    for x, y in pairs:
        R = [x[0], x[1], y[0], y[1], 0, 0]; mikro.calistir(g.o, R, a=4, b=0, c=2)
        e = ref.op_mul(name, x, y); n += 1
        if (R[4], R[5]) != e:
            bad += 1
            if bad <= 4: print('  FARK', hex(x[0]), hex(x[1]), hex(y[0]), hex(y[1]), '->', hex(R[4]), hex(R[5]), 'beklenen', hex(e[0]), hex(e[1]))
    print('  ', n, 'cift', bad, 'fark'); nbad += bad
sys.exit(1 if nbad else 0)
