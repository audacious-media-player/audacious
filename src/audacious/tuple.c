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

#include <glib.h>
#include <mowgli.h>

#include "tuple.h"

const TupleBasicType tuple_fields[FIELD_LAST] = {
    { "artist",         TUPLE_STRING },
    { "title",          TUPLE_STRING },
    { "album",          TUPLE_STRING },
    { "comment",        TUPLE_STRING },
    { "genre",          TUPLE_STRING },

    { "track",          TUPLE_STRING },
    { "track-number",   TUPLE_INT },
    { "length",         TUPLE_INT },
    { "year",           TUPLE_INT },
    { "quality",	TUPLE_STRING },

    { "codec",          TUPLE_STRING },
    { "file-name",      TUPLE_STRING },
    { "file-path",      TUPLE_STRING },
    { "file-ext",       TUPLE_STRING },
    { "song-artist",    TUPLE_STRING },

    { "mtime",          TUPLE_INT },
    { "formatter",      TUPLE_STRING },
    { "performer",      TUPLE_STRING },
    { "copyright",      TUPLE_STRING },
    { "date",           TUPLE_STRING },
};


static mowgli_heap_t *tuple_heap = NULL;
static mowgli_heap_t *tuple_value_heap = NULL;
static mowgli_object_class_t tuple_klass;

/* iterative destructor of tuple values. */
static void
tuple_value_destroy(mowgli_dictionary_elem_t *delem, gpointer privdata)
{
    TupleValue *value = (TupleValue *) delem->data;

    if (value->type == TUPLE_STRING) {
        g_free(value->value.string);
    }

    mowgli_heap_free(tuple_value_heap, value);
}

static void
tuple_destroy(gpointer data)
{
    Tuple *tuple = (Tuple *) data;

    mowgli_dictionary_destroy(tuple->dict, tuple_value_destroy, NULL);
    mowgli_heap_free(tuple_heap, tuple);
}

Tuple *
tuple_new(void)
{
    Tuple *tuple;

    if (tuple_heap == NULL)
    {
        tuple_heap = mowgli_heap_create(sizeof(Tuple), 32, BH_NOW);
        tuple_value_heap = mowgli_heap_create(sizeof(TupleValue), 512, BH_NOW);
        mowgli_object_class_init(&tuple_klass, "audacious.tuple", tuple_destroy, FALSE);
    }

    /* FIXME: use mowgli_object_bless_from_class() in mowgli 0.4
       when it is released --nenolod */
    tuple = mowgli_heap_alloc(tuple_heap);
    memset(tuple, 0, sizeof(Tuple));
    mowgli_object_init(mowgli_object(tuple), NULL, &tuple_klass, NULL);

    tuple->dict = mowgli_dictionary_create(g_ascii_strcasecmp);

    return tuple;
}

Tuple *
tuple_new_from_filename(const gchar *filename)
{
    gchar *scratch, *ext, *realfn;
    Tuple *tuple;

    g_return_val_if_fail(filename != NULL, NULL);

    tuple = tuple_new();
    
    g_return_val_if_fail(tuple != NULL, NULL);

    realfn = g_filename_from_uri(filename, NULL, NULL);

    scratch = g_path_get_basename(realfn ? realfn : filename);
    tuple_associate_string(tuple, FIELD_FILE_NAME, NULL, scratch);
    g_free(scratch);

    scratch = g_path_get_dirname(realfn ? realfn : filename);
    tuple_associate_string(tuple, FIELD_FILE_PATH, NULL, scratch);
    g_free(scratch);

    g_free(realfn);

    ext = strrchr(filename, '.');
    if (ext != NULL) {
        ++ext;
        tuple_associate_string(tuple, FIELD_FILE_EXT, NULL, scratch);
    }

    return tuple;
}        

