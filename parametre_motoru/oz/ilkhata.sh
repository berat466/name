#!/bin/bash
# ILK HATADA DUR: bir dogrulama komutunu calistirir, ciktisina ilk "HATA" satiri dustugunde komutu (ve alt islemlerini) sonlandirir.
# Mutasyon testinde mutant ilk basarisiz kontrolde yakalanmis sayilir; kalan kontrolleri kosmak yalnizca zaman harcar. Mutant KACARSA komut sonuna kadar kosar (tam kapsam korunur).
# Kullanim: ilkhata.sh CIKTI_DOSYASI KOMUT [ARG...]   -> cikis 0: komut gecti ; 1: HATA satiri goruldu ya da komut basarisiz
log=$1; shift
: > "$log"
setsid "$@" > "$log" 2>&1 < /dev/null &
pid=$!
while kill -0 $pid 2>/dev/null; do
  if grep -aq '^HATA' "$log" 2>/dev/null; then
    kill -TERM -- -$pid 2>/dev/null; sleep 0.3; kill -KILL -- -$pid 2>/dev/null; wait $pid 2>/dev/null; echo "(ilk hatada durduruldu)" >> "$log"; exit 1
  fi
  sleep 0.4
done
wait $pid; rc=$?
exit $rc
