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
#include <mowgli.h>

#include "tuple.h"
#include "tuple_formatter.h"

int
test_run(int argc, const char *argv[])
{
    Tuple *tuple;
    gchar *tstr;

    tuple = tuple_new();
    tuple_associate_string(tuple, "splork", "moo");
    tuple_associate_int(tuple, "splorkerz", 42);

    tstr = tuple_formatter_process_string(tuple, "${splork} ${splorkerz}");
    if (g_ascii_strcasecmp(tstr, "moo 42"))
    {
        g_print("fail 1: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${?fizz:${splork}} ${splorkerz}");
    if (g_ascii_strcasecmp(tstr, " 42"))
    {
        g_print("fail 2: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${?splork:${splork}} ${splorkerz}");
    if (g_ascii_strcasecmp(tstr, "moo 42"))
    {
        g_print("fail 3: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${==splork,splork:fields given matched}");
    if (g_ascii_strcasecmp(tstr, "fields given matched"))
    {
        g_print("fail 4: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${==splork,splork:${splork}}");
    if (g_ascii_strcasecmp(tstr, "moo"))
    {
        g_print("fail 5: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${!=splork,splorkerz:fields did not match}");
    if (g_ascii_strcasecmp(tstr, "fields did not match"))
    {
        g_print("fail 6: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${!=splork,splorkerz:${splorkerz}}");
    if (g_ascii_strcasecmp(tstr, "42"))
    {
        g_print("fail 7: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${!=splork,splork:${splorkerz}}");
    if (g_ascii_strcasecmp(tstr, ""))
    {
        g_print("fail 8: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${(empty)?splorky:${splorkerz}}");
    if (g_ascii_strcasecmp(tstr, "42"))
    {
        g_print("fail 9: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    mowgli_object_unref(tuple);

    return EXIT_SUCCESS;
}
