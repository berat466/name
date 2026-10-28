# Bagimsiz sahne cizici: ayni sekil tablosu, ayni tamsayi kurallari; cikti: PPM govdesi (R G B her satirda bir bayt).
function lerp(c0, c1, j, hm1,   d) { if (hm1 == 0) return c0; d = int((c1 - c0) * j / hm1); return c0 + d }
function put(x, y, col) { if (x < 0 || x >= 160 || y < 0 || y >= 120) return; fb[y * 160 + x] = col }
BEGIN {
  n = split("2 0 0 160 120 2109536 8438015   0 0 80 160 40 2121760 0   1 130 25 14 0 16769088 0   0 40 50 50 35 11550768 0   0 35 44 60 8 6299680 0   0 58 62 12 23 4202512 0   0 46 56 8 8 14737536 0   0 76 56 8 8 14737536 0   0 110 60 6 25 6307872 0   1 113 52 14 0 1081376 0   1 30 20 8 0 16777215 0   1 40 18 10 0 16777215 0   1 52 21 8 0 16777215 0", t, " ")
  for (i = 0; i < 19200; i++) fb[i] = 0
  for (s = 0; s < 13; s++) {
    o = s * 7; type = t[o + 1]; a = t[o + 2]; b = t[o + 3]; w = t[o + 4]; h = t[o + 5]; c0 = t[o + 6]; c1 = t[o + 7]
    if (type == 0) { for (yy = 0; yy < h; yy++) for (xx = 0; xx < w; xx++) put(a + xx, b + yy, c0) }
    else if (type == 1) { for (yy = -w; yy <= w; yy++) for (xx = -w; xx <= w; xx++) if (xx * xx + yy * yy <= w * w) put(a + xx, b + yy, c0) }
    else {
      hm1 = h - 1
      r0 = int(c0 / 65536) % 256; g0 = int(c0 / 256) % 256; b0 = c0 % 256
      r1 = int(c1 / 65536) % 256; g1 = int(c1 / 256) % 256; b1 = c1 % 256
      for (yy = 0; yy < h; yy++) {
        col = lerp(r0, r1, yy, hm1) * 65536 + lerp(g0, g1, yy, hm1) * 256 + lerp(b0, b1, yy, hm1)
        for (xx = 0; xx < w; xx++) put(a + xx, b + yy, col)
      }
    }
  }
  for (i = 0; i < 19200; i++) { v = fb[i]; print int(v / 65536) % 256; print int(v / 256) % 256; print v % 256 }
}
