import os,glob,re
from t import info
def g(f,k='B'):
    d,a0,kd,hz,rs=info(f); return d.get(k),a0.get(k)
print('### Bagimsiz kapi izgarasi (rho=0, temiz havuz tabani; hucre: dogr CE (alpha) / test PPL; alpha dogrulamayla)')
gs=['0','0.05','0.15','0.30']
print('| g_F \\ g_P | '+' | '.join(gs)+' |'); print('|---'*5+'|')
best=None
for gf in gs:
    row=[]
    for gp in gs:
        f='L_oof1_rk0.txt' if (gf=='0' and gp=='0') else f'G_gF{gf}_gP{gp}.txt'
        if not os.path.exists(f): row.append('YOK'); continue
        try: x,a0=g(f)
        except Exception: row.append('YOK'); continue
        if x is None: row.append('YOK'); continue
        row.append('%s (%s) / %s'%(x['val'],x['al'],x['test']))
        v=float(x['val'])
        if best is None or v<best[0]: best=(v,gf,gp,x['test'],x['al'])
    print('| %s | '%gf+' | '.join(row)+' |')
print('\nGrid en iyi dogrulama CE:',best)
print('\n### Eslem tohumlari (E, g=0.15, ridge-K; C-satiri)')
for ms in (1,2,3):
    f=f'M_E_mseed{ms}.txt'
    if os.path.exists(f):
        try:
            x,a0=g(f,'C'); print('eslem tohumu',ms,': alpha=%s dogr CE=%s test PPL=%s Top1=%s Top5=%s | alpha=0 dogr CE=%s'%(x['al'],x['val'],x['test'],x['t1'],x['t5'],a0))
        except Exception as e: print('eslem tohumu',ms,'YOK/hata')
    else: print('eslem tohumu',ms,'YOK')
print('\n### Kapilar karistirilmis, final gercek (SG_*) vs gercek (R_*) vs tam karisik (S_*)')
for lab,fr,fsg,fs,key in [('B gF=0.15','R_B_gF0.15.txt','SG_B_gF0.15.txt','S_B_gF0.15.txt','B'),('C gP=0.05','R_C_gP0.05.txt','SG_C_gP0.05.txt','S_C_gP0.05.txt','B'),('D g=0.05/0.05','R_D_g0.05.txt','SG_D_g0.05.txt','S_D_g0.05.txt','B'),('E g=0.15 (C-satiri)','R_E_g0.15.txt','SG_E_g0.15.txt','S_E_g0.15.txt','C')]:
    out=[]
    for f in (fr,fsg,fs):
        if os.path.exists(f):
            try: x,a0=g(f,key); out.append('a=%s val=%s test=%s a=0val=%s sabit0.2=%s'%(x['al'],x['val'],x['test'],a0,x.get('sab')))
            except Exception: out.append('hata')
        else: out.append('YOK')
    print('-',lab,'\n    gercek      :',out[0],'\n    kapi-karisik:',out[1],'\n    tam-karisik :',out[2])
