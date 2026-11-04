# GOREV 3 kehaneti. Cikti: sekiz sayi tek satirda.
BEGIN {
  N = 5000; first = -1; mx = -1; mn = 1e18
  for (i = 0; i < N; i++) {
    h = (i * 1664525 + 41 * 1013904223 + 12345) % 16777216; h = (h * 1664525 + 1013904223) % 16777216
    ham = 100 + int(h / 16) % 401
    k = int(ham * 3 / 2) - 10; al = (k > 700) ? 1 : 0
    tot += k; alarm += al; if (k > mx) mx = k; if (k < mn) mn = k
    if (i % 8 == 3) { s3 += k; s3a += al }
    if (al && (first < 0)) first = i
  }
  printf "%d %d %d %d %d %d %d %d\n", tot, alarm, mx, mn, s3, s3a, first, int(tot / N)
}
