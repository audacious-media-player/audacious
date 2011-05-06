/*
 * list.c
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

#include "list.h"

#include <audacious/gtk-compat.h>

enum {HIGHLIGHT_COLUMN, RESERVED_COLUMNS};

#define PATH_IS_SELECTED(w, p) (gtk_tree_selection_path_is_selected \
 (gtk_tree_view_get_selection ((GtkTreeView *) (w)), (p)))

typedef struct {
    GObject parent;
    const AudguiListCallbacks * cbs;
    void * user;
    gint rows, highlight;
    gint columns;
    GList * column_types;
    gboolean frozen, blocked;
    gboolean dragging;
    gboolean clicked_row, receive_row;
    gint scroll_source, scroll_speed;
} ListModel;

/* ==== MODEL ==== */

static GtkTreeModelFlags list_model_get_flags (GtkTreeModel * model)
{
    return GTK_TREE_MODEL_LIST_ONLY;
}

static gint list_model_get_n_columns (GtkTreeModel * model)
{
    return ((ListModel *) model)->columns;
}

static GType list_model_get_column_type (GtkTreeModel * _model, gint column)
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
    gint row = gtk_tree_path_get_indices (path)[0];
    if (row < 0 || row >= ((ListModel *) model)->rows)
        return FALSE;
    iter->user_data = GINT_TO_POINTER (row);
    return TRUE;
}

static GtkTreePath * list_model_get_path (GtkTreeModel * model,
 GtkTreeIter * iter)
{
    gint row = GPOINTER_TO_INT (iter->user_data);
    g_return_val_if_fail (row >= 0 && row < ((ListModel *) model)->rows, NULL);
    return gtk_tree_path_new_from_indices (row, -1);
}

static void list_model_get_value (GtkTreeModel * _model, GtkTreeIter * iter,
 gint column, GValue * value)
{
    ListModel * model = (ListModel *) _model;
    gint row = GPOINTER_TO_INT (iter->user_data);
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
    gint row = GPOINTER_TO_INT (iter->user_data);
    g_return_val_if_fail (row >= 0 && row < model->rows, FALSE);
    if (row + 1 >= model->rows)
        return FALSE;
    iter->user_data = GINT_TO_POINTER (row + 1);
    return TRUE;
}

static gboolean list_model_iter_children (GtkTreeModel * model,
 GtkTreeIter * iter, GtkTreeIter * parent)
{
    if (parent || ((ListModel *) model)->rows < 1)
        return FALSE;
    iter->user_data = GINT_TO_POINTER (0);
    return TRUE;
}

static gboolean list_model_iter_has_child (GtkTreeModel * model,
 GtkTreeIter * iter)
{
    return FALSE;
}

static gint list_model_iter_n_children (GtkTreeModel * model, GtkTreeIter * iter)
{
    return iter ? 0 : ((ListModel *) model)->rows;
}

static gboolean list_model_iter_nth_child (GtkTreeModel * model,
 GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
    if (parent || n < 0 || n >= ((ListModel *) model)->rows)
        return FALSE;
    iter->user_data = GINT_TO_POINTER (n);
    return TRUE;
}

