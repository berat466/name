# .pma metnini bozar: awk -v seed=N -v k=KAC -f mutpma.awk kaynak.pma  (satir silme/kopyalama/takas, jeton degistirme/silme, karakter degistirme, rastgele ekleme)
function r(n) { x = (x * 1664525 + 1013904223) % 4294967296; return int(x / 65536) % n }
BEGIN { x = seed * 7919 + 17; if (k == "") k = 1
  nj = split("add sub mul r0 r1 x y 0 1 -1 5 2147483648 -2147483648 4294967296 0x10 0xZ 'a' '\\n' '' main loop L 3.5 1e3 ; , { } ( ) : \" gt ge fgt fge mov ldi st ld halt ret jmp call host putc puti getc memory data space entry func", J, " ") }
{ L[NR] = $0 }
END {
  for (m = 0; m < k; m++) {
    t = r(100); i = r(NR) + 1; n = split(L[i], T, " ")
    if (t < 40 && n > 0) { j = r(n) + 1; T[j] = J[r(nj) + 1]; s = T[1]; for (q = 2; q <= n; q++) s = s " " T[q]; L[i] = s }
    else if (t < 50 && n > 1) { j = r(n) + 1; s = ""; for (q = 1; q <= n; q++) if (q != j) s = s (s == "" ? "" : " ") T[q]; L[i] = s }
    else if (t < 60) L[i] = ""
    else if (t < 68) L[i] = L[i] "\n" L[i]
    else if (t < 76) { j = r(NR) + 1; tmp = L[i]; L[i] = L[j]; L[j] = tmp }
    else if (t < 90 && length(L[i]) > 0) { p = r(length(L[i])) + 1; L[i] = substr(L[i], 1, p - 1) substr("abz09_:,;\"' -+x", r(15) + 1, 1) substr(L[i], p + 1) }
    else L[i] = L[i] " " J[r(nj) + 1]
  }
  for (i = 1; i <= NR; i++) print L[i]
}
