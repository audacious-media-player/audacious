/*
 * tuple_formatter.c
 * Copyright (c) 2007 William Pitcock
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

#include <glib.h>
#include <pthread.h>
#include <string.h>

#include "tuple_compiler.h"
#include "tuple_formatter.h"

/*
 * the tuple formatter:
 *
 * this is a data-driven meta-language which eventually hopes to be
 * turing complete.
 *
 * language constructs follow the following basic rules:
 *   - begin with ${
 *   - end with }
 *
 * language constructs:
 *   - ${field}: prints a field
 *   - ${?field:expr}: evaluates expr if field exists
 *   - ${=field,"value"}: defines field in the currently iterated
 *                        tuple as string value of "value"
 *   - ${=field,value}: defines field in the currently iterated
 *                      tuple as integer value of "value"
 *   - ${==field,field:expr}: evaluates expr if both fields are the same
 *   - ${!=field,field:expr}: evaluates expr if both fields are not the same
 *   - ${(empty)?field:expr}: evaluates expr if field is empty or does not exist
 *   - %{function:args,arg2,...}: runs function and inserts the result.
 *
 * everything else is treated as raw text.
 * additionally, plugins can add additional instructions and functions!
 */

/*
 * Compile a tuplez string and cache the result.
 * This caches the result for the last string, so that
 * successive calls are sped up.
 */

char * tuple_formatter_process_string (const Tuple * tuple, const char * string)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock (& mutex);

    static char *last_string = NULL;
    static TupleEvalContext *last_ctx = NULL;
    static TupleEvalNode *last_ev = NULL;

    if (! last_string || strcmp (string, last_string))
    {
        g_free(last_string);

        if (last_ctx != NULL)
        {
            tuple_evalctx_free(last_ctx);
            tuple_evalnode_free(last_ev);
        }

        last_ctx = tuple_evalctx_new();
        last_string = g_strdup(string);
        last_ev = tuple_formatter_compile(last_ctx, last_string);
    }

    static GString * buf;
    if (! buf)
        buf = g_string_sized_new (255);

    tuple_formatter_eval (last_ctx, last_ev, tuple, buf);
    tuple_evalctx_reset (last_ctx);

    char * result = str_get (buf->str);

    pthread_mutex_unlock (& mutex);
    return result;
}
