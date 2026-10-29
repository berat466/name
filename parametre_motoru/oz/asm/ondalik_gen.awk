# Test uretecinin parcasi: rastgele ve sinir ondalik sabitleri iceren .pma uretir (assembler = C++ strtod karsilastirmasi icin). Kullanim: awk -v seed=N [-v n=80] -f ondalik_gen.awk
function rd(n) { return int(rand() * n) }
function digits(k,   s, i) { s = ""; for (i = 0; i < k; i++) s = s rd(10); return s }
function zeros(k,   s, i) { s = ""; for (i = 0; i < k; i++) s = s "0"; return s }
function lit(   m, d, p, e, s, r, t) {
  r = rd(10)
  if (r == 0) return B[1 + rd(NB)]
  if (r == 2 && rd(3) == 0) {   # 800+ tam sayi basamagi (kirpilan basamaklar e10'u etkiler) ve yapisan kuyruk
    L = 801 + rd(30); s = (1 + rd(9)) digits(L - 1); return s "e-" (L - 1 - rd(25))
  }
  if (r == 3 && rd(3) == 0) {   # yarim-esitlik + cok uzak sifir-olmayan kuyruk
    t = rd(4)
    if (t == 0) return "9007199254740993." zeros(790 + rd(30)) "1"
    if (t == 1) return "9007199254740993." zeros(790 + rd(30))
    if (t == 2) return "2.4703282292062327208" zeros(780 + rd(30)) "1e-324"
    return "1.00000000000000011102230246251565404236316680908203125" zeros(740 + rd(20)) "1"
  }
  if (r == 1) { s = digits(790 + rd(40)); p = 1 + rd(length(s)); m = substr(s, 1, p) "." substr(s, p + 1); return (rd(2) ? "-" : "") m (rd(2) ? "e" (rd(20) - 10) : "") }
  d = 1 + rd(rd(3) == 0 ? 30 : 17)
  m = digits(d)
  if (rd(4) == 0) m = zeros(rd(4)) m
  p = rd(d + 2)
  if (p <= length(m)) m = substr(m, 1, p) "." substr(m, p + 1)
  else m = m "."
  if (m == ".") m = ".5"
  s = rd(3) == 0 ? "-" : (rd(8) == 0 ? "+" : "")
  e = ""
  t = rd(6)
  if (t == 0) e = "e" (rd(21) - 10)
  else if (t == 1) e = "E" (rd(2) ? "+" : "-") rd(330)
  else if (t == 2) e = "e" (rd(700) - 350)
  else if (t == 3) e = "e-" (300 + rd(30))
  else if (t == 4) e = "e" (290 + rd(30))
  return s m e
}
BEGIN {
  srand(seed + 0); if (n == 0) n = 80
  NB = split("2.2250738585072011e-308 2.2250738585072014e-308 4.9406564584124654e-324 2.4703282292062327e-324 2.4703282292062328e-324 1.7976931348623157e308 1.7976931348623158e308 1.7976931348623159e308 9007199254740993.0 9007199254740992.0 9007199254740993.00000000000000000001 0.1 0.3 1e23 8.41e21 5e-324 1e-400 1e400 0.000000000000000000000000000000000000001 123456789012345678901234567890.123456789012345678901234567890e-5 00000.5 -0.0 .5 5. 1.e3 +1.5E+3 1e0 0.0 -.0e5 1.5e-323 2.5e-324 7.4109846876186982e-324 7.4109846876186981e-324 1e308 1.0e-308 9.999999999999999e22 4.35e-322 1.5e300 18014398509481985.0 1.00000000000000011102230246251565404236316680908203125 1.00000000000000011102230246251565404236316680908203126 1.00000000000000011102230246251565404236316680908203124 9007199254740991.5 9007199254740990.5 0.99999999999999994449 0.99999999999999994 0.999999999999999944488848768742172978818416595458984375 1.7976931348623158e308 1.797693134862315807e308 2.2250738585072009e-308 2.225073858507201e-308 4.9406564584124654417656879286822137236505980e-324 2.4703282292062327208e-324 9007199254740995.0 18014398509481986.0 18014398509481987.0", B, " ")
  printf "data d ="; for (i = 0; i < n; i++) printf " %s", lit(); printf "\nfunc main() {\n"
  for (i = 0; i < n; i++) printf " fadd r%d, %s, %s\n", i, lit(), lit()
  printf " host putf, r0\n halt\n}\nentry main\n"
}
