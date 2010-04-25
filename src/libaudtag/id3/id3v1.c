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
#include "../../audacious/chardet.h"

gboolean id3v1_can_handle_file(VFSFile * f)
{
    gchar *tag = g_new0(gchar, 4);
    vfs_fseek(f, -128, SEEK_END);
    tag = read_char_data(f, 3);
    if (!strncmp(tag, "TAG", 3))
    {
        g_free(tag);
        return TRUE;
    }

    g_free(tag);
    return FALSE;
}

Tuple *id3v1_populate_tuple_from_file(Tuple * tuple, VFSFile * f)
{
    gchar *title = g_new0(gchar, 30);
    gchar *artist = g_new0(gchar, 30);
    gchar *album = g_new0(gchar, 30);
    gchar *year = g_new0(gchar, 4);
    gchar *comment = g_new0(gchar, 30);
    gchar *track = g_new0(gchar, 1);
    gchar *genre = g_new0(gchar, 1);
    vfs_fseek(f, -125, SEEK_END);
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

    tuple_associate_string(tuple, FIELD_TITLE, NULL, cd_str_to_utf8(title));
    tuple_associate_string(tuple, FIELD_ARTIST, NULL, cd_str_to_utf8(artist));
    tuple_associate_string(tuple, FIELD_ALBUM, NULL, cd_str_to_utf8(album));
    tuple_associate_int(tuple, FIELD_YEAR, NULL, atoi(year));
    tuple_associate_string(tuple, FIELD_COMMENT, NULL, cd_str_to_utf8(comment));
    tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, *track);
    tuple_associate_string(tuple, FIELD_GENRE, NULL, convert_numericgenre_to_text(*genre));

    g_free(title);
    g_free(artist);
    g_free(album);
    g_free(year);
    g_free(comment);
    g_free(track);
    g_free(genre);
    return tuple;
}

gboolean id3v1_write_tuple_to_file(Tuple * tuple, VFSFile * f)
{
    return FALSE;
}
