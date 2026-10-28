# sablon.txt -> "data TPLS = ..." (baslangic dizinleri, -1: yok) ve "data TPL = ..." (simge akisi, -1 ile biter). Yalnizca bicim donusumu.
# kullanim: awk -f sablon.awk ops.h sablon.txt
BEGIN {
  n = split("R_A R_B R_C I_A I_B I_C K_B MEMW STKEND NREGS8 NP8 CNT MSG_DIV0 MSG_OOB MSG_STACK MSG_DEPTH MSG_ALLOC RSBASE MBASE HEAP0 DATAIMG ND HDR_ENTRY FILESZ MEMSZ FUEL0 MSG_FUEL LIMRT RTW RTNEG STK4 R_B8 R_B16 RTIMG RTNW", S, " ")
  for (i = 1; i <= n; i++) code[S[i]] = 255 + i
  n = split("REL_DIV0 REL_OOB REL_STACK REL_DEPTH REL_ALLOC REL_PUTI REL_PUTC REL_TA REL_TB REL_FA REL_EXIT REL_ENTRY REL_FUEL REL_EXITC REL_RTEND", S, " ")
  for (i = 1; i <= n; i++) code[S[i]] = 299 + i
  code["IMM8C"] = 320
  nops = 0; pos = 0
}
FILENAME ~ /ops\.h/ { if ($0 ~ /^  X\(/) { s = $0; sub(/^  X\(/, "", s); sub(/,.*/, "", s); idx[tolower(s)] = nops; nops++ } ; next }
FNR == 1 && FILENAME !~ /ops\.h/ {
  idx["host_putc"] = 55; idx["host_puti"] = 56; idx["host_putf"] = 57; idx["host_getc"] = 58; idx["host_present"] = 59; idx["host_poll"] = 60; idx["host_frame"] = 61
  idx["prologue"] = 62; idx["exit0"] = 63; idx["trap_div0"] = 64; idx["trap_oob"] = 65; idx["trap_stack"] = 66; idx["trap_depth"] = 67; idx["trap_alloc"] = 68; idx["header"] = 69; idx["jmp_back"] = 70; idx["jz_back"] = 71; idx["jnz_back"] = 72; idx["trap_fuel"] = 73; idx["ldp"] = 74; idx["stp"] = 75; idx["call_p"] = 76; idx["host_sysw"] = 77; idx["exitc"] = 78; idx["exitc_rt"] = 79; idx["prologue_rt"] = 80
  for (i = 0; i < 81; i++) start[i] = -1
}
FILENAME !~ /ops\.h/ {
  sub(/#.*/, ""); if ($0 ~ /^[ \t]*$/) next
  name = $1; sub(/:$/, "", name); if (!(name in idx)) { print "bilinmeyen ad: " name > "/dev/stderr"; exit 1 }
  start[idx[name]] = pos
  for (k = 2; k <= NF; k++) {
    t = $k
    if (t == "FUELCHK") { tpl = tpl " 77 133 246 15 132 312 73 255 206"; pos += 9 }
    else if (t ~ /^(LDA|LDC|STA)\(.\)$/) {
      x = "R_" toupper(substr(t, 5, 1)); pre = (substr(t, 1, 3) == "LDA") ? "72 139 131" : (substr(t, 1, 3) == "LDC") ? "72 139 139" : "72 137 131"
      tpl = tpl " " pre " " code[x]; pos += 4
    } else if (t ~ /^[0-9A-F][0-9A-F]$/) { tpl = tpl " " hex(t); pos++ }
    else if (t in code) { tpl = tpl " " code[t]; pos++ }
    else { print "bilinmeyen simge: " t " (" name ")" > "/dev/stderr"; exit 1 }
  }
  tpl = tpl " -1"; pos++
}
END { s = ""; for (i = 0; i < 81; i++) s = s " " start[i]; print "data TPLS =" s; print "data TPL =" tpl }
function hex(s,   i, v, c) { v = 0; for (i = 1; i <= length(s); i++) { c = index("0123456789ABCDEF", substr(s, i, 1)) - 1; v = v * 16 + c } return v }
