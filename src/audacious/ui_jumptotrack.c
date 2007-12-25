/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gtk/gtkmessagedialog.h>

/* GDK including */
#include "platform/smartinclude.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#if defined(USE_REGEX_ONIGURUMA)
  #include <onigposix.h>
#elif defined(USE_REGEX_PCRE)
  #include <pcreposix.h>
#else
  #include <regex.h>
#endif

#include "ui_main.h"
#include "icons-stock.h"

#include "actions-mainwin.h"

#include "main.h"

#include "dnd.h"
#include "dock.h"
#include "hints.h"
#include "input.h"
#include "playback.h"
#include "playlist.h"
#include "pluginenum.h"
#include "ui_credits.h"
#include "ui_equalizer.h"
#include "ui_fileopener.h"
#include "ui_manager.h"
#include "ui_playlist.h"
#include "ui_preferences.h"
#include "ui_skinselector.h"
#include "ui_urlopener.h"
#include "strings.h"
#include "util.h"
#include "visualization.h"

#include "ui_skinned_window.h"

static GtkWidget *jump_to_track_win = NULL;
static gulong serial = 0;

static void
change_song(guint pos)
{
    if (playback_get_playing())
        playback_stop();

    playlist_set_position(playlist_get_active(), pos);
    playback_initiate();
}

void
ui_jump_to_track_hide(void)
{
    g_return_if_fail(jump_to_track_win != NULL);
    gtk_widget_hide(jump_to_track_win);
}

static void
ui_jump_to_track_jump(GtkTreeView * treeview)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    guint pos;

    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(treeview);

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &pos, -1);

    change_song(pos - 1);

    if(cfg.close_jtf_dialog)
        ui_jump_to_track_hide();
}

static void
ui_jump_to_track_toggle_cb(GtkWidget * toggle)
{
    cfg.close_jtf_dialog =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
}

static void
ui_jump_to_track_jump_cb(GtkTreeView * treeview,
                             gpointer data)
{
    ui_jump_to_track_jump(treeview);
}

static void
ui_jump_to_track_set_queue_button_label(GtkButton * button,
                                      guint pos)
{
    if (playlist_is_position_queued(playlist_get_active(), pos))
        gtk_button_set_label(button, _("Un_queue"));
    else
        gtk_button_set_label(button, _("_Queue"));
}

static void
ui_jump_to_track_queue_cb(GtkButton * button,
                              gpointer data)
{
    GtkTreeView *treeview;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    guint pos;

    treeview = GTK_TREE_VIEW(data);
    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(treeview);

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &pos, -1);

    playlist_queue_position(playlist_get_active(), (pos - 1));

    ui_jump_to_track_set_queue_button_label(button, (pos - 1));
}

static void
ui_jump_to_track_selection_changed_cb(GtkTreeSelection *treesel,
                                          gpointer data)
{
    GtkTreeView *treeview;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    guint pos;

    treeview = gtk_tree_selection_get_tree_view(treesel);
    model = gtk_tree_view_get_model(treeview);
    selection = gtk_tree_view_get_selection(treeview);

    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
        return;

    gtk_tree_model_get(model, &iter, 0, &pos, -1);

    ui_jump_to_track_set_queue_button_label(GTK_BUTTON(data), (pos - 1));
}

static gboolean
ui_jump_to_track_edit_keypress_cb(GtkWidget * object,
                 GdkEventKey * event,
                 gpointer data)
{
    switch (event->keyval) {
    case GDK_Return:
        if (gtk_im_context_filter_keypress (GTK_ENTRY (object)->im_context, event)) {
            GTK_ENTRY (object)->need_im_reset = TRUE;
            return TRUE;
        } else {
            ui_jump_to_track_jump(GTK_TREE_VIEW(data));
            return TRUE;
        }
    default:
        return FALSE;
    }
}

static gboolean
ui_jump_to_track_keypress_cb(GtkWidget * object,
                                 GdkEventKey * event,
                                 gpointer data)
{
    switch (event->keyval) {
    case GDK_Escape:
        ui_jump_to_track_hide();
        return TRUE;
    case GDK_KP_Enter:
        ui_jump_to_track_queue_cb(NULL, data);
        return TRUE;
    default:
        return FALSE;
    };

    return FALSE;
}

