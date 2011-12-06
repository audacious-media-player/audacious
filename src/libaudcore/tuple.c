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
/**
 * @file tuple.c
 * @brief Basic Tuple handling API.
 */

#include <glib.h>
#include <mowgli.h>

#include "tuple.h"
#include "audstrings.h"
#include "stringpool.h"

/** Ordered table of basic #Tuple field names and their #TupleValueType.
 */
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
    { "quality",        TUPLE_STRING },

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
    { "mime-type",      TUPLE_STRING },
    { "bitrate",        TUPLE_INT },

    { "segment-start",  TUPLE_INT },
    { "segment-end",    TUPLE_INT },
};


/** A mowgli heap containing all the allocated tuples. */
static mowgli_heap_t *tuple_heap = NULL;

/** A mowgli heap containing values contained by tuples. */
static mowgli_heap_t *tuple_value_heap = NULL;
static mowgli_object_class_t tuple_klass;

/** Global R/W lock for preserve data consistency of heaps */
static GStaticRWLock tuple_rwlock = G_STATIC_RW_LOCK_INIT;

//@{
/**
 * Convenience macro for read/write locking of the globally
 * used internal Tuple system structures.
 */
#define TUPLE_LOCK_WRITE(X)     g_static_rw_lock_writer_lock(&tuple_rwlock)
#define TUPLE_UNLOCK_WRITE(X)   g_static_rw_lock_writer_unlock(&tuple_rwlock)
#define TUPLE_LOCK_READ(X)      g_static_rw_lock_reader_lock(&tuple_rwlock)
#define TUPLE_UNLOCK_READ(X)    g_static_rw_lock_reader_unlock(&tuple_rwlock)
//@}

/* iterative destructor of tuple values. */
static void
tuple_value_destroy(mowgli_dictionary_elem_t *delem, gpointer privdata)
{
    TupleValue *value = (TupleValue *) delem->data;

    if (value->type == TUPLE_STRING) {
        stringpool_unref(value->value.string);
        value->value.string = NULL;
    }

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

            if (value->type == TUPLE_STRING) {
                stringpool_unref(value->value.string);
                value->value.string = NULL;
            }

            mowgli_heap_free(tuple_value_heap, value);
        }

    g_free(tuple->subtunes);

    mowgli_heap_free(tuple_heap, tuple);
    TUPLE_UNLOCK_WRITE();
}

static Tuple *
tuple_new_unlocked(void)
{
    Tuple *tuple;

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

    return tuple;
}

/**
 * Allocates a new empty #Tuple structure. Must be freed via tuple_free().
 *
 * @return Pointer to newly allocated Tuple.
 */
Tuple *
tuple_new(void)
{
    Tuple *tuple;

    TUPLE_LOCK_WRITE();
    
    tuple = tuple_new_unlocked();

    TUPLE_UNLOCK_WRITE();
    return tuple;
}

static TupleValue *
tuple_associate_data(Tuple *tuple, const gint cnfield, const gchar *field, TupleValueType ftype);


/**
 * Sets filename/URI related fields of a #Tuple structure, based
 * on the given filename argument. The fields set are:
 * #FIELD_FILE_PATH, #FIELD_FILE_NAME and #FIELD_FILE_EXT.
 *
 * @param[in] filename Filename URI.
 * @param[in,out] tuple Tuple structure to manipulate.
 */
void
tuple_set_filename(Tuple *tuple, const gchar *filename)
{
    gchar *local = g_strdup(filename);
    gchar *slash, *period, *question;

    string_decode_percent(local);

    slash = strrchr(local, '/');
    period = strrchr(local, '.');
    question = strrchr(local, '?');

    if (slash != NULL)
    {
        gchar temp = *(slash + 1);

        *(slash + 1) = 0;
        tuple_associate_string(tuple, FIELD_FILE_PATH, NULL, local);
        *(slash + 1) = temp;
        tuple_associate_string(tuple, FIELD_FILE_NAME, NULL, slash + 1);
    }

    if (question != NULL)
    {
        gint subtune;

        *question = 0;

        if (sscanf(question + 1, "%d", &subtune) == 1)
            tuple_associate_int(tuple, FIELD_SUBSONG_ID, NULL, subtune);
    }

    if (period != NULL)
        tuple_associate_string(tuple, FIELD_FILE_EXT, NULL, period + 1);

    g_free(local);
}

