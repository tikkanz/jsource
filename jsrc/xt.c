/* Copyright 1990-2008, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Xenos: time and space                                                   */

#ifdef _WIN32
#include <windows.h>
#include <winbase.h>
#ifndef __MINGW32__
#include <time.h>
#else
#include <sys/time.h>
#endif
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "j.h"
#include "cpuinfo.h"

#if !SY_WINCE && (SY_WIN32 || (SYS & SYS_LINUX))
#include <time.h>
#else
#if SYS & SYS_UNIX
#include <sys/time.h>
#endif
#endif

#if !SY_WIN32 && (SYS & SYS_DOS)
#include <dos.h>
#endif

#ifndef CLOCKS_PER_SEC
#if (SYS & SYS_UNIX)
#define CLOCKS_PER_SEC  1000000
#endif
#ifdef  CLK_TCK
#define CLOCKS_PER_SEC  CLK_TCK
#endif
#endif


F1(jtsp){ASSERTMTV(w); R sc(spbytesinuse());}  //  7!:0

// 7!:1
// Return (current allo),(max since reset)
// If arg is an atom, reset to it
F1(jtsphwmk){
  I curr = jt->malloctotal; I hwmk = jt->malloctotalhwmk;
  if(AN(w)){I new; RE(new=i0(w)); jt->malloctotalhwmk=new;}
  R v2(curr,hwmk);
}

F1(jtspit){A z;I k; 
 F1RANK(1,jtspit,DUMMYSELF); 
 jt->bytesmax=k=spstarttracking();  // start keeping track of bytesmax
 FDEPINC(1); z=exec1(w); FDEPDEC(1);
 spendtracking();  // end tracking, even if there was an error
 RZ(z);
 R sc(jt->bytesmax-k);
}   // 7!:2, calculate max space used

F1(jtparsercalls){ASSERTMTV(w); R sc(jt->parsercalls);}

// 6!:5, window into the running J code
F1(jtpeekdata){ I opeek=JT(jt,peekdata); JT(jt,peekdata) = i0(w); R sc(opeek); }

/*
// 6!:6: set y as processor architecture and return previous value.  Now cannot set.  Bit 0=AVX instructions supported
F1 (jtprocarch){
//RE(newval=i0(w));
//jt->cpuarchavx=(C)newval;
R sc(C_AVX);
}

// 6!:7: return cpu features
 enum in android ndk
    CPU_X86_FEATURE_SSSE3  = (1 << 0),
    CPU_X86_FEATURE_POPCNT = (1 << 1),
    CPU_X86_FEATURE_MOVBE  = (1 << 2),
    CPU_X86_FEATURE_SSE4_1 = (1 << 3),
    CPU_X86_FEATURE_SSE4_2 = (1 << 4),
    CPU_X86_FEATURE_AES_NI = (1 << 5),
    CPU_X86_FEATURE_AVX =    (1 << 6),
    CPU_X86_FEATURE_RDRAND = (1 << 7),
    CPU_X86_FEATURE_AVX2 =   (1 << 8),
    CPU_X86_FEATURE_SHA_NI = (1 << 9),
F1 (jtprocfeat){
R sc((I)getCpuFeatures());
}
*/

#if SY_WIN32
 /* defined in jdll.c */
#else
F1(jtts){A z;D*x;struct tm tr,*t=&tr;struct timeval tv;
 ASSERTMTV(w);
 gettimeofday(&tv,NULL); t=localtime_r((time_t*)&tv.tv_sec,t);
 GAT0(z,FL,6,1); x=DAV(z);
 x[0]=t->tm_year+1900;
 x[1]=t->tm_mon+1;
 x[2]=t->tm_mday;
 x[3]=t->tm_hour;
 x[4]=t->tm_min;
 x[5]=t->tm_sec+(D)tv.tv_usec/1e6;
 R z;
}
#endif

