import re,math,sys
def ce(p):
    r={}
    for l in open(p):
        m=re.match(r'V76_DBG\|alfa=(\S+?)\|CE=([^|]+)\|',l)
        if m: r['alfa'+m.group(1)]=float(m.group(2))
        m=re.match(r'V76_DBG\|test CE=([^|]+)\|',l)
        if m: r['test']=float(m.group(1))
        m=re.match(r'V76_DBG\|lin e=(\S+) a=(\S+)\|(\S+)',l)
        if m: r['lin'+m.group(1)+'_'+m.group(2)]=float(m.group(3))
    return r
a=ce(sys.argv[1]); b=ce(sys.argv[2])
for grp in ('alfa','test','lin'):
    ks=[k for k in a if k.startswith(grp)]
    print(grp,len(ks),'deger: maks mutlak %.3e'%max(abs(a[k]-b[k]) for k in ks),'maks bagil %.3e'%max(abs(a[k]-b[k])/abs(a[k]) for k in ks),'maks ULP %.1f'%max(abs(a[k]-b[k])/math.ulp(a[k]) for k in ks))
