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

#include <audacious/i18n.h>

#include "config.h"
#include "tuple.h"
#include "audstrings.h"
#include "stringpool.h"

static gboolean set_string (Tuple * tuple, const gint nfield,
 const gchar * field, gchar * string, gboolean take);

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

    { "gain-album-gain", TUPLE_INT },
    { "gain-album-peak", TUPLE_INT },
    { "gain-track-gain", TUPLE_INT },
    { "gain-track-peak", TUPLE_INT },
    { "gain-gain-unit", TUPLE_INT },
    { "gain-peak-unit", TUPLE_INT },

    { "composer",       TUPLE_STRING },
};


/** A mowgli heap containing all the allocated tuples. */
static mowgli_heap_t *tuple_heap = NULL;

/** A mowgli heap containing values contained by tuples. */
static mowgli_heap_t *tuple_value_heap = NULL;
static mowgli_object_class_t tuple_klass;

/** Global lock to preserve data consistency of heaps */
static GStaticMutex tuple_mutex = G_STATIC_MUTEX_INIT;

//@{
/**
 * Convenience macros to lock the globally
 * used internal Tuple system structures.
 */
#define TUPLE_LOCK_WRITE(X)     g_static_mutex_lock (& tuple_mutex)
#define TUPLE_UNLOCK_WRITE(X)   g_static_mutex_unlock (& tuple_mutex)
#define TUPLE_LOCK_READ(X)      g_static_mutex_lock (& tuple_mutex)
#define TUPLE_UNLOCK_READ(X)    g_static_mutex_unlock (& tuple_mutex)
//@}

static void tuple_value_destroy (TupleValue * value)
{
    if (value->type == TUPLE_STRING)
        stringpool_unref (value->value.string);

    memset (value, 0, sizeof (TupleValue));
    mowgli_heap_free (tuple_value_heap, value);
}

/* iterative destructor of tuple values. */
static void tuple_value_destroy_cb (const gchar * key, void * data, void * priv)
{
    tuple_value_destroy (data);
}

