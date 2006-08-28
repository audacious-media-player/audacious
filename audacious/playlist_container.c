/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
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
#include "playlist_container.h"

/*
 * PlaylistContainer objects handle the import and export of Playlist
 * data. Basically, a PlaylistContainer acts as a filter for a PlaylistEntry.
 */

static GList *registered_plcs = NULL;

void playlist_container_register(PlaylistContainer *plc)
{
	registered_plcs = g_list_append(registered_plcs, plc);
}

void playlist_container_unregister(PlaylistContainer *plc)
{
	registered_plcs = g_list_remove(registered_plcs, plc);
}

PlaylistContainer *playlist_container_find(char *ext)
{
	GList *node;
	PlaylistContainer *plc;

	/* check ext neither is NULL nor 1 (in a consequence of optimization). */
	g_return_val_if_fail(ext != NULL && ext != (void *)1, NULL);

	for (node = registered_plcs; node != NULL; node = g_list_next(node)) {
		plc = PLAYLIST_CONTAINER(node->data);

		if (!g_strcasecmp(plc->ext, ext))
			return plc;
	}

	return NULL;
}

void playlist_container_read(char *filename, gint pos)
{
	char *ext = strrchr(filename, '.') + 1;		/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc->plc_read == NULL)
		return;

	plc->plc_read(filename, pos);
}

void playlist_container_write(char *filename, gint pos)
{
	char *ext = strrchr(filename, '.') + 1;		/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc->plc_write == NULL)
		return;

	plc->plc_write(filename, pos);
}

gboolean is_playlist_name(char *filename)
{
	char *ext = strrchr(filename, '.') + 1;		/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc != NULL)
		return TRUE;

	return FALSE;
}
