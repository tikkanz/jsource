/* Copyright 1990-2016, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Conjunctions: Explicit Definition : and Associates                      */

/* Usage of the f,g,h fields of : defined verbs:                           */
/*  f  character matrix of  left argument to :                             */
/*  g  character matrix of right argument to :                             */
/*  h  4-element vector of boxes                                           */
/*       0  vector of boxed tokens for f                                   */
/*       1  vector of triples of control information                       */
/*       2  vector of boxed tokens for g                                   */
/*       3  vector of triples of control information                       */

#include "j.h"
#include "d.h"
#include "p.h"
#include "w.h"

// DD definitions
#define DDBGN (US)('{'+256*'{')  // digraph for start DD
#define DDEND (US)('}'+256*'}')  // digraph for end DD
#define DDSEP 0xa  // ASCII value used to mark line separator inside 9 : string.  Must have class CU so that it ends a final comment

#define BASSERT(b,e)   {if(unlikely(!(b))){jsignal(e); i=-1; z=0; continue;}}
#define BZ(e)          if(unlikely(!(e))){i=-1; z=0; continue;}

#if SY_64
#define CWSENTX (cwgroup>>32)  // .sentx
#else
#define CWSENTX (cw[i].ig.indiv.sentx)
#endif

// sv->h is the A block for the [2][4] array of saved info for the definition; hv->[4] boxes of info for the current valence;
// line-> box 0 - tokens; x->box 1 - A block for control words; n (in flag word)=#control words; cw->array of control-word data, a CW struct for each
#define LINE(sv)       {A x; \
                        hv=AAV(sv->fgh[2])+4*((nG0ysfctdl>>6)&1);  \
                        line=AAV(hv[0]); x=hv[1]; nG0ysfctdl&=-65536; nG0ysfctdl|=(AN(x)<<16); cw=(CW*)AV(x);}

// Parse/execute a line, result in z.  If locked, reveal nothing.  Save current line number in case we reexecute
// If the sentence passes a u/v into an operator, the current symbol table will become the prev and will have the u/v environment info
// If the sentence fails, we go into debug mode and don't return until the user releases us
#if NAMETRACK
#define SETTRACK mvc(sizeof(trackinfo),trackinfo,1,iotavec-IOTAVECBEGIN+' '); wx=0; \
 wlen=sprintf(trackinfo,"%d: ",cw[i].source); wx+=wlen; trackinfo[wx++]=' '; \
 AK(trackbox)=(C*)queue-(C*)trackbox; AN(trackbox)=AS(trackbox)[0]=m; trackstg=unparse(trackbox); \
 wlen=AN(trackstg); wlen=wlen+wx>sizeof(trackinfo)-1?sizeof(trackinfo)-1-wx:wlen; MC(trackinfo+wx,CAV(trackstg),wlen); wx+=wlen; \
 trackinfo[wx]=0;  // null-terminate the info
#else
#define SETTRACK
#endif
#define parseline(z) {C attnval=*JT(jt,adbreakr); A *queue=line+CWSENTX; I m=(cwgroup>>16)&0xffff; \
 SETTRACK \
 if(likely(!attnval)){if(likely(!(nG0ysfctdl&16)))z=PARSERVALUE(parsea(queue,m));else {thisframe->dclnk->dcix=i; z=PARSERVALUE(parsex(queue,m,cw+i,callframe));}}else{jsignal(EVATTN); z=0;} }

/* for_xyz. t do. control data   */
typedef struct{
 A t;  // iteration array for for_xyz., select. value, or nullptr for for.
 A item;  // if for_xyz, the sorta-virtual block we are using to hold the value
 I j;  // iteration index
 I niter;  // for for. and for_xyz., number of iterations (number of items in T block)
 I itemsiz;  // size of an item of xyz, in bytes
 I4 w; // cw code for the structure
 LX itemsym;  // symbol number of xyz, 0 for for.
 LX indexsym;  // symbol unmber of xyz_index, 0 for for.
} CDATA;

#define WCD            (sizeof(CDATA)/sizeof(I))

typedef struct{I4 d,t,e,b;} TD;  // line numbers of catchd., catcht., end. and try.
#define WTD            (sizeof(TD)/sizeof(I))
#define NTD            17     /* maximum nesting for try/catch */

// called from for. or select. to start filling in the entry
static B forinitnames(J jt,CDATA*cv,I cwtype,A line){
 cv->j=-1;                               /* iteration index     */
 cv->t=0;  // init no selector value/iterator list
 cv->w=cwtype;  // remember type of control struct
 if(cwtype==CFOR){
  // for for_xyz., get the symbol indexes for xyz & xyz_index
  I k=AN(line)-5;  /* length of item name; -1 if omitted (for.; for_. not allowed) */
  if(k>0){A x;  // if it is a for_xyz.
   // We need a string buffer for "xyz_index".  Use the stack if the name is short
   C ss[20], *s; if(unlikely(k>(I)(sizeof(ss)-6))){GATV0(x,LIT,k+6,1); s=CAV1(x);}else s=ss;  // s point to buffer
   MC(s,CAV(line)+4,k);  MC(s+k,"_index",6L);  // move "xyz_index" into *s
   cv->itemsym=(probeislocal(nfs(k,s)))-JT(jt,sympv);  // get index of symbol in table, which must have been preallocated
   L *indexl; cv->indexsym=(indexl=probeislocal(nfs(k+6,s)))-JT(jt,sympv);
   if(unlikely(k>(I)(sizeof(ss)-6))){ACINITZAP(x); fr(x);}  // remove tpop and free, now that we're done.  We may be in a loop 
   // Make initial assignment to xyz_index, and mark it readonly
   // Since we remove the readonly at the end of the loop, the user might have changed our value; so if there is
   // an incumbent value, we remove it.  We also zap the value we install, just as in any normal assignment
   ASSERT(!(indexl->flag&LREADONLY),EVRO)  // it had better not be readonly now
   fa(indexl->val);  // if there is an incumbent value, discard it
   A xx; GAT0(xx,INT,1,0); IAV0(xx)[0]=-1;  // -1 is the iteration number if there are no iterations
   ACINITZAP(xx); indexl->val=xx;  // raise usecount, install as value of xyz_index
   indexl->flag|=LREADONLY;  // in the loop, the user may not modify xyz_index
  }else{cv->itemsym=cv->indexsym=0;}  // if not for_xyz., indicate with 0 indexes
 }
 R 1;  // normal return
}

// called to init the iterator for for.
static B jtforinit(J jt,CDATA*cv,A t){A x;C*s,*v;I k;
 ASSERT(t!=0,EVCTRL);
 SETIC(t,cv->niter);                            /* # of items in t     */
 if(likely(cv->indexsym!=0)){
  // for_xyz.   protect iterator value and save it; create virtual item name
  ASSERT(!ISSPARSE(AT(t)),EVNONCE)
  rifv(t);  // it would be work to handle virtual t, because you can't just ra() a virtual, as virtuals are freed only from the tpop stack.  So we wimp out & realize.
  ra(t) cv->t=t;  // if we need to save iteration array, do so, and protect from free
  // create virtual block for the iteration.  We will store this in xyz.  We have to do usecount by hand because
  // true virtual blocks are freed only by tpop, and we will be freeing this in unstackcv, either normally or at end-of-definition
  // We must keep ABACK in case we create a virtual block from xyz.
  // We store the block in 2 places: cv and symp.val.  We ra() once for each place
  // If there is an incumbent value, discard it
  A *aval=&JT(jt,sympv)[cv->itemsym].val; A val=*aval;  // stored reference address; incumbent value there
  fa(val); *aval=0;  // free the incumbent if any, clear val in symbol in case of error
  // Calculate the item size and save it
  I isz; I r=AR(t)-1; r=r<0?0:r; PROD(isz,r,AS(t)+1); I tt=AT(t); cv->itemsiz=isz<<bplg(tt); // rank of item; number of bytes in an item
  // Allocate a sorta-virtual block.  Zap it, fill it in, make noninplaceable.  Point it to the item before the data, since we preincrement in the loop
  A svb; GA(svb,tt,isz,r,AS(t)+1); // one item
  AK(svb)=(CAV(t)-(C*)svb)-cv->itemsiz; ACINITZAP(svb); ACINIT(svb,2); AFLAGINIT(svb,(tt&RECURSIBLE)|AFVIRTUAL); ABACK(svb)=t;  // We DO NOT raise the backer because this is sorta-virtual
  // Install the virtual block as xyz, and remember its address
  cv->item=*aval=svb;  // save in 2 places, commensurate with AC of 2
 }
 R 1;
}    /* for. do. end. initializations */

// A for/select. block is ending.   Free the iteration array.  Don't delete any names.  Mark the index as no longer readonly (in case we start the loop again)
// if assignvirt is set (normal), the xyz value is realized and reassigned if it is still the svb.  Otherwise it is freed and the value expunged.
static B jtunstackcv(J jt,CDATA*cv,I assignvirt){
 if(cv->w==CFOR){
  if(cv->t){  // if for_xyz. that has processed forinit ...
   JT(jt,sympv)[cv->indexsym].flag&=~LREADONLY;  // xyz_index is no longer readonly.  It is still available for inspection
   // If xyz still points to the virtual block, we must be exiting the loop early: the value must remain, so realize it
   A svb=cv->item;  // the sorta-virtual block for the item
   if(unlikely(JT(jt,sympv)[cv->itemsym].val==svb)){A newb;
    fa(svb);   // remove svb from itemsym.val.  Safe, because it can't be the last free
    if(likely(assignvirt!=0)){RZ(newb=realize(svb)); ACINITZAP(newb); ra00(newb,AT(newb)); JT(jt,sympv)[cv->itemsym].val=newb;  // realize stored value, raise, make recursive, store in symbol table
    }else{JT(jt,sympv)[cv->itemsym].val=0;}  // after error, we needn't bother with a value
   }
   // Decrement the usecount to account for being removed from cv - this is the final free of the svb
   fr(svb);  // MUST NOT USE fa() so that we don't recur and free svb's current contents in cv->t
  }
 }
 fa(cv->t);  // decr the for/select value, protected at beginning.  NOP if it is 0
 R 1;
}

// call here when we find that xyz_index has been aliased.  We remove it, free it, and replace it with a new block.  Return 0 if error
static A swapitervbl(J jt,A old,A *valloc){
 fa(old);  // discard the old value
 GAT0(old,INT,1,0);
 ACINITZAP(old); *valloc=old;  // raise usecount, install as value of xyz_index
 R old;
}

static void jttryinit(J jt,TD*v,I i,CW*cw){I j=i,t=0;
 v->b=(I4)i;v->d=v->t=0;
 while(t!=CEND){
  j=(j+cw)->go;  // skip through just the control words for this try. structure
  switch(t=(j+cw)->ig.indiv.type){
   case CCATCHD: v->d=(I4)j; break;
   case CCATCHT: v->t=(I4)j; break;
   case CEND:    v->e=(I4)j; break;
  }
 }
}  /* processing on hitting try. */

// tdv points to the try stack or 0 if none; tdi is index of NEXT try. slot to fill; i is goto line number
// if there is a try stack, pop its stack frames if they don't include the line number of the goto
// result is new value for tdi
// This is called only if tdi is nonzero & therefore we have a stack
static I trypopgoto(TD* tdv, I tdi, I dest){
 NOUNROLL while(tdi&&!BETWEENC(dest,tdv[tdi-1].b,tdv[tdi-1].e))--tdi;  // discard stack frame if structure does not include dest
 R tdi;
}

#define CHECKNOUN if (unlikely(!(NOUN&AT(t)))){   /* error, T block not creating noun */ \
    /* Signal post-exec error*/ \
    t=pee(line,&cw[ti],EVNONNOUN,nG0ysfctdl<<(BW-2),callframe); \
    /* go to error loc; if we are in a try., send this error to the catch.  z may be unprotected, so clear it, to 0 if error shows, mtm otherwise */ \
    i = cw[ti].go; if (i<SMAX){ RESETERR; z=mtm; if (nG0ysfctdl&4){if(!--tdi){jt->uflags.us.cx.cx_c.db=(UC)(nG0ysfctdl>>8); nG0ysfctdl^=4;} } }else z=0; \
    break; }

