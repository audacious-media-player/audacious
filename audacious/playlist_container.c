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
