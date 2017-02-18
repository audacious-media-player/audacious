/*
 * list.c
 * Copyright 2011-2013 John Lindgren and Michał Lipski
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

#include <stddef.h>
#include <gtk/gtk.h>

#include <libaudcore/hook.h>
#include <libaudcore/objects.h>

#include "libaudgui-gtk.h"
#include "list.h"

enum {
    HIGHLIGHT_COLUMN,
    RESERVED_COLUMNS
};

#define MODEL_HAS_CB(m, cb) \
 ((m)->cbs_size > (int) offsetof (AudguiListCallbacks, cb) && (m)->cbs->cb)
#define PATH_IS_SELECTED(w, p) (gtk_tree_selection_path_is_selected \
 (gtk_tree_view_get_selection ((GtkTreeView *) (w)), (p)))

struct ListModel {
    GObject parent;
    const AudguiListCallbacks * cbs;
    int cbs_size;
    void * user;
    int charwidth;
    int rows, highlight;
    int columns;
    GList * column_types;
    bool resizable;
    bool frozen, blocked;
    bool dragging;
    int clicked_row, receive_row;
    int scroll_speed;
};

/* ==== MODEL ==== */

static GtkTreeModelFlags list_model_get_flags (GtkTreeModel * model)
{
    return GTK_TREE_MODEL_LIST_ONLY;
}

static int list_model_get_n_columns (GtkTreeModel * model)
{
    return ((ListModel *) model)->columns;
}

static GType list_model_get_column_type (GtkTreeModel * _model, int column)
{
    ListModel * model = (ListModel *) _model;
    g_return_val_if_fail (column >= 0 && column < model->columns, G_TYPE_INVALID);

    if (column == HIGHLIGHT_COLUMN)
        return PANGO_TYPE_WEIGHT;

    return GPOINTER_TO_INT (g_list_nth_data (model->column_types, column -
     RESERVED_COLUMNS));
}

static gboolean list_model_get_iter (GtkTreeModel * model, GtkTreeIter * iter,
 GtkTreePath * path)
{
    int row = gtk_tree_path_get_indices (path)[0];
    if (row < 0 || row >= ((ListModel *) model)->rows)
        return false;
    iter->user_data = GINT_TO_POINTER (row);
    return true;
}

static GtkTreePath * list_model_get_path (GtkTreeModel * model,
 GtkTreeIter * iter)
{
    int row = GPOINTER_TO_INT (iter->user_data);
    g_return_val_if_fail (row >= 0 && row < ((ListModel *) model)->rows, nullptr);
    return gtk_tree_path_new_from_indices (row, -1);
}

static void list_model_get_value (GtkTreeModel * _model, GtkTreeIter * iter,
 int column, GValue * value)
{
    ListModel * model = (ListModel *) _model;
    int row = GPOINTER_TO_INT (iter->user_data);
    g_return_if_fail (column >= 0 && column < model->columns);
    g_return_if_fail (row >= 0 && row < model->rows);

    if (column == HIGHLIGHT_COLUMN)
    {
        g_value_init (value, PANGO_TYPE_WEIGHT);
        g_value_set_enum (value, row == model->highlight ? PANGO_WEIGHT_BOLD :
         PANGO_WEIGHT_NORMAL);
        return;
    }

    g_value_init (value, GPOINTER_TO_INT (g_list_nth_data (model->column_types,
     column - RESERVED_COLUMNS)));
    model->cbs->get_value (model->user, row, column - RESERVED_COLUMNS, value);
}

static gboolean list_model_iter_next (GtkTreeModel * _model, GtkTreeIter * iter)
{
    ListModel * model = (ListModel *) _model;
    int row = GPOINTER_TO_INT (iter->user_data);
    g_return_val_if_fail (row >= 0 && row < model->rows, false);
    if (row + 1 >= model->rows)
        return false;
    iter->user_data = GINT_TO_POINTER (row + 1);
    return true;
}

static gboolean list_model_iter_children (GtkTreeModel * model,
 GtkTreeIter * iter, GtkTreeIter * parent)
{
    if (parent || ((ListModel *) model)->rows < 1)
        return false;
    iter->user_data = GINT_TO_POINTER (0);
    return true;
}

static gboolean list_model_iter_has_child (GtkTreeModel * model,
 GtkTreeIter * iter)
{
    return false;
}

