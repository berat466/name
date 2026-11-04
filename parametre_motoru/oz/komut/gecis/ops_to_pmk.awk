# TEK SEFERLIK GECIS + TEST ORAKLI: eski src/ops.h (X-makro) -> kanonik sayisal komut tablosu (PMK1 v1.0), SAYI AKISI olarak.
# Kullanim: awk -f ops_to_pmk.awk ops.h   (cikti: kanonik bicim; komut @@yaz ciktisiyla bayt-bayt karsilastirilir)
# Bicim: magic 1347242801, major 1, minor 0, nsec 3; bolum: id bayrak surum uzunluk yuk... ; kayitlar: uzunluk alanlar...
function chars(s,   i, r) { r = ""; for (i = 1; i <= length(s); i++) r = r " " ord[substr(s, i, 1)]; return r }
BEGIN {
  for (i = 0; i < 256; i++) ord[sprintf("%c", i)] = i
  kn = split("d r f s i k l F b h", KC, " ")
  kfl["d"] = 2; kfl["r"] = 1; kfl["f"] = 1; kfl["s"] = 1; kfl["i"] = 4; kfl["k"] = 32; kfl["l"] = 8; kfl["F"] = 16; kfl["b"] = 64; kfl["h"] = 128
  nh = split("putc puti putf getc present poll frame", HN, " "); split("1 1 1 0 3 0 0", HA, " "); split("1 1 1 2 3 3 3", HC, " ")
}
/^  X\(/ {
  s = $0; sub(/^  X\(/, "", s); split(s, a, ","); sym = a[1]; gsub(/ /, "", sym)
  q = s; sub(/^[^"]*"/, "", q); nm = q; sub(/".*/, "", nm); k = q; sub(/^[^"]*"[^"]*", *[0-9]+ *, *"/, "", k); k = ""; 
  t = $0; sub(/.*"[a-z0-9]+", *[0-9] *, *"/, "", t); sub(/".*/, "", t); k = t
  nops++; SYM[nops] = sym; NM[nops] = nm; KS[nops] = k
}
END {
  # KIND bolumu
  kt = ""; for (i = 1; i <= kn; i++) { c = KC[i]; kt = kt " 4 " i " " ord[c] " " kfl[c] " 0"; }  # reclen = id,char,flags,ne(0) = 4
  # reclen sayimi: alanlar id char flags + ext sayisi(0) = 4 jeton
  # OP bolumu
  ot = ""
  for (i = 1; i <= nops; i++) {
    nk = length(KS[i]); kk = ""
    for (j = 1; j <= nk; j++) { c = substr(KS[i], j, 1); for (m = 1; m <= kn; m++) if (KC[m] == c) kk = kk " " m }
    rec = (i - 1) " " length(SYM[i]) chars(SYM[i]) " " length(NM[i]) chars(NM[i]) " " nk kk " 0"
    n = split(rec, tmp, " "); ot = ot " " n " " rec
  }
  # HOST bolumu (ext: anahtar 1 = yetenek sinifi)
  ht = ""
  for (i = 1; i <= nh; i++) { rec = (i - 1) " " HA[i] " " length(HN[i]) chars(HN[i]) " 1 1 " HC[i]; n = split(rec, tmp, " "); ht = ht " " n " " rec }
  nk_tok = split(kt, tmp, " "); no_tok = split(ot, tmp, " "); nh_tok = split(ht, tmp, " ")
  print 1347242801, 1, 0, 3
  print 1, 1, 1, nk_tok + 1, kn, substr(kt, 2)
  print 2, 1, 1, no_tok + 1, nops, substr(ot, 2)
  print 3, 1, 1, nh_tok + 1, nh, substr(ht, 2)
}
