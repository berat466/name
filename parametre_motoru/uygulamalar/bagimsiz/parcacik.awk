# Bagimsiz parcacik benzetimi: ayni uretec (LCG), ayni kurallar. Cikti: son karenin PPM govdesi (R G B bayt/satir).
function rnd(i, seed, lo, hi,   h) { h = (i * 1664525 + seed * 1013904223 + 12345) % 16777216; h = (h * 1664525 + 1013904223) % 16777216; return lo + int(h / 16) % (hi - lo + 1) }
BEGIN {
  N = 10000
  for (i = 0; i < N; i++) { x[i] = rnd(i, 1, 0, 159); y[i] = rnd(i, 2, 0, 60); vy[i] = rnd(i, 3, 1, 3); c[i] = rnd(i, 4, 4210752, 16777215) }
  for (f = 0; f < 120; f++) {
    for (i = 0; i < N; i++) {
      vy[i] += 1; y[i] += vy[i]
      if (y[i] > 119) { vy[i] = 0 - vy[i]; vy[i] = vy[i] * 3; vy[i] = int(vy[i] / 4); y[i] = 119 }
    }
  }
  for (k = 0; k < 19200; k++) fb[k] = 16   # 0x000010
  for (i = 0; i < N; i++) if (x[i] >= 0 && x[i] < 160 && y[i] >= 0 && y[i] < 120) fb[y[i] * 160 + x[i]] = c[i]
  for (k = 0; k < 19200; k++) { v = fb[k]; print int(v / 65536) % 256; print int(v / 256) % 256; print v % 256 }
}
