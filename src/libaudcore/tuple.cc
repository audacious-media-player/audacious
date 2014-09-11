/*
 * tuple.c
 * Copyright 2007-2013 William Pitcock, Christian Birchinger, Matti Hämäläinen,
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

#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audstrings.h"
#include "i18n.h"
#include "tuple.h"
#include "vfs.h"

#if TUPLE_FIELDS > 64
#error The current tuple implementation is limited to 64 fields
#endif

#define BLOCK_VALS 4

struct TupleBasicType {
    const char *name;
    TupleValueType type;
};

union TupleVal
{
    String str;
    int x;

    // dummy constructor and destructor
    TupleVal () {}
    ~TupleVal () {}
};

/**
 * Structure for holding and passing around miscellaneous track
 * metadata. This is not the same as a playlist entry, though.
 */
struct TupleData {
    uint64_t setmask;      // which fields are present
    Index<TupleVal> vals;  // ordered list of field values

    int *subtunes;                 /**< Array of int containing subtune index numbers.
                                         Can be nullptr if indexing is linear or if
                                         there are no subtunes. */
    int nsubtunes;                 /**< Number of subtunes, if any. Values greater than 0
                                         mean that there are subtunes and #subtunes array
                                         may be set. */

    int refcount;
};

#define BIT(i) ((uint64_t) 1 << (i))

#define LOCK(t) tiny_lock ((TinyLock *) & t->lock)
#define UNLOCK(t) tiny_unlock ((TinyLock *) & t->lock)

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

struct FieldDictEntry {
    const char * name;
    int field;
};

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

#define VALID_FIELD(f) ((f) >= 0 && (f) < TUPLE_FIELDS)
#define FIELD_TYPE(f) (tuple_fields[f].type)

static int bitcount (uint64_t x)
{
    /* algorithm from http://en.wikipedia.org/wiki/Hamming_weight */
    x -= (x >> 1) & 0x5555555555555555;
    x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
    return (x * 0x0101010101010101) >> 56;
}

static int field_dict_compare (const void * a, const void * b)
{
    return strcmp (((FieldDictEntry *) a)->name, ((FieldDictEntry *) b)->name);
}

EXPORT int Tuple::field_by_name (const char * name)
{
    FieldDictEntry find = {name, -1};
    FieldDictEntry * found = (FieldDictEntry *) bsearch (& find, field_dict,
     TUPLE_FIELDS, sizeof (FieldDictEntry), field_dict_compare);

    return found ? found->field : -1;
}

EXPORT const char * Tuple::field_get_name (int field)
{
    g_return_val_if_fail (VALID_FIELD (field), nullptr);
    return tuple_fields[field].name;
}

EXPORT TupleValueType Tuple::field_get_type (int field)
{
    g_return_val_if_fail (VALID_FIELD (field), TUPLE_UNKNOWN);
    return tuple_fields[field].type;
}

static TupleVal * lookup_val (TupleData * tuple, int field, bool add, bool remove)
{
    /* calculate number of preceding fields */
    int pos = bitcount (tuple->setmask & (BIT (field) - 1));

    if ((tuple->setmask & BIT (field)))
    {
        if (remove)
        {
            if (FIELD_TYPE (field) == TUPLE_STRING)
                tuple->vals[pos].str.~String ();

            tuple->setmask &= ~BIT (field);
            tuple->vals.remove (pos, 1);
            return nullptr;
        }

        return & tuple->vals[pos];
    }

    if (! add)
        return nullptr;

    tuple->setmask |= BIT (field);
    tuple->vals.insert (pos, 1);
    return & tuple->vals[pos];
}

static TupleData * tuple_new ()
{
    TupleData * tuple = g_slice_new0 (TupleData);
    tuple->refcount = 1;
    return tuple;
}

static TupleData * tuple_copy (const TupleData * old)
{
    TupleData * tuple = tuple_new ();

    tuple->setmask = old->setmask;
    tuple->vals.insert (0, old->vals.len ());

    auto get = old->vals.begin ();
    auto set = tuple->vals.begin ();

    for (int f = 0; f < TUPLE_FIELDS; f ++)
    {
        if (old->setmask & BIT (f))
        {
            if (FIELD_TYPE (f) == TUPLE_STRING)
                set->str = get->str;
            else
                set->x = get->x;

            get ++;
            set ++;
        }
    }

    tuple->nsubtunes = old->nsubtunes;

    if (old->subtunes)
        tuple->subtunes = (int *) g_memdup (old->subtunes, sizeof (int) * old->nsubtunes);

    return tuple;
}

static void tuple_destroy (TupleData * tuple)
{
    auto iter = tuple->vals.begin ();

    for (int f = 0; f < TUPLE_FIELDS; f ++)
    {
        if (tuple->setmask & BIT (f))
        {
            if (FIELD_TYPE (f) == TUPLE_STRING)
                iter->str.~String ();

            iter ++;
        }
    }

    tuple->vals.clear ();

    g_free (tuple->subtunes);
    g_slice_free (TupleData, tuple);
}

static TupleData * tuple_ref (TupleData * tuple)
{
    if (tuple)
        __sync_fetch_and_add (& tuple->refcount, 1);

    return tuple;
}

