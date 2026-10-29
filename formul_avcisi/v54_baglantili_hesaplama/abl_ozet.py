import glob,re,statistics as st,sys
d=sys.argv[1]; base=None; names=[]
for f in sorted(glob.glob(f'{d}/*_[0-9].txt')):
    n=re.match(rf'{d}/(.+)_\d\.txt',f).group(1)
    if n not in names: names.append(n)
print('%-16s %10s %8s %8s %8s'%('varyant','modelhazir','fark','CPU-s','RSS_MB'))
names=['tam']+[n for n in names if n!='tam']
for n in names:
    rows=[]
    for f in sorted(glob.glob(f'{d}/{n}_[0-9].txt')):
        t=open(f).read(); x=dict(re.findall(r'(\w+) ([\d.]+)',open(f[:-4]+'.time').read())); m=re.search(r'V53H_MODEL_HAZIR\|([\d.]+)',t)
        if m: rows.append((float(m.group(1)),float(x['USER'])+float(x['SYS']),int(x['MAXRSS_KB'])/1024))
    mh=st.median(r[0] for r in rows); 
    if n=='tam': base=mh
    print('%-16s %10.2f %+8.2f %8.1f %8.0f'%(n,mh,mh-(base or mh),st.median(r[1] for r in rows),st.median(r[2] for r in rows)))