// Return next line to execute, in case debug changed it
// If debug is running we have to check for a new line to run, after any execution with error or on any line in case the debugger interrupted something
// result is line to continue on
static I debugnewi(I i, DC thisframe, A self){
 if(thisframe){DC siparent;  // if we can take debug input
  // debug mode was on when this execution started.  See if the execution pointer was changed by debug.
  if((siparent=thisframe->dclnk)&&siparent->dctype==DCCALL&&self==siparent->dcf){   // if prev stack frame is a call to here
   if(siparent->dcnewlineno){  // the debugger has asked for a jump
    i=siparent->dcix;  // get the jump-to line
    siparent->dcnewlineno=0;  // reset semaphore
   }
   siparent->dcix=i;  // notify the debugger of the line we are on, in case we stop  
  }
 }
 R i;  // return the current line, possibly modified
}

// Processing of explicit definitions, line by line
DF2(jtxdefn){F2PREFIP;PROLOG(0048);
 RE(0);
 A *line;   // pointer to the words of the definition.  Filled in by LINE
 CW *cw;  // pointer to control-word info for the definition.  Filled in by LINE
 UI nG0ysfctdl;  // flags: 1=locked 2=debug(& not locked) 4=tdi!=0 8=cd!=0 16=thisframe!=0 32=symtable was the original (i. e. !AR(symtab)&ARLSYMINUSE)
             // 64=call is dyadic 128=0    0xff00=original debug flag byte (must be highest bit)  0xffff0000=#cws in the definition
 DC callframe=0;  // pointer to the debug frame of the caller to this function (only if it's named), but 0 if we are not debugging
#if NAMETRACK
 // bring out the name, locale, and script into easy-to-display name
 C trackinfo[256];  // will hold name followed by locale
 fauxblock(trackunp); A trackbox; fauxBOXNR(trackbox,trackunp,0,1)  // faux block for line to unparse.  Will be filled in
 UI wx=0, wlen; A trackstg;   // index/len we will write to; unparsed line
 forcetomemory(&trackinfo);
#endif

 TD*tdv=0;  // pointer to base of try. stack
 I tdi=0;  // index of the next open slot in the try. stack
 CDATA*cv;  // pointer to the current entry in the for./select. stack

 A locsym;  // local symbol table to use

 nG0ysfctdl=((I)(a!=0)&(I)(w!=0))<<6;   // avoid branches, and relieve pressure on a and w
 DC thisframe=0;   // if we allocate a parser-stack frame, this is it
 {A *hv;  // will hold pointer to the precompiled parts
  V *sv=FAV(self); I sflg=sv->flag;   // fetch flags, which are the same even if VXOP is set
  A u,v;  // pointers to args
  // If this is adv/conj, it must be (1/2 : n) executed with no x or y.  Set uv then, and undefine x/y
  u=AT(self)&ADV+CONJ?a:0; v=AT(self)&ADV+CONJ?w:0;
// saved for next rev   a=AT(self)&ADV+CONJ?0:a; w=AT(self)&ADV+CONJ?0:w;   scaf should do this now?  may leave y undefined
  nG0ysfctdl|=SGNTO0(-(jt->glock|(sflg&VLOCK)));  // init flags: 1=lock bit, whether from locked script or locked verb
  // If this is a modifier-verb referring to x or y, set u, v to the modifier operands, and sv to the saved modifier (f=type, g=compiled text).  The flags don't change
  if(unlikely((sflg&VXOP)!=0)){u=sv->fgh[0]; v=sv->fgh[2]; sv=VAV(sv->fgh[1]);}
  // Read the info for the parsed definition, including control table and number of lines
  LINE(sv);
  // Create symbol table for this execution.  If the original symbol table is not in use (rank unflagged), use it;
  // otherwise clone a copy of it.  We have to do this before we create the debug frame
  locsym=hv[3];  // fetch pointer to preallocated symbol table
  ASSERT(locsym!=0,EVDOMAIN);  // if the valence is not defined, give valence error
  if(likely(!(AR(locsym)&ARLSYMINUSE))){AR(locsym)|=ARLSYMINUSE;nG0ysfctdl|=32;}  // remember if we are using the original symtab
  else{RZ(locsym=clonelocalsyms(locsym));}
  if(unlikely((jt->uflags.us.cx.cx_c.db | (sflg&(VTRY1|VTRY2))))){
   // special processing required
   // if we are in debug mode, the call to this defn should be on the stack (unless debug was entered under program control).  If it is, point to its
   // stack frame, which functions as a flag to indicate that we are debugging.  If the function is locked we ignore debug mode
   if(!(nG0ysfctdl&1)&&jt->uflags.us.cx.cx_c.db){
    if(jt->sitop&&jt->sitop->dctype==DCCALL){   // if current stack frame is a call
     nG0ysfctdl|=2;   // set debug flag if debug requested and not locked (no longer used)
     if(sv->flag&VNAMED){
      callframe=jt->sitop;  // if this is a named (rather than anonymous call like 3 : 0"1), there is a tight link with the caller.  Indicate that
     }
     // If we are in debug mode, and the current stack frame has the DCCALL type, pass the debugger
     // information about this execution: the local symbols and the control-word table
     if(self==jt->sitop->dcf){  // if the stack frame is for this exec
      jt->sitop->dcloc=locsym; jt->sitop->dcc=hv[1];  // install info about the exec
      // Use this out-of-the-way place to ensure that the compiler will not try to put for. and try. stuff into registers
      forcetomemory(&tdi); forcetomemory(&tdv); forcetomemory(&cv); 
     }
    }

    // With debug on, we will save pointers to the sentence being executed in the stack frame we just allocated
   }

   // If the verb contains try., allocate a try-stack area for it.  Remember debug state coming in so we can restore on exit
   if(sv->flag&VTRY1+VTRY2){A td; GAT0(td,INT,NTD*WTD,1); tdv=(TD*)AV(td); nG0ysfctdl |= jt->uflags.us.cx.cx_c.db<<8;}
  }
  // End of unusual processing
  SYMPUSHLOCAL(locsym);   // Chain the calling symbol table to this one

  // assignsym etc should never be set here; if it is, there must have been a pun-in-ASGSAFE that caused us to mark a
  // derived verb as ASGSAFE and it was later overwritten with an unsafe verb.  That would be a major mess; we'll invest
  // in preventing it - still not a full fix, since invalid inplacing may have been done already
  CLEARZOMBIE
  // Assign the special names x y m n u v.  Do this late in initialization because it would be bad to fail after assigning to yx (memory leak would result)
  // For low-rank short verbs, this takes a significant amount of time using IS, because the name doesn't have bucket info and is
  // not an assignment-in-place
  // So, we short-circuit the process by assigning directly to the name.  We take advantage of the fact that we know the
  // order in which the symbols were defined: y then x; and we know that insertions are made at the end; so we know
  // the bucketx for xy are 0 or maybe 1.  We have precalculated the buckets for each table size, so we can install the values
  // directly.
  // Assignments here are special.  At this point we know that the value is coming in from a different namespace, and that it will be
  // freed in that namespace.  Thus, (1) there is no need to realize a virtual block - we can just assign it to x/y, knowing that it is
  // not tying up a backer that could have been freed otherwise (we do have to raise the usecount); (2) if the input
  // has been abandoned, we do not need to raise the usecount here: we can just mark the arg non-inplaceable, usecount 1 and take advantage
  // of assignment in place here - in this case we must flag the name to suppress decrementing the usecount on reassignment or final free.  In
  // both cases we know the block will be freed by the caller.
  // Virtual abandoned blocks are both cases at once.  That's OK.
  UI4 yxbucks = *(UI4*)LXAV0(locsym);  // get the yx bucket indexes, stored in first hashchain by crelocalsyms
  L *sympv=JT(jt,sympv);  // bring into local
  L *ybuckptr = &sympv[LXAV0(locsym)[(US)yxbucks]];  // pointer to sym block for y, known to exist
  L *xbuckptr = &sympv[LXAV0(locsym)[yxbucks>>16]];  // pointer to sym block for x
  if(likely(w!=0)){  // If y given, install it & incr usecount as in assignment.  Include the script index of the modification
   // If input is abandoned inplace and not the same as x, DO NOT increment usecount, but mark as abandoned and make not-inplace.  Otherwise ra
   // We can handle an abandoned argument only if it is direct or recursive, since only those values can be assigned to a name
   if((a!=w)&SGNTO0(AC(w)&(((AT(w)^AFLAG(w))&RECURSIBLE)-1))&((I)jtinplace>>JTINPLACEWX)){
    ybuckptr->flag=LPERMANENT|LWASABANDONED; ACIPNO(w);  // remember, blocks from every may be 0x8..2, and we must preserve the usecount then as if we ra()d it
   }else{
    ra(w);  // not abandoned: raise the block
    if((likely(!(AFLAG(w)&AFVIRTUAL+AFNJA)))){AMNVRCINI(w)}  // since the block is now named, if it is not virtual it must switch to NVR interpretation of AM
   }
   ybuckptr->val=w; ybuckptr->sn=jt->currslistx;  // finish the assignment
  }
    // for x (if given), slot is from the beginning of hashchain EXCEPT when that collides with y; then follow y's chain
    // We have verified that hardware CRC32 never results in collision, but the software hashes do (needs to be confirmed on ARM CPU hardware CRC32C)
  if(a){
   if(!C_CRC32C&&xbuckptr==ybuckptr)xbuckptr=xbuckptr->next+sympv;
   if((a!=w)&SGNTO0(AC(a)&(((AT(a)^AFLAG(a))&RECURSIBLE)-1))&((I)jtinplace>>JTINPLACEAX)){
    xbuckptr->flag=LPERMANENT|LWASABANDONED; ACIPNO(a);
   }else{ra(a); if((likely(!(AFLAG(a)&AFVIRTUAL+AFNJA)))){AMNVRCINI(a)}}
   xbuckptr->val=a; xbuckptr->sn=jt->currslistx;
  }
  // Do the other assignments, which occur less frequently, with symbis
  if(unlikely(((I)u|(I)v)!=0)){
   if(u){(symbis(mnuvxynam[2],u,locsym)); if(NOUN&AT(u))symbis(mnuvxynam[0],u,locsym); }  // assign u, and m if u is a noun
   if(v){(symbis(mnuvxynam[3],v,locsym)); if(NOUN&AT(v))symbis(mnuvxynam[1],v,locsym); }  // bug errors here must be detected
  }
 }
 FDEPINC(1);   // do not use error exit after this point; use BASSERT, BGA, BZ
 // remember tnextpushx.  We will tpop after every sentence to free blocks.  Do this AFTER any memory
 // allocation that has to remain throughout this routine.
 // If the user turns on debugging in the middle of a definition, we will raise old when he does
 A *old=jt->tnextpushp;

 // loop over each sentence
 A cd=0;  // pointer to block holding the for./select. stack, if any
 I r=0;  // number of unused slots allocated in for./select. stack
 I i=0;  // control-word number of the executing line
 A z=mtm;  // last B-block result; will become the result of the execution. z=0 is treated as an error condition inside the loop, so we have to init the result to i. 0 0
 A t=0;  // last T-block result
 I4 bi;   // cw number of last B-block result.  Needed only if it gets a NONNOUN error - can force to memory
 I4 ti;   // cw number of last T-block result.  Needed only if it gets a NONNOUN error
 while(1){
  // i holds the control-word number of the current control word
  // Check for debug and other modes
  if(unlikely(jt->uflags.us.cx.cx_us!=0)){  // fast check to see if we have overhead functions to perform
   if(!(nG0ysfctdl&(16+1))&&jt->uflags.us.cx.cx_c.db){
    // If we haven't done so already, allocate an area to use for the SI entries for sentences executed here, if needed.  We need a new area only if we are debugging.  Don't do it if locked.
    // We have to have 1 debug frame to hold parse-error information in, but it is allocated earlier if debug is off
    // We check before every sentence in case the user turns on debug in the middle of this definition
    // NOTE: this stack frame could be put on the C stack, but that would reduce the recursion limit because the frame is pretty big
    // If there is no calling stack frame we can't turn on debug mode because we can't suspend
    // If we are executing a recursive call to JDo we can't go into debug because we can't prompt
    DC d; for(d=jt->sitop;d&&DCCALL!=d->dctype;d=d->dclnk);  /* find bottommost call                 */
    if(d&&jt->recurstate<RECSTATEPROMPT){  // if there is a call and thus we can suspend; and not prompting already
     BZ(thisframe=deba(DCPARSE,0L,0L,0L));  // if deba fails it will be before it modifies sitop.  Remember our stack frame
     old=jt->tnextpushp;  // protect the stack frame against free
     nG0ysfctdl|=16+2;  // indicate we have a debug frame and are in debug mode
     forcetomemory(&bi); forcetomemory(&ti);  // urge the compiler to leave these values in memory
    }
   }

   i=debugnewi(i,thisframe,self);  // get possibly-changed execution line
   if((UI)i>=(UI)(nG0ysfctdl>>16))break;

   // if performance monitor is on, collect data for it
   if(jt->uflags.us.cx.cx_c.pmctr&&C1==((PM0*)CAV1(JT(jt,pma)))->rec&&FAV(self)->flag&VNAMED)pmrecord(jt->curname,jt->global?LOCNAME(jt->global):0,i,nG0ysfctdl&64?VAL2:VAL1);
   // If the executing verb was reloaded during debug, switch over to the modified definition
   DC siparent;
   if(nG0ysfctdl&16){
     if(thisframe->dcredef&&(siparent=thisframe->dclnk)&&siparent->dcn&&DCCALL==siparent->dctype&&self!=siparent->dcf){A *hv;
      self=siparent->dcf; V *sv=FAV(self); LINE(sv); siparent->dcc=hv[1];  // LINE sets pointers for subsequent line lookups
      // Clear all local bucket info in the definition, since it doesn't match the symbol table now
      // This will affect the current definition and all future executions of this definition.  We allow it because
      // it's for debug only.  The symbol table itself persists
      DO(AN(hv[0]), if(AT(line[i])&NAME){NAV(line[i])->bucket=0;});
     }
    thisframe->dcredef=0;
   }
  }

  // Don't do the loop-exit test until debug has had the chance to update the execution line.  For example, we might be asked to reexecute the last line of the definition
  if(unlikely((UI)i>=(UI)(nG0ysfctdl>>16)))break;
  // process the control word according to its type
  I cwgroup;
  // **************** top of main dispatch loop ********************
  switch(((cwgroup=cw[i].ig.group[0])&31)){  // highest cw is 33, but it aliases to 1 & there is no 32

  // The top cases handle the case of if. T do. B B B... end B B...      without looping back to the switch except for the if.
  case CBBLOCK: case CBBLOCKEND:  // placed first because likely case for unpredicted first line of definition
dobblock:
   // B-block (present on every sentence in the B-block)
   // run the sentence
   tpop(old); parseline(z);
   // if there is no error, or ?? debug mode, step to next line
   if(likely(z!=0)){bi=i; i+=((cwgroup>>5)&1)+1;  // go to next sentence, or to the one after that if it's harmless end. 
    if(unlikely((UI)i>=(UI)(nG0ysfctdl>>16)))break;  // end of definition
    if(unlikely(((((cwgroup=cw[i].ig.group[0])^CBBLOCK)&0x1f)+jt->uflags.us.cx.cx_us)!=0))break;  // not another B block
    goto dobblock;  // avoid indirect-branch overhead on the likely case
    // BBLOCK is usually followed by another BBLOCK, but another important followon is END followed by BBLOCK.  BBLOCKEND means
    // 'bblock followed by end that falls through', i. e. a bblock whose successor is i+2.  By handling that we process all sequences of if. T do. B end. B... without having to go through the switch;
    // this means the switch will learn to go to the if.
   }else if((nG0ysfctdl&16)&&jt->uflags.us.cx.cx_c.db&(DB1)){  // error in debug mode
    z=mtm,bi=i,i=debugnewi(i+1,thisframe,self);   // Remember the line w/error; fetch continuation line if any. it is OK to have jerr set if we are in debug mode, but z must be a harmless value to avoid error protecting it
   // if the error is THROW, and there is a catcht. block, go there, otherwise pass the THROW up the line
   }else if(EVTHROW==jt->jerr){
    if(nG0ysfctdl&4&&(tdv+tdi-1)->t){i=(tdv+tdi-1)->t+1; RESETERR; z=mtm;}else BASSERT(0,EVTHROW);  // z might not be protected if we hit error
   // for other error, go to the error location; if that's out of range, keep the error; if not,
   // it must be a try. block, so clear the error.  Pop the try. stack, and if it pops back to 0, restore debug mode (since we no longer have a try.)
   // NOTE ERROR: if we are in a for. or select., going to the catch. will leave the stack corrupted,
   // with the for./select. structures hanging on.  Solution would be to save the for/select stackpointer in the
   // try. stack, so that when we go to the catch. we can cut the for/select stack back to where it
   // was when the try. was encountered
   }else{i=cw[i].go; if(i<SMAX){RESETERR; z=mtm; if(nG0ysfctdl&4){if(!--tdi){jt->uflags.us.cx.cx_c.db=(UC)(nG0ysfctdl>>8); nG0ysfctdl^=4;}}}  // z might not have been protected: keep it safe. This is B1 try. error catch. return. end.
   }
   break;

  case CIF: case CWHILE: case CELSEIF:
     // in a long run of only if. and B blocks, the only switches executed will go to the if. processor, predictably
   i=cw[i].go;  // Go to the next sentence, whatever it is
   if(unlikely((UI)i>=(UI)(nG0ysfctdl>>16)))break;  // no fallthrough if line exits
   if(unlikely((((cwgroup=cw[i].ig.group[0])^CTBLOCK)&0xff)+jt->uflags.us.cx.cx_us))break;  // avoid indirect-branch overhead on the likely case
   // fall through to...
  case CASSERT:
  case CTBLOCK:
tblockcase:
   // execute and parse line as if for B block, except save the result in t
   // If there is a possibility that the previous B result may become the result of this definition,
   // protect it during the frees during the T block.  Otherwise, just free memory
   if(likely(cwgroup&0x200))tpop(old);else z=gc(z,old);   // 2 means previous B can't be the result
   // Check for assert.  Since this is only for T-blocks we tolerate the test (rather than duplicating code)
   if(unlikely((cwgroup&0xff)==CASSERT)){
    if(JT(jt,assert)){parseline(t); if(t&&!(NOUN&AT(t)&&all1(eq(num(1),t))))t=pee(line,cw+i,EVASSERT,nG0ysfctdl<<(BW-2),callframe);  // if assert., signal post-execution error if result not all 1s.  May go into debug; sets to result after debug
    }else{++i; break;}  // if ignored assert, go to NSI
   }else{parseline(t);}
   if(likely(t!=0)){ti=i,++i;  // if no error, continue on
    if(unlikely((UI)i>=(UI)(nG0ysfctdl>>16)))break;  // exit if end of defn
    if(unlikely(((((cwgroup=cw[i].ig.group[0])^CDO)&0xff)+jt->uflags.us.cx.cx_us)!=0))break;  // break if T block extended
    goto docase;  // avoid indirect-branch overhead on the likely case, if. T do.
   }else if((nG0ysfctdl&16)&&DB1&jt->uflags.us.cx.cx_c.db)ti=i,i=debugnewi(i+1,thisframe,self);  // error in debug mode: when coming out of debug, go to new line (there had better be one)
   else if(EVTHROW==jt->jerr){if(nG0ysfctdl&4&&(tdv+tdi-1)->t){i=(tdv+tdi-1)->t+1; RESETERR;}else BASSERT(0,EVTHROW);}  // if throw., and there is a catch., do so
   else{i=cw[i].go; if(i<SMAX){RESETERR; z=mtm; if(nG0ysfctdl&4){if(!--tdi){jt->uflags.us.cx.cx_c.db=(UC)(nG0ysfctdl>>8); nG0ysfctdl^=4;}}}else z=0;}  // uncaught error: if we take error exit, we might not have protected z, which is not needed anyway; so clear it to prevent invalid use
     // if we are not taking the error exit, we still need to set z to a safe value since we might not have protected it.  This is B1 try. if. error do. end. catch. return. end.
   break;

  case CDO:
docase:
   // do. here is one following if., elseif., or while. .  It always follows a T block, and skips the
   // following B block if the condition is false.
  {A tt=t; tt=t?t:mtv;  // missing t looks like '' which is true
   //  Start by assuming condition is true; set to move to the next line then
   ++i;
   // Quick true cases are: nonexistent t; empty t; direct numeric t with low byte nonzero.  This gets most of the true.  We add in char types and BOX cause it's free (they are always true)
   if(likely(AN(tt)))if((-(AT(tt)&(B01|LIT|INT|FL|CMPX|C2T|C4T|BOX))&-((I)CAV(tt)[0]))>=0){I nexti=cw[i-1].go;  // C cond is false if (type direct or BOX) and (value not 0).  J cond is true then.  Musn't fetch CAV[0] if AN==0
    // here the type is indirect or the low byte is 0.  We must compare more
    while(1){  // 2 loops if sparse
     if(likely(AT(tt)&INT+B01)){i=BIV0(tt)?i:nexti; break;} // INT and B01 are most common
     if(AT(tt)&FL){i=DAV(tt)[0]?i:nexti; break;}
     if(AT(tt)&CMPX){i=DAV(tt)[0]||DAV(tt)[1]?i:nexti; break;}
     if(AT(tt)&(RAT|XNUM)){i=1<AN(XAV(tt)[0])||IAV(XAV(tt)[0])[0]?i:nexti; break;}
     if(!(AT(tt)&NOUN)){CHECKNOUN}  // will take error
     // other types test true, which is how i is set
     if(!ISSPARSE(AT(tt)))break;
     BZ(tt=denseit(tt)); if(AN(tt)==0)break;  // convert sparse to dense - this could make the length go to 0, in which case true
    }
   }
   }
   t=0;  // Indicate no T block, now that we have processed it
   if(unlikely((UI)i>=(UI)(nG0ysfctdl>>16)))break;
   if(unlikely(((((cwgroup=cw[i].ig.group[0])^CBBLOCK)&0x1f)+jt->uflags.us.cx.cx_us)!=0))break;  // check for end of definition of special types
   goto dobblock;   // normal case, continue with B processing, without switch overhead

  // ************* The rest of the cases are accessed only by indirect branch or fixed fallthrough ********************
  case CTRY:
   // try.  create a try-stack entry, step to next line
   BASSERT(tdi<NTD,EVLIMIT);
   tryinit(tdv+tdi,i,cw);
   // turn off debugging UNLESS there is a catchd; then turn on only if user set debug mode
   // if debugging is already off, it stays off
   if(unlikely(jt->uflags.us.cx.cx_c.db))jt->uflags.us.cx.cx_c.db=(nG0ysfctdl&16)&&(UC)(tdv+tdi)->d?JT(jt,dbuser):0;
   ++tdi; ++i; nG0ysfctdl|=4;  // bump tdi pointer, set flag
   break;
  case CCATCH: case CCATCHD: case CCATCHT:
   // catch.  pop the try-stack, go to end., reset debug state.  There should always be a try. stack here
   if(nG0ysfctdl&4){if(!--tdi){jt->uflags.us.cx.cx_c.db=(UC)(nG0ysfctdl>>8); nG0ysfctdl^=4;} i=1+(tdv+tdi)->e;}else i=cw[i].go; break;
  case CTHROW:
   // throw.  Create a throw error
   BASSERT(0,EVTHROW);
  case CFOR:
  case CSELECT: case CSELECTN:
   // for./select. push the stack.  If the stack has not been allocated, start with 9 entries.  After that,
   // if it fills up, double it as required
   if(unlikely(!r))
    if(unlikely(nG0ysfctdl&8)){I m=AN(cd)/WCD; BZ(cd=ext(1,cd)); cv=(CDATA*)AV(cd)+m-1; r=AN(cd)/WCD-m;}
    else  {r=9; GAT0E(cd,INT,9*WCD,1,i=-1; z=0; continue); ACINITZAP(cd) cv=(CDATA*)IAV1(cd)-1; nG0ysfctdl|=8;}   // 9=r

   ++cv; --r;
   BZ(forinitnames(jt,cv,cwgroup&0xff,line[CWSENTX]));  // setup the names, before we see the iteration value
   ++i;
   break;
  case CDOF:   // do. after for.
   // do. after for. .  If this is first time, initialize the iterator
   if(unlikely(cv->j<0)){
    BASSERT(t!=0,EVCTRL);   // Error if no sentences in T-block
    CHECKNOUN    // if t is not a noun, signal error on the last line executed in the T block
    BZ(forinit(cv,t)); t=0;
   }
   ++cv->j;  // step to first (or next) iteration
   if(likely(cv->indexsym!=0)){
    L *sympv=JT(jt,sympv);  // base of symbol array
    A *aval=&sympv[cv->indexsym].val;  // address of iteration-count slot
    A iterct=*aval;  // A block for iteration count
    if(unlikely(AC(iterct)>1))BZ(iterct=swapitervbl(jt,iterct,aval));  // if value is now aliased, swap it out before we change it
    IAV0(iterct)[0]=cv->j;  // Install iteration number into the readonly index
    aval=&sympv[cv->itemsym].val;  // switch aval to address of item slot
    if(unlikely(!(cwgroup&0x200)))BZ(z=rat(z));   // if z might be the result, protect it over the free
    if(likely(cv->j<cv->niter)){  // if there are more iterations to do...
    // if xyz has been reassigned, fa the incumbent and reinstate the sorta-virtual block, advanced to the next item
     AK(cv->item)+=cv->itemsiz;  // advance to next item
     if(unlikely(*aval!=cv->item)){A val=*aval; fa(val) val=cv->item; ra(val) *aval=val;}  // discard & free incumbent, switch to sorta-virtual, raise it
     ++i; continue;   // advance to next line and process it
    }
    // ending the iteration.  set xyz to i.0
    {A val=*aval; fa(val)}  // discard & free incumbent, probably the sorta-virtual block
    *aval=mtv;  // after last iteration, set xyz to mtv, which is permanent
   }else if(likely(cv->j<cv->niter)){++i; continue;}  // advance to next line and process it
   // if there are no more iterations, fall through...
  case CENDSEL:
   // end. for select., and do. for for. after the last iteration, must pop the stack - just once
   // Must rat() if the current result might be final result, in case it includes the variables we will delete in unstack
   // (this includes ONLY xyz_index, so perhaps we should avoid rat if stack empty or xyz_index not used)
   if(unlikely(!(cwgroup&0x200)))BZ(z=rat(z)); unstackcv(cv,1); --cv; ++r; 
   i=cw[i].go;    // continue at new location
   break;
  case CBREAKS:
  case CCONTS:
   // break./continue-in-while. must pop the stack if there is a select. nested in the loop.  These are
   // any number of SELECTN, up to the SELECT 
   if(unlikely(!(cwgroup&0x200)))BZ(z=rat(z));   // protect possible result from pop, if it might be the final result
   NOUNROLL do{I fin=cv->w==CSELECT; unstackcv(cv,1); --cv; ++r; if(fin)break;}while(1);
    // fall through to...
  case CBREAK:
  case CCONT:  // break./continue. in while., outside of select.
   i=cw[i].go;   // After popping any select. off the stack, continue at new address
   // It must also pop the try. stack, if the destination is outside the try.-end. range
   if(nG0ysfctdl&4){tdi=trypopgoto(tdv,tdi,i); nG0ysfctdl^=tdi?0:4;}
   break;
  case CBREAKF:
   // break. in a for. must first pop any active select., and then pop the for.
   // We just pop till we have popped a non-select.
   // Must rat() if the current result might be final result, in case it includes the variables we will delete in unstack
   if(unlikely(!(cwgroup&0x200)))BZ(z=rat(z));   // protect possible result from pop
   NOUNROLL do{I fin=cv->w; unstackcv(cv,1); --cv; ++r; if((fin^CSELECT)>(CSELECT^CSELECTN))break;}while(1);  // exit on non-SELECT/SELECTN
   i=cw[i].go;     // continue at new location
   // It must also pop the try. stack, if the destination is outside the try.-end. range
   if(nG0ysfctdl&4){tdi=trypopgoto(tdv,tdi,i); nG0ysfctdl^=tdi?0:4;}
   break;
  case CRETURN:
   // return.  Protect the result during free, pop the stack back to empty, set i (which will exit)
   i=cw[i].go;   // If there is a try stack, restore to initial debug state.  Probably safe to  do unconditionally
   if(nG0ysfctdl&4)jt->uflags.us.cx.cx_c.db=(UC)(nG0ysfctdl>>8);  // if we had an unfinished try. struct, restore original debug state
   break;
  case CCASE:
  case CFCASE:
   // case. and fcase. are used to start a selection.  t has the result of the T block; we check to
   // make sure this is a noun, and save it on the stack in cv->t.  Then clear t
   if(!cv->t){
    // This is the first case.  That means the t block has the select. value.  Save it.
    BASSERT(t!=0,EVCTRL);  // error if select. case.
    CHECKNOUN    // if t is not a noun, signal error on the last line executed in the T block
    BZ(ras(t)); cv->t=t; t=0;  // protect t from free while we are comparing with it, save in stack
   }
   i=cw[i].go;  // Go to next sentence, which might be in the default case (if T block is empty)
   if(likely((UI)i<(UI)(nG0ysfctdl>>16)))if(likely(!((((cwgroup=cw[i].ig.group[0])^CTBLOCK)&0xff)+jt->uflags.us.cx.cx_us)))goto tblockcase;  // avoid indirect-branch overhead on the likely case, which is case. t-block do.
   break;  // if it's not a t-block, take the indirect branch
  case CDOSEL:   // do. after case. or fcase.
   // do. for case./fcase. evaluates the condition.  t is the result (a T block); if it is nonexistent
   // or not all 0, we advance to the next sentence (in the case); otherwise skip to next test/end
   ++i;  // go to NSI if case tests true
   if(t){    // if t is not a noun, signal error on the last line executed in the T block
    CHECKNOUN
    if(!((AT(t)|AT(cv->t))&BOX)){
     // if neither t nor cv is boxed, just compare for equality.  Boxed empty goes through the other path
     if(!equ(t,cv->t))i=cw[i-1].go;  // should perhaps take the scalar case specially & send it through singleton code
    }else{
     if(all0(eps(boxopen(cv->t),boxopen(t))))i=cw[i-1].go;  // if case tests false, jump around bblock   test is cv +./@:,@:e. boxopen t
    }
    // Clear t to ensure that the next case./fcase. does not reuse this value
    t=0;
   }
   if(likely((UI)i<(UI)(nG0ysfctdl>>16)))if(likely(!((((cwgroup=cw[i].ig.group[0])^CBBLOCK)&0x1f)+jt->uflags.us.cx.cx_us)))goto dobblock;  // avoid indirect-branch overhead on the likely  case. ... do. bblock
   break;
  default:   //   CELSE CWHILST CGOTO CEND
   if(unlikely(2<=*JT(jt,adbreakr))) {BASSERT(0,EVBREAK);} 
     // JBREAK0, but we have to finish the loop.  This is double-ATTN, and bypasses the TRY block
   i=cw[i].go;  // Go to the next sentence, whatever it is
  }
 }  // end of main loop
 // We still must not take an error exit in this runout.  We have to hang around to the end to restore symbol tables, pointers, etc.

 FDEPDEC(1);  // OK to ASSERT now
 //  z may be 0 here and may become 0 before we exit
 if(likely(z!=0)){
  // There was a result (normal case)
  // If we are executing a verb (whether or not it started with 3 : or [12] :), make sure the result is a noun.
  // If it isn't, generate a post-execution error for the non-noun
  if(likely(AT(z)&NOUN)){
   // If we are returning a virtual block, we are going to have to realize it.  This is because it might be (indeed, probably is) backed by a local symbol that
   // is going to be summarily freed by the symfreeha() below.  We could modify symfreeha to recognize when we are freeing z, but the case is not common enough
   // to be worth the trouble
   realizeifvirtual(z);
  }else if(AT(self)&ADV+CONJ){  // non-noun result, but OK from adv/conj
   // if we are returning a non-noun, we have to cleanse it of any implicit locatives that refer to the symbol table in use now.
   // It is OK to refer to other symbol tables, since they will be removed if they try to escape at higher lavels and in the meantime can be executed; but
   // there is no way we could have a reference to such an implied locative unless we also had a reference to the current table; so we replace only the
   // first locative in each branch
   z=fix(z,sc(FIXALOCSONLY|FIXALOCSONLYLOWEST));
  }else {pee(line,&cw[bi],EVNONNOUN,nG0ysfctdl<<(BW-2),callframe); z=0;}  // signal error, set z to 'no result'
 }else{
  // No result.  Must be an error
  if(nG0ysfctdl&4)jt->uflags.us.cx.cx_c.db=(UC)(nG0ysfctdl>>8);  // if we had an unfinished try. struct, restore original debug state
  // Since we initialized z to i. 0 0, there's nothing more to do
 }

 if(unlikely(nG0ysfctdl&16)){debz();}   // pair with the deba if we did one
 A prevlocsyms=(A)AM(locsym);  // get symbol table to return to, before we free the old one
 if(likely((REPSGN(SGNIF(nG0ysfctdl,3))&((I)cv^(I)((CDATA*)IAV1(cd)-1)))==0)){  // if we never allocated cd, or the stack is empty
  // Normal path.  protect the result block and free everything allocated here, possibly including jt->locsyms if it was cloned (it is on the stack now)
  z=EPILOGNORET(z);  // protect return value from being freed when the symbol table is.  Must also be before stack cleanup, in case the return value is xyz_index or the like
 }else{
  // Unusual path with an unclosed contruct (e. g. return. from inside for. loop).  We have to free up the for. stack, but the return value might be one of the names
  // to be deleted on the for. stack, so we must protect the result before we pop the stack.  BUT, EPILOG frees all locally-allocated blocks, which might include the symbol
  // table that we need to pop from.  So we protect the symbol table during the cleanup of the result and stack.
  ra(locsym);  // protect local symtable - not contents
  z=EPILOGNORET(z);  // protect return value from being freed when the symbol table is.  Must also be before stack cleanup, in case the return value is xyz_index or the like
  NOUNROLL while(cv!=(CDATA*)IAV1(cd)-1){unstackcv(cv,0); --cv;}  // clean up any remnants left on the for/select stack
  fa(cd);  // have to delete explicitly, because we had to ext() the block and thus protect it with ra()
  fa(locsym);  // unprotect local syms.  This deletes them if they were cloned
 }
 // locsym may have been freed now

 // If we are using the original local symbol table, clear it (free all values, free non-permanent names) for next use.  We know it hasn't been freed yet
 // We detect original symbol table by rank ARLSYMINUSE - other symbol tables are assigned rank 0.
 // Tables are born with NAMEADDED off.  It gets set when a name is added.  Setting back to initial state here, we clear NAMEADDED
 if(likely(nG0ysfctdl&32)){symfreeha(locsym); AR(locsym)=ARLOCALTABLE;}
 // Pop the private-area stack
 SYMSETLOCAL(prevlocsyms);
 // Now that we have deleted all the local symbols, we can see if we were returning one.
 // See if EPILOG pushed a pointer to the block we are returning.  If it did, and the usecount we are returning is 1, set this
 // result as inplaceable and install the address of the tpop stack into AM (as is required for all inplaceable blocks).  If the usecount is inplaceable 1,
 // we don't do this, because it is possible that the AM slot was inherited from higher up the stack.
 // Note that we know we are not returning a virtual block here, so it is OK to write to AM
 // BUT: SPARSE value must NEVER be inplaceable, because the children aren't handled correctly during assignment
 if(likely(z!=0))if(likely((_ttop!=jt->tnextpushp)==AC(z))){ACRESET(z,(ACINPLACE&~AT(z))|ACUC1) AZAPLOC(z)=_ttop;}  // AC can't be 0.  The block is not in use elsewhere
 RETF(z);
}

