/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006-2007 William Pitcock, Tony Vroon, George Averill,
 *                         Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <glib.h>
#include <string.h>

#include "misc.h"
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

PlaylistContainer *playlist_container_find(gchar *ext)
{
	GList *node;
	PlaylistContainer *plc;

	/* check ext neither is NULL nor 1 (in a consequence of optimization). */
	g_return_val_if_fail(ext != NULL && ext != (void *)1, NULL);

	for (node = registered_plcs; node != NULL; node = g_list_next(node)) {
		plc = node->data;

		if (!g_ascii_strncasecmp(plc->ext, ext, strlen(plc->ext)))
			return plc;
	}

	return NULL;
}

void playlist_container_read(gchar *filename, gint pos)
{
	gchar *ext = strrchr(filename, '.') + 1;	/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc->plc_read == NULL)
		return;

	plc->plc_read(filename, pos);
}

void playlist_container_write(gchar *filename, gint pos)
{
	gchar *ext = strrchr(filename, '.') + 1;	/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc->plc_write == NULL)
		return;

	plc->plc_write(filename, pos);
}