F1(jtts0){A x,z;C s[9],*u,*v,*zv;D*xv;I n,q;
 ARGCHK1(w);
 ASSERT(1>=AR(w),EVRANK);
 RZ(x=ts(mtv));
 n=AN(w); xv=DAV(x);
 if(!n)R x;
 if(!ISDENSETYPE(AT(w),LIT))RZ(w=cvt(LIT,w));
 GATV(z,LIT,n,AR(w),AS(w)); zv=CAV(z); MC(zv,CAV(w),n);
 q=0; v=zv; DQ(n, q+='Y'==*v++;); u=2==q?s+2:s;   // if only 2 Y, advance over century
 sprintf(s,FMTI04,(I)xv[0]);             v=zv; DQ(n, if(*v=='Y'){*v=*u++; if(!*u)break;} ++v;);
 sprintf(s,FMTI02,(I)xv[1]);        u=s; v=zv; DQ(n, if(*v=='M'){*v=*u++; if(!*u)break;} ++v;);
 sprintf(s,FMTI02,(I)xv[2]);        u=s; v=zv; DQ(n, if(*v=='D'){*v=*u++; if(!*u)break;} ++v;);
 sprintf(s,FMTI02,(I)xv[3]);        u=s; v=zv; DQ(n, if(*v=='h'){*v=*u++; if(!*u)break;} ++v;);
 sprintf(s,FMTI02,(I)xv[4]);        u=s; v=zv; DQ(n, if(*v=='m'){*v=*u++; if(!*u)break;} ++v;);
 sprintf(s,FMTI05,(I)(1000*xv[5])); u=s; v=zv; DQ(n, if(*v=='s'){*v=*u++; if(!*u)break;} ++v;);
 R z;
}


#ifdef SY_GETTOD
D tod(void){struct timeval t; gettimeofday(&t,NULL); R t.tv_sec+(D)t.tv_usec/1e6;}
#else
#if SY_WINCE
D tod(void){SYSTEMTIME t; GetLocalTime(&t); R t.wSecond+(D)t.wMilliseconds/1e3;}
#else
D tod(void){R(D)clock()/CLOCKS_PER_SEC;}
#endif
#endif


#if SY_WIN32

typedef LARGE_INTEGER LI;

static const D qpm=4294967296.0;  /* 2^32 */

D qpf(void){LI n; QueryPerformanceFrequency(&n); R n.LowPart+qpm*n.HighPart;}

static D qpc(void){LI n; QueryPerformanceCounter(&n); R n.LowPart+qpm*n.HighPart;}

#else

D qpf(void){R (D)CLOCKS_PER_SEC;}

static D qpc(void){R tod()*CLOCKS_PER_SEC;}

#endif

/* 
// by Mark VanTassel from The Code Project

__int64 GetMachineCycleCount()
{      
   __int64 cycles;
   _asm rdtsc; // won't work on 486 or below - only pentium or above
   _asm lea ebx,cycles;
   _asm mov [ebx],eax;
   _asm mov [ebx+4],edx;
   return cycles;
}
*/


F1(jttss){ASSERTMTV(w); R scf(tod()-JT(jt,tssbase));}

F2(jttsit2){A z;D t;I n;
 F2RANK(0,1,jttsit2,DUMMYSELF);
 RE(n=i0(a));
 FDEPINC(1);  // No ASSERTs/returns till the DEPDEC below
 t=qpc(); 
 A *old=jt->tnextpushp; DQ(n, z=exec1(w); if(!z)break; tpop(old);); 
 t=qpc()-t;
 FDEPDEC(1);  // Assert OK now
 RZ(z);
 R scf(n?t/(n*pf):0);
}

F1(jttsit1){R tsit2(num(1),w);}

#ifdef _WIN32
#define sleepms(i) Sleep(i)
#else
#define sleepms(i) usleep(i*1000)
#endif

F1(jtdl){D m,n,*v;UINT ms,s;
 RZ(w=cvt(FL,w));
 n=0; v=DAV(w); DQ(AN(w), m=*v++; ASSERT(0<=m,EVDOMAIN); n+=m;);
 s=(UINT)jfloor(n); ms=(UINT)jround(1000*(n-s));
#if SYS & SYS_MACINTOSH
 {I t=TickCount()+(I)(60*n); while(t>TickCount())JBREAK0;}
#else
 DQ(s, sleepms(1000); JBREAK0;);
 sleepms(ms);
#endif
 R w;
}


F1(jtqpfreq){ASSERTMTV(w); R scf(pf);}

F1(jtqpctr ){ASSERTMTV(w); R scf(qpc());}

// 6!:12
F1(jtpmctr){D x;I q;
 RE(q=i0(w));
 ASSERT(JT(jt,pma),EVDOMAIN);
 x=q+(D)((PM0*)(CAV1(JT(jt,pma))))->pmctr;
 ASSERT(IMIN<=x&&x<FLIMAX,EVDOMAIN);
 ((PM0*)(CAV1(JT(jt,pma))))->pmctr=q=(I)x; jt->uflags.us.cx.cx_c.pmctr=!!q;  // tell cx and unquote to look for pm
 R sc(q);
}    /* add w to pmctr */