static gboolean list_model_iter_parent (GtkTreeModel * model,
 GtkTreeIter * iter, GtkTreeIter * child)
{
    return FALSE;
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

static const GInterfaceInfo iface_info =
{
    .interface_init = (GInterfaceInitFunc) iface_init,
    .interface_finalize = NULL,
    .interface_data = NULL,
};

static GType list_model_get_type (void)
{
    static GType type = G_TYPE_INVALID;
    if (type == G_TYPE_INVALID)
    {
        type = g_type_register_static_simple (G_TYPE_OBJECT, "AudguiListModel",
         sizeof (GObjectClass), NULL, sizeof (ListModel), NULL, 0);
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
    gint row = gtk_tree_path_get_indices (path)[0];
    g_return_if_fail (row >= 0 && row < model->rows);
    model->cbs->set_selected (model->user, row, TRUE);
}

static void select_cb (GtkTreeSelection * sel, ListModel * model)
{
    if (model->blocked)
        return;
    model->cbs->select_all (model->user, FALSE);
    gtk_tree_selection_selected_foreach (sel, select_row_cb, NULL);
}

static void activate_cb (GtkTreeView * tree, GtkTreePath * path,
 GtkTreeViewColumn * col, ListModel * model)
{
    gint row = gtk_tree_path_get_indices (path)[0];
    g_return_if_fail (row >= 0 && row < model->rows);
    model->cbs->activate_row (model->user, row);
}

static gboolean button_press_cb (GtkWidget * widget, GdkEventButton * event,
 ListModel * model)
{
    GtkTreePath * path = NULL;
    gtk_tree_view_get_path_at_pos ((GtkTreeView *) widget, event->x, event->y,
     & path, NULL, NULL, NULL);

    if (event->type == GDK_BUTTON_PRESS && event->button == 3 &&
     model->cbs->right_click)
    {
        /* Only allow GTK to select this row if it is not already selected.  We
         * don't want to clear a multiple selection. */
        if (path)
        {
            if (PATH_IS_SELECTED (widget, path))
                model->frozen = TRUE;
            gtk_tree_view_set_cursor ((GtkTreeView *) widget, path, NULL, FALSE);
            model->frozen = FALSE;
        }

        model->cbs->right_click (model->user, event);

        if (path)
            gtk_tree_path_free (path);
        return TRUE;
    }

    /* Only allow GTK to select this row if it is not already selected.  If we
     * are going to be dragging, we don't want to clear a multiple selection.
     * If this is just a simple click, we will clear the multiple selection in
     * button_release_cb. */
    if (event->type == GDK_BUTTON_PRESS && event->button == 1 && ! (event->state
     & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) && path && PATH_IS_SELECTED (widget,
     path))
        model->frozen = TRUE;

    if (path)
        model->clicked_row = gtk_tree_path_get_indices (path)[0];
    else
        model->clicked_row = -1;

    if (path)
        gtk_tree_path_free (path);
    return FALSE;
}

static gboolean button_release_cb (GtkWidget * widget, GdkEventButton * event,
 ListModel * model)
{
    /* If button_press_cb set "frozen", and we were not dragging, we need to
     * clear a multiple selection. */
    if (model->frozen && model->clicked_row >= 0 && model->clicked_row <
     model->rows)
    {
        model->frozen = FALSE;
        GtkTreePath * path = gtk_tree_path_new_from_indices (model->clicked_row,
         -1);
        gtk_tree_view_set_cursor ((GtkTreeView *) widget, path, NULL, FALSE);
        gtk_tree_path_free (path);
    }

    return FALSE;
}

/* ==== DRAG AND DROP ==== */

static void drag_begin (GtkWidget * widget, GdkDragContext * context,
 ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-begin");

    model->dragging = TRUE;
}

static void drag_end (GtkWidget * widget, GdkDragContext * context,
 ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-end");

    model->dragging = FALSE;
    model->clicked_row = -1;
}

static void drag_data_get (GtkWidget * widget, GdkDragContext * context,
 GtkSelectionData * sel, guint info, guint time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-data-get");

    void * data = NULL;
    gint length = 0;
    model->cbs->get_data (model->user, & data, & length);
    gtk_selection_data_set (sel, gdk_atom_intern (model->cbs->data_type, FALSE),
     8, data, length);
    g_free (data);
}

static gint calc_drop_row (ListModel * model, GtkWidget * widget, gint x, gint y)
{
    gint row = audgui_list_row_at_point (widget, x, y);
    if (row < 0)
        row = model->rows;
    return row;
}

static void stop_autoscroll (ListModel * model)
{
    if (! model->scroll_source)
        return;

    g_source_remove (model->scroll_source);
    model->scroll_source = 0;
    model->scroll_speed = 0;
}

static gboolean autoscroll (GtkWidget * widget)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) widget);

    GtkAdjustment * adj = gtk_tree_view_get_vadjustment ((GtkTreeView *) widget);
    if (! adj)
        return FALSE;

    gint new = gtk_adjustment_get_value (adj) + model->scroll_speed;
    gint clamped = CLAMP (new, 0, gtk_adjustment_get_upper (adj) -
     gtk_adjustment_get_page_size (adj));
    gtk_adjustment_set_value (adj, clamped);

    if (clamped != new) /* reached top or bottom? */
        return FALSE;

    if (model->scroll_speed > 0)
        model->scroll_speed = MIN (model->scroll_speed + 2, 100);
    else
        model->scroll_speed = MAX (model->scroll_speed - 2, -100);

    return TRUE;
}