static void tuple_unref (TupleData * tuple)
{
    if (tuple && ! __sync_sub_and_fetch (& tuple->refcount, 1))
        tuple_destroy (tuple);
}

static TupleData * tuple_get_writable (TupleData * tuple)
{
    if (! tuple)
        return tuple_new ();

    if (__sync_fetch_and_add (& tuple->refcount, 0) == 1)
        return tuple;

    TupleData * copy = tuple_copy (tuple);
    tuple_unref (tuple);
    return copy;
}

EXPORT Tuple::~Tuple ()
{
    tuple_unref (data);
}

EXPORT Tuple Tuple::ref () const
{
    Tuple tuple;
    tuple.data = tuple_ref (data);
    return tuple;
}

EXPORT TupleValueType Tuple::get_value_type (int field) const
{
    g_return_val_if_fail (VALID_FIELD (field), TUPLE_UNKNOWN);

    TupleVal * val = data ? lookup_val (data, field, false, false) : nullptr;
    return val ? FIELD_TYPE (field) : TUPLE_UNKNOWN;
}

EXPORT int Tuple::get_int (int field) const
{
    g_return_val_if_fail (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_INT, -1);

    TupleVal * val = data ? lookup_val (data, field, false, false) : nullptr;
    return val ? val->x : -1;
}

EXPORT String Tuple::get_str (int field) const
{
    g_return_val_if_fail (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_STRING, String ());

    TupleVal * val = data ? lookup_val (data, field, false, false) : nullptr;
    return val ? val->str : String ();
}

EXPORT void Tuple::set_int (int field, int x)
{
    g_return_if_fail (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_INT);

    data = tuple_get_writable (data);
    TupleVal * val = lookup_val (data, field, true, false);
    val->x = x;
}

EXPORT void Tuple::set_str (int field, const char * str)
{
    if (! str)
    {
        unset (field);
        return;
    }

    g_return_if_fail (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_STRING);

    data = tuple_get_writable (data);
    TupleVal * val = lookup_val (data, field, true, false);

    if (g_utf8_validate (str, -1, nullptr))
        val->str = String (str);
    else
    {
        StringBuf utf8 = str_to_utf8 (str);
        val->str = String (utf8 ? (const char *) utf8 : "(character encoding error)");
    }
}

EXPORT void Tuple::unset (int field)
{
    g_return_if_fail (VALID_FIELD (field));

    if (! data)
        return;

    data = tuple_get_writable (data);
    lookup_val (data, field, false, true);
}

EXPORT void Tuple::set_filename (const char * filename)
{
    const char * base, * ext, * sub;
    int isub;

    uri_parse (filename, & base, & ext, & sub, & isub);

    set_str (FIELD_FILE_PATH, str_decode_percent (filename, base - filename));
    set_str (FIELD_FILE_NAME, str_decode_percent (base, ext - base));

    if (ext < sub)
        set_str (FIELD_FILE_EXT, str_decode_percent (ext + 1, sub - ext - 1));

    if (sub[0])
        set_int (FIELD_SUBSONG_ID, isub);
}

EXPORT void Tuple::set_format (const char * format, int chans, int rate, int brate)
{
    if (format)
        set_str (FIELD_CODEC, format);

    StringBuf buf;

    if (chans > 0)
    {
        if (chans == 1)
            str_insert (buf, -1, _("Mono"));
        else if (chans == 2)
            str_insert (buf, -1, _("Stereo"));
        else
            buf.combine (str_printf (dngettext (PACKAGE, "%d channel", "%d channels", chans), chans));

        if (rate > 0)
            str_insert (buf, -1, ", ");
    }

    if (rate > 0)
        buf.combine (str_printf ("%d kHz", rate / 1000));

    if (buf[0])
        set_str (FIELD_QUALITY, buf);

    if (brate > 0)
        set_int (FIELD_BITRATE, brate);
}

EXPORT void Tuple::set_subtunes (int n_subtunes, const int * subtunes)
{
    data = tuple_get_writable (data);

    g_free (data->subtunes);
    data->subtunes = nullptr;

    data->nsubtunes = n_subtunes;
    if (subtunes)
        data->subtunes = (int *) g_memdup (subtunes, sizeof (int) * n_subtunes);
}

EXPORT int Tuple::get_n_subtunes () const
{
    return data ? data->nsubtunes : 0;
}

EXPORT int Tuple::get_nth_subtune (int n) const
{
    if (! data || n < 0 || n >= data->nsubtunes)
        return -1;

    return data->subtunes ? data->subtunes[n] : 1 + n;
}

EXPORT bool Tuple::fetch_stream_info (VFSFile * stream)
{
    bool updated = false;
    int value;

    String val = vfs_get_metadata (stream, "track-name");

    if (val && val != get_str (FIELD_TITLE))
    {
        set_str (FIELD_TITLE, val);
        updated = true;
    }

    val = vfs_get_metadata (stream, "stream-name");

    if (val && val != get_str (FIELD_ARTIST))
    {
        set_str (FIELD_ARTIST, val);
        updated = true;
    }

    val = vfs_get_metadata (stream, "content-bitrate");
    value = val ? atoi (val) / 1000 : 0;

    if (value && value != get_int (FIELD_BITRATE))
    {
        set_int (FIELD_BITRATE, value);
        updated = true;
    }

    return updated;
}
