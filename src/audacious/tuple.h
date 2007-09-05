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


enum {
    FIELD_ARTIST = 0,
    FIELD_TITLE,
    FIELD_ALBUM,
    FIELD_COMMENT,
    FIELD_GENRE,

    FIELD_TRACK,
    FIELD_TRACK_NUMBER,
    FIELD_LENGTH,
    FIELD_YEAR,
    FIELD_QUALITY,

    FIELD_CODEC,
    FIELD_FILE_NAME,
    FIELD_FILE_PATH,
    FIELD_FILE_EXT,
    FIELD_SONG_ARTIST,

    FIELD_MTIME,
    FIELD_FORMATTER,
    FIELD_PERFORMER,
    FIELD_COPYRIGHT,
    FIELD_DATE,

    /* special field, must always be last */
    FIELD_LAST
};

extern const gchar *tuple_fields[FIELD_LAST];

typedef enum {
    TUPLE_STRING,
    TUPLE_INT,
    TUPLE_UNKNOWN
} TupleValueType;

typedef struct {
    TupleValueType type;
    union {
        gchar *string;
        gint integer;
    } value;
} TupleValue;

typedef struct _Tuple {
    mowgli_object_t parent;
    mowgli_dictionary_t *dict;
    TupleValue *values[FIELD_LAST];
} Tuple;


Tuple *tuple_new(void);
Tuple *tuple_new_from_filename(const gchar *filename);
gboolean tuple_associate_string(Tuple *tuple, const gint nfield, const gchar *field, const gchar *string);
gboolean tuple_associate_int(Tuple *tuple, const gint nfield, const gchar *field, gint integer);
void tuple_disassociate(Tuple *tuple, const gint nfield, const gchar *field);
void tuple_disassociate_now(TupleValue *value);
TupleValueType tuple_get_value_type(Tuple *tuple, const gint nfield, const gchar *field);
const gchar *tuple_get_string(Tuple *tuple, const gint nfield, const gchar *field);
gint tuple_get_int(Tuple *tuple, const gint nfield, const gchar *field);
#define tuple_free(x) mowgli_object_unref(x);

#endif
