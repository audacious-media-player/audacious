
/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#ifndef XMMS_DIRBROWSER_H
#define XMMS_DIRBROWSER_H

#include <glib.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

GtkWidget *xmms_create_dir_browser(gchar * title, gchar * current_path,
                                   GtkSelectionMode mode,
                                   void (*handler) (gchar *));

G_END_DECLS

#endif