static int list_model_iter_n_children (GtkTreeModel * model, GtkTreeIter * iter)
{
    return iter ? 0 : ((ListModel *) model)->rows;
}

static gboolean list_model_iter_nth_child (GtkTreeModel * model,
 GtkTreeIter * iter, GtkTreeIter * parent, int n)
{
    if (parent || n < 0 || n >= ((ListModel *) model)->rows)
        return false;
    iter->user_data = GINT_TO_POINTER (n);
    return true;
}

static gboolean list_model_iter_parent (GtkTreeModel * model,
 GtkTreeIter * iter, GtkTreeIter * child)
{
    return false;
}

static void iface_init (GtkTreeModelIface * iface)
{
    iface->get_flags = list_model_get_flags;
    iface->get_n_columns = list_model_get_n_columns;
    iface->get_column_type = list_model_get_column_type;
    iface->get_iter = list_model_get_iter;
    iface->get_path = list_model_get_path;
    iface->get_value = list_model_get_value;
    iface->iter_next = list_model_iter_next;
    iface->iter_children = list_model_iter_children;
    iface->iter_has_child = list_model_iter_has_child;
    iface->iter_n_children = list_model_iter_n_children;
    iface->iter_nth_child = list_model_iter_nth_child;
    iface->iter_parent = list_model_iter_parent;
}

static const GInterfaceInfo iface_info = {
    (GInterfaceInitFunc) iface_init
};

static GType list_model_get_type ()
{
    static GType type = G_TYPE_INVALID;
    if (type == G_TYPE_INVALID)
    {
        type = g_type_register_static_simple (G_TYPE_OBJECT, "AudguiListModel",
         sizeof (GObjectClass), nullptr, sizeof (ListModel), nullptr, (GTypeFlags) 0);
        g_type_add_interface_static (type, GTK_TYPE_TREE_MODEL, & iface_info);
    }
    return type;
}

/* ==== CALLBACKS ==== */

static gboolean select_allow_cb (GtkTreeSelection * sel, GtkTreeModel * model,
 GtkTreePath * path, gboolean was, void * user)
{
    return ! ((ListModel *) model)->frozen;
}

static void select_row_cb (GtkTreeModel * _model, GtkTreePath * path,
 GtkTreeIter * iter, void * user)
{
    ListModel * model = (ListModel *) _model;
    int row = gtk_tree_path_get_indices (path)[0];
    g_return_if_fail (row >= 0 && row < model->rows);
    model->cbs->set_selected (model->user, row, true);
}

static void select_cb (GtkTreeSelection * sel, ListModel * model)
{
    if (model->blocked)
        return;
    model->cbs->select_all (model->user, false);
    gtk_tree_selection_selected_foreach (sel, select_row_cb, nullptr);
}

static void focus_cb (GtkTreeView * tree, ListModel * model)
{
    if (! model->blocked)
        model->cbs->focus_change (model->user,
         audgui_list_get_focus ((GtkWidget *) tree));
}

static void activate_cb (GtkTreeView * tree, GtkTreePath * path,
 GtkTreeViewColumn * col, ListModel * model)
{
    int row = gtk_tree_path_get_indices (path)[0];
    g_return_if_fail (row >= 0 && row < model->rows);
    model->cbs->activate_row (model->user, row);
}

static gboolean button_press_cb (GtkWidget * widget, GdkEventButton * event,
 ListModel * model)
{
    GtkTreePath * path = nullptr;
    gtk_tree_view_get_path_at_pos ((GtkTreeView *) widget, event->x, event->y,
     & path, nullptr, nullptr, nullptr);

    if (event->type == GDK_BUTTON_PRESS && event->button == 3
     && MODEL_HAS_CB (model, right_click))
    {
        /* Only allow GTK to select this row if it is not already selected.  We
         * don't want to clear a multiple selection. */
        if (path)
        {
            if (PATH_IS_SELECTED (widget, path))
                model->frozen = true;
            gtk_tree_view_set_cursor ((GtkTreeView *) widget, path, nullptr, false);
            model->frozen = false;
        }

        model->cbs->right_click (model->user, event);

        if (path)
            gtk_tree_path_free (path);
        return true;
    }

    /* Only allow GTK to select this row if it is not already selected.  If we
     * are going to be dragging, we don't want to clear a multiple selection.
     * If this is just a simple click, we will clear the multiple selection in
     * button_release_cb. */
    if (event->type == GDK_BUTTON_PRESS && event->button == 1 && ! (event->state
     & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) && path && PATH_IS_SELECTED (widget,
     path))
        model->frozen = true;

    if (path)
        model->clicked_row = gtk_tree_path_get_indices (path)[0];
    else
        model->clicked_row = -1;

    if (path)
        gtk_tree_path_free (path);
    return false;
}

