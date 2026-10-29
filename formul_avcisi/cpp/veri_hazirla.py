"""El yazisi rakamlari (sklearn, internet gerektirmez) train/test CSV olarak yazar. Kullanim: python veri_hazirla.py CIKTI_KLASORU [tohum]"""
import sys, numpy as np
from sklearn.datasets import load_digits
out = sys.argv[1]; seed = int(sys.argv[2]) if len(sys.argv) > 2 else 0
d = load_digits(); X = d.data / 16.0; y = d.target
idx = np.random.default_rng(seed).permutation(len(y)); nte = int(0.3 * len(y))
for name, ids in (("train", idx[nte:]), ("test", idx[:nte])):
    np.savetxt(f"{out}/digits_{name}.csv", np.hstack([X[ids], y[ids, None]]), delimiter=",", fmt="%.6g")
print("yazildi:", out)
