/*
 * tuple.c
 * Copyright 2007-2011 William Pitcock, Christian Birchinger, Matti Hämäläinen,
 *                     Giacomo Lozito, Eugene Zagidullin, and John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

/**
 * @file tuple.c
 * @brief Basic Tuple handling API.
 */

#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <audacious/i18n.h>

#include "audstrings.h"
#include "tuple.h"
#include "tuple_formatter.h"

#define BLOCK_VALS 4

typedef struct {
    char *name;
    TupleValueType type;
} TupleBasicType;

typedef union {
    char * str;
    int x;
} TupleVal;

typedef struct _TupleBlock TupleBlock;

struct _TupleBlock {
    TupleBlock * next;
    char fields[BLOCK_VALS];
    TupleVal vals[BLOCK_VALS];
};

/**
 * Structure for holding and passing around miscellaneous track
 * metadata. This is not the same as a playlist entry, though.
 */
struct _Tuple {
    int refcount;
    int64_t setmask;
    TupleBlock * blocks;

    int nsubtunes;                 /**< Number of subtunes, if any. Values greater than 0
                                         mean that there are subtunes and #subtunes array
                                         may be set. */
    int *subtunes;                 /**< Array of int containing subtune index numbers.
                                         Can be NULL if indexing is linear or if
                                         there are no subtunes. */
};

#define BIT(i) ((int64_t) 1 << (i))

/** Ordered table of basic #Tuple field names and their #TupleValueType.
 */
static const TupleBasicType tuple_fields[TUPLE_FIELDS] = {
    { "artist",         TUPLE_STRING },
    { "title",          TUPLE_STRING },
    { "album",          TUPLE_STRING },
    { "comment",        TUPLE_STRING },
    { "genre",          TUPLE_STRING },

    { "track-number",   TUPLE_INT },
    { "length",         TUPLE_INT },
    { "year",           TUPLE_INT },
    { "quality",        TUPLE_STRING },

    { "codec",          TUPLE_STRING },
    { "file-name",      TUPLE_STRING },
    { "file-path",      TUPLE_STRING },
    { "file-ext",       TUPLE_STRING },

    { "song-artist",    TUPLE_STRING },
    { "composer",       TUPLE_STRING },
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
};

typedef struct {
    const char * name;
    int field;
} FieldDictEntry;

/* used for binary search, MUST be in alphabetical order */
static const FieldDictEntry field_dict[TUPLE_FIELDS] = {
 {"album", FIELD_ALBUM},
 {"artist", FIELD_ARTIST},
 {"bitrate", FIELD_BITRATE},
 {"codec", FIELD_CODEC},
 {"comment", FIELD_COMMENT},
 {"composer", FIELD_COMPOSER},
 {"copyright", FIELD_COPYRIGHT},
 {"date", FIELD_DATE},
 {"file-ext", FIELD_FILE_EXT},
 {"file-name", FIELD_FILE_NAME},
 {"file-path", FIELD_FILE_PATH},
 {"gain-album-gain", FIELD_GAIN_ALBUM_GAIN},
 {"gain-album-peak", FIELD_GAIN_ALBUM_PEAK},
 {"gain-gain-unit", FIELD_GAIN_GAIN_UNIT},
 {"gain-peak-unit", FIELD_GAIN_PEAK_UNIT},
 {"gain-track-gain", FIELD_GAIN_TRACK_GAIN},
 {"gain-track-peak", FIELD_GAIN_TRACK_PEAK},
 {"genre", FIELD_GENRE},
 {"length", FIELD_LENGTH},
 {"mime-type", FIELD_MIMETYPE},
 {"performer", FIELD_PERFORMER},
 {"quality", FIELD_QUALITY},
 {"segment-end", FIELD_SEGMENT_END},
 {"segment-start", FIELD_SEGMENT_START},
 {"song-artist", FIELD_SONG_ARTIST},
 {"subsong-id", FIELD_SUBSONG_ID},
 {"subsong-num", FIELD_SUBSONG_NUM},
 {"title", FIELD_TITLE},
 {"track-number", FIELD_TRACK_NUMBER},
 {"year", FIELD_YEAR}};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