static gboolean
ui_jump_to_track_match(const gchar * song, GSList *regex_list)
{
    gboolean rv = TRUE;

    if ( song == NULL )
        return FALSE;

    for ( ; regex_list ; regex_list = g_slist_next(regex_list) )
    {
        regex_t *regex = regex_list->data;
        if ( regexec( regex , song , 0 , NULL , 0 ) != 0 )
        {
            rv = FALSE;
            break;
        }
    }

    return rv;
}

void
ui_jump_to_track_update(GtkWidget * widget, gpointer user_data)
{
    guint row;
    GList *playlist_glist;
    gchar *desc_buf = NULL;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    Playlist *playlist;

    GtkTreeModel *store;

    GtkTreeView *tree = GTK_TREE_VIEW(g_object_get_data(user_data, "treeview"));
    GtkEntry *edit = g_object_get_data(user_data, "edit");

    if (!jump_to_track_win)
        return;

    /* clear edit widget */
    if(edit){
        gtk_entry_set_text(edit, "\0");
    }

    store = gtk_tree_view_get_model(tree);
    gtk_list_store_clear(GTK_LIST_STORE(store));

    row = 1;
    playlist = playlist_get_active();
    for (playlist_glist = playlist->entries; playlist_glist;
         playlist_glist = g_list_next(playlist_glist)) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(playlist_glist->data);

        if (entry->title)
            desc_buf = g_strdup(entry->title);
        else {
            gchar *realfn = NULL;
            realfn = g_filename_from_uri(entry->filename, NULL, NULL);
            if (strchr(realfn ? realfn : entry->filename, '/'))
                desc_buf = str_to_utf8(strrchr(realfn ? realfn : entry->filename, '/') + 1);
            else
                desc_buf = str_to_utf8(realfn ? realfn : entry->filename);
            g_free(realfn); realfn = NULL;
        }

        gtk_list_store_append(GTK_LIST_STORE(store), &iter);
        gtk_list_store_set(GTK_LIST_STORE(store), &iter,
                           0, row, 1, desc_buf, -1);
        row++;

        g_free(desc_buf);
        desc_buf = NULL;
    }

    gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    selection = gtk_tree_view_get_selection(tree);
    gtk_tree_selection_select_iter(selection, &iter);
    serial = playlist->serial; // important. --yaz
}

static void
ui_jump_to_track_edit_cb(GtkEntry * entry, gpointer user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(user_data);
    GtkTreeSelection *selection;
    GtkTreeIter iter;

    GtkListStore *store;

    guint song_index = 0;
    gchar **words;
    GList *playlist_glist;
    Playlist *playlist;

    gboolean match = FALSE;

    GSList *regex_list = NULL, *regex_list_tmp = NULL;
    gint i = -1;

    /* Chop the key string into ' '-separated key regex-pattern strings */
    words = g_strsplit(gtk_entry_get_text(entry), " ", 0);

    /* create a list of regex using the regex-pattern strings */
    while ( words[++i] != NULL )
    {
        regex_t *regex = g_malloc(sizeof(regex_t));
    #if defined(USE_REGEX_PCRE)
        if ( regcomp( regex , words[i] , REG_NOSUB | REG_ICASE | REG_UTF8 ) == 0 )
    #else
        if ( regcomp( regex , words[i] , REG_NOSUB | REG_ICASE ) == 0 )
    #endif
            regex_list = g_slist_append( regex_list , regex );
    }

    /* FIXME: Remove the connected signals before clearing
     * (row-selected will still eventually arrive once) */
    store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
    /* detach model from treeview */
    g_object_ref( store );
    gtk_tree_view_set_model( GTK_TREE_VIEW(treeview) , NULL );

    gtk_list_store_clear(store);

    playlist = playlist_get_active();

    PLAYLIST_LOCK(playlist);

    for (playlist_glist = playlist->entries; playlist_glist;
         playlist_glist = g_list_next(playlist_glist))
    {
        PlaylistEntry *entry = PLAYLIST_ENTRY(playlist_glist->data);
        gchar *title = NULL;

        if (entry->title)
            title = g_strdup(entry->title);
        else {
            gchar *realfn = NULL;
            realfn = g_filename_from_uri(entry->filename, NULL, NULL);
            title = str_to_utf8(realfn ? realfn : entry->filename);
            g_free(realfn); realfn = NULL;
        }

        /*we are matching all the path not just the filename or title*/

        /* Compare the reg.expressions to the string - if all the
           regexp in regex_list match, add to the ListStore */

        /*
         * FIXME: The search string should be adapted to the
         * current display setting, e.g. if the user has set it to
         * "%p - %t" then build the match string like that too, or
         * even better, search for each of the tags seperatly.
         *
         * In any case the string to match should _never_ contain
         * something the user can't actually see in the playlist.
         */
        //g_print ("it passed\n");
        if (regex_list != NULL)
                match = ui_jump_to_track_match(title, regex_list);
        else
                match = TRUE;

        if (match) {
                if (entry->title)
                    title = g_strdup(entry->title);
                else {
                    gchar *realfn = NULL;
                    realfn = g_filename_from_uri(entry->filename, NULL, NULL);
                    if (strchr(realfn ? realfn : entry->filename, '/'))
                        title = str_to_utf8(strrchr(realfn ? realfn : entry->filename, '/') + 1);
                    else
                        title = str_to_utf8(realfn ? realfn : entry->filename);
                    g_free(realfn); realfn = NULL;
                }
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, song_index + 1 , 1, title, -1);
        }
        song_index++;
        g_free(title); title = NULL;
    }

    PLAYLIST_UNLOCK(playlist);

    /* attach the model again to the treeview */
    gtk_tree_view_set_model( GTK_TREE_VIEW(treeview) , GTK_TREE_MODEL(store) );
    g_object_unref( store );

    if ( regex_list != NULL )
    {
        regex_list_tmp = regex_list;
        while ( regex_list != NULL )
        {
            regex_t *regex = regex_list->data;
            regfree( regex );
            regex_list = g_slist_next(regex_list);
        }
        g_slist_free( regex_list_tmp );
    }
    g_strfreev(words);

    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
        selection = gtk_tree_view_get_selection(treeview);
        gtk_tree_selection_select_iter(selection, &iter);
    }
}

