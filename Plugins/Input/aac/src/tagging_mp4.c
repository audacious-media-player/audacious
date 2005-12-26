/*
 * Audacious -- Cross-platform Multimedia Player
 * Copyright (c) 2005 William Pitcock <nenolod@nenolod.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <string.h>
#include <unistd.h>

#include <mp4.h>

#include "tagging.h"

gchar *audmp4_get_artist(MP4FileHandle file)
{
	gchar *value;

	MP4GetMetadataArtist(file, &value);

	return value;
}

gchar *audmp4_get_title(MP4FileHandle file)
{
	gchar *value;

	MP4GetMetadataName(file, &value);

	return value;
}

gchar *audmp4_get_album(MP4FileHandle file)
{
	gchar *value;

	MP4GetMetadataAlbum(file, &value);

	return value;
}

gchar *audmp4_get_genre(MP4FileHandle file)
{
	gchar *value;

	MP4GetMetadataGenre(file, &value);

	return value;
}

gint audmp4_get_year(MP4FileHandle file)
{
	gchar *value;

	MP4GetMetadataYear(file, &value);

	if (!value)
		return 0;

	return atoi(value);
}