// execution of u : v, selecting the version of self to use based on  valence
static DF1(xv1){A z; R df1(z,  w,FAV(self)->fgh[0]);}
static DF2(xv2){A z; R df2(z,a,w,FAV(self)->fgh[1]);}

static DF1(xn1 ){R xdefn(0L,w, self);}  // Transfer monadic xdef to the common code - inplaceable
static DF1(xadv){R xdefn(w, 0L,self);}  // inplaceable

// Nilad.  See if an anonymous verb needs to be named.  If so, result is the name, otherwise 0
static F1(jtxopcall){R jt->uflags.us.cx.cx_c.db&&DCCALL==jt->sitop->dctype?jt->sitop->dca:0;}


// This handles adverbs/conjs that refer to x/y.  Install a[/w] into the derived verb as f/h, and copy the flags
// point g in the derived verb to the original self
// If we have to add a name for debugging purposes, do so
// Flag the operator with VOPR, and remove VFIX for it so that the compound can be fixed
DF2(jtxop2){A ff,x;
 RZ(ff=fdef(0,CCOLON,VERB, xn1,jtxdefn, a,self,w,  (VXOP|VFIX|VJTFLGOK1|VJTFLGOK2)^FAV(self)->flag, RMAX,RMAX,RMAX));
 R (x=xopcall(0))?namerefop(x,ff):ff;
}
static DF1(xop1){
 R xop2(w,0,self);
}


