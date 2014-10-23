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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>  /* for g_utf8_validate */

#include "audstrings.h"
#include "i18n.h"
#include "tuple.h"
#include "vfs.h"

#if TUPLE_FIELDS > 64
#error The current tuple implementation is limited to 64 fields
#endif

#define BIT(i) ((uint64_t) 1 << (i))

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
struct TupleData
{
    uint64_t setmask;      // which fields are present
    Index<TupleVal> vals;  // ordered list of field values

    int *subtunes;                 /**< Array of int containing subtune index numbers.
                                         Can be nullptr if indexing is linear or if
                                         there are no subtunes. */
    int nsubtunes;                 /**< Number of subtunes, if any. Values greater than 0
                                         mean that there are subtunes and #subtunes array
                                         may be set. */

    int refcount;

    TupleData ();
    ~TupleData ();

    TupleData (const TupleData & other);
    void operator= (const TupleData & other) = delete;

    TupleVal * lookup (int field, bool add, bool remove);
    void set_subtunes (int nsubs, const int * subs);

    static TupleData * ref (TupleData * tuple);
    static void unref (TupleData * tuple);

    static TupleData * copy_on_write (TupleData * tuple);
};

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

    { "album-artist",   TUPLE_STRING },
    { "composer",       TUPLE_STRING },
    { "performer",      TUPLE_STRING },
    { "copyright",      TUPLE_STRING },
    { "date",           TUPLE_STRING },
    { "mbid",           TUPLE_STRING },

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
 {"album-artist", FIELD_ALBUM_ARTIST},
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
 {"mbid", FIELD_MBID},
 {"mime-type", FIELD_MIMETYPE},
 {"performer", FIELD_PERFORMER},
 {"quality", FIELD_QUALITY},
 {"segment-end", FIELD_SEGMENT_END},
 {"segment-start", FIELD_SEGMENT_START},
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
    assert (VALID_FIELD (field));
    return tuple_fields[field].name;
}

EXPORT TupleValueType Tuple::field_get_type (int field)
{
    assert (VALID_FIELD (field));
    return tuple_fields[field].type;
}

TupleVal * TupleData::lookup (int field, bool add, bool remove)
{
    /* calculate number of preceding fields */
    int pos = bitcount (setmask & (BIT (field) - 1));

    if ((setmask & BIT (field)))
    {
        if ((add || remove) && FIELD_TYPE (field) == TUPLE_STRING)
            vals[pos].str.~String ();

        if (remove)
        {
            setmask &= ~BIT (field);
            vals.remove (pos, 1);
            return nullptr;
        }

        return & vals[pos];
    }

    if (! add)
        return nullptr;

    setmask |= BIT (field);
    vals.insert (pos, 1);
    return & vals[pos];
}

void TupleData::set_subtunes (int nsubs, const int * subs)
{
    nsubtunes = nsubs;

    delete subtunes;
    subtunes = nullptr;

    if (subs)
    {
        subtunes = new int[nsubs];
        memcpy (subtunes, subs, sizeof (int) * nsubs);
    }
}

TupleData::TupleData () :
    setmask (0),
    subtunes (nullptr),
    nsubtunes (0),
    refcount (1) {}

TupleData::TupleData (const TupleData & other) :
    setmask (other.setmask),
    subtunes (nullptr),
    nsubtunes (0),
    refcount (1)
{
    vals.insert (0, other.vals.len ());

    auto get = other.vals.begin ();
    auto set = vals.begin ();

    for (int f = 0; f < TUPLE_FIELDS; f ++)
    {
        if (other.setmask & BIT (f))
        {
            if (FIELD_TYPE (f) == TUPLE_STRING)
                new (& set->str) String (get->str);
            else
                set->x = get->x;

            get ++;
            set ++;
        }
    }

    set_subtunes (other.nsubtunes, other.subtunes);
}

TupleData::~TupleData ()
{
    auto iter = vals.begin ();

    for (int f = 0; f < TUPLE_FIELDS; f ++)
    {
        if (setmask & BIT (f))
        {
            if (FIELD_TYPE (f) == TUPLE_STRING)
                iter->str.~String ();

            iter ++;
        }
    }

    delete[] subtunes;
}

TupleData * TupleData::ref (TupleData * tuple)
{
    if (tuple)
        __sync_fetch_and_add (& tuple->refcount, 1);

    return tuple;
}

void TupleData::unref (TupleData * tuple)
{
    if (tuple && ! __sync_sub_and_fetch (& tuple->refcount, 1))
        delete tuple;
}

TupleData * TupleData::copy_on_write (TupleData * tuple)
{
    if (! tuple)
        return new TupleData;

    if (__sync_fetch_and_add (& tuple->refcount, 0) == 1)
        return tuple;

    TupleData * copy = new TupleData (* tuple);
    unref (tuple);
    return copy;
}

EXPORT Tuple::~Tuple ()
{
    TupleData::unref (data);
}

EXPORT Tuple Tuple::ref () const
{
    Tuple tuple;
    tuple.data = TupleData::ref (data);
    return tuple;
}

EXPORT TupleValueType Tuple::get_value_type (int field) const
{
    assert (VALID_FIELD (field));

    TupleVal * val = data ? data->lookup (field, false, false) : nullptr;
    return val ? FIELD_TYPE (field) : TUPLE_UNKNOWN;
}

EXPORT int Tuple::get_int (int field) const
{
    assert (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_INT);

    TupleVal * val = data ? data->lookup (field, false, false) : nullptr;
    return val ? val->x : -1;
}

EXPORT String Tuple::get_str (int field) const
{
    assert (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_STRING);

    TupleVal * val = data ? data->lookup (field, false, false) : nullptr;
    return val ? val->str : String ();
}

EXPORT void Tuple::set_int (int field, int x)
{
    assert (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_INT);

    data = TupleData::copy_on_write (data);
    TupleVal * val = data->lookup (field, true, false);
    val->x = x;
}

EXPORT void Tuple::set_str (int field, const char * str)
{
    if (! str)
    {
        unset (field);
        return;
    }

    assert (VALID_FIELD (field) && FIELD_TYPE (field) == TUPLE_STRING);

    data = TupleData::copy_on_write (data);
    TupleVal * val = data->lookup (field, true, false);

    if (g_utf8_validate (str, -1, nullptr))
        new (& val->str) String (str);
    else
    {
        StringBuf utf8 = str_to_utf8 (str);
        new (& val->str) String (utf8 ? (const char *) utf8 : "(character encoding error)");
    }
}

EXPORT void Tuple::unset (int field)
{
    assert (VALID_FIELD (field));

    if (! data)
        return;

    data = TupleData::copy_on_write (data);
    data->lookup (field, false, true);
}

EXPORT void Tuple::set_filename (const char * filename)
{
    assert (filename);

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
    data = TupleData::copy_on_write (data);
    data->set_subtunes (n_subtunes, subtunes);
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

EXPORT bool Tuple::fetch_stream_info (VFSFile & stream)
{
    bool updated = false;
    int value;

    String val = stream.get_metadata ("track-name");

    if (val && val != get_str (FIELD_TITLE))
    {
        set_str (FIELD_TITLE, val);
        updated = true;
    }

    val = stream.get_metadata ("stream-name");

    if (val && val != get_str (FIELD_ARTIST))
    {
        set_str (FIELD_ARTIST, val);
        updated = true;
    }

    val = stream.get_metadata ("content-bitrate");
    value = val ? atoi (val) / 1000 : 0;

    if (value && value != get_int (FIELD_BITRATE))
    {
        set_int (FIELD_BITRATE, value);
        updated = true;
    }

    return updated;
}
