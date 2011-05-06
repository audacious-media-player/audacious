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

#ifndef AUDACIOUS_TUPLE_FORMATTER_H
#define AUDACIOUS_TUPLE_FORMATTER_H

#include <glib.h>
#include <mowgli.h>

#include "tuple.h"

G_BEGIN_DECLS

gchar * tuple_formatter_process_string (const Tuple * tuple, const gchar *
 string);
gchar * tuple_formatter_make_title_string (const Tuple * tuple, const gchar *
 string);
void tuple_formatter_register_expression(const gchar *keyword,
        gboolean (*func)(Tuple *tuple, const gchar *argument));
void tuple_formatter_register_function(const gchar *keyword,
        gchar *(*func)(Tuple *tuple, gchar **argument));
gchar *tuple_formatter_process_expr(Tuple *tuple, const gchar *expression,
    const gchar *argument);
gchar *tuple_formatter_process_function(Tuple *tuple, const gchar *expression,
    const gchar *argument);
gchar *tuple_formatter_process_construct(Tuple *tuple, const gchar *string);

G_END_DECLS

#endif /* AUDACIOUS_TUPLE_FORMATTER_H */
