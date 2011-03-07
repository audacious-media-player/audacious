/*
 * list.h
 * Copyright 2011 John Lindgren
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

#ifndef AUDGUI_LIST_H
#define AUDGUI_LIST_H

#include <gtk/gtk.h>

typedef struct {
    void (* get_value) (void * user, gint row, gint column, GValue * value);

    /* selection (optional) */
    gboolean (* get_selected) (void * user, gint row);
    void (* set_selected) (void * user, gint row, gboolean selected);
    void (* select_all) (void * user, gboolean selected);

    void (* activate_row) (void * user, gint row); /* optional */
    void (* right_click) (void * user, GdkEventButton * event); /* optional */
    void (* shift_rows) (void * user, gint row, gint before); /* optional */

    /* cross-widget drag and drop (optional) */
    const gchar * data_type;
    void (* get_data) (void * user, void * * data, gint * length); /* data will
     be freed */
    void (* receive_data) (void * user, gint row, const void * data, gint length);
} AudguiListCallbacks;

GtkWidget * audgui_list_new (const AudguiListCallbacks * cbs, void * user,
 gint rows);
void * audgui_list_get_user (GtkWidget * list);
void audgui_list_add_column (GtkWidget * list, const gchar * title,
 gint column, GType type, gboolean expand);

gint audgui_list_row_count (GtkWidget * list);
void audgui_list_insert_rows (GtkWidget * list, gint at, gint rows);
void audgui_list_update_rows (GtkWidget * list, gint at, gint rows);
void audgui_list_delete_rows (GtkWidget * list, gint at, gint rows);
void audgui_list_update_selection (GtkWidget * list, gint at, gint rows);
void audgui_list_set_highlight (GtkWidget * list, gint row);
void audgui_list_set_focus (GtkWidget * list, gint row);
gint audgui_list_row_at_point (GtkWidget * list, gint x, gint y);

#endif