static void
tuple_destroy(gpointer data)
{
    Tuple *tuple = (Tuple *) data;
    gint i;

    TUPLE_LOCK_WRITE();
    mowgli_patricia_destroy(tuple->dict, tuple_value_destroy_cb, NULL);

    for (i = 0; i < FIELD_LAST; i++)
    {
        if (tuple->values[i])
            tuple_value_destroy (tuple->values[i]);
    }

    g_free(tuple->subtunes);

    memset (tuple, 0, sizeof (Tuple));
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

    tuple->dict = mowgli_patricia_create(string_canonize_case);

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
void tuple_set_filename (Tuple * tuple, const gchar * name)
{
    const gchar * slash;
    if ((slash = strrchr (name, '/')))
    {
        gchar path[slash - name + 2];
        memcpy (path, name, slash - name + 1);
        path[slash - name + 1] = 0;

        set_string (tuple, FIELD_FILE_PATH, NULL, uri_to_display (path), TRUE);
        name = slash + 1;
    }

    gchar buf[strlen (name) + 1];
    strcpy (buf, name);

    gchar * c;
    if ((c = strrchr (buf, '?')))
    {
        gint sub;
        if (sscanf (c + 1, "%d", & sub) == 1)
            tuple_associate_int (tuple, FIELD_SUBSONG_ID, NULL, sub);

        * c = 0;
    }

    gchar * base = uri_to_display (buf);

    if ((c = strrchr (base, '.')))
        set_string (tuple, FIELD_FILE_EXT, NULL, c + 1, FALSE);

    set_string (tuple, FIELD_FILE_NAME, NULL, base, TRUE);
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
    g_strlcpy(res->name, src->name, TUPLE_NAME_MAX);
    res->type = src->type;

    switch (src->type) {
    case TUPLE_STRING:
        res->value.string = stringpool_get (src->value.string, FALSE);
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
    mowgli_patricia_iteration_state_t state;
    gint i;

    g_return_val_if_fail(src != NULL, NULL);

    TUPLE_LOCK_WRITE();

    dst = tuple_new_unlocked();

    /* Copy basic fields */
    for (i = 0; i < FIELD_LAST; i++)
        dst->values[i] = tuple_copy_value(src->values[i]);

    /* Copy dictionary contents */
    MOWGLI_PATRICIA_FOREACH (tv, & state, src->dict)
    {
        if ((copied = tuple_copy_value (tv)) != NULL)
            mowgli_patricia_add (dst->dict, copied->name, copied);
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
        value = mowgli_patricia_retrieve(tuple->dict, tfield);
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
        {
            value->name[0] = 0;
            tuple->values[nfield] = value;
        }
        else
        {
            g_strlcpy (value->name, tfield, TUPLE_NAME_MAX);
            mowgli_patricia_add(tuple->dict, tfield, value);
        }
    }

    return value;
}

static gboolean set_string (Tuple * tuple, const gint nfield,
 const gchar * field, gchar * string, gboolean take)
{
    TUPLE_LOCK_WRITE ();

    TupleValue * value = tuple_associate_data (tuple, nfield, field,
     TUPLE_STRING);
    if (! value)
    {
        if (take)
            g_free (string);
        return FALSE;
    }

    if (! string)
        value->value.string = NULL;
    else
        value->value.string = stringpool_get (string, take);

    TUPLE_UNLOCK_WRITE ();
    return TRUE;
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

gboolean tuple_associate_string (Tuple * tuple, const gint nfield,
 const gchar * field, const gchar * string)
{
    if (string && ! g_utf8_validate (string, -1, NULL))
    {
        fprintf (stderr, "Invalid UTF-8: %s.\n", string);
        return set_string (tuple, nfield, field, str_to_utf8 (string), TRUE);
    }

    gboolean ret = set_string (tuple, nfield, field, (gchar *) string, FALSE);
    return ret;
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

gboolean tuple_associate_string_rel (Tuple * tuple, const gint nfield,
 const gchar * field, gchar * string)
{
    if (string && ! g_utf8_validate (string, -1, NULL))
    {
        fprintf (stderr, "Invalid UTF-8: %s.\n", string);
        gchar * copy = str_to_utf8 (string);
        g_free (string);
        string = copy;
    }

    return set_string (tuple, nfield, field, string, TRUE);
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
        value = mowgli_patricia_delete(tuple->dict, field);
    else {
        value = tuple->values[nfield];
        tuple->values[nfield] = NULL;
    }

    if (value)
        tuple_value_destroy (value);

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
TupleValueType tuple_get_value_type (const Tuple * tuple, gint cnfield,
 const gchar * field)
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
        if ((value = mowgli_patricia_retrieve(tuple->dict, field)) != NULL)
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
const gchar * tuple_get_string (const Tuple * tuple, gint cnfield, const gchar *
 field)
{
    TupleValue *value;
    gint nfield = cnfield;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(nfield < FIELD_LAST, NULL);

    if (nfield < 0)
        nfield = tuple_get_nfield(field);

    TUPLE_LOCK_READ();
    if (nfield < 0)
        value = mowgli_patricia_retrieve(tuple->dict, field);
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
gint tuple_get_int (const Tuple * tuple, gint cnfield, const gchar * field)
{
    TupleValue *value;
    gint nfield = cnfield;

    g_return_val_if_fail(tuple != NULL, 0);
    g_return_val_if_fail(nfield < FIELD_LAST, 0);

    if (nfield < 0)
        nfield = tuple_get_nfield(field);

    TUPLE_LOCK_READ();
    if (nfield < 0)
        value = mowgli_patricia_retrieve(tuple->dict, field);
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

#define APPEND(b, ...) snprintf (b + strlen (b), sizeof b - strlen (b), \
 __VA_ARGS__)

void tuple_set_format (Tuple * t, const gchar * format, gint chans, gint rate,
 gint brate)
{
    if (format)
        tuple_associate_string (t, FIELD_CODEC, NULL, format);

    gchar buf[32];
    buf[0] = 0;

    if (chans > 0)
    {
        if (chans == 1)
            APPEND (buf, _("Mono"));
        else if (chans == 2)
            APPEND (buf, _("Stereo"));
        else
            APPEND (buf, _("%d channels"), chans);

        if (rate > 0)
            APPEND (buf, ", ");
    }

    if (rate > 0)
        APPEND (buf, "%d kHz", rate / 1000);

    if (buf[0])
        tuple_associate_string (t, FIELD_QUALITY, NULL, buf);
    
    if (brate > 0)
        tuple_associate_int (t, FIELD_BITRATE, NULL, brate);
}
