# Gorunum metnini rastgele bozar (jeton silme/cogaltma/degistirme, satir silme/cogaltma/takas, kesme). Kullanim: awk -v seed=N [-v k=bozulma_sayisi] -f mutpmv.awk < gorunum
function sd(s) { RNG_X = (s * 7919 + 17) % 4294967296 }
function rnd() { RNG_X = (RNG_X * 1664525 + 1013904223) % 4294967296; return RNG_X / 4294967296 }
function ri(n) { return int(rnd() * n) }
BEGIN { sd(seed); if (k == "") k = 1
  ng = split("zz $ & ~ { } -0 007 99999999999999999999 -9223372036854775809 18446744073709551616 r r-1 c-2 #3 @-1 K0 end func data array words bss space L0: 5 -5 0 1 2147483648 *2 |", G, " ") }
{ L[NR] = $0 }
END {
  n = NR
  for (m = 0; m < k; m++) {
    t = ri(8); i = 1 + ri(n)
    if (t == 0 || t == 1 || t == 2) {   # jeton islemleri
      nt = split(L[i], w, " ")
      if (nt == 0) continue
      j = 1 + ri(nt)
      if (t == 0) w[j] = ""
      else if (t == 1) w[j] = w[j] " " w[j]
      else w[j] = G[1 + ri(ng)]
      s = ""; for (q = 1; q <= nt; q++) s = s (w[q] == "" ? "" : (s == "" ? "" : " ") w[q])
      L[i] = s
    } else if (t == 3) L[i] = ""
    else if (t == 4) L[i] = L[i] "\n" L[i]
    else if (t == 5) { j = 1 + ri(n); s = L[i]; L[i] = L[j]; L[j] = s }
    else if (t == 6) { for (q = i; q <= n; q++) L[q] = "" }
    else L[i] = L[i] " " G[1 + ri(ng)]
  }
  for (i = 1; i <= n; i++) print L[i]
}
