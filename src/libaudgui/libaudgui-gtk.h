/*
 * libaudgui-gtk.h
 * Copyright 2010-2011 Audacious Development Team
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef LIBAUDGUI_GTK_H
#define LIBAUDGUI_GTK_H

#include <gtk/gtk.h>
#include <libaudcore/core.h>

/* effects-menu.c */
GtkWidget * audgui_create_effects_menu (void);
GtkWidget * audgui_create_vis_menu (void);

/* iface-menu.c */
GtkWidget * audgui_create_iface_menu (void);

/* util.c */
void audgui_hide_on_delete (GtkWidget * widget);
void audgui_hide_on_escape (GtkWidget * widget);
void audgui_destroy_on_escape (GtkWidget * widget);
void audgui_simple_message (GtkWidget * * widget, GtkMessageType type,
 const char * title, const char * text);
void audgui_connect_check_box (GtkWidget * box, bool_t * setting);

GdkPixbuf * audgui_pixbuf_from_data (void * data, int size);
GdkPixbuf * audgui_pixbuf_for_entry (int playlist, int entry);
GdkPixbuf * audgui_pixbuf_for_current (void);
void audgui_pixbuf_scale_within (GdkPixbuf * * pixbuf, int size);

#endif
