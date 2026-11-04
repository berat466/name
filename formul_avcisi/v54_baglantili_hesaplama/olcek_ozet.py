import glob,re,statistics as st,sys
d=sys.argv[1] if len(sys.argv)>1 else 'ham/olcek2'; cfg=sys.argv[2] if len(sys.argv)>2 else 'full'
names=sorted({re.match(rf'{d}/{cfg}_(.+)_\d\.txt',f).group(1) for f in glob.glob(f'{d}/{cfg}_*_[0-9].txt')}, key=lambda x:(not x.startswith('T'),len(x),x))
base=None; print('%-12s %8s %8s %7s %7s %9s %6s %6s %8s %8s'%('varyant','modelhazir','duvar','user','sys','CPU/duvar','VCSW','ICSW','MINFLT','RSS_MB'))
for n in names:
    rows=[]
    for f in sorted(glob.glob(f'{d}/{cfg}_{n}_[0-9].txt')):
        t=open(f).read(); tm=open(f[:-4]+'.time').read(); m=re.search(r'V53H_MODEL_HAZIR\|([\d.]+)',t)
        if not m: continue
        x=dict(re.findall(r'(\w+) ([\d.]+)',tm)); rows.append((float(m.group(1)),float(x['WALL']),float(x['USER']),float(x['SYS']),int(x['VCSW']),int(x['ICSW']),int(x['MINFLT']),int(x['MAXRSS_KB'])/1024))
    if not rows: continue
    med=[st.median(r[i] for r in rows) for i in range(8)]
    if n=='T1': base=med[0]
    sp=f' x{base/med[0]:.2f}' if base and n.startswith('T') else ''
    print('%-12s %8.2f %8.2f %7.2f %7.2f %9.2f %6d %6d %8d %8.0f%s'%(n,med[0],med[1],med[2],med[3],(med[2]+med[3])/med[1],med[4],med[5],med[6],med[7],sp))
