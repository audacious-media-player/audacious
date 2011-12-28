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

#include <glib.h>
#include <stdio.h>

#include "util.h"
#include <inttypes.h>

/* convert windows time to unix time */
time_t unix_time(uint64_t win_time)
{
    uint64_t t = (uint64_t) ((win_time / 10000000LL) - 11644473600LL);
    return (time_t) t;
}

uint16_t get_year(uint64_t win_time)
{
    GDate *d = g_date_new();
    g_date_set_time_t(d, unix_time(win_time));
    uint16_t year = g_date_get_year(d);
    g_date_free(d);
    return year;
}

char * read_char_data (VFSFile * file, int size)
{
    char * value = g_malloc (size + 1);
    if (vfs_fread (value, 1, size, file) < size)
    {
        g_free (value);
        return NULL;
    }
    value[size] = 0;
    return value;
}

bool_t write_char_data(VFSFile * f, char * data, size_t i)
{
    return (vfs_fwrite(data, i, 1, f) == i);
}

uint8_t read_uint8(VFSFile * fd)
{
    uint16_t i;
    if (vfs_fread(&i, 1, 1, fd) == 1)
    {
        return i;
    }
    return -1;
}

uint16_t read_LEuint16(VFSFile * fd)
{
    uint16_t a;
    if (vfs_fget_le16(&a, fd))
        return a;
    else
        return -1;
}

uint16_t read_BEuint16(VFSFile * fd)
{
    uint16_t a;
    if (vfs_fget_be16(&a, fd))
        return a;
    else
        return -1;
}

uint32_t read_LEuint32(VFSFile * fd)
{
    uint32_t a;
    if (vfs_fget_le32(&a, fd))
        return a;
    else
        return -1;
}

uint32_t read_BEuint32(VFSFile * fd)
{
    uint32_t a;
    if (vfs_fget_be32(&a, fd))
        return a;
    else
        return -1;
}

uint64_t read_LEuint64(VFSFile * fd)
{
    uint64_t a;
    if (vfs_fget_le64(&a, fd))
        return a;
    else
        return -1;
}

uint64_t read_BEuint64(VFSFile * fd)
{
    uint64_t a;
    if (vfs_fget_be64(&a, fd))
        return a;
    else
        return 1;
}

bool_t write_uint8(VFSFile * fd, uint8_t val)
{
    return (vfs_fwrite(&val, 1, 1, fd) == 1);
}

bool_t write_LEuint16(VFSFile * fd, uint16_t val)
{
    uint16_t le_val = GUINT32_TO_LE(val);
    return (vfs_fwrite(&le_val, 2, 1, fd) == 2);
}

bool_t write_BEuint32(VFSFile * fd, uint32_t val)
{
    uint32_t be_val = GUINT32_TO_BE(val);
    return (vfs_fwrite(&be_val, 4, 1, fd) == 4);
}

bool_t write_LEuint32(VFSFile * fd, uint32_t val)
{
    uint32_t le_val = GUINT32_TO_LE(val);
    return (vfs_fwrite(&le_val, 4, 1, fd) == 4);
}

bool_t write_LEuint64(VFSFile * fd, uint64_t val)
{
    uint64_t le_val = GUINT64_TO_LE(val);
    return (vfs_fwrite(&le_val, 8, 1, fd) == 8);
}

bool_t cut_beginning_tag (VFSFile * handle, int64_t tag_size)
{
    unsigned char buffer[16384];
    gsize offset = 0, readed;

    if (! tag_size)
        return TRUE;

    do
    {
        if (vfs_fseek (handle, offset + tag_size, SEEK_SET))
            return FALSE;

        readed = vfs_fread (buffer, 1, sizeof buffer, handle);

        if (vfs_fseek (handle, offset, SEEK_SET))
            return FALSE;

        if (vfs_fwrite (buffer, 1, readed, handle) != readed)
            return FALSE;

        offset += readed;
    }
    while (readed);

    return vfs_ftruncate (handle, offset) == 0;
}

