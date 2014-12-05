/*
 * tuple.h
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

/**
 * @file tuple.h
 * @brief Basic Tuple handling API.
 */

#ifndef LIBAUDCORE_TUPLE_H
#define LIBAUDCORE_TUPLE_H

#include <libaudcore/objects.h>

struct ReplayGainInfo;
struct TupleData;
class VFSFile;

class Tuple
{
public:
    /* Smart pointer to the actual TupleData struct.
     * Uses create-on-write and copy-on-write. */

    enum Field {
        Invalid = -1,

        Title = 0,    /* Song title */
        Artist,       /* Song artist */
        Album,        /* Album name */
        Comment,      /* Freeform comment */
        Genre,        /* Song's genre */

        Track,        /* Track number */
        Length,       /* Track length in milliseconds */
        Year,         /* Year of production, performance, etc. */
        Quality,      /* String representing quality, such as "Stereo, 44 kHz" */
        Codec,        /* Codec name, such as "Ogg Vorbis" */

        Basename,     /* Base filename, not including the folder path */
        Path,         /* Folder path, including the trailing "/" */
        Suffix,       /* Filename extension, not including the "." */

        AlbumArtist,  /* Artist for entire album, if different than song artist */
        Composer,     /* Composer of song, if different than artist */
        Performer,
        Copyright,
        Date,
        MusicBrainz,  /* MusicBrainz identifer for the song */
        MIMEType,
        Bitrate,      /* Bitrate in kbits/sec */

        Subtune,      /* Index number of subtune */
        NumSubtunes,  /* Total number of subtunes in the file */

        StartTime,
        EndTime,

        /* Preserving replay gain information accurately is a challenge since there
         * are several differents formats around.  We use an integer fraction, with
         * the denominator stored in the *Divisor fields.  For example, if AlbumGain
         * is 512 and GainDivisor is 256, then the album gain is +2 dB.  If TrackPeak
         * is 787 and PeakDivisor is 1000, then the peak volume is 0.787 in a -1.0 to
         * 1.0 range. */
        AlbumGain,
        AlbumPeak,
        TrackGain,
        TrackPeak,
        GainDivisor,
        PeakDivisor,

        /* Title formatted for display; input plugins do not need to set this field */
        FormattedTitle,

        n_fields
    };

    typedef aud::range<Field, Title, FormattedTitle> all_fields;

    enum ValueType {
        String,
        Int,
        Empty
    };

    static Field field_by_name (const char * name);
    static const char * field_get_name (Field field);
    static ValueType field_get_type (Field field);

    constexpr Tuple () :
        data (nullptr) {}

    ~Tuple ();

    Tuple (Tuple && b) :
        data (b.data)
    {
        b.data = nullptr;
    }

    Tuple & operator= (Tuple && b)
    {
        if (this != & b)
        {
            this->~Tuple ();
            data = b.data;
            b.data = nullptr;
        }
        return * this;
    }

    explicit operator bool () const
        { return (bool) data; }

    bool operator== (const Tuple & b) const;
    bool operator!= (const Tuple & b) const
        { return ! operator== (b); }

    Tuple ref () const;

    /* Returns the value type of a field if set, otherwise Empty. */
    ValueType get_value_type (Field field) const;

    /* Returns the integer value of a field if set, otherwise -1.  If you need
     * to distinguish between a value of -1 and an unset value, use
     * get_value_type(). */
    int get_int (Field field) const;

    /* Returns the string value of a field if set, otherwise null. */
    ::String get_str (Field field) const;

    /* Sets a field to the integer value <x>. */
    void set_int (Field field, int x);

    /* Sets a field to the string value <str>.  If <str> is not valid UTF-8, it
     * will be converted according to the user's character set detection rules.
     * Equivalent to unset() if <str> is null. */
    void set_str (Field field, const char * str);

    /* Clears any value that a field is currently set to. */
    void unset (Field field);

    /* Parses the URI <filename> and sets Basename, Path, Suffix, and Subtune accordingly. */
    void set_filename (const char * filename);

    /* Fills in format-related fields (specifically Codec, Quality,
     * and Bitrate).  Plugins should use this function instead of setting
     * these fields individually to allow a consistent style across file
     * formats.  <format> should be a brief description such as "Ogg Vorbis",
     * "MPEG-1 layer 3", "Audio CD", and so on.  <samplerate> is in Hertz.
     * <bitrate> is in (decimal) kbps. */
    void set_format (const char * format, int channels, int samplerate, int bitrate);

    /* In addition to the normal fields, tuples contain an integer array of
     * subtune ID numbers.  This function sets that array.  It also sets
     * NumSubtunes to the value <n_subtunes>. */
    void set_subtunes (int n_subtunes, const int * subtunes);

    /* Returns the length of the subtune array.  If the array has not been set,
     * returns zero.  Note that if NumSubtunes is changed after
     * set_subtunes() is called, this function returns the value <n_subtunes>
     * passed to set_subtunes(), not the value of NumSubtunes. */
    int get_n_subtunes () const;

    /* Returns the <n>th member of the subtune array. */
    int get_nth_subtune (int n) const;

    /* Fills ReplayGainInfo struct from various fields. */
    ReplayGainInfo get_replay_gain () const;

    /* Set various fields based on the ICY metadata of <stream>.  Returns true
     * if any fields were changed. */
    bool fetch_stream_info (VFSFile & stream);

    /* Guesses the song title, artist, and album, if not already set, from the
     * filename. */
    void generate_fallbacks ();

    /* Removes guesses made by generate_fallbacks().  This function should be
     * called, for example, before writing a song tag from the tuple. */
    void delete_fallbacks ();

private:
    TupleData * data;
};

/* somewhat out of place here */
class PluginHandle;
struct PlaylistAddItem {
    String filename;
    Tuple tuple;
    PluginHandle * decoder;
};

#endif /* LIBAUDCORE_TUPLE_H */