static void start_autoscroll (ListModel * model, GtkWidget * widget, gint speed)
{
    if (model->scroll_source)
        return;

    model->scroll_source = g_timeout_add (50, (GSourceFunc) autoscroll, widget);
    model->scroll_speed = speed;
}

static gboolean drag_motion (GtkWidget * widget, GdkDragContext * context,
 gint x, gint y, guint time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-motion");

    /* If button_press_cb preserved a multiple selection, tell button_release_cb
     * not to clear it. */
    model->frozen = FALSE;

    if (model->dragging && model->cbs->shift_rows) /* dragging within same list */
        gdk_drag_status (context, GDK_ACTION_MOVE, time);
    else if (model->cbs->data_type) /* cross-widget dragging */
        gdk_drag_status (context, GDK_ACTION_COPY, time);
    else
        return FALSE;

    if (model->rows > 0)
    {
        gint row = calc_drop_row (model, widget, x, y);
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

    gint height;
    gdk_window_get_geometry (gtk_tree_view_get_bin_window ((GtkTreeView *)
     widget), NULL, NULL, NULL, & height);
    gtk_tree_view_convert_widget_to_bin_window_coords ((GtkTreeView *) widget,
     x, y, & x, & y);

    if (y >= 0 && y < 48)
        start_autoscroll (model, widget, -2);
    else if (y >= height - 48 && y < height)
        start_autoscroll (model, widget, 2);
    else
        stop_autoscroll (model);

    return TRUE;
}

static void drag_leave (GtkWidget * widget, GdkDragContext * context,
 guint time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-leave");

    gtk_tree_view_set_drag_dest_row ((GtkTreeView *) widget, NULL, 0);
    stop_autoscroll (model);
}

static gboolean drag_drop (GtkWidget * widget, GdkDragContext * context, gint x,
 gint y, guint time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-drop");

    gboolean success = TRUE;
    gint row = calc_drop_row (model, widget, x, y);

    if (model->dragging && model->cbs->shift_rows) /* dragging within same list */
    {
        if (model->clicked_row >= 0 && model->clicked_row < model->rows)
            model->cbs->shift_rows (model->user, model->clicked_row, row);
        else
            success = FALSE;
    }
    else if (model->cbs->data_type) /* cross-widget dragging */
    {
        model->receive_row = row;
        gtk_drag_get_data (widget, context, gdk_atom_intern
         (model->cbs->data_type, FALSE), time);
    }
    else
        success = FALSE;

    gtk_drag_finish (context, success, FALSE, time);
    gtk_tree_view_set_drag_dest_row ((GtkTreeView *) widget, NULL, 0);
    stop_autoscroll (model);
    return TRUE;
}

static void drag_data_received (GtkWidget * widget, GdkDragContext * context, gint x,
 gint y, GtkSelectionData * sel, guint info, guint time, ListModel * model)
{
    g_signal_stop_emission_by_name (widget, "drag-data-received");

    g_return_if_fail (model->receive_row >= 0 && model->receive_row <=
     model->rows);

    const guchar * data = gtk_selection_data_get_data (sel);
    gint length = gtk_selection_data_get_length (sel);

    if (data && length)
        model->cbs->receive_data (model->user, model->receive_row, data, length);

    model->receive_row = -1;
}

/* ==== PUBLIC FUNCS ==== */

static void destroy_cb (ListModel * model)
{
    stop_autoscroll (model);
    g_object_unref (model);
}

static void update_selection (GtkWidget * list, ListModel * model, gint at,
 gint rows)
{
    model->blocked = TRUE;
    GtkTreeSelection * sel = gtk_tree_view_get_selection ((GtkTreeView *) list);

    for (gint i = at; i < at + rows; i ++)
    {
        GtkTreeIter iter = {.user_data = GINT_TO_POINTER (i)};
        if (model->cbs->get_selected (model->user, i))
            gtk_tree_selection_select_iter (sel, & iter);
        else
            gtk_tree_selection_unselect_iter (sel, & iter);
    }

    model->blocked = FALSE;
}

GtkWidget * audgui_list_new (const AudguiListCallbacks * cbs, void * user,
 gint rows)
{
    g_return_val_if_fail (cbs->get_value, NULL);
    if (cbs->get_selected)
        g_return_val_if_fail (cbs->set_selected && cbs->select_all, NULL);
    if (cbs->data_type)
        g_return_val_if_fail (cbs->get_data && cbs->receive_data, NULL);

    ListModel * model = (ListModel *) g_object_new (list_model_get_type (), NULL);
    model->cbs = cbs;
    model->user = user;
    model->rows = rows;
    model->highlight = -1;
    model->columns = RESERVED_COLUMNS;
    model->column_types = NULL;
    model->frozen = FALSE;
    model->blocked = FALSE;
    model->dragging = FALSE;
    model->clicked_row = -1;
    model->receive_row = -1;
    model->scroll_source = 0;
    model->scroll_speed = 0;

    GtkWidget * list = gtk_tree_view_new_with_model ((GtkTreeModel *) model);
    g_signal_connect_swapped (list, "destroy", (GCallback) destroy_cb, model);

    if (cbs->get_selected)
    {
        GtkTreeSelection * sel = gtk_tree_view_get_selection
         ((GtkTreeView *) list);
        gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
        gtk_tree_selection_set_select_function (sel, select_allow_cb, NULL, NULL);
        g_signal_connect (sel, "changed", (GCallback) select_cb, model);

        update_selection (list, model, 0, rows);
    }

    if (cbs->activate_row)
        g_signal_connect (list, "row-activated", (GCallback) activate_cb, model);

    g_signal_connect (list, "button-press-event", (GCallback) button_press_cb,
     model);
    g_signal_connect (list, "button-release-event", (GCallback)
     button_release_cb, model);

    if (cbs->data_type)
    {
        const GtkTargetEntry target = {(gchar *) cbs->data_type, 0, 0};

        gtk_drag_source_set (list, GDK_BUTTON1_MASK, & target, 1,
         GDK_ACTION_COPY);
        gtk_drag_dest_set (list, 0, & target, 1, GDK_ACTION_COPY);

        g_signal_connect (list, "drag-data-get", (GCallback) drag_data_get,
         model);
        g_signal_connect (list, "drag-data-received", (GCallback)
         drag_data_received, model);
    }
    else if (cbs->shift_rows)
    {
        gtk_drag_source_set (list, GDK_BUTTON1_MASK, NULL, 0, GDK_ACTION_COPY);
        gtk_drag_dest_set (list, 0, NULL, 0, GDK_ACTION_COPY);
    }

    if (cbs->data_type || cbs->shift_rows)
    {
        g_signal_connect (list, "drag-begin", (GCallback) drag_begin, model);
        g_signal_connect (list, "drag-end", (GCallback) drag_end, model);
        g_signal_connect (list, "drag-motion", (GCallback) drag_motion, model);
        g_signal_connect (list, "drag-leave", (GCallback) drag_leave, model);
        g_signal_connect (list, "drag-drop", (GCallback) drag_drop, model);
    }

    return list;
}

void * audgui_list_get_user (GtkWidget * list)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    return model->user;
}