// w is a box containing enqueued words for the sentences of a definition, jammed together
// 8: nv found 4: mu found 2: x found 1: y found.  u./v. count as u./v.
static I jtxop(J jt,A w){I i,k;
 // init flags to 'not found'
 I fndflag=0;
 A *wv=AAV(w);
   
 I in=AN(w);
 // Loop over each word, starting at beginning where the references are more likely
 for(i=0;i<in;++i) {
  A w=wv[i];  // w is box containing a queue value.  If it's a name, we inspect it
  if(AT(w)&NAME){
   // Get length/string pointer
   I n=AN(w); C *s=NAV(w)->s;
   if(n){
    // Set flags if this is a special name, or an indirect locative referring to a special name in the last position, or u./v.
    if(n==1||(n>=3&&s[n-3]=='_'&&s[n-2]=='_')){
     if(s[n-1]=='n'||s[n-1]=='v')fndflag|=8;
     if(s[n-1]=='m'||s[n-1]=='u')fndflag|=4;
     else if(s[n-1]=='x')fndflag|=2;
     else if(s[n-1]=='y')fndflag|=1;
    }   // 'one-character name'
   }  // 'name is not empty'
  } // 'is name'
  if(AT(w)&VERB){
    if((FAV(w)->id&-2)==CUDOT)fndflag|=4;  // u./v.
  }
  // exit if we have seen enough: mnuv plus x.  No need to wait for y.  If we have seen only y, keep looking for x
  if(fndflag>=8+4+2)R fndflag;
 }  // loop for each word
 R fndflag;  // return what we found
}

