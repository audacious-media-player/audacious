/*
 * plugin-view.c
 * Copyright 2010-2012 John Lindgren
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

#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/plugins.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

enum {
    PVIEW_COL_NODE,
    PVIEW_COL_ENABLED,
    PVIEW_COL_NAME,
    PVIEW_COLS
};

struct Node {
    PluginHandle * p;
    GtkTreeModel * model;
    GtkTreePath * path;
};

static PluginHandle * get_selected_plugin (GtkTreeView * tree)
{
    Node * n = nullptr;

    GtkTreeSelection * sel = gtk_tree_view_get_selection (tree);

    /* the treeview may not have a model yet */
    if (! sel)
        return nullptr;

    GtkTreeModel * model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected (sel, & model, & iter))
        gtk_tree_model_get (model, & iter, PVIEW_COL_NODE, & n, -1);

    return n == nullptr ? nullptr : n->p;
}

static void do_enable (GtkCellRendererToggle * cell, const char * path_str,
 GtkTreeModel * model)
{
    GtkTreePath * path = gtk_tree_path_new_from_string (path_str);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (model, & iter, path);
    gtk_tree_path_free (path);

    Node * n = nullptr;
    gboolean enabled;
    gtk_tree_model_get (model, & iter, PVIEW_COL_NODE, & n,
     PVIEW_COL_ENABLED, & enabled, -1);
    g_return_if_fail (n != nullptr);

    aud_plugin_enable (n->p, ! enabled);
}

static bool list_watcher (PluginHandle * p, void * data)
{
    auto n = (Node *) data;

    GtkTreeIter iter;
    gtk_tree_model_get_iter (n->model, & iter, n->path);
    gtk_list_store_set ((GtkListStore *) n->model, & iter, PVIEW_COL_ENABLED,
     aud_plugin_get_enabled (n->p), -1);

    return true;
}

static void add_to_list (GtkTreeModel * model, PluginHandle * p)
{
    Node * n = new Node;

    GtkTreeIter iter;
    gtk_list_store_append ((GtkListStore *) model, & iter);
    gtk_list_store_set ((GtkListStore *) model, & iter, PVIEW_COL_NODE, n,
     PVIEW_COL_ENABLED, aud_plugin_get_enabled (p), PVIEW_COL_NAME,
     aud_plugin_get_name (p), -1);

    n->p = p;
    n->model = model;
    n->path = gtk_tree_model_get_path (model, & iter);

    aud_plugin_add_watch (p, list_watcher, n);
}

static void list_fill (GtkTreeView * tree, void * type)
{
    GtkTreeModel * model = (GtkTreeModel *) gtk_list_store_new (PVIEW_COLS,
     G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_STRING);
    gtk_tree_view_set_model (tree, model);

    GtkTreeViewColumn * col = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    gtk_tree_view_column_set_resizable (col, false);
    gtk_tree_view_append_column (tree, col);

    GtkCellRenderer * rend = gtk_cell_renderer_toggle_new ();
    g_signal_connect (rend, "toggled", (GCallback) do_enable, model);
    gtk_tree_view_column_pack_start (col, rend, false);
    gtk_tree_view_column_set_attributes (col, rend, "active", PVIEW_COL_ENABLED,
     nullptr);

    col = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_expand (col, true);
    gtk_tree_view_column_set_resizable (col, false);
    gtk_tree_view_append_column (tree, col);

    rend = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (col, rend, false);
    gtk_tree_view_column_set_attributes (col, rend, "text", PVIEW_COL_NAME, nullptr);

    for (PluginHandle * plugin : aud_plugin_list (aud::from_ptr<PluginType> (type)))
        add_to_list (model, plugin);
}

