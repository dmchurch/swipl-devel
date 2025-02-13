/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  1985-2020, University of Amsterdam
                              VU University Amsterdam
			      CWI, Amsterdam
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

#ifndef _PL_PROC_H
#define _PL_PROC_H

		 /*******************************
		 *    FUNCTION DECLARATIONS	*
		 *******************************/

Procedure	lookupProcedure(functor_t f, Module m) WUNUSED;
void		unallocProcedure(Procedure proc);
Procedure	isCurrentProcedure__LD(functor_t f, Module m ARG_LD);
int		importDefinitionModule(Module m,
				       Definition def, int flags);
Procedure	lookupProcedureToDefine(functor_t def, Module m);
ClauseRef	hasClausesDefinition(Definition def);
Procedure	getDefinitionProc(Definition def);
bool		isDefinedProcedure(Procedure proc);
void		shareDefinition(Definition def);
int		unshareDefinition(Definition def);
void		lingerDefinition(Definition def);
int		get_head_functor(term_t head, functor_t *fdef,
				 int flags ARG_LD);
int		get_functor(term_t descr, functor_t *fdef,
			    Module *m, term_t h, int how);
int		get_procedure(term_t descr, Procedure *proc,
			      term_t he, int f);
int		checkModifySystemProc(functor_t f);
int		overruleImportedProcedure(Procedure proc, Module target);
word		pl_current_predicate(term_t name, term_t functor, control_t h);
void		clear_meta_declaration(Definition def);
void		setMetapredicateMask(Definition def, arg_info *args);
int		isTransparentMetamask(Definition def, arg_info *args);
ClauseRef	assertDefinition(Definition def, Clause clause,
				 ClauseRef where ARG_LD);
ClauseRef	assertProcedure(Procedure proc, Clause clause,
				ClauseRef where ARG_LD);
bool		abolishProcedure(Procedure proc, Module module);
int		retract_clause(Clause clause, gen_t gen ARG_LD);
bool		retractClauseDefinition(Definition def, Clause clause,
					int notify);
void		unallocClause(Clause c);
void		freeClause(Clause c);
void		lingerClauseRef(ClauseRef c);
void		acquire_clause(Clause cl);
void		release_clause(Clause cl);
ClauseRef	newClauseRef(Clause cl, word key);
size_t		removeClausesPredicate(Definition def,
				       int sfindex, int fromfile);
void		reconsultFinalizePredicate(sf_reload *rl, Definition def,
					   p_reload *r ARG_LD);
void		destroyDefinition(Definition def);
Procedure	resolveProcedure__LD(functor_t f, Module module ARG_LD);
Definition	trapUndefined(Definition undef ARG_LD);
word		pl_abolish(term_t atom, term_t arity);
word		pl_abolish1(term_t pred);
int		redefineProcedure(Procedure proc, SourceFile sf,
				  unsigned int suppress);
word		pl_index(term_t pred);
Definition	autoImport(functor_t f, Module m);
word		pl_require(term_t pred);
word		pl_check_definition(term_t spec);
foreign_t	pl_list_generations(term_t desc);
foreign_t	pl_check_procedure(term_t desc);
void		checkDefinition(Definition def);
Procedure	isStaticSystemProcedure(functor_t fd);
foreign_t	pl_garbage_collect_clauses(void);
int		setDynamicDefinition(Definition def, bool isdyn);
int		setThreadLocalDefinition(Definition def, bool isdyn);
int		setAttrDefinition(Definition def, unsigned attr, int val);
int		PL_meta_predicate(predicate_t def, const char*);
void		ddi_add_access_gen(DirtyDefInfo ddi, gen_t access);
int		ddi_contains_gen(DirtyDefInfo ddi, gen_t access);
int		ddi_is_garbage(DirtyDefInfo ddi,
			       gen_t start, Buffer tr_starts,
			       Clause cl);
size_t		sizeof_predicate(Definition def);

		 /*******************************
		 *	LD-USING FUNCTIONS	*
		 *******************************/

#define isCurrentProcedure(f,m) isCurrentProcedure__LD(f, m PASS_LD)
#define resolveProcedure(f,m)	resolveProcedure__LD(f, m PASS_LD)

		 /*******************************
		 *	INLINE DEFINITIONS	*
		 *******************************/

static inline Definition lookupDefinition(functor_t f, Module m) WUNUSED;
static inline Definition
lookupDefinition(functor_t f, Module m)
{ Procedure proc = lookupProcedure(f, m);

  return proc ? proc->definition : NULL;
}



#endif /*_PL_PROC_H*/