"""KESIF#1'in neresi ise yariyor? Ogrenme orani izgarasi YUKARI genisletilmis, 200 gorunmemis gorev."""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s
from optim_search import BASELINES, V, C, U, B

s.LRS = [0.003, 0.01, 0.03, 0.1, 0.3, 1.0, 3.0, 10.0]
adam = BASELINES["Adam"]
den = B("+", U("sqrt", V("v")), V("v"))
forms = {
  "Adam": adam,
  "tanh(Adam)  [sadece sinirlama]": U("tanh", adam),
  "m/(sqrt(v)+v) [sadece ek payda]": B("/", V("m"), den),
  "tanh(m/(sqrt(v)+v)) [KESIF#1]": U("tanh", B("/", V("m"), den)),
  "AdamW": BASELINES["AdamW"],
}
if __name__ == "__main__":
    seeds = [10**7 + i for i in range(200)]
    with Pool() as p:
        res = p.map(s.suite_scores, [(e, seeds) for e in forms.values()])
    a = res[0]
    print("200 gorunmemis gorev, ogrenme orani izgarasi 0.003..10 (tek ortak lr)\n")
    for (n, _), (g, lr, rs) in zip(forms.items(), res):
        print(f"{n:34} lr={lr:<5} geo.ort={g:.4f}  Adam'a gore {(1-g/a[0])*100:+6.1f}%  kazanma %{np.mean(rs < a[2]-1e-9)*100:3.0f}  en kotu={rs.max():.2f}")
