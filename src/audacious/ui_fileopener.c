/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
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

#include "ui_fileopener.h"

#include <glib/gi18n.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "input.h"
#include "main.h"
#include "playback.h"
#include "strings.h"

static void
filebrowser_add_files(GtkFileChooser * browser,
                      GSList * files)
{
    GSList *cur;
    gchar *ptr;
    Playlist *playlist = playlist_get_active();

    for (cur = files; cur; cur = g_slist_next(cur)) {
        gchar *filename = g_filename_to_uri((const gchar *) cur->data, NULL, NULL);

        if (vfs_file_test(cur->data, G_FILE_TEST_IS_DIR)) {
            playlist_add_dir(playlist, filename ? filename : (const gchar *) cur->data);
        } else {
            playlist_add(playlist, filename ? filename : (const gchar *) cur->data);
        }       

        g_free(filename);
    } 

    hook_call("playlist update", playlist);

    ptr = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(browser));

    g_free(cfg.filesel_path);
    cfg.filesel_path = ptr;
}

static void
action_button_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *window = g_object_get_data(data, "window");
    GtkWidget *chooser = g_object_get_data(data, "chooser");
    GtkWidget *toggle = g_object_get_data(data, "toggle-button");
    gboolean play_button;
    GSList *files;
    cfg.close_dialog_open =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));

    play_button =
        GPOINTER_TO_INT(g_object_get_data(data, "play-button"));

    files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(chooser));
    if (!files) return;

    if (play_button)
        playlist_clear(playlist_get_active());

    filebrowser_add_files(GTK_FILE_CHOOSER(chooser), files);
    g_slist_foreach(files, (GFunc) g_free, NULL);
    g_slist_free(files);

    if (play_button)
        playback_initiate();

    if (cfg.close_dialog_open)
        gtk_widget_destroy(window);
}


static void
close_button_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}

static gboolean
filebrowser_on_keypress(GtkWidget * browser,
                        GdkEventKey * event,
                        gpointer data)
{
    if (event->keyval == GDK_Escape) {
        gtk_widget_destroy(browser);
        return TRUE;
    }

    return FALSE;
}

static void
util_run_filebrowser_gtk2style(gboolean play_button, gboolean show)
{
    static GtkWidget *window = NULL;
    GtkWidget *vbox, *hbox, *bbox;
    GtkWidget *chooser;
    GtkWidget *action_button, *close_button;
    GtkWidget *toggle;
    gchar *window_title, *toggle_text;
    gpointer action_stock, storage;

    if(!show) {
        if(window){
            gtk_widget_hide(window);
            return;
        }
        else
            return;
    }
    else {
        if(window) {
            gtk_window_present(GTK_WINDOW(window)); /* raise filebrowser */
            return;
        }
    }
    
    window_title = play_button ? _("Open Files") : _("Add Files");
    toggle_text = play_button ?
        _("Close dialog on Open") : _("Close dialog on Add");
    action_stock = play_button ? GTK_STOCK_OPEN : GTK_STOCK_ADD;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), window_title);
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 450);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);
    if (cfg.filesel_path)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser),
                                            cfg.filesel_path);
    gtk_box_pack_start(GTK_BOX(vbox), chooser, TRUE, TRUE, 3);

    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

    toggle = gtk_check_button_new_with_label(toggle_text);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),
                                 cfg.close_dialog_open ? TRUE : FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), toggle, TRUE, TRUE, 3);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 6);
    gtk_box_pack_end(GTK_BOX(hbox), bbox, TRUE, TRUE, 3);

    close_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    action_button = gtk_button_new_from_stock(action_stock);
    gtk_container_add(GTK_CONTAINER(bbox), close_button);
    gtk_container_add(GTK_CONTAINER(bbox), action_button);

    /* this storage object holds several other objects which are used in the
     * callback functions
     */
    storage = g_object_new(G_TYPE_OBJECT, NULL);
    g_object_set_data(storage, "window", window);
    g_object_set_data(storage, "chooser", chooser);
    g_object_set_data(storage, "toggle-button", toggle);
    g_object_set_data(storage, "play-button", GINT_TO_POINTER(play_button));

    g_signal_connect(chooser, "file-activated",
                     G_CALLBACK(action_button_cb), storage);
    g_signal_connect(action_button, "clicked",
                     G_CALLBACK(action_button_cb), storage);
    g_signal_connect(close_button, "clicked",
                     G_CALLBACK(close_button_cb), window);
    g_signal_connect(window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &window);

    g_signal_connect(window, "key_press_event",
		     G_CALLBACK(filebrowser_on_keypress),
		     NULL);

    gtk_widget_show_all(window);
}

