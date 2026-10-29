#!/usr/bin/env python3
# check_ref.py <log> <expected.json> : referans degerlerle karsilastirma. Cikis kodu: 0=KATI esit, 1=SAYISAL-YAKIN (platform farki), 2=HATA
import re, sys, json
log = open(sys.argv[1], errors="ignore").read(); ref = json.load(open(sys.argv[2]))
def g(p, t=log):
    m = re.search(p, t); return m.group(1) if m else None
res = {}
res["val_CE"] = g(r"V53H_SONUC\|VAL CE=([0-9.]+)"); res["test_CE"] = g(r"V53H_SONUC\|TEST CE=([0-9.]+)"); res["test_PPL"] = g(r"V53H_SONUC\|TEST CE=[0-9.]+ PPL=([0-9.]+)")
res["test_top1"] = g(r"V53H_SONUC\|TEST CE=[0-9.]+ PPL=[0-9.]+ Top1=([0-9.]+)"); res["test_top5"] = g(r"V53H_SONUC\|TEST CE=[0-9.]+ PPL=[0-9.]+ Top1=[0-9.]+ Top5=([0-9.]+)")
res["alpha_Q"] = g(r"alpha_Q=([0-9.]+)"); res["okuma_alpha"] = g(r"okuma alpha=([0-9.]+)"); res["eps"] = g(r"eps=([0-9.e+-]+)")
res["W_F_hash"] = g(r"V78_WFHASH\|([0-9a-f]{16})"); res["skK_hashleri"] = ",".join(re.findall(r"V80_SHASH\|tablo \d\|([0-9a-f]{16})", log)); res["graf_hashleri"] = ",".join(re.findall(r"V80_GHASH\|tablo \d\|(.+)", log))
res["n_egitim"] = g(r"n_egitim=(\d+)"); res["n_val"] = g(r"n_val=(\d+)"); res["n_test"] = g(r"n_test=(\d+)")
res["model_hazir_s"] = g(r"MODEL_HAZIR\|([0-9.]+)"); res["tanh_selftest"] = "BASARISIZ(libm tanhf kullanildi)" if "V54_TANH|oz-denetim BASARISIZ" in log else "gecti(bit-ozdes vektor tanhf)"
exact_keys = ["val_CE", "test_CE", "test_PPL", "test_top1", "test_top5", "alpha_Q", "okuma_alpha", "eps", "n_egitim", "n_val", "n_test"]
hash_keys = ["W_F_hash", "skK_hashleri", "graf_hashleri"]
bad = [k for k in exact_keys if res.get(k) != ref["beklenen"][k]]; hbad = [k for k in hash_keys if res.get(k) != ref["beklenen"][k]]
print(json.dumps(res, indent=1, ensure_ascii=False))
if res["test_PPL"] is None: print("DURUM: HATA -- program bitmedi / cikti okunamadi"); sys.exit(2)
if not bad and not hbad: print("DURUM: KATI ESIT -- CE/PPL/Top-k/secimler ve agirlik/graf karmalari referansla BIREBIR ayni"); sys.exit(0)
d_ppl = abs(float(res["test_PPL"]) - float(ref["beklenen"]["test_PPL"])) / float(ref["beklenen"]["test_PPL"]); d_ce = abs(float(res["val_CE"]) - float(ref["beklenen"]["val_CE"]))
print(f"FARKLAR: yazdirilan-degerler={bad} karmalar={hbad} ; test PPL bagil fark={d_ppl:.2e}, val CE mutlak fark={d_ce:.2e}")
if d_ppl < 2e-3 and d_ce < 5e-3: print("DURUM: SAYISAL-YAKIN -- karmalar farkli (platform libm/BLAS toplama sirasi); PPL/CE tolerans icinde. Bu BEKLENEN olabilir (Windows)."); sys.exit(1)
print("DURUM: HATA -- sonuc referanstan anlamli sapmis"); sys.exit(2)
