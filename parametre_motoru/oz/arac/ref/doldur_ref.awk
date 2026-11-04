# doldur.pma referansi (awk): sozluk ("ANAHTAR deger", ilk tanim gecerli, '#' yorum) + "@@SABLON" + sablon. Bilinmeyen anahtar -> cikis 1.
BEGIN { insab = 0 }
!insab && /^@@SABLON$/ { insab = 1; next }
!insab { if ($0 ~ /^#/ || $0 ~ /^[ \t]*$/) next; k = $1; v = $0; sub(/^[^ \t]+[ \t]*/, "", v); sub(/\r$/, "", v); if (!(k in D)) D[k] = v; next }
{ line = $0; out = ""
  while (1) {
    i = index(line, "@"); if (!i) { out = out line; break }
    out = out substr(line, 1, i - 1); rest = substr(line, i + 1)
    if (match(rest, /^[A-Za-z0-9_]+/) && substr(rest, RLENGTH + 1, 1) == "@") { key = substr(rest, 1, RLENGTH); if (!(key in D)) { bad = 1; exit 1 } out = out D[key]; line = substr(rest, RLENGTH + 2) }
    else { out = out "@"; line = rest }
  }
  print out }
END { if (bad) exit 1 }
