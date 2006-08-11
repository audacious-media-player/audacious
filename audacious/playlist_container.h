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

#ifndef _PLAYLIST_CONTAINER_H_
#define _PLAYLIST_CONTAINER_H_

struct _PlaylistContainer {
	char *name;					/* human-readable name */
	char *ext;					/* extension */
	GList *(*plc_read)(char *filename, gint pos);	/* plc_load */
	void (*plc_write)(char *filename, gint pos);	/* plc_write */
};

typedef struct _PlaylistContainer PlaylistContainer;

#define PLAYLIST_CONTAINER(x)		((PlaylistContainer *)(x))

extern void playlist_container_register(PlaylistContainer *plc);
extern void playlist_container_unregister(PlaylistContainer *plc);
extern void playlist_container_read(char *filename, gint pos);
extern void playlist_container_write(char *filename, gint pos);
extern PlaylistContainer *playlist_container_find(char *ext);

#endif
