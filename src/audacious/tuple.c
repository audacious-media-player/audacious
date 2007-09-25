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

    { "subsong-id",     TUPLE_INT },
    { "subsong-num",    TUPLE_INT },
};

static mowgli_heap_t *tuple_heap = NULL;
static mowgli_heap_t *tuple_value_heap = NULL;
static mowgli_object_class_t tuple_klass;


#define TUPLE_LOCKING
//#define TUPLE_DEBUG

#ifdef TUPLE_LOCKING
static GStaticRWLock tuple_rwlock = G_STATIC_RW_LOCK_INIT;
#  ifdef TUPLE_DEBUG
#    define TUPDEB(X) fprintf(stderr, "TUPLE_" X "(%s:%d)\n", __FUNCTION__, __LINE__)
#    define TUPLE_LOCK_WRITE(XX)   { TUPDEB("LOCK_WRITE"); g_static_rw_lock_writer_lock(&tuple_rwlock); }
#    define TUPLE_UNLOCK_WRITE(XX) { TUPDEB("UNLOCK_WRITE"); g_static_rw_lock_writer_unlock(&tuple_rwlock); }
#    define TUPLE_LOCK_READ(XX)    { TUPDEB("LOCK_READ"); g_static_rw_lock_reader_lock(&tuple_rwlock); }
#    define TUPLE_UNLOCK_READ(XX)  { TUPDEB("UNLOCK_READ"); g_static_rw_lock_reader_unlock(&tuple_rwlock); }
#  else
#    define TUPLE_LOCK_WRITE(XX) g_static_rw_lock_writer_lock(&tuple_rwlock)
#    define TUPLE_UNLOCK_WRITE(XX) g_static_rw_lock_writer_unlock(&tuple_rwlock)
#    define TUPLE_LOCK_READ(XX) g_static_rw_lock_reader_lock(&tuple_rwlock)
#    define TUPLE_UNLOCK_READ(XX) g_static_rw_lock_reader_unlock(&tuple_rwlock)
#  endif
#else
#  define TUPLE_LOCK_WRITE(XX)
#  define TUPLE_UNLOCK_WRITE(XX)
#  define TUPLE_LOCK_READ(XX)
#  define TUPLE_UNLOCK_READ(XX)
#endif

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
    gint i;

    TUPLE_LOCK_WRITE();
    mowgli_dictionary_destroy(tuple->dict, tuple_value_destroy, NULL);

    for (i = 0; i < FIELD_LAST; i++)
        if (tuple->values[i]) {
            TupleValue *value = tuple->values[i];
            
            if (value->type == TUPLE_STRING)
                g_free(value->value.string);
            
            mowgli_heap_free(tuple_value_heap, value);
        }
    
    g_free(tuple->subtunes);
    
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
        tuple_heap = mowgli_heap_create(sizeof(Tuple), 512, BH_NOW);
        tuple_value_heap = mowgli_heap_create(sizeof(TupleValue), 1024, BH_NOW);
        mowgli_object_class_init(&tuple_klass, "audacious.tuple", tuple_destroy, FALSE);
    }

    /* FIXME: use mowgli_object_bless_from_class() in mowgli 0.4
       when it is released --nenolod */
    tuple = mowgli_heap_alloc(tuple_heap);
    memset(tuple, 0, sizeof(Tuple));
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
    tuple_associate_string(tuple, FIELD_FILE_NAME, NULL, scratch);
    g_free(scratch);

    scratch = g_path_get_dirname(realfn ? realfn : filename);
    tuple_associate_string(tuple, FIELD_FILE_PATH, NULL, scratch);
    g_free(scratch);

    g_free(realfn); realfn = NULL;

    ext = strrchr(filename, '.');
    if (ext != NULL) {
        ++ext;
        tuple_associate_string(tuple, FIELD_FILE_EXT, NULL, scratch);
    }

    return tuple;
}        


static gint tuple_get_nfield(const gchar *field)
{
    gint i;
    for (i = 0; i < FIELD_LAST; i++)
        if (!strcmp(field, tuple_fields[i].name))
            return i;
    return -1;
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
        nfield = tuple_get_nfield(field);
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
            TUPLE_UNLOCK_WRITE();
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
        if (nfield >= 0)
            tuple->values[nfield] = value;
        else
            mowgli_dictionary_add(tuple->dict, tfield, value);
    }

    return value;
}

