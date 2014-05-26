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

/** Ordered enum for basic #Tuple fields.
 * @sa TupleBasicType
 */
enum {
    FIELD_ARTIST = 0,
    FIELD_TITLE,        /**< Song title */
    FIELD_ALBUM,        /**< Album name */
    FIELD_COMMENT,      /**< Freeform comment */
    FIELD_GENRE,        /**< Song's genre */

    FIELD_TRACK_NUMBER,
    FIELD_LENGTH,       /**< Track length in milliseconds */
    FIELD_YEAR,         /**< Year of production/performance/etc */
    FIELD_QUALITY,      /**< String representing quality, such as
                             "lossy", "lossless", "sequenced"  */

    FIELD_CODEC,        /**< Codec name or similar */
    FIELD_FILE_NAME,    /**< File name part of the location URI */
    FIELD_FILE_PATH,    /**< Path part of the location URI */
    FIELD_FILE_EXT,     /**< Filename extension part of the location URI */

    FIELD_SONG_ARTIST,
    FIELD_COMPOSER,     /**< Composer of song, if different than artist. */
    FIELD_PERFORMER,
    FIELD_COPYRIGHT,
    FIELD_DATE,

    FIELD_SUBSONG_ID,   /**< Index number of subsong/tune */
    FIELD_SUBSONG_NUM,  /**< Total number of subsongs in the file */
    FIELD_MIMETYPE,
    FIELD_BITRATE,      /**< Bitrate in kbps */

    FIELD_SEGMENT_START,
    FIELD_SEGMENT_END,

    /* Preserving replay gain information accurately is a challenge since there
     * are several differents formats around.  We use an integer fraction, with
     * the denominator stored in the *_UNIT fields.  For example, if ALBUM_GAIN
     * is 512 and GAIN_UNIT is 256, then the album gain is +2 dB.  If TRACK_PEAK
     * is 787 and PEAK_UNIT is 1000, then the peak volume is 0.787 in a -1.0 to
     * 1.0 range. */
    FIELD_GAIN_ALBUM_GAIN,
    FIELD_GAIN_ALBUM_PEAK,
    FIELD_GAIN_TRACK_GAIN,
    FIELD_GAIN_TRACK_PEAK,
    FIELD_GAIN_GAIN_UNIT,
    FIELD_GAIN_PEAK_UNIT,

    TUPLE_FIELDS
};

enum TupleValueType {
    TUPLE_STRING,
    TUPLE_INT,
    TUPLE_UNKNOWN
};

struct TupleData;

/* Smart pointer to the actual TupleData struct.
 * Uses create-on-write and copy-on-write. */

class Tuple
{
public:
    static int field_by_name (const char * name);
    static const char * field_get_name (int field);
    static TupleValueType field_get_type (int field);

    constexpr Tuple () :
        data (nullptr) {}

    ~Tuple ();

    Tuple (Tuple && b) :
        data (b.data)
    {
        b.data = nullptr;
    }

    void operator= (Tuple && b)
    {
        if (this != & b)
        {
            this->~Tuple ();
            data = b.data;
            b.data = nullptr;
        }
    }

    explicit operator bool () const
        { return (bool) data; }

    Tuple ref () const;

    /* Returns the value type of a field if set, otherwise TUPLE_UNKNOWN. */
    TupleValueType get_value_type (int field) const;

    /* Returns the integer value of a field if set, otherwise -1.  If you need
     * to distinguish between a value of -1 and an unset value, use
     * get_value_type(). */
    int get_int (int field) const;

    /* Returns the string value of a field if set, otherwise null. */
    String get_str (int field) const;

    /* Sets a field to the integer value <x>. */
    void set_int (int field, int x);

    /* Sets a field to the string value <str>.  If <str> is not valid UTF-8, it
     * will be converted according to the user's character set detection rules.
     * Equivalent to unset() if <str> is null. */
    void set_str (int field, const char * str);

    /* Clears any value that a field is currently set to. */
    void unset (int field);

    /* Parses the URI <filename> and sets FIELD_FILE_NAME, FIELD_FILE_PATH,
     * FIELD_FILE_EXT, and FIELD_SUBSONG_ID accordingly. */
    void set_filename (const char * filename);

    /* Fills in format-related fields (specifically FIELD_CODEC, FIELD_QUALITY,
     * and FIELD_BITRATE).  Plugins should use this function instead of setting
     * these fields individually to allow a consistent style across file
     * formats.  <format> should be a brief description such as "Microsoft WAV",
     * "MPEG-1 layer 3", "Audio CD", and so on.  <samplerate> is in Hertz.
     * <bitrate> is in (decimal) kbps. */
    void set_format (const char * format, int channels, int samplerate, int bitrate);

    /* In addition to the normal fields, tuples contain an integer array of
     * subtune ID numbers.  This function sets that array.  It also sets
     * FIELD_SUBSONG_NUM to the value <n_subtunes>. */
    void set_subtunes (int n_subtunes, const int * subtunes);

    /* Returns the length of the subtune array.  If the array has not been set,
     * returns zero.  Note that if FIELD_SUBSONG_NUM is changed after
     * set_subtunes() is called, this function returns the value <n_subtunes>
     * passed to set_subtunes(), not the value of FIELD_SUBSONG_NUM. */
    int get_n_subtunes () const;

    /* Returns the <n>th member of the subtune array. */
    int get_nth_subtune (int n) const;

private:
    TupleData * data;
};

/* somewhat out of place here */
struct PluginHandle;
struct PlaylistAddItem {
    String filename;
    Tuple tuple;
    PluginHandle * decoder;
};

#endif /* LIBAUDCORE_TUPLE_H */
