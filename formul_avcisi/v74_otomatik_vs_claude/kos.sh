#!/bin/bash
# Tek komut: bir calistirma. Kullanim: kos.sh <etiket>   (A ve B AYNI betigi, AYNI ikiliyi, AYNI parametreleri kullanir)
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh
O=/home/user/name/formul_avcisi/v74_otomatik_vs_claude/ham; mkdir -p $O; A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_10001#")
t0=$(date +%s.%N)
env V63_ARROW=2 V64_IMPL=1 V64_SPQ=1 V64_COMPACT=1 V72_UCSC=1 V73_RHS=1 V73_SYRK=1 V65_EVAL=1 V53H_LEAN=1 V53H_FAST=1 V53H_TEST=1 python3 calistir.py $O/$1 /home/user/name/formul_avcisi/cpp/tek_hamle_v74bin $A >/dev/null 2>&1
t1=$(date +%s.%N)
python3 - "$1" "$t0" "$t1" <<'P'
import re,sys,hashlib,json
et,t0,t1=sys.argv[1],float(sys.argv[2]),float(sys.argv[3]); O="/home/user/name/formul_avcisi/v74_otomatik_vs_claude/ham/"; t=open(O+et+".txt").read(); tm=open(O+et+".time").read()
g=lambda p:(re.search(p,t) or [None,None])[1]
w=re.search(r"WALL ([\d.]+) USER ([\d.]+) SYS ([\d.]+) MAXRSS_KB (\d+)",tm)
key=[l for l in t.splitlines() if l.startswith("V65|TEST|") or l.startswith("V65|VAL|") or "V53H_SONUC" in l]
r=dict(etiket=et,program_s=t1-t0,rusage_wall=float(w[1]),cpu_user=float(w[2]),cpu_sys=float(w[3]),cpu_oran=(float(w[2])+float(w[3]))/float(w[1]),rss_gb=int(w[4])/1e6,hazir_s=float(g(r"MODEL_HAZIR\|([\d.]+) s")),test_ppl=float(g(r"V65\|TEST\|n=\d+\|CE=[\d.]+\|PPL=([\d.]+)")),val_ce=float(g(r"V53H_SONUC\|VAL CE=([\d.]+)")),sonuc_hash=hashlib.sha256("\n".join(l.split("|bin")[0] for l in key).encode()).hexdigest()[:16],t0=t0,t1=t1)
json.dump(r,open(O+et+".json","w")); print(json.dumps(r))
P
