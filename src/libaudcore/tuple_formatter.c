/*
 * Audacious
 * Copyright (c) 2007 William Pitcock
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
