/*
 * ui_fileopener.c
 * Copyright 2007-2011 Michael Färber and John Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <gtk/gtk.h>

#include <audacious/i18n.h>
#include <audacious/drct.h>
#include <audacious/gtk-compat.h>
#include <audacious/misc.h>

#include "config.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static void filebrowser_add_files (GtkFileChooser * browser, GSList * files,
 bool_t play)
{
    Index * list = index_new ();

    for (GSList * node = files; node; node = node->next)
        index_append (list, str_get (node->data));

    if (play)
        aud_drct_pl_open_list (list);
    else
        aud_drct_pl_add_list (list, -1);

    char * path = gtk_file_chooser_get_current_folder (browser);
    if (path)
    {
        aud_set_string ("audgui", "filesel_path", path);
        g_free (path);
    }
}

static void
action_button_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *window = g_object_get_data(data, "window");
    GtkWidget *chooser = g_object_get_data(data, "chooser");
    GtkWidget *toggle = g_object_get_data(data, "toggle-button");

    GSList * files = gtk_file_chooser_get_uris ((GtkFileChooser *) chooser);

    bool_t play = GPOINTER_TO_INT (g_object_get_data (data, "play-button"));
    filebrowser_add_files ((GtkFileChooser *) chooser, files, play);

    g_slist_foreach(files, (GFunc) g_free, NULL);
    g_slist_free(files);

    if (gtk_toggle_button_get_active ((GtkToggleButton *) toggle))
        gtk_widget_destroy (window);
}

static void toggled_cb (GtkToggleButton * toggle, void * option)
{
    aud_set_bool ("audgui", (const char *) option, gtk_toggle_button_get_active (toggle));
}

static void
close_button_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void
run_filebrowser_gtk2style(bool_t play_button, bool_t show)
{
    static GtkWidget *window = NULL;
    GtkWidget *vbox, *hbox, *bbox;
    GtkWidget *chooser;
    GtkWidget *action_button, *close_button;
    GtkWidget *toggle;
    char *window_title, *toggle_text;
    gpointer action_stock, storage;

    if (!show) {
        if (window){
            gtk_widget_hide(window);
            return;
        }
        else
            return;
    }
    else {
        if (window) {
            gtk_window_present(GTK_WINDOW(window)); /* raise filebrowser */
            return;
        }
    }

    window_title = play_button ? _("Open Files") : _("Add Files");
    toggle_text = play_button ?
        _("Close dialog on Open") : _("Close dialog on Add");
    action_stock = play_button ? GTK_STOCK_OPEN : GTK_STOCK_ADD;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title(GTK_WINDOW(window), window_title);
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 450);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);

    char * path = aud_get_string ("audgui", "filesel_path");
    if (path[0])
        gtk_file_chooser_set_current_folder ((GtkFileChooser *) chooser, path);
    g_free (path);

    gtk_box_pack_start(GTK_BOX(vbox), chooser, TRUE, TRUE, 3);

    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

    const char * option = play_button ? "close_dialog_open" : "close_dialog_add";
    toggle = gtk_check_button_new_with_label(toggle_text);
    gtk_toggle_button_set_active ((GtkToggleButton *) toggle, aud_get_bool
     ("audgui", option));
    g_signal_connect (toggle, "toggled", (GCallback) toggled_cb, (void *) option);
    gtk_box_pack_start(GTK_BOX(hbox), toggle, TRUE, TRUE, 3);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(bbox), 6);
    gtk_box_pack_end(GTK_BOX(hbox), bbox, TRUE, TRUE, 3);

    close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    action_button = gtk_button_new_from_stock(action_stock);
    gtk_container_add(GTK_CONTAINER(bbox), close_button);
    gtk_container_add(GTK_CONTAINER(bbox), action_button);

    gtk_widget_set_can_default (action_button, TRUE);
    gtk_widget_grab_default (action_button);

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

    audgui_destroy_on_escape (window);
    gtk_widget_show_all (window);
}

/*
 * run_filebrowser(bool_t play_button)
 *
 * Inputs:
 *     - bool_t play_button
 *       TRUE  - open files
 *       FALSE - add files
 *
 * Outputs:
 *     - none
 */
void
audgui_run_filebrowser(bool_t play_button)
{
    run_filebrowser_gtk2style(play_button, TRUE);
}

void
audgui_hide_filebrowser(void)
{
    run_filebrowser_gtk2style(FALSE, FALSE);
}