/**
 * Creates a copy of given TupleValue structure, with copied data.
 *
 * @param[in] src TupleValue structure to be made a copy of.
 * @return Pointer to newly allocated TupleValue or NULL
 *         if error occured or source was NULL.
 */
static TupleValue *
tuple_copy_value(TupleValue *src)
{
    TupleValue *res;

    if (src == NULL) return NULL;

    res = mowgli_heap_alloc(tuple_value_heap);
    res->type = src->type;

    switch (src->type) {
    case TUPLE_STRING:
        res->value.string = stringpool_get(src->value.string);
        break;
    case TUPLE_INT:
        res->value.integer = src->value.integer;
        break;
    default:
        mowgli_heap_free (tuple_value_heap, res);
        return NULL;
    }
    return res;
}

/**
 * Creates a copy of given Tuple structure, with copied data.
 *
 * @param[in] src Tuple structure to be made a copy of.
 * @return Pointer to newly allocated Tuple.
 */
Tuple *
tuple_copy(const Tuple *src)
{
    Tuple *dst;
    TupleValue * tv, * copied;
    mowgli_dictionary_iteration_state_t state;
    gint i;

    TUPLE_LOCK_WRITE();

    dst = tuple_new_unlocked();

    /* Copy basic fields */
    for (i = 0; i < FIELD_LAST; i++)
        dst->values[i] = tuple_copy_value(src->values[i]);

    /* Copy dictionary contents */
    MOWGLI_DICTIONARY_FOREACH (tv, & state, src->dict)
    {
        if ((copied = tuple_copy_value (tv)) != NULL)
            mowgli_dictionary_add (dst->dict, state.cur->key, copied);
    }

    /* Copy subtune number information */
    if (src->subtunes && src->nsubtunes > 0)
    {
        dst->nsubtunes = src->nsubtunes;
        dst->subtunes = g_new(gint, dst->nsubtunes);
        memcpy(dst->subtunes, src->subtunes, sizeof(gint) * dst->nsubtunes);
    }

    TUPLE_UNLOCK_WRITE();
    return dst;
}

/**
 * Allocates a new #Tuple structure, setting filename/URI related
 * fields based on the given filename argument by calling #tuple_set_filename.
 *
 * @param[in] filename Filename URI.
 * @return Pointer to newly allocated Tuple.
 */
Tuple *
tuple_new_from_filename(const gchar *filename)
{
    Tuple *tuple = tuple_new();

    tuple_set_filename(tuple, filename);
    return tuple;
}


static gint
tuple_get_nfield(const gchar *field)
{
    gint i;
    for (i = 0; i < FIELD_LAST; i++)
        if (!strcmp(field, tuple_fields[i].name))
            return i;
    return -1;
}


