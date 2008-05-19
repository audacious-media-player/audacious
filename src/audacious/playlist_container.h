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

#ifndef AUDACIOUS_PLAYLIST_CONTAINER_H
#define AUDACIOUS_PLAYLIST_CONTAINER_H

G_BEGIN_DECLS

struct _PlaylistContainer {
	gchar *name;					/* human-readable name */
	gchar *ext;					/* extension */
	void (*plc_read)(const gchar *filename, gint pos);	/* plc_load */
	void (*plc_write)(const gchar *filename, gint pos);	/* plc_write */
};

typedef struct _PlaylistContainer PlaylistContainer;

#define PLAYLIST_CONTAINER(x)		((PlaylistContainer *)(x))

extern void playlist_container_register(PlaylistContainer *plc);
extern void playlist_container_unregister(PlaylistContainer *plc);
extern void playlist_container_read(gchar *filename, gint pos);
extern void playlist_container_write(gchar *filename, gint pos);
extern PlaylistContainer *playlist_container_find(gchar *ext);

G_END_DECLS

#endif /* AUDACIOUS_PLAYLIST_CONTAINER_H */
