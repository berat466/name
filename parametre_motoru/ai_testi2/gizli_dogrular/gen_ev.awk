# -v seed=N -v n=ADET : olay dosyasi uretir (kare kod)
BEGIN { x = seed * 7919 + 13; f = 0
  for (k = 0; k < n; k++) {
    x = (x * 1664525 + 1013904223) % 4294967296; f += int(x / 65536) % 5 > 2 ? 2 : 0
    x = (x * 1664525 + 1013904223) % 4294967296; if (f > 99) f = 99
    key = int(x / 65536) % 7 + 1; if (key == 6) key = 3; if (key == 7) key = 9
    print f, key } }