static gboolean button_release_cb (GtkWidget * widget, GdkEventButton * event,
 ListModel * model)
{
    /* If button_press_cb set "frozen", and we were not dragging, we need to
     * clear a multiple selection. */
    if (model->frozen && model->clicked_row >= 0 && model->clicked_row <
     model->rows)
    {
        model->frozen = false;
        GtkTreePath * path = gtk_tree_path_new_from_indices (model->clicked_row, -1);
        gtk_tree_view_set_cursor ((GtkTreeView *) widget, path, nullptr, false);
        gtk_tree_path_free (path);
    }

    return false;
}

static gboolean key_press_cb (GtkWidget * widget, GdkEventKey * event, ListModel * model)
{
    /* GTK thinks the spacebar should activate a row; I (jlindgren) disagree */
    if (event->keyval == ' ' && ! (event->state & GDK_CONTROL_MASK))
        return true;

    return false;
}

static gboolean motion_notify_cb (GtkWidget * widget, GdkEventMotion * event, ListModel * model)
{
    if (MODEL_HAS_CB (model, mouse_motion))
    {
        int x, y;
        gtk_tree_view_convert_bin_window_to_widget_coords ((GtkTreeView *)
         widget, event->x, event->y, & x, & y);

        int row = audgui_list_row_at_point (widget, x, y);
        model->cbs->mouse_motion (model->user, event, row);
    }

    return false;
}

static gboolean leave_notify_cb (GtkWidget * widget, GdkEventMotion * event, ListModel * model)
{
    if (MODEL_HAS_CB (model, mouse_leave))
    {
        int x, y;
        gtk_tree_view_convert_bin_window_to_widget_coords ((GtkTreeView *)
         widget, event->x, event->y, & x, & y);

        int row = audgui_list_row_at_point (widget, x, y);
        model->cbs->mouse_leave (model->user, event, row);
    }

    return false;
}

/* ==== DRAG AND DROP ==== */

static void drag_begin (GtkWidget * widget, GdkDragContext * context,
 ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-begin");

    model->dragging = true;

    /* If button_press_cb preserved a multiple selection, tell button_release_cb
     * not to clear it. */
    model->frozen = false;
}

static void drag_end (GtkWidget * widget, GdkDragContext * context,
 ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-end");

    model->dragging = false;
    model->clicked_row = -1;
}

static void drag_data_get (GtkWidget * widget, GdkDragContext * context,
 GtkSelectionData * sel, unsigned info, unsigned time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-data-get");

    Index<char> data = model->cbs->get_data (model->user);
    gtk_selection_data_set (sel, gdk_atom_intern (model->cbs->data_type, false),
     8, (const unsigned char *) data.begin (), data.len ());
}

static void get_scroll_pos (GtkAdjustment * adj, int & pos, int & end)
{
    pos = gtk_adjustment_get_value (adj);
    end = gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj);
}

static bool can_scroll (int pos, int end, int speed)
{
    if (speed > 0)
        return pos < end;
    if (speed < 0)
        return pos > 0;

    return false;
}

static void autoscroll (void * widget)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) widget);

    GtkAdjustment * adj = gtk_tree_view_get_vadjustment ((GtkTreeView *) widget);
    g_return_if_fail (adj);

    int pos, end;
    get_scroll_pos (adj, pos, end);
    pos = aud::clamp (pos + model->scroll_speed, 0, end);
    gtk_adjustment_set_value (adj, pos);

    if (! can_scroll (pos, end, model->scroll_speed))
    {
        model->scroll_speed = 0;
        timer_remove (TimerRate::Hz30, autoscroll, widget);
    }
}