static F1(jtpmfree){A x,y;C*c;I m;PM*v;PM0*u;
 if(w){
  c=CAV(w); u=(PM0*)c; v=(PM*)(c+sizeof(PM0)); 
  m=u->wrapped?u->n:u->i; 
  DQ(m, x=v->name; if(x&&NAME&AT(x)&&AN(x)==AS(x)[0])fa(x); 
        y=v->loc;  if(y&&NAME&AT(y)&&AN(y)==AS(y)[0])fa(y); ++v;);
  fa(w);
 }
 R num(1);
}    /* free old data area */

F1(jtpmarea1){R pmarea2(vec(B01,2L,&zeroZ),w);}  // 6!:10

// 6!:10  y is data area to use   x is 2-ele list (record all lines),(wrap the buffer)
F2(jtpmarea2){A x;B a0,a1,*av;C*v;I an,n=0,s=sizeof(PM),s0=sizeof(PM0),wn;PM0*u;
 ARGCHK2(a,w);
 RZ(a=cvt(B01,a));  // force x boolean
 an=AN(a);
 ASSERT(1>=AR(a),EVRANK);
 ASSERT(2>=an,EVLENGTH);  // x must be list/atom no more than 2 long
 av=BAV(a); 
 a0=0<an?av[0]:0;  // a0 set for 'record all'
 a1=1<an?av[1]:0;  // a1 set for 'wrap buffer'
 RZ(w=vs(w)); RZ(w=mkwris(w));  // make buffer a rank-1 list, and make sure it is writable
 wn=AN(w);  // wn=length in bytes
 ASSERT(!wn||wn>=s+s0,EVLENGTH);  // make sure it can record at least 1 sample
 x=JT(jt,pma);   // read incumbent sample buffer
 jt->uflags.us.cx.cx_c.pmctr=0;  // clear sample counter and tracking status
 if(wn){ras(w); JT(jt,pma)=w;}else JT(jt,pma)=0;  // set new buffer address, if it is valid
 if(JT(jt,pma))spstarttracking();else spendtracking();  // track memory usage whenever PM is running
 RZ(pmfree(x));  // free the old buffer and all its contents, if there was one
 if(wn){  // if we are turning on profiling
  v=CAV1(w);   // we put a PM0 struct at the beginning of the buffer, and use the rest for PM records
  u=(PM0*)v; 
  u->rec=a0;
  u->n=n=(wn-s0)/s;   // fill in the header block
  u->i=0;
  u->s=jt->bytesmax=spbytesinuse();
  u->trunc=a1; 
  u->wrapped=0;
  u->pmctr=0;
 }
 R sc(n);
}

// Add an entry to the Performance Monitor area
// name and loc are A blocks for the name and current locale
// lc is the line number being executed, or _1 for start function, _2 for end function
// val is the PM counter
void jtpmrecord(J jt,A name,A loc,I lc,int val){A x,y;B b;PM*v;PM0*u;
 u=(PM0*)CAV1(JT(jt,pma));  // u-> pm control area
 v=(PM*)(CAV1(JT(jt,pma))+sizeof(PM0))+u->i;  // v -> next PM slot to fill
 if(b=u->wrapped){x=v->name; y=v->loc;}  // If this slot already has valid name/loc, extract those values for free
 ++u->i;  // Advance index to next slot
 if(u->i>u->n){u->wrapped=1; if(u->trunc){u->i=u->n; R;}else u->i=0;}  // If we stepped off the end,
  // reset next pointer to 0 (if not trunc) or stay pegged at then end (if trunc).  Trunc comes from the original x to start_jpm_
 v->name=name; if(name)ACINCRLOCAL(name);  // move name/loc; incr use counts
 v->loc =loc;  if(loc )ACINCRLOCAL(loc ); if(b){fa(x); fa(y);}  // If this slot was overwritten, decr use counts, freeing
 v->val =val;  // Save the NSI data
 v->lc  =lc;
 v->s=jt->bytesmax-u->s;
 u->s=jt->bytesmax=jt->bytes;
#if SY_WIN32
 QueryPerformanceCounter((LI*)v->t);  // Save PM info
#else
 {D d=tod(); MC(v->t,&d,sizeof(D));}
#endif
}

