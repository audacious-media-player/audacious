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

#ifndef LIBAUDCORE_TUPLE_COMPILER_H
#define LIBAUDCORE_TUPLE_COMPILER_H

#include <glib.h>
#include <libaudcore/tuple.h>

struct _TupleEvalNode;
typedef struct _TupleEvalNode TupleEvalNode;

struct _TupleEvalContext;
typedef struct _TupleEvalContext TupleEvalContext;

TupleEvalContext * tuple_evalctx_new(void);
void tuple_evalctx_reset(TupleEvalContext *ctx);
void tuple_evalctx_free(TupleEvalContext *ctx);

void tuple_evalnode_free(TupleEvalNode *expr);

TupleEvalNode *tuple_formatter_compile(TupleEvalContext *ctx, char *expr);
void tuple_formatter_eval (TupleEvalContext * ctx, TupleEvalNode * expr,
 const Tuple * tuple, GString * out);

#endif /* LIBAUDCORE_TUPLE_COMPILER_H */
