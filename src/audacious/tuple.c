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

static mowgli_heap_t *tuple_heap = NULL;
static mowgli_heap_t *tuple_value_heap = NULL;
static mowgli_object_class_t tuple_klass;

static GStaticRWLock tuple_rwlock = G_STATIC_RW_LOCK_INIT;

#define TUPLE_LOCK_WRITE(XX) g_static_rw_lock_writer_lock(&tuple_rwlock)
#define TUPLE_UNLOCK_WRITE(XX) g_static_rw_lock_writer_unlock(&tuple_rwlock)
#define TUPLE_LOCK_READ(XX) g_static_rw_lock_reader_lock(&tuple_rwlock)
#define TUPLE_UNLOCK_READ(XX) g_static_rw_lock_reader_unlock(&tuple_rwlock)

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

    TUPLE_LOCK_WRITE();
    mowgli_dictionary_destroy(tuple->dict, tuple_value_destroy, NULL);
    mowgli_heap_free(tuple_heap, tuple);
    TUPLE_UNLOCK_WRITE();
}

Tuple *
tuple_new(void)
{
    Tuple *tuple;

    TUPLE_LOCK_WRITE();
    
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

    TUPLE_UNLOCK_WRITE();
    
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

    TUPLE_LOCK_WRITE();
    if ((value = mowgli_dictionary_delete(tuple->dict, field)))
        tuple_disassociate_now(value);
    
    if (string == NULL) {
        TUPLE_UNLOCK_WRITE();
        return TRUE;
    }

    value = mowgli_heap_alloc(tuple_value_heap);
    value->type = TUPLE_STRING;
    value->value.string = g_strdup(string);

    mowgli_dictionary_add(tuple->dict, field, value);
    TUPLE_UNLOCK_WRITE();
    
    return TRUE;
}

gboolean
tuple_associate_int(Tuple *tuple, const gchar *field, gint integer)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, FALSE);
    g_return_val_if_fail(field != NULL, FALSE);

    TUPLE_LOCK_WRITE();
    if ((value = mowgli_dictionary_delete(tuple->dict, field)))
        tuple_disassociate_now(value);

    value = mowgli_heap_alloc(tuple_value_heap);
    value->type = TUPLE_INT;
    value->value.integer = integer;

    mowgli_dictionary_add(tuple->dict, field, value);
    TUPLE_UNLOCK_WRITE();
    
    return TRUE;
}

void
tuple_disassociate_now(TupleValue *value)
{
    if (value->type == TUPLE_STRING)
        g_free(value->value.string);

    mowgli_heap_free(tuple_value_heap, value);
}

void
tuple_disassociate(Tuple *tuple, const gchar *field)
{
    TupleValue *value;

    g_return_if_fail(tuple != NULL);
    g_return_if_fail(field != NULL);

    /* why _delete()? because _delete() returns the dictnode's data on success */
    TUPLE_LOCK_WRITE();
    value = mowgli_dictionary_delete(tuple->dict, field);
    
    if (value == NULL) {
        TUPLE_UNLOCK_WRITE();
        return;
    }
    
    tuple_disassociate_now(value);
    TUPLE_UNLOCK_WRITE();
}

TupleValueType
tuple_get_value_type(Tuple *tuple, const gchar *field)
{
    TupleValue *value;

    g_return_val_if_fail(tuple != NULL, TUPLE_UNKNOWN);
    g_return_val_if_fail(field != NULL, TUPLE_UNKNOWN);

    TUPLE_LOCK_READ();
    value = mowgli_dictionary_retrieve(tuple->dict, field);
    TUPLE_UNLOCK_READ();

    if (value == NULL)
        return TUPLE_UNKNOWN;

    return value->type;
}

const gchar *
tuple_get_string(Tuple *tuple, const gchar *field)
{
    TupleValue *value;
    gchar *val;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(field != NULL, NULL);

    TUPLE_LOCK_READ();
    value = mowgli_dictionary_retrieve(tuple->dict, field);
    
    if (value == NULL) {
        TUPLE_UNLOCK_READ();
        return NULL;
    }

    if (value->type != TUPLE_STRING) {
        TUPLE_UNLOCK_READ();
        mowgli_throw_exception_val(audacious.tuple.invalid_type_request, NULL);
    }

    val = value->value.string;
    TUPLE_UNLOCK_READ();
    
    return val;
}

int
tuple_get_int(Tuple *tuple, const gchar *field)
{
    TupleValue *value;
    gint val;

    g_return_val_if_fail(tuple != NULL, 0);
    g_return_val_if_fail(field != NULL, 0);

    TUPLE_LOCK_READ();
    value = mowgli_dictionary_retrieve(tuple->dict, field);
    
    if (value == NULL) {
        TUPLE_UNLOCK_READ();
        return 0;
    }

    if (value->type != TUPLE_INT) {
        TUPLE_UNLOCK_READ();
        mowgli_throw_exception_val(audacious.tuple.invalid_type_request, 0);
    }

    val = value->value.integer;
    TUPLE_UNLOCK_READ();

    return val;
}
