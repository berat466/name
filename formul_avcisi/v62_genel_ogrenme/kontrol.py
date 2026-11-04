import subprocess, re, os, json, numpy as np
src = "/home/user/name/formul_avcisi/v54_baglantili_hesaplama"; env = dict(os.environ)
out = subprocess.run(f"cd {src}; source env_hafif.sh >/dev/null; env", shell=True, capture_output=True, text=True, executable="/bin/bash").stdout
for l in out.splitlines():
    if "=" in l: k, v = l.split("=", 1); env[k] = v
env.update(V53H_LEAN="1", V53H_FAST="1", V53H_TEST="1"); cfg = {"digits": (0.1, 0.3), "bc": (0.2, 0.3), "wine": (0.1, 3)}; R = {}
for name, (g, lf) in cfg.items():
    for tag, d in (("gercek", f"/tmp/v62_{name}"), ("etiket_karisik", f"/tmp/v62_{name}_shuf")):
        ts = []
        for rep in range(3):
            t = subprocess.run([f"{src}/../cpp/tek_hamle_v61"] + f"2 512 1 {g} 1 0.5 Z 1 Y {d} 85 85 {lf}".split(), capture_output=True, text=True, env=env).stdout
            v = re.search(r"VAL CE=([\d.]+) PPL=[\d.]+ Top1=([\d.]+)", t); te = re.search(r"TEST CE=([\d.]+) PPL=[\d.]+ Top1=([\d.]+)", t); th = float(re.search(r"MODEL_HAZIR\|([\d.]+) s", t).group(1)); ts.append(th)
        R[f"{name}_{tag}"] = dict(val_acc=float(v.group(2)), test_acc=float(te.group(2)), test_ce=float(te.group(1)), hazir_s_medyan=float(np.median(ts)), hazir_s_tum=ts)
        print(name, tag, R[f"{name}_{tag}"])
    y = np.fromfile(f"/tmp/v62_{name}/c2_test_y.i32", dtype=np.int32); ytr = np.fromfile(f"/tmp/v62_{name}/c2_train_y.i32", dtype=np.int32); R[f"{name}_cogunluk_siniflama"] = float((y == np.bincount(ytr).argmax()).mean()); print(name, "cogunluk", R[f"{name}_cogunluk_siniflama"])
json.dump(R, open("tekhamle_kontrol.json", "w"), indent=1)