/**
 * (Re)associates data into given #Tuple field.
 * If specified field already exists in the #Tuple, any data from it
 * is freed and this current TupleValue struct is returned.
 *
 * If field does NOT exist, a new structure is allocated from global
 * heap, added to Tuple and returned.
 *
 * @attention This function has (unbalanced) Tuple structure unlocking,
 * so please make sure you use it only exactly like it is used in
 * #tuple_associate_string(), etc.
 *
 * @param[in] tuple Tuple structure to be manipulated.
 * @param[in] cnfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 * @param[in] ftype Type of the field to be associated.
 * @return Pointer to associated TupleValue structure.
 */
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
        if (nfield >= 0)
            g_warning("Tuple FIELD_* not used for '%s'!\n", field);
    }

    /* Check if field was known */
    if (nfield >= 0) {
        tfield = tuple_fields[nfield].name;
        value = tuple->values[nfield];

        if (ftype != tuple_fields[nfield].type) {
            g_warning("Invalid type for [%s](%d->%d), %d != %d\n",
                tfield, cnfield, nfield, ftype, tuple_fields[nfield].type);
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
            stringpool_unref(value->value.string);
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

/**
 * Associates copy of given string to a field in specified #Tuple.
 * If field already exists, old value is freed and replaced.
 * 
 * Desired field can be specified either by key name or if it is
 * one of basic fields, by #TupleBasicType index.
 *
 * @param[in] tuple #Tuple structure pointer.
 * @param[in] nfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 * @param[in] string String to be associated to given field in Tuple.
 * @return TRUE if operation was succesful, FALSE if not.
 */
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
        value->value.string = stringpool_get(string);

    TUPLE_UNLOCK_WRITE();
    return TRUE;
}

/**
 * Associates given string to a field in specified #Tuple. The caller
 * gives up ownership of the string. If field already exists, old
 * value is freed and replaced.
 * 
 * Desired field can be specified either by key name or if it is
 * one of basic fields, by #TupleBasicType index.
 *
 * @param[in] tuple #Tuple structure pointer.
 * @param[in] nfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 * @param[in] string String to be associated to given field in Tuple.
 * @return TRUE if operation was succesful, FALSE if not.
 */
gboolean
tuple_associate_string_rel(Tuple *tuple, const gint nfield, const gchar *field, gchar *string)
{
    TupleValue *value;

    TUPLE_LOCK_WRITE();
    if ((value = tuple_associate_data(tuple, nfield, field, TUPLE_STRING)) == NULL)
        return FALSE;

    if (string == NULL)
        value->value.string = NULL;
    else
    {
        value->value.string = stringpool_get(string);
        g_free(string);
    }

    TUPLE_UNLOCK_WRITE();
    return TRUE;
}

/**
 * Associates given integer to a field in specified #Tuple.
 * If field already exists, old value is freed and replaced.
 *
 * Desired field can be specified either by key name or if it is
 * one of basic fields, by #TupleBasicType index.
 *
 * @param[in] tuple #Tuple structure pointer.
 * @param[in] nfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 * @param[in] integer Integer to be associated to given field in Tuple.
 * @return TRUE if operation was succesful, FALSE if not.
 */
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

/**
 * Disassociates given field from specified #Tuple structure.
 * Desired field can be specified either by key name or if it is
 * one of basic fields, by #TupleBasicType index.
 *
 * @param[in] tuple #Tuple structure pointer.
 * @param[in] cnfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 */
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
        stringpool_unref(value->value.string);
        value->value.string = NULL;
    }

    mowgli_heap_free(tuple_value_heap, value);
    TUPLE_UNLOCK_WRITE();
}

/**
 * Returns #TupleValueType of given #Tuple field.
 * Desired field can be specified either by key name or if it is
 * one of basic fields, by #TupleBasicType index.
 *
 * @param[in] tuple #Tuple structure pointer.
 * @param[in] cnfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 * @return #TupleValueType of the field or TUPLE_UNKNOWN if there was an error.
 */
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

/**
 * Returns pointer to a string associated to #Tuple field.
 * Desired field can be specified either by key name or if it is
 * one of basic fields, by #TupleBasicType index.
 *
 * @param[in] tuple #Tuple structure pointer.
 * @param[in] cnfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 * @return Pointer to string or NULL if the field/key did not exist.
 * The returned string is const, and must not be freed or modified.
 */
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

/**
 * Returns integer associated to #Tuple field.
 * Desired field can be specified either by key name or if it is
 * one of basic fields, by #TupleBasicType index.
 *
 * @param[in] tuple #Tuple structure pointer.
 * @param[in] cnfield #TupleBasicType index or -1 if key name is to be used instead.
 * @param[in] field String acting as key name or NULL if nfield is used.
 * @return Integer value or 0 if the field/key did not exist.
 *
 * @bug There is no way to distinguish error situations if the associated value is zero.
 */
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
