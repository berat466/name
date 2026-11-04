BEGIN { N = 4000; mn = 1e9; mx = -1e9
  for (i = 0; i < N; i++) {
    v = (i * i * 7 + i * 13) % 1009 - 500
    b = int((v + 500) / 127); c[b]++
    s += v; if (v < mn) mn = v; if (v > mx) mx = v
    if (i % 3 == 0 && v > 0) p += v
  }
  for (b = 0; b < 8; b++) printf "%d ", c[b]
  printf "%d %d %d %d\n", s, mn, mx, p }
