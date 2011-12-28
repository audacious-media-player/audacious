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

#ifndef TAGUTIL_H

#define TAGUTIL_H

#include <stdint.h>
#include <time.h>

#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"

#define BROKEN 1

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
    GENRE_PSYCHEDELIC,
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
    GENRE_HARD_ROCK,
    GENRE_FOLK,
    GENRE_FOLK_ROCK,
    GENRE_NATIONAL_FOLK,
    GENRE_SWING,
    GENRE_FAST_FUSION,
    GENRE_BEBOB,
    GENRE_LATIN,
    GENRE_REVIVAL,
    GENRE_CELTIC,
    GENRE_BLUEGRASS,
    GENRE_AVANTGARDE,
    GENRE_GOTHIC_ROCK,
    GENRE_PROGRESSIVE_ROCK,
    GENRE_PSYCHEDELIC_ROCK,
    GENRE_SYMPHONIC_ROCK,
    GENRE_SLOW_ROCK,
    GENRE_BIG_BAND,
    GENRE_CHORUS,
    GENRE_EASY_LISTENING,
    GENRE_ACOUSTIC,
    GENRE_HUMOUR,
    GENRE_SPEECH,
    GENRE_CHANSON,
    GENRE_OPERA,
    GENRE_CHAMBER_MUSIC,
    GENRE_SONATA,
    GENRE_SYMPHONY,
    GENRE_BOOTY_BASS,
    GENRE_PRIMUS,
    GENRE_PORN_GROOVE,
    GENRE_SATIRE,
    GENRE_SLOW_JAM,
    GENRE_CLUB,
    GENRE_TANGO,
    GENRE_SAMBA,
    GENRE_FOLKLORE,
    GENRE_BALLAD,
    GENRE_POWER_BALLAD,
    GENRE_RHYTHMIC_SOUL,
    GENRE_FREESTYLE,
    GENRE_DUET,
    GENRE_PUNK_ROCK,
    GENRE_DRUM_SOLO,
    GENRE_A_CAPELLA,
    GENRE_EURO_HOUSE
};

extern bool_t tag_verbose;

#define TAGDBG(...) do {if (tag_verbose) {printf ("%s:%d [%s]: ", __FILE__, __LINE__, __FUNCTION__); printf (__VA_ARGS__);}} while (0)

time_t unix_time(uint64_t win_time);

uint16_t get_year(uint64_t win_time);

char *read_char_data(VFSFile *fd, int size);
bool_t write_char_data(VFSFile *f, char *data, size_t i);

uint8_t read_uint8(VFSFile *fd);
uint16_t read_LEuint16(VFSFile *fd);
uint16_t read_BEuint16(VFSFile *fd);
uint32_t read_LEuint32(VFSFile *fd);
uint32_t read_BEuint32(VFSFile *fd);
uint64_t read_LEuint64(VFSFile *fd);
uint64_t read_BEuint64(VFSFile *fd);


bool_t write_uint8(VFSFile *fd, uint8_t val);
bool_t write_BEuint16(VFSFile *fd, uint16_t val);
bool_t write_LEuint16(VFSFile *fd, uint16_t val);
bool_t write_BEuint32(VFSFile *fd, uint32_t val);
bool_t write_LEuint32(VFSFile *fd, uint32_t val);
bool_t write_BEuint64(VFSFile *fd, uint64_t val);
bool_t write_LEuint64(VFSFile *fd, uint64_t val);

uint64_t read_LEint64(VFSFile *fd);
bool_t cut_beginning_tag (VFSFile * handle, int64_t tag_size);

char *convert_numericgenre_to_text(int numericgenre);

uint32_t unsyncsafe32 (uint32_t x);
uint32_t syncsafe32 (uint32_t x);

#endif