static void start_autoscroll (ListModel * model, GtkWidget * widget, int speed)
{
    GtkAdjustment * adj = gtk_tree_view_get_vadjustment ((GtkTreeView *) widget);
    g_return_if_fail (adj);

    int pos, end;
    get_scroll_pos (adj, pos, end);

    if (can_scroll (pos, end, speed))
    {
        model->scroll_speed = speed;
        timer_add (TimerRate::Hz30, autoscroll, widget);
    }
}

static void stop_autoscroll (ListModel * model, GtkWidget * widget)
{
    model->scroll_speed = 0;
    timer_remove (TimerRate::Hz30, autoscroll, widget);
}

static gboolean drag_motion (GtkWidget * widget, GdkDragContext * context,
 int x, int y, unsigned time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-motion");

    if (model->dragging && MODEL_HAS_CB (model, shift_rows))
        gdk_drag_status (context, GDK_ACTION_MOVE, time);  /* dragging within same list */
    else if (MODEL_HAS_CB (model, data_type) && MODEL_HAS_CB (model, receive_data))
        gdk_drag_status (context, GDK_ACTION_COPY, time);  /* cross-widget dragging */
    else
        return false;

    if (model->rows > 0)
    {
        int row = audgui_list_row_at_point_rounded (widget, x, y);
        if (row == model->rows)
        {
            GtkTreePath * path = gtk_tree_path_new_from_indices (row - 1, -1);
            gtk_tree_view_set_drag_dest_row ((GtkTreeView *) widget, path,
             GTK_TREE_VIEW_DROP_AFTER);
            gtk_tree_path_free (path);
        }
        else
        {
            GtkTreePath * path = gtk_tree_path_new_from_indices (row, -1);
            gtk_tree_view_set_drag_dest_row ((GtkTreeView *) widget, path,
             GTK_TREE_VIEW_DROP_BEFORE);
            gtk_tree_path_free (path);
        }
    }

    gtk_tree_view_convert_widget_to_bin_window_coords ((GtkTreeView *) widget,
     x, y, & x, & y);

    int height = gdk_window_get_height (gtk_tree_view_get_bin_window ((GtkTreeView *) widget));
    int hotspot = aud::min (height / 4, audgui_get_dpi () / 2);

    if (y >= 0 && y < hotspot)
        start_autoscroll (model, widget, y - hotspot);
    else if (y >= height - hotspot && y < height)
        start_autoscroll (model, widget, y - (height - hotspot));
    else
        stop_autoscroll (model, widget);

    return true;
}

static void drag_leave (GtkWidget * widget, GdkDragContext * context,
 unsigned time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-leave");

    gtk_tree_view_set_drag_dest_row ((GtkTreeView *) widget, nullptr, (GtkTreeViewDropPosition) 0);
    stop_autoscroll (model, widget);
}

static gboolean drag_drop (GtkWidget * widget, GdkDragContext * context, int x,
 int y, unsigned time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-drop");

    gboolean success = true;
    int row = audgui_list_row_at_point_rounded (widget, x, y);

    if (model->dragging && MODEL_HAS_CB (model, shift_rows))
    {
        /* dragging within same list */
        if (model->clicked_row >= 0 && model->clicked_row < model->rows)
            model->cbs->shift_rows (model->user, model->clicked_row, row);
        else
            success = false;
    }
    else if (MODEL_HAS_CB (model, data_type) && MODEL_HAS_CB (model, receive_data))
    {
        /* cross-widget dragging */
        model->receive_row = row;
        gtk_drag_get_data (widget, context, gdk_atom_intern
         (model->cbs->data_type, false), time);
    }
    else
        success = false;

    gtk_drag_finish (context, success, false, time);
    gtk_tree_view_set_drag_dest_row ((GtkTreeView *) widget, nullptr, (GtkTreeViewDropPosition) 0);
    stop_autoscroll (model, widget);
    return true;
}

static void drag_data_received (GtkWidget * widget, GdkDragContext * context, int x,
 int y, GtkSelectionData * sel, unsigned info, unsigned time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-data-received");

    g_return_if_fail (model->receive_row >= 0 && model->receive_row <=
     model->rows);

    auto data = (const char *) gtk_selection_data_get_data (sel);
    int length = gtk_selection_data_get_length (sel);

    if (data && length)
        model->cbs->receive_data (model->user, model->receive_row, data, length);

    model->receive_row = -1;
}

/* ==== PUBLIC FUNCS ==== */