static void list_destroy (GtkTreeView * tree)
{
    GtkTreeModel * model = gtk_tree_view_get_model (tree);
    if (model == nullptr)
        return;

    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first (model, & iter))
    {
        do
        {
            Node * n = nullptr;
            gtk_tree_model_get (model, & iter, PVIEW_COL_NODE, & n, -1);
            g_return_if_fail (n != nullptr);

            aud_plugin_remove_watch (n->p, list_watcher, n);
            gtk_tree_path_free (n->path);
            delete n;
        }
        while (gtk_tree_model_iter_next (model, & iter));
    }

    g_object_unref ((GObject *) model);
}

static bool watcher (PluginHandle * p, void * b)
{
    bool is_about = GPOINTER_TO_INT (g_object_get_data ((GObject *) b, "is_about"));

    if (is_about)
        gtk_widget_set_sensitive ((GtkWidget *) b,
         aud_plugin_has_about (p) && aud_plugin_get_enabled (p));
    else
        gtk_widget_set_sensitive ((GtkWidget *) b,
         aud_plugin_has_configure (p) && aud_plugin_get_enabled (p));

    return true;
}

static void button_update (GtkTreeView * tree, GtkWidget * b)
{
    PluginHandle * p = (PluginHandle *) g_object_steal_data ((GObject *) b, "plugin");
    if (p != nullptr)
        aud_plugin_remove_watch (p, watcher, b);

    p = get_selected_plugin (tree);
    if (p != nullptr)
    {
        g_object_set_data ((GObject *) b, "plugin", p);
        watcher (p, b);
        aud_plugin_add_watch (p, watcher, b);
    }
    else
        gtk_widget_set_sensitive (b, false);
}

static void do_config (void * tree)
{
    PluginHandle * plugin = get_selected_plugin ((GtkTreeView *) tree);
    g_return_if_fail (plugin != nullptr);
    audgui_show_plugin_prefs (plugin);
}

static void do_about (void * tree)
{
    PluginHandle * plugin = get_selected_plugin ((GtkTreeView *) tree);
    g_return_if_fail (plugin != nullptr);
    audgui_show_plugin_about (plugin);
}

static void button_destroy (GtkWidget * b)
{
    PluginHandle * p = (PluginHandle *) g_object_steal_data ((GObject *) b, "plugin");
    if (p != nullptr)
        aud_plugin_remove_watch (p, watcher, b);
}

GtkWidget * plugin_view_new (PluginType type)
{
    GtkWidget * vbox = gtk_vbox_new (false, 6);
    gtk_container_set_border_width ((GtkContainer *) vbox, 6);

    GtkWidget * scrolled = gtk_scrolled_window_new (nullptr, nullptr);
    gtk_box_pack_start ((GtkBox *) vbox, scrolled, true, true, 0);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scrolled,
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scrolled, GTK_SHADOW_IN);

    GtkWidget * tree = gtk_tree_view_new ();
    gtk_container_add ((GtkContainer *) scrolled, tree);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) tree, false);
    g_signal_connect (tree, "realize", (GCallback) list_fill, aud::to_ptr (type));
    g_signal_connect (tree, "destroy", (GCallback) list_destroy, nullptr);

    GtkWidget * hbox = gtk_hbox_new (false, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox, false, false, 0);

    GtkWidget * config = audgui_button_new (_("_Settings"), "preferences-system", do_config, tree);
    gtk_box_pack_start ((GtkBox *) hbox, config, false, false, 0);
    gtk_widget_set_sensitive (config, false);
    g_object_set_data ((GObject *) config, "is_about", GINT_TO_POINTER (false));
    g_signal_connect (tree, "cursor-changed", (GCallback) button_update, config);
    g_signal_connect (config, "destroy", (GCallback) button_destroy, nullptr);

    GtkWidget * about = audgui_button_new (_("_About"), "help-about", do_about, tree);
    gtk_box_pack_start ((GtkBox *) hbox, about, false, false, 0);
    gtk_widget_set_sensitive (about, false);
    g_object_set_data ((GObject *) about, "is_about", GINT_TO_POINTER (true));
    g_signal_connect (tree, "cursor-changed", (GCallback) button_update, about);
    g_signal_connect (about, "destroy", (GCallback) button_destroy, nullptr);

    return vbox;
}
