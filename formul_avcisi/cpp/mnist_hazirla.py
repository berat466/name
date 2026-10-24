"""MNIST'i C++ icin ikili dosyaya yazar (mnist_olcek.py ile AYNI on isleme: piksel ortalamasi cikar, genel std'ye bol).
Kullanim: python mnist_hazirla.py KLASOR   (varsayilan /tmp/mnist_bin). Girdi: /tmp/mnist_X.npy, /tmp/mnist_y.npy"""
import sys, os
import numpy as np
out = sys.argv[1] if len(sys.argv) > 1 else "/tmp/mnist_bin"
os.makedirs(out, exist_ok=True)
X = np.load("/tmp/mnist_X.npy"); y = np.load("/tmp/mnist_y.npy")
mu = X[:60000].mean(0); sd = X[:60000].std()
X = ((X - mu) / sd).astype(np.float32)
X[:60000].tofile(f"{out}/Xtr.f32"); X[60000:].tofile(f"{out}/Xte.f32")
y[:60000].astype(np.int32).tofile(f"{out}/ytr.i32"); y[60000:].astype(np.int32).tofile(f"{out}/yte.i32")
print("yazildi:", out, X[:60000].shape, X[60000:].shape)
