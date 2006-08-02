/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef HINTS_H
#define HINTS_H

#include <glib.h>
#include <gtk/gtk.h>

/* Window Managers */
#define WM_HINTS_NONE				0
#define WM_HINTS_GNOME				1
#define WM_HINTS_NET				2

void check_wm_hints(void);
void hint_set_always(gboolean always);
void hint_set_skip_winlist(GtkWidget * window);
void hint_set_sticky(gboolean sticky);
gboolean hint_always_on_top_available(void);
gboolean hint_move_resize_available(void);
void hint_move_resize(GtkWidget * window, gint x, gint y, gboolean move);

#endif