void audgui_list_add_column (GtkWidget * list, const gchar * title,
 gint column, GType type, gboolean expand)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (RESERVED_COLUMNS + column == model->columns);

    model->columns ++;
    model->column_types = g_list_append (model->column_types, GINT_TO_POINTER
     (type));

    GtkCellRenderer * renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn * tree_column = gtk_tree_view_column_new_with_attributes
     (title, renderer, "text", RESERVED_COLUMNS + column, "weight",
     HIGHLIGHT_COLUMN, NULL);

    if (expand)
    {
        gtk_tree_view_column_set_resizable (tree_column, TRUE);
        gtk_tree_view_column_set_expand (tree_column, TRUE);
        g_object_set ((GObject *) renderer, "ellipsize-set", TRUE, "ellipsize",
         PANGO_ELLIPSIZE_END, NULL);
    }
    else
    {
        gtk_tree_view_column_set_sizing (tree_column,
         GTK_TREE_VIEW_COLUMN_GROW_ONLY);
        g_object_set ((GObject *) renderer, "xalign", (gfloat) 1, NULL);
    }

    gtk_tree_view_append_column ((GtkTreeView *) list, tree_column);
}

gint audgui_list_row_count (GtkWidget * list)
{
    return ((ListModel *) gtk_tree_view_get_model ((GtkTreeView *) list))->rows;
}

