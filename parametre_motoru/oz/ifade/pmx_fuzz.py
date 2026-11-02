#!/usr/bin/env python3
"""PMX1 sayi listesini rastgele bozar; pm ifade --pmx1-oku bozuk girdide asla cokmemeli (cikis kodu 0 ya da 1). Cikti: cokme sayisi (0 = temiz)."""
import sys, random, subprocess
pm, f, n, tmp = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
v = open(f).read().split(); r = random.Random(12345); bad = 0; first = ''
for i in range(n):
    w = list(v); k = r.randrange(3, len(w))
    for _ in range(r.choice([1, 1, 2, 3])):
        k = r.randrange(3, len(w)); w[k] = r.choice([str(r.randrange(-3, 70)), str(r.randrange(0, len(w))), '-1', '999999999', str(1 << 40)])
    if r.random() < 0.15: w = w[:r.randrange(3, len(w))]
    open(tmp + '/m.pmx', 'w').write(' '.join(w))
    try: c = subprocess.run([pm, 'ifade', 'x', '--pmx1-oku', tmp + '/m.pmx', '--pma', tmp + '/m.pma'], capture_output=True, timeout=30)
    except subprocess.TimeoutExpired: bad += 1; first = first or 'zaman asimi'; continue
    if c.returncode not in (0, 1) or b'Sanitizer' in c.stderr: bad += 1; first = first or 'rc=%d' % c.returncode
print(0 if bad == 0 else '%d cokme/gecersiz cikis (%s)' % (bad, first))
