/*
 * Audacious2
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
 * Copyright (c) 2008 Tomasz Moń <desowin@gmail.com>
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

/*
 * This is the Interface API.
 *
 * Everything here is like totally subject to change.
 *     --nenolod
 */

#ifndef __AUDACIOUS2_INTERFACE_H__
#define __AUDACIOUS2_INTERFACE_H__

#include <gtk/gtk.h>

typedef struct {
	void (*create_prefs_window)(void);
	void (*show_prefs_window)(void);
	void (*hide_prefs_window)(void);

	void (*filebrowser_show)(gboolean play_button);
	void (*urlopener_show)(void);
	void (*jump_to_track_show)(void);
	void (*aboutwin_show)(void);
} InterfaceOps;

typedef struct _Interface {
	gchar *id;		/* simple ID like 'skinned' */
	gchar *desc;		/* description like 'Skinned Interface' */
	gboolean (*init)(void); /* init UI */
	gboolean (*fini)(void); /* shutdown UI */

	InterfaceOps *ops;
} Interface;

void interface_register(Interface *i);
void interface_deregister(Interface *i);
void interface_run(Interface *i);
void interface_destroy(Interface *i);

Interface *interface_get(gchar *id);
const Interface *interface_get_current(void);

#endif
