import subprocess, re, sys, os, itertools, json
data = sys.argv[1]; src = os.path.expanduser("/home/user/name/formul_avcisi/v54_baglantili_hesaplama")
env = dict(os.environ); 
for l in open(f"{src}/env_hafif.sh"):
    if l.startswith("source"): 
        for l2 in open(os.path.join(src, l.split()[1].replace("../","../"))):
            pass
out = subprocess.run(f"cd {src}; source env_hafif.sh >/dev/null; env", shell=True, capture_output=True, text=True, executable="/bin/bash").stdout
for l in out.splitlines():
    if "=" in l: k, v = l.split("=", 1); env[k] = v
env.update(V53H_LEAN="1", V53H_FAST="1", V53H_TEST="1")
res = []
for g, lf in itertools.product([0.05, 0.1, 0.2, 0.5, 1.0], [0.3, 1, 3, 10, 30, 100, 1000, 9000]):
    a = f"2 512 1 {g} 1 0.5 Z 1 Y {data} 85 85 {lf}".split()
    t = subprocess.run([f"{src}/../cpp/tek_hamle_v61"] + a, capture_output=True, text=True, env=env, timeout=60).stdout
    v = re.search(r"VAL CE=([\d.]+) PPL=[\d.]+ Top1=([\d.]+)", t); te = re.search(r"TEST CE=([\d.]+) PPL=[\d.]+ Top1=([\d.]+)", t); th = re.search(r"MODEL_HAZIR\|([\d.]+) s", t)
    if v and te: res.append((float(v.group(2)), -float(v.group(1)), g, lf, float(te.group(2)), float(te.group(1)), float(th.group(1))))
res.sort(reverse=True); print("en iyi 5 (val Top1'e gore secim):")
for r in res[:5]: print(f"  gamma={r[2]} lamF={r[3]} VAL acc={r[0]:.4f} CE={-r[1]:.4f} | TEST acc={r[4]:.4f} CE={r[5]:.4f} | hazir={r[6]:.3f}s")
json.dump(res, open(f"grid_{os.path.basename(data)}.json", "w"))
