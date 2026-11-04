# Bagimsiz kovalamaca benzetimi. Girdi: kovala.girdi. Cikti: "skor kalan oyuncu_x oyuncu_y".
function rnd(i, seed, lo, hi,   h) { h = (i * 1664525 + seed * 1013904223 + 12345) % 16777216; h = (h * 1664525 + 1013904223) % 16777216; return lo + int(h / 16) % (hi - lo + 1) }
function ov(ax, ay, bx, by, bw) { return (ax + 3 > bx && ax < bx + bw && ay + 3 > by && ay < by + bw) }
{ ev_f[NR] = $1; ev_k[NR] = $2; n = NR }
END {
  px = 40; py = 30; skor = 0; next_ev = 1
  for (i = 0; i < 30; i++) { gx[i] = rnd(i, 21, 2, 150); gy[i] = rnd(i, 22, 2, 110); alive[i] = 1 }
  for (i = 0; i < 3; i++) { dx[i] = rnd(i, 23, 0, 159); dy[i] = rnd(i, 24, 0, 119) }
  for (f = 0; f < 500; f++) {
    while (next_ev <= n && ev_f[next_ev] <= f) { k = ev_k[next_ev]; next_ev++; if (k == 1) px -= 2; else if (k == 2) px += 2; else if (k == 3) py -= 2; else if (k == 4) py += 2 }
    if (px < 0) px = 0; if (px > 157) px = 157; if (py < 0) py = 0; if (py > 117) py = 117
    for (i = 0; i < 3; i++) {
      if (f % 2 == 0) { if (dx[i] < px) dx[i]++; if (dx[i] > px) dx[i]--; if (dy[i] < py) dy[i]++; if (dy[i] > py) dy[i]-- }
    }
    for (j = 0; j < 30; j++) if (alive[j] && ov(px, py, gx[j], gy[j], 3)) { alive[j] = 0; skor++ }
    for (j = 0; j < 3; j++) if (ov(px, py, dx[j], dy[j], 4)) { skor -= 5; dx[j] = 150; dy[j] = 110; px = 40; py = 30 }
    kalan = 0; for (j = 0; j < 30; j++) kalan += alive[j]
    if (kalan == 0) break
  }
  print skor, kalan, px, py
}
