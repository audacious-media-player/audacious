/*
 * tuple.h
 * Copyright 2007-2011 William Pitcock, Christian Birchinger, Matti Hämäläinen,
 *                     Giacomo Lozito, Eugene Zagidullin, and John Lindgren
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
 * @file tuple.h
 * @brief Basic Tuple handling API.
 */

#ifndef LIBAUDCORE_TUPLE_H
#define LIBAUDCORE_TUPLE_H

#include <libaudcore/core.h>

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

typedef enum {
    TUPLE_STRING,
    TUPLE_INT,
    TUPLE_UNKNOWN
} TupleValueType;

int tuple_field_by_name (const char * name);
const char * tuple_field_get_name (int field);
TupleValueType tuple_field_get_type (int field);

typedef struct _Tuple Tuple;

/* Creates a new, blank tuple with a reference count of one. */
Tuple * tuple_new (void);

/* Increments the reference count of <tuple> by one. */
Tuple * tuple_ref (Tuple * tuple);

/* Decrements the reference count of <tuple> by one.  If the reference count
 * drops to zero, releases all memory used by <tuple>. */
void tuple_unref (Tuple * tuple);

/* Makes a copy of <tuple>.  Only use tuple_copy() if you need to modify one
 * copy of the tuple while not modifying the other.  In most cases, tuple_ref()
 * is more appropriate. */
Tuple *tuple_copy(const Tuple *);

/* Parses the URI <filename> and sets FIELD_FILE_NAME, FIELD_FILE_PATH,
 * FIELD_FILE_EXT, and FIELD_SUBSONG_ID accordingly. */
void tuple_set_filename(Tuple *tuple, const char *filename);

/* Convenience function, equivalent to calling tuple_new() and then
 * tuple_set_filename(). */
Tuple *tuple_new_from_filename(const char *filename);

/* Sets a field to the integer value <x>. */
void tuple_set_int (Tuple * tuple, int nfield, const char * field, int x);

/* Sets the field specified by <nfield> (one of the FIELD_* constants) or
 * <field> (one of the names returned by tuple_field_get_name() to the string
 * value <str>.  Only one of <nfield> or <field> may be set.  If <nfield> is
 * set, <field> must be NULL; if <field> is set, <nfield> must be -1.  As a
 * special case, if <str> is NULL, the result is equivalent to calling
 * tuple_unset(). */
void tuple_set_str (Tuple * tuple, int nfield, const char * field, const char * str);

/* Clears any value that a field is currently set to. */
void tuple_unset (Tuple * tuple, int nfield, const char * field);

/* Returns the value type of a field, or TUPLE_UNKNOWN if the field has not been
 * set to any value. */
TupleValueType tuple_get_value_type (const Tuple * tuple, int nfield,
 const char * field);

/* Returns the string value of a field.  The returned string is pooled and must
 * be released with str_unref() when no longer needed.  If the field has not
 * been set to any value, returns NULL. */
char * tuple_get_str (const Tuple * tuple, int nfield, const char * field);

/* Returns the integer value of a field.  If the field has not been set to any
 * value, returns 0.  (In hindsight, it would have been better to return -1 in
 * this case.  If you need to distinguish between a value of 0 and a field not
 * set to any value, use tuple_get_value_type().) */
int tuple_get_int (const Tuple * tuple, int nfield, const char * field);

/* Fills in format-related fields (specifically FIELD_CODEC, FIELD_QUALITY, and
 * FIELD_BITRATE).  Plugins should use this function instead of setting these
 * fields individually so that the style is consistent across file formats.
 * <format> should be a brief description such as "Microsoft WAV", "MPEG-1 layer
 * 3", "Audio CD", and so on.  <samplerate> is in Hertz.  <bitrate> is in 1000
 * bits per second. */
void tuple_set_format (Tuple * tuple, const char * format, int channels, int
 samplerate, int bitrate);

/* In addition to the normal fields, tuples contain an integer array of subtune
 * ID numbers.  This function sets that array.  It also sets FIELD_SUBSONG_NUM
 * to the value <n_subtunes>. */
void tuple_set_subtunes (Tuple * tuple, int n_subtunes, const int * subtunes);

/* Returns the length of the subtune array.  If the array has not been set,
 * returns zero.  Note that if FIELD_SUBSONG_NUM is changed after
 * tuple_set_subtunes() is called, this function returns the value <n_subtunes>
 * passed to tuple_set_subtunes(), not the value of FIELD_SUBSONG_NUM. */
int tuple_get_n_subtunes (Tuple * tuple);

/* Returns the <n>th member of the subtune array. */
int tuple_get_nth_subtune (Tuple * tuple, int n);

/* Generates a formatted title string for <tuple> according to <format>.  The
 * syntax of <format> is documented in tuple_formatter.c.  The returned string
 * is pooled and must be released with str_unref() when no longer need.  The
 * returned string is never NULL, though it may be the empty string. */
char * tuple_format_title (Tuple * tuple, const char * format);

#endif /* LIBAUDCORE_TUPLE_H */