static gboolean
ui_jump_to_track_fill(gpointer treeview)
{
    GList *playlist_glist;
    Playlist *playlist;
    gchar *desc_buf = NULL;
    guint row;
    GtkTreeIter iter;
    GtkListStore *jtf_store = (GtkListStore*)gtk_tree_view_get_model( GTK_TREE_VIEW(treeview) );

    /* detach model from treeview before fill */
    g_object_ref(jtf_store);
    gtk_tree_view_set_model( GTK_TREE_VIEW(treeview), NULL );

    gtk_list_store_clear(jtf_store);

    row = 1;
    playlist = playlist_get_active();

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
                desc_buf = str_to_utf8(strrchr(realfn ? realfn : entry->filename, '/') + 1);
            else
                desc_buf = str_to_utf8(realfn ? realfn : entry->filename);
            g_free(realfn); realfn = NULL;
        }

        gtk_list_store_append(GTK_LIST_STORE(jtf_store), &iter);
        gtk_list_store_set(GTK_LIST_STORE(jtf_store), &iter,
                           0, row, 1, desc_buf, -1);
        row++;

        g_free(desc_buf);
        desc_buf = NULL;
    }
    PLAYLIST_UNLOCK(playlist);

    /* attach liststore to treeview */
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(jtf_store));
    g_object_unref(jtf_store);
    serial = playlist->serial;
    return FALSE;
}

static gboolean
watchdog(gpointer storage)
{
    GtkWidget *widget;
    Playlist *playlist = playlist_get_active();

    if(serial == playlist->serial)
        return TRUE;

    widget = g_object_get_data(storage, "widget");
    ui_jump_to_track_update(widget, storage);
    return TRUE;
}

