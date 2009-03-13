/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2008 Tomasz Moń <desowin@gmail.com>
 *  Copyright (C) 2009 William Pitcock <nenolod@atheme.org>
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "playlist.h"
#include "playback.h"
#include "strings.h"

enum {
    COLUMN_NUM = 0,
    COLUMN_TEXT,
    COLUMN_TIME,
    COLUMN_WEIGHT,
    N_COLUMNS
};

typedef struct {
    gint old_index;
    gint new_index;
    GtkTreeRowReference *ref;
} UiPlaylistDragTracker;

static gint
ui_playlist_widget_get_index_from_path(GtkTreePath *path)
{
    gint *pos;

    g_return_val_if_fail(path != NULL, -1);

    if (!(pos = gtk_tree_path_get_indices(path)))
        return -1;

    return pos[0];
}

static void
_ui_playlist_widget_drag_begin(GtkTreeView *widget, GdkDragContext *context, gpointer data)
{
    UiPlaylistDragTracker *t = g_slice_new0(UiPlaylistDragTracker);
    GtkTreeModel *model;
    GtkTreeSelection *sel;
    GtkTreePath *path;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(widget);
    sel = gtk_tree_view_get_selection(widget);

    if (!gtk_tree_selection_get_selected(sel, NULL, &iter))
        return;

    model = gtk_tree_view_get_model(widget);
    path = gtk_tree_model_get_path(model, &iter);
    t->old_index = ui_playlist_widget_get_index_from_path(path);

    g_object_set_data(G_OBJECT(widget), "ui_playlist_drag_context", t);
}

static int
_ui_playlist_widget_get_drop_index(GtkTreeView *widget, GdkDragContext *context, gint x, gint y)
{
    GtkTreePath *path;
    gint cx, cy, ins_pos = -1;

    gdk_window_get_geometry(gtk_tree_view_get_bin_window(widget), &cx, &cy, NULL, NULL, NULL);

    if (gtk_tree_view_get_path_at_pos(widget, x -= cx, y -= cy, &path, NULL, &cx, &cy))
    {
        GdkRectangle rect;

        /* in lower 1/3 of row? use next row as target */
        gtk_tree_view_get_background_area(widget, path, gtk_tree_view_get_column(widget, 0), &rect);
        
        if (cy >= rect.height * 2 / 3.0)
        {
            gtk_tree_path_free(path);

            if (gtk_tree_view_get_path_at_pos(widget, x, y + rect.height, &path, NULL, &cx, &cy))
                ins_pos = ui_playlist_widget_get_index_from_path(path);
        }
        else
            ins_pos = ui_playlist_widget_get_index_from_path(path);

        gtk_tree_path_free(path);
    }

    return ins_pos;
} 

static void
_ui_playlist_widget_drag_data_received(GtkTreeView *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info2, guint time)
{
    UiPlaylistDragTracker *t;
    t = g_object_get_data(G_OBJECT(widget), "ui_playlist_drag_context");

    t->new_index = _ui_playlist_widget_get_drop_index(widget, context, x, y);
}

static void
_ui_playlist_widget_drag_end(GtkTreeView *widget, GdkDragContext *context, gpointer data)
{
    Playlist *playlist = g_object_get_data(G_OBJECT(widget), "my_playlist");
    gint delta;
    UiPlaylistDragTracker *t;

    t = g_object_get_data(G_OBJECT(widget), "ui_playlist_drag_context");

    delta = t->new_index - t->old_index;

    playlist_shift(playlist, delta);
    g_slice_free(UiPlaylistDragTracker, t);
}

static void
ui_playlist_widget_change_song(GtkTreeView *treeview, guint pos)
{
    Playlist *playlist = g_object_get_data(G_OBJECT(treeview), "my_playlist");
    playlist_set_position(playlist, pos);

    if (!playback_get_playing())
        playback_initiate();
}

static void
ui_playlist_widget_set_title_active(GtkTreeModel *model, gint pos,
                                    gboolean active)
{
    GtkTreeIter iter;
    GtkTreePath *path;

    path = gtk_tree_path_new_from_indices(pos, -1);
    gtk_tree_model_get_iter(model, &iter, path);

    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COLUMN_WEIGHT, active ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL, -1);

    gtk_tree_path_free(path);
}

void
ui_playlist_widget_set_current(GtkWidget *treeview, gint pos)
{
    GtkTreeModel *model;
    gint old_pos;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    old_pos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(treeview), "current"));

    if (old_pos != -1)
        ui_playlist_widget_set_title_active(model, old_pos, FALSE);
    if (pos != -1)
        ui_playlist_widget_set_title_active(model, pos, TRUE);

    g_object_set_data(G_OBJECT(treeview), "current", GINT_TO_POINTER(pos));
}

static void
ui_playlist_widget_jump(GtkTreeView * treeview, gpointer data)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreePath *path;
    guint pos;

    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(treeview);

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    path = gtk_tree_model_get_path(model, &iter);
    pos = ui_playlist_widget_get_index_from_path(path);

    ui_playlist_widget_change_song(treeview, pos);
}

static gboolean
ui_playlist_widget_keypress_cb(GtkWidget * widget,
                            GdkEventKey * event,
                            gpointer data)
{
    switch (event->keyval) {
    case GDK_KP_Enter:
        ui_playlist_widget_jump(GTK_TREE_VIEW(widget), NULL);
        return TRUE;
    default:
        return FALSE;
    };
}

