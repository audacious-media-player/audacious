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

struct _Tuple {
    mowgli_object_t parent;
    mowgli_dictionary_t *dict;
};

typedef struct {
    TupleValueType type;
    union {
        gchar *string;
        gint integer;
    } value;
} TupleValue;

static mowgli_heap_t *tuple_heap = NULL;
static mowgli_heap_t *tuple_value_heap = NULL;
static mowgli_object_class_t tuple_klass;

/* iterative destructor of tuple values. */
static void
tuple_value_destroy(mowgli_dictionary_elem_t *delem, gpointer privdata)
{
    TupleValue *value = (TupleValue *) delem->data;

    if (value->type == TUPLE_STRING)
        g_free(value->value.string);

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
        tuple_heap = mowgli_heap_create(sizeof(Tuple), 256, BH_NOW);
        tuple_value_heap = mowgli_heap_create(sizeof(TupleValue), 512, BH_NOW);
        mowgli_object_class_init(&tuple_klass, "audacious.tuple", tuple_destroy, FALSE);
    }

    /* FIXME: use mowgli_object_bless_from_class() in mowgli 0.4
       when it is released --nenolod */
    tuple = mowgli_heap_alloc(tuple_heap);
    mowgli_object_init(mowgli_object(tuple), NULL, &tuple_klass, NULL);

    tuple->dict = mowgli_dictionary_create(g_ascii_strcasecmp);

    return tuple;
}

Tuple *
tuple_new_from_filename(gchar *filename)
{
    gchar *scratch, *ext, *realfn;
    Tuple *tuple;

    g_return_val_if_fail(filename != NULL, NULL);

    tuple = tuple_new();
    
    g_return_val_if_fail(tuple != NULL, NULL);

    realfn = g_filename_from_uri(filename, NULL, NULL);

    scratch = g_path_get_basename(realfn ? realfn : filename);
    tuple_associate_string(tuple, "file-name", scratch);
    g_free(scratch);

    scratch = g_path_get_dirname(realfn ? realfn : filename);
    tuple_associate_string(tuple, "file-path", scratch);
    g_free(scratch);

    g_free(realfn); realfn = NULL;

    ext = strrchr(filename, '.');
    if (ext != NULL) {
        ++ext;
        tuple_associate_string(tuple, "file-ext", scratch);
    }

    return tuple;
}        

gboolean
tuple_associate_string(Tuple *tuple, const gchar *field, const gchar *string)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, FALSE);
    g_return_val_if_fail(field != NULL, FALSE);
    g_return_val_if_fail(string != NULL, FALSE);

    if (mowgli_dictionary_find(tuple->dict, field))
        tuple_disassociate(tuple, field);

    value = mowgli_heap_alloc(tuple_value_heap);
    value->type = TUPLE_STRING;
    value->value.string = g_strdup(string);

    mowgli_dictionary_add(tuple->dict, field, value);

    return TRUE;
}

gboolean
tuple_associate_int(Tuple *tuple, const gchar *field, gint integer)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, FALSE);
    g_return_val_if_fail(field != NULL, FALSE);

    if (mowgli_dictionary_find(tuple->dict, field))
        tuple_disassociate(tuple, field);

    value = mowgli_heap_alloc(tuple_value_heap);
    value->type = TUPLE_INT;
    value->value.integer = integer;

    mowgli_dictionary_add(tuple->dict, field, value);

    return TRUE;
}

void
tuple_disassociate(Tuple *tuple, const gchar *field)
{
    TupleValue *value;

    g_return_if_fail(tuple != NULL);
    g_return_if_fail(field != NULL);

    /* why _delete()? because _delete() returns the dictnode's data on success */
    if ((value = mowgli_dictionary_delete(tuple->dict, field)) == NULL)
        return;

    if (value->type == TUPLE_STRING)
        g_free(value->value.string);

    mowgli_heap_free(tuple_value_heap, value);
}

TupleValueType
tuple_get_value_type(Tuple *tuple, const gchar *field)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, TUPLE_UNKNOWN);
    g_return_val_if_fail(field != NULL, TUPLE_UNKNOWN);

    if ((value = mowgli_dictionary_retrieve(tuple->dict, field)) == NULL)
        return TUPLE_UNKNOWN;

    return value->type;
}

const gchar *
tuple_get_string(Tuple *tuple, const gchar *field)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(field != NULL, NULL);

    if ((value = mowgli_dictionary_retrieve(tuple->dict, field)) == NULL)
        return NULL;

    if (value->type != TUPLE_STRING)
        mowgli_throw_exception_val(audacious.tuple.invalid_type_request, NULL);

    return value->value.string;
}

int
tuple_get_int(Tuple *tuple, const gchar *field)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, 0);
    g_return_val_if_fail(field != NULL, 0);

    if ((value = mowgli_dictionary_retrieve(tuple->dict, field)) == NULL)
        return 0;

    if (value->type != TUPLE_INT)
        mowgli_throw_exception_val(audacious.tuple.invalid_type_request, 0);

    return value->value.integer;
}
