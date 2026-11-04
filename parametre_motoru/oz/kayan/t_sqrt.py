import sf, ref, mikro, ops, sys, random
from sf import F128, F80
fm = {'f128': F128, 'f80': F80}
nbad = 0
for name in ('f128', 'f80'):
    g = ops.sqrt(fm[name]); print(name, 'sqrt', len(g.o), 'mikro-islem', g.n, 'kazima')
    vals = ref.rand_vals(name, 60, seed=13); f = ref.FM[name]; P = f['P']; r = random.Random(8)
    xs = list(vals)
    for _ in range(80):
        e = r.choice([f['bias'], f['bias'] + 1, f['bias'] - 1, r.randrange(1, 0x7FFE), r.randrange(0, 4)])
        m = r.getrandbits(P - 1) | ((1 << (P - 1)) if e else 0)
        if r.random() < 0.25: m = (1 << (P - 1)) if e else 1       # tam kareler
        if name == 'f128' and e == 0: m &= (1 << 112) - 1
        xs.append(ref.pack(name, 0 if r.random() < 0.85 else 1, e, m))
    bad = 0
    for x in xs:
        R = [x[0], x[1], 0, 0]; mikro.calistir(g.o, R, a=2, b=0, c=0)
        e = ref.op_sqrt(name, x)
        if (R[2], R[3]) != e:
            bad += 1
            if bad <= 4: print('  FARK', hex(x[0]), hex(x[1]), '->', hex(R[2]), hex(R[3]), 'beklenen', hex(e[0]), hex(e[1]))
    print('  ', len(xs), 'deger', bad, 'fark'); nbad += bad
sys.exit(1 if nbad else 0)
