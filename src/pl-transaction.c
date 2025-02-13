/*  Part of SWI-Prolog

    Author:        Jan Wielemaker
    E-mail:        J.Wielemaker@vu.nl
    WWW:           http://www.swi-prolog.org
    Copyright (c)  2020, University of Amsterdam
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
#include "pl-transaction.h"
#include "pl-event.h"
#include "pl-dbref.h"
#include "pl-tabling.h"
#include "pl-proc.h"
#include "pl-util.h"
#include "pl-pro.h"
#include "pl-wam.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This module implements _transactions_, notably _isolating_ transactions.
This reuses most of the visibility stuff we need anyway for the _logical
updates semantics_ that  dictate  that  a   running  goal  on  a dynamic
predicate does not see changes to the predicate.

Transactions take this a step further:

  - The generation space 1..GEN_TRANSACTION_BASE is used for _globally_
    visible data.
  - The remainder is separated in 2^31 segments of 2^32 generations, as
    defined by GEN_TRANSACTION_SIZE (2^32).
  - A thread starting a transaction reserves as space starting at
    GEN_TRANSACTION_BASE+TID*GEN_TRANSACTION_SIZE and sets its
    LD->transaction.generation to the base.
  - setGenerationFrame() sets the frame generation to the transaction
    (if active) or the global generation.

Clause updates are handles as follows inside a transaction:

  - The clause is added to the LD->transaction.clauses table, where
    the value is GEN_ASSERTA or GEN_ASSERTZ or the local offset of the
    deleted generation (gen-LD->transaction.gen_base). - If the clause
    was retracted, clause.tr_erased_no is incremented.

A clause is visible iff

  - Its it is visible in the frame.generation.  In a transaction, this
    means it is created inside the transaction.
  - If we are in a transaction (gen >= GEN_TRANSACTION_BASE) and the
    clause was visible in the transaction start generation *and* the
    clause was not retracted in this transaction, it is visible.  The
    latter is the case if `clause.tr_erased_no > 0`, the clause is in
    our LD->transaction.clauses table at a generation smaller than the
    current.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/* Avoid conflict with HTABLE_TOMBSTONE and HTABLE_SENTINEL */
#define GEN_ASSERTA		((uintptr_t)(void*)-3)
#define GEN_ASSERTZ		((uintptr_t)(void*)-4)
#define GEN_NESTED_RETRACT	((uintptr_t)(void*)-5)

#define IS_ASSERT_GEN(g) ((g)==GEN_ASSERTA||(g)==GEN_ASSERTZ)

typedef struct tr_stack
{ struct tr_stack  *parent;		/* parent transaction */
  gen_t		    gen_nest;		/* Saved nesting generation */
  gen_t             generation;		/* Parent generation */
  Table		    clauses;		/* Parent changed clauses */
  struct tbl_trail *table_trail;	/* Parent changes to tables */
  term_t	    id;			/* Parent goal */
  unsigned int	    flags;		/* TR_* flags */
} tr_stack;

static void
tr_free_clause_symbol(void *k, void *v)
{ Clause cl = k;
  (void)v;

  release_clause(cl);
}

