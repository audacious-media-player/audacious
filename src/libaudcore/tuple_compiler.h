/*
 * tuple_compiler.h
 * Copyright (c) 2007 Matti 'ccr' Hämäläinen
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef LIBAUDCORE_TUPLE_COMPILER_H
#define LIBAUDCORE_TUPLE_COMPILER_H

#include <glib.h>
#include <libaudcore/tuple.h>

typedef GArray TupleEvalContext;
typedef struct _TupleEvalNode TupleEvalNode;

TupleEvalContext * tuple_evalctx_new(void);
void tuple_evalctx_reset(TupleEvalContext *ctx);
void tuple_evalctx_free(TupleEvalContext *ctx);

void tuple_evalnode_free(TupleEvalNode *expr);

TupleEvalNode *tuple_formatter_compile(TupleEvalContext *ctx, const char *expr);
void tuple_formatter_eval (TupleEvalContext * ctx, TupleEvalNode * expr,
 const Tuple * tuple, GString * out);

#endif /* LIBAUDCORE_TUPLE_COMPILER_H */
