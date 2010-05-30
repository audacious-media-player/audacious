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
#include "util.h"
#include <inttypes.h>

/* convert windows time to unix time */
time_t unix_time(guint64 win_time)
{
    guint64 t = (guint64) ((win_time / 10000000LL) - 11644473600LL);
    return (time_t) t;
}

guint16 get_year(guint64 win_time)
{
    GDate *d = g_date_new();
    g_date_set_time_t(d, unix_time(win_time));
    guint16 year = g_date_get_year(d);
    g_date_free(d);
    return year;
}

Tuple *makeTuple(Tuple * tuple, const gchar * title, const gchar * artist, const gchar * comment, const gchar * album, const gchar * genre, const gchar * year, const gchar * filePath, int tracnr)
{

    tuple_associate_string(tuple, FIELD_ARTIST, NULL, artist);
    tuple_associate_string(tuple, FIELD_TITLE, NULL, title);
    tuple_associate_string(tuple, FIELD_COMMENT, NULL, comment);
    tuple_associate_string(tuple, FIELD_ALBUM, NULL, album);
    tuple_associate_string(tuple, FIELD_GENRE, NULL, genre);
    tuple_associate_string(tuple, FIELD_YEAR, NULL, year);
    tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, tracnr);
    tuple_associate_string(tuple, FIELD_FILE_PATH, NULL, filePath);
    return tuple;
}

const gchar *get_complete_filepath(Tuple * tuple)
{
    const gchar *filepath;
    const gchar *dir;
    const gchar *file;

    dir = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
    file = tuple_get_string(tuple, FIELD_FILE_NAME, NULL);
    filepath = g_strdup_printf("%s/%s", dir, file);
    AUDDBG("file path = %s\n", filepath);
    return filepath;
}

void print_tuple(Tuple * tuple)
{
#if WMA_DEBUG
    AUDDBG("--------------TUPLE PRINT --------------------\n");
    const gchar *title = tuple_get_string(tuple, FIELD_TITLE, NULL);
    AUDDBG("title = %s\n", title);
    /* artist */
    const gchar *artist = tuple_get_string(tuple, FIELD_ARTIST, NULL);
    AUDDBG("artist = %s\n", artist);

    /* copyright */
    const gchar *copyright = tuple_get_string(tuple, FIELD_COPYRIGHT, NULL);
    AUDDBG("copyright = %s\n", copyright);

    /* comment / description */

    const gchar *comment = tuple_get_string(tuple, FIELD_COMMENT, NULL);
    AUDDBG("comment = %s\n", comment);

    /* codec name */
    const gchar *codec_name = tuple_get_string(tuple, FIELD_CODEC, NULL);
    AUDDBG("codec = %s\n", codec_name);

    /* album */
    const gchar *album = tuple_get_string(tuple, FIELD_ALBUM, NULL);
    AUDDBG("Album = %s\n", album);

    /*track number */
    gint track_nr = tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL);
    AUDDBG("Track nr = %d\n", track_nr);

    /* genre */
    const gchar *genre = tuple_get_string(tuple, FIELD_GENRE, NULL);
    AUDDBG("Genre = %s \n", genre);

    /* length */
    gint length = tuple_get_int(tuple, FIELD_LENGTH, NULL);
    AUDDBG("Length = %d\n", length);

    /* year */
    gint year = tuple_get_int(tuple, FIELD_YEAR, NULL);
    AUDDBG("Year = %d\n", year);

    /* quality */
    const gchar *quality = tuple_get_string(tuple, FIELD_QUALITY, NULL);
    AUDDBG("quality = %s\n", quality);

    /* path */
    const gchar *path = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
    AUDDBG("path = %s\n", path);

    /* filename */
    const gchar *filename = tuple_get_string(tuple, FIELD_FILE_NAME, NULL);
    AUDDBG("filename = %s\n", filename);

    AUDDBG("-----------------END---------------------\n");
#endif
}

gchar *read_char_data(VFSFile * fd, int size)
{
    gchar *value = g_new0(gchar, size + 1);
    vfs_fread(value, size, 1, fd);
    return value;
}

gboolean write_char_data(VFSFile * f, gchar * data, size_t i)
{
    return (vfs_fwrite(data, i, 1, f) == i);
}

gchar *utf8(gunichar2 * s)
{
    g_return_val_if_fail(s != NULL, NULL);
    return g_utf16_to_utf8(s, -1, NULL, NULL, NULL);
}

gunichar2 *fread_utf16(VFSFile * f, guint64 size)
{
    gunichar2 *p = (gunichar2 *) g_malloc0(size);
    if (vfs_fread(p, 1, size, f) != size)
    {
        g_free(p);
        p = NULL;
    }
    gchar *s = utf8(p);
    AUDDBG("Converted to UTF8: '%s'\n", s);
    g_free(s);
    return p;
}