// handle m : 0.  deftype=m.
// For 0 : and 13 :, the result is a string.  For other types, it is a list of boxes, one per
// line of the definition.  DDs in the input will have been translated to 9 : string form.
// Stop when we hit ) line, or EOF
static A jtcolon0(J jt, I deftype){A l,z;C*p,*q,*s;A *sb;I m,n;
 n=0;
 I isboxed=BETWEENC(deftype,1,9);  // do we return boxes?
 // Allocate the return area, which we extend as needed
 if(isboxed){RZ(z=exta(BOX,1L,1L,20L)); sb=AAV(z);}
 else{RZ(z=exta(LIT,1L,1L,300L)); s=CAV(z);}
 while(1){
  RE(l=jgets("\001"));   // abort if error on input
  if(!l)break;  // exit loop if EOF.  The incomplete definition will be processed
  if(deftype!=0)RZ(l=ddtokens(l,8+2));  // if non-noun def, handle DDs, for explicit def, return string, allow jgets().  Leave noun contents untouched
  // check for end: ) by itself, possibly with spaces
  m=AN(l); p=q=CAV(l); 
  NOUNROLL while(p<q+m&&' '==*p)++p; if(p<q+m&&')'==*p){NOUNROLL while(p<q+m&&' '==*++p); if(p>=m+q)break;}  // if ) with nothing else but blanks, stop
  // There is a new line.  Append it to the growing result.
  if(isboxed){
   if((C2T+C4T)&AT(l))RZ(l=cvt(LIT,l));  // each line must be LIT
   NOUNROLL while(AN(z)<=n+1){RZ(z=ext(0,z)); sb=AAV(z);}  // extend the result if necessary
   sb[n]=incorp(l); ++n; // append the line, increment line number
  }else{
   NOUNROLL while(AN(z)<=n+m){RZ(z=ext(0,z)); s=CAV(z);}  // extend the result if necessary
   MC(s+n,q,m); n+=m; s[n]=CLF; ++n;  // append LF at end of each line
  }
 }
 // Return the string.  No need to trim down the list of boxes, as it's transitory
 if(isboxed){AN(z)=AS(z)[0]=n; R z;}
 R str(n,s);
}    /* enter nl terminated lines; ) on a line by itself to exit */

// w is character array or list
// if table, take , w ,. LF    if list take ,&LF^:(LF~:{:) w)
static F1(jtlineit){
 R 1<AR(w)?ravel(stitch(w,scc(CLF))):AN(w)&&CLF==cl(w)?w:over(w,scc(CLF));
}

// Convert ASCII w to boxed lines.  Create separate lists of boxes for monad and dyad
// if preparsed is set, we know the lines have gone through wordil already & it is OK
// to do it again.  This means we are processing 9 :  n
static A jtsent12c(J jt,A w){C*p,*q,*r,*s,*x;A z;
 ASSERT(!AN(w)||LIT&AT(w),EVDOMAIN);
 ASSERT(2>=AR(w),EVRANK);
 if(AR(w)>1)R IRS1(w,0L,1,jtbox,z);  // table, just box lines individually 

 // otherwise we have a single string.  Could be from 9 : string
 if(!(AN(w)&&DDSEP==cl(w)))RZ(w=over(w,scc(DDSEP)));  // add LF if missing
 // Lines are separated by DDSEP, and there may be DDSEP embedded in strings.  Convert the whole thing to words, which will
 // leave the embedded DDSEP embedded; then split on the individual DDSEP tokens
 // tokenize the lines.  Each LF is its own token.
 A wil; RZ(wil=wordil(w)); ASSERT(AM(wil)>=0,EVOPENQ) makewritable(wil); I wiln=AS(wil)[0]; I (*wilv)[2]=voidAV(wil); // line index, and number of lines; array of (start,end+1) for each line
 // Compact the word-list to a line-list.  Go through the words looking for LF.  For each line, add an entry to the line-list with all the characters except the LF
 I i=0;  // start of dyad, input pointer through wilv
 I currlinest=0;  // character# at which current line (ending in next LF) starts
 I linex=0;  // index of next output (start,end) to fill
 C *wv=CAV(w);  // the character list backing the words
 while(i<wiln){
  // we have just finished a line.  currlinest is the first character position of the next line
  // a real line.  scan to find next DDSEP.  There must be one.
  NOUNROLL while(wv[wilv[i][0]]!=DDSEP)++i;  // advance to LF
  // add the new line - everything except the LF - to the line list
  wilv[linex][0]=currlinest; wilv[linex][1]=wilv[i][0];  // add the line
  ++linex; currlinest=wilv[i][0]+1;  // advance line pointer, skip over the LF to start of next line
  ++i;  // skip over the LF we processed
 }
 // Now we have compacted all the lines.  Box them
 AS(wil)[0]=linex;  // advance to dyad, set its length
 R jtboxcut0(jt,wil,w,ds(CWORDS));
}    /* literal fret-terminated or matrix sentences into monad/dyad */

// Audit w to make sure it contains all strings; convert to LIT if needed
static A jtsent12b(J jt,A w){A t,*wv,y,*yv;I j,*v;
 ASSERT(1>=AR(w),EVRANK);
 wv=AAV(w); 
 GATV(y,BOX,AN(w),AR(w),AS(w)); yv=AAV(y);
 DO(AN(w), RZ(yv[i]=incorp(vs(wv[i]))););
 R y;
}    /* boxed sentences into monad/dyad */

// If *t is a local name, replace it with a pointer to a shared copy, namely the block in the symbol-name already
// Also install bucket info into a local name (but leave the hashes calculated for others)
// in any case, if dobuckets is 0, remove the bucket field (NOT bucketx).  Clearing the bucket field will prevent
// a name's escaping and being used in another context with invalid bucket info.  bucketx must survive in case it holds
// a valid hash of a locative name
// actstv points to the chain headers, actstn is the number of chains
// all the chains have had the non-PERMANENT flag cleared in the pointers
// recur is set if *t is part of a recursive noun
static A jtcalclocalbuckets(J jt, A *t, LX *actstv, I actstn, I dobuckets, I recur){LX k;
 A tv=*t;  // the actual NAME block
 L *sympv=JT(jt,sympv);  // base of symbol table
 if(!(NAV(tv)->flag&(NMLOC|NMILOC))){  // don't store if we KNOW we won't be looking up in the local symbol table - and bucketx contains a hash/# for NMLOC/NMILOC
  I4 compcount=0;  // number of comparisons before match
  // tv is a simplename.  We will install the bucket/index fields
  // Get the bucket number by reproducing the calculation in the symbol-table routine
  I4 bucket=(I4)SYMHASH(NAV(tv)->hash,actstn);  // bucket number of name hash
  // search through the chain, looking for a match on name.  If we get a match, the bucket index is the one's complement
  // of the number of items compared before the match.  If we get no match, the bucket index is the number
  // of items compared (= the number of items in the chain)
  for(k=actstv[bucket];k;++compcount,k=sympv[k].next){  // k chases the chain of symbols in selected bucket
   if(NAV(tv)->m==NAV(sympv[k].name)->m&&!memcmpne(NAV(tv)->s,NAV(sympv[k].name)->s,NAV(tv)->m)){
    // match found.  this is a local name.  Replace it with the shared copy, flag as shared, set negative bucket#
    A oldtv=tv;
    *t=tv=sympv[k].name;  // use shared copy
    if(recur){ras(tv); fa(oldtv);} // if we are installing into a recursive box, increment/decr usecount new/old
    NAV(tv)->flag|=NMSHARED;  // tag the shared copy as shared
    // Remember the exact location of the symbol.  It will not move as long as this symbol table is alive.  We can
    // use it only when we are in this primary symbol table
    NAV(tv)->symx=k;  // keep index of the allocated symbol
    compcount=~compcount;  // negative bucket indicates found symbol
    break;
   }
  }
  NAV(tv)->bucket=bucket;  // fill in the bucket in the (possibly modified) name
  NAV(tv)->bucketx=compcount;
 }
 if(!dobuckets)NAV(tv)->bucket=0;  // remove bucket if this name not allowed to have them
 R tv;
}

EVERYFS(onmself,jtonm,0,0,VFLAGNONE)  // create self to pass into every

// create local-symbol table for a definition
//
// The goal is to save time allocating/deallocating the symbol table for a verb; and to
// save time in name lookups.  We scan the tokens, looking for assignments, and extract
// all names that are the target of local assignment (including fixed multiple assignments).
// We then allocate a symbol table sufficient to hold these values, and a symbol for each
// one.  These symbols are marked LPERMANENT.  When an LPERMANENT symbol is deleted, its value is
// cleared but the symbol remains.
// Then, all simplenames in the definition, whether assigned or not, are converted to bucket/index
// form, using the known size of the symbol table.  If the name was an assigned name, its index
// is given; otherwise the index tells how many names to skip before starting the name search.
//
// This symbol table is created with no values.  When the explicit definition is started, it
// is used; values are filled in as they are defined & removed at the end of execution (but the symbol-table
// entries for them are not removed).  If
// a definition is recursive, it will create a new symbol table, starting it off with the
// permanent entries from this one (with no values).  We create this table with rank 0, and we set
// the rank to 1 while it is in use, to signify that it must be cloned rather than used inplace.