static Table
tr_clause_table(ARG1_LD)
{ Table t;

  if ( !(t=LD->transaction.clauses) )
  { t = newHTable(16);
    t->free_symbol = tr_free_clause_symbol;
    LD->transaction.clauses = t;
  }

  return t;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This distinguishes three cases.

  - If we retract a globally visible clause we may need to commit and we
    need to know the generation at which we retracted the clause.

  - If the clause was added in an outer transaction we can update
    the generation and put it into our table such that we can undo
    the generation update (discard) or retract the clause (commit)

  - If we added the clause ourselves, we no longer have to commit it.
    Its visibility is guaranteed by the created and erased generations,
    so there is no need to keep it in our tables.

    (*) This is not true.  Commit/discard need to move the generations
    out of the transaction or later transactions will see this clause.
    We cannot do that now as the clause may be visible from a choice
    point in the transaction.  What to do?  I see two options:

    - Make clause CG set a flag and trigger us, so we can check the
      table and removed GCed clauses from it.
    - Scan here to see whether the clause is involved in a choicepoint.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
transaction_retract_clause(Clause clause ARG_LD)
{ if ( clause->generation.created < LD->transaction.gen_base )
  { uintptr_t lgen = ( next_generation(clause->predicate PASS_LD) -
		       LD->transaction.gen_base
		     );

    acquire_clause(clause);
    ATOMIC_INC(&clause->tr_erased_no);
    addHTable(tr_clause_table(PASS_LD1), clause, (void*)lgen);

    return TRUE;
  } else if ( clause->generation.created <= LD->transaction.gen_nest )
  { gen_t egen = next_generation(clause->predicate PASS_LD);
    if ( !egen )
      return PL_representation_error("transaction_generations"),-1;
    clause->generation.erased = egen;
    acquire_clause(clause);
    addHTable(tr_clause_table(PASS_LD1), clause, (void*)GEN_NESTED_RETRACT);

    return TRUE;
#if 0					/* see (*) */
  } else if ( LD->transaction.clauses )
  { deleteHTable(LD->transaction.clauses, clause);
#endif
  }
  DEBUG(MSG_TRANSACTION,
	Sdprintf("Deleting locally asserted clause for %s %s..%s\n",
		 predicateName(clause->predicate),
		 generationName(clause->generation.created),
		 generationName(clause->generation.erased)));

  return FALSE;
}

int
transaction_assert_clause(Clause clause, ClauseRef where ARG_LD)
{ uintptr_t lgen = (where == CL_START ? GEN_ASSERTA : GEN_ASSERTZ);

  acquire_clause(clause);
  addHTable(tr_clause_table(PASS_LD1), clause, (void*)lgen);

  return TRUE;
}


int
transaction_visible_clause(Clause cl, gen_t gen ARG_LD)
{ if ( cl->generation.created <= LD->transaction.gen_start &&
       cl->generation.erased   > LD->transaction.gen_start )
  { intptr_t lgen;

    if ( cl->tr_erased_no && LD->transaction.clauses &&
	 (lgen = (intptr_t)lookupHTable(LD->transaction.clauses, cl)) &&
	 !IS_ASSERT_GEN(lgen) )
    { if ( lgen+LD->transaction.gen_base <= gen )
	return FALSE;
    }

    return TRUE;
  }

  return FALSE;
}


static int
transaction_commit(ARG1_LD)
{ if ( LD->transaction.clauses )
  { gen_t gen_commit;

    PL_LOCK(L_GENERATION);
    gen_commit = global_generation()+1;

    for_table(LD->transaction.clauses, n, v,
	      { Clause cl = n;
		uintptr_t lgen = (uintptr_t)v;

		if ( IS_ASSERT_GEN(lgen) )
		{ if ( false(cl, CL_ERASED) )
		  { cl->generation.erased  = GEN_MAX;
		    MEMORY_RELEASE();
		    cl->generation.created = gen_commit;
		    DEBUG(MSG_COMMIT,
			Sdprintf("Commit added clause %p for %s\n",
				 cl, predicateName(cl->predicate)));
		  } else
		  { DEBUG(MSG_COMMIT,
			  Sdprintf("Discarded in-transaction clause %p for %s\n",
				   cl, predicateName(cl->predicate)));
		    cl->generation.erased  = 2;
		    MEMORY_RELEASE();
		    cl->generation.created = 2;
		  }
		} else if ( lgen == GEN_NESTED_RETRACT )
		{ retract_clause(cl, gen_commit PASS_LD);
		} else
		{ DEBUG(MSG_COMMIT,
			Sdprintf("Commit erased clause %p for %s\n",
				 cl, predicateName(cl->predicate)));
		  ATOMIC_DEC(&cl->tr_erased_no);
		  retract_clause(cl, gen_commit PASS_LD);
		}
	      });
    MEMORY_RELEASE();
    GD->_generation = gen_commit;
    PL_UNLOCK(L_GENERATION);

    destroyHTable(LD->transaction.clauses);
    LD->transaction.clauses = NULL;
  }

  return TRUE;
}

static int
transaction_discard(ARG1_LD)
{ int rc = TRUE;

  if ( LD->transaction.clauses )
  { for_table(LD->transaction.clauses, n, v,
	      { Clause cl = n;
		Definition def = cl->predicate;
		uintptr_t lgen = (uintptr_t)v;
		atom_t action = 0;

		if ( IS_ASSERT_GEN(lgen) )
		{ if ( false(cl, CL_ERASED) )
		  { cl->generation.erased  = 3;
		    MEMORY_RELEASE();
		    cl->generation.created = 3;
		    retract_clause(cl, cl->generation.created PASS_LD);
		    DEBUG(MSG_COMMIT,
			  Sdprintf("Discarded asserted clause %p for %s\n",
				   cl, predicateName(cl->predicate)));
		    action = (lgen==GEN_ASSERTA ? ATOM_asserta : ATOM_assertz);
		  } else
		  { cl->generation.erased  = 4;
		    MEMORY_RELEASE();
		    cl->generation.created = 4;
		    DEBUG(MSG_COMMIT,
			  Sdprintf("Discarded asserted&retracted "
				   "clause %p for %s\n",
				   cl, predicateName(cl->predicate)));
		  }
		} else if ( lgen == GEN_NESTED_RETRACT )
		{ cl->generation.erased = LD->transaction.gen_max;
		  action = ATOM_retract;
		} else
		{ ATOMIC_DEC(&cl->tr_erased_no);
		  action = ATOM_retract;
		}

		if ( def && def->events && action &&
		     !(LD->transaction.flags&TR_BULK) )
		{ if ( !predicate_update_event(def, action, cl,
					       P_EVENT_ROLLBACK PASS_LD) )
		    rc = FALSE;
		}
	      });
    destroyHTable(LD->transaction.clauses);
    LD->transaction.clauses = NULL;
  }

  return rc;
}

static void
merge_tables(Table into, Table from)
{ for_table(from, n, v,
	    { Clause cl = n;

	      acquire_clause(cl);
	      addHTable(into, n, v);
	    });
}

		 /*******************************
		 *	      UPDATES		*
		 *******************************/

typedef struct tr_update
{ Clause    clause;
  functor_t update;
} tr_update;

static gen_t
update_gen(const tr_update *u)
{ return u->update == FUNCTOR_erased1 ?
		u->clause->generation.erased :
		u->clause->generation.created;
}

static int
cmp_updates(const void *ptr1, const void *ptr2)
{ const tr_update *u1 = ptr1;
  const tr_update *u2 = ptr2;
  gen_t g1 = update_gen(u1);
  gen_t g2 = update_gen(u2);

  return g1 < g2 ? -1 : g1 > g2 ? 1 : 0;
}


static int
is_trie_clause(const Clause cl)
{ return ( cl->predicate == GD->procedures.trie_gen_compiled3->definition ||
	   cl->predicate == GD->procedures.trie_gen_compiled2->definition );
}


static int
transaction_updates(Buffer b ARG_LD)
{ if ( LD->transaction.clauses )
  { for_table(LD->transaction.clauses, n, v,
	      { Clause cl = n;
		uintptr_t lgen = (uintptr_t)v;

		if ( IS_ASSERT_GEN(lgen) )
		{ if ( false(cl, CL_ERASED) )
		  { tr_update u;
		    u.clause = cl;
		    if ( lgen == GEN_ASSERTA )
		      u.update = FUNCTOR_asserta1;
		    else
		      u.update = FUNCTOR_assertz1;
		    addBuffer(b, u, tr_update);
		  }
		} else if ( !is_trie_clause(cl) )
		{ tr_update u;
		  u.clause = cl;
		  u.update = FUNCTOR_erased1;
		  addBuffer(b, u, tr_update);
		}
	      });
    qsort(baseBuffer(b, void),
	  entriesBuffer(b, tr_update), sizeof(tr_update),
	  cmp_updates);
  }

  return TRUE;
}


static int
announce_updates(Buffer updates ARG_LD)
{ tr_update *u, *e;

  u = baseBuffer(updates, tr_update);
  e = topBuffer(updates, tr_update);

  for(; u<e; u++)
  { Definition def = u->clause->predicate;

    if ( !predicate_update_event(def, nameFunctor(u->update), u->clause, 0 PASS_LD) )
      return FALSE;
  }

  return TRUE;
}


		 /*******************************
		 *	PROLOG CONNECTION	*
		 *******************************/

static int
transaction(term_t goal, term_t constraint, term_t lock, int flags ARG_LD)
{ int rc;
  buffer updates;

  initBuffer(&updates);

  if ( LD->transaction.generation )
  { tr_stack parent = { .generation  = LD->transaction.generation,
			.gen_nest    = LD->transaction.gen_nest,
			.clauses     = LD->transaction.clauses,
			.table_trail = LD->transaction.table_trail,
			.parent      = LD->transaction.stack,
			.id          = LD->transaction.id,
			.flags       = LD->transaction.flags
		      };

    LD->transaction.clauses     = NULL;
    LD->transaction.id          = goal;
    LD->transaction.stack       = &parent;
    LD->transaction.gen_nest    = LD->transaction.generation;
    LD->transaction.flags       = flags;
    LD->transaction.table_trail = NULL;
    rc = callProlog(NULL, goal, PL_Q_PASS_EXCEPTION, NULL);
    if ( rc && constraint )
      rc = callProlog(NULL, constraint, PL_Q_PASS_EXCEPTION, NULL);
    if ( rc && (flags&TR_TRANSACTION) )
    { if ( LD->transaction.clauses )
      { if ( parent.clauses )
	{ if ( (flags&TR_BULK) )
	  { transaction_updates(&updates PASS_LD);
	    if ( !announce_updates(&updates PASS_LD) )
	      goto nested_discard;
	  }
	  merge_tables(parent.clauses, LD->transaction.clauses);
	  destroyHTable(LD->transaction.clauses);
	} else
	{ parent.clauses = LD->transaction.clauses;
	}
	if ( parent.table_trail )
	{ merge_tabling_trail(parent.table_trail, LD->transaction.table_trail);
	} else
	{ parent.table_trail = LD->transaction.table_trail;
	}
      }
    } else
    { nested_discard:
      rc = transaction_discard(PASS_LD1) && rc;
      rc = transaction_rollback_tables(PASS_LD1) && rc;
      LD->transaction.generation = parent.generation;
    }
    LD->transaction.gen_nest    = parent.gen_nest;
    LD->transaction.clauses     = parent.clauses;
    LD->transaction.table_trail = parent.table_trail;
    LD->transaction.stack       = parent.parent;
    LD->transaction.id          = parent.id;
    LD->transaction.flags	= parent.flags;
  } else
  { int tid = PL_thread_self();
#ifdef O_PLMT
    pl_mutex *mutex = NULL;
    if ( lock && !get_mutex(lock, &mutex, TRUE) )
      return FALSE;
#define TR_LOCK() PL_mutex_lock(mutex)
#define TR_UNLOCK() PL_mutex_unlock(mutex)
#else
#define TR_LOCK() (void)0
#define TR_UNLOCK() (void)0
#endif

    LD->transaction.gen_start  = global_generation();
    LD->transaction.gen_base   = GEN_TRANSACTION_BASE + tid*GEN_TRANSACTION_SIZE;
    LD->transaction.gen_max    = LD->transaction.gen_base+GEN_TRANSACTION_SIZE-6;
    LD->transaction.generation = LD->transaction.gen_base;
    LD->transaction.id         = goal;
    rc = callProlog(NULL, goal, PL_Q_PASS_EXCEPTION, NULL);
    if ( rc && (flags&TR_TRANSACTION) )
    { if ( constraint )
      { TR_LOCK();
	LD->transaction.gen_start = global_generation();
	rc = callProlog(NULL, constraint, PL_Q_PASS_EXCEPTION, NULL);
      }
      if ( rc && (flags&TR_BULK) )
      { transaction_updates(&updates PASS_LD);
	rc = announce_updates(&updates PASS_LD);
      }
      if ( rc )
      { rc = ( transaction_commit_tables(PASS_LD1) &&
	       transaction_commit(PASS_LD1) );
	if ( constraint ) TR_UNLOCK();
      } else
      { if ( constraint ) TR_UNLOCK();
	transaction_discard(PASS_LD1);
	transaction_rollback_tables(PASS_LD1);
      }
    } else
    { rc = transaction_discard(PASS_LD1) && rc;
      rc = transaction_rollback_tables(PASS_LD1) && rc;
    }
    LD->transaction.id         = 0;
    LD->transaction.generation = 0;
    LD->transaction.gen_max    = 0;
    LD->transaction.gen_base   = GEN_INFINITE;
    LD->transaction.gen_start  = 0;
  }

  if ( (flags&TR_BULK) )
    discardBuffer(&updates);

  return rc;
}

static const opt_spec transaction_options[] =
{ { ATOM_bulk,		 OPT_BOOL },
  { NULL_ATOM,		 0 }
};

static
PRED_IMPL("$transaction", 2, transaction, PL_FA_TRANSPARENT)
{ PRED_LD
  int flags = TR_TRANSACTION;
  int bulk = FALSE;

  if ( !scan_options(A2, 0,
		     ATOM_transaction_option, transaction_options,
		     &bulk) )
    return FALSE;
  if ( bulk )
    flags |= TR_BULK;

  return transaction(A1, 0, 0, TR_TRANSACTION PASS_LD);
}

static
PRED_IMPL("$transaction", 3, transaction, PL_FA_TRANSPARENT)
{ PRED_LD

  return transaction(A1, A2, A3, TR_TRANSACTION PASS_LD);
}

static
PRED_IMPL("$snapshot", 1, snapshot, PL_FA_TRANSPARENT)
{ PRED_LD

  return transaction(A1, 0, 0, TR_SNAPSHOT PASS_LD);
}

static
PRED_IMPL("current_transaction", 1, current_transaction, PL_FA_NONDETERMINISTIC)
{ PRED_LD
  tr_stack *stack;
  term_t id0, id;
  Module m0 = contextModule(environment_frame);

  switch( CTX_CNTRL )
  { case FRG_FIRST_CALL:
    { if ( !LD->transaction.id )
	return FALSE;
      id0 = LD->transaction.id;
      stack = LD->transaction.stack;
      break;
    }
    case FRG_REDO:
    { stack = CTX_PTR;
      id0 = stack->id;
      break;
    }
    default:
      return TRUE;
  }

  id = PL_new_term_ref();
  Mark(fli_context->mark);
  for(;;)
  { Module m = NULL;
    int rc;

    if ( !PL_strip_module(id0, &m, id) )
      return FALSE;
    if ( m == m0 )
      rc = PL_unify(A1, id);
    else
      rc = PL_unify(A1, id0);

    if ( rc )
    { if ( stack )
	ForeignRedoPtr(stack);
      else
	return TRUE;
    }
    Undo(fli_context->mark);

    if ( stack )
    { id0 = stack->id;
      stack = stack->parent;
    } else
      return FALSE;
  }
}

static int
add_update(Clause cl, functor_t action,
	   term_t tail, term_t head, term_t tmp ARG_LD)
{ return ( PL_put_clref(tmp, cl) &&
	   PL_cons_functor(tmp, action, tmp) &&
	   PL_unify_list(tail, head, tail) &&
	   PL_unify(head, tmp)
	 );
}

static
PRED_IMPL("transaction_updates", 1, transaction_updates, 0)
{ PRED_LD

  if ( !LD->transaction.generation )
    return FALSE;			/* error? */

  if ( LD->transaction.clauses )
  { tmp_buffer buf;
    tr_update *u, *e;
    term_t tail = PL_copy_term_ref(A1);
    term_t head = PL_new_term_ref();
    term_t tmp  = PL_new_term_ref();
    int rc = TRUE;

    initBuffer(&buf);
    transaction_updates((Buffer)&buf PASS_LD);
    u = baseBuffer(&buf, tr_update);
    e = topBuffer(&buf, tr_update);

    for(; u<e; u++)
    { if ( !add_update(u->clause, u->update, tail, head, tmp PASS_LD) )
      { rc = FALSE;
	break;
      }
    }
    discardBuffer(&buf);

    return rc && PL_unify_nil(tail);
  } else
  { return PL_unify_nil(A1);
  }
}

#ifdef O_DEBUG
static
PRED_IMPL("pred_generations", 1, pred_generations, PL_FA_TRANSPARENT)
{ PRED_LD
  Procedure proc;

  if ( get_procedure(A1, &proc, 0, GP_NAMEARITY) )
  { Definition def = getProcDefinition(proc);
    size_t count = 0;
    gen_t gen = current_generation(def PASS_LD);
    ClauseRef c;

    Sdprintf("Clauses for %s at %s (gen_start = %s)\n",
	     predicateName(def), generationName(gen),
	     generationName(LD->transaction.gen_start));

    count = 0;
    acquire_def(def);
    for(c = def->impl.clauses.first_clause; c; c = c->next)
    { Clause cl = c->value.clause;

      Sdprintf("  %s %p %s..%s%s\n",
	       visibleClause(cl, gen) ? "V" : "i",
	       cl,
	       generationName(cl->generation.created),
	       generationName(cl->generation.erased),
	       true(cl, CL_ERASED) ? " (erased)" : "");

      if ( visibleClause(cl, gen) )
        count++;
    }
    release_def(def);

    return TRUE;
  }

  return FALSE;
}
#endif


#define META PL_FA_TRANSPARENT
#define NDET PL_FA_NONDETERMINISTIC

BeginPredDefs(transaction)
  PRED_DEF("$transaction",        2, transaction,         META)
  PRED_DEF("$transaction",        3, transaction,         META)
  PRED_DEF("$snapshot",           1, snapshot,            META)
  PRED_DEF("current_transaction", 1, current_transaction, NDET|META)
  PRED_DEF("transaction_updates", 1, transaction_updates, 0)
#ifdef O_DEBUG
  PRED_DEF("pred_generations",    1, pred_generations,    META)
#endif
EndPredDefs

