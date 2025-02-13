/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  1985-2021, University of Amsterdam
                              VU University Amsterdam
			      CWI, Amsterdam
			      SWI-Prolog Solutions b.v.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#include "pl-incl.h"

#ifndef _PL_GC_H
#define _PL_GC_H

		 /*******************************
		 *    FUNCTION DECLARATIONS	*
		 *******************************/

int		considerGarbageCollect(Stack s);
void		call_tune_gc_hook(void);
int		garbageCollect(gc_reason_t reason);
word		pl_garbage_collect(term_t d);
gc_stat *	last_gc_stats(gc_stats *stats);
Word		findGRef(int n);
size_t		nextStackSizeAbove(size_t n);
int		shiftTightStacks(void);
int		growStacks(size_t l, size_t g, size_t t);
size_t		nextStackSize(Stack s, size_t minfree);
int		makeMoreStackSpace(int overflow, int flags);
int		f_ensureStackSpace__LD(size_t gcells, size_t tcells,
				       int flags ARG_LD);
int		growLocalSpace__LD(size_t bytes, int flags ARG_LD);
void		clearUninitialisedVarsFrame(LocalFrame, Code);
void		clearLocalVariablesFrame(LocalFrame fr);
void		setLTopInBody(void);
word		check_foreign(void);	/* DEBUG(CHK_SECURE...) stuff */
void		markAtomsOnStacks(PL_local_data_t *ld, void *ctx);
void		markPredicatesInEnvironments(PL_local_data_t *ld,
					     void *ctx);
QueryFrame	queryOfFrame(LocalFrame fr);
void		mark_active_environment(struct bit_vector *active,
					LocalFrame fr, Code PC);
void		unmark_stacks(PL_local_data_t *ld,
			      LocalFrame fr, Choice ch, uintptr_t mask);
#if defined(O_DEBUG) || defined(SECURE_GC) || defined(O_MAINTENANCE)
word		checkStacks(void *vm_state);
bool		scan_global(int marked);
char *		print_addr(Word p, char *buf);
char *		print_val(word w, char *buf);
#endif

		 /*******************************
		 *	LD-USING FUNCTIONS	*
		 *******************************/

#define ensureLocalSpace(n)	likely(ensureLocalSpace__LD(n PASS_LD))
#define ensureGlobalSpace(n,f)  likely(ensureStackSpace__LD(n,0,f PASS_LD))
#define ensureTrailSpace(n)     likely(ensureStackSpace__LD(0,n,ALLOW_GC PASS_LD))
#define ensureStackSpace(g,t)   likely(ensureStackSpace__LD(g,t,ALLOW_GC PASS_LD))

		 /*******************************
		 *	INLINE DEFINITIONS	*
		 *******************************/

static inline int
ensureLocalSpace__LD(size_t bytes ARG_LD)
{ int rc;

  if ( likely(addPointer(lTop, bytes) <= (void*)lMax) )
    return TRUE;

  if ( (rc=growLocalSpace__LD(bytes, ALLOW_SHIFT PASS_LD)) == TRUE )
    return TRUE;

  return raiseStackOverflow(rc);
}

static inline int
ensureStackSpace__LD(size_t gcells, size_t tcells, int flags ARG_LD)
{ gcells += BIND_GLOBAL_SPACE;
  tcells += BIND_TRAIL_SPACE;

  if ( likely(gTop+gcells <= gMax) && likely(tTop+tcells <= tMax) )
    return TRUE;

  return f_ensureStackSpace__LD(gcells, tcells, flags PASS_LD);
}

#endif /*_PL_GC_H*/