#!/usr/bin/env python3
# calistir.py <cikti_on_eki> <komut...> : komutu calistirir, stdout -> .txt, rusage (duvar, user, sys, maxrss, baglam degisimi, sayfa hatalari) -> .time
import sys, subprocess, resource, time
pre = sys.argv[1]; cmd = sys.argv[2:]
r0 = resource.getrusage(resource.RUSAGE_CHILDREN); t0 = time.time()
with open(pre + '.txt', 'w') as f: p = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)
t1 = time.time(); r1 = resource.getrusage(resource.RUSAGE_CHILDREN)
open(pre + '.time', 'w').write('WALL %.3f USER %.3f SYS %.3f MAXRSS_KB %d VCSW %d ICSW %d MINFLT %d MAJFLT %d RC %d\n' % (t1 - t0, r1.ru_utime - r0.ru_utime, r1.ru_stime - r0.ru_stime, r1.ru_maxrss, r1.ru_nvcsw - r0.ru_nvcsw, r1.ru_nivcsw - r0.ru_nivcsw, r1.ru_minflt - r0.ru_minflt, r1.ru_majflt - r0.ru_majflt, p.returncode))
