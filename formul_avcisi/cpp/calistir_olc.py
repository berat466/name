#!/usr/bin/env python3
"""Bir komutu calistirir; ham stdout/stderr'i dosyaya yazar; duvar suresi ve tepe RAM (cocuk surecin RUSAGE maxrss, KB) olcer.
Kullanim: calistir_olc.py CIKTI_DOSYASI MANIFEST_TSV -- komut arg...   (her cagri ayri surec -> maxrss yalniz bu cocuga ait)"""
import sys, subprocess, time, resource, shlex
out, manifest = sys.argv[1], sys.argv[2]
cmd = sys.argv[sys.argv.index("--") + 1:]
t0 = time.time()
p = subprocess.run(cmd, capture_output=True, text=True)
dt = time.time() - t0
ru = resource.getrusage(resource.RUSAGE_CHILDREN)
open(out, "w").write(p.stdout)
if p.stderr: open(out + ".stderr", "w").write(p.stderr)
open(manifest, "a").write(f"{shlex.join(cmd)}\t{out}\trc={p.returncode}\twall_s={dt:.2f}\tuser_cpu_s={ru.ru_utime:.2f}\tsys_cpu_s={ru.ru_stime:.2f}\tmaxrss_MB={ru.ru_maxrss/1024:.0f}\n")