// l is the A block for all the words/queues used in the definition
// c is the table of control-word info used in the definition
// type is the m operand to m : n, indicating part of speech to be produce
// dyad is 1 if this is the dyadic definition
// flags is the flag field for the verb we are creating; indicates whether uvmn are to be defined
//
// We save the symbol chain numbers for y/x in the AM field of the SYMB block
A jtcrelocalsyms(J jt, A l, A c,I type, I dyad, I flags){A actst,*lv,pfst,t,wds;C *s;I j,ln;
 // Allocate a pro-forma symbol table to hash the names into
 RZ(pfst=stcreate(2,40,0L,0L));
 // Do a probe-for-assignment for every name that is locally assigned in this definition.  This will
 // create a symbol-table entry for each such name
 // Start with the argument names.  We always assign y, and x EXCEPT when there is a monadic guaranteed-verb
 RZ(probeis(mnuvxynam[5],pfst));if(!(!dyad&&(type>=3||(flags&VXOPR)))){RZ(probeis(mnuvxynam[4],pfst));}
 if(type<3){RZ(probeis(mnuvxynam[2],pfst)); RZ(probeis(mnuvxynam[0],pfst));}
 if(type==2){RZ(probeis(mnuvxynam[3],pfst)); RZ(probeis(mnuvxynam[1],pfst));}
 // Go through the definition, looking for local assignment.  If the previous token is a simplename, add it
 // to the table.  If it is a literal constant, break it into words, convert each to a name, and process.
 ln=AN(l); lv=AAV(l);  // Get # words, address of first box
 for(j=1;j<ln;++j) {   // start at 1 because we look at previous word
  t=lv[j-1];  // t is the previous word
  // look for 'names' =./=: .  If found (and the names do not begin with `, replace the string with a special form: a list of boxes where each box contains a name.
  // This form can appear only in compiled definitions
  if(AT(lv[j])&ASGN&&AT(t)&LIT&&AN(t)&&CAV(t)[0]!=CGRAVE){
   A neww=words(t);
   if(AN(neww)){  // ignore blank string
    A newt=every(neww,(A)&onmself);  // convert every word to a NAME block
    if(newt){t=lv[j-1]=incorp(newt); AT(t)|=BOXMULTIASSIGN;}else RESETERR  // if no error, mark the block as MULTIASSIGN type and save it in the compiled definition; also set as t for below.  If error, catch it later
   }
  }

  if((AT(lv[j])&ASGN+ASGNLOCAL)==(ASGN+ASGNLOCAL)) {  // local assignment
   if(AT(lv[j])&ASGNTONAME){    // preceded by name?
    // Lookup the name, which will create the symbol-table entry for it
    RZ(probeis(t,pfst));
   } else if(AT(t)&LIT) {
    // LIT followed by =.  Probe each word.  Now that we support lists of NAMEs, this is used only for AR assignments
    // First, convert string to words
    s=CAV(t);   // s->1st character; remember if it is `
    if(wds=words(s[0]==CGRAVE?str(AN(t)-1,1+s):t)){I kk;  // convert to words (discarding leading ` if present)
     I wdsn=AN(wds); A *wdsv = AAV(wds), wnm;
     for(kk=0;kk<wdsn;++kk) {
      // Convert word to NAME; if local name, add to symbol table
      if((wnm=onm(wdsv[kk]))) {
       if(!(NAV(wnm)->flag&(NMLOC|NMILOC)))RZ(probeis(wnm,pfst));
      } else RESETERR
     }
    } else RESETERR  // if invalid words, ignore - we don't catch it here
   }else if((AT(t)&BOX+BOXMULTIASSIGN)==BOX+BOXMULTIASSIGN){  // not NAME, not LIT; is it NAMEs box?
    // the special form created above.  Add each non-global name to the symbol table
    A *tv=AAV(t); DO(AN(t), if(!(NAV(tv[i])->flag&(NMLOC|NMILOC)))RZ(probeis(tv[i],pfst));)
   }
  } // end 'local assignment'
 }  // for each word in sentence

 // Go through the control-word table, looking for for_xyz.  Add xyz and xyz_index to the local table too.
 I cn=AN(c); CW *cwv=(CW*)AV(c);  // Get # control words, address of first
 for(j=0;j<cn;++j) {   // look at each control word
  if(cwv[j].ig.indiv.type==CFOR){  // for.
   I cwlen = AN(lv[cwv[j].ig.indiv.sentx]);
   if(cwlen>4){  // for_xyz.
    // for_xyz. found.  Lookup xyz and xyz_index
    A xyzname = str(cwlen+1,CAV(lv[cwv[j].ig.indiv.sentx])+4);  // +1 is -5 for_. +6 _index
    RZ(probeis(nfs(cwlen-5,CAV(xyzname)),pfst));  // create xyz
    MC(CAV(xyzname)+cwlen-5,"_index",6L);    // append _index to name
    RZ(probeis(nfs(cwlen+1,CAV(xyzname)),pfst));  // create xyz_index
   }
  }
 }

 // Count the assigned names, and allocate a symbol table of the right size to hold them.  We won't worry too much about collisions, since we will be assigning indexes in the definition.
 // We choose the smallest feasible table to reduce the expense of clearing it at the end of executing the verb
 I pfstn=AN(pfst); LX*pfstv=LXAV0(pfst),pfx; I asgct=0; L *sympv=JT(jt,sympv);
 for(j=SYMLINFOSIZE;j<pfstn;++j){  // for each hashchain
  for(pfx=pfstv[j];pfx=SYMNEXT(pfx),pfx;pfx=sympv[pfx].next){++asgct;}  // chase the chain and count.  The chains have MSB flag, which must be removed
 }

 asgct = asgct + ((asgct+4)>>1); // leave 33% empty space + 2, since we will have resolved most names here
 RZ(actst=stcreate(2,asgct,0L,0L));  // Allocate the symbol table we will use
 *(UI4*)LXAV0(actst)=(UI4)((SYMHASH(NAV(mnuvxynam[4])->hash,AN(actst)-SYMLINFOSIZE)<<16)+SYMHASH(NAV(mnuvxynam[5])->hash,AN(actst)-SYMLINFOSIZE));  // get the yx bucket indexes for a table of this size, save in first hashchain

 // Transfer the symbols from the pro-forma table to the result table, hashing using the table size
 // For fast argument assignment, we insist that the arguments be the first symbols added to the table.
 // So we add them by hand - just y and possibly x.  They will be added later too
 RZ(probeis(ca(mnuvxynam[5]),actst));if(!(!dyad&&(type>=3||(flags&VXOPR)))){RZ(probeis(ca(mnuvxynam[4]),actst));}
 for(j=1;j<pfstn;++j){  // for each hashchain
  for(pfx=pfstv[j];pfx=SYMNEXT(pfx);pfx=JT(jt,sympv)[pfx].next){L *newsym;
   A nm=JT(jt,sympv)[pfx].name;
   // If we are transferring a PERMANENT name, we have to clone it, because the name may be local & if it is we may install bucket info or a symbol index
   if(ACISPERM(AC(nm)))RZ(nm=ca(nm));   // only cases are mnuvxy
   RZ(newsym=probeis(nm,actst));  // create new symbol (or possibly overwrite old argument name)
   newsym->flag = JT(jt,sympv)[pfx].flag|LPERMANENT;   // Mark as permanent
  }
 }
 I actstn=AN(actst); LX*actstv=LXAV0(actst);  // # hashchains in new symbol table, and pointer to hashchain table

 // Go through all the newly-created chains and clear the non-PERMANENT flag that was set in each root and next pointer.  This flag is set to
 // indicate that the symbol POINTED TO is non-permanent.
 sympv=JT(jt,sympv);  // refresh pointer to symbols
 for(j=1;j<actstn;++j){  // for each hashchain
  actstv[j]=SYMNEXT(actstv[j]); for(pfx=actstv[j];pfx;pfx=sympv[pfx].next)sympv[pfx].next=SYMNEXT(sympv[pfx].next);  // set PERMANENT for all symbols in the table
 }

 // Go back through the words of the definition, and add bucket/index information for each simplename, and cachability flag
 // If this definition might return a non-noun (i. e. is an adverb/conjunction not operating on xy) there is a problem.
 // In that case, the returned result might contain local names; but these names contain bucket information
 // and are valid only in conjunction with the symbol table for this definition.  To prevent the escape of
 // incorrect bucket information, don't have any (this is easier than trying to remove it from the returned
 // result).  The definition will still benefit from the preallocation of the symbol table.
 for(j=0;j<ln;++j) {
  if(AT(t=lv[j])&NAME) {
   t=jtcalclocalbuckets(jt,&lv[j],actstv,actstn-SYMLINFOSIZE,type>=3 || flags&VXOPR,0);  // install bucket info into name
   // if the name is not shared, it is not a simple local name.
   // If it is also not indirect, x., or u., it is eligible for caching - if that is enabled
   if(jt->namecaching && !(NAV(t)->flag&(NMILOC|NMDOT|NMIMPLOC|NMSHARED)))NAV(t)->flag|=NMCACHED;
  }else if((AT(t)&BOX+BOXMULTIASSIGN)==BOX+BOXMULTIASSIGN){
   A *tv=AAV(t); DO(AN(t), jtcalclocalbuckets(jt,&tv[i],actstv,actstn-SYMLINFOSIZE,type>=3 || flags&VXOPR,AFLAG(t)&BOX);)  // calculate details about the boxed names
  }
 }
 R actst;
}

// l is the A block for all the words/queues used in the definition
// c is the table of control-word info used in the definition
// type is the m operand to m : n, indicating part of speech to be produced
// We preparse what we can in the definition
static I pppp(J jt, A l, A c){I j; A fragbuf[20], *fragv=fragbuf; I fragl=sizeof(fragbuf)/sizeof(fragbuf[0]);
 // Go through the control-word table, looking at each sentence
 I cn=AN(c); CW *cwv=(CW*)AV(c);  // Get # control words, address of first
 A *lv=AAV(l);  // address of words in sentences
 for(j=0;j<cn;++j) {   // look at each control word
  if(((((I)1<<(BW-CBBLOCK-1))|((I)1<<(BW-CTBLOCK-1)))<<(cwv[j].ig.indiv.type&31))<0){  // BBLOCK or TBLOCK
   // scan the sentence for PPPP.  If found, parse the PPPP and replace the sequence in the sentence; reduce the length
   A *lvv=lv+cwv[j].ig.indiv.sentx;  // pointer to sentence words
   I startx=0, endx=cwv[j].ig.indiv.sentn;  // start and end+1 index of sentence
   // loop till we have found all the parens
   while(1){
    // Look forward for )
    while(startx<endx && !(AT(lvv[startx])&RPAR))++startx; if(startx==endx)break;  // find ), exit loop if none, finished
    // Scan backward looking for (, to get length, and checking for disqualifying entities
    I rparx=startx; // remember where the ) is
    while(--startx>=0 && !(AT(lvv[startx])==LPAR)){  // look for matching (   use = because LPAR can be a NAMELESS flag
     if(AT(lvv[startx])&RPAR+ASGN+NAME)break;  // =. not allowed; ) indicates previous disqualified block; NAME is unknowable
     if(AT(lvv[startx])&VERB && FAV(lvv[startx])->flag2&VF2IMPLOC)break;  // u. v. not allowed: they are half-names
     if(AT(lvv[startx])&CONJ && FAV(lvv[startx])->id==CIBEAM)break;  // !: not allowed: might produce adverb/conj to do who-knows-what
     if(AT(lvv[startx])&CONJ && FAV(lvv[startx])->id==CCOLON && !(AT(lvv[startx-1])&VERB))break;  // : allowed only in u : v form
    }
    if(startx>=0 && (AT(lvv[startx])==LPAR)){
     // The ) was matched and the () can be processed.
     // See if the () block was a (( )) block.  If it is, we will execute it even if it contains verbs
     I doublep = (rparx+1<endx) && (AT(lvv[rparx+1])==RPAR) && (startx>0) && (AT(lvv[startx-1])==LPAR);  // is (( ))?
     if(doublep){--startx, ++rparx;  // (( )), expand the look to include outer ()
     }else{
      // Not (( )).  We have to make sure no verbs in the fragment will be executed.  They might have side effects, such as increased space usage.
      // copy the fragment between () to a temp buffer, replacing any verb with [:
      if(fragl<rparx-startx-1){A fb; GATV0(fb,INT,rparx-startx-1,0) fragv=AAV0(fb); fragl=AN(fb);}  // if the fragment buffer isn't big enough, allocate a new one
      DO(rparx-startx-1, fragv[i]=AT(lvv[startx+i+1])&VERB?ds(CCAP):lvv[startx+i+1];)  // copy the fragment, not including (), with verbs replaced
      // parse the temp for error, which will usually be an attempt to execute a verb
      parsea(fragv,rparx-startx-1);
     }
     if(likely(jt->jerr==0)){
      // no error: parse the actual () block
      A pfrag; RZ(pfrag=parsea(&lvv[startx+1],rparx-startx-1)); INCORP(pfrag); AFLAGORLOCAL(pfrag,doublep<<AFDPARENX);  // if this came from (( )), mark such in the value
      // Replace the () block with its parse, close up the sentence, zero the ending area
      lvv[startx]=pfrag; DO(endx-(rparx+1), lvv[startx+1+i]=lvv[rparx+1+i];) DP(rparx-startx, lvv[endx+i]=0;) 
      // Adjust the end pointer and the ) position
      cwv[j].ig.indiv.sentn=endx-=rparx-startx; rparx=startx;  // back up to account for discarded tokens; resume as if the parse result was at ) position
     }else{RESETERR} // skipping because of error; clear error indic
    }
    // Whether we skipped or not, rpar now has the adjusted position of the ) and endx is correct relative to it.  Advance to next ) search
    startx=rparx+1;  // continue look after )
   }
  }
 }
 R 1;
}