// 6!:12
F1(jtpmunpack){A*au,*av,c,t,x,z,*zv;B*b;D*dv;I*iv,k,k1,m,n,p,q,wn,*wv;PM*v,*v0,*vq;PM0*u;
 ARGCHK1(w);
 ASSERT(JT(jt,pma),EVDOMAIN);
 if(!ISDENSETYPE(AT(w),INT))RZ(w=cvt(INT,w));
 wn=AN(w); wv=AV(w);
 u=(PM0*)AV(JT(jt,pma)); p=u->wrapped?u->n-u->i:0; q=u->i; n=p+q;
 GATV0(x,B01,n,1); b=BAV(x); mvc(n,b,1,iotavec-IOTAVECBEGIN+(wn?C0:C1));
 if(wn){
  DO(wn, k=wv[i]; if(0>k)k+=n; ASSERT((UI)k<(UI)n,EVINDEX); b[k]=1;);
  m=0; 
  DO(n, if(b[i])++m;);
 }else m=n;
 v0=(PM*)(CAV1(JT(jt,pma))+sizeof(PM0)); vq=q+v0;
 GAT0(z,BOX,1+PMCOL,1); zv=AAV(z);
 GATV0(t,BOX,2*m,1); av=AAV(t); au=m+av;
 v=vq; DO(p, if(b[  i]){RZ(*av++=v->name?incorp(sfn(0,v->name)):mtv); RZ(*au++=v->loc?incorp(sfn(0,v->loc)):mtv);} ++v;); 
 v=v0; DO(q, if(b[p+i]){RZ(*av++=v->name?incorp(sfn(0,v->name)):mtv); RZ(*au++=v->loc?incorp(sfn(0,v->loc)):mtv);} ++v;); 
 RZ(x=indexof(t,t));
 RZ(c=eq(x,IX(SETIC(x,k1))));
 RZ(zv[6]=incorp(repeat(c,t)));
 RZ(x=indexof(repeat(c,x),x)); iv=AV(x);
 RZ(zv[0]=incorp(vec(INT,m,  iv)));
 RZ(zv[1]=incorp(vec(INT,m,m+iv)));
 GATV0(t,INT,m,1); zv[2]=incorp(t); iv=AV(t); v=vq; DO(p, if(b[i])*iv++=(I)v->val;  ++v;); v=v0; DO(q, if(b[p+i])*iv++=(I)v->val; ++v;);
 GATV0(t,INT,m,1); zv[3]=incorp(t); iv=AV(t); v=vq; DO(p, if(b[i])*iv++=v->lc; ++v;); v=v0; DO(q, if(b[p+i])*iv++=v->lc; ++v;);
 GATV0(t,INT,m,1); zv[4]=incorp(t); iv=AV(t); v=vq; DO(p, if(b[i])*iv++=v->s;  ++v;); v=v0; DO(q, if(b[p+i])*iv++=v->s;  ++v;); 
 GATV0(t,FL, m,1); zv[5]=incorp(t); dv=DAV(t);
#if SY_WIN32
 v=vq; DO(p, if(b[i]  )*dv++=(((LI*)v->t)->LowPart+qpm*((LI*)v->t)->HighPart)/pf; ++v;);
 v=v0; DO(q, if(b[p+i])*dv++=(((LI*)v->t)->LowPart+qpm*((LI*)v->t)->HighPart)/pf; ++v;);
#else
 v=vq; DO(p, if(b[i]  ){MC(dv,v->t,sizeof(D)); ++dv;} ++v;); 
 v=v0; DO(q, if(b[p+i]){MC(dv,v->t,sizeof(D)); ++dv;} ++v;); 
#endif
 R z;
}

// 6!:14
F1(jtpmstats){A x,z;I*zv;PM0*u;
 ASSERTMTV(w);
 GAT0(z,INT,6,1); zv=AV(z);
 if(x=JT(jt,pma)){
  u=(PM0*)AV(x);
  zv[0]=u->rec;
  zv[1]=u->trunc;
  zv[2]=u->n;
  zv[3]=u->wrapped?u->n:u->i;
  zv[4]=u->wrapped;
  zv[5]=((PM0*)(CAV1(JT(jt,pma))))->pmctr;
 }else zv[0]=zv[1]=zv[2]=zv[3]=zv[4]=zv[5]=0;
 R z;
}

