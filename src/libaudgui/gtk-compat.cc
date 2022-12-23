/*
 * gtk-compat.cc
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

#include <gtk/gtk.h>

EXPORT GtkWidget * audgui_box_new (GtkOrientation orientation, int spacing)
{
#ifdef USE_GTK3
    return gtk_box_new (orientation, spacing);
#else
    return gtk_widget_new (orientation == GTK_ORIENTATION_HORIZONTAL
                               ? GTK_TYPE_HBOX
                               : GTK_TYPE_VBOX,
                           "spacing", spacing,
                           nullptr);
#endif
}

EXPORT GtkWidget * audgui_button_box_new (GtkOrientation orientation)
{
#ifdef USE_GTK3
    return gtk_button_box_new (orientation);
#else
    return gtk_widget_new (orientation == GTK_ORIENTATION_HORIZONTAL
                               ? GTK_TYPE_HBUTTON_BOX
                               : GTK_TYPE_VBUTTON_BOX,
                           nullptr);
#endif
}

EXPORT GtkWidget * audgui_paned_new (GtkOrientation orientation)
{
#ifdef USE_GTK3
    return gtk_paned_new (orientation);
#else
    return gtk_widget_new (orientation == GTK_ORIENTATION_HORIZONTAL
                               ? GTK_TYPE_HPANED
                               : GTK_TYPE_VPANED,
                           nullptr);
#endif
}

EXPORT GtkWidget * audgui_scale_new (GtkOrientation orientation,
                                     GtkAdjustment * adjustment)
{
#ifdef USE_GTK3
    return gtk_scale_new (orientation, adjustment);
#else
    return gtk_widget_new (orientation == GTK_ORIENTATION_HORIZONTAL
                               ? GTK_TYPE_HSCALE
                               : GTK_TYPE_VSCALE,
                           "adjustment", adjustment,
                           nullptr);
#endif
}

EXPORT GtkWidget * audgui_separator_new (GtkOrientation orientation)
{
#ifdef USE_GTK3
    return gtk_separator_new (orientation);
#else
    return gtk_widget_new (orientation == GTK_ORIENTATION_HORIZONTAL
                               ? GTK_TYPE_HSEPARATOR
                               : GTK_TYPE_VSEPARATOR,
                           nullptr);
#endif
}

EXPORT GtkAdjustment * audgui_tree_view_get_hadjustment (GtkWidget * tree_view)
{
#ifdef USE_GTK3
    return gtk_scrollable_get_hadjustment ((GtkScrollable *) tree_view);
#else
    return gtk_tree_view_get_hadjustment ((GtkTreeView *) tree_view);
#endif
}

EXPORT GtkAdjustment * audgui_tree_view_get_vadjustment (GtkWidget * tree_view)
{
#ifdef USE_GTK3
    return gtk_scrollable_get_vadjustment ((GtkScrollable *) tree_view);
#else
    return gtk_tree_view_get_vadjustment ((GtkTreeView *) tree_view);
#endif
}

EXPORT GtkWidget * audgui_grid_new ()
{
#ifdef USE_GTK3
    return gtk_grid_new ();
#else
    return gtk_table_new (1, 1, false);
#endif
}

EXPORT void audgui_grid_set_row_spacing (GtkWidget * grid, unsigned int spacing)
{
#ifdef USE_GTK3
    gtk_grid_set_row_spacing ((GtkGrid *) grid, spacing);
#else
    gtk_table_set_row_spacings ((GtkTable *) grid, spacing);
#endif
}

EXPORT void audgui_grid_set_column_spacing (GtkWidget * grid, unsigned int spacing)
{
#ifdef USE_GTK3
    gtk_grid_set_column_spacing ((GtkGrid *) grid, spacing);
#else
    gtk_table_set_col_spacings ((GtkTable *) grid, spacing);
#endif
}
