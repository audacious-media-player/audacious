/*
 * libaudgui/library-store.c
 * Copyright 2010 John Lindgren
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

#include <audacious/plugin.h>

#include "libaudgui-gtk.h"

typedef GObjectClass LibraryStoreClass;

typedef struct
{
    GObject parent;
    gint rows, active;
}
LibraryStore;

static void library_store_init (LibraryStore * store)
{
    store->rows = aud_playlist_count ();
    store->active = aud_playlist_get_active ();
}

static GtkTreeModelFlags library_store_get_flags (GtkTreeModel * model)
{
    return GTK_TREE_MODEL_LIST_ONLY;
}

static gint library_store_get_n_columns (GtkTreeModel * model)
{
    return AUDGUI_LIBRARY_STORE_COLUMNS;
}

static GType library_store_get_column_type (GtkTreeModel * model, gint column)
{
    switch (column)
    {
    case AUDGUI_LIBRARY_STORE_TITLE:
        return G_TYPE_STRING;
    case AUDGUI_LIBRARY_STORE_FONT_WEIGHT:
        return PANGO_TYPE_WEIGHT;
    case AUDGUI_LIBRARY_STORE_ENTRY_COUNT:
        return G_TYPE_INT;
    default:
        return G_TYPE_INVALID;
    }
}

static gboolean library_store_get_iter (GtkTreeModel * model, GtkTreeIter *
 iter, GtkTreePath * path)
{
    LibraryStore * store = (LibraryStore *) model;
    gint playlist = gtk_tree_path_get_indices (path)[0];

    if (playlist < 0 || playlist >= store->rows)
        return FALSE;

    iter->user_data = GINT_TO_POINTER (playlist);
    return TRUE;
}

static GtkTreePath * library_store_get_path (GtkTreeModel * model, GtkTreeIter *
 iter)
{
    return gtk_tree_path_new_from_indices (GPOINTER_TO_INT (iter->user_data), -1);
}

static void library_store_get_value (GtkTreeModel * model, GtkTreeIter * iter,
 gint column, GValue * value)
{
    LibraryStore * store = (LibraryStore *) model;
    gint playlist = GPOINTER_TO_INT (iter->user_data);
    
    switch (column)
    {
    case AUDGUI_LIBRARY_STORE_TITLE:
        g_value_init (value, G_TYPE_STRING);
        g_value_set_string (value, aud_playlist_get_title (playlist));
        break;
    case AUDGUI_LIBRARY_STORE_FONT_WEIGHT:
        g_value_init (value, PANGO_TYPE_WEIGHT);
        g_value_set_enum (value, (playlist == store->active) ? PANGO_WEIGHT_BOLD
         : PANGO_WEIGHT_NORMAL);
        break;
    case AUDGUI_LIBRARY_STORE_ENTRY_COUNT:
        g_value_init (value, G_TYPE_INT);
        g_value_set_int (value, aud_playlist_entry_count (playlist));
        break;
    }
}

static gboolean library_store_iter_next (GtkTreeModel * model, GtkTreeIter *
 iter)
{
    if (GPOINTER_TO_INT (iter->user_data) + 1 < aud_playlist_count ())
    {
        iter->user_data = GINT_TO_POINTER (GPOINTER_TO_INT (iter->user_data) + 1);
        return TRUE;
    }
    
    return FALSE;
}

static gboolean library_store_iter_children (GtkTreeModel * model, GtkTreeIter *
 iter, GtkTreeIter * parent)
{
    if (parent == NULL) /* top level */
    {
        /* there is always at least one playlist */
        iter->user_data = GINT_TO_POINTER (0);
        return TRUE;
    }
    
    return FALSE;
}

static gboolean library_store_iter_has_child (GtkTreeModel * model,
 GtkTreeIter * iter)
{
    return FALSE;
}

static gint library_store_iter_n_children (GtkTreeModel * model, GtkTreeIter *
 iter)
{
    LibraryStore * store = (LibraryStore *) model;

    if (iter == NULL) /* top level */
        return store->rows;
    
    return 0;
}

static gboolean library_store_iter_nth_child (GtkTreeModel * model,
 GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
    LibraryStore * store = (LibraryStore *) model;

    if (parent != NULL) /* not top level */
        return FALSE;

    if (n < 0 || n >= store->rows)
        return FALSE;

    iter->user_data = GINT_TO_POINTER (n);
    return TRUE;
}

static gboolean library_store_iter_parent (GtkTreeModel * model, GtkTreeIter *
 iter, GtkTreeIter * child)
{
    return FALSE;
}

static void interface_init (GtkTreeModelIface * interface)
{
    interface->get_flags = library_store_get_flags;
    interface->get_n_columns = library_store_get_n_columns;
    interface->get_column_type = library_store_get_column_type;
    interface->get_iter = library_store_get_iter;
    interface->get_path = library_store_get_path;
    interface->get_value = library_store_get_value;
    interface->iter_next = library_store_iter_next;
    interface->iter_children = library_store_iter_children;
    interface->iter_has_child = library_store_iter_has_child;
    interface->iter_n_children = library_store_iter_n_children;
    interface->iter_nth_child = library_store_iter_nth_child;
    interface->iter_parent = library_store_iter_parent;
}

static const GInterfaceInfo interface_info =
{
    .interface_init = (GInterfaceInitFunc) interface_init,
    .interface_finalize = NULL,
    .interface_data = NULL,
};

static GType library_store_get_type (void)
{
    static GType type = 0;

    if (! type)
    {
        type = g_type_register_static_simple (G_TYPE_OBJECT, "LibraryStore",
         sizeof (LibraryStoreClass), NULL, sizeof (LibraryStore),
         (GInstanceInitFunc) library_store_init, 0);
        g_type_add_interface_static (type, GTK_TYPE_TREE_MODEL, & interface_info);
    }

    return type;
}

static void library_store_update (GtkTreeModel * model)
{
    LibraryStore * store = (LibraryStore *) model;
    gint old_rows = store->rows;
    GtkTreePath * path;
    GtkTreeIter iter;
    gint row;
    
    store->rows = aud_playlist_count ();
    store->active = aud_playlist_get_active ();

    if (store->rows < old_rows)
    {
        path = gtk_tree_path_new_from_indices (store->rows, -1);

        for (row = store->rows; row < old_rows; row ++)
            gtk_tree_model_row_deleted (model, path);
        
        gtk_tree_path_free (path);
        old_rows = store->rows;
    }

    path = gtk_tree_path_new_first ();

    for (row = 0; row < old_rows; row ++)
    {
        iter.user_data = GINT_TO_POINTER (row);
        gtk_tree_model_row_changed (model, path, & iter);
        gtk_tree_path_next (path);
    }
    
    for (; row < store->rows; row ++)
    {
        iter.user_data = GINT_TO_POINTER (row);
        gtk_tree_model_row_inserted (model, path, & iter);
        gtk_tree_path_next (path);
    }
    
    gtk_tree_path_free (path);
}

static void update_cb (void * data, void * user_data)
{
    if (GPOINTER_TO_INT (data) >= PLAYLIST_UPDATE_STRUCTURE)
        library_store_update ((GtkTreeModel *) user_data);
}

GtkTreeModel * audgui_get_library_store (void)
{
    static GtkTreeModel * store = NULL;
    
    if (store == NULL)
    {
        store = (GtkTreeModel *) g_object_new (library_store_get_type (), NULL);
        aud_hook_associate ("playlist update", update_cb, store);
    }
    
    return store;
}
