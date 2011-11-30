/*
 * Copyright 2010 Tony Vroon
 * Copyright 2011 Michał Lipski <tallica@o2.pl>
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
#include <glib/gstdio.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "id3v1.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"

gboolean id3v1_can_handle_file(VFSFile *f)
{
    if (vfs_fseek(f, -128, SEEK_END))
        return FALSE;

    gchar *tag = read_char_data(f, 3);

    if (tag && !strncmp(tag, "TAG", 3))
    {
        g_free(tag);
        return TRUE;
    }

    g_free(tag);
    return FALSE;
}

static gchar *convert_to_utf8(gchar *str)
{
    if (!str)
        return NULL;

    gchar *tmp = str;
    str = str_to_utf8(str);
    g_free(tmp);

    return g_strchomp(str);
}

gboolean id3v1_read_tag (Tuple * tuple, VFSFile * f)
{
    gboolean genre_set = FALSE;

    if (vfs_fseek(f, -125, SEEK_END))
        return FALSE;

    gchar *title = read_char_data(f, 30);
    gchar *artist = read_char_data(f, 30);
    gchar *album = read_char_data(f, 30);
    gchar *year = read_char_data(f, 4);
    gchar *comment = read_char_data(f, 30);
    gchar *genre = read_char_data(f, 1);
    gchar track = 0;

    if (comment && comment[28] == 0 && comment[29] != 0)
        track = comment[29];

    title = convert_to_utf8(title);
    artist = convert_to_utf8(artist);
    album = convert_to_utf8(album);
    comment = convert_to_utf8(comment);

    if (vfs_fseek(f, -355, SEEK_END))
        return FALSE;

    gchar *tag = read_char_data(f, 4);

    if (tag && ! strncmp (tag, "TAG+", 4))
    {
        gchar *ext_title = convert_to_utf8(read_char_data(f, 60));
        gchar *ext_artist = convert_to_utf8(read_char_data(f, 60));
        gchar *ext_album = convert_to_utf8(read_char_data(f, 60));
        gchar *tmp_title = title ? g_strconcat(title, ext_title, NULL) : NULL;
        gchar *tmp_artist = artist ? g_strconcat(artist, ext_artist, NULL) : NULL;
        gchar *tmp_album = album ? g_strconcat(album, ext_album, NULL) : NULL;
        g_free(title);
        g_free(artist);
        g_free(album);
        g_free(ext_title);
        g_free(ext_artist);
        g_free(ext_album);
        title = tmp_title;
        artist = tmp_artist;
        album = tmp_album;

        if (vfs_fseek (f, -170, SEEK_END))
            goto ERR;

        gchar *ext_genre = convert_to_utf8(read_char_data(f, 30));

        if (ext_genre)
        {
            tuple_associate_string(tuple, FIELD_GENRE, NULL, ext_genre);
            genre_set = TRUE;
            g_free(ext_genre);
        }
    }

    if (title)
        tuple_associate_string(tuple, FIELD_TITLE, NULL, title);
    if (artist)
        tuple_associate_string(tuple, FIELD_ARTIST, NULL, artist);
    if (album)
        tuple_associate_string(tuple, FIELD_ALBUM, NULL, album);
    if (year)
        tuple_associate_int(tuple, FIELD_YEAR, NULL, atoi(year));
    if (comment)
        tuple_associate_string(tuple, FIELD_COMMENT, NULL, comment);
    if (track)
        tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, track);

    if (genre && !genre_set)
        tuple_associate_string(tuple, FIELD_GENRE, NULL, convert_numericgenre_to_text(*genre));

    g_free(title);
    g_free(artist);
    g_free(album);
    g_free(year);
    g_free(comment);
    g_free(genre);

    return TRUE;

ERR:
    g_free (title);
    g_free (artist);
    g_free (album);
    g_free (year);
    g_free (comment);
    g_free (genre);
    return FALSE;
}

static gboolean id3v1_write_tag (const Tuple * tuple, VFSFile * handle)
{
    fprintf (stderr, "Writing ID3v1 tags is not implemented yet, sorry.\n");
    return FALSE;
}

tag_module_t id3v1 = {
    .name = "ID3v1",
    .can_handle_file = id3v1_can_handle_file,
    .read_tag = id3v1_read_tag,
    .write_tag = id3v1_write_tag,
};
