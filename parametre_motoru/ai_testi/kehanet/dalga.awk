# GOREV 2 kehaneti: 192 kare, faz kurali; cikti: son karenin PPM govdesi (bayt/satir).
BEGIN {
  for (i = 0; i < 192; i++) ph[i] = ((i % 16) * 16 + int(i / 16) * 8) % 256
  for (f = 0; f < 60; f++) for (i = 0; i < 192; i++) ph[i] = (ph[i] + int(i / 16) + 1) % 256
  for (k = 0; k < 19200; k++) fb[k] = 0
  for (i = 0; i < 192; i++) {
    p = ph[i]; col = (p >= 224) ? 16777215 : p * 65536 + (255 - p) * 256 + 64
    x0 = (i % 16) * 10; y0 = int(i / 16) * 10
    for (b = 0; b < 10; b++) for (a = 0; a < 10; a++) fb[(y0 + b) * 160 + x0 + a] = col
  }
  for (k = 0; k < 19200; k++) { v = fb[k]; print int(v / 65536) % 256; print int(v / 256) % 256; print v % 256 }
}
