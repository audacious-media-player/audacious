/*
 * Audacious -- Cross-platform Multimedia Player
 * Copyright (c) 2005-2006 William Pitcock <nenolod@nenolod.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "tagging.h"

gchar *audmp4_get_artist(mp4ff_t *file)
{
	gchar *value;

	mp4ff_meta_get_artist(file, &value);

	return value;
}

gchar *audmp4_get_title(mp4ff_t *file)
{
	gchar *value;

	mp4ff_meta_get_title(file, &value);

	return value;
}

gchar *audmp4_get_album(mp4ff_t *file)
{
	gchar *value;

	mp4ff_meta_get_album(file, &value);

	return value;
}

gchar *audmp4_get_genre(mp4ff_t *file)
{
	gchar *value;

	mp4ff_meta_get_genre(file, &value);

	return value;
}

gint audmp4_get_year(mp4ff_t *file)
{
	gchar *value;

	mp4ff_meta_get_date(file, &value);

	if (!value)
		return 0;

	return atoi(value);
}
