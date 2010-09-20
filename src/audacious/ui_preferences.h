/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef AUDACIOUS_UI_PREFERENCES_H
#define AUDACIOUS_UI_PREFERENCES_H

#include <glib.h>
#include <gtk/gtk.h>

#include "preferences.h"

/* GtkWidget * * create_prefs_window (void); */
void * * create_prefs_window (void);
void destroy_prefs_window(void);
void show_prefs_window(void);
void hide_prefs_window(void);

/* gint prefswin_page_new (GtkWidget * container, const gchar * name,
 const gchar * imgurl); */
gint prefswin_page_new (void * container, const gchar * name, const gchar *
 imgurl);
void prefswin_page_destroy(GtkWidget *container);

void plugin_preferences_show (PluginPreferences * p);
void plugin_preferences_cleanup (PluginPreferences * p);

/* plugin-view.c */
GtkWidget * plugin_view_new (gint type);

#endif /* AUDACIOUS_UI_PREFERENCES_H */
