# GOREV 1 kehaneti (gorev metnine gore bagimsiz benzetim). Cikti: "skor can gemi_x"; -v ppm=dosya ile son sunulan karenin PPM govdesi (bayt/satir).
function put(x, y, col) { if (x < 0 || x >= 160 || y < 0 || y >= 120) return; fb[y * 160 + x] = col }
function rect(x, y, w, h, col,   a, b) { for (b = 0; b < h; b++) for (a = 0; a < w; a++) put(x + a, y + b, col) }
{ ev_f[NR] = $1; ev_k[NR] = $2; n = NR }
END {
  gx = 76; skor = 0; can = LIVES; next_ev = 1
  for (i = 0; i < 8; i++) { mx[i] = 10 + 18 * i; my[i] = -14 * i; mv[i] = 2 + (i % 3) }
  have = 0
  for (f = 0; f < 600; f++) {
    while (next_ev <= n && ev_f[next_ev] <= f) { k = ev_k[next_ev]; next_ev++; if (k == 1) gx -= 4; else if (k == 2) gx += 4 }
    if (gx < 0) gx = 0; if (gx > 152) gx = 152
    for (i = 0; i < 8; i++) my[i] += mv[i]
    for (i = 0; i < 8; i++) if (gx < mx[i] + 8 && gx + 8 > mx[i] && 110 < my[i] + 8 && 110 + 6 > my[i]) { can--; my[i] = -8; mx[i] = (mx[i] + 37) % 152 }
    for (i = 0; i < 8; i++) if (my[i] > 119) { skor++; my[i] = -8; mx[i] = (mx[i] + 37) % 152 }
    if (can == 0) break
    for (k = 0; k < 19200; k++) fb[k] = 32      # 0x000020
    for (i = 0; i < 8; i++) rect(mx[i], my[i], 8, 8, 12615744)   # 0xC08040
    rect(gx, 110, 8, 6, 4259648)                                 # 0x40FF40
    have = 1
    for (k = 0; k < 19200; k++) last[k] = fb[k]
  }
  print skor, can, gx
  if (ppm != "" && have) for (k = 0; k < 19200; k++) { v = last[k]; print int(v / 65536) % 256 > ppm; print int(v / 256) % 256 > ppm; print v % 256 > ppm }
}
