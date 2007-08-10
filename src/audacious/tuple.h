/*
 * Audacious
 * Copyright (c) 2006-2007 Audacious team
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

#ifndef __AUDACIOUS_TUPLE_H__
#define __AUDACIOUS_TUPLE_H__

#include <glib.h>
#include <mowgli.h>

struct _Tuple;
typedef struct _Tuple Tuple;

typedef enum {
    TUPLE_STRING,
    TUPLE_INT,
    TUPLE_UNKNOWN
} TupleValueType;

Tuple *tuple_new(void);
Tuple *tuple_new_from_filename(gchar *filename);
gboolean tuple_associate_string(Tuple *tuple, const gchar *field, const gchar *string);
gboolean tuple_associate_int(Tuple *tuple, const gchar *field, gint integer);
void tuple_disassociate(Tuple *tuple, const gchar *field);
TupleValueType tuple_get_value_type(Tuple *tuple, const gchar *field);
const gchar *tuple_get_string(Tuple *tuple, const gchar *field);
int tuple_get_int(Tuple *tuple, const gchar *field);

#endif