/*
 * Derived from Beep Media Player 0.9.6.1.
 * Which is (C) 2003 - 2006 Milosz Derezynski &c
 *
 * Although I changed it quite a bit. -nenolod
 */
static void filebrowser_changed_classic(GtkFileSelection * filesel)
{
    GList *list;
    GList *node;
    char *filename = (char *)
    gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel));
    GtkListStore *store;
    GtkTreeIter iter;

    if ((list = input_scan_dir(filename)) != NULL)
    {
        /*
         * We enter a directory that has been "hijacked" by an
         * input-plugin. This is used by the CDDA plugin
         */
        store =
            GTK_LIST_STORE(gtk_tree_view_get_model
                   (GTK_TREE_VIEW(filesel->file_list)));
        gtk_list_store_clear(store);

        node = list;
        while (node) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, node->data, -1);
            g_free(node->data);
            node = g_list_next(node);
        }

        g_list_free(list);
    }
}

static void filebrowser_entry_changed_classic(GtkEditable * entry, gpointer data)
{
    filebrowser_changed_classic(GTK_FILE_SELECTION(data));
}


static gboolean
util_filebrowser_is_dir_classic(GtkFileSelection * filesel)
{
    char *text;
    struct stat buf;
    gboolean retv = FALSE;

    text = g_strdup(gtk_file_selection_get_filename(filesel));

    if (stat(text, &buf) == 0 && S_ISDIR(buf.st_mode)) {
    /* Selected directory */
    int len = strlen(text);
    if (len > 3 && !strcmp(text + len - 4, "/../")) {
        if (len == 4)
        /* At the root already */
        *(text + len - 3) = '\0';
        else {
        char *ptr;
        *(text + len - 4) = '\0';
        ptr = strrchr(text, '/');
        *(ptr + 1) = '\0';
        }
    } else if (len > 2 && !strcmp(text + len - 3, "/./"))
        *(text + len - 2) = '\0';
    gtk_file_selection_set_filename(filesel, text);
    retv = TRUE;
    }
    g_free(text);
    return retv;
}

static void filebrowser_add_files_classic(gchar ** files,
                  GtkFileSelection * filesel)
{
    int ctr = 0;
    char *ptr;
    Playlist *playlist = playlist_get_active();


    while (files[ctr] != NULL) {
        gchar *filename = g_filename_to_uri((const gchar *) files[ctr++], NULL, NULL);
        playlist_add(playlist, filename);
        g_free(filename);
    }
    hook_call("playlist update", playlist);

    gtk_label_get(GTK_LABEL(GTK_BIN(filesel->history_pulldown)->child),
          &ptr);

    /* This will give an extra slash if the current dir is the root. */
    cfg.filesel_path = g_strconcat(ptr, "/", NULL);
}

static void filebrowser_ok_classic(GtkWidget * w, GtkWidget * filesel)
{
    gchar **files;

    if (util_filebrowser_is_dir_classic(GTK_FILE_SELECTION(filesel)))
    return;
    files = gtk_file_selection_get_selections(GTK_FILE_SELECTION(filesel));
    filebrowser_add_files_classic(files, GTK_FILE_SELECTION(filesel));
    gtk_widget_destroy(filesel);
}

static void filebrowser_play_classic(GtkWidget * w, GtkWidget * filesel)
{
    gchar **files;

    if (util_filebrowser_is_dir_classic
    (GTK_FILE_SELECTION(GTK_FILE_SELECTION(filesel))))
    return;
    playlist_clear(playlist_get_active());
    files = gtk_file_selection_get_selections(GTK_FILE_SELECTION(filesel));
    filebrowser_add_files_classic(files, GTK_FILE_SELECTION(filesel));
    gtk_widget_destroy(filesel);
    playback_initiate();
}

static void filebrowser_add_selected_files_classic(GtkWidget * w, gpointer data)
{
    gchar **files;

    GtkFileSelection *filesel = GTK_FILE_SELECTION(data);
    files = gtk_file_selection_get_selections(filesel);

    filebrowser_add_files_classic(files, filesel);
    gtk_tree_selection_unselect_all(gtk_tree_view_get_selection
                    (GTK_TREE_VIEW(filesel->file_list)));

    gtk_entry_set_text(GTK_ENTRY(filesel->selection_entry), "");
}

static void filebrowser_add_all_files_classic(GtkWidget * w, gpointer data)
{
    gchar **files;
    GtkFileSelection *filesel;

    filesel = data;
    gtk_tree_selection_select_all(gtk_tree_view_get_selection
                  (GTK_TREE_VIEW(filesel->file_list)));
    files = gtk_file_selection_get_selections(filesel);
    filebrowser_add_files_classic(files, filesel);
    gtk_tree_selection_unselect_all(gtk_tree_view_get_selection
                    (GTK_TREE_VIEW(filesel->file_list)));
    gtk_entry_set_text(GTK_ENTRY(filesel->selection_entry), "");
}

