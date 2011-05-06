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

#include <libaudcore/tuple.h>
#include <libaudcore/tuple_formatter.h>

static gboolean
test_functor(Tuple *tuple, const char *expr)
{
    return TRUE;
}

int
test_run(int argc, const char *argv[])
{
    Tuple *tuple;
    gchar *tstr;

    tuple_formatter_register_expression("(true)", test_functor);

    tuple = tuple_new();
    tuple_associate_string(tuple, FIELD_ARTIST, "splork", "moo");

    tstr = tuple_formatter_process_string(tuple, "${(true):${splork}}");
    if (g_ascii_strcasecmp(tstr, "moo"))
    {
        g_print("fail 1: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "%{audacious-version}");
    if (g_str_has_prefix(tstr, "audacious") == FALSE)
    {
        g_print("fail 2: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    tstr = tuple_formatter_process_string(tuple, "${(true):%{audacious-version}}");
    if (g_str_has_prefix(tstr, "audacious") == FALSE)
    {
        g_print("fail 3: '%s'\n", tstr);
        return EXIT_FAILURE;
    }
    g_free(tstr);

    mowgli_object_unref(tuple);

    return EXIT_SUCCESS;
}