static void destroy_cb (GtkWidget * list, ListModel * model)
{
    stop_autoscroll (model, list);
    g_list_free (model->column_types);
    g_object_unref (model);
}

static void update_selection (GtkWidget * list, ListModel * model, int at,
 int rows)
{
    model->blocked = true;
    GtkTreeSelection * sel = gtk_tree_view_get_selection ((GtkTreeView *) list);

    for (int i = at; i < at + rows; i ++)
    {
        GtkTreeIter iter = {0, GINT_TO_POINTER (i)};
        if (model->cbs->get_selected (model->user, i))
            gtk_tree_selection_select_iter (sel, & iter);
        else
            gtk_tree_selection_unselect_iter (sel, & iter);
    }

    model->blocked = false;
}

EXPORT GtkWidget * audgui_list_new_real (const AudguiListCallbacks * cbs, int cbs_size,
 void * user, int rows)
{
    g_return_val_if_fail (cbs->get_value, nullptr);

    ListModel * model = (ListModel *) g_object_new (list_model_get_type (), nullptr);
    model->cbs = cbs;
    model->cbs_size = cbs_size;
    model->user = user;
    model->rows = rows;
    model->highlight = -1;
    model->columns = RESERVED_COLUMNS;
    model->column_types = nullptr;
    model->resizable = true;
    model->frozen = false;
    model->blocked = false;
    model->dragging = false;
    model->clicked_row = -1;
    model->receive_row = -1;
    model->scroll_speed = 0;

    GtkWidget * list = gtk_tree_view_new_with_model ((GtkTreeModel *) model);
    gtk_tree_view_set_fixed_height_mode ((GtkTreeView *) list, true);
    g_signal_connect (list, "destroy", (GCallback) destroy_cb, model);

    model->charwidth = audgui_get_digit_width (list);

    if (MODEL_HAS_CB (model, get_selected) && MODEL_HAS_CB (model, set_selected)
     && MODEL_HAS_CB (model, select_all))
    {
        GtkTreeSelection * sel = gtk_tree_view_get_selection
         ((GtkTreeView *) list);
        gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
        gtk_tree_selection_set_select_function (sel, select_allow_cb, nullptr, nullptr);
        g_signal_connect (sel, "changed", (GCallback) select_cb, model);

        update_selection (list, model, 0, rows);
    }

    if (MODEL_HAS_CB (model, focus_change))
        g_signal_connect (list, "cursor-changed", (GCallback) focus_cb, model);

    if (MODEL_HAS_CB (model, activate_row))
        g_signal_connect (list, "row-activated", (GCallback) activate_cb, model);

    g_signal_connect (list, "button-press-event", (GCallback) button_press_cb, model);
    g_signal_connect (list, "button-release-event", (GCallback) button_release_cb, model);
    g_signal_connect (list, "key-press-event", (GCallback) key_press_cb, model);
    g_signal_connect (list, "motion-notify-event", (GCallback) motion_notify_cb, model);
    g_signal_connect (list, "leave-notify-event", (GCallback) leave_notify_cb, model);

    gboolean supports_drag = false;

    if (MODEL_HAS_CB (model, data_type) &&
     (MODEL_HAS_CB (model, get_data) || MODEL_HAS_CB (model, receive_data)))
    {
        const GtkTargetEntry target = {(char *) cbs->data_type, 0, 0};

        if (MODEL_HAS_CB (model, get_data))
        {
            gtk_drag_source_set (list, GDK_BUTTON1_MASK, & target, 1, GDK_ACTION_COPY);
            g_signal_connect (list, "drag-data-get", (GCallback) drag_data_get, model);
        }

        if (MODEL_HAS_CB (model, receive_data))
        {
            gtk_drag_dest_set (list, (GtkDestDefaults) 0, & target, 1, GDK_ACTION_COPY);
            g_signal_connect (list, "drag-data-received", (GCallback) drag_data_received, model);
        }

        supports_drag = true;
    }
    else if (MODEL_HAS_CB (model, shift_rows))
    {
        gtk_drag_source_set (list, GDK_BUTTON1_MASK, nullptr, 0, GDK_ACTION_COPY);
        gtk_drag_dest_set (list, (GtkDestDefaults) 0, nullptr, 0, GDK_ACTION_COPY);
        supports_drag = true;
    }

    if (supports_drag)
    {
        g_signal_connect (list, "drag-begin", (GCallback) drag_begin, model);
        g_signal_connect (list, "drag-end", (GCallback) drag_end, model);
        g_signal_connect (list, "drag-motion", (GCallback) drag_motion, model);
        g_signal_connect (list, "drag-leave", (GCallback) drag_leave, model);
        g_signal_connect (list, "drag-drop", (GCallback) drag_drop, model);
    }

    return list;
}

