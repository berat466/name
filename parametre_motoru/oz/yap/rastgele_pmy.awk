# Rastgele GECERLI PMY1 modeli uretir (sayi listesi bicimi: ai_testi3/paketle.awk ile baytlara cevrilir). awk -v seed=N -f rastgele_pmy.awk
# Baglam duyarli: alan/aalan/balan/indis yalniz uygun baglamda, kir/devam yalniz dongude, don yalniz islevde, param yalniz davranista.
function r(n) { x = (x * 1664525 + 1013904223) % 4294967296; return int(x / 65536) % n }
function z(v) { return "z" v }
function sayi_tok(  q) { q = r(8); return q == 0 ? r(5) : q == 1 ? -r(5) : q == 2 ? r(1000) : q == 3 ? -r(1000) : q == 4 ? 2147483647 - r(3) : q == 5 ? -2147483648 + r(3) : q == 6 ? r(100) : 0 }
function leaf(  c, k, opts, n) {
  n = 0
  opts[n++] = "1 " z(sayi_tok()); opts[n++] = "1 " z(sayi_tok()); opts[n++] = "6"; opts[n++] = "18"
  if (NG > 0) { opts[n++] = "3 " r(NG); opts[n++] = "3 " r(NG) }
  if (CT >= 0) { opts[n++] = "5"; opts[n++] = "2 " r(NFD[CT]) ; opts[n++] = "2 " r(NFD[CT]) }
  if (CA >= 0) { opts[n++] = "9"; opts[n++] = "10"; opts[n++] = "7 " r(NFD[CA]); opts[n++] = "8 " r(NFD[CB]) }
  if (CNLOC > 0 && !INBH) { opts[n++] = "4 " r(CNLOC) }
  if (INBH && NPB > 0) { opts[n++] = "14 " r(NPB); opts[n++] = "14 " r(NPB) }
  return opts[r(n)]
}
function idxexpr(size, d) {
  if (r(2)) return "1 " z(r(size))
  return "15 5 " E(d + 1) " 1 " z(size)       # ikili % -> sabit olmayan: calisma zamani denetimi
}
function E(d,   q, f, na, s, i) {
  if (d >= 3 || r(3) == 0) return leaf()
  q = r(12)
  if (q <= 3) return "15 " (1 + r(20)) " " E(d + 1) " " E(d + 1)
  if (q == 4) return "16 " (1 + r(3)) " " E(d + 1)
  if (q == 5) return "17 " E(d + 1) " " E(d + 1) " " E(d + 1)
  if (q == 6 && NA > 0) { f = r(NA); return "12 " f " " idxexpr(ASZ[f], d) }
  if (q == 7 && NTM > 0) { f = r(NTM); return "11 " f " " r(NFD[f]) " " idxexpr(TN[f], d) }
  if (q == 8 && CFN_MAX > 0) { f = r(CFN_MAX); na = FNP[f]; s = "13 " f " " na; for (i = 0; i < na; i++) s = s " " E(d + 1); return s }
  return leaf()
}
function lval(d,   n, opts, f, t) {
  n = 0
  if (NG > 0) { opts[n++] = "3 " r(NG) }
  if (CT >= 0) { opts[n++] = "2 " r(NFD[CT]) }
  if (CA >= 0) { opts[n++] = "7 " r(NFD[CA]); opts[n++] = "8 " r(NFD[CB]) }
  if (CNLOC > 0 && !INBH) { opts[n++] = "4 " r(CNLOC) }
  if (NA > 0) { f = r(NA); opts[n++] = "12 " f " " idxexpr(ASZ[f], d) }
  if (NTM > 0) { t = r(NTM); opts[n++] = "11 " t " " r(NFD[t]) " " idxexpr(TN[t], d) }
  if (n == 0) return ""
  return opts[r(n)]
}
function S(d, loop,   q, l, n, s, i, f, na, b) {
  q = r(14)
  if (q <= 3) { l = lval(d); if (l == "") return "8 " E(1); return "1 " l " " E(1) }
  if (q == 4) return "2 " E(1) " " BLK(d + 1, loop) " " BLK(d + 1, loop)
  if (q == 5 && d < 2) return "3 " E(1) " " BLK(d + 1, 1)
  if (q == 6 && d < 2 && CNLOC > 0 && !INBH) return "4 " r(CNLOC) " " E(1) " " E(1) " " BLK(d + 1, 1)
  if (q == 7 && CFN_MAX > 0) { f = r(CFN_MAX); na = FNP[f]; s = "5 " f " " na; for (i = 0; i < na; i++) s = s " " E(1); return s }
  if (q == 8 && NBH_MAX > 0 && !INBH) { b = r(NBH_MAX); na = BNP[b]; s = "6 " b " " na; for (i = 0; i < na; i++) s = s " " E(1); return s }
  if (q == 9 && CFN && !INBH) return "7 " E(1)
  if (q == 10) return "8 " E(1)
  if (q == 11) return "9 " E(1)
  if (q == 12 && loop) return (r(2) ? "10" : "11")
  if (q == 13) { n = 1 + r(3); s = "12 " n; for (i = 0; i < n; i++) s = s " " E(1); return s }
  return "8 " E(1)
}
function BLK(d, loop,   n, s, i) {
  n = r(4); if (d > 3) n = r(2)
  s = n; for (i = 0; i < n; i++) s = s " " S(d, loop)
  return s
}
BEGIN {
  x = seed * 7919 + 101
  # sahne ve global/dizi
  scr = r(2); W = scr ? 4 + r(6) : 0; H = scr ? 3 + r(5) : 0
  NG = r(4); NA = r(3); NTM = r(4); NF_ = r(3); NB_ = r(3); NE_ = 0; NGI = 0; NAG = 0
  frames = 0
  out = "80 77 89 49 " W " " H " " r(16777216) " "
  # model ogeleri uretilmeden once boyutlari belirle
  for (i = 0; i < NTM; i++) { TN[i] = 1 + r(5); NFD[i] = 1 + r(3) }
  for (i = 0; i < NA; i++) ASZ[i] = 1 + r(6)
  for (i = 0; i < NF_; i++) { FNP[i] = r(3); FNL[i] = FNP[i] + r(3) }
  for (i = 0; i < NB_; i++) BNP[i] = r(3)
  # ---- yerlesim sirasi: frames, globals, arrays, fns, bhs, tms, ets, gis, ags, bitis, ilk, son
  # (frames daha sonra karar verilir: kare dongusu gerekirse) -> once tum govdeleri uret
  CT = -1; CA = -1; CB = -1; CFN = 0; CNLOC = 16; INBH = 0; NPB = 0; CFN_MAX = 0; NBH_MAX = 0
  gl = NG; for (i = 0; i < NG; i++) gl = gl " " z(sayi_tok())
  ar = NA; for (i = 0; i < NA; i++) { ni = r(ASZ[i] + 1); ar = ar " " ASZ[i] " " ni; for (j = 0; j < ni; j++) ar = ar " " z(sayi_tok()) }
  fs = NF_
  for (i = 0; i < NF_; i++) { CFN = 1; CNLOC = FNL[i]; CFN_MAX = i; fs = fs " " FNP[i] " " FNL[i] " " BLK(0, 0) }
  CFN = 0; CNLOC = 16; CFN_MAX = NF_
  bs = NB_
  for (i = 0; i < NB_; i++) { INBH = 1; NPB = BNP[i]; NBH_MAX = i; bs = bs " " BNP[i] " " BLK(0, 0) }
  INBH = 0; NPB = 0; NBH_MAX = NB_
  ts = NTM; anyframe = 0
  for (i = 0; i < NTM; i++) {
    ts = ts " " TN[i] " " NFD[i]
    for (f = 0; f < NFD[i]; f++) {
      g = 1 + r(5)
      if (g == 1) ts = ts " 1 " z(sayi_tok())
      else if (g == 2) ts = ts " 2 " z(sayi_tok()) " " z(r(10))
      else if (g == 3) ts = ts " 3 " (1 + r(4)) " " z(r(10)) " " z(1 + r(5))
      else if (g == 4) ts = ts " 4 " (1 + r(4)) " " z(r(10)) " " z(1 + r(5))
      else { lo = r(10); ts = ts " 5 " z(lo) " " z(lo + r(10)) " " z(r(1000)) }
    }
    CT = i; CNLOC = 16
    nb = BLK(0, 1); ts = ts " " nb; if (substr(nb, 1, 2) != "0 " && nb != "0") anyframe = 1
    nd = scr ? r(3) : 0; ts = ts " " nd; if (nd) anyframe = 1
    for (j = 0; j < nd; j++) { if (r(2)) ts = ts " 1 " E(1) " " E(1) " " E(1) " " E(1) " " E(1); else ts = ts " 2 " E(1) " " E(1) " " E(1) }
  }
  CT = -1
  es = ""; ne = 0
  if (NTM >= 2) { ne = r(3); for (i = 0; i < ne; i++) { a = r(NTM); b = (a + 1 + r(NTM - 1)) % NTM; CA = a; CB = b; es = es " " a " " b " " E(1) " " BLK(0, 1) } ; if (ne) anyframe = 1 }
  CA = -1; CB = -1
  gs = ""; ngi = 0
  if (r(2)) { ngi = 1 + r(2); for (i = 0; i < ngi; i++) { t = NTM > 0 ? r(NTM + 1) - 1 : -1; CT = t; gs = gs " " (65 + r(10)) " " (t + 1) " " BLK(0, 1) } ; anyframe = 1 }
  CT = -1
  as = ""; nag = 0
  if (NG > 0 && NTM > 0 && r(2)) { nag = 1 + r(2); for (i = 0; i < nag; i++) { t = r(NTM); CT = t; as = as " " r(NG) " " (1 + r(4)) " " t " " E(1) " " E(1) } ; anyframe = 1 }
  CT = -1
  hb = r(3) == 0
  bit = hb ? E(1) : ""
  if (hb) anyframe = 1
  ilk = BLK(0, 0); son = BLK(0, 0)
  # kare dongusu kurali: anyframe ise sahne ya da kare>0 ya da bitis gerekir
  if (anyframe && !scr && !hb) frames = 1 + r(4)
  else if (!scr && !hb && r(2)) frames = 0
  else if (r(3) == 0) frames = r(4)
  if (!scr && frames == 0 && !hb && anyframe) frames = 2
  # cizim yoksa ama sahne varsa: sorun degil
  rest = " " frames " " gl " " ar " " fs " " bs " " ts " " NE_
  print out frames " " gl " " ar " " fs " " bs " " ts " " ne " " es " " ngi " " gs " " nag " " as " " (hb ? "1 " bit : "0") " " ilk " " son
}
