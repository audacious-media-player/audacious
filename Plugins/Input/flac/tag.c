/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003,2004,2005  Josh Coalson
 * Copyright (C) 2002,2003,2004,2005  Daisuke Shimamura
 *
 * Based on FLAC plugin.c and mpg123 plugin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <audacious/plugin.h>
#include <audacious/util.h>
#include <libaudacious/titlestring.h>

#include "FLAC/metadata.h"
#include "plugin_common/tags.h"
#include "charset.h"
#include "configure.h"

/*
 * Function local__extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
static char *local__extname(const char *filename)
{
	char *ext = strrchr(filename, '.');

	if (ext != NULL)
		++ext;

	return ext;
}

static char *local__getstr(char* str)
{
	if (str && strlen(str) > 0)
		return g_strdup(str);
	return NULL;
}

static int local__getnum(char* str)
{
	if (str && strlen(str) > 0)
		return atoi(str);
	return 0;
}

static char *local__getfield(const FLAC__StreamMetadata *tags, const char *name)
{
	if (0 != tags) {
		const char *utf8 = FLAC_plugin__tags_get_tag_utf8(tags, name);
		if (0 != utf8) {
			if(flac_cfg.title.convert_char_set)
				return convert_from_utf8_to_user(utf8);
			else
				return strdup(utf8);
		}
	}

	return 0;
}

/*
 * Function flac_format_song_title (tag, filename)
 *
 *    Create song title according to `tag' and/or `filename' and
 *    return it.  The title must be subsequently freed using g_free().
 *
 */
TitleInput *flac_get_tuple(char *filename)
{
	TitleInput *input = NULL;
	FLAC__StreamMetadata *tags;
	FLAC__StreamMetadata info;
	char *title, *artist, *performer, *album, *date, *tracknumber, *genre, *description;
	gchar *filename_proxy = g_strdup(filename);

	FLAC_plugin__tags_get(filename_proxy, &tags);

	title       = local__getfield(tags, "TITLE");
	artist      = local__getfield(tags, "ARTIST");
	performer   = local__getfield(tags, "PERFORMER");
	album       = local__getfield(tags, "ALBUM");
	date        = local__getfield(tags, "DATE");
	tracknumber = local__getfield(tags, "TRACKNUMBER");
	genre       = local__getfield(tags, "GENRE");
	description = local__getfield(tags, "DESCRIPTION");

	input = bmp_title_input_new();

	input->performer = local__getstr(performer);
	if(!input->performer)
		input->performer = local__getstr(artist);
	input->album_name = local__getstr(album);
	input->track_name = local__getstr(title);
	input->track_number = local__getnum(tracknumber);
	input->year = local__getnum(date);
	input->genre = local__getstr(genre);
	input->comment = local__getstr(description);

	input->file_name = g_path_get_basename(filename_proxy);
	input->file_path = g_path_get_dirname(filename_proxy);
	input->file_ext = local__extname(filename_proxy);

        FLAC__metadata_get_streaminfo(filename, &info);

	input->length = (unsigned)((double)info.data.stream_info.total_samples / (double)info.data.stream_info.sample_rate * 1000.0 + 0.5);

	return input;
}

gchar *flac_format_song_title(gchar *filename)
{
	gchar *ret = NULL;
	TitleInput *tuple = flac_get_tuple(filename);

	ret = xmms_get_titlestring(flac_cfg.title.tag_override ? flac_cfg.title.tag_format : xmms_get_gentitle_format(), tuple);

	if (!ret) {
		/*
		 * Format according to filename.
		 */
		ret = g_strdup(g_basename(filename));
		if (local__extname(ret) != NULL)
			*(local__extname(ret) - 1) = '\0';	/* removes period */
	}

	bmp_title_input_free(tuple);

	return ret;
}