EXPORT void * audgui_list_get_user (GtkWidget * list)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    return model->user;
}

EXPORT void audgui_list_add_column (GtkWidget * list, const char * title,
 int column, GType type, int width, bool use_markup)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (RESERVED_COLUMNS + column == model->columns);

    model->columns ++;
    model->column_types = g_list_append (model->column_types, GINT_TO_POINTER
     (type));

    GtkCellRenderer * renderer = gtk_cell_renderer_text_new ();

    GtkTreeViewColumn * tree_column = use_markup ?
        gtk_tree_view_column_new_with_attributes
         (title, renderer, "markup", RESERVED_COLUMNS + column, nullptr) :
        gtk_tree_view_column_new_with_attributes
         (title, renderer, "text", RESERVED_COLUMNS + column, "weight", HIGHLIGHT_COLUMN, nullptr);

    gtk_tree_view_column_set_sizing (tree_column, GTK_TREE_VIEW_COLUMN_FIXED);

    int pad1, pad2, pad3;
    gtk_widget_style_get (list, "horizontal-separator", & pad1, "focus-line-width", & pad2, nullptr);
    gtk_cell_renderer_get_padding (renderer, & pad3, nullptr);
    int padding = pad1 + 2 * pad2 + 2 * pad3;

    if (width < 0)
    {
        gtk_tree_view_column_set_expand (tree_column, true);
        model->resizable = false;  // columns to the right will not be resizable
    }
    else
    {
        gtk_tree_view_column_set_resizable (tree_column, model->resizable);
        gtk_tree_view_column_set_min_width (tree_column,
         width * model->charwidth + model->charwidth / 2 + padding);
    }

    if (width >= 0 && width < 10)
        g_object_set ((GObject *) renderer, "xalign", (float) 1, nullptr);
    else
        g_object_set ((GObject *) renderer, "ellipsize-set", true, "ellipsize",
         PANGO_ELLIPSIZE_END, nullptr);

    gtk_tree_view_append_column ((GtkTreeView *) list, tree_column);
}

EXPORT int audgui_list_row_count (GtkWidget * list)
{
    return ((ListModel *) gtk_tree_view_get_model ((GtkTreeView *) list))->rows;
}

EXPORT void audgui_list_insert_rows (GtkWidget * list, int at, int rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (at >= 0 && at <= model->rows && rows >= 0);

    model->rows += rows;
    if (model->highlight >= at)
        model->highlight += rows;

    GtkTreeIter iter = {0, GINT_TO_POINTER (at)};
    GtkTreePath * path = gtk_tree_path_new_from_indices (at, -1);

    for (int i = rows; i --; )
        gtk_tree_model_row_inserted ((GtkTreeModel *) model, path, & iter);

    gtk_tree_path_free (path);

    if (model->cbs->get_selected)
        update_selection (list, model, at, rows);
}

EXPORT void audgui_list_update_rows (GtkWidget * list, int at, int rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (at >= 0 && rows >= 0 && at + rows <= model->rows);

    GtkTreeIter iter = {0, GINT_TO_POINTER (at)};
    GtkTreePath * path = gtk_tree_path_new_from_indices (at, -1);

    while (rows --)
    {
        gtk_tree_model_row_changed ((GtkTreeModel *) model, path, & iter);
        iter.user_data = GINT_TO_POINTER (GPOINTER_TO_INT (iter.user_data) + 1);
        gtk_tree_path_next (path);
    }

    gtk_tree_path_free (path);
}

