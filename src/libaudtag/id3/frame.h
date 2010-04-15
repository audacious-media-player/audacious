/*
 * Copyright 2009 Paula Stanciu
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUD_ID3_FRAME
#define AUD_ID3_FRAME

#include <glib-2.0/glib.h>

enum {
    ID3_ALBUM = 0,
    ID3_TITLE,
    ID3_COMPOSER,
    ID3_COPYRIGHT,
    ID3_DATE,
    ID3_TIME,
    ID3_LENGTH,
    ID3_ARTIST,
    ID3_TRACKNR,
    ID3_YEAR,
    ID3_GENRE,
    ID3_COMMENT,
    ID3_PRIVATE,
    ID3_ENCODER,
    ID3_RECORDING_TIME,
    ID3_TAGS_NO
};

enum {
    GENRE_BLUES = 0,
    GENRE_CLASSIC_ROCK,
    GENRE_COUNTRY,
    GENRE_DANCE,
    GENRE_DISCO,
    GENRE_FUNK,
    GENRE_GRUNGE,
    GENRE_HIPHOP,
    GENRE_JAZZ,
    GENRE_METAL,
    GENRE_NEW_AGE,
    GENRE_OLDIES,
    GENRE_OTHER,
    GENRE_POP,
    GENRE_R_B,
    GENRE_RAP,
    GENRE_REGGAE,
    GENRE_ROCK,
    GENRE_TECHNO,
    GENRE_INDUSTRIAL,
    GENRE_ALTERNATIVE,
    GENRE_SKA,
    GENRE_DEATH_METAL,
    GENRE_PRANKS,
    GENRE_SOUNDTRACK,
    GENRE_EURO_TECHNO,
    GENRE_AMBIENT,
    GENRE_TRIP_HOP,
    GENRE_VOCAL,
    GENRE_JAZZ_FUNK,
    GENRE_FUSION,
    GENRE_TRANCE,
    GENRE_CLASSICAL,
    GENRE_INSTRUMENTAL,
    GENRE_ACID,
    GENRE_HOUSE,
    GENRE_GAME,
    GENRE_SOUND_CLIP,
    GENRE_GOSPEL,
    GENRE_NOISE,
    GENRE_ALTERNROCK,
    GENRE_BASS,
    GENRE_SOUL,
    GENRE_PUNK,
    GENRE_SPACE,
    GENRE_MEDITATIVE,
    GENRE_INSTRUMENTAL_POP,
    GENRE_INSTRUMENTAL_ROCK,
    GENRE_ETHNIC,
    GENRE_GOTHIC,
    GENRE_DARKWAVE,
    GENRE_TECHNO_INDUSTRIAL,
    GENRE_ELECTRONIC,
    GENRE_POP_FOLK,
    GENRE_EURODANCE,
    GENRE_DREAM,
    GENRE_SOUTHERN_ROCK,
    GENRE_COMEDY,
    GENRE_CULT,
    GENRE_GANGSTA,
    GENRE_TOP40,
    GENRE_CHRISTIAN_RAP,
    GENRE_POP_FUNK,
    GENRE_JUNGLE,
    GENRE_NATIVE_AMERICAN,
    GENRE_CABARET,
    GENRE_NEW_WAVE,
    GENRE_PSYCHADELIC,
    GENRE_RAVE,
    GENRE_SHOWTUNES,
    GENRE_TRAILER,
    GENRE_LO_FI,
    GENRE_TRIBAL,
    GENRE_ACID_PUNK,
    GENRE_ACID_JAZZ,
    GENRE_POLKA,
    GENRE_RETRO,
    GENRE_MUSICAL,
    GENRE_ROCK_ROLL,
    GENRE_HARD_ROCK
};

char * id3_frames[] = {"TALB","TIT2","TCOM", "TCOP", "TDAT", "TIME", "TLEN",
"TPE1", "TRCK", "TYER","TCON", "COMM", "PRIV", "TSSE","TDRC"};

#endif