void
ui_playlist_widget_update(GtkWidget *widget)
{
    guint row;
    gboolean valid;

    GList *playlist_glist;
    gchar *desc_buf = NULL;
    gchar *length = NULL;
    GtkTreeIter iter;
    Playlist *playlist;
    GtkTreeModel *store;
    GtkTreeView *tree = GTK_TREE_VIEW(widget);

    store = gtk_tree_view_get_model(tree);
    valid = gtk_tree_model_get_iter_first(store, &iter);

    row = 1;
    playlist = g_object_get_data(G_OBJECT(widget), "my_playlist");
    g_message("widget_update: playlist:%s", playlist->filename);

    for (playlist_glist = playlist->entries; playlist_glist;
         playlist_glist = g_list_next(playlist_glist)) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(playlist_glist->data);

        if (entry->title)
            desc_buf = g_strdup(entry->title);
        else {
            gchar *realfn = NULL;
            realfn = g_filename_from_uri(entry->filename, NULL, NULL);
            if (strchr(realfn ? realfn : entry->filename, '/'))
                desc_buf = str_assert_utf8(strrchr(realfn ? realfn : entry->filename, '/') + 1);
            else
                desc_buf = str_assert_utf8(realfn ? realfn : entry->filename);
            g_free(realfn); realfn = NULL;
        }

        if (entry->length != -1) {
            length = g_strdup_printf("%d:%-2.2d", entry->length / 60000, (entry->length / 1000) % 60);
        }

        if (!valid)
            gtk_list_store_append(GTK_LIST_STORE(store), &iter);
        gtk_list_store_set(GTK_LIST_STORE(store), &iter,
                           COLUMN_NUM, row, COLUMN_TEXT, desc_buf,
                           COLUMN_TIME, length,
                           COLUMN_WEIGHT, PANGO_WEIGHT_NORMAL, -1);
        row++;

        g_free(desc_buf);
        desc_buf = NULL;

        if (length) g_free(length);
        length = NULL;

        valid = gtk_tree_model_iter_next(store, &iter);
    }

    /* remove additional rows */
    while (valid) {
        valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
    }

    ui_playlist_widget_set_current(widget, playlist_get_position(playlist));
}

static gboolean
ui_playlist_widget_fill(gpointer treeview)
{
    GList *playlist_glist;
    Playlist *playlist;
    gchar *desc_buf = NULL;
    gchar *length = NULL;
    guint row;
    GtkTreeIter iter;
    GtkListStore *store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );

    /* detach model from treeview before fill */
    g_object_ref(store);
    gtk_tree_view_set_model( GTK_TREE_VIEW(treeview), NULL );

    gtk_list_store_clear(store);

    row = 1;
    playlist = g_object_get_data(G_OBJECT(treeview), "my_playlist");

    PLAYLIST_LOCK(playlist);
    for (playlist_glist = playlist->entries; playlist_glist;
         playlist_glist = g_list_next(playlist_glist)) {

        PlaylistEntry *entry = PLAYLIST_ENTRY(playlist_glist->data);

        if (entry->title)
            desc_buf = g_strdup(entry->title);
        else {
            gchar *realfn = NULL;
            realfn = g_filename_from_uri(entry->filename, NULL, NULL);
            if (strchr(realfn ? realfn : entry->filename, '/'))
                desc_buf = str_assert_utf8(strrchr(realfn ? realfn : entry->filename, '/') + 1);
            else
                desc_buf = str_assert_utf8(realfn ? realfn : entry->filename);
            g_free(realfn); realfn = NULL;
        }

        if (entry->length != -1) {
            length = g_strdup_printf("%d:%-2.2d", entry->length / 60000, (entry->length / 1000) % 60);
        }

        gtk_list_store_append(GTK_LIST_STORE(store), &iter);
        gtk_list_store_set(GTK_LIST_STORE(store), &iter,
                           COLUMN_NUM, row, COLUMN_TEXT, desc_buf,
                           COLUMN_TIME, length,
                           COLUMN_WEIGHT, PANGO_WEIGHT_NORMAL, -1);
        row++;

        g_free(desc_buf);
        desc_buf = NULL;

        if (length) g_free(length);
        length = NULL;
    }
    PLAYLIST_UNLOCK(playlist);

    /* attach liststore to treeview */
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
    g_object_unref(store);

    return FALSE;
}

GtkWidget *
ui_playlist_widget_new(Playlist *playlist)
{
    GtkWidget *treeview;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    store = gtk_list_store_new(N_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, PANGO_TYPE_WEIGHT);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(treeview), TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);

    column = gtk_tree_view_column_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(column, 4);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", COLUMN_NUM, "weight", COLUMN_WEIGHT, NULL);
    g_object_set(G_OBJECT(renderer), "ypad", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", COLUMN_TEXT, "weight", COLUMN_WEIGHT, NULL);
    g_object_set(G_OBJECT(renderer), "ypad", 0, "ellipsize-set", TRUE,
                 "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", COLUMN_TIME, "weight", COLUMN_WEIGHT, NULL);
    g_object_set(G_OBJECT(renderer), "ypad", 0, "xalign", 1.0, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    g_signal_connect(treeview, "row-activated",
                     G_CALLBACK(ui_playlist_widget_jump), NULL);

    g_signal_connect(treeview, "key-press-event",
                     G_CALLBACK(ui_playlist_widget_keypress_cb), NULL);

    g_signal_connect(treeview, "drag-begin",
                     G_CALLBACK(_ui_playlist_widget_drag_begin), NULL);
    g_signal_connect(treeview, "drag-data-received",
                     G_CALLBACK(_ui_playlist_widget_drag_data_received), NULL);
    g_signal_connect(treeview, "drag-end",
                     G_CALLBACK(_ui_playlist_widget_drag_end), NULL);

    g_object_set_data(G_OBJECT(treeview), "current", GINT_TO_POINTER(-1));
    g_object_set_data(G_OBJECT(treeview), "my_playlist", playlist);

    ui_playlist_widget_fill(treeview);

    return treeview;
}