// a is a local symbol table, possibly in use
// result is a copy of it, ready to use.  All PERMANENT symbols are copied over and given empty values, without inspecting any non-PERMANENT ones
// The rank-flag of the table is 'not modified'
// static A jtclonelocalsyms(J jt, A a){A z;I j;I an=AN(a); I *av=AV(a);I *zv;
A jtclonelocalsyms(J jt, A a){A z;I j;I an=AN(a); LX *av=LXAV0(a),*zv;
 RZ(z=stcreate(2,AN(a),0L,0L)); zv=LXAV0(z); AR(z)|=ARLCLONED;  // allocate the clone; zv->clone hashchains; set flag to indicate cloned
 // Copy the first hashchain, which has the x/v hashes
 zv[0]=av[0]; // Copy as LX; really it's a UI4
 // Go through each hashchain of the model, after the first one.  We know the non-PERMANENT flags are off
 for(j=SYMLINFOSIZE;j<an;++j) {LX *zhbase=&zv[j]; LX ahx=av[j]; LX ztx=0; // hbase->chain base, hx=index of current element, ztx is element to insert after
  while(SYMNEXTISPERM(ahx)) {L *l;  // for each permanent entry...
   RZ(l=symnew(zhbase,SYMNEXT(ztx)));   // append new symbol after tail (or head, if tail is empty), as PERMANENT
   *zhbase=SYMNEXT(*zhbase);  // zhbase points to the pointer to the entry we just added.  First time, that's the chain base
   A nm=(JT(jt,sympv))[ahx].name;
   l->name=nm; ra(l->name);  // point symbol table to the name block, and increment its use count accordingly
   l->flag=(JT(jt,sympv))[ahx].flag&(LINFO|LPERMANENT);  // Preserve only flags that persist
   ztx = ztx?(JT(jt,sympv))[ztx].next : *zhbase;  // ztx=index to value we just added.  We avoid address calculation because of the divide.  If we added
      // at head, the added block is the new head; otherwise it's pointed to by previous tail
   zhbase=&l->next;  // after the first time, zhbase is the chain field of the tail
   ahx = (JT(jt,sympv))[ahx].next;  // advance to next symbol
  }
 }
 R z;
}

F2(jtcolon){A d,h,*hv,m;C*s;I flag=VFLAGNONE,n,p;
 ARGCHK2(a,w);PROLOG(778);
 if(VERB&AT(a)){  // v : v case
  ASSERT(AT(w)&VERB,EVDOMAIN);   // v : noun is an error
  // If nested v : v, prune the tree
  if(CCOLON==FAV(a)->id&&FAV(a)->fgh[0]&&VERB&AT(FAV(a)->fgh[0])&&VERB&AT(FAV(a)->fgh[1]))a=FAV(a)->fgh[0];  // look for v : v; don't fail if fgh[0]==0 (namerefop).  Must test fgh[0] first
  if(CCOLON==FAV(w)->id&&FAV(w)->fgh[0]&&VERB&AT(FAV(w)->fgh[0])&&VERB&AT(FAV(w)->fgh[1]))w=FAV(w)->fgh[1];
  R fdef(0,CCOLON,VERB,xv1,xv2,a,w,0L,((FAV(a)->flag&FAV(w)->flag)&VASGSAFE),mr(a),lr(w),rr(w));  // derived verb is ASGSAFE if both parents are 
 }
 RE(n=i0(a));  // m : n; set n=value of a argument
 I col0;  // set if it was m : 0
 if(col0=equ(w,num(0))){RZ(w=colon0(n)); }   // if m : 0, read up to the ) .  If 0 : n, return the string unedited
 if(!n){ra0(w); RCA(w);}  // noun - return it.  Give it recursive usecount
 if((C2T+C4T)&AT(w))RZ(w=cvt(LIT,w));
 I splitloc=-1;   // will hold line number of : line
 if(10<n){ASSERT(AT(w)&LIT,EVDOMAIN) s=CAV(w); p=AN(w); if(p&&CLF==s[p-1])RZ(w=str(p-1,s));}  // if tacit form, discard trailing LF
 else{  // not tacit translator - preparse the body
  // we want to get all forms to a common one: a list of boxed strings.  If we went through m : 0, we are in that form
  // already.  Convert strings
  if(!col0)if(BOX&AT(w)){RZ(w=sent12b(w))}else{RZ(w=sent12c(w))}  // convert to list of boxes
  // If there is a control line )x at the top of the definition, parse it now and discard it from m
  if(likely(AN(w)!=0))if(unlikely(AN(AAV(w)[0])&&CAV(AAV(w)[0])[0]==')')){
   // there is a control line.  parse it.  Cut to words
   A cwds=wordil(AAV(w)[0]); RZ(cwds); ASSERT(AM(cwds)==2,EVDOMAIN);  // must be exactly 2 words: ) and type
   ASSERT(((IAV(cwds)[1]-IAV(cwds)[0])|(IAV(cwds)[3]-IAV(cwds)[2]))==1,EVDOMAIN);  // the ) and the next char must be 1-letter words  
   C ctltype=CAV(AAV(w)[0])[IAV(cwds)[2]];  // look at the second char, which must be one of acmdv*  (n is handled in ddtokens)
   I newn=-1; newn=ctltype=='a'?1:newn; newn=ctltype=='c'?2:newn; newn=ctltype=='m'?3:newn; newn=ctltype=='d'?4:newn; newn=ctltype=='v'?3:newn; newn=ctltype=='*'?9:newn;  // choose type based on char
   ASSERT(newn>=0,EVDOMAIN);  // error if invalid char
   n=newn;  // accept the type the user specified
   // discard the control line
   RZ(w=beheadW(w));
   // Noun DD
  }
  // find the location of the ':' divider line, if any.  But don't recognize : on the last line, since it could
  // conceivably be the return value from a modifier
  A *wv=AAV(w); DO(AN(w)-1, I st=0;
    DO(AN(*wv), I c=CAV(*wv)[i]; if(c!=':'&&c!=' '){st=0; break;} if(c!=' ')if(st==1){s=0; break;}else st=1;)
    if(st==1){splitloc=wv-AAV(w); break;} ++wv;)
  // split the definition into monad and dyad.
  I mn=splitloc<0?AN(w):splitloc; I nn=splitloc<0?0:AN(w)-splitloc-1;
  RZ(m=take(sc(mn),w)); RZ(d=take(sc(-nn),w));
  INCORP(m); INCORP(d);  // we are incorporating them into hv[]
  if(4==n){if((-AN(m)&(AN(d)-1))<0)d=m; m=mtv;}  //  for 4 :, make the single def given the dyadic one
  GAT0(h,BOX,2*HN,1); hv=AAV(h);
  if(n){B b;  // if not noun, audit the valences as valid sentences and convert to a queue to send into parse()
   RE(b=preparse(m,hv,hv+1)); if(b)flag|=VTRY1; hv[2]=JT(jt,retcomm)?m:mtv;
   RE(b=preparse(d,hv+HN,hv+HN+1)); if(b)flag|=VTRY2; hv[2+HN]=JT(jt,retcomm)?d:mtv;
  }
 }
 // The h argument is logically h[2][HN] where the boxes hold (parsed words, in a row);(info for each control word);(original commented text (optional));(local symbol table)
 // Non-noun results cannot become inputs to verbs, so we do not force them to be recursive
 if((1LL<<n)&0x206){  // types 1, 2, 9
  I fndflag=xop(hv[0])|xop(hv[0+HN]);   // 8=mu 4=nv 2=x 1=y, combined for both valences
  // for 9 : n, figure out best type after looking at n
  if(n==9){
   I defflg=(fndflag&((splitloc>>(BW-1))|-4))|1; CTLZI(defflg,n); n=(0x2143>>(n<<2))&0xf; // replace 9 by value depending on what was seen; if : seen, ignore x
   if(n==4){hv[HN]=hv[0]; hv[0]=mtv; hv[HN+1]=hv[1]; hv[1]=mtv; hv[HN+2]=hv[2]; hv[2]=mtv; flag=(flag&~VTRY2)+VTRY1; }  // if we created a dyadic verb, shift the monad over to the dyad and clear the monad, incl try flag
  } 
  if(n<=2){  // adv or conj after autodetection
    flag|=REPSGN(-(fndflag&3))&VXOPR;   // if this def refers to xy, set VXOPR
   // if there is only one valence defined, that will be the monad.  Swap it over to the dyad in two cases: (1) it is a non-operator conjunction: the operands will be the two verbs;
   // (2) it is an operator with a reference to x
   if(((-AN(m))&(AN(d)-1)&((SGNIFNOT(flag,VXOPRX)&(1-n))|(SGNIF(flag,VXOPRX)&SGNIF(fndflag,1))))<0){A*u=hv,*v=hv+HN,x; DQ(HN, x=*u; *u++=*v; *v++=x;);}  // if not, it executes on uv only; if conjunction, make the default the 'dyad' by swapping monad/dyad
   // for adv/conj, flag has operator status from here on
  }
 }
 flag|=VFIX;  // ensures that f. will not look inside n : n
 // Create a symbol table for the locals that are assigned in this definition.  It would be better to wait until the
 // definition is executed, so that we wouldn't take up the space for library verbs; but since we don't explicitly free
 // the components of the explicit def, we'd better do it now, so that the usecounts are all identical
 if(4>=n) {
  // explicit definitions.  Create local symbol table and pppp
  I hnofst=0; do{  // for each valence
   // Don't bother to create a symbol table for an empty definition, since it is a domain error
// obsolete   if(AN(hv[1]))RZ(hv[3] = incorp(crelocalsyms(hv[0],hv[1],n,0,flag)));  // wordss,cws,type,monad,flag
   if(AN(hv[hnofst+1])){
    RZ(hv[hnofst+3] = incorp(crelocalsyms(hv[hnofst+0], hv[hnofst+1],n,!!hnofst,flag)));  // words,cws,type,dyad,flag
    pppp(jt, hv[hnofst+0], hv[hnofst+1]);  // words,cws,type,dyad,flag
   }
  }while((hnofst+=HN)<=HN);
 }
 A z;
 switch(n){
 case 3:  z=fdef(0,CCOLON, VERB, xn1,jtxdefn,       num(n),0L,h, flag|VJTFLGOK1|VJTFLGOK2, RMAX,RMAX,RMAX); break;
 case 1:  z=fdef(0,CCOLON, ADV,  flag&VXOPR?xop1:xadv,0L,    num(n),0L,h, flag, RMAX,RMAX,RMAX); break;
 case 2:  z=fdef(0,CCOLON, CONJ, 0L,flag&VXOPR?jtxop2:jtxdefn, num(n),0L,h, flag, RMAX,RMAX,RMAX); break;
 case 4:  z=fdef(0,CCOLON, VERB, xn1,jtxdefn,       num(n),0L,h, flag|VJTFLGOK1|VJTFLGOK2, RMAX,RMAX,RMAX); break;
 case 13: z=vtrans(w); break;
 default: ASSERT(0,EVDOMAIN);
 }
 // EPILOG is called for because of the allocations we made, but it is essential to make sure the pfsts created during crelocalsyms get deleted.  They have symbols with no value
 // that will cause trouble if 18!:31 is executed before they are expunged.  As a nice side effect, all explicit definitions are recursive.
 EPILOG(z);
}