void audgui_list_insert_rows (GtkWidget * list, gint at, gint rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (at >= 0 && at <= model->rows && rows >= 0);

    model->rows += rows;
    if (model->highlight >= at)
        model->highlight += rows;

    GtkTreeIter iter = {.user_data = GINT_TO_POINTER (at)};
    GtkTreePath * path = gtk_tree_path_new_from_indices (at, -1);

    for (gint i = rows; i --; )
        gtk_tree_model_row_inserted ((GtkTreeModel *) model, path, & iter);

    gtk_tree_path_free (path);

    update_selection (list, model, at, rows);
}

void audgui_list_update_rows (GtkWidget * list, gint at, gint rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (at >= 0 && rows >= 0 && at + rows <= model->rows);

    GtkTreeIter iter = {.user_data = GINT_TO_POINTER (at)};
    GtkTreePath * path = gtk_tree_path_new_from_indices (at, -1);

    while (rows --)
    {
        gtk_tree_model_row_changed ((GtkTreeModel *) model, path, & iter);
        iter.user_data = GINT_TO_POINTER (GPOINTER_TO_INT (iter.user_data) + 1);
        gtk_tree_path_next (path);
    }

    gtk_tree_path_free (path);
}

void audgui_list_delete_rows (GtkWidget * list, gint at, gint rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (at >= 0 && rows >= 0 && at + rows <= model->rows);

    model->rows -= rows;
    if (model->highlight >= at + rows)
        model->highlight -= rows;
    else if (model->highlight >= at)
        model->highlight = -1;

    model->blocked = TRUE;
    GtkTreePath * path = gtk_tree_path_new_from_indices (at, -1);

    while (rows --)
        gtk_tree_model_row_deleted ((GtkTreeModel *) model, path);

    gtk_tree_path_free (path);
    model->blocked = FALSE;
}

void audgui_list_update_selection (GtkWidget * list, gint at, gint rows)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (model->cbs->get_selected);
    g_return_if_fail (at >= 0 && rows >= 0 && at + rows <= model->rows);
    update_selection (list, model, at, rows);
}

void audgui_list_set_highlight (GtkWidget * list, gint row)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (row >= -1 && row < model->rows);

    gint old = model->highlight;
    if (row == old)
        return;
    model->highlight = row;

    if (old >= 0)
        audgui_list_update_rows (list, old, 1);
    if (row >= 0)
        audgui_list_update_rows (list, row, 1);
}

void audgui_list_set_focus (GtkWidget * list, gint row)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model
     ((GtkTreeView *) list);
    g_return_if_fail (row >= -1 && row < model->rows);

    if (row < 0)
    {
        if (model->rows < 1)
            return;
        row = 0;
    }

    model->frozen = TRUE;
    GtkTreePath * path = gtk_tree_path_new_from_indices (row, -1);
    gtk_tree_view_set_cursor ((GtkTreeView *) list, path, NULL, FALSE);
    gtk_tree_view_scroll_to_cell ((GtkTreeView *) list, path, NULL, FALSE, 0, 0);
    gtk_tree_path_free (path);
    model->frozen = FALSE;
}

gint audgui_list_row_at_point (GtkWidget * list, gint x, gint y)
{
    ListModel * model = (ListModel *) gtk_tree_view_get_model ((GtkTreeView *)
     list);

    GtkTreePath * path = NULL;
    gtk_tree_view_convert_widget_to_bin_window_coords ((GtkTreeView *) list, x,
     y, & x, & y);
    gtk_tree_view_get_path_at_pos ((GtkTreeView *) list, x, y, & path, NULL,
     NULL, NULL);

    if (! path)
        return -1;

    gint row = gtk_tree_path_get_indices (path)[0];
    g_return_val_if_fail (row >= 0 && row < model->rows, -1);

    GdkRectangle rect;
    gtk_tree_view_get_background_area ((GtkTreeView *) list, path, NULL,
     & rect);
    if (y > rect.y + rect.height / 2)
        row ++;

    gtk_tree_path_free (path);
    return row;
}
