/*
 * list.h
 * Copyright 2011-2012 John Lindgren and Michał Lipski
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef AUDGUI_LIST_H
#define AUDGUI_LIST_H

/* okay to use without audgui_init() */

#include <gtk/gtk.h>

#include <libaudcore/index.h>

/* New callbacks should be added to the end of this struct.  The
 * audgui_list_new() macro tells us the size of the callback struct as it was
 * defined when the caller code was compiled, allowing us to expand the struct
 * without breaking backward compatibility. */

struct AudguiListCallbacks {
    void (* get_value) (void * user, int row, int column, GValue * value);

    /* selection (optional) */
    bool (* get_selected) (void * user, int row);
    void (* set_selected) (void * user, int row, bool selected);
    void (* select_all) (void * user, bool selected);

    void (* activate_row) (void * user, int row); /* optional */
    void (* right_click) (void * user, GdkEventButton * event); /* optional */
    void (* shift_rows) (void * user, int row, int before); /* optional */

    /* cross-widget drag and drop (optional) */
    const char * data_type;
    Index<char> (* get_data) (void * user);
    void (* receive_data) (void * user, int row, const char * data, int len);

    void (* mouse_motion) (void * user, GdkEventMotion * event, int row); /* optional */
    void (* mouse_leave) (void * user, GdkEventMotion * event, int row); /* optional */

    void (* focus_change) (void * user, int row); /* optional */
};

GtkWidget * audgui_list_new_real (const AudguiListCallbacks * cbs, int cbs_size,
 void * user, int rows);

#define audgui_list_new(c, u, r) \
 audgui_list_new_real (c, sizeof (AudguiListCallbacks), u, r)

void * audgui_list_get_user (GtkWidget * list);
void audgui_list_add_column (GtkWidget * list, const char * title,
 int column, GType type, int width, bool use_markup = false);

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
int audgui_list_row_at_point_rounded (GtkWidget * list, int x, int y);

#endif
