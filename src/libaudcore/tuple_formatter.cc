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

#include "tuple.h"
#include "tuple_compiler.h"

/*
 * the tuple formatter:
 *
 * this is a data-driven meta-language.
 *
 * language constructs follow the following basic rules:
 *   - begin with ${
 *   - end with }
 *
 * language constructs:
 *   - ${field}: prints a field
 *   - ${?field:expr}: evaluates expr if field exists
 *   - ${==field,field:expr}: evaluates expr if both fields are the same
 *   - ${!=field,field:expr}: evaluates expr if both fields are not the same
 *   - ${(empty)?field:expr}: evaluates expr if field does not exist
 *
 * everything else is treated as raw text.
 */

struct TupleFormatter {
    TupleCompiler context;
};

EXPORT TupleFormatter * tuple_formatter_new (const char * format)
{
    TupleFormatter * formatter = new TupleFormatter;
    formatter->context.compile (format);
    return formatter;
}

EXPORT void tuple_formatter_free (TupleFormatter * formatter)
{
    delete formatter;
}

EXPORT String tuple_format_title (TupleFormatter * formatter, const Tuple & tuple)
{
    StringBuf str = formatter->context.evaluate (tuple);

    if (str[0])
        return String (str);

    /* formatting failed, try fallbacks */
    static const int fallbacks[] = {FIELD_TITLE, FIELD_FILE_NAME};

    for (unsigned i = 0; i < ARRAY_LEN (fallbacks); i ++)
    {
        String title = tuple.get_str (fallbacks[i]);
        if (title)
            return title;
    }

    return String ("");
}