char *convert_numericgenre_to_text(int numericgenre)
{
    const struct
    {
        int numericgenre;
        char *genre;
    }
    table[] =
    {
        {GENRE_BLUES, "Blues"},
        {GENRE_CLASSIC_ROCK, "Classic Rock"},
        {GENRE_COUNTRY, "Country"},
        {GENRE_DANCE, "Dance"},
        {GENRE_DISCO, "Disco"},
        {GENRE_FUNK, "Funk"},
        {GENRE_GRUNGE, "Grunge"},
        {GENRE_HIPHOP, "Hip-Hop"},
        {GENRE_JAZZ, "Jazz"},
        {GENRE_METAL, "Metal"},
        {GENRE_NEW_AGE, "New Age"},
        {GENRE_OLDIES, "Oldies"},
        {GENRE_OTHER, "Other"},
        {GENRE_POP, "Pop"},
        {GENRE_R_B, "R&B"},
        {GENRE_RAP, "Rap"},
        {GENRE_REGGAE, "Reggae"},
        {GENRE_ROCK, "Rock"},
        {GENRE_TECHNO, "Techno"},
        {GENRE_INDUSTRIAL, "Industrial"},
        {GENRE_ALTERNATIVE, "Alternative"},
        {GENRE_SKA, "Ska"},
        {GENRE_DEATH_METAL, "Death Metal"},
        {GENRE_PRANKS, "Pranks"},
        {GENRE_SOUNDTRACK, "Soundtrack"},
        {GENRE_EURO_TECHNO, "Euro-Techno"},
        {GENRE_AMBIENT, "Ambient"},
        {GENRE_TRIP_HOP, "Trip-Hop"},
        {GENRE_VOCAL, "Vocal"},
        {GENRE_JAZZ_FUNK, "Jazz+Funk"},
        {GENRE_FUSION, "Fusion"},
        {GENRE_TRANCE, "Trance"},
        {GENRE_CLASSICAL, "Classical"},
        {GENRE_INSTRUMENTAL, "Instrumental"},
        {GENRE_ACID, "Acid"},
        {GENRE_HOUSE, "House"},
        {GENRE_GAME, "Game"},
        {GENRE_SOUND_CLIP, "Sound Clip"},
        {GENRE_GOSPEL, "Gospel"},
        {GENRE_NOISE, "Noise"},
        {GENRE_ALTERNROCK, "AlternRock"},
        {GENRE_BASS, "Bass"},
        {GENRE_SOUL, "Soul"},
        {GENRE_PUNK, "Punk"},
        {GENRE_SPACE, "Space"},
        {GENRE_MEDITATIVE, "Meditative"},
        {GENRE_INSTRUMENTAL_POP, "Instrumental Pop"},
        {GENRE_INSTRUMENTAL_ROCK, "Instrumental Rock"},
        {GENRE_ETHNIC, "Ethnic"},
        {GENRE_GOTHIC, "Gothic"},
        {GENRE_DARKWAVE, "Darkwave"},
        {GENRE_TECHNO_INDUSTRIAL, "Techno-Industrial"},
        {GENRE_ELECTRONIC, "Electronic"},
        {GENRE_POP_FOLK, "Pop-Folk"},
        {GENRE_EURODANCE, "Eurodance"},
        {GENRE_DREAM, "Dream"},
        {GENRE_SOUTHERN_ROCK, "Southern Rock"},
        {GENRE_COMEDY, "Comedy"},
        {GENRE_CULT, "Cult"},
        {GENRE_GANGSTA, "Gangsta"},
        {GENRE_TOP40, "Top 40"},
        {GENRE_CHRISTIAN_RAP, "Christian Rap"},
        {GENRE_POP_FUNK, "Pop/Funk"},
        {GENRE_JUNGLE, "Jungle"},
        {GENRE_NATIVE_AMERICAN, "Native American"},
        {GENRE_CABARET, "Cabaret"},
        {GENRE_NEW_WAVE, "New Wave"},
        {GENRE_PSYCHEDELIC, "Psychedelic"},
        {GENRE_RAVE, "Rave"},
        {GENRE_SHOWTUNES, "Showtunes"},
        {GENRE_TRAILER, "Trailer"},
        {GENRE_LO_FI, "Lo-Fi"},
        {GENRE_TRIBAL, "Tribal"},
        {GENRE_ACID_PUNK, "Acid Punk"},
        {GENRE_ACID_JAZZ, "Acid Jazz"},
        {GENRE_POLKA, "Polka"},
        {GENRE_RETRO, "Retro"},
        {GENRE_MUSICAL, "Musical"},
        {GENRE_ROCK_ROLL, "Rock & Roll"},
        {GENRE_HARD_ROCK, "Hard Rock"},
        {GENRE_FOLK, "Folk"},
        {GENRE_FOLK_ROCK, "Folk-Rock"},
        {GENRE_NATIONAL_FOLK, "National Folk"},
        {GENRE_SWING, "Swing"},
        {GENRE_FAST_FUSION, "Fast Fusion"},
        {GENRE_BEBOB, "Bebob"},
        {GENRE_LATIN, "Latin"},
        {GENRE_REVIVAL, "Revival"},
        {GENRE_CELTIC, "Celtic"},
        {GENRE_BLUEGRASS, "Bluegrass"},
        {GENRE_AVANTGARDE, "Avantgarde"},
        {GENRE_GOTHIC_ROCK, "Gothic Rock"},
        {GENRE_PROGRESSIVE_ROCK, "Progressive Rock"},
        {GENRE_PSYCHEDELIC_ROCK, "Psychedelic Rock"},
        {GENRE_SYMPHONIC_ROCK, "Symphonic Rock"},
        {GENRE_SLOW_ROCK, "Slow Rock"},
        {GENRE_BIG_BAND, "Big Band"},
        {GENRE_CHORUS, "Chorus"},
        {GENRE_EASY_LISTENING, "Easy Listening"},
        {GENRE_ACOUSTIC, "Acoustic"},
        {GENRE_HUMOUR, "Humour"},
        {GENRE_SPEECH, "Speech"},
        {GENRE_CHANSON, "Chanson"},
        {GENRE_OPERA, "Opera"},
        {GENRE_CHAMBER_MUSIC, "Chamber Music"},
        {GENRE_SONATA, "Sonata"},
        {GENRE_SYMPHONY, "Symphony"},
        {GENRE_BOOTY_BASS, "Booty Bass"},
        {GENRE_PRIMUS, "Primus"},
        {GENRE_PORN_GROOVE, "Porn Groove"},
        {GENRE_SATIRE, "Satire"},
        {GENRE_SLOW_JAM, "Slow Jam"},
        {GENRE_CLUB, "Club"},
        {GENRE_TANGO, "Tango"},
        {GENRE_SAMBA, "Samba"},
        {GENRE_FOLKLORE, "Folklore"},
        {GENRE_BALLAD, "Ballad"},
        {GENRE_POWER_BALLAD, "Power Ballad"},
        {GENRE_RHYTHMIC_SOUL, "Rhythmic Soul"},
        {GENRE_FREESTYLE, "Freestyle"},
        {GENRE_DUET, "Duet"},
        {GENRE_PUNK_ROCK, "Punk Rock"},
        {GENRE_DRUM_SOLO, "Drum Solo"},
        {GENRE_A_CAPELLA, "A capella"},
        {GENRE_EURO_HOUSE, "Euro-House"},
    };

    int count;

    for (count = 0; count < G_N_ELEMENTS(table); count++)
    {
        if (table[count].numericgenre == numericgenre)
        {
             return table[count].genre;
        }
    }

    return "Unknown";
}

uint32_t unsyncsafe32 (uint32_t x)
{
    return (x & 0x7f) | ((x & 0x7f00) >> 1) | ((x & 0x7f0000) >> 2) | ((x &
     0x7f000000) >> 3);
}

uint32_t syncsafe32 (uint32_t x)
{
    return (x & 0x7f) | ((x & 0x3f80) << 1) | ((x & 0x1fc000) << 2) | ((x &
     0xfe00000) << 3);
}

