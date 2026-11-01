import sf, ref, mikro, ops, sys, random
from sf import F128, F80
fm = {'f128': F128, 'f80': F80}
nbad = 0
for name in ('f128', 'f80'):
    for sub in (False, True):
        g = ops.addsub(fm[name], sub); print(name, 'sub' if sub else 'add', len(g.o), 'mikro-islem', g.n, 'kazima')
        vals = ref.rand_vals(name, 40, seed=7); r = random.Random(3)
        pairs = [(x, y) for x in vals[:26] for y in vals[:26]] + [(r.choice(vals), r.choice(vals)) for _ in range(150)] + ref.near_pairs(name, 400)
        bad = n = 0
        for x, y in pairs:
            R = [x[0], x[1], y[0], y[1], 0, 0]; mikro.calistir(g.o, R, a=4, b=0, c=2)
            e = ref.op_addsub(name, x, y, sub); n += 1
            if (R[4], R[5]) != e:
                bad += 1
                if bad <= 4: print('  FARK', hex(x[0]), hex(x[1]), hex(y[0]), hex(y[1]), '->', hex(R[4]), hex(R[5]), 'beklenen', hex(e[0]), hex(e[1]))
        print('  ', n, 'cift', bad, 'fark'); nbad += bad
sys.exit(1 if nbad else 0)