static TupleValue *
tuple_associate_data(Tuple *tuple, const gint cnfield, const gchar *field, TupleValueType ftype)
{
    const gchar *tfield = field;
    gint nfield = cnfield;
    TupleValue *value = NULL;
    
    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(cnfield < FIELD_LAST, NULL);

    /* Check for known fields */
    if (nfield < 0) {
        gint i;
        for (i = 0; i < FIELD_LAST && nfield < 0; i++)
            if (!strcmp(field, tuple_fields[i].name))
                nfield = i;
        
        if (nfield >= 0) {
            fprintf(stderr, "WARNING! FIELD_* not used for '%s'!\n", field);
        }
    }

    /* Check if field was known */
    if (nfield >= 0) {
        tfield = tuple_fields[nfield].name;
        value = tuple->values[nfield];
        
        if (ftype != tuple_fields[nfield].type) {
            /* FIXME! Convert values perhaps .. or not? */
            fprintf(stderr, "Invalid type for [%s](%d->%d), %d != %d\n", tfield, cnfield, nfield, ftype, tuple_fields[nfield].type);
            //mowgli_throw_exception_val(audacious.tuple.invalid_type_request, 0);
            return NULL;
        }
    } else {
        value = mowgli_dictionary_retrieve(tuple->dict, tfield);
    }

    if (value != NULL) {
        /* Value exists, just delete old associated data */
        if (value->type == TUPLE_STRING) {
            g_free(value->value.string);
            value->value.string = NULL;
        }
    } else {
        /* Allocate a new value */
        value = mowgli_heap_alloc(tuple_value_heap);
        value->type = ftype;
        mowgli_dictionary_add(tuple->dict, tfield, value);
        if (nfield >= 0)
            tuple->values[nfield] = value;
    }
    
    return value;
}

gboolean
tuple_associate_string(Tuple *tuple, const gint nfield, const gchar *field, const gchar *string)
{
    TupleValue *value;

    if ((value = tuple_associate_data(tuple, nfield, field, TUPLE_STRING)) == NULL)
        return FALSE;

    if (string == NULL)
        value->value.string = NULL;
    else
        value->value.string = g_strdup(string);

    return TRUE;
}

gboolean
tuple_associate_int(Tuple *tuple, const gint nfield, const gchar *field, gint integer)
{
    TupleValue *value;
    
    if ((value = tuple_associate_data(tuple, nfield, field, TUPLE_INT)) == NULL)
        return FALSE;

    value->value.integer = integer;
    return TRUE;
}

void
tuple_disassociate(Tuple *tuple, const gint nfield, const gchar *field)
{
    TupleValue *value;
    const gchar *tfield;

    g_return_if_fail(tuple != NULL);
    g_return_if_fail(nfield < FIELD_LAST);

    if (nfield < 0)
        tfield = field;
    else {
        tfield = tuple_fields[nfield].name;
        tuple->values[nfield] = NULL;
    }

    /* why _delete()? because _delete() returns the dictnode's data on success */
    if ((value = mowgli_dictionary_delete(tuple->dict, tfield)) == NULL)
        return;
    
    /* Free associated data */
    if (value->type == TUPLE_STRING) {
        g_free(value->value.string);
        value->value.string = NULL;
    }

    mowgli_heap_free(tuple_value_heap, value);
}

TupleValueType
tuple_get_value_type(Tuple *tuple, const gint nfield, const gchar *field)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, TUPLE_UNKNOWN);
    g_return_val_if_fail(nfield < FIELD_LAST, TUPLE_UNKNOWN);
    
    if (nfield < 0) {
        if ((value = mowgli_dictionary_retrieve(tuple->dict, field)) != NULL)
            return value->type;
    } else {
        if (tuple->values[nfield])
            return tuple->values[nfield]->type;
    }

    return TUPLE_UNKNOWN;
}

const gchar *
tuple_get_string(Tuple *tuple, const gint nfield, const gchar *field)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(nfield < FIELD_LAST, NULL);

    if (nfield < 0) {
        if ((value = mowgli_dictionary_retrieve(tuple->dict, field)) == NULL)
            return NULL;
    } else {
        if (tuple->values[nfield])
            value = tuple->values[nfield];
        else
            return NULL;
    }

    if (value->type != TUPLE_STRING)
        mowgli_throw_exception_val(audacious.tuple.invalid_type_request, NULL);

    return value->value.string;
}

gint
tuple_get_int(Tuple *tuple, const gint nfield, const gchar *field)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, 0);
    g_return_val_if_fail(nfield < FIELD_LAST, 0);

    if (nfield < 0) {
        if ((value = mowgli_dictionary_retrieve(tuple->dict, field)) == NULL)
            return 0;
    } else {
        if (tuple->values[nfield])
            value = tuple->values[nfield];
        else
            return 0;
    }

    if (value->type != TUPLE_INT)
        mowgli_throw_exception_val(audacious.tuple.invalid_type_request, 0);

    return value->value.integer;
}