void
ui_jump_to_track(void)
{
    GtkWidget *scrollwin;
    GtkWidget *vbox, *bbox, *sep;
    GtkWidget *toggle;
    GtkWidget *jump, *queue, *close;
    GtkWidget *rescan;
    GtkWidget *search_label, *hbox;
    static GtkWidget *edit;

    GtkWidget *treeview = NULL;
    GtkListStore *jtf_store;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    gpointer storage;

    if (jump_to_track_win) {
        gtk_window_present(GTK_WINDOW(jump_to_track_win));
        gtk_entry_set_text(edit, "");
        gtk_widget_grab_focus(edit);
        return;
    }

    #if defined(USE_REGEX_ONIGURUMA)
    /* set encoding for Oniguruma regex to UTF-8 */
    reg_set_encoding( REG_POSIX_ENCODING_UTF8 );
    onig_set_default_syntax( ONIG_SYNTAX_POSIX_BASIC );
    #endif

    jump_to_track_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(jump_to_track_win),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_title(GTK_WINDOW(jump_to_track_win), _("Jump to Track"));

    gtk_window_set_position(GTK_WINDOW(jump_to_track_win), GTK_WIN_POS_CENTER);
    g_signal_connect(jump_to_track_win, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &jump_to_track_win);

    gtk_container_border_width(GTK_CONTAINER(jump_to_track_win), 10);
    gtk_window_set_default_size(GTK_WINDOW(jump_to_track_win), 550, 350);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(jump_to_track_win), vbox);

    jtf_store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(jtf_store));
    g_object_unref(jtf_store);

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);

    column = gtk_tree_view_column_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 0, NULL);
    gtk_tree_view_column_set_spacing(column, 4);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_attributes(column, renderer, "text", 1, NULL);
    gtk_tree_view_column_set_spacing(column, 4);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview), 1);

    g_signal_connect(treeview, "row-activated",
                     G_CALLBACK(ui_jump_to_track_jump), NULL);

    hbox = gtk_hbox_new(FALSE, 3);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

    search_label = gtk_label_new(_("Filter: "));
    gtk_label_set_markup_with_mnemonic(GTK_LABEL(search_label), _("_Filter:"));
    gtk_box_pack_start(GTK_BOX(hbox), search_label, FALSE, FALSE, 0);

    edit = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(edit), TRUE);
    gtk_label_set_mnemonic_widget(GTK_LABEL(search_label), edit);
    g_signal_connect(edit, "changed",
                     G_CALLBACK(ui_jump_to_track_edit_cb), treeview);

    g_signal_connect(edit, "key_press_event",
                     G_CALLBACK(ui_jump_to_track_edit_keypress_cb), treeview);

    g_signal_connect(jump_to_track_win, "key_press_event",
                     G_CALLBACK(ui_jump_to_track_keypress_cb), treeview);

    gtk_box_pack_start(GTK_BOX(hbox), edit, TRUE, TRUE, 3);

    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin),
                                        GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);

    sep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 0);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    /* close dialog toggle */
    toggle = gtk_check_button_new_with_label(_("Close on Jump"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),
                                 cfg.close_jtf_dialog ? TRUE : FALSE);
    gtk_box_pack_start(GTK_BOX(bbox), toggle, FALSE, FALSE, 0);
    g_signal_connect(toggle, "clicked", 
                     G_CALLBACK(ui_jump_to_track_toggle_cb),
                     toggle);


    queue = gtk_button_new_with_mnemonic(_("_Queue"));
    gtk_box_pack_start(GTK_BOX(bbox), queue, FALSE, FALSE, 0);
    GTK_WIDGET_SET_FLAGS(queue, GTK_CAN_DEFAULT);
    g_signal_connect(queue, "clicked", 
                     G_CALLBACK(ui_jump_to_track_queue_cb),
                     treeview);
    g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed",
                     G_CALLBACK(ui_jump_to_track_selection_changed_cb),
                     queue);

    rescan = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
    gtk_box_pack_start(GTK_BOX(bbox), rescan, FALSE, FALSE, 0);

    /* pack to container */
    storage = g_object_new(G_TYPE_OBJECT, NULL);
    g_object_set_data(storage, "widget", rescan);
    g_object_set_data(storage, "treeview", treeview);
    g_object_set_data(storage, "edit", edit);

    g_signal_connect(rescan, "clicked",
                     G_CALLBACK(ui_jump_to_track_update), storage);

    GTK_WIDGET_SET_FLAGS(rescan, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(rescan);

    jump = gtk_button_new_from_stock(GTK_STOCK_JUMP_TO);
    gtk_box_pack_start(GTK_BOX(bbox), jump, FALSE, FALSE, 0);

    g_signal_connect_swapped(jump, "clicked",
                             G_CALLBACK(ui_jump_to_track_jump_cb),
                             treeview);

    GTK_WIDGET_SET_FLAGS(jump, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(jump);

    close = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(bbox), close, FALSE, FALSE, 0);
    g_signal_connect_swapped(close, "clicked",
                             G_CALLBACK(gtk_widget_hide),
                             jump_to_track_win); // just hide --yaz
    GTK_WIDGET_SET_FLAGS(close, GTK_CAN_DEFAULT);

    g_timeout_add(100, (GSourceFunc)ui_jump_to_track_fill, treeview);
    g_timeout_add(500, (GSourceFunc)watchdog, storage);

    gtk_widget_show_all(jump_to_track_win);
    gtk_widget_grab_focus(edit);
}