static int field_dict_compare (const void * a, const void * b)
{
    return strcmp (((FieldDictEntry *) a)->name, ((FieldDictEntry *) b)->name);
}

EXPORT int tuple_field_by_name (const char * name)
{
    FieldDictEntry find = {name, -1};
    FieldDictEntry * found = bsearch (& find, field_dict, TUPLE_FIELDS,
     sizeof (FieldDictEntry), field_dict_compare);

    if (found)
        return found->field;

    fprintf (stderr, "Unknown tuple field name \"%s\".\n", name);
    return -1;
}

EXPORT const char * tuple_field_get_name (int field)
{
    if (field < 0 || field >= TUPLE_FIELDS)
        return NULL;

    return tuple_fields[field].name;
}

EXPORT TupleValueType tuple_field_get_type (int field)
{
    if (field < 0 || field >= TUPLE_FIELDS)
        return TUPLE_UNKNOWN;

    return tuple_fields[field].type;
}

static TupleVal * lookup_val (Tuple * tuple, int field, bool_t add, bool_t remove)
{
    if ((tuple->setmask & BIT (field)))
    {
        for (TupleBlock * block = tuple->blocks; block; block = block->next)
        {
            for (int i = 0; i < BLOCK_VALS; i ++)
            {
                if (block->fields[i] == field)
                {
                    if (remove)
                    {
                        tuple->setmask &= ~BIT (field);
                        block->fields[i] = -1;
                    }

                    return & block->vals[i];
                }
            }
        }
    }

    if (! add)
        return NULL;

    tuple->setmask |= BIT (field);

    for (TupleBlock * block = tuple->blocks; block; block = block->next)
    {
        for (int i = 0; i < BLOCK_VALS; i ++)
        {
            if (block->fields[i] < 0)
            {
                block->fields[i] = field;
                return & block->vals[i];
            }
        }
    }

    TupleBlock * block = g_slice_new0 (TupleBlock);
    memset (block->fields, -1, BLOCK_VALS);

    block->next = tuple->blocks;
    tuple->blocks = block;

    block->fields[0] = field;
    return & block->vals[0];
}

static void tuple_destroy_unlocked (Tuple * tuple)
{
    TupleBlock * next;
    for (TupleBlock * block = tuple->blocks; block; block = next)
    {
        next = block->next;

        for (int i = 0; i < BLOCK_VALS; i ++)
        {
            int field = block->fields[i];
            if (field >= 0 && tuple_fields[field].type == TUPLE_STRING)
                str_unref (block->vals[i].str);
        }

        memset (block, 0, sizeof (TupleBlock));
        g_slice_free (TupleBlock, block);
    }

    g_free(tuple->subtunes);

    memset (tuple, 0, sizeof (Tuple));
    g_slice_free (Tuple, tuple);
}

EXPORT Tuple * tuple_new (void)
{
    Tuple * tuple = g_slice_new0 (Tuple);
    tuple->refcount = 1;
    return tuple;
}

EXPORT Tuple * tuple_ref (Tuple * tuple)
{
    pthread_mutex_lock (& mutex);

    tuple->refcount ++;

    pthread_mutex_unlock (& mutex);
    return tuple;
}

EXPORT void tuple_unref (Tuple * tuple)
{
    if (! tuple)
        return;

    pthread_mutex_lock (& mutex);

    if (! -- tuple->refcount)
        tuple_destroy_unlocked (tuple);

    pthread_mutex_unlock (& mutex);
}

/**
 * Sets filename/URI related fields of a #Tuple structure, based
 * on the given filename argument. The fields set are:
 * #FIELD_FILE_PATH, #FIELD_FILE_NAME and #FIELD_FILE_EXT.
 *
 * @param[in] filename Filename URI.
 * @param[in,out] tuple Tuple structure to manipulate.
 */
