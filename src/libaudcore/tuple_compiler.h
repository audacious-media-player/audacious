/*
 * Audacious - Tuplez compiler
 * Copyright (c) 2007 Matti 'ccr' Hämäläinen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */
#ifndef AUDACIOUS_TUPLE_COMPILER_H
#define AUDACIOUS_TUPLE_COMPILER_H

#include <glib.h>
#include <mowgli.h>
#include "tuple.h"

G_BEGIN_DECLS

#define TUPLEZ_MAX_VARS (4)


enum {
    OP_RAW = 0,		/* plain text */
    OP_FIELD,		/* a field/variable */
    OP_EXISTS,
    OP_DEF_STRING,
    OP_DEF_INT,
    OP_EQUALS,
    OP_NOT_EQUALS,
    OP_GT,
    OP_GTEQ,
    OP_LT,
    OP_LTEQ,
    OP_IS_EMPTY,

    OP_FUNCTION,	/* function */
    OP_EXPRESSION	/* additional registered expressions */
};


enum {
    TUPLE_VAR_FIELD = 0,
    TUPLE_VAR_CONST,
    TUPLE_VAR_DEF
};


/* Caching structure for deterministic functions
 */
typedef struct {
    gchar *name;
    gboolean istemp;		/* Scope of variable - TRUE = temporary */
    gint type;			/* Type of variable, see VAR_* */
    gchar *defvals;		/* Defined value ${=foo,bar} */
    gint defvali;
    TupleValueType ctype;	/* Type of constant/def value */

    gint fieldidx;		/* if >= 0: Index # of "pre-defined" Tuple fields */
    TupleValue *fieldref;	/* Cached tuple field ref */
} TupleEvalVar;


typedef struct {
    gchar *name;
    gboolean isdeterministic;
    gchar *(*func)(Tuple *tuple, TupleEvalVar **argument);
} TupleEvalFunc;


typedef struct _TupleEvalNode {
    gint opcode;		/* operator, see OP_ enums */
    gint var[TUPLEZ_MAX_VARS];	/* tuple / global variable references */
    gboolean global[TUPLEZ_MAX_VARS];
    gchar *text;		/* raw text, if any (OP_RAW) */
    gint function, expression;	/* for OP_FUNCTION and OP_EXPRESSION */
    struct _TupleEvalNode *children, *next, *prev; /* children of this struct, and pointer to next node. */
} TupleEvalNode;


typedef struct {
    gint nvariables, nfunctions, nexpressions;
    TupleEvalVar **variables;
    TupleEvalFunc **functions;

    /* Error context */
    gboolean iserror;
    gchar *errmsg;
} TupleEvalContext;


TupleEvalContext * tuple_evalctx_new(void);
void tuple_evalctx_reset(TupleEvalContext *ctx);
void tuple_evalctx_free(TupleEvalContext *ctx);
gint tuple_evalctx_add_var(TupleEvalContext *ctx, const gchar *name, const gboolean istemp, const gint type, const TupleValueType ctype);

void tuple_evalnode_free(TupleEvalNode *expr);

gint tuple_formatter_print(FILE *f, gint *level, TupleEvalContext *ctx, TupleEvalNode *expr);
TupleEvalNode *tuple_formatter_compile(TupleEvalContext *ctx, gchar *expr);
gchar * tuple_formatter_eval (TupleEvalContext * ctx, TupleEvalNode * expr,
 const Tuple * tuple);


G_END_DECLS

#endif /* AUDACIOUS_TUPLE_COMPILER_H */
