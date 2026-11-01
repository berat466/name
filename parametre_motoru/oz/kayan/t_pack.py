import sf, ref, mikro, sys
from sf import F128, F80
def prog(fmt):
    g = sf.G()
    g.unpack('A', fmt, 'b')
    for n in ('s', 'E'): g.ld('t1', 'A_' + n); g.st('R_' + n, 't1')
    g.copy2('R_m', 'A_m'); g.e('lit t1 0'); g.st('R_st', 't1')
    g.pack(fmt, 'R', 'W'); g.ld('t1', 'W', 0); g.ld('t2', 'W', 1); g.e('wrw a 0 t1', 'wrw a 1 t2')
    return g
for name, fmt in (('f128', F128), ('f80', F80)):
    g = prog(fmt); ops = g.o; print(name, len(ops), 'mikro-islem', g.n, 'kazima')
    bad = 0; n = 0
    for (lo, hi) in ref.rand_vals(name, 80):
        cls, s, val = ref.decode(name, lo, hi)
        if cls not in ('fin',): continue
        R = [lo, hi, 0, 0, 0, 0] ; mikro.calistir(ops, R, a=2, b=0, c=0)
        n += 1
        if (R[2], R[3]) != (lo, hi): bad += 1; print('FARK', name, hex(lo), hex(hi), '->', hex(R[2]), hex(R[3])); 
        if bad > 5: break
    print(name, 'tur-gidis-donus', n, 'deger,', bad, 'fark')
