# fmt.txt -> data STRT/STO (NUL ayrilmis metinler, baslangic konumlari) ve @F_AD@ -> indeks eslemesi. Referans (awk): fmtc.pma ile karsilastirilir.
/^#/ || /^$/ { next }
{ name = $1; text = substr($0, length($1) + 2); gsub(/\|/, "\n", text); id[n] = name; start[n] = pos; n++
  for (i = 1; i <= length(text); i++) { c = substr(text, i, 1); data = data " " ord(c); pos++ }
  data = data " 0"; pos++ }
function ord(c) { if (c == "\n") return 10; return index(ASC, c) + 31 }
BEGIN { n = 0; pos = 0; ASC = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" }
END { for (i = 0; i < n; i++) print "F_" id[i], i; s = ""; for (i = 0; i < n; i++) s = s " " start[i]; print "STO" s; print "STRT" data }
