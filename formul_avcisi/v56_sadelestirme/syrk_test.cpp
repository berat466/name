// V56 mikro test: simetrik Gram tipi carpimlar (F^T F parcasi ve K-tablosu grafi S=Nn Nn^T): sgemm vs ssyrk(ust ucgen)+yansitma, bit karsilastirma ve sure
#include <bits/stdc++.h>
#include <omp.h>
extern "C" { void scipy_cblas_sgemm(int,int,int,int,int,int,float,const float*,int,const float*,int,float,float*,int); void scipy_cblas_ssyrk(int,int,int,int,int,float,const float*,int,float,float*,int); }
static double now(){ return omp_get_wtime(); }
int main(){ const int V=2001, m=513; 
 // --- (1) F^T F: gercek F, 4096 satirlik parca (V54 'cross' parcasi)
 size_t n=113046; std::vector<float> F(n*m); { FILE* f=fopen("/tmp/f56/Fc.f32","rb"); fread(F.data(),4,n*m,f); fclose(f);} 
 const int CH=4096; std::vector<float> G1((size_t)m*m), G2((size_t)m*m);
 long diff_up=0, asym=0; double tg=0, ts=0; int nch=0;
 for(size_t s0=0; s0+CH<=n && nch<8; s0+=CH, nch++){ const float* A=&F[s0*m];
   double t=now(); scipy_cblas_sgemm(101,112,111,m,m,CH,1.f,A,m,A,m,0.f,G1.data(),m); tg+=now()-t;
   t=now(); std::fill(G2.begin(),G2.end(),0.f); scipy_cblas_ssyrk(101,121,112,m,CH,1.f,A,m,0.f,G2.data(),m); ts+=now()-t;   // ust ucgen (121=Upper), Trans
   for(int i=0;i<m;i++) for(int j=i;j<m;j++){ diff_up += G1[(size_t)i*m+j]!=G2[(size_t)i*m+j]; asym += G1[(size_t)i*m+j]!=G1[(size_t)j*m+i]; } }
 printf("F^T F parca (x%d): sgemm %.2f ms, ssyrk %.2f ms ; ust ucgende sgemm!=ssyrk eleman=%ld ; sgemm sonucunun kendi asimetrisi=%ld (toplam ust ucgen eleman=%ld)\n",nch,tg/nch*1e3,ts/nch*1e3,diff_up,asym,(long)nch*m*(m+1)/2);
 // --- (2) K graf S: gercek sayimlardan Nn
 std::vector<int> ctx(2*113046), y(113046); { FILE* f=fopen("/tmp/v53A_yeni/c2_train_ctx.i32","rb"); fread(ctx.data(),4,ctx.size(),f); fclose(f); f=fopen("/tmp/f56/ytr.i32","rb"); fread(y.data(),4,y.size(),f); fclose(f);} 
 std::vector<float> Cn((size_t)V*V,0.f); for(size_t i=0;i<113046;i++) Cn[(size_t)ctx[2*i+1]*V+y[i]]+=1.f;
 std::vector<float> Nn((size_t)V*V); for(int u=0;u<V;u++){ double nn=0; for(int v=0;v<V;v++){ float x=sqrtf(Cn[(size_t)u*V+v]); Nn[(size_t)u*V+v]=x; nn+=(double)x*x;} nn=sqrt(nn)+1e-9; for(int v=0;v<V;v++) Nn[(size_t)u*V+v]/=(float)nn; }
 std::vector<float> S1((size_t)V*V), S2((size_t)V*V); double t=now(); scipy_cblas_sgemm(101,111,112,V,V,V,1.f,Nn.data(),V,Nn.data(),V,0.f,S1.data(),V); double tgm=now()-t;
 t=now(); scipy_cblas_ssyrk(101,121,111,V,V,1.f,Nn.data(),V,0.f,S2.data(),V); double tsy=now()-t; for(int rep=0;rep<3;rep++){ t=now(); scipy_cblas_sgemm(101,111,112,V,V,V,1.f,Nn.data(),V,Nn.data(),V,0.f,S1.data(),V); tgm=std::min(tgm,now()-t); t=now(); scipy_cblas_ssyrk(101,121,111,V,V,1.f,Nn.data(),V,0.f,S2.data(),V); tsy=std::min(tsy,now()-t);} 
 long d2=0, a2=0; for(int i=0;i<V;i++) for(int j=i;j<V;j++){ d2+=S1[(size_t)i*V+j]!=S2[(size_t)i*V+j]; a2+=S1[(size_t)i*V+j]!=S1[(size_t)j*V+i]; }
 printf("K graf S=NnNn^T (V=2001): sgemm %.2f ms, ssyrk %.2f ms ; ust ucgende sgemm!=ssyrk=%ld / %ld ; sgemm asimetri=%ld\n",tgm*1e3,tsy*1e3,d2,(long)V*(V+1)/2,a2);
}