EXPORT void tuple_set_filename (Tuple * tuple, const char * filename)
{
    const char * base, * ext, * sub;
    int isub;

    uri_parse (filename, & base, & ext, & sub, & isub);

    char path[base - filename + 1];
    str_decode_percent (filename, base - filename, path);
    tuple_set_str (tuple, FIELD_FILE_PATH, NULL, path);

    char name[ext - base + 1];
    str_decode_percent (base, ext - base, name);
    tuple_set_str (tuple, FIELD_FILE_NAME, NULL, name);

    if (ext < sub)
    {
        char extbuf[sub - ext];
        str_decode_percent (ext + 1, sub - ext - 1, extbuf);
        tuple_set_str (tuple, FIELD_FILE_EXT, NULL, extbuf);
    }

    if (sub[0])
        tuple_set_int (tuple, FIELD_SUBSONG_ID, NULL, isub);
}

/**
 * Creates a copy of given Tuple structure, with copied data.
 *
 * @param[in] src Tuple structure to be made a copy of.
 * @return Pointer to newly allocated Tuple.
 */
EXPORT Tuple * tuple_copy (const Tuple * old)
{
    pthread_mutex_lock (& mutex);

    Tuple * new = tuple_new ();

    for (int f = 0; f < TUPLE_FIELDS; f ++)
    {
        TupleVal * oldval = lookup_val ((Tuple *) old, f, FALSE, FALSE);
        if (oldval)
        {
            TupleVal * newval = lookup_val (new, f, TRUE, FALSE);
            if (tuple_fields[f].type == TUPLE_STRING)
                newval->str = str_ref (oldval->str);
            else
                newval->x = oldval->x;
        }
    }

    new->nsubtunes = old->nsubtunes;

    if (old->subtunes)
        new->subtunes = g_memdup (old->subtunes, sizeof (int) * old->nsubtunes);

    pthread_mutex_unlock (& mutex);
    return new;
}

/**
 * Allocates a new #Tuple structure, setting filename/URI related
 * fields based on the given filename argument by calling #tuple_set_filename.
 *
 * @param[in] filename Filename URI.
 * @return Pointer to newly allocated Tuple.
 */
EXPORT Tuple *
tuple_new_from_filename(const char *filename)
{
    Tuple *tuple = tuple_new();

    tuple_set_filename(tuple, filename);
    return tuple;
}

EXPORT void tuple_set_int (Tuple * tuple, int nfield, const char * field, int x)
{
    if (nfield < 0)
        nfield = tuple_field_by_name (field);
    if (nfield < 0 || nfield >= TUPLE_FIELDS || tuple_fields[nfield].type != TUPLE_INT)
        return;

    pthread_mutex_lock (& mutex);

    TupleVal * val = lookup_val (tuple, nfield, TRUE, FALSE);
    val->x = x;

    pthread_mutex_unlock (& mutex);
}

EXPORT void tuple_set_str (Tuple * tuple, int nfield, const char * field, const char * str)
{
    if (! str)
    {
        tuple_unset (tuple, nfield, field);
        return;
    }

    if (! g_utf8_validate (str, -1, NULL))
    {
        fprintf (stderr, "Invalid UTF-8: %s\n", str);
        return;
    }

    if (nfield < 0)
        nfield = tuple_field_by_name (field);
    if (nfield < 0 || nfield >= TUPLE_FIELDS || tuple_fields[nfield].type != TUPLE_STRING)
        return;

    pthread_mutex_lock (& mutex);

    TupleVal * val = lookup_val (tuple, nfield, TRUE, FALSE);
    if (val->str)
        str_unref (val->str);
    val->str = str_get (str);

    pthread_mutex_unlock (& mutex);
}

