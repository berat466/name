# Bagimsiz Pong benzetimi: ayni kurallar, ayni olay semasi. Girdi: pong.girdi ("kare kod"). Cikti: "skor raket top_x top_y".
# Olay, o kareye (sunulan kare sayisi >= olay karesi) gelince teslim edilir; her iterasyonda kuyruktaki uygun olaylarin hepsi okunur.
{ ev_f[NR] = $1; ev_k[NR] = $2; n = NR }
END {
  px = 33; bx = 10; by = 10; vx = 1; vy = 1; score = 0; next_ev = 1
  for (f = 0; f < 400; f++) {
    while (next_ev <= n && ev_f[next_ev] <= f) { key = ev_k[next_ev]; next_ev++; if (key == 1) px -= 3; else if (key == 2) px += 3 }
    if (px < 0) px = 0
    if (px > 66) px = 66
    bx += vx; by += vy
    if (bx < 0) { bx = 0; vx = -vx }
    if (bx >= 80) { bx = 79; vx = -vx }
    if (by < 0) { by = 0; vy = -vy }
    if (by >= 55 && vy > 0) {
      if (bx >= px - 1 && bx <= px + 14) { vy = -vy; by = 55; score++ }
      else { score--; bx = 40; by = 10; vx = 1; vy = 1 }
    }
  }
  print score, px, bx, by
}