static void
util_run_filebrowser_classic(gboolean play_button, gboolean show)
{
    static GtkWidget *dialog;
    GtkWidget *button_add_selected, *button_add_all, *button_close,
    *button_add;
    char *title;

    if (!show) {
        if(dialog) {
            gtk_widget_hide(dialog);
            return;
        }
        else
            return;
    }
    else {
        if (dialog) {
            gtk_window_present(GTK_WINDOW(dialog));
            return;
        }
    }

    if (play_button)
    title = _("Play files");
    else
    title = _("Load files");

    dialog = gtk_file_selection_new(title);

    gtk_file_selection_set_select_multiple
    (GTK_FILE_SELECTION(dialog), TRUE);

    if (cfg.filesel_path)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog),
                    cfg.filesel_path);

    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(dialog));
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    gtk_widget_hide(GTK_FILE_SELECTION(dialog)->ok_button);
    gtk_widget_destroy(GTK_FILE_SELECTION(dialog)->cancel_button);

    /*
     * The mnemonics are quite unorthodox, but that should guarantee they're unique in any locale
     * plus kinda easy to use
     */
    button_add_selected =
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Add selected",
                  GTK_RESPONSE_NONE);
    gtk_button_set_use_underline(GTK_BUTTON(button_add_selected), TRUE);
    g_signal_connect(G_OBJECT(button_add_selected), "clicked",
             G_CALLBACK(filebrowser_add_selected_files_classic), dialog);

    button_add_all =
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Add all",
                  GTK_RESPONSE_NONE);
    gtk_button_set_use_underline(GTK_BUTTON(button_add_all), TRUE);
    g_signal_connect(G_OBJECT(button_add_all), "clicked",
             G_CALLBACK(filebrowser_add_all_files_classic), dialog);

    if (play_button) {
    button_add =
        gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_MEDIA_PLAY,
                  GTK_RESPONSE_NONE);
    gtk_button_set_use_stock(GTK_BUTTON(button_add), TRUE);
    g_signal_connect(G_OBJECT(button_add), "clicked",
             G_CALLBACK(filebrowser_play_classic), dialog);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(dialog)->ok_button),
             "clicked", G_CALLBACK(filebrowser_play_classic), dialog);
    } else {
    button_add =
        gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_ADD,
                  GTK_RESPONSE_NONE);
    gtk_button_set_use_stock(GTK_BUTTON(button_add), TRUE);
    g_signal_connect(G_OBJECT(button_add), "clicked",
             G_CALLBACK(filebrowser_ok_classic), dialog);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(dialog)->ok_button),
             "clicked", G_CALLBACK(filebrowser_ok_classic), dialog);
    }

    button_close =
    gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE,
                  GTK_RESPONSE_NONE);
    gtk_button_set_use_stock(GTK_BUTTON(button_close), TRUE);
    g_signal_connect_swapped(G_OBJECT(button_close), "clicked",
                 G_CALLBACK(gtk_widget_destroy),
                 G_OBJECT(dialog));

    gtk_widget_set_size_request(dialog, 600, 450);
    gtk_widget_realize(dialog);

    g_signal_connect(G_OBJECT
             (GTK_FILE_SELECTION(dialog)->history_pulldown),
             "changed", G_CALLBACK(filebrowser_entry_changed_classic),
             dialog);

    g_signal_connect(G_OBJECT(dialog), "destroy",
             G_CALLBACK(gtk_widget_destroyed), &dialog);

    filebrowser_changed_classic(GTK_FILE_SELECTION(dialog));

    gtk_widget_show(dialog);
}

/*
 * util_run_filebrowser(gboolean play_button)
 *
 * Inputs:
 *     - gboolean play_button
 *       TRUE  - open files
 *       FALSE - add files
 *
 * Outputs:
 *     - none
 *
 * Side Effects:
 *     - either a GTK1 or a GTK2 fileselector is launched
 */
void
run_filebrowser(gboolean play_button)
{
    if (!cfg.use_xmms_style_fileselector)
        util_run_filebrowser_gtk2style(play_button, TRUE);
    else
        util_run_filebrowser_classic(play_button, TRUE);
}

void
hide_filebrowser(void)
{
    if (!cfg.use_xmms_style_fileselector)
        util_run_filebrowser_gtk2style(FALSE, FALSE);
    else
        util_run_filebrowser_classic(FALSE, FALSE);
}
