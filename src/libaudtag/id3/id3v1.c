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
#include <stdlib.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "id3v1.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"

bool_t id3v1_can_handle_file(VFSFile *f)
{
    if (vfs_fseek(f, -128, SEEK_END))
        return FALSE;

    char *tag = read_char_data(f, 3);

    if (tag && !strncmp(tag, "TAG", 3))
    {
        g_free(tag);
        return TRUE;
    }

    g_free(tag);
    return FALSE;
}

static char *convert_to_utf8(char *str)
{
    if (!str)
        return NULL;

    char *tmp = str;
    str = str_to_utf8(str);
    g_free(tmp);

    return g_strchomp(str);
}

bool_t id3v1_read_tag (Tuple * tuple, VFSFile * f)
{
    bool_t genre_set = FALSE;

    if (vfs_fseek(f, -125, SEEK_END))
        return FALSE;

    char *title = read_char_data(f, 30);
    char *artist = read_char_data(f, 30);
    char *album = read_char_data(f, 30);
    char *year = read_char_data(f, 4);
    char *comment = read_char_data(f, 30);
    char *genre = read_char_data(f, 1);
    char track = 0;

    if (comment && comment[28] == 0 && comment[29] != 0)
        track = comment[29];

    title = convert_to_utf8(title);
    artist = convert_to_utf8(artist);
    album = convert_to_utf8(album);
    comment = convert_to_utf8(comment);

    if (vfs_fseek(f, -355, SEEK_END))
        return FALSE;

    char *tag = read_char_data(f, 4);

    if (tag && ! strncmp (tag, "TAG+", 4))
    {
        char *ext_title = convert_to_utf8(read_char_data(f, 60));
        char *ext_artist = convert_to_utf8(read_char_data(f, 60));
        char *ext_album = convert_to_utf8(read_char_data(f, 60));
        char *tmp_title = title ? g_strconcat(title, ext_title, NULL) : NULL;
        char *tmp_artist = artist ? g_strconcat(artist, ext_artist, NULL) : NULL;
        char *tmp_album = album ? g_strconcat(album, ext_album, NULL) : NULL;
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

        char *ext_genre = convert_to_utf8(read_char_data(f, 30));

        if (ext_genre)
        {
            tuple_set_str(tuple, FIELD_GENRE, NULL, ext_genre);
            genre_set = TRUE;
            g_free(ext_genre);
        }
    }

    if (title)
        tuple_set_str(tuple, FIELD_TITLE, NULL, title);
    if (artist)
        tuple_set_str(tuple, FIELD_ARTIST, NULL, artist);
    if (album)
        tuple_set_str(tuple, FIELD_ALBUM, NULL, album);
    if (year)
        tuple_set_int(tuple, FIELD_YEAR, NULL, atoi(year));
    if (comment)
        tuple_set_str(tuple, FIELD_COMMENT, NULL, comment);
    if (track)
        tuple_set_int(tuple, FIELD_TRACK_NUMBER, NULL, track);

    if (genre && !genre_set)
        tuple_set_str(tuple, FIELD_GENRE, NULL, convert_numericgenre_to_text(*genre));

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

static bool_t id3v1_write_tag (const Tuple * tuple, VFSFile * handle)
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