gboolean
tuple_associate_string(Tuple *tuple, const gint nfield, const gchar *field, const gchar *string)
{
    TupleValue *value;

    TUPLE_LOCK_WRITE();
    if ((value = tuple_associate_data(tuple, nfield, field, TUPLE_STRING)) == NULL)
        return FALSE;

    if (string == NULL)
        value->value.string = NULL;
    else
        value->value.string = g_strdup(string);

    TUPLE_UNLOCK_WRITE();
    return TRUE;
}

gboolean
tuple_associate_int(Tuple *tuple, const gint nfield, const gchar *field, gint integer)
{
    TupleValue *value;
    
    TUPLE_LOCK_WRITE();
    if ((value = tuple_associate_data(tuple, nfield, field, TUPLE_INT)) == NULL)
        return FALSE;

    value->value.integer = integer;
    
    TUPLE_UNLOCK_WRITE();
    return TRUE;
}

void
tuple_disassociate(Tuple *tuple, const gint cnfield, const gchar *field)
{
    TupleValue *value;
    gint nfield = cnfield;

    g_return_if_fail(tuple != NULL);
    g_return_if_fail(nfield < FIELD_LAST);

    if (nfield < 0)
        nfield = tuple_get_nfield(field);

    TUPLE_LOCK_WRITE();
    if (nfield < 0)
        /* why _delete()? because _delete() returns the dictnode's data on success */
        value = mowgli_dictionary_delete(tuple->dict, field);
    else {
        value = tuple->values[nfield];
        tuple->values[nfield] = NULL;
    }

    if (value == NULL) {
        TUPLE_UNLOCK_WRITE();
        return;
    }
    
    /* Free associated data */
    if (value->type == TUPLE_STRING) {
        g_free(value->value.string);
        value->value.string = NULL;
    }

    mowgli_heap_free(tuple_value_heap, value);
    TUPLE_UNLOCK_WRITE();
}

TupleValueType
tuple_get_value_type(Tuple *tuple, const gint cnfield, const gchar *field)
{
    TupleValueType type = TUPLE_UNKNOWN;
    gint nfield = cnfield;

    g_return_val_if_fail(tuple != NULL, TUPLE_UNKNOWN);
    g_return_val_if_fail(nfield < FIELD_LAST, TUPLE_UNKNOWN);

    if (nfield < 0)
        nfield = tuple_get_nfield(field);
    
    TUPLE_LOCK_READ();
    if (nfield < 0) {
        TupleValue *value;
        if ((value = mowgli_dictionary_retrieve(tuple->dict, field)) != NULL)
            type = value->type;
    } else {
        if (tuple->values[nfield])
            type = tuple->values[nfield]->type;
    }
    
    TUPLE_UNLOCK_READ();
    return type;
}

const gchar *
tuple_get_string(Tuple *tuple, const gint cnfield, const gchar *field)
{
    TupleValue *value;
    gint nfield = cnfield;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(nfield < FIELD_LAST, NULL);

    if (nfield < 0)
        nfield = tuple_get_nfield(field);
    
    TUPLE_LOCK_READ();
    if (nfield < 0)
        value = mowgli_dictionary_retrieve(tuple->dict, field);
    else
        value = tuple->values[nfield];

    if (value) {
        if (value->type != TUPLE_STRING)
            mowgli_throw_exception_val(audacious.tuple.invalid_type_request, NULL);
        
        TUPLE_UNLOCK_READ();
        return value->value.string;
    } else {
        TUPLE_UNLOCK_READ();
        return NULL;
    }
}

gint
tuple_get_int(Tuple *tuple, const gint cnfield, const gchar *field)
{
    TupleValue *value;
    gint nfield = cnfield;

    g_return_val_if_fail(tuple != NULL, 0);
    g_return_val_if_fail(nfield < FIELD_LAST, 0);

    if (nfield < 0)
        nfield = tuple_get_nfield(field);
    
    TUPLE_LOCK_READ();
    if (nfield < 0)
        value = mowgli_dictionary_retrieve(tuple->dict, field);
    else
        value = tuple->values[nfield];

    if (value) {
        if (value->type != TUPLE_INT)
            mowgli_throw_exception_val(audacious.tuple.invalid_type_request, 0);
        
        TUPLE_UNLOCK_READ();
        return value->value.integer;
    } else {
        TUPLE_UNLOCK_READ();
        return 0;
    }
}
