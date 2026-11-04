# Bagimsiz Yasam Oyunu: ayni baslangic (glider + R-pentomino), 32x24 simit, 60 nesil.
# Cikti: "60 <nesil-60 nufusu>" + 59. nesil (en son sunulan kare) 32x24 '#'/'.' haritasi.
function step(   y, x, dy, dx, n, nx, ny) {
  for (y = 0; y < 24; y++) for (x = 0; x < 32; x++) {
    n = 0
    for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dy == 0) continue
      nx = (x + dx + 32) % 32; ny = (y + dy + 24) % 24
      n += c[ny * 32 + nx]
    }
    nx_[y * 32 + x] = (n == 3 || (n == 2 && c[y * 32 + x])) ? 1 : 0
  }
  for (i = 0; i < 768; i++) c[i] = nx_[i]
}
BEGIN {
  split("1 0 2 1 0 2 1 2 2 2 17 10 18 10 16 11 17 11 17 12", s, " ")
  for (i = 0; i < 768; i++) c[i] = 0
  for (k = 1; k <= 20; k += 2) c[s[k + 1] * 32 + s[k]] = 1
  for (g = 0; g < 60; g++) {
    if (g == 59) for (i = 0; i < 768; i++) shown[i] = c[i]
    step()
  }
  pop = 0; for (i = 0; i < 768; i++) pop += c[i]
  print 60, pop
  for (y = 0; y < 24; y++) { l = ""; for (x = 0; x < 32; x++) l = l (shown[y * 32 + x] ? "#" : "."); print l }
}
