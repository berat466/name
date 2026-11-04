# Liste bicimini bozar: awk -v seed=N -v k=KAC -f mut.awk liste.txt  (satir yapisi korunur; belirteç/satir degistirme, silme, kopyalama, ekleme)
function sd(s) { RNG_X = (s * 7919 + 17) % 4294967296 }
function rnd() { RNG_X = (RNG_X * 1664525 + 1013904223) % 4294967296; return RNG_X / 4294967296 }   # TEKRARLANABILIR (mawk srand/rand tohumla ayni diziyi vermiyor)
BEGIN { sd(seed); if (k == "") k = 1
  nv = split("0 1 -1 2 3 4 5 7 8 31 33 34 40 54 55 63 64 65534 65535 65536 4194304 4194305 134217728 134217729 2147483647 2147483648 -2147483648 -2147483649 9223372036854775807 -9223372036854775808 18446744073709551615 18446744073709551616 9223372036854775808 524288 524289 256 257 255", V, " ") }
{ L[NR] = $0 }
END {
  for (m = 0; m < k; m++) {
    r = rnd(); i = int(rnd() * NR) + 1
    n = split(L[i], T, " ")
    if (r < 0.62 && n > 0) { j = int(rnd() * n) + 1; c = rnd()
      if (c < 0.55) T[j] = V[int(rnd() * nv) + 1]
      else if (c < 0.8) T[j] = T[j] + 1
      else if (c < 0.95) T[j] = T[j] - 1
      else T[j] = int(rnd() * 40) - 5
      s = T[1]; for (q = 2; q <= n; q++) s = s " " T[q]; L[i] = s }
    else if (r < 0.72 && n > 1) { j = int(rnd() * n) + 1; s = ""; for (q = 1; q <= n; q++) if (q != j) s = s (s == "" ? "" : " ") T[q]; L[i] = s }
    else if (r < 0.80) { L[i] = L[i] " " V[int(rnd() * nv) + 1] }
    else if (r < 0.88) { L[i] = "" }
    else if (r < 0.94) { L[i] = L[i] "\n" L[i] }
    else { j = int(rnd() * NR) + 1; t = L[i]; L[i] = L[j]; L[j] = t }
  }
  for (i = 1; i <= NR; i++) print L[i]
}
