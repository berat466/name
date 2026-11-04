#!/bin/bash
# Uygulama denetimi:
#  (1) yorumlayici = makine kodu (uc kipte; cikti, kare zinciri, son kare, YAKIT) ve .pmb = kaynak
#  (2) bagimsiz awk dogrulamalari (Yasam Oyunu, sahne baytlari, Pong, parcacik baytlari, tugla, kovalamaca, veri isleme)
#  (3) SABLON KATMANI esdegerligi: pong.pmt kare zinciri = elle yazilmis pong.pma; parcacik.pmt = parcacik_elle.pma
PM=${1:-./pm}
cd "$(dirname "$0")"; PM=$(cd .. && realpath "$PM")
T=$(mktemp -d); pass=0; fail=0; failed=""
ok() { pass=$((pass+1)); }; bad() { fail=$((fail+1)); failed="$failed\n  $1"; }
eklines() { grep -E '^(ekran|yakit):' "$1" | tr '\n' ' '; }
check_app() {   # etiket dosya [arguman...]
  local label=$1 file=$2; shift 2
  "$PM" run $file "$@" --kare-rapor --stats > $T/oi 2> $T/ei; local ri=$? ref; ref=$(eklines $T/ei)
  [ $ri = 0 ] && ok || bad "$label: yorumlayici cikis kodu $ri"
  for prom in "" "--native-promote 0" "--native-promote 100000"; do
    "$PM" run $file "$@" --native-strict $prom --kare-rapor --stats > $T/on 2> $T/en; local rn=$? got; got=$(eklines $T/en)
    [ $rn = 0 ] && cmp -s $T/oi $T/on && [ "$ref" = "$got" ] && ok || bad "$label: makine kodu farkli ($prom): rc $rn | '$got' <> '$ref'"
  done
  if [ "${file##*.}" = pma ]; then "$PM" asm $file -o $T/x.pmb --strip > /dev/null; else "$PM" sablon $file -o $T/x.pmb --strip > /dev/null; fi
  "$PM" run $T/x.pmb "$@" --kare-rapor --stats > $T/ob 2> $T/eb
  [ "$(eklines $T/eb)" = "$ref" ] && cmp -s $T/oi $T/ob && ok || bad "$label: .pmb farkli"
}
chain() { "$PM" run $1 "${@:2}" --kare-rapor 2>&1 >/dev/null | grep -E '^ekran:'; }
check_app hayat hayat.pma
check_app sahne sahne.pma
check_app pong pong.pma --girdi pong.girdi
check_app pong.pmt sablon/pong.pmt --girdi pong.girdi
check_app parcacik.pmt sablon/parcacik.pmt
check_app parcacik_elle sablon/parcacik_elle.pma
check_app tugla.pmt sablon/tugla.pmt --girdi tugla.girdi
check_app kovala.pmt sablon/kovala.pmt --girdi kovala.girdi
check_app veri.pmt sablon/veri.pmt
# --- bagimsiz dogrulamalar
"$PM" run hayat.pma --ascii 4 2>/dev/null > $T/a1; awk -f bagimsiz/hayat.awk > $T/a2; cmp -s $T/a1 $T/a2 && ok || bad "hayat: awk farki"
"$PM" run sahne.pma --ekran $T/s.ppm > /dev/null 2>&1; tail -c +16 $T/s.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > $T/s1; awk -f bagimsiz/sahne.awk > $T/s2
[ "$(head -c 15 $T/s.ppm | tr '\n' ' ')" = "P6 160 120 255 " ] && cmp -s $T/s1 $T/s2 && ok || bad "sahne: awk bayt farki"
"$PM" run pong.pma --girdi pong.girdi 2>/dev/null > $T/p1; awk -f bagimsiz/pong.awk pong.girdi > $T/p2; cmp -s $T/p1 $T/p2 && ok || bad "pong: awk farki"
"$PM" run sablon/pong.pmt --girdi pong.girdi 2>/dev/null > $T/p3; cmp -s $T/p3 $T/p2 && ok || bad "pong.pmt: awk farki"
"$PM" run sablon/parcacik.pmt --ekran $T/pc.ppm > /dev/null 2>&1; tail -c +16 $T/pc.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > $T/pc1; awk -f bagimsiz/parcacik.awk > $T/pc2
[ "$(head -c 15 $T/pc.ppm | tr '\n' ' ')" = "P6 160 120 255 " ] && cmp -s $T/pc1 $T/pc2 && ok || bad "parcacik: awk bayt farki"
"$PM" run sablon/tugla.pmt --girdi tugla.girdi 2>/dev/null > $T/t1; awk -f bagimsiz/tugla.awk tugla.girdi > $T/t2; cmp -s $T/t1 $T/t2 && ok || bad "tugla: awk farki"
"$PM" run sablon/kovala.pmt --girdi kovala.girdi 2>/dev/null > $T/k1; awk -f bagimsiz/kovala.awk kovala.girdi > $T/k2; cmp -s $T/k1 $T/k2 && ok || bad "kovala: awk farki"
"$PM" run sablon/veri.pmt 2>/dev/null > $T/v1
awk 'BEGIN{ for(i=0;i<2000;i++){ h=(i*1664525 + 11*1013904223 + 12345)%16777216; h=(h*1664525+1013904223)%16777216; f=5+int(h/16)%496; h=(i*1664525 + 12*1013904223 + 12345)%16777216; h=(h*1664525+1013904223)%16777216; a=1+int(h/16)%20; t=f*a; s+=t; if(t>3000)q++; if(t>mx)mx=t; if(i==0||t<mn)mn=t } printf "%d %d %d %d %d\n", s, q, mx, mn, int(s/2000) }' > $T/v2
cmp -s $T/v1 $T/v2 && ok || bad "veri: awk farki"
# --- sablon esdegerligi: hic uygulamaya ozgu komut yok ama kare zinciri BIRE BIR ayni
[ "$(chain pong.pma --girdi pong.girdi)" = "$(chain sablon/pong.pmt --girdi pong.girdi)" ] && ok || bad "pong: sablon kare zinciri elle yazilmisla farkli"
[ "$(chain sablon/parcacik_elle.pma)" = "$(chain sablon/parcacik.pmt)" ] && ok || bad "parcacik: sablon kare zinciri elle yazilmisla farkli"
# --- hatali tanimlar anlasilir mesajla reddedilir
printf 'kare 1\nsablon a adet 3\n  alan x sabit 1\n  kural y = x + 1\nson\n' > $T/h1.pmt
"$PM" sablon $T/h1.pmt > /dev/null 2> $T/he; grep -q 'hedef bulunamadi' $T/he && ok || bad "hatali tanim: tanimsiz hedef mesaji"
printf 'sablon a adet 3\n  alan x sabit 1\nson\n' > $T/h2.pmt
"$PM" sablon $T/h2.pmt > /dev/null 2> $T/he; grep -q 'kare N' $T/he && ok || bad "hatali tanim: sonlanmayan uygulama"
printf 'kare 1\nsablon a adet 3\n  davran yok x\nson\n' > $T/h3.pmt
"$PM" sablon $T/h3.pmt > /dev/null 2> $T/he; grep -q 'bilinmeyen davranis' $T/he && ok || bad "hatali tanim: bilinmeyen davranis"
# --- gecersiz present argumani cokmez
cat > $T/gecersiz.pma <<'EOS'
space fb 16
func main() {
    host r, present, fb, 300, 300
    host puti, r
    host putc, 10
    host r2, present, 0, 4, 4
    host puti, r2
    host putc, 10
    host r3, poll
    host puti, r3
    host putc, 10
    host r4, frame
    host puti, r4
    host putc, 10
}
EOS
"$PM" run $T/gecersiz.pma > $T/g1 2>&1; printf -- '-1\n-1\n0\n0\n' | cmp -s - $T/g1 && ok || bad "gecersiz present: beklenmeyen cikti"
"$PM" run $T/gecersiz.pma --native-strict > $T/g2 2>&1; cmp -s $T/g1 $T/g2 && ok || bad "gecersiz present: makine kodu farkli"
rm -rf $T
echo "uygulama denetimi: gecen $pass, kalan $fail"
[ $fail -eq 0 ] || { printf "BASARISIZ:$failed\n"; exit 1; }
echo "KATI ESIT: yorumlayici = makine kodu = bagimsiz dogrulama; sablon = elle yazilmis"
