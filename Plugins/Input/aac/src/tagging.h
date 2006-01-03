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

#ifndef _AUDMP4_TAGGING_H
#define _AUDMP4_TAGGING_H

#include <glib.h>
#include <string.h>
#include <unistd.h>

#include <mp4.h>
#include "xmms-id3.h"

/* XXX: We will need the same for AAC eventually */
extern gchar *audmp4_get_artist(MP4FileHandle);
extern gchar *audmp4_get_title(MP4FileHandle);
extern gchar *audmp4_get_album(MP4FileHandle);
extern gchar *audmp4_get_genre(MP4FileHandle);
extern gint   audmp4_get_year(MP4FileHandle);

#define GENRE_MAX 0x94
extern const char *audmp4_id3_genres[GENRE_MAX];

#endif
