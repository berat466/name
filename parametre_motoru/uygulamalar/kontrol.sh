#!/bin/bash
# Uygulama denetimi: (1) yorumlayici = makine kodu (uc kipte; cikti, kare zinciri, son kare ve YAKIT)
#                    (2) .pmb (parametre dosyasi) = .pma
#                    (3) bagimsiz awk dogrulamasi (Yasam Oyunu, sahne baytlari, Pong son durumu)
PM=${1:-./pm}
cd "$(dirname "$0")"; PM=$(cd .. && realpath "$PM")
T=$(mktemp -d); pass=0; fail=0; failed=""
ok() { pass=$((pass+1)); }; bad() { fail=$((fail+1)); failed="$failed\n  $1"; }
run() { "$PM" run "$@" --kare-rapor --stats 2> $T/err; rc=$?; }   # stdout cagirana, stderr $T/err
frame() { grep -E '^(ekran|yakit):' $T/err | tr '\n' ' '; }
for app in hayat sahne pong; do
  extra=""; [ "$app" = pong ] && extra="--girdi pong.girdi"
  "$PM" run $app.pma $extra --kare-rapor --stats > $T/oi 2> $T/ei; ri=$?; ref=$(grep -E '^(ekran|yakit):' $T/ei | tr '\n' ' ')
  [ $ri = 0 ] && ok || bad "$app: yorumlayici cikis kodu $ri"
  for prom in "" "--native-promote 0" "--native-promote 100000"; do
    "$PM" run $app.pma $extra --native-strict $prom --kare-rapor --stats > $T/on 2> $T/en; rn=$?
    got=$(grep -E '^(ekran|yakit):' $T/en | tr '\n' ' ')
    [ $rn = 0 ] && cmp -s $T/oi $T/on && [ "$ref" = "$got" ] && ok || bad "$app: makine kodu farkli ($prom): rc $rn | '$got' <> '$ref'"
  done
  "$PM" asm $app.pma -o $T/x.pmb --strip > /dev/null && "$PM" run $T/x.pmb $extra --kare-rapor --stats > $T/ob 2> $T/eb
  [ "$(grep -E '^(ekran|yakit):' $T/eb | tr '\n' ' ')" = "$ref" ] && cmp -s $T/oi $T/ob && ok || bad "$app: .pmb farkli"
done
# bagimsiz dogrulamalar
"$PM" run hayat.pma --ascii 4 2>/dev/null > $T/h1; awk -f bagimsiz/hayat.awk > $T/h2
cmp -s $T/h1 $T/h2 && ok || bad "hayat: awk benzetimiyle fark"
"$PM" run sahne.pma --ekran $T/s.ppm > /dev/null 2>&1; tail -c +16 $T/s.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > $T/s1; awk -f bagimsiz/sahne.awk > $T/s2
[ "$(head -c 15 $T/s.ppm | tr '\n' ' ')" = "P6 160 120 255 " ] && cmp -s $T/s1 $T/s2 && ok || bad "sahne: awk ile bayt farki"
"$PM" run pong.pma --girdi pong.girdi 2>/dev/null > $T/p1; awk -f bagimsiz/pong.awk pong.girdi > $T/p2
cmp -s $T/p1 $T/p2 && ok || bad "pong: awk benzetimiyle fark"
# gecersiz present argumani: cokmez, -1 doner
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
echo "KATI ESIT: uygulamalar yorumlayici = makine kodu = bagimsiz dogrulama"