EXPORT void audgui_list_delete_rows (GtkWidget * list, int at, int rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (at >= 0 && rows >= 0 && at + rows <= model->rows);

    model->rows -= rows;
    if (model->highlight >= at + rows)
        model->highlight -= rows;
    else if (model->highlight >= at)
        model->highlight = -1;

    model->frozen = true;
    model->blocked = true;

    int focus = audgui_list_get_focus (list);

    // first delete rows after cursor so it does not get moved to one of them
    if (focus >= at && focus + 1 < at + rows)
    {
        GtkTreePath * path = gtk_tree_path_new_from_indices (focus + 1, -1);

        while (focus + 1 < at + rows)
        {
            gtk_tree_model_row_deleted ((GtkTreeModel *) model, path);
            rows --;
        }

        gtk_tree_path_free (path);
    }

    // now delete rows preceding cursor and finally cursor row itself
    GtkTreePath * path = gtk_tree_path_new_from_indices (at, -1);

    while (rows --)
        gtk_tree_model_row_deleted ((GtkTreeModel *) model, path);

    gtk_tree_path_free (path);

    model->frozen = false;
    model->blocked = false;
}

EXPORT void audgui_list_update_selection (GtkWidget * list, int at, int rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (model->cbs->get_selected);
    g_return_if_fail (at >= 0 && rows >= 0 && at + rows <= model->rows);
    update_selection (list, model, at, rows);
}

EXPORT int audgui_list_get_highlight (GtkWidget * list)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    return model->highlight;
}

EXPORT void audgui_list_set_highlight (GtkWidget * list, int row)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (row >= -1 && row < model->rows);

    int old = model->highlight;
    if (row == old)
        return;
    model->highlight = row;

    if (old >= 0)
        audgui_list_update_rows (list, old, 1);
    if (row >= 0)
        audgui_list_update_rows (list, row, 1);
}

EXPORT int audgui_list_get_focus (GtkWidget * list)
{
    GtkTreePath * path = nullptr;
    gtk_tree_view_get_cursor ((GtkTreeView *) list, & path, nullptr);

    if (! path)
        return -1;

    int row = gtk_tree_path_get_indices (path)[0];

    gtk_tree_path_free (path);
    return row;
}

EXPORT void audgui_list_set_focus (GtkWidget * list, int row)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (row >= -1 && row < model->rows);

    if (row < 0 || row == audgui_list_get_focus (list))
        return;

    model->frozen = true;
    model->blocked = true;

    GtkTreePath * path = gtk_tree_path_new_from_indices (row, -1);
    gtk_tree_view_set_cursor ((GtkTreeView *) list, path, nullptr, false);
    gtk_tree_view_scroll_to_cell ((GtkTreeView *) list, path, nullptr, false, 0, 0);
    gtk_tree_path_free (path);

    model->frozen = false;
    model->blocked = false;
}

EXPORT int audgui_list_row_at_point (GtkWidget * list, int x, int y)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model ((GtkTreeView *) list);

    GtkTreePath * path = nullptr;
    gtk_tree_view_convert_widget_to_bin_window_coords ((GtkTreeView *) list, x, y, & x, & y);
    gtk_tree_view_get_path_at_pos ((GtkTreeView *) list, x, y, & path, nullptr, nullptr, nullptr);

    if (! path)
        return -1;

    int row = gtk_tree_path_get_indices (path)[0];
    g_return_val_if_fail (row >= 0 && row < model->rows, -1);

    gtk_tree_path_free (path);
    return row;
}

/* note that this variant always returns a valid row (or row + 1) */
EXPORT int audgui_list_row_at_point_rounded (GtkWidget * list, int x, int y)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model ((GtkTreeView *) list);

    gtk_tree_view_convert_widget_to_bin_window_coords ((GtkTreeView *) list, x, y, & x, & y);

    /* bound the mouse cursor within the bin window to get the nearest row */
    GdkWindow * bin = gtk_tree_view_get_bin_window ((GtkTreeView *) list);
    x = aud::clamp (x, 0, gdk_window_get_width (bin) - 1);
    y = aud::clamp (y, 0, gdk_window_get_height (bin) - 1);

    GtkTreePath * path = nullptr;
    gtk_tree_view_get_path_at_pos ((GtkTreeView *) list, x, y, & path, nullptr, nullptr, nullptr);

    if (! path)
        return model->rows;

    int row = gtk_tree_path_get_indices (path)[0];
    g_return_val_if_fail (row >= 0 && row < model->rows, -1);

    GdkRectangle rect;
    gtk_tree_view_get_background_area ((GtkTreeView *) list, path, nullptr, & rect);
    if (y > rect.y + rect.height / 2)
        row ++;

    gtk_tree_path_free (path);
    return row;
}
