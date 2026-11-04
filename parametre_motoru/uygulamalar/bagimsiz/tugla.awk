# Bagimsiz tugla kirma benzetimi (sablon/etkilesim sirasi ve yarisma kurallari aynen). Girdi: tugla.girdi. Cikti: "skor kalan top_x top_y".
{ ev_f[NR] = $1; ev_k[NR] = $2; n = NR }
END {
  px = 33; bx = 40; by = 40; vx = 1; vy = -1; skor = 0; kalan = 50; next_ev = 1
  for (i = 0; i < 50; i++) { tx[i] = 4 + 7 * (i % 10); ty[i] = 6 + 5 * int(i / 10); alive[i] = 1 }
  for (f = 0; f < 600; f++) {
    while (next_ev <= n && ev_f[next_ev] <= f) { k = ev_k[next_ev]; next_ev++; if (k == 1) px -= 3; else if (k == 2) px += 3 }
    if (px < 0) px = 0
    if (px > 66) px = 66
    bx += vx; by += vy
    if (bx < 0) { vx = 0 - vx; bx = 0 }
    if (bx > 79) { vx = 0 - vx; bx = 79 }
    if (by < 0) { vy = 0 - vy; by = 0 }
    for (j = 0; j < 50; j++) if (alive[j] && bx + 2 > tx[j] && bx < tx[j] + 6 && by + 2 > ty[j] && by < ty[j] + 3) { alive[j] = 0; vy = 0 - vy; skor++ }
    if (by >= 54 && vy > 0 && bx >= px - 2 && bx <= px + 14) { vy = 0 - vy; by = 54 }
    if (by >= 58 && vy > 0) { skor--; bx = 40; by = 40; vy = -1 }
    kalan = 0; for (j = 0; j < 50; j++) kalan += alive[j]
    if (kalan == 0) break
  }
  print skor, kalan, bx, by
}
