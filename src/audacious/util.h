/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2008  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#ifndef AUDACIOUS_UTIL_H
#define AUDACIOUS_UTIL_H

#ifdef _AUDACIOUS_CORE
# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif
#endif

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#include "audacious/plugin.h"

#define SWAP(a, b)      { a^=b; b^=a; a^=b; }

typedef gboolean(*DirForeachFunc) (const gchar * path,
                                   const gchar * basename,
                                   gpointer user_data);


gchar *find_file_recursively(const gchar * dirname, const gchar * file);
gchar *find_path_recursively(const gchar * dirname, const gchar * file);
gboolean dir_foreach(const gchar * path, DirForeachFunc function,
                     gpointer user_data, GError ** error);


INIFile *open_ini_file(const gchar *filename);
void close_ini_file(INIFile *key_file);
gchar *read_ini_string(INIFile *key_file, const gchar *section,
					   const gchar *key);
GArray *read_ini_array(INIFile *key_file, const gchar *section,
                       const gchar *key);

GArray *string_to_garray(const gchar * str);

void glist_movedown(GList * list);
void glist_moveup(GList * list);

void util_set_cursor(GtkWidget * window);
gboolean text_get_extents(const gchar * fontname, const gchar * text,
                          gint * width, gint * height, gint * ascent,
                          gint * descent);

guint gint_count_digits(gint n);


GtkWidget *util_info_dialog(const gchar * title, const gchar * text,
    const gchar * button_text, gboolean modal, GCallback button_action,
    gpointer action_data);

gchar *util_get_localdir(void);

gchar *construct_uri(gchar *string, const gchar *playlist_name);

/* minimizes number of realloc's */
gpointer smart_realloc(gpointer ptr, gsize *size);

gint file_get_mtime (const gchar * filename);
void make_directory(const gchar * path, mode_t mode);

void util_add_url_history_entry(const gchar * url);

G_END_DECLS

#endif /* AUDACIOUS_UTIL_H */
