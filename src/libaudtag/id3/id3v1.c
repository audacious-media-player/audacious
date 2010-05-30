/*
 * Copyright 2010 Tony Vroon
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

#include "id3v1.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"

static gboolean has_id3v1_ext;

gboolean id3v1_can_handle_file(VFSFile *f)
{
    gchar *tag = g_new0(gchar, 4);

    if (vfs_fseek(f, -355, SEEK_END))
        return FALSE;
    tag = read_char_data(f, 4);
    if (!strncmp(tag, "TAG+", 4))
        has_id3v1_ext = TRUE;
    else
        has_id3v1_ext = FALSE;

    if (vfs_fseek(f, -128, SEEK_END))
        return FALSE;
    tag = read_char_data(f, 3);
    if (!strncmp(tag, "TAG", 3))
    {
        g_free(tag);
        return TRUE;
    }

    g_free(tag);
    return FALSE;
}

static gchar *convert_to_utf8(gchar *str)
{
    return g_strchomp(str_to_utf8(str));
}

gboolean id3v1_read_tag (Tuple * tuple, VFSFile * f)
{
    gchar *title = g_new0(gchar, 30);
    gchar *artist = g_new0(gchar, 30);
    gchar *album = g_new0(gchar, 30);
    gchar *year = g_new0(gchar, 4);
    gchar *comment = g_new0(gchar, 30);
    gchar *track = g_new0(gchar, 1);
    gchar *genre = g_new0(gchar, 1);
    gboolean genre_set = FALSE;
    if (vfs_fseek(f, -125, SEEK_END))
        return FALSE;
    title = read_char_data(f, 30);
    artist = read_char_data(f, 30);
    album = read_char_data(f, 30);
    year = read_char_data(f, 4);
    comment = read_char_data(f, 30);
    genre = read_char_data(f, 1);

    if (comment[28] == 0 && comment[29] != 0)
    {
        *track = comment[29];
    }

    title = convert_to_utf8(title);
    artist = convert_to_utf8(artist);
    album = convert_to_utf8(album);
    comment = convert_to_utf8(comment);

    if (has_id3v1_ext)
    {
        vfs_fseek(f, -351, SEEK_END);
        gchar *tmp_title = g_strconcat(title, convert_to_utf8(read_char_data(f, 60)), NULL);
        gchar *tmp_artist = g_strconcat(artist, convert_to_utf8(read_char_data(f, 60)), NULL);
        gchar *tmp_album = g_strconcat(album, convert_to_utf8(read_char_data(f, 60)), NULL);
        vfs_fseek(f, -170, SEEK_END);
        gchar *tmp_genre = g_new0(gchar, 30);
        tmp_genre = convert_to_utf8(read_char_data(f, 30));
        g_free(title);
        g_free(artist);
        g_free(album);
        title = tmp_title;
        artist = tmp_artist;
        album = tmp_album;

        if (g_strcmp0(tmp_genre, NULL) == 1)
        {
            tuple_associate_string(tuple, FIELD_GENRE, NULL, tmp_genre);
            genre_set = TRUE;
        }

        g_free(tmp_genre);
    }

    tuple_associate_string(tuple, FIELD_TITLE, NULL, title);
    tuple_associate_string(tuple, FIELD_ARTIST, NULL, artist);
    tuple_associate_string(tuple, FIELD_ALBUM, NULL, album);
    tuple_associate_int(tuple, FIELD_YEAR, NULL, atoi(year));
    tuple_associate_string(tuple, FIELD_COMMENT, NULL, comment);
    tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, *track);
    if (!genre_set) tuple_associate_string(tuple, FIELD_GENRE, NULL, convert_numericgenre_to_text(*genre));

    g_free(title);
    g_free(artist);
    g_free(album);
    g_free(year);
    g_free(comment);
    g_free(track);
    g_free(genre);

    return TRUE;
}

gboolean id3v1_write_tag (Tuple * tuple, VFSFile * handle)
{
    return FALSE;
}

tag_module_t id3v1 = {
    .name = "ID3v1",
    .can_handle_file = id3v1_can_handle_file,
    .read_tag = id3v1_read_tag,
    .write_tag = id3v1_write_tag,
};