EXPORT void tuple_unset (Tuple * tuple, int nfield, const char * field)
{
    if (nfield < 0)
        nfield = tuple_field_by_name (field);
    if (nfield < 0 || nfield >= TUPLE_FIELDS)
        return;

    pthread_mutex_lock (& mutex);

    TupleVal * val = lookup_val (tuple, nfield, FALSE, TRUE);
    if (val)
    {
        if (tuple_fields[nfield].type == TUPLE_STRING)
        {
            str_unref (val->str);
            val->str = NULL;
        }
        else
            val->x = 0;
    }

    pthread_mutex_unlock (& mutex);
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
EXPORT TupleValueType tuple_get_value_type (const Tuple * tuple, int nfield, const char * field)
{
    if (nfield < 0)
        nfield = tuple_field_by_name (field);
    if (nfield < 0 || nfield >= TUPLE_FIELDS)
        return TUPLE_UNKNOWN;

    pthread_mutex_lock (& mutex);

    TupleValueType type = TUPLE_UNKNOWN;

    TupleVal * val = lookup_val ((Tuple *) tuple, nfield, FALSE, FALSE);
    if (val)
        type = tuple_fields[nfield].type;

    pthread_mutex_unlock (& mutex);
    return type;
}

EXPORT char * tuple_get_str (const Tuple * tuple, int nfield, const char * field)
{
    if (nfield < 0)
        nfield = tuple_field_by_name (field);
    if (nfield < 0 || nfield >= TUPLE_FIELDS || tuple_fields[nfield].type != TUPLE_STRING)
        return NULL;

    pthread_mutex_lock (& mutex);

    char * str = NULL;

    TupleVal * val = lookup_val ((Tuple *) tuple, nfield, FALSE, FALSE);
    if (val)
        str = str_ref (val->str);

    pthread_mutex_unlock (& mutex);
    return str;
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
EXPORT int tuple_get_int (const Tuple * tuple, int nfield, const char * field)
{
    if (nfield < 0)
        nfield = tuple_field_by_name (field);
    if (nfield < 0 || nfield >= TUPLE_FIELDS || tuple_fields[nfield].type != TUPLE_INT)
        return 0;

    pthread_mutex_lock (& mutex);

    int x = 0;

    TupleVal * val = lookup_val ((Tuple *) tuple, nfield, FALSE, FALSE);
    if (val)
        x = val->x;

    pthread_mutex_unlock (& mutex);
    return x;
}

#define APPEND(b, ...) snprintf (b + strlen (b), sizeof b - strlen (b), \
 __VA_ARGS__)

EXPORT void tuple_set_format (Tuple * t, const char * format, int chans, int rate,
 int brate)
{
    if (format)
        tuple_set_str (t, FIELD_CODEC, NULL, format);

    char buf[32];
    buf[0] = 0;

    if (chans > 0)
    {
        if (chans == 1)
            APPEND (buf, _("Mono"));
        else if (chans == 2)
            APPEND (buf, _("Stereo"));
        else
            APPEND (buf, dngettext (PACKAGE, "%d channel", "%d channels",
             chans), chans);

        if (rate > 0)
            APPEND (buf, ", ");
    }

    if (rate > 0)
        APPEND (buf, "%d kHz", rate / 1000);

    if (buf[0])
        tuple_set_str (t, FIELD_QUALITY, NULL, buf);

    if (brate > 0)
        tuple_set_int (t, FIELD_BITRATE, NULL, brate);
}

EXPORT void tuple_set_subtunes (Tuple * tuple, int n_subtunes, const int * subtunes)
{
    pthread_mutex_lock (& mutex);

    g_free (tuple->subtunes);
    tuple->subtunes = NULL;

    tuple->nsubtunes = n_subtunes;
    if (subtunes)
        tuple->subtunes = g_memdup (subtunes, sizeof (int) * n_subtunes);

    pthread_mutex_unlock (& mutex);
}

EXPORT int tuple_get_n_subtunes (Tuple * tuple)
{
    pthread_mutex_lock (& mutex);

    int n_subtunes = tuple->nsubtunes;

    pthread_mutex_unlock (& mutex);
    return n_subtunes;
}

EXPORT int tuple_get_nth_subtune (Tuple * tuple, int n)
{
    pthread_mutex_lock (& mutex);

    int subtune = -1;
    if (n >= 0 && n < tuple->nsubtunes)
        subtune = tuple->subtunes ? tuple->subtunes[n] : 1 + n;

    pthread_mutex_unlock (& mutex);
    return subtune;
}

EXPORT char * tuple_format_title (Tuple * tuple, const char * format)
{
    static const gint fallbacks[] = {FIELD_TITLE, FIELD_FILE_NAME, FIELD_FILE_PATH};

    char * title = tuple_formatter_process_string (tuple, format);

    for (int i = 0; i < G_N_ELEMENTS (fallbacks); i ++)
    {
        if (title && title[0])
            break;

        str_unref (title);
        title = tuple_get_str (tuple, fallbacks[i], NULL);
    }

    return title ? title : str_get ("");
}
