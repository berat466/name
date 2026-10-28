# -v ppm=cikti_dosyasi ; stdin: olay dosyasi (kare kod)
BEGIN { NE = 0 }
{ ef[NE] = $1; ek[NE] = $2; NE++ }
END {
  W = 160; H = 96; for (s = 0; s < 4; s++) v[s] = 4 + 4 * s; sec = 0; nx = 0
  for (f = 0; f < 100; f++) {
    while (nx < NE && ef[nx] <= f) { k = ek[nx++]
      if (k == 1) sec = (sec + 3) % 4; else if (k == 2) sec = (sec + 1) % 4
      else if (k == 3) { if (v[sec] < 20) v[sec]++ } else if (k == 4) { if (v[sec] > 0) v[sec]-- }
      else if (k == 5) for (s = 0; s < 4; s++) v[s] = 0 } }
  for (y = 0; y < H; y++) for (x = 0; x < W; x++) { c = 1052696   # 0x101018
    for (s = 0; s < 4; s++) if (x >= 8 && x < 8 + 7 * v[s] && y >= 8 + 22 * s && y < 22 + 22 * s) c = (s == sec) ? 16760896 : 4227264
    printf "%d\n%d\n%d\n", int(c / 65536), int(c / 256) % 256, c % 256 > ppm }
  printf "%d %d %d %d %d\n", sec, v[0], v[1], v[2], v[3] }
