BEGIN { best = 0; arg = 0
  for (n = 1; n <= 3000; n++) { x = n; st = 0; while (x != 1) { x = (x % 2 == 0) ? x / 2 : 3 * x + 1; st++ }
    if (n == 27) s27 = st; tot += st; if (st > 100) over++; if (st > best) { best = st; arg = n } }
  printf "%d %d %d %d %d\n", s27, arg, best, tot, over }
