# Desteklenen altkumeden rastgele .pma uretir (dogrulama araci). -v seed=N
# 12 yazmac, dogrusal blok + sinirli dongu, puti ile yazdirma; bol/mod yalnizca sifirdan farkli, bellek erisimi araligin icinde.
function r(n) { x = (x * 1664525 + 1013904223) % 4294967296; return int(x / 65536) % n }
function reg() { return "r" r(12) }
function neg_(v) { return sprintf("%d", -v) }
function imm(  kk) { kk = r(6); if (kk == 0) return r(10); if (kk == 1) return neg_(r(10)); if (kk == 2) return r(100000); if (kk == 3) return neg_(r(100000)); if (kk == 4) return 2147483647 - r(3); return -2147483648 + r(3) }
function emit1(  t, d, a, b, q, c) {
  t = r(34); d = reg(); a = reg(); b = reg()
  if (t < 3) print "  add " d ", " a ", " b; else if (t < 5) print "  sub " d ", " a ", " b; else if (t < 7) print "  mul " d ", " a ", " b
  else if (t < 9) print "  and " d ", " a ", " b; else if (t == 9) print "  or " d ", " a ", " b; else if (t == 10) print "  xor " d ", " a ", " b
  else if (t == 11) print "  shl " d ", " a ", " b; else if (t == 12) print "  shr " d ", " a ", " b; else if (t == 13) print "  sar " d ", " a ", " b
  else if (t == 14) print "  eq " d ", " a ", " b; else if (t == 15) print "  ne " d ", " a ", " b; else if (t == 16) print "  lt " d ", " a ", " b; else if (t == 17) print "  le " d ", " a ", " b
  else if (t == 18) print "  neg " d ", " a; else if (t == 19) print "  not " d ", " a
  else if (t == 20) print "  ldi " d ", " imm(); else if (t == 21) print "  mov " d ", " a
  else if (t == 22) print "  addi " d ", " a ", " imm(); else if (t == 23) print "  muli " d ", " a ", " imm(); else if (t == 24) print "  andi " d ", " a ", " imm()
  else if (t == 25) print "  shli " d ", " a ", " r(70); else if (t == 26) print "  shri " d ", " a ", " r(70); else if (t == 27) print "  sari " d ", " a ", " r(70)
  else if (t == 28) { q = r(5); if (q == 0) print "  divi " d ", " a ", " (r(99) + 2); else if (q == 1) print "  modi " d ", " a ", " (r(99) + 2); else if (q == 2) print "  divi " d ", " a ", " neg_(r(99) + 2); else if (q == 3) print "  modi " d ", " a ", " neg_(r(99) + 2); else print "  divi " d ", " a ", " (r(5) + 2) }
  else if (t == 29) { c = r(6); print "  " (c == 0 ? "eqi" : c == 1 ? "nei" : c == 2 ? "lti" : c == 3 ? "lei" : c == 4 ? "gti" : "gei") " " d ", " a ", " imm() }
  else if (t == 30) print "  or " d ", " a ", " b
  else if (t == 31 || t == 32) { print "  andi rt, " a ", 63"; print "  addi rt, rt, 1"; print "  st " b ", rt, 0"; print "  ld " d ", rt, 0" }
  else print "  xor " d ", " a ", " b
}
BEGIN { x = seed * 7919 + 13; print "memory 128"; print "func main() {"
  for (ii = 0; ii < 12; ii++) print "  ldi r" ii ", " imm()
  n = 20 + r(60)
  for (kk = 0; kk < n; kk++) {
    if (r(9) == 0) { lab = "L" kk; print "  ldi cnt, " (2 + r(5)); print lab ":"; m = 2 + r(5); for (jj = 0; jj < m; jj++) emit1(); print "  addi cnt, cnt, -1"; print "  jnz cnt, " lab }
    else if (r(8) == 0) { print "  host puti, " reg(); print "  ldi sp, 32"; print "  host putc, sp" }
    else if (r(23) == 0) { d = reg(); a = reg(); b = reg(); print "  div " d ", " a ", " b; if (r(2)) print "  mod " reg() ", " a ", " b }
    else emit1()
  }
  for (ii = 0; ii < 12; ii++) { print "  host puti, r" ii; print "  ldi sp, 10"; print "  host putc, sp" }
  print "  halt"; print "}" }
