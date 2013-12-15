/*
 * tuple_formatter.c
 * Copyright (c) 2007-2013 William Pitcock and John Lindgren
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

#include "tuple.h"
#include "tuple_compiler.h"

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
 */

struct _TupleFormatter
{
    TupleEvalContext * context;
    TupleEvalNode * node;
    GString * buf;
};

EXPORT TupleFormatter * tuple_formatter_new (const char * format)
{
    TupleFormatter * formatter = g_slice_new (TupleFormatter);

    formatter->context = tuple_evalctx_new ();
    formatter->node = tuple_formatter_compile (formatter->context, format);
    formatter->buf = g_string_sized_new (255);

    return formatter;
}

EXPORT void tuple_formatter_free (TupleFormatter * formatter)
{
    tuple_evalctx_free (formatter->context);
    tuple_evalnode_free (formatter->node);
    g_string_free (formatter->buf, TRUE);

    g_slice_free (TupleFormatter, formatter);
}

EXPORT char * tuple_format_title (TupleFormatter * formatter, const Tuple * tuple)
{
    tuple_formatter_eval (formatter->context, formatter->node, tuple, formatter->buf);
    tuple_evalctx_reset (formatter->context);

    if (formatter->buf->len)
        return str_get (formatter->buf->str);

    /* formatting failed, try fallbacks */
    static const int fallbacks[] = {FIELD_TITLE, FIELD_FILE_NAME};

    for (int i = 0; i < ARRAY_LEN (fallbacks); i ++)
    {
        char * title = tuple_get_str (tuple, fallbacks[i]);
        if (title)
            return title;
    }

    return str_get ("");
}
