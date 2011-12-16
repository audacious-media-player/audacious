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
#include <libaudcore/core.h>

typedef struct {
    void (* get_value) (void * user, int row, int column, GValue * value);

    /* selection (optional) */
    boolean (* get_selected) (void * user, int row);
    void (* set_selected) (void * user, int row, boolean selected);
    void (* select_all) (void * user, boolean selected);

    void (* activate_row) (void * user, int row); /* optional */
    void (* right_click) (void * user, GdkEventButton * event); /* optional */
    void (* shift_rows) (void * user, int row, int before); /* optional */

    /* cross-widget drag and drop (optional) */
    const char * data_type;
    void (* get_data) (void * user, void * * data, int * length); /* data will
     be freed */
    void (* receive_data) (void * user, int row, const void * data, int length);
} AudguiListCallbacks;

GtkWidget * audgui_list_new (const AudguiListCallbacks * cbs, void * user,
 int rows);
void * audgui_list_get_user (GtkWidget * list);
void audgui_list_add_column (GtkWidget * list, const char * title,
 int column, GType type, int width);

int audgui_list_row_count (GtkWidget * list);
void audgui_list_insert_rows (GtkWidget * list, int at, int rows);
void audgui_list_update_rows (GtkWidget * list, int at, int rows);
void audgui_list_delete_rows (GtkWidget * list, int at, int rows);
void audgui_list_update_selection (GtkWidget * list, int at, int rows);

int audgui_list_get_highlight (GtkWidget * list);
void audgui_list_set_highlight (GtkWidget * list, int row);
int audgui_list_get_focus (GtkWidget * list);
void audgui_list_set_focus (GtkWidget * list, int row);

int audgui_list_row_at_point (GtkWidget * list, int x, int y);

#endif
