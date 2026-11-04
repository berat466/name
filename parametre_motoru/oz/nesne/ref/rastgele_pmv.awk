# Rastgele PMO1 gorunumu (PMV) ureteci: oz/arac/gorunum.pmb (PMA) ile oz/nesne/ref/pmo_ref.py (bagimsiz referans) bu girdilerde karsilastirilir.
# Kullanim: awk -v seed=N -v dict=SOZLUK.txt [-v hata=OLASILIK_YUZDE] -f rastgele_pmv.awk      (SOZLUK: `komut @@tab` ciktisi; komutlar ve operand siniflari oradan okunur)
# Uretim TEKRARLANABILIR (kendi LCG'si; mawk srand/rand tohumla ayni diziyi vermez). Gecerli gorunumlerin yaninda `hata` yuzdesi ile bilerek gecersiz operand de uretir.
function sd(s) { RNG_X = (s * 7919 + 17) % 4294967296 }
function rnd() { RNG_X = (RNG_X * 1664525 + 1013904223) % 4294967296; return RNG_X / 4294967296 }
function ri(n) { return int(rnd() * n) }
function pick(str,   a, n) { n = split(str, a, " "); return a[1 + ri(n)] }
function bit(f, b) { return int(f / b) % 2 }
function cls(op, j,   k, f) {
  k = kd[op * 3 + j]; f = kfl[k] + 0
  if (bit(f, 1) || bit(f, 2)) return "r"
  if (bit(f, 4)) return "i"
  if (bit(f, 8)) return "l"
  if (bit(f, 16)) return "F"
  if (bit(f, 32)) return "k"
  if (bit(f, 128)) return "h"
  return "n"
}
function imm(   c, r, k, sp) {
  r = ri(100)
  if (r < 30) return pick("0 1 -1 2 5 7 100 255 65535 2147483647 -2147483648 31 64 -7 4096")
  if (r < 33) return pick("2147483648 -2147483649 99999999999 007 -0 1.5 abc $ & ~ ")
  if (r < 45 && nsc > 0) return (ri(3) == 0 ? "-" : "") "$" sc[1 + ri(nsc)]
  if (r < 50 && nar > 0 && nhead > 0) { k = hd[1 + ri(nhead)]; return "&" k (ri(2) ? "+" ri(5) : "") }
  if (r < 58 && nar > 0 && nhead > 0) { k = hd[1 + ri(nhead)]; return "~" k (ri(2) ? "+" ri(5) : "") }
  if (r < 65) return "&bss" (ri(2) ? "+" ri(100000) : "")
  if (r < 73 && nsp > 0) return "&" spn[1 + ri(nsp)] (ri(2) ? "+" ri(50) : "")
  if (r < 85 && nar > 0) {
    k = ar[1 + ri(nar)]
    if (ri(3) == 0 && nar > 1) { c = ar[1 + ri(nar)]; if (c != k) return ri(10) "{" k "*" pick("1 2 -1 3") "|" c "*" pick("1 1 2") "}" }
    return ri(500) "{" k "*" pick("1 1 1 2 -1") "}"
  }
  if (r < 88 && nar > 0) return ri(10) "{" ar[1 + ri(nar)] "*0}"
  return ri(2000) - 1000
}
BEGIN {
  sd(seed); if (hata == "") hata = 0
  while ((getline line < dict) > 0) {
    n = split(line, w, " ")
    if (w[1] == "OPCOUNT") nop = w[2] + 0
    else if (w[1] == "HOSTCOUNT") nhost = w[2] + 0
    else if (w[1] == "OPNAMES") { nm = ""; k = 0; for (i = 2; i <= n; i++) { if (w[i] == 0) { nms[k++] = nm; nm = "" } else nm = nm sprintf("%c", w[i]) } }
    else if (w[1] == "NOPS") { for (i = 2; i <= n; i++) nops[i - 2] = w[i] + 0 }
    else if (w[1] == "KIND") { for (i = 2; i <= n; i++) kd[i - 2] = w[i] + 0 }
    else if (w[1] == "KFLAGS") { for (i = 2; i <= n; i++) kfl[i - 2] = w[i] + 0 }
  }
  print "; rastgele gorunum " seed
  print "pmo 1"
  flag = ri(2) == 0 ? 1 : 0
  print "flags " flag
  print "memory " pick("0 1 4096 16777216 123456789")
  nf = 1 + ri(4)
  for (f = 0; f < nf; f++) fn[f] = (f == 0 ? "main" : pick("fa fb helper g_") f)
  print "entry " (ri(8) == 0 ? "#" ri(6) : fn[ri(nf)])
  nk = 1 + ri(6); nsc = 0; nar = 0
  for (i = 0; i < nk; i++) {
    nmk = pick("K A_B OPX ZZ Q9 NOPS") i
    if (ri(2)) { sc[++nsc] = nmk; kk[i] = "scalar" } else { ar[++nar] = nmk; kk[i] = "array" }
    kn[i] = nmk
  }
  if (ri(5) == 0) { nk = 0; nsc = 0; nar = 0 }
  for (i = 0; i < nk; i++) print "key " kn[i] " " kk[i]
  nk_out = nk
  # sabitler
  line = "const"
  nc = ri(4)
  for (i = 0; i < nc; i++) { if (nsc > 0 && ri(4) == 0) line = line " $" sc[1 + ri(nsc)]; else line = line " " pick("0 1 255 4294967295 9223372036854775807 9223372036854775808 18446744073709551615") }
  print line
  print "data"
  # veri: her dizi anahtari en cok bir kez
  nhead = 0
  for (i = 1; i <= nar; i++) used[ar[i]] = 0
  nitems = ri(7)
  for (i = 0; i < nitems; i++) {
    if (ri(2) && nar > 0) {
      k = ar[1 + ri(nar)]
      if (!used[k]) { used[k] = 1; hd[++nhead] = k; print "  array " k; continue }
    }
    line = "  words"; m = 1 + ri(4)
    for (j = 0; j < m; j++) { if (nsc > 0 && ri(5) == 0) line = line " " (ri(3) == 0 ? "-" : "") "$" sc[1 + ri(nsc)]; else line = line " " pick("0 1 -1 65 100 255 2147483647 -9223372036854775808 9223372036854775807") }
    print line
  }
  bss = pick("0 100 4096 12345678")
  print "bss " bss
  nsp = 0
  off = 0
  ns = ri(4)
  for (i = 0; i < ns; i++) {
    off += ri(1 + (bss + 0) / 4 + 1)
    if (off > bss + 0) break
    spn[++nsp] = pick("SP IB NM") i
    print "space " spn[nsp] " " off
    off += 1
  }
  # islevler
  for (f = 0; f < nf; f++) {
    print ""
    np = ri(3); nr = np + ri(6)
    print "func " fn[f] " " np " " nr
    ni = 1 + ri(12); nlab = ri(4)
    for (l = 0; l < nlab; l++) lab[l] = "L" l
    for (l = 0; l < nlab; l++) lpos[l] = ri(ni + 1)
    for (p = 0; p <= ni; p++) {
      for (l = 0; l < nlab; l++) if (lpos[l] == p) print "L" l ":"
      if (p == ni) break
      op = ri(nop)
      line = "  " nms[op]
      for (j = 0; j < nops[op]; j++) {
        c = cls(op, j)
        if (c == "r") v = "r" ri(8)
        else if (c == "i") v = imm()
        else if (c == "l") v = (nlab > 0 && ri(10) > 0) ? lab[ri(nlab)] : (ri(5) == 0 ? "@" ri(20) : "nolabel")
        else if (c == "F") v = ri(8) > 0 ? fn[ri(nf)] : (ri(2) ? "#" ri(10) : "nofunc")
        else if (c == "k") v = "c" ri(5)
        else if (c == "h") v = ri(6) > 0 ? nms[nop + ri(nhost)] : ri(10)
        else v = ri(20)
        if (ri(100) < hata) v = pick("zz $ -0 007 r r-- c#")
        line = line (j == 0 ? " " : ", ") v
      }
      print line
    }
    print "end"
  }
}
