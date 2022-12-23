/*
 * gtk-compat.h
 * Copyright 2022 Thomas Lange
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

#ifndef AUDGUI_GTK_COMPAT_H
#define AUDGUI_GTK_COMPAT_H

#include <gtk/gtk.h>

#ifdef USE_GTK3
  #define AUDGUI_DRAW_SIGNAL "draw"
#else
  #define AUDGUI_DRAW_SIGNAL "expose-event"
#endif

#define audgui_hbox_new(spacing) audgui_box_new (GTK_ORIENTATION_HORIZONTAL, spacing)
#define audgui_vbox_new(spacing) audgui_box_new (GTK_ORIENTATION_VERTICAL, spacing)

GtkWidget * audgui_box_new (GtkOrientation orientation, int spacing);
GtkWidget * audgui_button_box_new (GtkOrientation orientation);
GtkWidget * audgui_paned_new (GtkOrientation orientation);
GtkWidget * audgui_scale_new (GtkOrientation orientation, GtkAdjustment * adjustment);
GtkWidget * audgui_separator_new (GtkOrientation orientation);

GtkAdjustment * audgui_tree_view_get_hadjustment (GtkWidget * tree_view);
GtkAdjustment * audgui_tree_view_get_vadjustment (GtkWidget * tree_view);

GtkWidget * audgui_grid_new ();
void audgui_grid_set_row_spacing (GtkWidget * grid, unsigned int spacing);
void audgui_grid_set_column_spacing (GtkWidget * grid, unsigned int spacing);

#endif /* AUDGUI_GTK_COMPAT_H */