// input reader for direct definition
// This is a drop-in for tokens().  It takes a string and env, and returns tokens created by enqueue().  Can optionally return string
//
// Any DDs found are collected and converted into ( m : string ).  This is done recursively.  If an unterminated
// DD is found, we call jgets() to get the next line, taking as many lines as needed to leave with a valid line.
// The string for a DD will contain a trailing LF plus one LF for each LF found inside the DD.
//
// Bit 2 of env suppresses the call to jgets().  It will be set if it is known that there is no way to get more
// input, for example if the string comes from (". y) or from an event.  If an unterminated DD is found when bit 2 is set,
// the call fails with control error
//
// Bit 3 of env is set if the caller wants the returned value as a string rather than as enqueued words
//
// If the call to jgets() returns EOF, indicating end-of-script, that is also a control error
A jtddtokens(J jt,A w,I env){
// TODO: Use LF for DDSEP, support {{), make nouns work
 PROLOG(000);F1PREFIP;
 ARGCHK1(w);
 // find word boundaries, remember if last word is NB
 A wil; RZ(wil=wordil(w));  // get index to words
 C *wv=CAV(w); I nw=AS(wil)[0]; I (*wilv)[2]=voidAV(wil);  // cv=pointer to chars, nw=#words including final NB   wilv->[][2] array of indexes into wv word start/end
 // scan for start of DD/end of DD.
 I firstddbgnx;  // index of first/last start of DD, and end of DD
 I ddschbgnx=0; // place where we started looking for DD
 for(firstddbgnx=ddschbgnx;firstddbgnx<nw;++firstddbgnx){US ch2=*(US*)(wv+wilv[firstddbgnx][0]); ASSERT(!(ch2==DDEND&&(wilv[firstddbgnx][1]-wilv[firstddbgnx][0]==2)),EVCTRL) if(ch2==DDBGN&&(wilv[firstddbgnx][1]-wilv[firstddbgnx][0]==2))break; }
 if(firstddbgnx>=nw){ASSERT(AM(wil)>=0,EVOPENQ) R env&8?w:enqueue(wil,w,env&3);}    //   If no DD chars found, and caller wants a string, return w fast
 // loop till all DDs found
 while(firstddbgnx<nw){
  // We know that firstddbgnx is DDBGN
  // Move all the words before the DD into one long megaword - if there are any.  We mustn't disturb the DD itself
  // Close up after the megaword with empties
  I *fillv=&wilv[ddschbgnx][1]; DQ(2*(firstddbgnx-ddschbgnx)-1, *fillv++=wilv[firstddbgnx][0];)
  I ddendx=-1, ddbgnx=firstddbgnx;  // end/start of DD, indexes into wilv.  This will be the pair we process.  We advance ddbgnx and stop when we hit ddendx
  I scanstart=firstddbgnx;  // start looking for DDEND/nounDD at the first known DDBGN.  But if there turns out to be no DDEND, advance start ptr to avoid rescanning
  while(1){I i;  // loop till we find a complete DD
   for(i=scanstart;i<nw;++i){
    US ch2=*(US*)(wv+wilv[i][0]);  // digraph for next word
    if(ch2==DDEND&&(wilv[i][1]-wilv[i][0]==2)){ddendx=i; break;}  // if end, break, we can process
    if(ch2==DDBGN&&(wilv[i][1]-wilv[i][0]==2)){
     //  Nested DD found.  We have to go back and preserve the spacing for everything that precedes it
     ddbgnx=i;  // set new start pointer, when we find an end
     fillv=&wilv[firstddbgnx][1]; DQ(2*(ddbgnx-firstddbgnx)-1, *fillv++=wilv[ddbgnx][0];)
     if(AN(w)>=wilv[i][1]+2 && wv[wilv[i][0]+2]==')' && wv[wilv[i][0]+3]=='n'){  // is noun DD?
      // if the nested DD is a noun, break to process it immediately
      ddendx=0;  // use the impossible starting }} to signify noun DD
      break;  // go process the noun DD
     }
    }
   }  // find DD pair
   if(ddendx>=0)break;  // we found an end, and we started at a begin, so we have a pair, or a noun DD if endx=0.  Go process it

   // Here the current line ended with no end DD or noun DD.  We have to continue onto the next line
   ASSERT(AM(wil)>=0,EVOPENQ);  // if the line didn't contain noun DD, we have to give error if open quote
   ASSERT(!(env&4),EVCTRL);   // Abort if we are not allowed to continue (as for an event or ". y)
   scanstart=AM(wil);  // Get # words, not including final NB.  We have looked at em all, so start next look after all of them
   A neww=jgets("\001");  // fetch next line, in raw mode
   RE(0); ASSERT(neww!=0,EVCTRL); // fail if jgets failed, or if it returned EOF - problem either way
   // join the new line onto the end of the old one (after discarding trailing NB in the old).  Must add an LF character and a word for it
   w=jtapip(jtinplace,w,scc(DDSEP));   // append a separator, which is all that remains of the original line   scaf use faux or constant block
   jtinplace=(J)((I)jtinplace|JTINPLACEW);  // after the first one, we can certainly inplace on top of w
   I oldchn=AN(w);  // len after adding LF, before adding new line
   RZ(w=jtapip(jtinplace,w,neww));   // join the new character list onto the old, inplace if possible
   // Remove the trailing comment if any from wil, and add a word for the added LF
   makewritable(wil);  // We will modify the block rather than curtailing it
   AS(wil)[0]=scanstart; AN(wil)=2*scanstart;  // set AS and AN to discard comment
   A lfwd; fauxblockINT(lffaux,2,2); fauxINT(lfwd,lffaux,2,2)  AS(lfwd)[0]=2; AS(lfwd)[1]=1; IAV(lfwd)[0]=oldchn-1; IAV(lfwd)[1]=oldchn; 
   RZ(wil=jtapip(jtinplace,wil,lfwd));  // add a new word for added LF.  # words in wil is now scanstart+1
   ++scanstart;  // update count of words already examined so we start after the added LF word
   A newwil; RZ(newwil=wordil(neww));  // get index to new words
   I savam=AM(newwil);  // save AM for insertion in final new string block
   makewritable(newwil);  // We will modify the block rather than adding to it
   I *nwv=IAV(newwil); DO(AN(newwil), *nwv++ += oldchn;)  // relocate all the new words so that they point to chars after the added LF
   RZ(wil=jtapip(jtinplace,wil,newwil));   // add new words after LF, giving old LF new
   if(likely(savam>=0)){AM(wil)=savam+scanstart;}else{AM(wil)=0;}  // set total # words in combined list, not including final comment; remember if error scanning line
   wv=CAV(w); nw=AS(wil)[0]; wilv=voidAV(wil);  // refresh pointer to word indexes, and length
  }

  if(ddendx==0){
   // ******* NOUN DD **********
   // we found a noun DD at ddbgnx: process it, replacing it with its string.  Look for delimiters on the first line, or at the start of a later line
   I nounstart=wilv[ddbgnx][0]+4, wn=AN(w);  // the start of the DD string, the length of the string
   I enddelimx=wn;  // will be character index of the ending }}, not found if >=wn-1, first line empty if =wn
   // characters after nounstart cannot be examined as words, since they might have mismatched quotes.  Scan them for }}
   if(nounstart<wn){  // {{)n at EOL does not add LF
    for(enddelimx=nounstart;enddelimx<wn-1;++enddelimx)if(wv[enddelimx]==(DDEND&0xff) && wv[enddelimx+1]==(DDEND>>8))break;
   }
   if(enddelimx>=wn-1){
    ASSERT(!(env&4),EVCTRL);   // Abort if we are not allowed to consume a new line (as for an event or ". y)
    // if a delimiter was not found on the first line, consume lines until we hit a delimiter
    while(1){
     // append the LF for the previous line (unless an empty first line), then the new line itself, to w
     if(enddelimx<wn){
      w=jtapip(jtinplace,w,scc(DDSEP));   // append a separator   scaf use faux or constant block
      jtinplace=(J)((I)jtinplace|JTINPLACEW);  // after the first one, we can certainly inplace on top of w
     }
     enddelimx=0;   // after the first line, we always install the LF
     A neww=jgets("\001");  // fetch next line, in raw mode
     RE(0); ASSERT(neww!=0,EVCTRL); // fail if jgets failed, or if it returned EOF - problem either way
     // join the new line onto the end of the old one
     I oldchn=AN(w);  // len after adding LF, before adding new line
     RZ(w=jtapip(jtinplace,w,neww));   // join the new character list onto the old, inplace if possible
     jtinplace=(J)((I)jtinplace|JTINPLACEW);  // after the first one, we can certainly inplace on top of w
     wv=CAV(w);   // refresh data pointer.  Number of words has not changed, nor have indexes
     // see if the new line starts with the delimiter - if so, we're done looking
     if(AN(w)>=oldchn+2 && wv[oldchn]==(DDEND&0xff) && wv[oldchn+1]==(DDEND>>8)){enddelimx=oldchn; break;}
    }
   }
   // We have found the end delimiter, which starts at enddelimx.  We reconstitute the input line: we convert the noun DD to a quoted string
   // and append the unprocessed part of the last line.  For safety, we put spaces around the string.  We rescan the combined line without
   // trying to save the scan pointer, since the case is rare
   A remnant; RZ(remnant=str(AN(w)-enddelimx-2,CAV(w)+enddelimx+2));  // get a string for the preserved tail of w
   AS(wil)[0]=ddbgnx; RZ(w=unwordil(wil,w,0));  // take everything up to the {{)n - it may have been put out of order
   A spacea; RZ(spacea=scc(' ')); RZ(w=apip(w,spacea));  // put space before quoted string
   RZ(w=apip(w,strq(enddelimx-nounstart,wv+nounstart)));  // append quoted string
   RZ(w=apip(w,spacea));  // put space after quoted string
   RZ(w=apip(w,remnant));  // install unprocessed chars of original line
   // line is ready.  Process it from the beginning
   RZ(wil=wordil(w));  // get index to words
   wv=CAV(w); nw=AS(wil)[0]; wilv=voidAV(wil);  // cv=pointer to chars, nw=#words including final NB   wilv->[][2] array of indexes into wv word start/end
   ddschbgnx=0;  // start scan back at the beginning
  }else{
   // ********* NORMAL DD *******
   // We have found an innermost non-noun DD, running from ddbgnx to ddendx.  Convert it to ( 9 : 'string' ) form
   // convert all the chars of the DD to a quoted string block
   // if the first line is empty (meaning the first word is a linefeed by itself), skip it
   I firstcharx=wilv[ddbgnx+1][0]; if(wv[firstcharx]=='\n')++firstcharx;  // just one skipped line
   A ddqu; RZ(ddqu=strq(wilv[ddendx][0]-firstcharx,wv+firstcharx));
   // append the string for the start/end of DD
   I bodystart=AN(w), bodylen=AN(ddqu), trailstart=wilv[ddendx][1];  // start/len of body in w, and start of after-DD text
   RZ(ddqu=jtapip(jtinplace,ddqu,str(8,") ( 9 : ")));
   // append the new stuff to w
   RZ(w=jtapip(jtinplace,w,ddqu));
   wv=CAV(w);   // refresh data pointer.  Number of words has not changed, nor have indexes
   // Replace ddbgnx and ddendx with the start/end strings.  Fill in the middle, if any, with everything in between
   wilv[ddbgnx][0]=AN(w)-6; wilv[ddbgnx][1]=AN(w);  //  ( 9 :  
   wilv[ddbgnx+1][0]=bodystart; fillv=&wilv[ddbgnx+1][1]; DQ(2*(ddendx-ddbgnx)-1, *fillv++=bodystart+bodylen+2;)  // everything in between, and trailing )SP
   // continue the search.
   if(ddbgnx==firstddbgnx){ddschbgnx=ddendx+1;  //   If this was not a nested DD, we can simply pick up the search after ddendx.
   }else{
    // Nested DD.  The characters below ddendx are out of order and there will be trouble if we try to preserve
    // the user's spacing by grouping them.  We must run the ending lines together, to save their spacing, and then
    // refresh w and wil to get the characters in order
    if(++ddendx<nw){wilv[ddendx][0]=trailstart; wilv[ddendx][1]=bodystart; AN(wil)=2*(AS(wil)[0]=ddendx+1);}  // make one string of DDEND to end of string
    RZ(w=unwordil(wil,w,0)); RZ(wil=wordil(w));  // run chars in order; get index to words
    wv=CAV(w); nw=AS(wil)[0]; wilv=voidAV(wil);  // cv=pointer to chars, nw=#words including final NB   wilv->[][2] array of indexes into wv word start/end
    ddschbgnx=0;  // start scan back at the beginning
   }
  }

  // We have replaced one DD with its equivalent explicit definition.  Rescan the line, starting at the first location where DDBGN was seen
  for(firstddbgnx=ddschbgnx;firstddbgnx<nw;++firstddbgnx){US ch2=*(US*)(wv+wilv[firstddbgnx][0]); ASSERT(!(ch2==DDEND&&(wilv[firstddbgnx][1]-wilv[firstddbgnx][0]==2)),EVCTRL) if(ch2==DDBGN&&(wilv[firstddbgnx][1]-wilv[firstddbgnx][0]==2))break; }
 }
 ASSERT(AM(wil)>=0,EVOPENQ);  // we have to make sure there is not an unscannable remnant at the end of the last line
 // All DDs replaced. convert word list back to either text form or enqueued form
 // We searched starting with ddschbgnx looking for DDBGN, and din't find one.  Now we need to lump
 // the words together, since the spacing may be necessary
 if(ddschbgnx<nw){wilv[ddschbgnx][1]=wilv[nw-1][1]; AN(wil)=2*(AS(wil)[0]=ddschbgnx+1);}  // make one word of the last part

 w=unwordil(wil,w,0);  // the word list is where everything is.  Collect to string
 if(!(env&8))w=tokens(w,env&3);  // enqueue if called for
 EPILOG(w);
}
