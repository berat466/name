# Test uretecinin parcasi: rastgele GECERLI (bicim) PMK1 sayisal tablosu uretir. awk -v seed=N [-v dense=1] -f rastgele_pmk.awk
# dense=1: komut/host kimlikleri 0..n-1 yogun, komut basina en cok 3 operand (arac profili); dense=0: seyrek kimlikler, 5 operand, bilinmeyen bolumler.
function sd(s) { RNG_X = (s * 7919 + 17) % 4294967296 }
function rnd() { RNG_X = (RNG_X * 1664525 + 1013904223) % 4294967296; return RNG_X / 4294967296 }   # TEKRARLANABILIR (mawk srand/rand tohumla ayni diziyi vermiyor)
function rd(n) { return int(rnd() * n) }
function nm(lo, hi, up,   s, l, i, al) { al = up ? "ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "abcdefghijklmnopqrstuvwxyz"; l = lo + rd(hi - lo + 1); s = substr(al, 1 + rd(26), 1); for (i = 1; i < l; i++) s = s substr(al "0123456789_", 1 + rd(37), 1); return s }
function chars(s,   i, r) { r = ""; for (i = 1; i <= length(s); i++) r = r " " ord[substr(s, i, 1)]; return r }
function ext(   n, i, r) { n = rd(3); r = n; for (i = 0; i < n; i++) r = r " " (rd(20) - 5) " " (rd(2000) - 500); n = rd(3); if (rd(3) == 0) for (i = 0; i < n; i++) r = r " " (rd(300) - 50); return r }
function rec(body,   a, n) { n = split(body, a, " "); return n " " body }
BEGIN {
  sd(seed + 0); for (i = 0; i < 256; i++) ord[sprintf("%c", i)] = i
  # KIND
  nkind = 2 + rd(10); CH = "dr fsiklFbhxyzABC"; kt = ""
  perm = ""
  for (i = 1; i <= nkind; i++) { do { id = dense ? i : 1 + rd(255) } while (id in usedk); usedk[id] = 1; KID[i] = id; do { c = substr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", 1 + rd(62), 1) } while (c in usedc); usedc[c] = 1
    kt = kt " " rec(id " " ord[c] " " rd(65536) " " ext()) }
  # OP
  nop = 1 + rd(40); ot = ""
  for (i = 0; i < nop; i++) OPC[i] = i
  if (!dense) for (i = 0; i < nop; i++) OPC[i] = 7 * i + 3 * rd(2)
  for (i = 0; i < nop; i++) {
    do { sy = nm(1, 12, 1) } while (sy in uss); uss[sy] = 1
    do { na = nm(1, 10, 0) } while (na in usn); usn[na] = 1
    nk = rd(dense ? 4 : 6); ks = ""; for (j = 0; j < nk; j++) ks = ks " " KID[1 + rd(nkind)]
    ot = ot " " rec(OPC[i] " " length(sy) chars(sy) " " length(na) chars(na) " " nk ks " " ext())
  }
  # HOST
  nh = rd(8); ht = ""
  for (i = 0; i < nh; i++) { do { na = nm(1, 9, 0) } while (na in ush); ush[na] = 1; hid = dense ? i : i * 3 + rd(3); ht = ht " " rec(hid " " rd(5) " " length(na) chars(na) " " ext()) }
  # bolumler (sirasi karisik) + bilinmeyen bolumler: ID FLAG VER KAYITSAYISI(-1: opak) GOVDE
  ns = 0
  ns++; SID[ns] = 1; SFL[ns] = rd(2); SVR[ns] = 1; SCN[ns] = nkind; SBD[ns] = kt
  ns++; SID[ns] = 2; SFL[ns] = rd(2); SVR[ns] = 1; SCN[ns] = nop; SBD[ns] = ot
  ns++; SID[ns] = 3; SFL[ns] = rd(2); SVR[ns] = 1; SCN[ns] = nh; SBD[ns] = ht
  nu = dense ? rd(2) : rd(4)
  for (u = 0; u < nu; u++) { ns++; SID[ns] = 10 + rd(190); SFL[ns] = rd(2) * 2; SVR[ns] = 1 + rd(3); SCN[ns] = -1; m = rd(15); p = ""; for (q = 0; q < m; q++) p = p " " (rd(1000) - 200); SBD[ns] = p }
  if (!dense && rd(3) == 0) { ns++; SID[ns] = 2; SFL[ns] = 0; SVR[ns] = 2; SCN[ns] = -1; m = rd(8); p = ""; for (q = 0; q < m; q++) p = p " " rd(50); SBD[ns] = p }
  for (i = ns; i > 1; i--) { j = 1 + rd(i); t = SID[i]; SID[i] = SID[j]; SID[j] = t; t = SFL[i]; SFL[i] = SFL[j]; SFL[j] = t; t = SVR[i]; SVR[i] = SVR[j]; SVR[j] = t; t = SCN[i]; SCN[i] = SCN[j]; SCN[j] = t; t = SBD[i]; SBD[i] = SBD[j]; SBD[j] = t }
  printf "%d %d %d %d\n", 1347242801, 1, rd(4), ns
  for (i = 1; i <= ns; i++) {
    if (SCN[i] < 0) { nb = split(SBD[i], b, " "); printf "%d %d %d %d%s\n", SID[i], SFL[i], SVR[i], nb, SBD[i] }
    else { nb = split(SBD[i], b, " ") + 1; printf "%d %d %d %d %d%s\n", SID[i], SFL[i], SVR[i], nb, SCN[i], SBD[i] }
  }
}