gboolean write_utf16(VFSFile * f, gunichar2 * data, size_t i)
{
    return (vfs_fwrite(data, i, 1, f) == i);
}

guint8 read_uint8(VFSFile * fd)
{
    guint16 i;
    if (vfs_fread(&i, 1, 1, fd) == 1)
    {
        return i;
    }
    return -1;
}

guint16 read_LEuint16(VFSFile * fd)
{
    guint16 a;
    if (vfs_fget_le16(&a, fd))
        return a;
    else
        return -1;
}

guint16 read_BEuint16(VFSFile * fd)
{
    guint16 a;
    if (vfs_fget_be16(&a, fd))
        return a;
    else
        return -1;
}

guint32 read_LEuint32(VFSFile * fd)
{
    guint32 a;
    if (vfs_fget_le32(&a, fd))
        return a;
    else
        return -1;
}

guint32 read_BEuint32(VFSFile * fd)
{
    guint32 a;
    if (vfs_fget_be32(&a, fd))
        return a;
    else
        return -1;
}

guint64 read_LEuint64(VFSFile * fd)
{
    guint64 a;
    if (vfs_fget_le64(&a, fd))
        return a;
    else
        return -1;
}

guint64 read_BEuint64(VFSFile * fd)
{
    guint64 a;
    if (vfs_fget_be64(&a, fd))
        return a;
    else
        return 1;
}

gboolean write_uint8(VFSFile * fd, guint8 val)
{
    return (vfs_fwrite(&val, 1, 1, fd) == 1);
}

gboolean write_LEuint16(VFSFile * fd, guint16 val)
{
    guint16 le_val = GUINT32_TO_LE(val);
    return (vfs_fwrite(&le_val, 2, 1, fd) == 2);
}

gboolean write_BEuint32(VFSFile * fd, guint32 val)
{
    guint32 be_val = GUINT32_TO_BE(val);
    return (vfs_fwrite(&be_val, 4, 1, fd) == 4);
}

gboolean write_LEuint32(VFSFile * fd, guint32 val)
{
    guint32 le_val = GUINT32_TO_LE(val);
    return (vfs_fwrite(&le_val, 4, 1, fd) == 4);
}

gboolean write_LEuint64(VFSFile * fd, guint64 val)
{
    guint64 le_val = GUINT64_TO_LE(val);
    return (vfs_fwrite(&le_val, 8, 1, fd) == 8);
}

void copyAudioToFile(VFSFile * from, VFSFile * to, guint32 pos)
{
    vfs_fseek(from, pos, SEEK_SET);
    while (vfs_feof(from) == 0)
    {
        gchar buf[4096];
        gint n = vfs_fread(buf, 1, 4096, from);
        vfs_fwrite(buf, n, 1, to);
    }
}

void copyAudioData(VFSFile * from, VFSFile * to, guint32 pos_from, guint32 pos_to)
{
    vfs_fseek(from, pos_from, SEEK_SET);
    int bytes_read = pos_from;
    while (bytes_read < pos_to - 4096)
    {
        gchar buf[4096];
        guint32 n = vfs_fread(buf, 1, 4096, from);
        vfs_fwrite(buf, n, 1, to);
        bytes_read += n;
    }
    if (bytes_read < pos_to)
    {
        guint32 buf_size = pos_to - bytes_read;
        gchar buf2[buf_size];
        int nn = vfs_fread(buf2, 1, buf_size, from);
        vfs_fwrite(buf2, nn, 1, to);
    }
}

gboolean cut_beginning_tag (VFSFile * handle, gsize tag_size)
{
    guchar buffer[16384];
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

gchar *convert_numericgenre_to_text(gint numericgenre)
{
    const struct
    {
        gint numericgenre;
        gchar *genre;
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
        {GENRE_PSYCHADELIC, "Psychadelic"},
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

    gint count;

    for (count = 0; count < G_N_ELEMENTS(table); count++)
    {
        if (table[count].numericgenre == numericgenre)
        {
             return table[count].genre;
        }
    }

    return "Unknown";
}

guint32 unsyncsafe32 (guint32 x)
{
    return (x & 0x7f) | ((x & 0x7f00) >> 1) | ((x & 0x7f0000) >> 2) | ((x &
     0x7f000000) >> 3);
}

guint32 syncsafe32 (guint32 x)
{
    return (x & 0x7f) | ((x & 0x3f80) << 1) | ((x & 0x1fc000) << 2) | ((x &
     0xfe00000) << 3);
}

